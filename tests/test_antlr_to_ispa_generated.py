"""Integration checks against the repository's real ISPA generator/runtime."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'cli/pkg'))
from antlr_to_ispa.parser import parse
from antlr_to_ispa.emitter import Emitter

GENERATOR = Path(os.environ.get('ISPA_GENERATOR', str(ROOT / 'cmake-build-release/ispa')))


@unittest.skipUnless(GENERATOR.is_file(), 'set ISPA_GENERATOR to a built ISPA executable')
class GeneratedTests(unittest.TestCase):
    def generate_and_run(self, grammar, program):
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp) / 'Parser'
            subprocess.run([str(GENERATOR), '-l', 'python', '-a', 'LL', '--no-incremental', str(grammar), '-o', str(output)], check=True, capture_output=True, text=True)
            setup = f'import sys; sys.path[:0] = {repr([temp, str(ROOT / "parser/stdlibs/target")])}; import Parser; '
            return subprocess.run([sys.executable, '-c', setup + program], check=True, capture_output=True, text=True)

    def test_metagrammar_parses_actions_modes_and_labels(self):
        samples = [
            'grammar G; options {language=Java;} main: ID {if (true) { x = 1; }}; ID: [a-z]+;',
            "lexer grammar G; A: 'a'..'z'+ -> skip; mode M; B: .;",
            'grammar G; main: left=ID | ID #Other; ID: [a-z]+;',
        ]
        self.generate_and_run(ROOT / 'cli/pkg/antlr_to_ispa/grammar/antlr.isc',
                              f'[(Parser.Parser().parse(s)) for s in {samples!r}]')

    def test_converted_alternatives_and_actions_run(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'output.isc'
            text, _ = Emitter(parse("grammar G; main: 'a' 'b' | 'c' 'd';")).emit()
            path.write_text(text)
            self.generate_and_run(path, 'Parser.Parser().parse("ab"); Parser.Parser().parse("cd")')
            text, _ = Emitter(parse("grammar G; main: 'a' | ;")).emit()
            path.write_text(text)
            self.generate_and_run(path, 'Parser.Parser().parse(""); Parser.Parser().parse("a")')
            text, report = Emitter(parse('grammar G; main: n=Number {System.out.println($n.text);}; Number: [0-9]+;')).emit()
            self.assertTrue(report['actions'][0]['integrated'])
            path.write_text(text)
            result = self.generate_and_run(path, 'Parser.Parser().parse("123")')
            self.assertEqual(result.stdout.strip(), '123')


if __name__ == '__main__':
    unittest.main()
