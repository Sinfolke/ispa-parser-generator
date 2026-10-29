"""assign.hpp: the source-extraction cases of assign_compatible.

The C++ function is overload resolution over target/source types (vectors,
variants, tuples with omitted strings, node-storage conversions); that part is
static typing and stays in the host runtime. What is language-neutral is how a
matched node becomes a String, a one-character String, or a list of those."""
from typing import Callable, TypeVar

from ispa_std.failures import internal_error
from ispa_std.node import Node

T = TypeVar("T")


def node_text(node: Node) -> str:
    """target.assign(source.start(), source.length())"""
    return node.text()


def node_char(node: Node) -> str:
    if node.length() != 1 or not node.has_source():
        internal_error("assign_compatible: expected one character", "Parser", True, [])
    return node.text()


def assign_list(source: list[Node], convert: Callable[[Node], T]) -> list[T]:
    """The vector -> vector case: convert each element; empty source nodes
    leave their destination default (callers skip them with node.empty())."""
    out = []
    i = 0
    while i < len(source):
        out.append(convert(source[i]))
        i = i + 1
    return out
