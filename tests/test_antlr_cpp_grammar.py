"""Opt-in regression against an independently cloned ANTLR C++14 grammar.

ANTLR_CPP_GRAMMAR=/path/to/grammars-v4/cpp/CPP14Parser.g4 python -m unittest tests/test_antlr_cpp_grammar.py -v
"""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'cli/pkg'))
from antlr_to_ispa.analysis import load
from antlr_to_ispa.emitter import Emitter


@unittest.skipUnless(os.environ.get('ANTLR_CPP_GRAMMAR'), 'set ANTLR_CPP_GRAMMAR to the cloned CPP14Parser.g4')
class AntlrCppGrammarTests(unittest.TestCase):
    def test_convert_real_cpp_grammar(self):
        grammar = load(Path(os.environ['ANTLR_CPP_GRAMMAR']))
        self.assertGreater(len(grammar.rules), 300)
        output, report = Emitter(grammar).emit()
        self.assertIn('translationUnit:', output)
        self.assertIn('(postfixExpression LEFTBRACKET', output)
        self.assertIn('IsPureSpecifierAllowed', str(report['actions']))
        self.assertTrue(report['requires_changes'])
        self.assertFalse({'undefined-rule', 'dependency', 'duplicate-rule'} & {d['code'] for d in report['diagnostics']})
        generator = Path(os.environ.get('ISPA_GENERATOR', str(ROOT / 'cmake-build-release/ispa')))
        if generator.is_file():
            with tempfile.TemporaryDirectory() as temporary:
                path = Path(temporary) / 'CPP14.isc'
                path.write_text(output, encoding='utf-8')
                checked = subprocess.run([str(generator), '-l', 'python', '--check-only', str(path)], capture_output=True, text=True, timeout=120)
                self.assertEqual(checked.returncode, 0, checked.stdout + checked.stderr)


if __name__ == '__main__':
    unittest.main()
