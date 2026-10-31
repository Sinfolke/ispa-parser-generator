"""Small AST-building helpers used by the grammar standard library."""

from internals import Node
from langapi import size


def sep_seq_to_list(a: Node, b: list[Node]) -> list[Node]:
    """Convert the head plus repeated separator captures into a flat list."""
    b.insert(0, a)
    return b


def kv_to_dict(key: Node, value: Node, key_seq: list[Node], value_seq: list[Node]) -> dict[Node, Node]:
    """Pair the first key/value with subsequent repeated key/value captures."""
    initial: dict[Node, Node] = {}
    initial[key] = value
    i = 0
    while i < size(key_seq) and i < size(value_seq):
        initial[key_seq[i]] = value_seq[i]
        i += 1
    return initial
