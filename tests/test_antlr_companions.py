"""Behavior and generator integration for companion-source conversion."""
import contextlib
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'cli/pkg'))
from antlr_to_ispa.companions import convert_companions
from antlr_to_ispa.cli import main
from antlr_to_ispa.emitter import Emitter
from antlr_to_ispa.parser import parse


SOURCES = {
    'Helpers.java': 'class Helpers { public static int twice(int x) { return x * 2; } }',
    'helpers.cpp': 'int twice(int x) { return x * 2; }',
    'Helpers.cs': 'class Helpers { public static int twice(int x) { return x * 2; } }',
    'helpers.js': 'export function twice(x) { return x * 2; }',
    'helpers.ts': 'export function twice(x: number): number { return x * 2; }',
    'helpers.go': 'package helpers\nfunc twice(x int) int { return x * 2 }',
    'helpers.rs': 'pub fn twice(x: i32) -> i32 { x * 2 }',
    'helpers.swift': 'func twice(_ x: Int) -> Int { return x * 2 }',
    'helpers.dart': 'int twice(int x) { return x * 2; }',
    'helpers.php': '<?php function twice($x) { return $x * 2; }',
    'helpers.py': 'def twice(x: int) -> int:\n    return x * 2\n',
}


class CompanionTests(unittest.TestCase):
    def test_compilation_units_in_all_languages(self):
        for filename, source in SOURCES.items():
            with self.subTest(filename=filename), tempfile.TemporaryDirectory() as temp:
                path = Path(temp) / filename
                path.write_text(source)
                converted = convert_companions([path])
                unit = converted.units[0]
                self.assertFalse(unit.omitted, unit.omitted)
                scope = {}
                exec(compile(unit.python, filename, 'exec'), scope)
                name = 'Helpers_twice' if path.suffix in ('.java', '.cs') else 'twice'
                self.assertEqual(scope[name](21), 42)
                self.assertIn('import py antlr_helpers;', converted.imports)

    def test_local_calls_are_linked_and_imported_from_grammar(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'Helpers.java'
            path.write_text('class Helpers { static int twice(int x) { return x * 2; } static int answer(int x) { return twice(x); } }')
            companions = convert_companions([path])
            output, report = Emitter(parse("grammar G; main: 'x' {System.out.println(Helpers.answer(21));};"), companions=companions).emit()
            self.assertIn('import py antlr_helpers;', output)
            self.assertIn('print(antlr_helpers.Helpers_answer(21))', output)
            self.assertTrue(report['actions'][0]['integrated'])
            scope = {}
            exec(companions.units[0].python, scope)
            self.assertEqual(scope['Helpers_answer'](21), 42)

    def test_unknown_library_and_transitive_callers_warn(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'Helpers.java'
            path.write_text('import external.Library;\nclass Helpers { static int bad(int x) { return Library.run(x); } static int caller(int x) { return bad(x); } static int good(int x) { return x + 1; } }')
            result = convert_companions([path])
            unit = result.units[0]
            self.assertIn('companion-import', [d.code for d in result.diagnostics])
            self.assertEqual(len(unit.omitted), 3)
            self.assertIn('def Helpers_good', unit.python)
            self.assertNotIn('def Helpers_bad', unit.python)
            self.assertNotIn('def Helpers_caller', unit.python)
            self.assertNotIn('Helpers.bad', result.symbols)
            self.assertTrue(any(d.span.line == 2 for d in result.diagnostics))

    def test_linking_across_files(self):
        with tempfile.TemporaryDirectory() as temp:
            first, second = Path(temp) / 'a.cpp', Path(temp) / 'b.cpp'
            first.write_text('int twice(int x) { return x * 2; }')
            second.write_text('int answer(int x) { return twice(x); }')
            result = convert_companions([first, second])
            self.assertIn('import antlr_a', result.files['antlr_b.py'])
            self.assertIn('return antlr_a.twice(x)', result.files['antlr_b.py'])

    def test_python_imports_to_supplied_files_are_rewritten(self):
        with tempfile.TemporaryDirectory() as temp:
            first, second = Path(temp) / 'a.py', Path(temp) / 'b.py'
            first.write_text('def twice(x: int) -> int:\n    return x * 2\n')
            second.write_text('import a\ndef answer(x: int) -> int:\n    return a.twice(x)\n')
            result = convert_companions([first, second])
            self.assertIn('import antlr_a as a', result.files['antlr_b.py'])
            self.assertNotIn('companion-import', [d.code for d in result.diagnostics])

    def test_missing_globals_warn_and_locals_shadow_other_files(self):
        with tempfile.TemporaryDirectory() as temp:
            first, second = Path(temp) / 'a.cpp', Path(temp) / 'b.cpp'
            first.write_text('int count = 100;')
            second.write_text('int local() { int count = 2; return count; } int missing() { return external_value; }')
            result = convert_companions([first, second])
            self.assertIn('unresolved global/library names: external_value', str(result.units[1].omitted))
            scope = {}
            exec(result.units[1].python, scope)
            self.assertEqual(scope['local'](), 2)
            self.assertNotIn('missing', result.symbols)

    def test_unicode_spans_and_python_diagnostic_locations(self):
        with tempfile.TemporaryDirectory() as temp:
            foreign = Path(temp) / 'helpers.cpp'
            foreign.write_text('// café\nint bad() { return unknown(); }', encoding='utf-8')
            result = convert_companions([foreign])
            diagnostic = next(d for d in result.diagnostics if d.code == 'companion-code')
            self.assertEqual(diagnostic.span.line, 2)
            self.assertEqual(diagnostic.span.start, len('// café\n'))
            python = Path(temp) / 'helpers.py'
            python.write_text('# first line\nimport unknown\n\ndef foo(x):\n    return x\n')
            result = convert_companions([python])
            self.assertEqual(next(d.span.line for d in result.diagnostics if d.code == 'companion-import'), 2)
            self.assertEqual(next(d.span.line for d in result.diagnostics if d.code == 'companion-types'), 4)

    def test_output_cannot_overwrite_source_or_report(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            grammar, code = root / 'G.g4', root / 'Helpers.java'
            grammar.write_text("grammar G; main: 'x';")
            code.write_text(SOURCES['Helpers.java'])
            with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(main([str(grammar), '--code', str(code), '--report', str(code)]), 1)
                self.assertEqual(main([str(grammar), '--code', str(code), '--report', str(root / 'antlr_helpers.py')]), 1)
            self.assertEqual(code.read_text(), SOURCES['Helpers.java'])
            self.assertFalse(grammar.with_suffix('.isc').exists())

    def test_parse_errors_and_instance_state_are_preserved(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'Helpers.java'
            path.write_text('class Helpers { int state = 1; int get() { return state; } static int good(int x) { return x; } }')
            result = convert_companions([path])
            self.assertEqual(len(result.units[0].omitted), 2)
            self.assertIn('def Helpers_good', result.units[0].python)
            path.write_text('class Helpers { broken')
            result = convert_companions([path])
            self.assertIn('Tree-sitter', result.diagnostics[0].message)

    def test_overloads_are_not_silently_merged(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'helpers.cpp'
            path.write_text('int f(int x) { return x; } double f(double x) { return x; }')
            result = convert_companions([path])
            self.assertNotIn('f', result.symbols)
            self.assertEqual(len(result.units[0].omitted), 2)

    def test_cli_report_imports_and_existing_file_protection(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            grammar, code = root / 'G.g4', root / 'Helpers.java'
            grammar.write_text("grammar G; main: 'x' {System.out.println(Helpers.twice(21));};")
            code.write_text(SOURCES['Helpers.java'])
            stderr = io.StringIO()
            with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(stderr):
                self.assertEqual(main([str(grammar), '--code', str(code)]), 0)
            self.assertIn('import py antlr_helpers;', grammar.with_suffix('.isc').read_text())
            report = json.loads(grammar.with_suffix('.migration.json').read_text())
            self.assertEqual(report['sources'][str(code.resolve())], code.read_text())
            self.assertIn('Changes required', stderr.getvalue())
            helper = root / 'antlr_helpers.py'
            helper.write_text('# user code\n')
            with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(main([str(grammar), '--code', str(code)]), 1)
            self.assertEqual(helper.read_text(), '# user code\n')

    def test_python_dependencies_and_missing_annotations_warn(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'helpers.py'
            path.write_text('import third_party\ndef f(x):\n    return third_party.run(x)\n')
            result = convert_companions([path])
            self.assertIn('import third_party', result.units[0].python)
            self.assertTrue({'companion-import', 'companion-types'} <= {d.code for d in result.diagnostics})


GENERATOR = Path(os.environ.get('ISPA_GENERATOR', str(ROOT / 'cmake-build-release/ispa')))


@unittest.skipUnless(GENERATOR.is_file(), 'requires built ISPA generator')
class CompanionGeneratedTests(unittest.TestCase):
    def test_generated_parser_calls_converted_helper(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            grammar, code = root / 'G.g4', root / 'Helpers.java'
            grammar.write_text("grammar G; main: 'x' {System.out.println(Helpers.twice(21));};")
            code.write_text(SOURCES['Helpers.java'])
            with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(main([str(grammar), '--code', str(code)]), 0)
            generated = root / 'generated/Parser'
            command = [str(GENERATOR), '-l', 'python', '-a', 'LL', '--no-incremental', str(grammar.with_suffix('.isc')), '-o', str(generated)]
            build = subprocess.run(command, capture_output=True, text=True, timeout=120)
            self.assertEqual(build.returncode, 0, build.stdout + build.stderr)
            program = f'import sys; sys.path[:0]={repr([str(generated.parent), str(ROOT / "parser/stdlibs/target")])}; import Parser; Parser.Parser().parse("x")'
            run = subprocess.run([sys.executable, '-c', program], capture_output=True, text=True, timeout=30)
            self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
            self.assertEqual(run.stdout.strip(), '42')


if __name__ == '__main__':
    unittest.main()
