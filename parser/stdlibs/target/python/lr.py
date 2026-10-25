"""LR runtime for the ISPA Python target.

Port of ispastdlib/lr_machine.hpp (LRParser), lr_conflicts.hpp (LRGeneralizedParser) and the
lr_* helpers. A generated parser class supplies the tables as static members, exactly like the
C++ `Generated::lr_*` callbacks:

    lr_k() -> int                          lookahead length
    lr_action(state, window) -> int        shift = state+1, reduce = -rule-1, accept = 0,
                                           INVALID_ACTION = error, CONFLICT_ACTION = see lr_conflict
    lr_conflict(state, window) -> list[int]   (generalized parser only)
    lr_goto(state, lhs) -> int
    lr_length(rule) -> int ; lr_lhs(rule) -> int
    lr_shift(token) -> value ; lr_reduce(rule, rhs) -> value
  optional: lr_is_space(token), lr_space(rule, index), lr_expected(state), lr_context(state),
            lr_token_name(kind), lr_rule_label(rule); instance hooks lr_recover(region, error)
            and lr_semantic_reduce(rule, region, span, rhs).
"""
from __future__ import annotations
import enum
from dataclasses import dataclass, field
from typing import Any

from .lexer import Cursor
from .model import MatchResult, Node
from .parser import LLParser
from .recovery import ABSENT, Diagnostic   # ABSENT: numeric_limits<size_t>::max(), end of input

CONFLICT_ACTION = (1 << 63) - 1        # std::numeric_limits<long long>::max()
INVALID_ACTION = -CONFLICT_ACTION


class LRControlPhase(enum.IntEnum):
    Decide = 0
    Shift = 1
    Reduce = 2
    Semantic = 3
    Recover = 4
    Accept = 5


@dataclass(slots=True)
class LRControlState:
    phase: LRControlPhase = LRControlPhase.Decide
    continuation: int = 0
    production: int = 0


@dataclass(slots=True)
class LRRuleSpan:
    begin: int = 0
    end: int = 0
    leading: bool = False
    trailing: bool = False
    empty: bool = True


@dataclass(slots=True)
class LRRegionFrame:
    pos: Cursor
    states: list[int] = field(default_factory=lambda: [0])
    values: list[Any] = field(default_factory=list)
    spans: list[LRRuleSpan] = field(default_factory=list)
    capture_destinations: list[int] = field(default_factory=list)
    cursor: int = 0
    recovered_prefix: bool = False
    control: LRControlState = field(default_factory=LRControlState)


@dataclass(slots=True)
class LRTokenValue:
    node: Any
    text: str


@dataclass(slots=True)
class LRCaptures:
    columns: list[list[Any]]
    repeated: bool = False


# --------------------------------------------------------------------------- helpers

def lr_token(node: Any) -> LRTokenValue:
    """Synthetic grammar symbols carry captures, never user-visible AST nodes."""
    text = ''
    text_of = getattr(node, 'text', None)
    if callable(text_of):
        text = text_of()
    return LRTokenValue(node, text)


def lr_pack(values: list, indices: list[int], offsets: list[int], splice: list[int],
            count: int, repeated: bool = False) -> LRCaptures:
    result = LRCaptures([[] for _ in range(count)], repeated)
    for i, index in enumerate(indices):
        value = values[index]
        if splice[i]:
            if not isinstance(value, LRCaptures):
                raise TypeError('lr_pack: spliced value is not a capture pack')
            for c, column in enumerate(value.columns):
                result.columns[offsets[i] + c].extend(column)
        else:
            result.columns[offsets[i]].append(value)
    return result


def lr_get(values: list, index: int) -> Any:
    return values[index]


def lr_node(value: Any) -> Node:
    node = Node()
    node.set_data(value)
    return node


def _default_like(sample: Any) -> Any:
    if isinstance(sample, list):
        return []
    if isinstance(sample, dict):
        return {}
    if isinstance(sample, str):
        return ''
    if sample is None:
        return None
    try:
        return type(sample)()
    except Exception:
        return None


