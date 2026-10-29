"""dfa_api.hpp: the TDFA tables' vocabulary, capture access and dynamic matching.

Sentinels are small signed Ints so the same table literal works in every target
(the C++ values are size_t maxima). Tables are native arrays:
  table         array of rows, one Int per character class
  class_table   256 Ints, character code -> class
  action_table  rows {op, a, b, next_state}
A state number < len(table) is a table state; the next len(action_table) are
action rows; everything above is a semantic dispatch index."""
from typing import ClassVar

from ispa_std.chars import at, code_of
from ispa_std.failures import runtime_error
from ispa_std.raw_token import RawToken
from ispa_std.string import substr
from ispa_std.utility import Pair

NULL_STATE = -1
LEXER_DELETE_STATE = -2
# A dynamic matcher (`$var`, `!(...)`) whose match failed: the lexer gives up
# on this token exactly like a dead end in the table.
DYNAMIC_FAIL_STATE = -3
UNSET = -1


class Action:
    # Same numbering as NFA::TNFA::Action.
    UNDEF: ClassVar[int] = 0
    # reg[a] = position
    SET: ClassVar[int] = 1
    # reg[a] = position + 1 (one past the character being consumed)
    SET_NEXT: ClassVar[int] = 2
    # reg[a] = reg[b]
    COPY: ClassVar[int] = 3
    # reg[a] = append(reg[b], position)   (b == NULL_STATE: empty list)
    APPEND: ClassVar[int] = 4
    APPEND_NEXT: ClassVar[int] = 5


class TdfaLayout:
    """Registers 0 .. outputs-1 are the outputs."""
    registers: int
    outputs: int

    def __init__(self, registers: int, outputs: int):
        self.registers = registers
        self.outputs = outputs
        if registers < outputs:
            runtime_error("output registers are registers 0 .. outputs-1")


def dynamic_match(src: str, cursor: int, alternatives: list[str], negative: bool, quantifier: str) -> int:
    """A semantic transition tests the whole runtime string. Returns the new
    cursor, or -1 when the match failed. Negation consumes one character, or a
    run when the quantifier is "*"."""
    n = len(src)
    if negative:
        if quantifier == "*":
            p = cursor
            while p < n and not _excluded(src, p, alternatives):
                p = p + 1
            return p
        if cursor >= n or _excluded(src, cursor, alternatives):
            return -1
        return cursor + 1
    i = 0
    while i < len(alternatives):
        value = alternatives[i]
        if len(value) > 0 and _starts_at(src, cursor, value):
            return cursor + len(value)
        i = i + 1
    return -1


def _starts_at(src: str, p: int, value: str) -> bool:
    if p + len(value) > len(src):
        return False
    i = 0
    while i < len(value):
        if src[p + i] != value[i]:
            return False
        i = i + 1
    return True


def _excluded(src: str, p: int, alternatives: list[str]) -> bool:
    i = 0
    while i < len(alternatives):
        if _starts_at(src, p, alternatives[i]):
            return True
        i = i + 1
    return False


class DFADebug:
    """Maps a DFA step back to a grammar position (emitted by the generator)."""
    member: str
    token_name: str
    position_in_token: int
    call: int
    group: int
    rule_run: int
    offset: int
    length: int
    ch: str

    def __init__(self, member: str, token_name: str, position_in_token: int, call: int, group: int,
                 rule_run: int, offset: int, length: int, ch: str):
        self.member = member
        self.token_name = token_name
        self.position_in_token = position_in_token
        self.call = call
        self.group = group
        self.rule_run = rule_run
        self.offset = offset
        self.length = length
        self.ch = ch


