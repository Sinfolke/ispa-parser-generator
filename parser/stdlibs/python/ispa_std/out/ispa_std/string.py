"""std::string / std::string_view style operations on the native String.

Positions are Int indices and "not found" is NPOS (-1). Only indexing, len and
concatenation are used, so it lowers wherever the native string does."""
from ispa_std import prim
from ispa_std.errors import ErrorKind

NPOS = -1


def substr(s: str, pos: int, count: int) -> str:
    # count < 0 means "to the end", like std::string::npos.
    n = len(s)
    if pos < 0 or pos > n:
        prim.fail(ErrorKind.OutOfRange, "string.substr: pos out of range")
    end = n
    if count >= 0 and pos + count < n:
        end = pos + count
    out = ""
    i = pos
    while i < end:
        out = out + s[i]
        i = i + 1
    return out


def compare(a: str, b: str) -> int:
    i = 0
    while i < len(a) and i < len(b):
        if a[i] < b[i]:
            return -1
        if b[i] < a[i]:
            return 1
        i = i + 1
    if len(a) < len(b):
        return -1
    if len(b) < len(a):
        return 1
    return 0


def matches_at(s: str, needle: str, pos: int) -> bool:
    if pos < 0 or pos + len(needle) > len(s):
        return False
    i = 0
    while i < len(needle):
        if s[pos + i] != needle[i]:
            return False
        i = i + 1
    return True


def starts_with(s: str, prefix: str) -> bool:
    return matches_at(s, prefix, 0)


def ends_with(s: str, suffix: str) -> bool:
    return matches_at(s, suffix, len(s) - len(suffix))


def find(s: str, needle: str, start: int) -> int:
    if start < 0 or start > len(s):
        return NPOS
    pos = start
    while pos + len(needle) <= len(s):
        if matches_at(s, needle, pos):
            return pos
        pos = pos + 1
    return NPOS


def rfind(s: str, needle: str) -> int:
    pos = len(s) - len(needle)
    while pos >= 0:
        if matches_at(s, needle, pos):
            return pos
        pos = pos - 1
    return NPOS


def contains(s: str, needle: str) -> bool:
    return find(s, needle, 0) != NPOS