def lr_convert(target: Any, value: Any) -> tuple[bool, Any]:
    """Counterpart of the C++ type-driven lr_convert. Python annotations are erased, so the
    destination's *current value* plays the role of the C++ target type (variable() initialises
    every declared field with a default of its type). A None sample accepts the value as is."""
    from .model import Variant, RawToken
    alternatives = getattr(type(target), 'alternatives', None) if isinstance(target, Variant) else None

    if isinstance(value, LRTokenValue):
        if isinstance(target, str):
            return True, value.text
        if alternatives is None:
            return lr_convert(target, value.node)
        # A variant may hold a literal char/string alternative as well as typed nodes.
    if target is not None and type(value) is type(target) and alternatives is None:
        return True, value
    if isinstance(value, LRCaptures):
        if all(not column for column in value.columns):
            return True, _default_like(target)
        if isinstance(target, list):
            out: list = []
            if value.repeated and len(value.columns) > 1:
                rows = len(value.columns[0])
                if any(len(column) != rows for column in value.columns):
                    return False, target
                for row in range(rows):
                    out.append(LRCaptures([[column[row]] for column in value.columns]))
                return True, out
            for column in value.columns:
                for item in column:
                    out.append(item.node if isinstance(item, LRTokenValue) and not isinstance(item.node, LRTokenValue) else item)
            return True, out
        if isinstance(target, tuple):
            if len(value.columns) != len(target):
                return False, target
            items = []
            for sample, column in zip(target, value.columns):
                ok, converted = lr_convert(sample, LRCaptures([column]))
                if not ok:
                    return False, target
                items.append(converted)
            return True, tuple(items)
        selected = None
        for column in value.columns:
            for item in column:
                if selected is not None:
                    return False, target
                selected = item
        if selected is None:
            return True, _default_like(target)
        return lr_convert(target, selected)
    if alternatives is not None:
        # First alternative that accepts the value wins, in declaration order.
        for alternative in alternatives:
            sample = _default_like(alternative()) if callable(alternative) and isinstance(alternative, type) else None
            ok, converted = lr_convert(sample, value)
            if ok:
                result = type(target)()
                result.emplace(converted)
                return True, result
        return False, target
    if target is None:
        if isinstance(value, LRTokenValue):
            return True, value.node
        return True, value
    if isinstance(value, RawToken) and not isinstance(target, RawToken):
        return lr_convert(target, value.to_node())
    if isinstance(value, type(target)):
        return True, value
    return False, target


def lr_capture_assign(target: Any, values: list, index: int, column: int | None) -> Any:
    """C++ `lr_capture_assign(T& target, ...)`; Python returns the new value for `target = ...`.
    `column` of None / ABSENT / -1 means the whole symbol rather than one capture column."""
    value = values[index]
    if column is None or column < 0 or column >= ABSENT:
        ok, converted = lr_convert(target, value)
    else:
        if not isinstance(value, LRCaptures):
            raise TypeError('LR capture column requested from a non-capture symbol')
        ok, converted = lr_convert(target, LRCaptures([value.columns[column]], value.repeated))
    if not ok:
        raise RuntimeError('LR capture payload does not match its declared AST field type: '
                           + type(target).__name__)
    return converted


# --------------------------------------------------------------------------- parsers

