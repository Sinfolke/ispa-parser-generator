"""Runtime value types for ISPA's Python code-generation target.

Indices are offsets in Python str (Unicode codepoints), not C++ char* byte offsets.
The generator should use UTF-8 decoded source consistently.
"""
from __future__ import annotations
import functools
from dataclasses import dataclass, field
from typing import Any, ClassVar, Generic, TypeVar

T = TypeVar('T')
_MISSING = object()


class CallableInt(int):
    """`offset` and `offset()` both work: C++-style metadata accessors survive."""
    def __call__(self) -> int:
        return int(self)


def _callable_metadata(obj) -> None:
    for name in ('startpos', 'length', 'line', 'column'):
        setattr(obj, name, CallableInt(getattr(obj, name)))
    # Generated grammar conditions use token.kind(), while Python runtime
    # consumers also read token.kind as an integer.
    if isinstance(obj.kind, int):
        obj.kind = CallableInt(obj.kind)


@dataclass(slots=True)
class Node(Generic[T]):
    kind: Any = None
    value: T | None = None
    startpos: int = -1
    length: int = 0
    line: int = 0
    column: int = 0
    source: str = ''
    absent: bool = True

    def __post_init__(self) -> None:
        _callable_metadata(self)

    @classmethod
    def create(cls, startpos: int, source: str, length: int, line: int,
               kind: Any, value: T = _MISSING, column: int = 0) -> 'Node[T]':
        return cls(kind, None if value is _MISSING else value, startpos, length,
                   line, column, source, value is _MISSING)

    def empty(self) -> bool:
        return self.absent

    def data(self) -> T | None:
        return self.value

    def set_data(self, value: T) -> None:
        self.value, self.absent = value, False

    def clear(self) -> None:
        self.value, self.absent = None, True

    def name(self) -> Any:
        return self.kind

    def endpos(self) -> int:
        return self.startpos + self.length

    def start(self) -> int:
        return self.startpos

    def end(self) -> int:
        return self.endpos()

    def text(self) -> str:
        return self.source[self.startpos:self.endpos()] if self.startpos >= 0 else ''

    @classmethod
    def from_cpp(cls, startpos, start, end, length, line, column, kind, *payload):
        source = start.source if isinstance(start, Position) else start if isinstance(start, str) else ''
        return cls(kind, payload[0] if payload else None, startpos, length,
                   line, column, source, not bool(payload))

    def __bool__(self) -> bool:
        return not self.absent


@dataclass(slots=True)
class RawToken:
    kind: Any = None
    value: Any = None
    startpos: int = -1
    length: int = 0
    line: int = 0
    column: int = 0
    source: str = ''
    absent: bool = True
    alternatives: ClassVar[tuple[type, ...]] = ()

    def __post_init__(self) -> None:
        _callable_metadata(self)

    @classmethod
    def specialize(cls, *alternatives: type) -> type['RawToken']:
        # Each generated Token alias gets a stable alternative layout. The result is
        # cached so repeated evaluation of an annotation (overload dispatch, isinstance
        # checks) yields the *same* class instead of a fresh, never-equal one.
        return _specialize_raw_token(cls, alternatives)

    @classmethod
    def create(cls, startpos: int, source: str, length: int, line: int,
               kind: Any, value: Any = _MISSING, column: int = 0) -> 'RawToken':
        if isinstance(source, Position):
            source = source.source
        if isinstance(value, Node):
            value = _MISSING if value.empty() else value.data()
        return cls(kind, None if value is _MISSING else value, startpos, length,
                   line, column, source, value is _MISSING)

    @classmethod
    def from_node(cls, node: Node) -> 'RawToken':
        return cls(node.kind, node.value, node.startpos, node.length,
                   node.line, node.column, node.source, node.absent)

    def empty(self) -> bool:
        return self.absent

    def __bool__(self) -> bool:
        return not self.absent

    def name(self) -> Any:
        return self.kind

    def data(self) -> Any:
        return self.value

    def endpos(self) -> int:
        return self.startpos + self.length

    def text(self) -> str:
        return self.source[self.startpos:self.endpos()] if self.startpos >= 0 else ''

    def index(self) -> int:
        if self.absent:
            return -1
        alternatives = type(self).alternatives
        for i, alternative in enumerate(alternatives):
            if type(self.value) is alternative:
                return i
        for i, alternative in enumerate(alternatives):
            if isinstance(alternative, type) and isinstance(self.value, alternative):
                return i
        return 0 if not alternatives else -1

    def holds(self, target: type) -> bool:
        return not self.absent and isinstance(self.value, target)

    def get(self, target: type | None = None) -> Any:
        if target is not None and not self.holds(target):
            raise TypeError(f'RawToken is not {target!r}')
        return self.value

    def to_node(self, target: type | None = None) -> Node:
        if target is not None and not self.holds(target):
            return Node(self.kind, startpos=self.startpos, length=self.length,
                        line=self.line, column=self.column, source=self.source)
        return Node(self.kind, self.value, self.startpos, self.length,
                    self.line, self.column, self.source, self.absent)

    def meta(self) -> 'RawToken':
        return self

    def start(self) -> int:
        return self.startpos

    def end(self) -> int:
        return self.startpos + self.length


