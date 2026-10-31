"""lr_machine.hpp: the deterministic LR driver.

`generated` is the grammar object (C++: the CRTP Generated class). Subclass
LRGrammar and override what the grammar defines; the has_* methods replace the
C++ detection idioms:
  required   lr_k, lr_action, lr_shift, lr_length, lr_reduce, lr_lhs, lr_goto
  optional   lr_is_space/lr_space, lr_expected, lr_context, lr_token_name,
             lr_rule_label, lr_recover, lr_semantic_reduce
Actions are signed: shift = state+1, reduce = -rule-1, accept = 0; LR_INVALID
marks a missing cell, LR_CONFLICT a cell with several candidate actions
(see lr_conflicts.py). Semantic values are plain values (C++: std::any).

The type-directed capture conversion (lr_convert & co) is expressed with an
explicit `Shape` instead of the target's static type; the typed C++ version
stays in the host runtime."""
from typing import ClassVar, Generic, TypeVar, cast

from ispa_std.failures import logic_error, runtime_error
from ispa_std.format import int_to_string
from ispa_std.lexer import TokenCursor
from ispa_std.match_result import MatchResult
from ispa_std.node import Node
from ispa_std.raw_token import RawToken
from ispa_std.recovery import Diagnostic

V = TypeVar("V")

LR_INVALID = -9223372036854775807
LR_CONFLICT = 9223372036854775807
ABSENT = -1


class LRControlPhase:
    Decide: ClassVar[int] = 0
    Shift: ClassVar[int] = 1
    Reduce: ClassVar[int] = 2
    Semantic: ClassVar[int] = 3
    Recover: ClassVar[int] = 4
    Accept: ClassVar[int] = 5


class LRControlState:
    phase: int
    continuation: int
    production: int

    def __init__(self):
        self.phase = LRControlPhase.Decide
        self.continuation = 0
        self.production = 0


class LRRuleSpan:
    """Token-index extent of a (partial) rule plus where whitespace is allowed."""
    begin: int
    end: int
    leading: bool
    trailing: bool
    empty: bool

    def __init__(self, begin: int, end: int, leading: bool, trailing: bool, empty: bool):
        self.begin = begin
        self.end = end
        self.leading = leading
        self.trailing = trailing
        self.empty = empty

    def copy(self) -> "LRRuleSpan":
        return LRRuleSpan(self.begin, self.end, self.leading, self.trailing, self.empty)


class LRRegionFrame(Generic[V]):
    """A region owns its recognition stack and the values produced inside it.
    The deterministic parser currently uses one root region."""
    pos: TokenCursor
    states: list[int]
    values: list[V]
    # The spans of the stack entries, as parallel lists (an object per entry cost two allocations per step).
    span_begin: list[int]
    span_end: list[int]
    span_leading: list[bool]
    span_trailing: list[bool]
    span_empty: list[bool]
    # The span of the reduction in progress.
    rb: int
    re: int
    rl: bool
    rt: bool
    rx: bool
    capture_destinations: list[int]
    cursor: int
    recovered_prefix: bool
    control: LRControlState

    def __init__(self, start: TokenCursor, continuation: int):
        self.pos = start
        self.states = [0]
        self.values = []
        self.span_begin = []
        self.span_end = []
        self.span_leading = []
        self.span_trailing = []
        self.span_empty = []
        self.rb = 0
        self.re = 0
        self.rl = False
        self.rt = False
        self.rx = True
        self.capture_destinations = []
        self.cursor = 0
        self.recovered_prefix = False
        self.control = LRControlState()
        self.control.continuation = continuation


