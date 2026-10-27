"""Byte/character-class TDFA engine for generated ISPA Python lexers.

Actions use the enum numbering from the supplied C++ dfa_api.hpp:
0 UNDEF, 1 SET, 2 SET_NEXT, 3 COPY, 4 APPEND, 5 APPEND_NEXT.
This module handles flat state transitions and capture histories. Nested DFA
transitions require additional lowering by the generator.
"""
from __future__ import annotations
from dataclasses import dataclass
from inspect import signature
from typing import Any, Callable, Sequence
from .model import Position

NULL_STATE = -1


def dynamic_match(cursor: Position, alternatives: Sequence[str], negative: bool,
                  quantifier: str = '') -> bool:
    remaining = cursor.source[cursor.index:]
    if negative:
        if quantifier == '*':
            while cursor.peek() != '\0' and not any(
                    cursor.source[cursor.index:].startswith(value) for value in alternatives):
                cursor.advance()
            return True
        if not remaining or any(remaining.startswith(value) for value in alternatives):
            return False
        cursor.advance()
        return True
    for value in alternatives:
        if value and remaining.startswith(value):
            cursor.advance(len(value))
            return True
    return False


@dataclass(frozen=True, slots=True)
class DFADebug:
    member: str
    token_name: str
    position_in_token: int
    call: int = -1
    group: int = -1
    rule_run: int = -1
    offset: int = 0
    length: int = 1
    ch: str = '\0'


@dataclass(slots=True)
class Captures:
    source: str
    base: int
    registers: list[int]
    histories: dict[int, list[int]]
    first_line: int = 1
    history_nodes: list[tuple[int, int]] | None = None

    def is_set(self, index: int) -> bool:
        i = 2 * index
        return i + 1 < len(self.registers) and self.registers[i] >= 0 and self.registers[i + 1] >= 0

    def begin(self, index: int) -> int:
        return self.base + self.registers[2 * index]

    def end(self, index: int) -> int:
        return self.base + self.registers[2 * index + 1]

    def view(self, index: int) -> str:
        if not self.is_set(index):
            return ''
        return self.source[self.begin(index):self.end(index)]

    def text(self, index: int) -> str:
        return self.view(index)

    def character(self, index: int) -> str:
        return self.source[self.begin(index)]

    def list_views(self, index: int) -> list[str]:
        if self.history_nodes is not None:
            offsets = []
            node = self.registers[2 * index]
            while node >= 0:
                offset, node = self.history_nodes[node]
                offsets.append(offset)
            offsets.reverse()
        else:
            offsets = self.histories.get(2 * index, [])
        return [self.source[self.base + a:self.base + b]
                for a, b in zip(offsets[::2], offsets[1::2])]

    def list(self, index: int) -> list[str]:
        return self.list_views(index)

    def line(self, index: int) -> int:
        return self.first_line + self.source.count('\n', self.base, self.begin(index))

    def column(self, index: int) -> int:
        # Search the whole source: the line start usually lies *before* the token base.
        begin = self.begin(index)
        return begin - self.source.rfind('\n', 0, begin)


@dataclass(slots=True)
class DFAResult:
    state: int
    end: int
    captures: Captures
    accepted: bool


def run_dfa(source: str, start: int, transitions: Sequence[Sequence[int]],
            classes: Sequence[int], accepts: Sequence[bool] | set[int],
            action_rows: dict[tuple[int, int], list[tuple[int, int, int]]] | None = None,
            initial: int = 0, registers: int = 0) -> DFAResult:
    """Longest-accept matching. Transition -1 signals failure.

    action_rows are keyed by (state, character_class) and contain (op, a, b)
    tuples. The generated C++ action table may instead encode a flattened
    chain; that encoding must be normalized before calling this routine.
    """
    start_state = state = initial
    regs = [-1] * registers
    histories: dict[int, list[int]] = {}
    def accepted(s: int) -> bool:
        return s in accepts if isinstance(accepts, set) else 0 <= s < len(accepts) and bool(accepts[s])
    best = (start, state, list(regs), {}) if accepted(state) else None
    i = start
    while i < len(source):
        ch = ord(source[i])
        if ch >= len(classes):
            break
        klass = classes[ch]
        if state < 0 or state >= len(transitions) or klass >= len(transitions[state]):
            break
        next_state = transitions[state][klass]
        if next_state == NULL_STATE or next_state < 0:
            break
        for op, a, b in (action_rows or {}).get((state, klass), ()):
            if a >= len(regs):
                regs.extend([-1] * (a + 1 - len(regs)))
            if op == 1: regs[a] = i - start
            elif op == 2: regs[a] = i + 1 - start
            elif op == 3: regs[a] = regs[b] if 0 <= b < len(regs) else -1
            elif op == 4: histories.setdefault(a, []).append(i - start)
            elif op == 5: histories.setdefault(a, []).append(i + 1 - start)
            elif op != 0: raise ValueError(f'unsupported TDFA action {op}')
        state, i = next_state, i + 1
        if accepted(state):
            best = (i, state, regs.copy(), {k: v.copy() for k, v in histories.items()})
    if best is None:
        return DFAResult(start_state, start, Captures(source, start, regs, histories), False)
    end, state, regs, histories = best
    return DFAResult(state, end, Captures(source, start, regs, histories), True)

