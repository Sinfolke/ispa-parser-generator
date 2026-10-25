import ispa_std as s
from ispa_std.model import RawToken
A = s.ABSENT
ID, PLUS = 0, 1
class Lex(s.Lexer):
    def makeToken(self, pos):
        c = pos.peek()
        if c == ' ':
            pos.advance(); return None
        st = pos.index
        if c == '+':
            pos.advance(); return RawToken.create(st, self.source, 1, 1, PLUS, '+')
        while pos.peek().isdigit(): pos.advance()
        if pos.index == st: pos.advance()
        return RawToken.create(st, self.source, pos.index-st, 1, ID, int(self.source[st:pos.index] or 0))

def run(parser_cls, text):
    p = parser_cls()
    lex = Lex(text)
    return p.main(lex.lazy_iterator())

class Det(s.LRParser):
    # E -> E + T | T ; T -> ID      rules: 0:E->E+T 1:E->T 2:T->ID
    lr_k = staticmethod(lambda: 1)
    @staticmethod
    def lr_action(st, w):
        return {(0,ID):4,(1,A):0,(1,PLUS):5,(2,A):-2,(2,PLUS):-2,(3,A):-3,(3,PLUS):-3,
                (4,ID):4,(5,A):-1,(5,PLUS):-1}.get((st,w[0]), s.INVALID_ACTION)
    lr_goto = staticmethod(lambda st,l: {(0,0):1,(0,1):2,(4,1):5}[(st,l)])
    lr_length = staticmethod(lambda r: [3,1,1][r])
    lr_lhs = staticmethod(lambda r: [0,0,1][r])
    lr_shift = staticmethod(lambda t: t.value)
    @staticmethod
    def lr_reduce(r, rhs): return [lambda x: x[0]+x[2], lambda x: x[0], lambda x: x[0]][r](rhs)
    def main(self, pos): return self.lr_parse(Det, pos)

r = run(Det, "1 + 22 + 3")
assert r.status and r.node == 26, r
try: run(Det, "1 + + 3"); assert False
except RuntimeError as e: print("error ok:", e)

class Amb(s.LRGeneralizedParser):
    # E -> E + E | ID ; rules 0:E->E+E 1:E->ID ; S4 has a shift/reduce conflict on PLUS
    lr_k = staticmethod(lambda: 1)
    @staticmethod
    def lr_action(st, w):
        t = {(0,ID):3,(1,A):0,(1,PLUS):3+0,(2,A):-2,(2,PLUS):-2,(3,ID):3,(4,A):-1}.get((st,w[0]))
        # states: 0 start,1 E.,2 ID.,3 E+.E,4 E+E.
        tab = {(0,ID):3,(1,A):0,(1,PLUS):4,(2,A):-2,(2,PLUS):-2,(3,ID):3,(4,A):-1}
        if (st,w[0]) == (4,PLUS): return s.CONFLICT_ACTION
        return tab.get((st,w[0]), s.INVALID_ACTION)
    lr_conflict = staticmethod(lambda st,w: [4, -1])   # shift -> state 3 ; reduce rule 0
    lr_goto = staticmethod(lambda st,l: {(0,0):1,(3,0):4}[(st,l)])
    lr_length = staticmethod(lambda r: [3,1][r])
    lr_lhs = staticmethod(lambda r: 0)
    lr_shift = staticmethod(lambda t: t.value)
    @staticmethod
    def lr_reduce(r, rhs): return rhs[0]+rhs[2] if r == 0 else rhs[0]
    lr_rule_label = staticmethod(lambda r: ["E -> E + E", "E -> ID"][r])
    def main(self, pos): return self.lr_parse_generalized(Amb, pos)
