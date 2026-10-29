from ispa_std.dfa_api import Action, DebugIndex, NULL_STATE, TdfaLayout
from ispa_std.dfa_run import DfaTables
from ispa_std.raw_token import RawToken, RawTokenMeta


class ListIt:
    """Minimal token iterator over a Python list (stands in for the lexer's iterators)."""

    def __init__(self, toks, i=0):
        self.toks, self.i = toks, i

    def isEnd(self):
        return self.i >= len(self.toks)

    def current(self):
        return self.toks[self.i]

    def next(self):
        self.i += 1
        return self

    def copy(self):
        return ListIt(self.toks, self.i)

    def assign(self, other):
        self.toks, self.i = other.toks, other.i

    def distance(self):
        return self.i


def toks(kinds):
    return [RawToken.make(RawTokenMeta(i, "", False, 1, 1, i + 1, k), 0) for i, k in enumerate(kinds)]


def make_tables():
    N = NULL_STATE
    cls = [0] * 256
    for c in range(ord("a"), ord("z") + 1):
        cls[c] = 1
    for c in range(ord("0"), ord("9") + 1):
        cls[c] = 2
    cls[ord(" ")] = 3
    cls[ord("\n")] = 3
    cls[ord("+")] = 4
    table = [[N, 5, 7, 3, 4], [6, 1, 1, 6, 6], [9, 9, 2, 9, 9], [10, 10, 10, 3, 10], [11, 11, 11, 11, 11]]
    actions = [[Action.SET, 0, N, 1], [Action.SET, 1, N, 8], [Action.SET, 0, N, 2]]
    return DfaTables(table, cls, actions, TdfaLayout(3, 2), [], DebugIndex())
