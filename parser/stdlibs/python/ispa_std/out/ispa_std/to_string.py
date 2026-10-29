"""to_string.hpp: to_string_helper / concat.

Typed entry points lower anywhere. `to_string` is the dynamic convenience that
dispatches on the value (see reflect.py): Strings as is, bool "true"/"false",
ints in decimal, arrays and Pairs by concatenating their elements, "no value"
(the C++ std::monostate) as "", and anything with a to_string() method
(Node, RawToken, generated AST structs) through it."""
from ispa_std import reflect
from ispa_std.format import bool_to_string, int_to_string


def str_to_string(value: str) -> str:
    return value


def bool_value_to_string(value: bool) -> str:
    return bool_to_string(value)


def int_value_to_string(value: int) -> str:
    return int_to_string(value)


def list_to_string(items: list, element_to_string) -> str:
    out = ""
    i = 0
    while i < len(items):
        out = out + element_to_string(items[i])
        i = i + 1
    return out


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
        out = ""
        i = 0
        while i < len(value):
            out = out + to_string(value[i])
            i = i + 1
        return out
    if reflect.has_method(value, "to_string"):
        return value.to_string()
    if reflect.has_attr(value, "first") and reflect.has_attr(value, "second"):
        return to_string(value.first) + to_string(value.second)
    return ""


def concat(items: list) -> str:
    return list_to_string(items, to_string)
