"""node.hpp: Node (a typed AST value + where it came from) and MatchResult.

The C++ Node<DataStorageType> stores the payload inline or behind a
shared_ptr, with converting constructors between the two. Values here are
references already, so only the metadata and the "empty" flag remain; the
storage-conversion machinery is C++-only (see cpp_residue/MANIFEST.md).

Positions are Int offsets into `source`, a native String shared by reference
(C++: `const char* start/end`). `copy()` replaces C++ value semantics: a Node
is never mutated through an alias unless the caller wants that."""
from ispa_std.failures import api_error
from ispa_std.string import substr

NO_POS = -1
NO_NAME = -1


class Node:
    _startpos: int
    _length: int
    _line: int
    _column: int
    _source: str
    _has_source: bool
    _name: int
    _empty: bool
    _data: object

    def __init__(self, startpos: int, source: str, has_source: bool, length: int, line: int,
                 column: int, name: int, empty: bool, data: object):
        self._startpos = startpos
        self._length = length
        self._line = line
        self._column = column
        self._source = source
        self._has_source = has_source
        self._name = name
        self._empty = empty
        self._data = data

    @staticmethod
    def blank() -> "Node":
        """Node{}: empty, no position."""
        return Node(NO_POS, "", False, 0, 0, 0, NO_NAME, True, 0)

    @staticmethod
    def with_meta(startpos: int, source: str, has_source: bool, length: int, line: int,
                  column: int, name: int) -> "Node":
        """A positioned node that carries no data yet (still empty())."""
        return Node(startpos, source, has_source, length, line, column, name, True, 0)

    @staticmethod
    def with_data(startpos: int, source: str, has_source: bool, length: int, line: int,
                  column: int, name: int, data: object) -> "Node":
        return Node(startpos, source, has_source, length, line, column, name, False, data)

    @staticmethod
    def create(startpos: int, source: str, length: int, line: int, enumv: int, value: object) -> "Node":
        # Column cannot be recovered from the start alone; callers needing it
        # use with_data.
        return Node(startpos, source, True, length, line, 0, enumv, False, value)

    def copy(self) -> "Node":
        return Node(self._startpos, self._source, self._has_source, self._length, self._line,
                    self._column, self._name, self._empty, self._data)

    def assign(self, other: "Node") -> None:
        self._startpos = other._startpos
        self._length = other._length
        self._line = other._line
        self._column = other._column
        self._source = other._source
        self._has_source = other._has_source
        self._name = other._name
        self._empty = other._empty
        self._data = other._data

    def endpos(self) -> int:
        if self._startpos == NO_POS or not self._has_source:
            api_error("Can't retrieve end position on an empty token", [])
        return self._startpos + self._length

    def clear(self) -> None:
        self._startpos = NO_POS
        self._line = 0
        self._column = 0
        self._length = 0
        self._source = ""
        self._has_source = False
        self._name = NO_NAME
        self._data = 0
        self._empty = True

    def empty(self) -> bool:
        return self._empty

    def startpos(self) -> int:
        return self._startpos

    def line(self) -> int:
        return self._line

    def column(self) -> int:
        return self._column

    def length(self) -> int:
        return self._length

    def source(self) -> str:
        return self._source

    def has_source(self) -> bool:
        return self._has_source

    def name(self) -> int:
        return self._name

    def data(self) -> object:
        return self._data

    def set_data(self, value: object) -> None:
        self._data = value
        self._empty = False

    def text(self) -> str:
        if not self._has_source or self._startpos < 0:
            return ""
        return substr(self._source, self._startpos, self._length)
