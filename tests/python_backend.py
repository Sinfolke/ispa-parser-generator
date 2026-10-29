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
        library = runtime.parent / "python"
        (grammar / "langapi.py").write_text((library / "langapi.py").read_text())
        (grammar / "stdfun.py").write_text((library / "stdfun.py").read_text())
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
        (grammar / "templates.isc").write_text(
            "A:'a'; B:'b'; leaf: @ A {@}; outer<T>: @ T {@};\n"
            "WRAP<T>: '[' @ ${T} ']' {@}; forwarded<U>: @ outer<U> {@};\n"
            "main: @ forwarded<leaf> @ outer<B> @ WRAP<A> @ WRAP<B> @{first, second, third, fourth};\n"
        )
        (grammar / "counts.isc").write_text(
            "A:'a'; B:'b'; main: @ A=x ':' @ body @ B=2 '!' @{first, body, last};\n"
            "body: @ item=x {@}; item: @ A {@};\n"
        )
        (grammar / "helpers.py").write_text('''from langapi import size, push, extend, clear
def semantic_collections() -> int:
    values: list[int] = []
    other: list[int] = []
    push(values, 1)
    push(other, 2)
    push(other, 3)
    extend(values, other)
    clear(other)
    push(values, 4)
    return size(values) + size(other)
def sorting() -> int:
    xs = [1, 3, 2]
    xs.sort(reverse=True)
    return xs[0]
def negate(value: int) -> int:
    return -value
def keyed() -> int:
    xs = [1, 3, 2]
    xs.sort(key=negate)
    return xs[0]
def updating() -> int:
    d = {'a': 1}
    d.update({'a': 3}, b=4)
    d.update(c=5)
    return d['a'] + d['b'] + d['c']
def division(a: float, b: float) -> float:
    return a / b
class Box:
    def __init__(self, value: int):
        self.value = value
    def get(self) -> int:
        return self.value
''')
        (grammar / "helpers.isc").write_text("import py helpers; import py stdfun; main: 'x';\n")

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

        generate("Helpers", "--dir", grammar, grammar / "helpers.isc")
        execute('''import Helpers
h = Helpers.python.helpers
assert h.sorting() == 3 and h.keyed() == 3
assert h.semantic_collections() == 4
assert Helpers.python.stdfun.range(4) == [0, 1, 2, 3]
assert Helpers.python.stdfun.reversed([1, 2, 3]) == [3, 2, 1]
assert h.updating() == 12
assert h.division(5.0, 2.0) == 2.5
assert h.Box(7).get() == 7
''')

        generate("Templates", grammar / "templates.isc")
        execute('''import Templates as t
tree = t.Parser().parse('ab[a][b]').data()
assert isinstance(tree.first.data(), t.Types.forwarded(t.Types.leaf))
assert isinstance(tree.second.data(), t.Types.outer(t.Types.B))
assert isinstance(tree.third.data(), t.Types.WRAP(t.Types.A))
assert isinstance(tree.fourth.data(), t.Types.WRAP(t.Types.B))
assert type(tree.third.data()) is not type(tree.fourth.data())
assert tree.third.data().value.startpos == 3
assert tree.fourth.data().value.startpos == 6
''')

        generate("Counts", grammar / "counts.isc")
        execute('''import Counts
for source, count in [('aa:aabb!', 2), ('a:a b b!', 1), (':bb!', 0)]:
    parser = Counts.Parser()
    tree = parser.parse(source).data()
    assert len(tree.first) == count
    assert len(tree.body.data().value) == count
    assert len(tree.last) == 2
    assert not parser.getErrors()
    assert not parser.counters
for source in ['aa:abb!', 'a:a b!', 'a:a bbb!']:
    parser = Counts.Parser()
    try:
        parser.parse(source)
    except RuntimeError:
        pass
    else:
        assert parser.getErrors(), source
    assert not parser.counters
''')

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

    print("Python backend: parsing, helpers, templates, counted repetitions, dynamic patterns, and external imports passed")


if __name__ == "__main__":
    main()
