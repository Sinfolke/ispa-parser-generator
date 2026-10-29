"""external.hpp: hand-over to a foreign parser (a `Binding`) for part of the source.

A Binding is any object with
  starts(origin, pos) -> bool
  parse(origin, pos)  -> ExternalResult
Where C++ lets parse() throw ExternalParseError, a Binding here either raises
the std error ExternalParse (through failures.external_parse_error) or any
runtime error; both reach the parser as ExternalParse. Cursors must be lazy:
they expose has_source(), sourceOrigin(), sourcePosition(), consumeExternal(end)."""
from ispa_std import prim
from ispa_std.errors import ErrorKind
from ispa_std.failures import external_parse_error, logic_error, rethrow
from ispa_std.node import MatchResult, Node


class ExternalResult:
    status: bool
    node: Node
    end: int

    def __init__(self, status: bool, node: Node, end: int):
        self.status = status
        self.node = node
        self.end = end

    @staticmethod
    def failed() -> "ExternalResult":
        return ExternalResult(False, Node.blank(), -1)


def external_node(origin: str, start: int, end: int, node: Node) -> Node:
    """Re-anchor a foreign node to the host source: position, line, column."""
    if node.empty():
        return Node.blank()
    line = 1
    i = 0
    while i < start and i < len(origin):
        if origin[i] == "\n":
            line = line + 1
        i = i + 1
    line_start = start
    while line_start > 0 and origin[line_start - 1] != "\n":
        line_start = line_start - 1
    return Node.with_data(start, origin, True, end - start, line, start - line_start + 1, node.name(), node.data())


def _require_lazy(pos) -> None:
    if not pos.has_source():
        logic_error("AMBIGUOUS eager token stream: external parsers require lazy tokenization")


def external_starts(binding, pos) -> bool:
    _require_lazy(pos)
    return binding.starts(pos.sourceOrigin(), pos.sourcePosition())


def external_match(binding, pos) -> MatchResult:
    """Try the binding at pos. On success `pos` is advanced past the foreign text."""
    _require_lazy(pos)
    if not binding.starts(pos.sourceOrigin(), pos.sourcePosition()):
        return MatchResult.failed(pos.copy())
    holder = []
    origin = pos.sourceOrigin()
    begin = pos.sourcePosition()
    failed = prim.attempt(lambda: holder.append(binding.parse(origin, begin)))
    if failed != 0:
        message = prim.failure_message()
        if failed == ErrorKind.RuntimeFailure:
            external_parse_error(message)
        else:
            rethrow(failed, message)
        return MatchResult.failed(pos.copy())
    result = holder[0]
    if not result.status:
        return MatchResult.failed(pos.copy())
    pos.consumeExternal(result.end)
    return MatchResult(True, result.node, pos.copy())
