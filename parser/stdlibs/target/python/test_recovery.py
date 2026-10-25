"""Recovery runtime tests (run: `python test_recovery.py` with this package importable as ispa_std)."""
import ispa_std as s
from ispa_std import Recovery as R
from ispa_std.model import RawToken, MatchResult, Node

ID, EQ, SEMI, ERR, LBRACE, RBRACE = 0, 1, 2, 3, 4, 5
KINDS = {'=': EQ, ';': SEMI, '{': LBRACE, '}': RBRACE}


class Lex(s.Lexer):
    def makeToken(self, pos):
        c = pos.peek()
        if c.isspace():
            pos.advance()
            return None
        st = pos.index
        if c.isalnum():
            while pos.peek().isalnum():
                pos.advance()
            return RawToken.create(st, self.source, pos.index - st, 1, ID, self.source[st:pos.index])
        pos.advance()
        return RawToken.create(st, self.source, 1, 1, KINDS.get(c, ERR), c)


def H(rule, expected, continuation=(), follow=(), policy=(True, True, True, 64)):
    return R.handler((expected,) if isinstance(expected, int) else expected, continuation, follow,
                     (), policy, rule, 'token')


class P(s.LLParser):
    lexer_type = Lex
    NO_REPAIR = (False, False, False, 64)
    use_context = False

    def program(self, pos):
        cur = pos.copy()
        guard = self.recovery_frame('program')
        try:
            self.recovery_at([(ID,)], 'statement', 'stmt')
            items = self.range_optional(cur, 'stmt', lambda c: self.stmt(c))
            node = Node()
            node.set_data([i.data() for i in items])
            return MatchResult(True, node, cur)
        finally:
            guard.close()

    def stmt(self, pos):
        cur = pos.copy()
        out = []
        guard = self.recovery_frame('stmt')
        try:
            policy = self.NO_REPAIR if self.use_context else (True, True, True, 64)
            self.recovery_at([] if self.use_context else [(ID,)], 'identifier', 'ID')
            a = self.required(cur, ID, None, H('stmt', ID, (EQ,), (SEMI,), policy))
            out.append(a.data() if a else None)
            if self.unwinding():
                return MatchResult(True, _node(out), cur)
            self.recovery_at([(EQ,)], "'='", 'EQ')
            self.required(cur, EQ, None, H('stmt', EQ, (ID,), (SEMI,)))
            if self.unwinding():
                return MatchResult(True, _node(out), cur)
            self.recovery_at([] if self.use_context else [(ID,)], 'identifier', 'ID')
            b = self.required(cur, ID, None, H('stmt', ID, (SEMI,), (SEMI,), policy))
            if self.unwinding():
                return MatchResult(True, _node(out), cur)
            out.append(b.data() if b else None)
            self.recovery_at([(SEMI,)], "';'", 'SEMI')
            self.required(cur, SEMI, None, H('stmt', SEMI, (ID,), (ID,)))
            return MatchResult(True, _node(out), cur)
        finally:
            guard.close()


def _node(value):
    n = Node()
    n.set_data(value)
    return n


def parse(text, **attrs):
    p = P()
    for k, v in attrs.items():
        setattr(p, k, v)
    tree = p.parse(text, p.program)
    return p, tree.data() if isinstance(tree, Node) else tree


def modes(p):
    return [d.decision.mode for d in p.getErrors()]


# 1. clean input: no diagnostics
p, t = parse('a = b; c = d;')
assert t == [['a', 'b'], ['c', 'd']] and not p.getErrors() and not p.getContextErrors(), (t, p.getErrors())

# 2. missing '=': Insert (nothing consumed)
p, t = parse('a b; c = d;')
assert t == [['a', 'b'], ['c', 'd']], t
assert modes(p) == [R.Mode.Insert], modes(p)
assert "expected token" in p.getErrors()[0].message and 'inserted missing token' in p.getErrors()[0].message

# 3. extra '=': Delete, then the real token matches
p, t = parse('a = = b; c = d;')
assert t == [['a', 'b'], ['c', 'd']] and modes(p) == [R.Mode.Delete], (t, modes(p))

# 4. missing identifier before ';' (token is in FOLLOW): Insert, ';' still matched
p, t = parse('a = ; c = d;')
assert t == [['a', None], ['c', 'd']] and modes(p) == [R.Mode.Insert], (t, modes(p))

# 5. garbage skipped up to a boundary (panic), statement completes
p, t = parse('a = @ @ ; c = d;')
assert t[1] == ['c', 'd'] and modes(p) == [R.Mode.PanicUntil], (t, modes(p))
assert p.getErrors()[0].decision.skipped == 2

# 6. context recovery: innermost frame cannot repair, skip to what the enclosing frame claims, unwind
p, t = parse('a = ; c = d;', use_context=True)
assert t == [['a'], ['c', 'd']], t
ctx = p.getContextErrors()
assert len(ctx) == 1 and ctx[0].kind == R.ContextKind.Mismatch and ctx[0].resume_rule == 'program', ctx
assert "resuming at 'stmt' of 'program'" in ctx[0].message and ctx[0].skipped == 1, ctx[0].message
assert not p.frames and p._unwind == 0 and p.recovery_state.anchors == []

