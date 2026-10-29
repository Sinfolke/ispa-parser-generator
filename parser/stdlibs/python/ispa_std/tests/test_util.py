from ispa_std.chars import at, char_of, code_of, is_digit, is_space
from ispa_std.format import fmt, int_to_string, join, repeat, replace_all
from ispa_std.sortedmap import IntSet, StrIntMap
from ispa_std.ast_printer import ASTPrinter


def test_format():
    assert int_to_string(-120) == "-120" and int_to_string(0) == "0"
    assert fmt("{} at {}", ["x", "3"]) == "x at 3"
    assert replace_all("a.b.c", ".", "::") == "a::b::c"
    assert repeat("ab", 3) == "ababab" and join(["a", "b"], ", ") == "a, b"


def test_chars():
    assert code_of("A") == 65 and code_of("") == 0
    assert char_of(97) == "a" and at("abc", 5) == ""
    assert is_space("\n") and is_digit("7") and not is_digit("x")
    assert all(code_of(char_of(i)) == i for i in range(1, 256))


def test_sorted_containers():
    m = StrIntMap()
    for k in ["b", "a", "c", "a"]:
        m.set(k, len(k))
    assert m.contains("a") and not m.contains("z") and m.get("b") == 1
    s = IntSet()
    assert s.add(5) and s.add(2) and not s.add(5)
    assert s.size() == 2 and s.smallest() == 2


def test_ast_printer():
    p = ASTPrinter()
    p.node("root", True)
    p.down()
    p.scalar("x", "1", True)
    p.up(False)
    assert "root" in p.text() and "x" in p.text()
