import pytest

from ispa_std.lr import LRGrammar, LRParser, LR_INVALID, LR_CONFLICT, ABSENT
from ispa_std.lr_conflicts import LRGeneralizedParser
from helpers import ListIt, toks

N, PLUS = 1, 2


class Sum(LRGrammar):
    """E -> E '+' n | n"""

    def lr_k(self):
        return 1

    def lr_action(self, state, window):
        t = window[0]
        if state == 0:
            return 2 if t == N else LR_INVALID
        if state == 1:
            return -2 if (t == PLUS or t == ABSENT) else LR_INVALID
        if state == 2:
            if t == PLUS:
                return 4
            return 0 if t == ABSENT else LR_INVALID
        if state == 3:
            return 5 if t == N else LR_INVALID
        if state == 4:
            return -1 if (t == PLUS or t == ABSENT) else LR_INVALID
        return LR_INVALID

    def lr_shift(self, token):
        return token.meta().startpos() + 1

    def lr_length(self, rule):
        return 3 if rule == 0 else 1

    def lr_reduce(self, rule, rhs):
        return rhs[0] + rhs[2] if rule == 0 else rhs[0]

    def lr_lhs(self, rule):
        return 0

    def lr_goto(self, state, lhs):
        return 2


def test_lr_sums():
    # tokens n + n + n at positions 0,1,2,3,4 -> shift values are pos+1
    r = LRParser().lr_parse(Sum(), ListIt(toks([N, PLUS, N, PLUS, N])))
    assert r.status and r.node.data() == 1 + 3 + 5


def test_lr_error():
    p = LRParser()
    with pytest.raises(Exception):
        p.lr_parse(Sum(), ListIt(toks([N, N])))
    assert len(p.getErrors()) == 1


class Ambig(Sum):
    """E -> E '+' E | n   (shift/reduce conflict in state 4 on '+')"""

    def lr_action(self, state, window):
        t = window[0]
        if state == 3:
            return 2 if t == N else LR_INVALID
        if state == 4:
            if t == PLUS:
                return LR_CONFLICT
            return -1 if t == ABSENT else LR_INVALID
        return Sum.lr_action(self, state, window)

    def lr_conflict(self, state, window):
        return [4, -1]

    def lr_goto(self, state, lhs):
        return 2 if state == 0 else 4


def test_glr_unambiguous():
    r = LRGeneralizedParser().lr_parse_generalized(Ambig(), ListIt(toks([N, PLUS, N])))
    assert r.status and r.node.data() == 1 + 3


def test_glr_ambiguous_reports():
    with pytest.raises(Exception) as e:
        LRGeneralizedParser().lr_parse_generalized(Ambig(), ListIt(toks([N, PLUS, N, PLUS, N])))
    assert "Ambiguous" in str(e.value)
