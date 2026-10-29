"""lr_machine.hpp: the deterministic LR driver.

`generated` is the grammar object (C++: the CRTP Generated class). Subclass
LRGrammar and override what the grammar defines; the has_* methods replace the
C++ detection idioms:
  required   lr_k, lr_action, lr_shift, lr_length, lr_reduce, lr_lhs, lr_goto
  optional   lr_is_space/lr_space, lr_expected, lr_context, lr_token_name,
             lr_rule_label, lr_recover, lr_semantic_reduce
Actions are signed: shift = state+1, reduce = -rule-1, accept = 0; LR_INVALID
marks a missing cell, LR_CONFLICT a cell with several candidate actions
(see lr_glr.py). Semantic values are plain values (C++: std::any).

The type-directed capture conversion (lr_convert & co) is expressed with an
explicit `Shape` instead of the target's static type; the typed C++ version
stays in cpp_residue."""
from ispa_std import reflect
from ispa_std.failures import runtime_error
from ispa_std.format import int_to_string
from ispa_std.node import MatchResult, Node
from ispa_std.recovery import Diagnostic

LR_INVALID = -9223372036854775807
LR_CONFLICT = 9223372036854775807
ABSENT = -1


class LRControlPhase:
    Decide = 0
    Shift = 1
    Reduce = 2
    Semantic = 3
    Recover = 4
    Accept = 5


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


class LRRegionFrame:
    """A region owns its recognition stack and the values produced inside it.
    The deterministic parser currently uses one root region."""
    pos: object
    states: list
    values: list
    spans: list
    capture_destinations: list
    cursor: int
    recovered_prefix: bool
    control: LRControlState

    def __init__(self, start, continuation: int):
        self.pos = start
        self.states = [0]
        self.values = []
        self.spans = []
        self.capture_destinations = []
        self.cursor = 0
        self.recovered_prefix = False
        self.control = LRControlState()
        self.control.continuation = continuation


class LRGrammar:
    """Base class for generated LR tables. Override the required methods."""

    def lr_k(self) -> int:
        return 1

    def lr_action(self, state: int, window: list) -> int:
        return LR_INVALID

    def lr_conflict(self, state: int, window: list) -> list:
        return []

    def lr_shift(self, token):
        return token

    def lr_length(self, rule: int) -> int:
        return 0

    def lr_reduce(self, rule: int, rhs: list):
        return 0

    def lr_lhs(self, rule: int) -> int:
        return 0

    def lr_goto(self, state: int, lhs: int) -> int:
        return 0

    def has_is_space(self) -> bool:
        return False

    def lr_is_space(self, token) -> bool:
        return False

    def has_space(self) -> bool:
        return False

    def lr_space(self, rule: int, index: int) -> bool:
        return False

    def has_expected(self) -> bool:
        return False

    def lr_expected(self, state: int) -> list:
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

    def lr_recover(self, region: LRRegionFrame, diagnostic: Diagnostic) -> bool:
        return False

    def has_semantic_reduce(self) -> bool:
        return False

    def lr_semantic_reduce(self, rule: int, region: LRRegionFrame, span: LRRuleSpan, rhs: list):
        return 0


class LRTokenValue:
    """A shifted terminal: the token and its text. Synthetic grammar symbols
    carry captures, never user-visible AST nodes."""
    node: object
    text: str

    def __init__(self, node, text: str):
        self.node = node
        self.text = text


def lr_token(node) -> LRTokenValue:
    text = ""
    if node.has_source() and node.length() >= 0:
        text = node.text()
    return LRTokenValue(node, text)


class LRCaptures:
    """Captured values per destination column; `repeated` marks a repetition."""
    columns: list
    repeated: bool

    def __init__(self, columns: list, repeated: bool):
        self.columns = columns
        self.repeated = repeated

    def is_empty(self) -> bool:
        i = 0
        while i < len(self.columns):
            if len(self.columns[i]) > 0:
                return False
            i = i + 1
        return True


def lr_pack(values: list, indices: list, offsets: list, splice: list, count: int, repeated: bool) -> LRCaptures:
    columns = []
    i = 0
    while i < count:
        columns.append([])
        i = i + 1
    i = 0
    while i < len(indices):
        value = values[indices[i]]
        if splice[i] != 0:
            c = 0
            while c < len(value.columns):
                columns[offsets[i] + c].extend(value.columns[c])
                c = c + 1
        else:
            columns[offsets[i]].append(value)
        i = i + 1
    return LRCaptures(columns, repeated)


def lr_get(values: list, index: int):
    return values[index]


def lr_node(value) -> Node:
    node = Node.blank()
    node.set_data(value)
    return node


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
    parts: list

    def __init__(self, kind: int, parts: list):
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
    def tuple_of(elements: list) -> "Shape":
        return Shape(ShapeKind.TUPLE, elements)

    @staticmethod
    def variant_of(alternatives: list) -> "Shape":
        return Shape(ShapeKind.VARIANT, alternatives)


class Converted:
    ok: bool
    value: object

    def __init__(self, ok: bool, value):
        self.ok = ok
        self.value = value


def _is_token_value(v) -> bool:
    return reflect.has_attr(v, "text") and reflect.has_attr(v, "node") and not reflect.has_attr(v, "columns")


def _is_captures(v) -> bool:
    return reflect.has_attr(v, "columns") and reflect.has_attr(v, "repeated")


def _empty_for(shape: Shape):
    if shape.kind == ShapeKind.TEXT:
        return ""
    if shape.kind == ShapeKind.CHAR:
        return ""
    if shape.kind == ShapeKind.LIST:
        return []
    return Node.blank()


def lr_convert(shape: Shape, value) -> Converted:
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

    def __init__(self, index: int, value):
        self.index = index
        self.value = value


