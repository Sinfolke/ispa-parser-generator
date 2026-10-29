"""Host-only (not lowered): shape-directed conversion of LR captures.

Python has no static target types, so a generated LR parser describes each AST
field with a Shape and converts dynamically. The typed targets convert with
their own static dispatch (cpp_residue: lr_machine.hpp), which is why this
module is reflection and stays out of the lowered set."""
from ispa_std import reflect
from ispa_std.failures import runtime_error
from ispa_std.lr import LRCaptures, LRTokenValue
from ispa_std.node import Node


# --- shape-directed capture conversion ---------------------------------------
class ShapeKind:
    ANY = 0
    TEXT = 1
    CHAR = 2
    LIST = 3
    TUPLE = 4
    VARIANT = 5


class Shape:
    """What the AST field expects (stands in for the C++ target type)."""
    kind: int
    parts: list["Shape"]

    def __init__(self, kind: int, parts: list["Shape"]):
        self.kind = kind
        self.parts = parts

    @staticmethod
    def any() -> "Shape":
        return Shape(ShapeKind.ANY, [])

    @staticmethod
    def text() -> "Shape":
        return Shape(ShapeKind.TEXT, [])

    @staticmethod
    def char() -> "Shape":
        return Shape(ShapeKind.CHAR, [])

    @staticmethod
    def list_of(element: "Shape") -> "Shape":
        return Shape(ShapeKind.LIST, [element])

    @staticmethod
    def tuple_of(elements: list["Shape"]) -> "Shape":
        return Shape(ShapeKind.TUPLE, elements)

    @staticmethod
    def variant_of(alternatives: list["Shape"]) -> "Shape":
        return Shape(ShapeKind.VARIANT, alternatives)


class Converted:
    ok: bool
    value: object

    def __init__(self, ok: bool, value: object):
        self.ok = ok
        self.value = value


def _is_token_value(v: object) -> bool:
    return reflect.has_attr(v, "text") and reflect.has_attr(v, "node") and not reflect.has_attr(v, "columns")


def _is_captures(v: object) -> bool:
    return reflect.has_attr(v, "columns") and reflect.has_attr(v, "repeated")


def _empty_for(shape: Shape) -> object:
    if shape.kind == ShapeKind.TEXT:
        return ""
    if shape.kind == ShapeKind.CHAR:
        return ""
    if shape.kind == ShapeKind.LIST:
        return []
    return Node.blank()


def lr_convert(shape: Shape, value: object) -> Converted:
    if _is_token_value(value):
        if shape.kind == ShapeKind.CHAR:
            if len(value.text) != 1:
                return Converted(False, 0)
            return Converted(True, value.text)
        if shape.kind == ShapeKind.TEXT:
            return Converted(True, value.text)
        if shape.kind != ShapeKind.VARIANT:
            return lr_convert(shape, value.node)
        # A variant may contain a literal char/string alternative as well as
        # typed nodes: keep the token text until alternatives are tried.
    if _is_captures(value):
        if value.is_empty():
            return Converted(True, _empty_for(shape))
        if shape.kind == ShapeKind.LIST:
            element = shape.parts[0]
            out = []
            if value.repeated and len(value.columns) > 1:
                rows = len(value.columns[0])
                c = 0
                while c < len(value.columns):
                    if len(value.columns[c]) != rows:
                        return Converted(False, 0)
                    c = c + 1
                row = 0
                while row < rows:
                    columns = []
                    c = 0
                    while c < len(value.columns):
                        columns.append([value.columns[c][row]])
                        c = c + 1
                    item = lr_convert(element, LRCaptures(columns, False))
                    if not item.ok:
                        return Converted(False, 0)
                    out.append(item.value)
                    row = row + 1
                return Converted(True, out)
            c = 0
            while c < len(value.columns):
                j = 0
                while j < len(value.columns[c]):
                    item = lr_convert(element, value.columns[c][j])
                    if item.ok:
                        out.append(item.value)
                    else:
                        nested = lr_convert(shape, value.columns[c][j])
                        if not nested.ok:
                            return Converted(False, 0)
                        out.extend(nested.value)
                    j = j + 1
                c = c + 1
            return Converted(True, out)
        if shape.kind == ShapeKind.TUPLE:
            if len(value.columns) != len(shape.parts):
                return Converted(False, 0)
            out = []
            c = 0
            while c < len(shape.parts):
                item = lr_convert(shape.parts[c], LRCaptures([value.columns[c]], False))
                if not item.ok:
                    return Converted(False, 0)
                out.append(item.value)
                c = c + 1
            return Converted(True, out)
        selected = []
        c = 0
        while c < len(value.columns):
            j = 0
            while j < len(value.columns[c]):
                selected.append(value.columns[c][j])
                j = j + 1
            c = c + 1
        if len(selected) > 1:
            return Converted(False, 0)
        if len(selected) == 0:
            return Converted(True, _empty_for(shape))
        return lr_convert(shape, selected[0])
    if shape.kind == ShapeKind.VARIANT:
        i = 0
        while i < len(shape.parts):
            attempt = lr_convert(shape.parts[i], value)
            if attempt.ok:
                return Converted(True, Variant(i, attempt.value))
            i = i + 1
        return Converted(False, 0)
    if shape.kind == ShapeKind.ANY or shape.kind == ShapeKind.TUPLE or shape.kind == ShapeKind.LIST:
        if shape.kind == ShapeKind.ANY:
            return Converted(True, value)
    return Converted(False, 0)


class Variant:
    """Which alternative of a variant shape matched, and its value."""
    index: int
    value: object

    def __init__(self, index: int, value: object):
        self.index = index
        self.value = value


def lr_capture_assign(shape: Shape, values: list[object], index: int, column: int) -> object:
    """Convert values[index] (or one of its columns; column < 0 = whole value)
    to `shape`, or fail when the payload does not match its declared field."""
    value = values[index]
    if column >= 0:
        value = LRCaptures([value.columns[column]], value.repeated)
    result = lr_convert(shape, value)
    if not result.ok:
        runtime_error("LR capture payload does not match its declared AST field type")
    return result.value


