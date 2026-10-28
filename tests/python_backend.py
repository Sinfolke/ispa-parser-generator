"""Generate and execute LL parsers with the Python backend.

Run: python3 tests/python_backend.py --ispa cmake-build-debug/ispa
"""

import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument("--ispa", required=True, type=Path)
    args = cli.parse_args()
    binary = args.ispa.resolve()
    runtime = Path(__file__).resolve().parents[1] / "parser/stdlibs/target"

    with tempfile.TemporaryDirectory(prefix="ispa-python-") as temporary:
        root = Path(temporary)
        grammar = root / "grammar"
        grammar.mkdir()
        (grammar / "simple.isc").write_text("WORD: @ [a-z]+ {@};\nmain: @ WORD {@};\n")
        (grammar / "foreign.isc").write_text(
            "module expression -> value;\nWORD: @ [a-z]+ {@};\nvalue: @ WORD {@};\n"
        )
        (grammar / "host.isc").write_text(
            "module host -> main;\nimport extern foreign {value} from expression;\n"
            "main: '[' @ value ']' {@};\n"
        )
        (grammar / "optional_rule.isc").write_text(
            "AT: '@';\nCLASS: 'class';\n"
            "decorator: AT;\n"
            "decorators: decorator+;\n"
            "main: decorators? CLASS;\n"
        )
        (grammar / "dynamic.isc").write_text(
            "REPEAT: &q [a-z]+ ':' $q;\n"
            "QUOTED: '#' &q [a-z]+ ':' !($q | '!')* $q;\n"
            "NEGATED: '~' &q [a-z]+ ':' !$q;\n"
            "main: REPEAT | QUOTED | NEGATED;\n"
        )

        def generate(name, *options):
            result = subprocess.run(
                [str(binary), "-l", "python", "-a", "LL", "--debug", "false",
                 "--no-nfa-debug", "-o", root / name, *map(str, options)],
                cwd=root, text=True, capture_output=True,
            )
            if result.returncode:
                raise AssertionError(result.stdout + result.stderr)

        def execute(code):
            env = os.environ.copy()
            env["PYTHONPATH"] = os.pathsep.join((str(root), str(runtime)))
            env["PYTHONDONTWRITEBYTECODE"] = "1"
            subprocess.run([sys.executable, "-c", code], cwd=root, env=env, check=True)

        generate("Simple", grammar / "simple.isc")
        execute("import Simple; assert Simple.Parser().parse('  hello  ').data().value.data().value == 'hello'")

        generate("OptionalRule", grammar / "optional_rule.isc")
        execute("""
import OptionalRule
from python.parser import ParseError
for source in ('class', '@class'):
    parser = OptionalRule.Parser()
    parser.parse(source)
    assert not parser.errors
try:
    OptionalRule.Parser().parse('@')
except ParseError:
    pass
else:
    raise AssertionError('a started optional rule must not hide a missing CLASS')
""")

        generate("Dynamic", grammar / "dynamic.isc")
        execute("""
import Dynamic
from python.lexer import LexError
for source in ('abc:abc', '#abc:yyyabc', '#abc:abc', '~abc:x'):
    assert Dynamic.Parser().parse(source)
for source in ('abc:abd', '#abc:!abc', '~abc:abc'):
    try:
        Dynamic.Parser().parse(source)
    except (LexError, RuntimeError):
        pass
    else:
        raise AssertionError(f'dynamic pattern accepted {source!r}')
""")

        generate("Foreign", grammar / "foreign.isc")
        generate("Host", "--dep", "foreign", root / "Foreign", grammar / "host.isc")
        execute("""
import Foreign, Host
for source, start in (('[hello]', 1), ('[ hello ]', 2)):
    node = Host.Parser().parse(source).data().value.data().value
    assert node.startpos == start and node.length == 5
    assert isinstance(node.data(), Foreign.FlatTypes.modules_expression_value)
    assert node.data().value.data().value == 'hello'
""")

        automatic = root / "automatic"
        generate("automatic/Host", "--no-dep", "--dir", grammar, grammar / "host.isc")
        env = os.environ.copy()
        env["PYTHONPATH"] = os.pathsep.join((str(automatic), str(runtime)))
        env["PYTHONDONTWRITEBYTECODE"] = "1"
        subprocess.run([sys.executable, "-c", """
import Host, foreign
node = Host.Parser().parse('[ hello ]').data().value.data().value
assert isinstance(node.data(), foreign.FlatTypes.modules_expression_value)
assert node.data().value.data().value == 'hello'
"""], cwd=automatic, env=env, check=True)

        (grammar / "host_token.isc").write_text(
            "module host -> main;\nimport extern foreign {WORD} from expression;\n"
            "main: '[' @ WORD ']' {@};\n"
        )
        generate("HostToken", "--dep", "foreign", root / "Foreign", grammar / "host_token.isc")
        execute("""
import Foreign, HostToken
node = HostToken.Parser().parse('[hello]').data().value.data().value
assert isinstance(node.data(), Foreign.FlatTypes.modules_expression_WORD)
assert node.data().value == 'hello'
""")

    print("Python backend: simple parse, optional rule, dynamic patterns, external rule/token --dep, and --no-dep passed")


if __name__ == "__main__":
    main()
