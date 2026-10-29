"""The one place that needs run-time type tests.

C++ picks overloads at compile time (to_string_helper, ASTPrinter's
is_vector/is_variant/has_write_to_output ...). A dynamically typed target can
test the value instead. Statically typed targets should not lower this module:
the generator knows each value's type and emits the matching typed call
(`to_string.str_to_string`, `ASTPrinter.node_with_value`, ...) directly."""

STR = 1
BOOL = 2
INT = 3
FLOAT = 4
LIST = 5
TUPLE = 6
NONE = 7
OBJECT = 8


def kind_of(value) -> int:
    if value is None:
        return NONE
    if isinstance(value, bool):
        return BOOL
    if isinstance(value, int):
        return INT
    if isinstance(value, float):
        return FLOAT
    if isinstance(value, str):
        return STR
    if isinstance(value, list):
        return LIST
    if isinstance(value, tuple):
        return TUPLE
    return OBJECT


def has_method(value, name: str) -> bool:
    return callable(getattr(value, name, None))


def has_attr(value, name: str) -> bool:
    return hasattr(value, name)


def float_to_string(value) -> str:
    return "%f" % value