class LRParser(LLParser):
    """Deterministic LR driver (ISPA_STD::LRParser::lr_parse)."""

    def getErrors(self) -> list[Diagnostic]:
        return self._lr_errors

    @property
    def _lr_errors(self) -> list[Diagnostic]:
        errors = self.__dict__.get('_lr_errors_list')
        if errors is None:
            errors = self.__dict__['_lr_errors_list'] = []
        return errors

    @staticmethod
    def _lr_skip(generated, it: Cursor, offset: list[int] | None) -> None:
        is_space = getattr(generated, 'lr_is_space', None)
        if is_space is None:
            return
        while not it.isEnd() and is_space(it.peek()):
            it.advance()
            if offset is not None:
                offset[0] += 1

    @staticmethod
    def _lr_gap(generated, rule: int, index: int) -> bool:
        gap = getattr(generated, 'lr_space', None)
        return bool(gap(rule, index)) if gap is not None else False

    def _lr_diagnostic(self, generated, region: LRRegionFrame, window: list[int]) -> Diagnostic:
        states, position = region.states, region.pos
        d = Diagnostic(unexpected=window[0])
        expected = getattr(generated, 'lr_expected', None)
        if expected is not None:
            d.expected = list(expected(states[-1]))
        context = getattr(generated, 'lr_context', None)
        d.rule = context(states[-1]) if context is not None else '<rule>'
        if not position.isEnd():
            token = position.peek()
            if hasattr(token, 'startpos') and hasattr(token, 'length'):
                d.offset, d.length = int(token.startpos), int(token.length)
        name = getattr(generated, 'lr_token_name', None)
        label = (lambda k: name(k)) if name is not None else (lambda k: f'token {k}')
        message = 'Unexpected ' + label(d.unexpected)
        if d.offset != ABSENT:
            message += f' at offset {d.offset}'
        message += f' in rule {d.rule}'
        if d.expected:
            message += '; expected ' + ', '.join(label(k) for k in d.expected)
        d.message, d.code = message, 'lr-unexpected-token'
        return d

    def lr_parse(self, generated: Any, pos: Cursor) -> MatchResult:
        generated = generated or type(self)
        errors = self._lr_errors
        errors.clear()
        region = LRRegionFrame(pos)
        states, values, spans, control = region.states, region.values, region.spans, region.control
        position = region.pos
        offset = [0]  # region.cursor, shared with _lr_skip
        k = generated.lr_k()
        while True:
            control.phase = LRControlPhase.Decide
            offset[0] = region.cursor
            self._lr_skip(generated, position, offset)
            region.cursor = offset[0]
            peek = position.copy()
            window: list[int] = []
            for _ in range(k):
                self._lr_skip(generated, peek, None)
                end = peek.isEnd()
                window.append(ABSENT if end else int(peek.peek().kind))
                if not end:
                    peek.advance()
            action = generated.lr_action(states[-1], window)
            if action == INVALID_ACTION:
                errors.append(self._lr_diagnostic(generated, region, window))
                recover = getattr(self, 'lr_recover', None)
                if recover is not None:
                    control.phase = LRControlPhase.Recover
                    old_cursor, old_size, old_state = region.cursor, len(states), states[-1]
                    if recover(region, errors[-1]):
                        if region.cursor == old_cursor and len(states) == old_size and states[-1] == old_state:
                            raise RuntimeError('LR recovery handler made no progress')
                        if region.cursor != old_cursor:
                            if not spans:
                                region.recovered_prefix = True
                            else:
                                spans[-1].trailing = True
                        continue
                raise RuntimeError(errors[-1].message)
            if action == 0:
                control.phase = LRControlPhase.Accept
                if len(values) != 1 or not position.isEnd():
                    raise RuntimeError('Invalid LR accept stack')
                last = spans[-1]
                if (last.begin and not last.leading and not region.recovered_prefix) or \
                        (region.cursor > last.end and not last.trailing):
                    raise RuntimeError('Whitespace is not allowed at the input boundary')
                return MatchResult(True, values[-1], position)
            if action > 0:
                control.phase = LRControlPhase.Shift
                if position.isEnd():
                    raise RuntimeError('LR shift past end of input')
                values.append(generated.lr_shift(position.peek()))
                spans.append(LRRuleSpan(region.cursor, region.cursor + 1, False, False, False))
                region.cursor += 1
                states.append(action - 1)
                position.advance()
                continue
            rule = -action - 1
            control.phase = LRControlPhase.Reduce
            control.production = rule
            count = generated.lr_length(rule)
            if count > len(values):
                raise RuntimeError('LR stack underflow')
            begin = len(values) - count
            rhs = values[begin:]
            span = LRRuleSpan(region.cursor, region.cursor)
            allowed = self._lr_gap(generated, rule, 0)
            for i in range(count):
                child = spans[begin + i]
                if child.empty:
                    allowed = allowed or child.leading or child.trailing
                else:
                    if span.empty:
                        span.begin = child.begin
                        span.leading = allowed or child.leading
                    elif child.begin > span.end and not (allowed or child.leading):
                        raise RuntimeError('Whitespace is not allowed between these rule members')
                    span.empty = False
                    span.end = child.end
                    allowed = child.trailing
                allowed = allowed or self._lr_gap(generated, rule, i + 1)
            span.trailing = allowed
            if span.empty:
                span.leading = allowed
            semantic = getattr(self, 'lr_semantic_reduce', None)
            if semantic is not None:
                # Runs at the ACTION table's exact reduce point and may adjust cursor/span.
                control.phase = LRControlPhase.Semantic
                value = semantic(rule, region, span, rhs)
            else:
                value = generated.lr_reduce(rule, rhs)
            del values[begin:]
            del spans[begin:]
            del states[begin + 1:]
            values.append(value)
            spans.append(span)
            states.append(generated.lr_goto(states[-1], generated.lr_lhs(rule)))


