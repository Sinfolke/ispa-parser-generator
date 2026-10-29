"""assign.hpp: the source-extraction cases of assign_compatible.

The C++ function is overload resolution over target/source types (vectors,
variants, tuples with omitted strings, node-storage conversions); that part is
static typing and stays in cpp_residue. What is language-neutral is how a
matched node becomes a String, a one-character String, or a list of those."""
from ispa_std.failures import internal_error


def node_text(node) -> str:
    """target.assign(source.start(), source.length())"""
    return node.text()


def node_char(node) -> str:
    if node.length() != 1 or not node.has_source():
        internal_error("assign_compatible: expected one character", "Parser", True, [])
    return node.text()


def assign_list(source: list, convert) -> list:
    """The vector -> vector case: convert each element; empty source nodes
    leave their destination default (callers skip them with node.empty())."""
    out = []
    i = 0
    while i < len(source):
        out.append(convert(source[i]))
        i = i + 1
    return out