class LRGrammar(Generic[V]):
    """Base class for generated LR tables. Override the required methods."""

    def lr_k(self) -> int:
        return 1

    def lr_action(self, state: int, window: list[int]) -> int:
        return LR_INVALID

    def lr_conflict(self, state: int, window: list[int]) -> list[int]:
        return []

    def lr_shift(self, token: RawToken) -> V:
        logic_error("LRGrammar.lr_shift is abstract")
        return self.lr_reduce(0, [])

    def lr_length(self, rule: int) -> int:
        return 0

    def lr_reduce(self, rule: int, rhs: list[V]) -> V:
        logic_error("LRGrammar.lr_reduce is abstract")
        return rhs[0]

    def lr_lhs(self, rule: int) -> int:
        return 0

    def lr_goto(self, state: int, lhs: int) -> int:
        return 0

    def has_is_space(self) -> bool:
        return False

    def lr_is_space(self, token: RawToken) -> bool:
        return False

    def has_space(self) -> bool:
        return False

    def lr_space(self, rule: int, index: int) -> bool:
        return False

    def has_expected(self) -> bool:
        return False

    def lr_expected(self, state: int) -> list[int]:
        return []

    def has_context(self) -> bool:
        return False

    def lr_context(self, state: int) -> str:
        return ""

    def has_token_name(self) -> bool:
        return False

    def lr_token_name(self, id: int) -> str:
        return ""

    def has_rule_label(self) -> bool:
        return False

    def lr_rule_label(self, rule: int) -> str:
        return ""

    def has_recover(self) -> bool:
        return False

    def lr_recover(self, region: LRRegionFrame[V], diagnostic: Diagnostic) -> bool:
        return False

    def has_semantic_reduce(self) -> bool:
        return False

    def lr_semantic_reduce(self, rule: int, region: LRRegionFrame[V], span: LRRuleSpan, rhs: list[V]) -> V:
        return self.lr_reduce(rule, rhs)


class LRTokenValue:
    """A shifted terminal: the token and its text. Synthetic grammar symbols
    carry captures, never user-visible AST nodes."""
    node: Node
    text: str

    def __init__(self, node: Node, text: str):
        self.node = node
        self.text = text


def lr_token(node: Node) -> LRTokenValue:
    text = ""
    if node.has_source() and node.length() >= 0:
        text = node.text()
    return LRTokenValue(node, text)


class LRCaptures(Generic[V]):
    """Captured values per destination column; `repeated` marks a repetition."""
    columns: list[list[V]]
    repeated: bool

    def __init__(self, columns: list[list[V]], repeated: bool):
        self.columns = columns
        self.repeated = repeated

    def is_empty(self) -> bool:
        i = 0
        while i < len(self.columns):
            if len(self.columns[i]) > 0:
                return False
            i = i + 1
        return True


def lr_captures_of(value: V) -> LRCaptures[V]:
    """The captures alternative of a value (V is std::any or a closed variant containing LRCaptures[V])."""
    return cast(LRCaptures[V], value)


def lr_pack(values: list[V], indices: list[int], offsets: list[int], splice: list[int], count: int, repeated: bool) -> LRCaptures[V]:
    columns: list[list[V]] = []
    i = 0
    while i < count:
        columns.append([])
        i = i + 1
    i = 0
    while i < len(indices):
        value = values[indices[i]]
        if splice[i] != 0:
            c = 0
            spliced: LRCaptures[V] = lr_captures_of(value)
            while c < len(spliced.columns):
                columns[offsets[i] + c].extend(spliced.columns[c])
                c = c + 1
        else:
            columns[offsets[i]].append(value)
        i = i + 1
    return LRCaptures[V](columns, repeated)


def lr_get(values: list[V], index: int) -> V:
    return values[index]


def lr_node(value: object) -> Node:
    node = Node.blank()
    node.set_data(value)
    return node