def lr_capture_assign(shape: Shape, values: list, index: int, column: int) -> object:
    """Convert values[index] (or one of its columns; column < 0 = whole value)
    to `shape`, or fail when the payload does not match its declared field."""
    value = values[index]
    if column >= 0:
        value = LRCaptures([value.columns[column]], value.repeated)
    result = lr_convert(shape, value)
    if not result.ok:
        runtime_error("LR capture payload does not match its declared AST field type")
    return result.value


# --- the driver ------------------------------------------------------------
class LRParser:
    """Holds the diagnostics of the last lr_parse."""
    errors_: list

    def __init__(self):
        self.errors_ = []

    def getErrors(self) -> list:
        return self.errors_

    def _skip(self, g: LRGrammar, it, offset: list) -> None:
        if g.has_is_space():
            while (not it.isEnd()) and g.lr_is_space(it.current()):
                it.next()
                if len(offset) > 0:
                    offset[0] = offset[0] + 1

    def _gap(self, g: LRGrammar, rule: int, index: int) -> bool:
        if g.has_space():
            return g.lr_space(rule, index)
        return False

    def _window(self, g: LRGrammar, position) -> list:
        peek = position.copy()
        window = []
        i = 0
        while i < g.lr_k():
            self._skip(g, peek, [])
            end = peek.isEnd()
            if end:
                window.append(ABSENT)
            else:
                window.append(peek.current().kind())
                peek.next()
            i = i + 1
        return window

    def _unexpected(self, g: LRGrammar, states: list, window: list, position) -> Diagnostic:
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

    def _reduce_span(self, g: LRGrammar, rule: int, count: int, begin: int, spans: list, cursor: int) -> LRRuleSpan:
        span = LRRuleSpan(cursor, cursor, False, False, True)
        allowed = self._gap(g, rule, 0)
        i = 0
        while i < count:
            child = spans[begin + i]
            if child.empty:
                allowed = allowed or child.leading or child.trailing
            else:
                if span.empty:
                    span.begin = child.begin
                    span.leading = allowed or child.leading
                elif child.begin > span.end and not (allowed or child.leading):
                    runtime_error("Whitespace is not allowed between these rule members")
                span.empty = False
                span.end = child.end
                allowed = child.trailing
            allowed = allowed or self._gap(g, rule, i + 1)
            i = i + 1
        span.trailing = allowed
        if span.empty:
            span.leading = allowed
        return span

    def lr_parse(self, g: LRGrammar, pos) -> MatchResult:
        self.errors_ = []
        region = LRRegionFrame(pos, 0)
        states = region.states
        values = region.values
        spans = region.spans
        control = region.control
        position = region.pos
        while True:
            control.phase = LRControlPhase.Decide
            offset = [region.cursor]
            self._skip(g, position, offset)
            region.cursor = offset[0]
            window = self._window(g, position)
            action = g.lr_action(states[len(states) - 1], window)
            if action == LR_INVALID:
                self.errors_.append(self._unexpected(g, states, window, position))
                recovered = False
                if g.has_recover():
                    control.phase = LRControlPhase.Recover
                    old_cursor = region.cursor
                    old_size = len(states)
                    old_state = states[len(states) - 1]
                    if g.lr_recover(region, self.errors_[len(self.errors_) - 1]):
                        if region.cursor == old_cursor and len(states) == old_size \
                                and states[len(states) - 1] == old_state:
                            runtime_error("LR recovery handler made no progress")
                        if region.cursor != old_cursor:
                            if len(spans) == 0:
                                region.recovered_prefix = True
                            else:
                                spans[len(spans) - 1].trailing = True
                        recovered = True
                if not recovered:
                    runtime_error(self.errors_[len(self.errors_) - 1].message)
            elif action == 0:
                control.phase = LRControlPhase.Accept
                if len(values) != 1 or not position.isEnd():
                    runtime_error("Invalid LR accept stack")
                last = spans[len(spans) - 1]
                if (last.begin != 0 and not last.leading and not region.recovered_prefix) \
                        or (region.cursor > last.end and not last.trailing):
                    runtime_error("Whitespace is not allowed at the input boundary")
                return MatchResult(True, values[len(values) - 1], position)
            elif action > 0:
                control.phase = LRControlPhase.Shift
                if position.isEnd():
                    runtime_error("LR shift past end of input")
                values.append(g.lr_shift(position.current()))
                spans.append(LRRuleSpan(region.cursor, region.cursor + 1, False, False, False))
                region.cursor = region.cursor + 1
                states.append(action - 1)
                position.next()
            else:
                rule = -action - 1
                control.phase = LRControlPhase.Reduce
                control.production = rule
                count = g.lr_length(rule)
                if count > len(values):
                    runtime_error("LR stack underflow")
                begin = len(values) - count
                rhs = []
                i = begin
                while i < len(values):
                    rhs.append(values[i])
                    i = i + 1
                span = self._reduce_span(g, rule, count, begin, spans, region.cursor)
                if g.has_semantic_reduce():
                    # Semantic reductions run at the ACTION table's exact reduce
                    # point. The callback may update the cursor and span, and
                    # may enter an independent nested LR region.
                    control.phase = LRControlPhase.Semantic
                    value = g.lr_semantic_reduce(rule, region, span, rhs)
                else:
                    value = g.lr_reduce(rule, rhs)
                while len(values) > begin:
                    values.pop()
                while len(spans) > begin:
                    spans.pop()
                while len(states) > begin + 1:
                    states.pop()
                nxt = g.lr_goto(states[len(states) - 1], g.lr_lhs(rule))
                values.append(value)
                spans.append(span)
                states.append(nxt)
