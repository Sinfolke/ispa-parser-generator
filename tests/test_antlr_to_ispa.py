import ast
import contextlib
import hashlib
import io
import json
from pathlib import Path
import sys
import tarfile
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'cli/pkg'))
from antlr_to_ispa.actions import translate, TARGETS
from antlr_to_ispa.analysis import load, analyze
from antlr_to_ispa.cli import main
from antlr_to_ispa.emitter import Emitter
from antlr_to_ispa.model import walk
from antlr_to_ispa.parser import parse
from antlr_to_ispa.scanner import GrammarSyntaxError
from ispa_pkg.tools import install_tool, release_platform
from ispa_pkg.cli import PackageError, main as package_main


SAMPLES = {
    'Java': 'int x = 1; x = x + 2;',
    'Cpp': 'int x = 1; x = x + 2;',
    'CSharp': 'int x = 1; x = x + 2;',
    'JavaScript': 'let x = 1; x = x + 2;',
    'TypeScript': 'let x = 1; x = x + 2;',
    'Go': 'x := 1; x = x + 2;',
    'Rust': 'let mut x = 1; x = x + 2;',
    'Swift': 'var x = 1; x = x + 2;',
    'Dart': 'var x = 1; x = x + 2;',
    'PHP': '$x = 1; $x = $x + 2;',
    'Python3': 'x = 1\nx = x + 2',
}