@dataclass(frozen=True, slots=True)
class TdfaLayout:
    registers: int
    outputs: int


def run_tdfa(source: str, pos: Position, table: Sequence[Sequence[int]],
             class_table: Sequence[int], action_table: Sequence[Sequence[int]],
             layout: TdfaLayout, semantic: Callable,
             first_line: int = 1) -> tuple[Any, int]:
    """Faithful control flow for the supplied C++ `DFA::run` state encoding.

    Lexical states: [0, len(table)). Action states follow lexical states.
    Higher states call semantic(state - lexer_states - action_states, ...).
    `semantic` must return (next_state, token), where -1 returns the token.
    """
    if layout.registers < layout.outputs:
        raise ValueError('TDFA output register count exceeds total registers')
    start = pos.index
    start_line = first_line
    regs = [-1] * layout.registers
    history_nodes: list[tuple[int, int]] = []
    line = first_line
    state = 0
    immediate_action = False
    last_pos = pos.index
    idle_steps = 0
    nstates = len(table)
    nactions = len(action_table)
    semantic_accepts_cursor = len(signature(semantic).parameters) >= 7
    while state != NULL_STATE:
        if state < nstates:
            if immediate_action:
                immediate_action = False
                if pos.peek() == '\n':
                    line += 1
                if pos.peek() != '\0':
                    pos.advance()
            ch = ord(pos.peek())
            if ch >= len(class_table):
                raise ValueError('TDFA char-class table does not cover code point ' + str(ch))
            klass = class_table[ch]
            next_state = table[state][klass]
            if next_state == NULL_STATE or next_state < 0:
                break
            state = next_state
            if next_state < nstates and pos.peek() != '\0':
                if pos.peek() == '\n':
                    line += 1
                pos.advance()
            else:
                immediate_action = True
        elif state < nstates + nactions:
            action, dst, src, next_state = action_table[state - nstates]
            here = pos.index - start
            if dst < 0 or dst >= layout.registers or (src != NULL_STATE and src >= layout.registers):
                raise ValueError('TDFA register out of range')
            if action == 1:
                regs[dst] = here
            elif action == 2:
                regs[dst] = here + 1
            elif action == 3:
                regs[dst] = regs[src]
            elif action in (4, 5):
                history_nodes.append((here + (action == 5), -1 if src == NULL_STATE else regs[src]))
                regs[dst] = len(history_nodes) - 1
            else:
                raise ValueError(f'Unknown TDFA action {action}')
            state = next_state
        else:
            captures = Captures(source, start, regs, {}, first_line, history_nodes)
            semantic_start = pos.index
            semantic_args = (state - nstates - nactions, captures, start, source,
                             pos.index - start, first_line)
            next_state, token = (semantic(*semantic_args, pos) if semantic_accepts_cursor
                                 else semantic(*semantic_args))
            if pos.index < semantic_start:
                raise RuntimeError('TDFA semantic matcher moved the cursor backwards')
            if pos.index != semantic_start:
                line += source[semantic_start:pos.index].count('\n')
                immediate_action = False
            state = next_state
            if state == NULL_STATE:
                if token is None and pos.index == start:
                    return None, start_line
                return token, line
            if pos.index == last_pos:
                idle_steps += 1
                if idle_steps > 100:
                    raise RuntimeError('Possible zero-width TDFA cycle')
            else:
                last_pos = pos.index
                idle_steps = 0
    # Failure: rewind so a failed attempt never silently swallows input. The lexer
    # then reports "no progress" with a position instead of dropping characters.
    pos.index = start
    return None, start_line