@functools.cache
def _specialize_raw_token(base: type, alternatives: tuple) -> type:
    return type('SpecializedRawToken', (base,), {'alternatives': alternatives, '__slots__': ()})


@dataclass(slots=True)
class MatchResult(Generic[T]):
    # Matches C++ `struct MatchResult { bool status; Node node; IT it; }`.
    status: bool = False
    node: T | None = field(default_factory=Node)
    it: Any = None

    def __bool__(self) -> bool:
        return self.status

    def empty(self) -> bool:
        return not self.status


@dataclass(slots=True)
class Variant:
    """Tagged union using C++'s 0=monostate and 1..N alternatives."""
    tag: int = 0
    value: Any = None
    alternatives: ClassVar[tuple[type, ...]] = ()

    def valueless(self) -> bool:
        return self.tag == 0

    def reset(self) -> None:
        self.tag, self.value = 0, None

    @classmethod
    def specialize(cls, *alternatives: type) -> type['Variant']:
        # Cached: Variant.specialize(int, str) must be the same class every time it is
        # evaluated, otherwise isinstance()/overload dispatch can never succeed.
        return _specialize_variant(cls, alternatives)

    def index(self) -> int:
        return self.tag

    def holds(self, target: type) -> bool:
        return self.tag != 0 and isinstance(self.value, target)

    def get(self, target: type) -> Any:
        if not self.holds(target):
            raise TypeError(f'Variant alternative is not {target!r}')
        return self.value


def _variant_tag(alternatives: tuple, value: Any) -> int:
    """1-based alternative index for `value`, or raise TypeError.

    Exact type wins (so True selects `bool`, not `int`); otherwise the most
    derived isinstance() match wins; true ties are an error, as in C++.
    """
    exact = [i + 1 for i, typ in enumerate(alternatives) if type(value) is typ]
    if len(exact) == 1:
        return exact[0]
    loose = [i + 1 for i, typ in enumerate(alternatives)
             if isinstance(typ, type) and isinstance(value, typ)]
    if len(loose) > 1:
        # Drop alternatives that are strict bases of another matching alternative.
        loose = [i for i in loose
                 if not any(j != i and issubclass(alternatives[j - 1], alternatives[i - 1])
                            for j in loose)]
    if len(loose) != 1:
        raise TypeError(f'Variant value {value!r} must match exactly one alternative, got {loose}')
    return loose[0]


@functools.cache
def _specialize_variant(base: type, alternatives: tuple) -> type:
    class SpecializedVariant(base):
        __slots__ = ()

        def __init__(self, value: Any = _MISSING):
            if value is _MISSING:
                base.__init__(self, 0, None)
            else:
                base.__init__(self, _variant_tag(alternatives, value), value)

        def emplace(self, value: Any) -> None:
            self.tag, self.value = _variant_tag(alternatives, value), value

    SpecializedVariant.alternatives = alternatives
    return SpecializedVariant


