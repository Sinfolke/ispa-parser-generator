"""ast_printer.hpp: the tree printer behind `write_to_output`.

Output goes to an in-memory list of Strings (join with `text()`), replacing the
std::ostream template parameter. The value-directed printing (dispatch on what a
value is) is reflection and lives in printing.py, a host-only module."""
from ispa_std.format import join


class ASTPrinter:
    _out: list[str]
    _branches: list[bool]

    def __init__(self):
        self._out = []
        self._branches = []

    def text(self) -> str:
        return join(self._out, "")

    def up(self, has_more_siblings: bool) -> None:
        self._branches.append(has_more_siblings)

    def down(self) -> None:
        if len(self._branches) > 0:
            self._branches.pop()

    def _prefix(self, last: bool) -> None:
        depth = len(self._branches)
        if depth == 0:
            return
        i = 0
        while i + 1 < depth:
            if self._branches[i]:
                self._out.append("│   ")
            else:
                self._out.append("    ")
            i = i + 1
        if last:
            self._out.append("└── ")
        else:
            self._out.append("├── ")

    def node(self, name: str, last: bool) -> None:
        self._prefix(last)
        self._out.append(name + "\n")

    def scalar(self, name: str, text: str, last: bool) -> None:
        """`name: value` for an already-formatted scalar."""
        self._prefix(last)
        self._out.append(name + ": " + text + "\n")