class DebugIndex:
    """(state, class) -> index into the DFADebug table. Keys are packed
    state * 256 + class and kept sorted."""
    _keys: list[int]
    _values: list[int]

    def __init__(self):
        self._keys = []
        self._values = []

    def put(self, state: int, cls: int, index: int) -> None:
        key = state * 256 + cls
        lo = 0
        hi = len(self._keys)
        while lo < hi:
            mid = lo + (hi - lo) // 2
            if self._keys[mid] < key:
                lo = mid + 1
            else:
                hi = mid
        if lo < len(self._keys) and self._keys[lo] == key:
            self._values[lo] = index
        else:
            self._keys.insert(lo, key)
            self._values.insert(lo, index)

    def find(self, state: int, cls: int) -> int:
        key = state * 256 + cls
        lo = 0
        hi = len(self._keys)
        while lo < hi:
            mid = lo + (hi - lo) // 2
            if self._keys[mid] < key:
                lo = mid + 1
            else:
                hi = mid
        if lo < len(self._keys) and self._keys[lo] == key:
            return self._values[lo]
        return -1


class HistoryNode:
    pos: int
    prev: int

    def __init__(self, pos: int, prev: int):
        self.pos = pos
        self.prev = prev


class Captures:
    """Read access to the registers a token's DFA run recorded. Offsets are
    relative to the token start `base` inside `src`. Capture k owns registers
    2k (begin, or the head of a list capture's history) and 2k + 1 (end)."""
    _src: str
    _base: int
    _regs: list[int]
    _history: list[HistoryNode]
    _first_line: int

    def __init__(self, src: str, base: int, regs: list[int], history: list[HistoryNode], first_line: int):
        self._src = src
        self._base = base
        self._regs = regs
        self._history = history
        self._first_line = first_line

    def begin_off(self, k: int) -> int:
        return self._regs[2 * k]

    def end_off(self, k: int) -> int:
        return self._regs[2 * k + 1]

    def is_set(self, k: int) -> bool:
        b = self.begin_off(k)
        e = self.end_off(k)
        return b != UNSET and e != UNSET and e >= b

    def begin(self, k: int) -> int:
        return self._base + self.begin_off(k)

    def end(self, k: int) -> int:
        return self._base + self.end_off(k)

    def text(self, k: int) -> str:
        if not self.is_set(k):
            return ""
        return substr(self._src, self.begin(k), self.end_off(k) - self.begin_off(k))

    def view(self, k: int) -> str:
        return self.text(k)

    def character(self, k: int) -> str:
        return at(self._src, self.begin(k))

    def list_positions(self, k: int) -> list[int]:
        positions = []
        h = self.begin_off(k)
        while h != UNSET:
            positions.append(self._history[h].pos)
            h = self._history[h].prev
        positions.reverse()
        return positions

    def list_texts(self, k: int) -> list[str]:
        """One entry per iteration of a list capture, in input order."""
        positions = self.list_positions(k)
        out = []
        i = 0
        while i + 1 < len(positions):
            out.append(substr(self._src, self._base + positions[i], positions[i + 1] - positions[i]))
            i = i + 2
        return out

    def list_chars(self, k: int) -> list[str]:
        texts = self.list_texts(k)
        out = []
        i = 0
        while i < len(texts):
            if len(texts[i]) == 0:
                out.append("")
            else:
                out.append(texts[i][0])
            i = i + 1
        return out

    def line(self, k: int) -> int:
        count = 0
        i = self._base
        stop = self.begin(k)
        while i < stop and i < len(self._src):
            if self._src[i] == "\n":
                count = count + 1
            i = i + 1
        return self._first_line + count

    def column(self, k: int) -> int:
        b = self.begin(k)
        line_start = b
        while line_start > self._base and at(self._src, line_start - 1) != "\n":
            line_start = line_start - 1
        return (b - line_start) + 1


def semantic_result(state: int, token: RawToken) -> Pair[int, RawToken]:
    """What a generated semantic function returns: (next state, token)."""
    return Pair[int, RawToken](state, token)


def class_of(class_table: list[int], ch: str) -> int:
    return class_table[code_of(ch)]