def variant_get(value: Any, target: type) -> Any:
    if isinstance(value, Variant):
        return value.get(target)
    if isinstance(value, RawToken):
        return value.to_node(target)
    if not isinstance(value, target):
        raise TypeError(f'Expected {target!r}, got {type(value)!r}')
    return value


def variant_holds(value: Any, target: type) -> bool:
    if isinstance(value, Variant):
        return value.holds(target)
    if isinstance(value, RawToken):
        return value.holds(target)
    return isinstance(value, target)


def variant_index(value: Any) -> int:
    if isinstance(value, Variant):
        return value.index()
    if isinstance(value, RawToken):
        return value.index()
    return 0 if value is None else 1


def assign_compatible(_old: Any, value: Any) -> Any:
    """Python assignments don't require variant visitation or implicit Node copies."""
    if isinstance(value, MatchResult):
        return value.node
    return value


def concat(*values: Any) -> str:
    return ''.join(map(str, values))


def step_attr(obj: Any, name: str, delta: int, post: bool) -> Any:
    """C++ `obj.name++` / `++obj.name` (and `--`) used as a *value*.

    Plain local names are lowered to a walrus by the generator; attributes and
    subscripts cannot be, so they go through these helpers.
    """
    old = getattr(obj, name)
    new = old + delta
    setattr(obj, name, new)
    return old if post else new


def step_item(obj: Any, key: Any, delta: int, post: bool) -> Any:
    """Subscript form of step_attr: `a[i]++` as a value."""
    old = obj[key]
    new = old + delta
    obj[key] = new
    return old if post else new


def _assigned_value(old: Any, value: Any, operation: str) -> Any:
    if operation == '=':
        return value
    if operation == '+':
        return old + value
    if operation == '-':
        return old - value
    if operation == '*':
        return old * value
    if operation == '/':
        return old / value
    if operation == '//':
        return old // value
    if operation == '%':
        return old % value
    raise ValueError(f'unknown assignment operation: {operation}')


def assign_attr(obj: Any, name: str, value: Any, operation: str = '=') -> Any:
    """Assign an attribute and return the assigned value for an action expression."""
    result = _assigned_value(getattr(obj, name) if operation != '=' else None, value, operation)
    setattr(obj, name, result)
    return result


def assign_item(obj: Any, key: Any, value: Any, operation: str = '=') -> Any:
    """Assign a subscript and return the assigned value for an action expression."""
    result = _assigned_value(obj[key] if operation != '=' else None, value, operation)
    obj[key] = result
    return result


@dataclass(slots=True)
class Position:
    source: str
    index: int = 0

    def peek(self, offset: int = 0) -> str:
        pos = self.index + offset
        return self.source[pos] if 0 <= pos < len(self.source) else '\0'

    def advance(self, n: int = 1) -> 'Position':
        self.index += n
        return self

    def copy(self) -> 'Position':
        return Position(self.source, self.index)

    def __iadd__(self, value: int) -> 'Position':
        return self.advance(value)

    def __int__(self) -> int:
        return self.index

    def __add__(self, n: int) -> 'Position':
        return Position(self.source, self.index + n)

    def __sub__(self, n: int | 'Position') -> int | 'Position':
        return self.index - n.index if isinstance(n, Position) else Position(self.source, self.index - n)

    def at_end(self) -> bool:
        """True only at real end of input (peek() returns '\\0' for both EOF and a literal NUL)."""
        return self.index >= len(self.source)

    def __lt__(self, other: 'Position | int') -> bool:
        return self.index < (other.index if isinstance(other, Position) else other)

    def __le__(self, other: 'Position | int') -> bool:
        return self.index <= (other.index if isinstance(other, Position) else other)

    def __gt__(self, other: 'Position | int') -> bool:
        return self.index > (other.index if isinstance(other, Position) else other)

    def __ge__(self, other: 'Position | int') -> bool:
        return self.index >= (other.index if isinstance(other, Position) else other)

    def __eq__(self, other: object) -> bool:
        if isinstance(other, Position):
            return self.index == other.index and self.source is other.source
        return self.index == other if isinstance(other, int) else False
