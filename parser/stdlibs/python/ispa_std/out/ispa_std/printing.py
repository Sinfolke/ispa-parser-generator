"""Host-only (not lowered): the dynamic printers that dispatch on what a value is.

Every target prints values with its own static dispatch (C++ overloads, Rust
traits); the Python target uses reflect.py. Nodes and tokens are printed from
their public accessors."""
from ispa_std import reflect
from ispa_std.ast_printer import ASTPrinter
from ispa_std.format import bool_to_string, int_to_string
from ispa_std.node import Node
from ispa_std.raw_token import RawToken


def to_string(value) -> str:
    kind = reflect.kind_of(value)
    if kind == reflect.NONE:
        return ""
    if kind == reflect.STR:
        return value
    if kind == reflect.BOOL:
        return bool_to_string(value)
    if kind == reflect.INT:
        return int_to_string(value)
    if kind == reflect.FLOAT:
        return reflect.float_to_string(value)
    if kind == reflect.LIST or kind == reflect.TUPLE:
        return "".join([to_string(item) for item in value])
    if isinstance(value, Node):
        if value.empty():
            raise ValueError("Can't retrieve string representation of an empty node")
        return to_string(value.data())
    if isinstance(value, RawToken):
        if value.empty():
            raise ValueError("Can't retrieve string representation of an empty node")
        # RawToken carries no payload (see raw_token.py): its source text is the closest substitute.
        return value.text()
    if reflect.has_method(value, "to_string"):
        return value.to_string()
    if reflect.has_attr(value, "first") and reflect.has_attr(value, "second"):
        return to_string(value.first) + to_string(value.second)
    return ""


def concat(items: list) -> str:
    return "".join([to_string(item) for item in items])


def node_with_value(printer: ASTPrinter, name: str, value, last: bool) -> None:
    print_named_value(printer, name, value, last)


def subtree(printer: ASTPrinter, name: str, value, last: bool) -> None:
    printer.node(name, last)
    printer.up(not last)
    print_value_tree(printer, value)
    printer.down()


def print_named_value(printer: ASTPrinter, name: str, value, last: bool) -> None:
    kind = reflect.kind_of(value)
    if kind == reflect.NONE:
        printer.scalar(name, "nullptr", last)
    elif kind == reflect.LIST or kind == reflect.TUPLE or isinstance(value, (Node, RawToken)):
        subtree(printer, name, value, last)
    elif kind == reflect.OBJECT and (reflect.has_method(value, "write_to_output") or reflect.has_method(value, "print")):
        subtree(printer, name, value, last)
    else:
        printer.scalar(name, to_string(value), last)


def print_value_tree(printer: ASTPrinter, value) -> None:
    kind = reflect.kind_of(value)
    if kind == reflect.NONE:
        printer.node("nullptr", True)
    elif kind == reflect.LIST:
        for i, item in enumerate(value):
            last = i + 1 == len(value)
            printer.node("item", last)
            printer.up(not last)
            print_value_tree(printer, item)
            printer.down()
    elif kind == reflect.TUPLE:
        for i, item in enumerate(value):
            node_with_value(printer, "item", item, i + 1 == len(value))
    elif isinstance(value, (Node, RawToken)):
        if value.empty():
            printer.node("<empty>", False)
        elif isinstance(value, Node):
            print_value_tree(printer, value.data())
        else:
            # RawToken carries no payload (see raw_token.py): its source text is the closest substitute.
            printer.node(value.text(), True)
    elif kind == reflect.OBJECT and reflect.has_method(value, "write_to_output"):
        value.write_to_output(printer)
    elif kind == reflect.OBJECT and reflect.has_method(value, "print"):
        value.print(printer)
    else:
        node_with_value(printer, "value", value, True)
