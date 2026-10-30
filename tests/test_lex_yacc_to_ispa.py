import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'cli/pkg'))
from lex_yacc_to_ispa.converter import convert
from lex_yacc_to_ispa.bison_cli import main

FLEX = r'''%option noyywrap
DIGIT [0-9]
%%
{DIGIT}+ { return NUMBER; }
"+" return PLUS;
[ \t\n]+ ;
%%
'''
BISON = '''%token NUMBER PLUS "+"
%left PLUS
%%
expr: NUMBER | expr "+" NUMBER { $$ = $1 + $3; };
optional: %empty | expr;
%%
'''

class ConversionTests(unittest.TestCase):
    def test_flex_definitions_tokens_and_skip(self):
        text, report = convert(FLEX, 'flex')
        self.assertIn('NUMBER: @ (([0-9]) +) {@};', text)
        self.assertIn("PLUS: @ ('+') {@};", text)
        self.assertIn('#[skip]', text)
        self.assertEqual(report['source'], FLEX)
        self.assertEqual(len(report['actions']), 3)

    def test_bison_alias_empty_and_action(self):
        text, report = convert(BISON, 'bison')
        self.assertIn('expr: (NUMBER) | (expr PLUS NUMBER);', text)
        self.assertIn('optional: ((expr))?;', text)
        self.assertIn('$$ = $1 + $3;', report['actions'][0]['source'])
        self.assertTrue(report['requires_changes'])

    def test_malformed_input(self):
        for source in ('no sections', '%%\nexpr: NUMBER', '%%\nexpr: { unclosed'):
            with self.assertRaises(ValueError):
                convert(source, 'bison')

    def test_cli_and_input_collision(self):
        with tempfile.TemporaryDirectory() as temp:
            source = Path(temp) / 'example.y'
            source.write_text(BISON)
            self.assertEqual(main([str(source)]), 0)
            self.assertIn('expr:', source.with_suffix('.isc').read_text())
            self.assertEqual(json.loads(source.with_suffix('.migration.json').read_text())['source'], BISON)
            with self.assertRaises(SystemExit):
                main([str(source), '-o', str(source)])
            self.assertEqual(source.read_text(), BISON)

if __name__ == '__main__':
    unittest.main()
