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
