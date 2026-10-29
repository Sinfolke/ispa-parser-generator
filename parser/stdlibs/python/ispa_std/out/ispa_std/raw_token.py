"""raw_token.hpp: RawToken and its source metadata.

The C++ class is a hand-rolled variant (aligned storage + function tables) over
a compile-time list of payload types. Here the alternative is a small Int
`type_id` the generator assigns (index in the token alternative list) and the
payload is any value. `holds(type_id)` / `to_node(type_id)` replace
`holds<T>()` / `to_node<T>()`.

Positions are Int offsets into `source` (a native String shared by reference),
not pointers: start() -> startpos, end() -> startpos + length."""
from ispa_std.chars import at
from ispa_std.failures import api_error, logic_error
from ispa_std.node import Node
from ispa_std.string import substr

NO_POS = -1
NO_NAME = -1
NO_TYPE = -1
# RawToken::ambiguous_id
AMBIGUOUS_ID = -2


class RawTokenMeta:
    _startpos: int
    _length: int
    _line: int
    _column: int
    _source: str
    _has_source: bool
    _name: int

    def __init__(self, startpos: int, source: str, has_source: bool, length: int, line: int, column: int, name: int):
        self._startpos = startpos
        self._length = length
        self._line = line
        self._column = column
        self._source = source
        self._has_source = has_source
        self._name = name

    @staticmethod
    def blank() -> "RawTokenMeta":
        return RawTokenMeta(NO_POS, "", False, 0, 0, 0, NO_NAME)

    def startpos(self) -> int:
        return self._startpos

    def length(self) -> int:
        return self._length

    def line(self) -> int:
        return self._line

    def column(self) -> int:
        return self._column

    def name(self) -> int:
        return self._name

    def source(self) -> str:
        return self._source

    def has_source(self) -> bool:
        # C++: start() != nullptr
        return self._has_source

    def end_pos(self) -> int:
        # C++: end() as an offset
        return self._startpos + self._length

    def text(self) -> str:
        if not self._has_source or self._startpos < 0:
            return ""
        return substr(self._source, self._startpos, self._length)


class RawToken:
    _type_id: int
    _payload: object
    _meta: RawTokenMeta

    def __init__(self, meta: RawTokenMeta, type_id: int, payload: object):
        self._meta = meta
        self._type_id = type_id
        self._payload = payload

    # --- construction -----------------------------------------------------
    @staticmethod
    def empty_token() -> "RawToken":
        """The default-constructed (end-of-stream) token."""
        return RawToken(RawTokenMeta.blank(), NO_TYPE, 0)

    @staticmethod
    def ambiguous(meta: RawTokenMeta) -> "RawToken":
        return RawToken(meta, NO_TYPE, 0)

    @staticmethod
    def make(meta: RawTokenMeta, type_id: int, payload: object) -> "RawToken":
        if type_id < 0:
            logic_error("RawToken: payload type is not an alternative")
        return RawToken(meta, type_id, payload)

    @staticmethod
    def from_node(node: Node, type_id: int) -> "RawToken":
        meta = RawTokenMeta(node.startpos(), node.source(), node.has_source(), node.length(),
                            node.line(), node.column(), node.name())
        if node.empty():
            return RawToken(meta, NO_TYPE, 0)
        return RawToken(meta, type_id, node.data())

    @staticmethod
    def create(startpos: int, source: str, has_source: bool, length: int, line: int,
               enumv: int, type_id: int, value: object) -> "RawToken":
        meta = RawTokenMeta(startpos, source, has_source, length, line, 0, enumv)
        return RawToken(meta, type_id, value)

    def copy(self) -> "RawToken":
        return RawToken(self._meta, self._type_id, self._payload)

    # --- queries ------------------------------------------------------------
    def empty(self) -> bool:
        return self._type_id == NO_TYPE

    def isAmbiguous(self) -> bool:
        return self._meta.name() == AMBIGUOUS_ID

    def meta(self) -> RawTokenMeta:
        return self._meta

    def kind(self) -> int:
        return self._meta.name()

    def index(self) -> int:
        return self._type_id

    def holds(self, type_id: int) -> bool:
        return self._type_id == type_id

    def get(self) -> object:
        if self._type_id == NO_TYPE:
            logic_error("RawToken::get: alternative mismatch")
        return self._payload

    def to_node(self, type_id: int) -> Node:
        """The payload as a Node (RawToken::to_node<T>): metadata always, data
        only when this token holds `type_id`."""
        m = self._meta
        if self._type_id == NO_TYPE or self._type_id != type_id:
            return Node.with_meta(m.startpos(), m.source(), m.has_source(), m.length(),
                                  m.line(), m.column(), m.name())
        return Node.with_data(m.startpos(), m.source(), m.has_source(), m.length(),
                              m.line(), m.column(), m.name(), self._payload)

    def text(self) -> str:
        """The source text this token covers ("" when it has no source)."""
        return self._meta.text()

def char_at_source(meta: RawTokenMeta, offset: int) -> str:
    return at(meta.source(), meta.startpos() + offset)