@dataclass(slots=True)
class _Span:
    begin: int = 0
    end: int = 0
    leading: bool = False
    trailing: bool = False
    empty: bool = True


@dataclass(slots=True)
class _Forest:
    span: _Span
    token: Cursor | None
    alternatives: list[tuple[int, tuple[int, ...]]] = field(default_factory=list)


class LRGeneralizedParser(LRParser):
    """Conflict-tolerant driver (ISPA_STD::LRGeneralizedParser): shared stacks plus a packed
    forest; semantic callbacks run only once exactly one derivation survives."""

    NODE_LIMIT = 250_000
    WORK_LIMIT = 2_000_000

    def lr_parse_generalized(self, generated: Any, pos: Cursor) -> MatchResult:
        generated = generated or type(self)
        forest_index: dict[tuple, int] = {}
        stack_index: dict[tuple, int] = {}
        forest: list[_Forest] = []
        stacks: list[tuple[int, int, int]] = [(ABSENT, 0, ABSENT)]  # previous, state, forest
        work = 0
        cursor = [0]

        def tick() -> None:
            nonlocal work
            work += 1
            if work > self.WORK_LIMIT or len(forest) + len(stacks) > self.NODE_LIMIT:
                raise RuntimeError('LR automatic conflict resolution resource limit exceeded')

        def intern_forest(terminal: bool, symbol: int, span: _Span, token: Cursor | None) -> int:
            tick()
            key = (terminal, symbol, span.begin, span.end, span.leading, span.trailing, span.empty)
            index = forest_index.get(key)
            if index is None:
                index = forest_index[key] = len(forest)
                forest.append(_Forest(span, token))
            return index

        def push(previous: int, state: int, value: int) -> int:
            tick()
            key = (previous, state, value)
            index = stack_index.get(key)
            if index is None:
                index = stack_index[key] = len(stacks)
                stacks.append(key)
            return index

        k = generated.lr_k()
        active: set[int] = {0}
        roots: set[int] = set()
        while True:
            self._lr_skip(generated, pos, cursor)
            peek = pos.copy()
            window: list[int] = []
            for _ in range(k):
                self._lr_skip(generated, peek, None)
                end = peek.isEnd()
                window.append(ABSENT if end else int(peek.peek().kind))
                if not end:
                    peek.advance()
            pending = sorted(active)
            closure = set(active)
            shifted: set[int] = set()
            nxt = 0
            while nxt < len(pending):
                tick()
                top_id = pending[nxt]
                nxt += 1
                _prev, top_state, top_forest = stacks[top_id]
                selected = generated.lr_action(top_state, window)
                if selected == CONFLICT_ACTION:
                    actions = list(generated.lr_conflict(top_state, window))
                elif selected != INVALID_ACTION:
                    actions = [selected]
                else:
                    actions = []
                for action in actions:
                    tick()
                    if action == 0:
                        if not pos.isEnd() or top_forest == ABSENT or stacks[top_id][0] != 0:
                            continue
                        span = forest[top_forest].span
                        if (span.begin and not span.leading) or (cursor[0] > span.end and not span.trailing):
                            continue
                        roots.add(top_forest)
                    elif action > 0:
                        if pos.isEnd():
                            continue
                        leaf = intern_forest(True, int(pos.peek().kind),
                                             _Span(cursor[0], cursor[0] + 1, False, False, False), pos.copy())
                        shifted.add(push(top_id, action - 1, leaf))
                    else:
                        rule = -action - 1
                        count = generated.lr_length(rule)
                        children: list[int] = []
                        base = top_id
                        while len(children) < count and base != 0:
                            children.append(stacks[base][2])
                            base = stacks[base][0]
                        if len(children) != count:
                            continue
                        children.reverse()
                        span = _Span(cursor[0], cursor[0])
                        allowed = self._lr_gap(generated, rule, 0)
                        valid = True
                        for i, child_id in enumerate(children):
                            child = forest[child_id].span
                            if child.empty:
                                allowed = allowed or child.leading or child.trailing
                            else:
                                if span.empty:
                                    span.begin = child.begin
                                    span.leading = allowed or child.leading
                                elif child.begin > span.end and not (allowed or child.leading):
                                    valid = False
                                span.empty = False
                                span.end = child.end
                                allowed = child.trailing
                            allowed = allowed or self._lr_gap(generated, rule, i + 1)
                        if not valid:
                            continue
                        span.trailing = allowed
                        if span.empty:
                            span.leading = allowed
                        lhs = generated.lr_lhs(rule)
                        value = intern_forest(False, lhs, span, None)
                        alternative = (rule, tuple(children))
                        if alternative not in forest[value].alternatives:
                            forest[value].alternatives.append(alternative)
                        dest = push(base, generated.lr_goto(stacks[base][1], lhs), value)
                        if dest not in closure:
                            closure.add(dest)
                            pending.append(dest)
            if pos.isEnd():
                break
            if not shifted:
                raise RuntimeError('Unexpected input during LR automatic conflict resolution at token '
                                   + str(cursor[0]))
            active = shifted
            pos.advance()
            cursor[0] += 1
        if not roots:
            raise RuntimeError('Unexpected end of input during LR automatic conflict resolution')

        # Count derivations (saturating at 2) with an iterative post-order walk.
        visiting = [0] * len(forest)
        ways = [0] * len(forest)
        ambiguous = ABSENT
        order: list[int] = []
        for root in sorted(roots):
            if visiting[root] == 2:
                continue
            pending_visits = [[root, 0, 0]]
            visiting[root] = 1
            while pending_visits and ambiguous == ABSENT:
                tick()
                frame = pending_visits[-1]
                node = forest[frame[0]]
                if frame[1] == len(node.alternatives):
                    visiting[frame[0]] = 2
                    order.append(frame[0])
                    pending_visits.pop()
                else:
                    _rule, kids = node.alternatives[frame[1]]
                    if frame[2] == len(kids):
                        frame[1] += 1
                        frame[2] = 0
                    else:
                        child = kids[frame[2]]
                        frame[2] += 1
                        if visiting[child] == 1:
                            ambiguous = child  # productive nullable cycle
                        elif not visiting[child]:
                            visiting[child] = 1
                            pending_visits.append([child, 0, 0])
            if ambiguous != ABSENT:
                break
        if ambiguous != ABSENT:
            total = 2
        else:
            for ident in order:
                n = 1 if forest[ident].token is not None else 0
                for _rule, kids in forest[ident].alternatives:
                    product = 1
                    for child in kids:
                        product = min(2, product * ways[child])
                    n = min(2, n + product)
                ways[ident] = n
                if n > 1 and ambiguous == ABSENT:
                    ambiguous = ident
            total = min(2, sum(ways[root] for root in roots))
        if total != 1:
            message = 'Ambiguous LR input: multiple complete parses'
            if ambiguous != ABSENT:
                node = forest[ambiguous]
                message += f' at tokens {node.span.begin}..{node.span.end}'
                label = getattr(generated, 'lr_rule_label', None)
                for rule, _kids in node.alternatives:
                    message += '\n  ' + (label(rule) if label is not None else f'production {rule}')
            raise RuntimeError(message)

        # Evaluate the surviving tree. No memoisation: a shared epsilon subtree still needs one
        # semantic action per occurrence. Iterative so deep trees do not hit the recursion limit.
        pending_eval: list[list] = [[min(roots), 0, []]]  # node, next child, rhs values
        result: Any = None
        while pending_eval:
            tick()
            frame = pending_eval[-1]
            node = forest[frame[0]]
            if node.token is None and frame[1] < len(node.alternatives[0][1]):
                child = node.alternatives[0][1][frame[1]]
                frame[1] += 1
                pending_eval.append([child, 0, []])
                continue
            if node.token is not None:
                value = generated.lr_shift(node.token.peek())
            else:
                value = generated.lr_reduce(node.alternatives[0][0], frame[2])
            pending_eval.pop()
            if not pending_eval:
                result = value
            else:
                pending_eval[-1][2].append(value)
        return MatchResult(True, result, pos)
