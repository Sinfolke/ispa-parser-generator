"""Focused Python target runtime regressions: python3 tests/python_runtime.py."""
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'parser/stdlibs/target'))
import ispa_std as s
from python.dfa import run_tdfa
from python.model import assign_attr, assign_item


class RuntimeTests(unittest.TestCase):
    def test_semantic_callback_arity_preserves_signature_semantics(self):
        import functools
        import inspect
        from python.dfa import _semantic_accepts_cursor
        def six(a, b, c, d, e, f): pass
        def seven(a, b, c, d, e, f, cursor): pass
        def variadic(a, b, c, d, e, *args, **kwargs): pass
        def keyword(a, b, c, d, e, f, *, cursor): pass
        class Receiver:
            def method(self, a, b, c, d, e, f): pass
            def __call__(self, a, b, c, d, e, f, cursor): pass
        @functools.wraps(seven)
        def wrapped(*args, **kwargs): pass
        def custom(*args): pass
        custom.__signature__ = inspect.signature(seven)
        for callback in (six, seven, variadic, keyword, Receiver().method,
                         Receiver(), wrapped, custom, functools.partial(seven, 1)):
            self.assertEqual(_semantic_accepts_cursor(callback),
                             len(inspect.signature(callback).parameters) >= 7)

    def test_lexer_lookup_with_and_without_debug_tables(self):
        for instrumented in (False, True):
            lexer = s.Lexer('a')
            pos = s.Position('a')
            token = object()
            def accept(index, captures, start, length, line, cursor):
                return -1, token
            args = ([], {}, pos) if instrumented else (pos,)
            self.assertIs(lexer.lookup([[1], [2]], [0] * 256, [], s.TdfaLayout(0, 0), accept, *args), token)
            self.assertEqual(pos.index, 1)

    def test_recovery_distinguishes_token_names_from_literal_text(self):
        lexer = s.Lexer('NEWLINE\n')
        lexer.tokens = [s.RawToken.create(0, lexer.source, 7, 1, 0, 'NEWLINE'),
                        s.RawToken.create(8, lexer.source, 0, 2, 1, 'synthetic')]
        lexer._finished = True
        cursor = lexer.lazy_iterator()
        names = lambda kind: ('tokens.ID', 'tokens.NEWLINE')[kind]
        ctx = s.FailContext(cursor, None, cursor.copy(), 'statement', 'expression',
                            s.Recovery.State(), lexer.source, 0, names)
        self.assertTrue(ctx.at('NEWLINE'))
        self.assertFalse(ctx.at_token('NEWLINE'))
        ctx.sync_tokens(ctx.token_boundary('NEWLINE'))
        self.assertEqual(cursor.index, 1)
        self.assertTrue(ctx.at_token('NEWLINE'))
        self.assertFalse(ctx.at('NEWLINE'))
        self.assertTrue(ctx.at_token('tokens::NEWLINE'))

    def test_arithmetic_uses_operand_types_and_truncates_toward_zero(self):
        for left, right, quotient, remainder in [(-5, 2, -2, -1), (5, -2, -2, 1), (-5, -2, 2, -1)]:
            self.assertEqual(s.divide(left, right), quotient)
            self.assertEqual(s.modulo(left, right), remainder)
        self.assertEqual(s.divide(5.0, 2), 2.5)
        self.assertEqual(s.divide(2**80 + 1, 3), (2**80 + 1) // 3)
        with self.assertRaises(ZeroDivisionError):
            s.divide(1, 0)
        obj = type('Box', (), {'value': -5})()
        self.assertEqual(assign_attr(obj, 'value', 2, '/'), -2)
        values = [-5]
        self.assertEqual(assign_item(values, 0, 2, '%'), -1)
        self.assertEqual(values, [-1])

    def test_counter_scopes_restore_inherited_bindings(self):
        parser = s.LLParser()
        parser.counters['outer'] = 3
        scope = parser.counter_scope()
        parser.bind_counter('inner', [1, 2])
        nested = parser.counter_scope()
        parser.bind_counter('nested', [1])
        with self.assertRaises(RuntimeError):
            parser.bind_counter('inner', [1])
        nested.close()
        self.assertEqual(parser.counters, {'outer': 3, 'inner': 2})
        scope.close()
        scope.close()
        self.assertEqual(parser.counters, {'outer': 3})

    def test_templates_and_type_tags_keep_distinct_cached_identities(self):
        @s.template
        def box(target):
            return type('Box', (), {'target': target})
        self.assertIs(box(int), box(int))
        self.assertIsNot(box(int), box(str))
        self.assertIs(s.TypeTag.specialize(box(int)), s.TypeTag.specialize(box(int)))
        self.assertIsNot(s.TypeTag.specialize(box(int)), s.TypeTag.specialize(box(str)))

    def test_dfa_trace_keeps_transition_provenance(self):
        debug = s.DFADebug('[a-z]', 'tokens.WORD', 2, 3, 4, 5, 6, 7, 'a')
        events = []
        pos = s.Position('a')
        with patch('ispa_debug.enabled', return_value=True), patch('ispa_debug.event', side_effect=lambda *a, **kw: events.append((a, kw))):
            token, _ = run_tdfa('a', pos, [[1], [2]], [0] * 256, [], s.TdfaLayout(0, 0),
                                lambda *_args: (-1, 'ok'), debug_array=[debug], debug_index={0: {0: 0}})
        self.assertEqual(token, 'ok')
        args, data = events[0]
        self.assertEqual(args, ('dfa_transition', 'tokens.WORD'))
        self.assertEqual((data['member'], data['rule_run'], data['offset'], data['length']), ('[a-z]', 5, 6, 7))


if __name__ == '__main__':
    unittest.main()
