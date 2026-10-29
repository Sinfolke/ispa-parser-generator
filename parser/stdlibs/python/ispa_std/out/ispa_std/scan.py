"""recovery_scan.hpp: scan opaque Python payloads for a boundary without tokenizing.

Positions are Int indices into a native String; "" past the end plays the C++
NUL terminator."""
from ispa_std.chars import at
from ispa_std.string import substr


class ScanResult:
    end: int
    exhausted: bool

    def __init__(self, end: int, exhausted: bool):
        self.end = end
        self.exhausted = exhausted


def _boundary_at(src: str, p: int, boundary: str) -> bool:
    i = 0
    while i < len(boundary):
        c = at(src, p + i)
        if len(c) == 0 or c != boundary[i]:
            return False
        i = i + 1
    return True


def scan_balanced(src: str, begin: int, boundaries: list, budget: int) -> ScanResult:
    """Scan forward from `begin`. The budget is in source units, including
    strings and comments. Stops at the first boundary outside brackets (a ";"
    boundary also stops inside brackets), or at an unmatched closer."""
    p = begin
    n = len(src)
    closers = []
    quote = ""
    triple = False
    comment = False
    while p < n:
        if p - begin >= budget:
            return ScanResult(p, True)
        c = src[p]
        skip_tail = False
        if comment:
            if c == "\n" or c == "\r":
                comment = False
        elif len(quote) > 0:
            if c == "\\" and len(at(src, p + 1)) > 0:
                if 2 > budget - (p - begin):
                    return ScanResult(p, True)
                p = p + 2
                skip_tail = True
            elif c == quote and ((not triple) or (at(src, p + 1) == quote and at(src, p + 2) == quote)):
                width = 1
                if triple:
                    width = 3
                if width > budget - (p - begin):
                    return ScanResult(p, True)
                p = p + width
                quote = ""
                skip_tail = True
            elif (not triple) and (c == "\n" or c == "\r"):
                # A broken single-line string must not swallow later declarations.
                return ScanResult(p, True)
        else:
            i = 0
            while i < len(boundaries):
                boundary = boundaries[i]
                if (len(closers) == 0 or boundary == ";") and _boundary_at(src, p, boundary):
                    return ScanResult(p, False)
                i = i + 1
            if c == ")" or c == "]" or c == "}":
                if len(closers) == 0 or closers[len(closers) - 1] != c:
                    return ScanResult(p, False)
                closers.pop()
            elif c == "(":
                closers.append(")")
            elif c == "[":
                closers.append("]")
            elif c == "{":
                closers.append("}")
            elif c == "#":
                comment = True
            elif c == "'" or c == '"':
                quote = c
                triple = at(src, p + 1) == c and at(src, p + 2) == c
                width = 1
                if triple:
                    width = 3
                if width > budget - (p - begin):
                    return ScanResult(p, True)
                p = p + width
                skip_tail = True
        if not skip_tail:
            if 1 > budget - (p - begin):
                return ScanResult(p, True)
            p = p + 1
    return ScanResult(p, len(quote) > 0)


def text_between(src: str, begin: int, end: int) -> str:
    return substr(src, begin, end - begin)
