"""Host-only (not lowered): typed JSON for arbitrary values, by reflection.

debug.value / debug.lexer_capture / Snapshot.add take values already encoded
with this (the typed targets encode with their own static dispatch)."""
from ispa_std import reflect
from ispa_std.debug import obj, quote
from ispa_std.format import int_to_string, join
from ispa_std.node import Node
from ispa_std.raw_token import RawToken


def encode(value, depth: int) -> str:
    """Typed JSON for a value; objects expose debug_type()/debug_fields()."""
    kind = reflect.kind_of(value)
    if kind == reflect.NONE:
        return obj("null", "null")
    if kind == reflect.BOOL:
        if value:
            return obj("bool", "true")
        return obj("bool", "false")
    if kind == reflect.INT:
        return obj("int", int_to_string(value))
    if kind == reflect.FLOAT:
        return obj("double", reflect.float_to_string(value))
    if kind == reflect.STR:
        return obj("string", quote(value))
    if kind == reflect.LIST or kind == reflect.TUPLE:
        if depth >= 16:
            return obj("list", '"<depth limit>"')
        parts = []
        i = 0
        while i < len(value) and i < 256:
            parts.append(encode(value[i], depth + 1))
            i = i + 1
        return obj("list", "[" + join(parts, ",") + "]")
    if reflect.has_method(value, "debug_fields"):
        name = value.debug_type()
        if depth >= 16:
            return obj(name, '"<depth limit>"')
        fields = value.debug_fields()
        parts = []
        i = 0
        while i < len(fields):
            pair = fields[i]
            parts.append(quote(pair.first) + ":" + encode(pair.second, depth + 1))
            i = i + 1
        return obj(name, "{" + join(parts, ",") + "}")
    if isinstance(value, Node):
        if depth >= 16:
            return obj("ISPA.Node", '"<depth limit>"')
        fields = [quote("empty") + ":" + encode(value.empty(), depth + 1)]
        if not value.empty():
            fields.append(quote("name") + ":" + encode(value.name(), depth + 1))
            fields.append(quote("startpos") + ":" + encode(value.startpos(), depth + 1))
            fields.append(quote("length") + ":" + encode(value.length(), depth + 1))
            fields.append(quote("line") + ":" + encode(value.line(), depth + 1))
            fields.append(quote("column") + ":" + encode(value.column(), depth + 1))
            fields.append(quote("data") + ":" + encode(value.data(), depth + 1))
        return obj("ISPA.Node", "{" + join(fields, ",") + "}")
    return obj("opaque", '"<unavailable: expose debug_fields>"')