class ConversionTests(unittest.TestCase):
    def test_uppercase_tokens_and_collision_safe_references(self):
        output, report = Emitter(parse("grammar G; main: Name NAME Name_2; Name: Letter+; NAME: 'x'; Name_2: 'y'; fragment Letter: [a-z];")).emit()
        self.assertEqual(report['token_names']['NAME'], 'NAME')
        self.assertEqual(len(set(report['token_names'].values())), 4)
        self.assertTrue(all(n == n.upper() for n in report['token_names'].values()))
        self.assertIn('main: ' + ' '.join(report['token_names'][n] for n in ('Name', 'NAME', 'Name_2')) + ';', output)
        self.assertIn('LETTER: @ ([a-z])', output)

    def test_mixed_case_token_action_bindings(self):
        output, report = Emitter(parse('grammar G; main: Number {System.out.println($Number.text);}; Number: [0-9]+;')).emit()
        self.assertIn('match @ NUMBER -> antlr_Number;', output)
        self.assertIn('print(antlr_Number.value.value)', output)
        self.assertTrue(report['actions'][0]['integrated'])

    def test_recursion_and_lexer_conversion(self):
        grammar = parse("grammar Calc; expr: expr '+' INT | INT; INT: [0-9]+; WS: [ \\t\\r\\n]+ -> skip;")
        output, report = Emitter(grammar).emit()
        self.assertIn("expr: (expr '+' INT) | (INT);", output)
        self.assertIn('#[skip]\nWS:', output)
        self.assertFalse(report['requires_changes'])

    def test_opaque_actions_and_source_locations(self):
        grammar = parse('grammar G;\nr: ID { if (true) { x = "};"; /* } */ } };\nID: [a-z]+;', 'G.g4')
        action = next(n for n in walk(grammar.rules[0].body) if n.kind == 'action')
        self.assertIn('/* } */', action.value)
        self.assertEqual(action.span.line, 2)
        self.assertEqual(action.span.source, 'G.g4')

    def test_headers_modes_commands_and_predicates(self):
        grammar = parse('lexer grammar G; options {language=Rust;} tokens { OTHER } @members { int x; } A: [a-z]+ -> pushMode(M); mode M; B: {ok()}? . -> type(OTHER), popMode;')
        output, report = Emitter(grammar).emit()
        self.assertEqual(report['language'], 'Rust')
        self.assertTrue(report['requires_changes'])
        self.assertIn('lexer-mode', {d['code'] for d in report['diagnostics']})
        self.assertEqual(report['actions'][0]['source'], 'ok()')
        self.assertIn('Changes required', output)

    def test_import_and_token_vocab(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / 'Tokens.g4').write_text("lexer grammar Tokens; ID: [a-z]+;")
            (root / 'Base.g4').write_text('parser grammar Base; item: ID;')
            (root / 'Main.g4').write_text('parser grammar Main; options {tokenVocab=Tokens;} import Base; main: item;')
            grammar = load(root / 'Main.g4')
            self.assertEqual({r.name for r in grammar.rules}, {'main', 'item', 'ID'})
            self.assertFalse(analyze(grammar))

    def test_missing_dependency_and_cycles_warn(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'G.g4'
            path.write_text('grammar G; import G, Missing; main: UNKNOWN;')
            codes = [d.code for d in analyze(load(path))]
            self.assertEqual(codes.count('dependency'), 2)
            self.assertIn('undefined-rule', codes)

    def test_nullable_repeat(self):
        diagnostics = analyze(parse("grammar G; main: empty*; empty: 'x'?;"))
        self.assertIn('nullable-repeat', [d.code for d in diagnostics])

    def test_invalid_source_is_error(self):
        for source in ('grammar G; a: (ID;', 'grammar G; a: { missing;', "grammar G; a: 'unterminated;"):
            with self.subTest(source=source), self.assertRaises(GrammarSyntaxError):
                parse(source)

    def test_cli_warnings_are_nonfatal_and_preserve_sources(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'G.g4'
            path.write_text('grammar G; main: {externalCall();} EOF;')
            stderr = io.StringIO()
            with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(stderr):
                self.assertEqual(main([str(path)]), 0)
            report = json.loads(path.with_suffix('.migration.json').read_text())
            self.assertEqual(report['sources'][str(path.resolve())], path.read_text())
            self.assertTrue(report['requires_changes'])
            self.assertIn('Changes required before running', stderr.getvalue())

    def test_direct_action_is_integrated(self):
        output, report = Emitter(parse("grammar G; main: ID {int x = 1; x = x + 2;}; ID: [a-z]+;")).emit()
        self.assertIn('match @ ID -> antlr_ID;', output)
        self.assertIn('x = (x + 2)', output)
        self.assertTrue(report['actions'][0]['integrated'])

    def test_token_attribute_binding(self):
        output, report = Emitter(parse("grammar G; main: n=INT {int x = $n.int;}; INT: [0-9]+;")).emit()
        self.assertIn('match @ INT -> antlr_n;', output)
        self.assertIn('int(antlr_n.value.value)', output)
        self.assertTrue(report['actions'][0]['integrated'])

    def test_unlabelled_token_attribute(self):
        output, report = Emitter(parse("grammar G; main: INT {int x = $INT.int;}; INT: [0-9]+;")).emit()
        self.assertIn('int(antlr_INT.value.value)', output)
        self.assertTrue(report['actions'][0]['integrated'])

    def test_negation_quantifier_binding(self):
        grammar = parse('lexer grammar G; TEXT: ~[x]+;')
        expr = grammar.rules[0].body.children[0]
        self.assertEqual(expr.kind, 'repeat')
        self.assertEqual(expr.children[0].kind, 'not')

    def test_charset_open_bracket_and_raw_action_strings(self):
        grammar = parse('lexer grammar G; OPEN: [[];')
        self.assertEqual(grammar.rules[0].body.children[0].value, '[[]')
        for action in ('auto s = R"tag( } " { )tag";', 'let s = r#" } " { "#;'):
            parsed = parse('grammar G; main: {' + action + '};')
            self.assertEqual(parsed.rules[0].body.children[0].value, action)


class ActionTests(unittest.TestCase):
    def test_loops_in_every_language(self):
        samples = {
            'Java': 'int x = 0; while(x < 3) {x += 1;}',
            'Cpp': 'int x = 0; while(x < 3) {x += 1;}',
            'CSharp': 'int x = 0; while(x < 3) {x += 1;}',
            'JavaScript': 'let x = 0; while(x < 3) {x += 1;}',
            'TypeScript': 'let x = 0; while(x < 3) {x += 1;}',
            'Go': 'x := 0; for x < 3 {x += 1}',
            'Rust': 'let mut x = 0; while x < 3 {x += 1;}',
            'Swift': 'var x = 0; while x < 3 {x += 1}',
            'Dart': 'var x = 0; while(x < 3) {x += 1;}',
            'PHP': '$x = 0; while($x < 3) {$x += 1;}',
            'Python3': 'x = 0\nwhile x < 3:\n    x += 1',
        }
        for language, source in samples.items():
            with self.subTest(language=language):
                result = translate(source, language)
                self.assertFalse(result.reason, result.reason)
                scope = {}
                exec(result.python, {}, scope)
                self.assertEqual(scope['x'], 3)

    def test_common_call_mappings(self):
        samples = {'Java': 'System.out.println(3);', 'CSharp': 'Console.WriteLine(3);',
                   'JavaScript': 'console.log(3);', 'TypeScript': 'console.log(3);',
                   'Go': 'fmt.Println(3);', 'Swift': 'print(3);', 'Dart': 'print(3);',
                   'Rust': 'println!("hello");', 'PHP': 'echo 3;', 'Python3': 'print(3)'}
        for language, source in samples.items():
            with self.subTest(language=language):
                result = translate(source, language)
                self.assertFalse(result.reason, result.reason)
                output = io.StringIO()
                with contextlib.redirect_stdout(output):
                    exec(result.python, {})
                self.assertEqual(output.getvalue().strip(), 'hello' if language == 'Rust' else '3')
        self.assertFalse(translate('int x = std::stoi("3");', 'Cpp').reason)

    def test_all_target_adapters(self):
        self.assertEqual(set(SAMPLES), set(TARGETS))
        for language, source in SAMPLES.items():
            with self.subTest(language=language):
                result = translate(source, language)
                self.assertFalse(result.reason, result.reason)
                scope = {}
                exec(compile(ast.parse(result.python), '<test>', 'exec'), {}, scope)
                self.assertEqual(scope['x'], 3)

    def test_unsupported_action_is_atomic(self):
        result = translate('int x = 1; unknownLibrary.run();', 'Java')
        self.assertTrue(result.reason)
        self.assertEqual(result.python, '')

    def test_unknown_reference_and_division_are_not_guessed(self):
        self.assertTrue(translate('x = $missing.text;', 'Java').reason)
        self.assertTrue(translate('int x = 7 / 2;', 'Java').reason)

    def test_literals_and_comments_do_not_rewrite_attributes(self):
        result = translate('String x = "$not_an_attribute";', 'Java')
        # The string translator conservatively rejects interpolated-looking strings.
        self.assertNotIn('unresolved ANTLR reference', result.reason)

    def test_conditional_and_predicate(self):
        result = translate('int x = 1; if (x > 0) { x = 3; } else { x = 4; }', 'Java')
        self.assertFalse(result.reason)
        scope = {}
        exec(result.python, {}, scope)
        self.assertEqual(scope['x'], 3)
        predicate = translate('x > 0', 'Java', predicate=True)
        self.assertFalse(predicate.reason)
        self.assertTrue(eval(predicate.python, {}, {'x': 1}))


class ToolInstallTests(unittest.TestCase):
    def archive(self, name='antlr-to-ispa'):
        data = io.BytesIO()
        with tarfile.open(fileobj=data, mode='w:gz') as archive:
            entry = tarfile.TarInfo(name)
            content = b'compiled-test-binary'
            entry.size = len(content)
            archive.addfile(entry, io.BytesIO(content))
        return data.getvalue()

    def release(self, data, digest=None):
        return json.dumps({'tag_name': 'v1', 'assets': [{
            'name': 'antlr-to-ispa-linux-x64.tar.gz',
            'browser_download_url': 'https://example.invalid/tool',
            'digest': digest or 'sha256:' + hashlib.sha256(data).hexdigest(),
        }]}).encode()

    def test_install_compiled_asset_and_no_overwrite(self):
        data = self.archive()
        with tempfile.TemporaryDirectory() as temp, patch('ispa_pkg.tools.release_platform', return_value='linux-x64'):
            root = Path(temp) / 'libraries'
            with patch('ispa_pkg.cli.request', side_effect=[self.release(data), data]), contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(package_main(['--root', str(root), 'tool', 'antlr-to-ispa']), 0)
            binary = root.parent / 'bin/antlr-to-ispa'
            self.assertEqual(binary.read_bytes(), b'compiled-test-binary')
            with patch('ispa_pkg.cli.request', side_effect=[self.release(data), data]), self.assertRaises(PackageError):
                install_tool('antlr-to-ispa', root)

    def test_checksum_and_traversal_rejected(self):
        with tempfile.TemporaryDirectory() as temp, patch('ispa_pkg.tools.release_platform', return_value='linux-x64'):
            for data, digest in ((self.archive(), 'sha256:bad'), (self.archive('../escape'), None)):
                with patch('ispa_pkg.cli.request', side_effect=[self.release(data, digest), data]), self.assertRaises(PackageError):
                    install_tool('antlr-to-ispa', Path(temp) / 'libraries')
            self.assertFalse((Path(temp) / 'bin/antlr-to-ispa').exists())

    def test_platform_mapping(self):
        with patch('platform.system', return_value='Darwin'), patch('platform.machine', return_value='arm64'):
            self.assertEqual(release_platform(), 'macos-arm64')
        with patch('platform.system', return_value='Linux'), patch('platform.machine', return_value='aarch64'), self.assertRaises(PackageError):
            release_platform()


if __name__ == '__main__':
    unittest.main()
