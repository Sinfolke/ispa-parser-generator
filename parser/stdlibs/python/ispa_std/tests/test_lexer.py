import helpers as d
from ispa_std.lexer import Lexer_base
from ispa_std.refs import Cursor


from ispa_std.raw_token import RawToken, RawTokenMeta
from ispa_std.utility import Pair
from ispa_std.dfa_api import NULL_STATE


def sem(idx, cap, start_pos, length, start_line, cur):
    if idx == 2:
        return Pair(NULL_STATE, RawToken.empty_token())
    return Pair(NULL_STATE, RawToken.make(RawTokenMeta(start_pos, "", False, length, start_line, 1, idx), idx))


class L(Lexer_base):
    def __init__(self):
        Lexer_base.__init__(self)
        self.tables = d.make_tables()

    def makeToken(self, cur):
        return self.lookup(self.tables, sem, cur)


def test_eager_tokens_and_lines():
    lx = L()
    toks = lx.makeTokensFromText("ab 12\n+x")
    # the whitespace token is "" -> dropped by makeTokens
    assert [t.kind() for t in toks[:-1]] == [0, 1, 3, 0]  # ID NUM PLUS ID
    assert lx.line == 2


def test_no_progress_is_internal_error():
    import pytest
    lx = L()
    with pytest.raises(Exception):
        lx.makeTokensFromText("!")
