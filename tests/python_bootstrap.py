"""Regenerate and exercise the Python grammar's Python target bootstrap.

Run: python3 tests/python_bootstrap.py --ispa cmake-build-release/ispa
"""

import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument('--ispa', required=True, type=Path)
    args = cli.parse_args()
    root = Path(__file__).resolve().parents[1]
    python_root = root / 'parser' / 'python'
    with tempfile.TemporaryDirectory(prefix='ispa-python-bootstrap-') as temporary:
        output = Path(temporary)
        subprocess.run([
            str(args.ispa.resolve()), '-l', 'python', '-a', 'LL',
            '--no-incremental', '--dir', str(python_root / 'grammar'),
            str(python_root / 'entry.isc'), '-o', str(output / 'Parser'),
        ], cwd=root, check=True, capture_output=True, text=True)
        env = os.environ.copy()
        env['PYTHONPATH'] = os.pathsep.join((str(output), str(python_root),
                                           str(root / 'parser' / 'stdlibs' / 'target')))
        env['PYTHONDONTWRITEBYTECODE'] = '1'
        subprocess.run([sys.executable, '-c', """
from pathlib import Path
from Parser import Parser, Types
from python.parser import ParseError

cases = [
    (Path('parser/python/tests/cls.py').read_text(), 1),
    (Path('parser/python/tests/grammar_parse_test_small.py').read_text(), 2),
    ('class C:\\n    pass\\n', 1),
    ('@decorator\\nclass C:\\n    pass\\n', 1),
    ('@decorator\\ndef f():\\n    pass\\n', 1),
    ('class C:\\n    # comment\\n    value = "text"\\n', 1),
    ('class C:\\n\\n    pass\\n', 1),
]
for source, expected_count in cases:
    parser = Parser()
    tree = parser.parse(source)
    assert len(tree.data().value.data().value) == expected_count
    assert not parser.errors
layout_source = 'class C:\\n    x = (\\n        1\\n    )\\n'
lexer = Parser.lexer_type(layout_source)
tokens = lexer.makeTokens()
assert sum(token.holds(Types.modules.tokens.INDENT) for token in tokens) == 1
assert sum(token.holds(Types.modules.tokens.DEDENT) for token in tokens) == 1
assert sum(token.holds(Types.modules.tokens.NEWLINE) for token in tokens) == 2
Parser().parse(layout_source)
for source in ('x =\\ny = 2\\n', 'x = 1 +\\ny = 2\\n',
               'x = 1 unexpected\\ny = 2\\n', 'x = f(1, +)\\ny = 2\\n'):
    parser = Parser()
    tree = parser.parse(source)
    assert len(tree.data().value.data().value) == 2, 'recovery lost the following statement'
    assert parser.getErrors(), 'malformed Python must retain its diagnostic'
parser = Parser()
try:
    parser.parse('class C\\n    pass\\n')
except ParseError:
    pass
else:
    assert parser.getErrors(), 'invalid class header must report a diagnostic after recovery'
"""], cwd=root, env=env, check=True)
    print('Python grammar bootstrap passed')


if __name__ == '__main__':
    main()
