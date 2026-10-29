"""to_string.hpp: to_string_helper / concat.

The typed entry points lower anywhere. The dynamic `to_string` (dispatch on the
value) is reflection and lives in printing.py, a host-only module."""
from typing import Callable, TypeVar

from ispa_std.format import bool_to_string, int_to_string

T = TypeVar("T")


def str_to_string(value: str) -> str:
    return value


def bool_value_to_string(value: bool) -> str:
    return bool_to_string(value)


def int_value_to_string(value: int) -> str:
    return int_to_string(value)


def list_to_string(items: list[T], element_to_string: Callable[[T], str]) -> str:
    out = ""
    i = 0
    while i < len(items):
        out = out + element_to_string(items[i])
        i = i + 1
    return out
