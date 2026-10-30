"""Test helpers from a real local std package installation."""
import importlib.util
import os
import subprocess
import shutil
from pathlib import Path
import sys
import tempfile
import types
import unittest
from unittest.mock import patch

REPO = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO / 'cli/pkg'))
from ispa_pkg.cli import install

class StandardLibraryTests(unittest.TestCase):
    @unittest.skipUnless(os.environ.get('ISPA'), 'set ISPA to test generated Python code')
    def test_generated_library(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            install('std', {'path': str(REPO)}, root / 'installed')
            library = root / 'installed/std/ispalib'
            root = root / 'consumer'
            root.mkdir()
            for folder in ('grammar', 'python'):
                for source in (library / folder).iterdir():
                    if source.name in ('numbers.isc', 'whitespace.isc', 'stdmath.py', 'stdseq.py', 'stdtext.py', 'langapi.py'):
                        shutil.copy(source, root / source.name)
            grammar = root / 'smoke.isc'
            grammar.write_text('import {INTEGER, HEX_INTEGER, BINARY_INTEGER, OCTAL_INTEGER, DECIMAL} from std.numbers;\n'
                               'import py stdmath; import py stdseq; import py stdtext;\n'
                               'main: INTEGER | HEX_INTEGER | BINARY_INTEGER | OCTAL_INTEGER | DECIMAL;\n')
            result = subprocess.run([os.environ['ISPA'], '-l', 'python', '-a', 'LL',
                                     '--no-incremental', '--dir', str(root), str(grammar),
                                     '-o', str(root / 'Generated')], cwd=root, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            code = """import sys
sys.path.insert(0, sys.argv[1])
import Generated as g
assert g.python.stdmath.gcd(-54, 24) == 6
assert g.python.stdseq.unique([3, 1, 3, 2]) == [3, 1, 2]
assert g.python.stdtext.join(['a', '', 'b'], ',') == 'a,,b'
assert g.python.stdtext.contains('abc', 'bc')
for source in ('42', '-12', '0xFF', '0b101', '0o77', '1.25e-3', '+2e10'):
    parser = g.Parser()
    parser.parse(source)
    assert not parser.getErrors(), source
"""
            result = subprocess.run([sys.executable, '-B', '-c', code,
                                     str(REPO / 'parser/stdlibs/target')], cwd=root,
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_installed_helpers(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            install('std', {'path': str(REPO)}, root)
            library = root / 'std/ispalib'
            # langapi is a compiler intrinsic module, not a CPython runtime.
            shim = types.ModuleType('langapi')
            shim.size = len
            shim.push = lambda xs, x: xs.append(x)
            def load(name):
                spec = importlib.util.spec_from_file_location(name, library / 'python' / (name + '.py'))
                module = importlib.util.module_from_spec(spec)
                spec.loader.exec_module(module)
                return module
            with patch.dict(sys.modules, langapi=shim):
                math, seq, text = map(load, ('stdmath', 'stdseq', 'stdtext'))
            self.assertEqual(math.gcd(-54, 24), 6)
            self.assertEqual(math.gcd(0, 0), 0)
            self.assertEqual([math.sign(x) for x in (-9, 0, 9)], [-1, 0, 1])
            self.assertEqual([math.clamp(x, 0, 5) for x in (-1, 3, 8)], [0, 3, 5])
            self.assertFalse(math.in_range(5, 0, 5))
            values = [3, 1, 3, 2]
            self.assertEqual(seq.unique(values), [3, 1, 2])
            self.assertEqual(seq.count(values, 3), 2)
            self.assertEqual(seq.find(values, 7), -1)
            self.assertEqual(seq.find(values, 3), 0)
            self.assertEqual(seq.take(values, -1), [])
            self.assertEqual(seq.take(values, 20), values)
            self.assertEqual(seq.drop(values, 2), [3, 2])
            self.assertEqual(seq.drop(values, -1), values)
            self.assertEqual(seq.drop(values, 20), [])
            self.assertEqual(seq.product([]), 1)
            self.assertEqual(seq.product(values), 18)
            self.assertEqual(values, [3, 1, 3, 2])
            for source in ('', 'abc', 'ababa'):
                for needle in ('', 'a', 'ab', 'ba', 'abcd'):
                    self.assertEqual(text.starts_with(source, needle), source.startswith(needle))
                    self.assertEqual(text.ends_with(source, needle), source.endswith(needle))
                    self.assertEqual(text.find(source, needle), source.find(needle))
                    self.assertEqual(text.contains(source, needle), needle in source)
            self.assertEqual(text.repeat('xy', 3), 'xyxyxy')
            self.assertEqual(text.repeat('x', -1), '')
            self.assertEqual(text.join([], ','), '')
            self.assertEqual(text.join(['a', '', 'b'], ','), 'a,,b')
            for name in ('numbers', 'whitespace'):
                self.assertTrue((library / 'grammar' / (name + '.isc')).is_file())

if __name__ == '__main__':
    unittest.main()