# --- the driver ------------------------------------------------------------
class LRParser:
    """Holds the diagnostics of the last lr_parse."""
    errors_: list[Diagnostic]

    def __init__(self):
        self.errors_ = []

    def getErrors(self) -> list[Diagnostic]:
        return self.errors_

    def _skip(self, g: LRGrammar[V], it: TokenCursor, offset: list[int]) -> None:
        if g.has_is_space():
            while (not it.isEnd()) and g.lr_is_space(it.current()):
                it.next()
                if len(offset) > 0:
                    offset[0] = offset[0] + 1

    def _gap(self, g: LRGrammar[V], rule: int, index: int) -> bool:
        if g.has_space():
            return g.lr_space(rule, index)
        return False

    def _window_into(self, g: LRGrammar[V], position: TokenCursor, window: list[int]) -> None:
        # The decision window written into a reused list (no allocation per step for k = 1).
        window.clear()
        if g.lr_k() == 1:
            # The caller has already skipped trivia, so the first token is the current one.
            if position.isEnd():
                window.append(ABSENT)
            else:
                window.append(position.current().kind())
            return
        for kind in self._window(g, position):
            window.append(kind)

    def _window(self, g: LRGrammar[V], position: TokenCursor) -> list[int]:
        peek = position.copy()
        window: list[int] = []
        scratch: list[int] = []
        i = 0
        while i < g.lr_k():
            self._skip(g, peek, scratch)
            end = peek.isEnd()
            if end:
                window.append(ABSENT)
            else:
                window.append(peek.current().kind())
                # Do not step past the last lookahead token: with lazy lexing that would
                # tokenize input the decision does not need (and report its errors early).
                if i + 1 < g.lr_k():
                    peek.next()
            i = i + 1
        return window

    def _unexpected(self, g: LRGrammar[V], states: list[int], window: list[int], position: TokenCursor) -> Diagnostic:
        d = Diagnostic()
        d.unexpected = window[0]
        if g.has_expected():
            d.expected = g.lr_expected(states[len(states) - 1])
        if g.has_context():
            d.rule = g.lr_context(states[len(states) - 1])
        else:
            d.rule = "<rule>"
        if not position.isEnd():
            d.offset = position.current().meta().startpos()
            d.length = position.current().meta().length()
        message = "Unexpected "
        if g.has_token_name():
            message = message + g.lr_token_name(d.unexpected)
        else:
            message = message + "token " + int_to_string(d.unexpected)
        if d.offset != -1:
            message = message + " at offset " + int_to_string(d.offset)
        message = message + " in rule " + d.rule
        if len(d.expected) > 0:
            message = message + "; expected "
            i = 0
            while i < len(d.expected):
                if i > 0:
                    message = message + ", "
                if g.has_token_name():
                    message = message + g.lr_token_name(d.expected[i])
                else:
                    message = message + "token " + int_to_string(d.expected[i])
                i = i + 1
        d.message = message
        d.code = "lr-unexpected-token"
        return d

    def _push_span(self, region: LRRegionFrame[V], begin: int, end: int, leading: bool, trailing: bool, empty: bool) -> None:
        region.span_begin.append(begin)
        region.span_end.append(end)
        region.span_leading.append(leading)
        region.span_trailing.append(trailing)
        region.span_empty.append(empty)

    def _pop_spans(self, region: LRRegionFrame[V], keep: int) -> None:
        while len(region.span_begin) > keep:
            region.span_begin.pop()
            region.span_end.pop()
            region.span_leading.pop()
            region.span_trailing.pop()
            region.span_empty.pop()

    # Computes the span of a reduction of `count` entries into region.rb/re/rl/rt/rx.
    def _reduce_span(self, g: LRGrammar[V], rule: int, count: int, begin: int, region: LRRegionFrame[V]) -> None:
        region.rb = region.cursor
        region.re = region.cursor
        region.rl = False
        region.rx = True
        allowed = self._gap(g, rule, 0)
        i = 0
        while i < count:
            at = begin + i
            if region.span_empty[at]:
                allowed = allowed or region.span_leading[at] or region.span_trailing[at]
            else:
                if region.rx:
                    region.rb = region.span_begin[at]
                    region.rl = allowed or region.span_leading[at]
                elif region.span_begin[at] > region.re and not (allowed or region.span_leading[at]):
                    runtime_error("Whitespace is not allowed between these rule members")
                region.rx = False
                region.re = region.span_end[at]
                allowed = region.span_trailing[at]
            allowed = allowed or self._gap(g, rule, i + 1)
            i = i + 1
        region.rt = allowed
        if region.rx:
            region.rl = allowed

    def lr_parse(self, g: LRGrammar[V], pos: TokenCursor) -> MatchResult:
        self.errors_ = []
        region: LRRegionFrame[V] = LRRegionFrame[V](pos, 0)
        control = region.control
        position = region.pos
        offset: list[int] = [0]
        window: list[int] = []
        rhs: list[V] = []
        while True:
            control.phase = LRControlPhase.Decide
            offset[0] = region.cursor
            self._skip(g, position, offset)
            region.cursor = offset[0]
            self._window_into(g, position, window)
            action = g.lr_action(region.states[len(region.states) - 1], window)
            if action == LR_INVALID:
                self.errors_.append(self._unexpected(g, region.states, window, position))
                recovered = False
                if g.has_recover():
                    control.phase = LRControlPhase.Recover
                    old_cursor = region.cursor
                    old_size = len(region.states)
                    old_state = region.states[len(region.states) - 1]
                    if g.lr_recover(region, self.errors_[len(self.errors_) - 1]):
                        # A handler that reported its own diagnostic replaces the generic one.
                        if self.errors_[len(self.errors_) - 1].code == "lr-superseded":
                            self.errors_.pop()
                        if region.cursor == old_cursor and len(region.states) == old_size \
                                and region.states[len(region.states) - 1] == old_state:
                            runtime_error("LR recovery handler made no progress")
                        if region.cursor != old_cursor:
                            if len(region.span_begin) == 0:
                                region.recovered_prefix = True
                            else:
                                region.span_trailing[len(region.span_trailing) - 1] = True
                        recovered = True
                if not recovered:
                    runtime_error(self.errors_[len(self.errors_) - 1].message)
            elif action == 0:
                control.phase = LRControlPhase.Accept
                if len(region.values) != 1 or not position.isEnd():
                    runtime_error("Invalid LR accept stack")
                at = len(region.span_begin) - 1
                if (region.span_begin[at] != 0 and not region.span_leading[at] and not region.recovered_prefix) \
                        or (region.cursor > region.span_end[at] and not region.span_trailing[at]):
                    runtime_error("Whitespace is not allowed at the input boundary")
                return MatchResult(True, lr_node(region.values[len(region.values) - 1]), position)
            elif action > 0:
                control.phase = LRControlPhase.Shift
                if position.isEnd():
                    runtime_error("LR shift past end of input")
                region.values.append(g.lr_shift(position.current()))
                self._push_span(region, region.cursor, region.cursor + 1, False, False, False)
                region.cursor = region.cursor + 1
                region.states.append(action - 1)
                position.next()
            else:
                rule = -action - 1
                control.phase = LRControlPhase.Reduce
                control.production = rule
                count = g.lr_length(rule)
                if count > len(region.values):
                    runtime_error("LR stack underflow")
                begin = len(region.values) - count
                rhs.clear()
                self._reduce_span(g, rule, count, begin, region)
                value: V
                if g.has_semantic_reduce():
                    # Semantic reductions run at the ACTION table's exact reduce
                    # point. The callback may update the cursor and span, and
                    # may enter an independent nested LR region. The values stay
                    # on the stack for it, so the right-hand side is a copy.
                    i = begin
                    while i < len(region.values):
                        rhs.append(region.values[i])
                        i = i + 1
                    control.phase = LRControlPhase.Semantic
                    span = LRRuleSpan(region.rb, region.re, region.rl, region.rt, region.rx)
                    value = g.lr_semantic_reduce(rule, region, span, rhs)
                    region.rb = span.begin
                    region.re = span.end
                    region.rl = span.leading
                    region.rt = span.trailing
                    region.rx = span.empty
                else:
                    # The right-hand side is moved off the stack: the reduction owns
                    # it, so packed captures can be extended in place, not copied.
                    i = 0
                    while i < count:
                        rhs.append(region.values.pop(begin))
                        i = i + 1
                    value = g.lr_reduce(rule, rhs)
                rhs.clear()
                while len(region.values) > begin:
                    region.values.pop()
                self._pop_spans(region, begin)
                while len(region.states) > begin + 1:
                    region.states.pop()
                nxt = g.lr_goto(region.states[len(region.states) - 1], g.lr_lhs(rule))
                region.values.append(value)
                self._push_span(region, region.rb, region.re, region.rl, region.rt, region.rx)
                region.states.append(nxt)
