"""Sequential {} formatting and the number/string helpers the runtime needs.

Everything is built from indexing, len, concatenation and integer arithmetic so
it lowers wherever the native String does."""
from ispa_std.string import find, substr


def int_to_string(n: int) -> str:
    if n == 0:
        return "0"
    negative = n < 0
    if negative:
        n = 0 - n
    digits = ""
    while n > 0:
        d = n - (n // 10) * 10
        digits = "0123456789"[d] + digits
        n = n // 10
    if negative:
        return "-" + digits
    return digits


def bool_to_string(value: bool) -> str:
    if value:
        return "true"
    return "false"


def fmt(format_text: str, args: list) -> str:
    # ISPA_STD::fmt: replaces the next "{}" for each argument. Arguments are
    # already Strings; extra arguments are ignored, missing ones leave "{}".
    res = format_text
    cursor = 0
    i = 0
    while i < len(args):
        pos = find(res, "{}", cursor)
        if pos != -1:
            value = args[i]
            res = substr(res, 0, pos) + value + substr(res, pos + 2, -1)
            cursor = pos + len(value)
        i = i + 1
    return res


def replace_all(s: str, needle: str, replacement: str) -> str:
    if len(needle) == 0:
        return s
    out = ""
    i = 0
    while i < len(s):
        pos = find(s, needle, i)
        if pos == -1:
            out = out + substr(s, i, -1)
            i = len(s)
        else:
            out = out + substr(s, i, pos - i) + replacement
            i = pos + len(needle)
    return out


def repeat(s: str, count: int) -> str:
    out = ""
    i = 0
    while i < count:
        out = out + s
        i = i + 1
    return out


def join(items: list, separator: str) -> str:
    out = ""
    i = 0
    while i < len(items):
        if i > 0:
            out = out + separator
        out = out + items[i]
        i = i + 1
    return out


def concat_all(items: list) -> str:
    return join(items, "")
