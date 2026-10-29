"""ast_printer.hpp: the tree printer behind `write_to_output`.

Output goes to an in-memory list of Strings (join with `text()`), replacing the
std::ostream template parameter. Typed calls (`node`, `node_with_value`,
`subtree`) lower anywhere; `print_named_value` / `print_value_tree` are the
dynamic equivalents of the C++ compile-time dispatch (see reflect.py)."""
from ispa_std import reflect
from ispa_std.format import join
from ispa_std.to_string import to_string


class ASTPrinter:
    _out: list
    _branches: list

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

    def node_with_value(self, name: str, value, last: bool) -> None:
        self.print_named_value(name, value, last)

    def subtree(self, name: str, value, last: bool) -> None:
        self.node(name, last)
        self.up(not last)
        self.print_value_tree(value)
        self.down()

    def print_named_value(self, name: str, value, last: bool) -> None:
        kind = reflect.kind_of(value)
        if kind == reflect.NONE:
            self.scalar(name, "nullptr", last)
        elif kind == reflect.LIST or kind == reflect.TUPLE:
            self.subtree(name, value, last)
        elif kind == reflect.OBJECT and (reflect.has_method(value, "write_to_output") or reflect.has_method(value, "print")):
            self.subtree(name, value, last)
        else:
            self.scalar(name, to_string(value), last)

    def print_value_tree(self, value) -> None:
        kind = reflect.kind_of(value)
        if kind == reflect.NONE:
            self.node("nullptr", True)
        elif kind == reflect.LIST:
            i = 0
            while i < len(value):
                last = i + 1 == len(value)
                self.node("item", last)
                self.up(not last)
                self.print_value_tree(value[i])
                self.down()
                i = i + 1
        elif kind == reflect.TUPLE:
            i = 0
            while i < len(value):
                self.node_with_value("item", value[i], i + 1 == len(value))
                i = i + 1
        elif kind == reflect.OBJECT and reflect.has_method(value, "write_to_output"):
            value.write_to_output(self)
        elif kind == reflect.OBJECT and reflect.has_method(value, "print"):
            value.print(self)
        else:
            self.node_with_value("value", value, True)