# 7. error budget: recovery aborts instead of looping forever
p = P()
p.recovery_state.max_errors = 1
lexer = Lex('a b; c d; e f;')
try:
    p.parse(lexer, p.program)
except RuntimeError:
    pass
assert len(p.getErrors()) == 1 and p.getErrors()[0].decision.mode == R.Mode.Abort, modes(p)

# 8. legacy callers (no recovery handler) keep the old behaviour: ParseError, diagnostics recorded
class Legacy(s.LLParser):
    lexer_type = Lex
    def main(self, pos):
        cur = pos.copy()
        self.required(cur, ID)
        self.required(cur, EQ)
        return MatchResult(True, Node(), cur)
q = Legacy()
try:
    q.parse('a b')
    assert False
except s.ParseError as e:
    assert e.offset == 1
assert q.getErrors() and q.errors

# 9. recover_choice: unambiguous FIRST token wins, shared tokens do not
class Choice(s.LLParser):
    lexer_type = Lex
c = Choice()
c.lexer = Lex('@ b')
pos = c.lexer.lazy_iterator()
# alternatives: (first-set, interior-set) per alternative
assert c.recover_choice(pos, 'r', 'x or y', [(ID,), (), (EQ,), ()]) == 0 and pos.index == 1
assert c.getErrors()[-1].decision.mode == R.Mode.Synchronize and c.getErrors()[-1].decision.skipped == 1
c.lexer = Lex('b'); pos = c.lexer.lazy_iterator()
assert c.recover_choice(pos, 'r', 'x or y', [(ID,), (), (ID,), ()]) == -1       # ambiguous

# 10. Plan-level recover(): deletion needs the next token to be expected
st = R.State()
lx = Lex('; b'); cur = lx.lazy_iterator()
d = R.recover(cur, st, R.Plan('r', expected=[ID], continuation=[EQ], follow=[]))
assert d.mode == R.Mode.Delete and cur.index == 1 and st.total_skipped == 1

# 11. scan_balanced: boundaries only count at depth 0, strings/comments are opaque
text = 'f(a, "x,y") # , no\n + [1,2] , rest'
end, exhausted = R.scan_balanced(text, 0, [','], 100)
assert text[end:] == ', rest' and not exhausted, (text[end:], exhausted)
assert R.scan_balanced('a(b;c);d', 0, [';'], 100)[0] == 3      # ';' stops even inside brackets (as in C++)
assert R.scan_balanced('abc"unterminated', 0, [';'], 100) == (16, True)
assert R.scan_balanced('a b c d', 0, [';'], 3) == (3, True)
assert R.decode('1, 2 ,3') == [1, 2, 3]

# 12. FailContext: custom diagnostics, token-level sync, requested decision
lx = Lex('x @ @ ; y'); cur = lx.lazy_iterator(); cur.advance()
st = R.State()
ctx = s.FailContext(cur, None, cur.copy(), 'stmt', 'expression', st, lx.source, 0)
before = cur.copy()
ctx.error('bad expression', 'E001')
ctx.note('while parsing assignment')
ctx.sync_tokens(';')
decision = ctx.finish(before)
assert cur.peek().kind == SEMI and decision.mode == R.Mode.Retry and decision.skipped == 2
assert st.diagnostics[0].code == 'E001' and st.diagnostics[0].notes == ['while parsing assignment']
assert st.diagnostics[0].decision.mode == R.Mode.Retry and st.total_skipped == 2
try:
    ctx.error('x', source_offset=1)
    assert False
except ValueError:
    pass

# 13. custom_failure through required(): the user block decides
class Custom(s.LLParser):
    lexer_type = Lex
    def main(self, pos):
        cur = pos.copy()
        def action(p, state):
            ctx = self.fail_context(p, None, cur, 'main', 'identifier')
            before = p.copy()
            ctx.error('custom failure', 'C1')
            ctx.skip()
            return self.custom_failure_decision(before, p, ctx)
        node = self.required(cur, ID, None, s.custom_failure(action))
        return MatchResult(True, node, cur)
cu = Custom()
tree = cu.parse('= a', cu.main)
# skip() without insert() => Retry: the loop re-matches and finds `a`
assert tree.data() == 'a' and [d.code for d in cu.getErrors()] == ['C1']
assert cu.getErrors()[0].decision.mode == R.Mode.Retry and cu.getErrors()[0].decision.skipped == 1

# 14. ctx.at / collect_braced
lx = Lex('{ a { b } c } d'); cur = lx.lazy_iterator(); cur.advance()
rc = s.RuleContext(cur, None, cur.copy(), 'blk')
assert rc.at('a') and rc.at('{', 1) and not rc.at('b')
body = rc.collect_braced()
assert [t.text() for t in body] == ['a', '{', 'b', '}', 'c'] and cur.peek().text() == 'd'

print('recovery: all tests passed')
