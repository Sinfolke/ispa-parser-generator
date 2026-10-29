"""Mutable holders for the C++ by-reference parameters (`const char*&`, `long long&`)."""


class Cursor:
    """A scan position and the 1-based line it sits on."""
    pos: int
    line: int

    def __init__(self, pos: int, line: int):
        self.pos = pos
        self.line = line

    def copy(self) -> "Cursor":
        return Cursor(self.pos, self.line)


class IntRef:
    value: int

    def __init__(self, value: int):
        self.value = value
