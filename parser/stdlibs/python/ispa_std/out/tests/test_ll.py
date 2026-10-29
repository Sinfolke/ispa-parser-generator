import pytest

from ispa_std import recovery
from ispa_std.counter import CountSpec
from ispa_std.parser_ll import LLParser
from ispa_std.raw_token import RawToken, RawTokenMeta
from helpers import ListIt

LP, RP, ID, SEMI = 1, 2, 3, 4


def tok(i, t):
    return RawToken.make(RawTokenMeta(i, "", False, 1, 1, i + 1, t), t)


def stream(types):
    return ListIt([tok(i, t) for i, t in enumerate(types)] + [RawToken.empty_token()])


def handler(rule, expected, cont=(), follow=(), sync=()):
    return recovery.Handler(rule, "expected", recovery.TokenSet(list(expected)), recovery.TokenSet(list(cont)),
                            recovery.TokenSet(list(follow)), recovery.TokenSet(list(sync)),
                            recovery.Policy.initial())


def test_optional_and_required_token():
    p = LLParser()
    it = stream([LP, ID, RP])
    assert not p.optional_token(it, LP).empty()
    assert it.i == 1
    assert p.optional_token(it, RP).empty() and it.i == 1
    n = p.required_token(it, ID, handler("r", [ID]))
    assert not n.empty() and it.i == 2


def test_required_failure_raises_and_records():
    p = LLParser()
    it = stream([ID])
    with pytest.raises(Exception):
        p.required_token(it, LP, recovery.LegacyFail(lambda pos: None))


def test_range_optional_token_collects():
    p = LLParser()
    it = stream([ID, ID, ID, RP])
    got = p.range_optional_token(it, ID)
    assert len(got) == 3 and it.i == 3


def test_required_inserts_missing_closer():
    p = LLParser()
    it = stream([ID, SEMI])
    p.required_token(it, ID, handler("r", [ID]))
    # ")" is missing; SEMI follows -> insertion is a legal repair
    p.required_token(it, RP, handler("r", [RP], follow=[SEMI]))
    errs = p.getErrors()
    assert len(errs) == 1
    assert it.i == 1  # nothing was consumed for the synthesized token


def test_counted_token():
    p = LLParser()
    it = stream([ID, ID, ID, ID])
    got = p.counted_token(it, ID, CountSpec.fixed(2, 0))
    assert len(got) == 2 and it.i == 2
