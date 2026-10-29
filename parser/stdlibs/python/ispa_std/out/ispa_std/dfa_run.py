"""dfa_run.hpp: DFA::run, the TDFA interpreter.

One call scans one token. `cur` is the shared cursor (position and line); on
return it sits after the consumed text. `semantic(index, captures, start_pos,
length, start_line, cur)` is the generated accept handler and returns
`Pair(next_state, token)`: NULL_STATE accepts the token, LEXER_DELETE_STATE
drops one character, DYNAMIC_FAIL_STATE abandons the token, anything else
continues the walk. It may advance `cur` (dynamic matchers do)."""
from typing import Callable

from ispa_std import debug
from ispa_std.raw_token import RawToken
from ispa_std.utility import Pair
from ispa_std.chars import at, code_of
from ispa_std.dfa_api import (Action, Captures, DebugIndex, DFADebug, DYNAMIC_FAIL_STATE, HistoryNode,
                              LEXER_DELETE_STATE, NULL_STATE, TdfaLayout, UNSET)
from ispa_std.failures import internal_error, runtime_error
from ispa_std.format import int_to_string, repeat
from ispa_std.refs import Cursor


class DfaTables:
    table: list[list[int]]
    class_table: list[int]
    action_table: list[list[int]]
    layout: TdfaLayout
    debug_array: list[DFADebug]
    debug_index: DebugIndex

    def __init__(self, table: list[list[int]], class_table: list[int], action_table: list[list[int]], layout: TdfaLayout,
                 debug_array: list[DFADebug], debug_index: DebugIndex):
        self.table = table
        self.class_table = class_table
        self.action_table = action_table
        self.layout = layout
        self.debug_array = debug_array
        self.debug_index = debug_index


class TraceSink:
    """Where DFA step traces go (C++: std::cout). Subclass or duck-type `write`."""

    def write(self, text: str) -> None:
        return


_TRACE: list[TraceSink] = [TraceSink()]
_TRACE_ON: list[bool] = [False]


def set_trace_sink(sink: TraceSink) -> None:
    _TRACE[0] = sink
    _TRACE_ON[0] = True


def clear_trace_sink() -> None:
    _TRACE_ON[0] = False


def _trace_step(tables: DfaTables, state: int, cls: int, current_ch: str, pos_ch: str) -> None:
    if not _TRACE_ON[0]:
        return
    sink = _TRACE[0]
    index = -1
    if len(tables.debug_array) > 0:
        index = tables.debug_index.find(state, cls)
    if index >= 0:
        d = tables.debug_array[index]
        head = d.token_name + ": "
        line = head + d.member
        if d.call != -1:
            line = line + "$call" + int_to_string(d.call)
        if d.group != -1:
            line = line + "$group" + int_to_string(d.group)
        line = line + "; " + int_to_string(d.offset + 1) + "/" + int_to_string(d.length) + ";\n"
        sink.write(line)
        pad = repeat(" ", len(head) + d.offset)
        mem = d.member
        marked = False
        if len(mem) >= 2 and mem[0] == "[":
            escaped = False
            i = 1
            while i < len(mem) and not marked:
                c = mem[i]
                if escaped:
                    escaped = False
                    if current_ch == c:
                        sink.write(pad + repeat(" ", i) + "^\n")
                        marked = True
                elif c == "\\":
                    escaped = True
                elif current_ch == c:
                    sink.write(pad + repeat(" ", i) + "^\n")
                    marked = True
                elif c == "-" and i > 1 and i + 1 < len(mem) and mem[i + 1] != "]":
                    if current_ch >= mem[i - 1] and current_ch <= mem[i + 1]:
                        sink.write(pad + repeat(" ", i - 1) + "^ ^\n")
                        marked = True
                i = i + 1
        else:
            sink.write(pad + "^\n")
    else:
        sink.write("State " + int_to_string(state) + " char '" + pos_ch + "'\n")


def run(src: str, cur: Cursor, start_pos: int, tables: DfaTables, semantic: Callable[[int, Captures, int, int, int, Cursor], Pair[int, RawToken]], empty_token: RawToken) -> RawToken:
    """Scan one token starting at cur.pos. Returns the accepted token, or
    `empty_token` when nothing matched (cur is then left where the walk died,
    which the caller treats as no progress)."""
    n = len(src)
    start = cur.pos
    start_line = cur.line
    first = "<end of input>"
    if start < n:
        first = src[start]
    scope = debug.MatchScope("lexer scan", first)

    table = tables.table
    class_table = tables.class_table
    action_table = tables.action_table
    registers = tables.layout.registers
    nstates = len(table)
    nactions = len(action_table)
    trace_on = len(tables.debug_array) > 1 or (len(tables.debug_array) == 1 and tables.debug_array[0].member != "<Undefined>")

    regs = []
    i = 0
    width = registers
    if width < 1:
        width = 1
    while i < width:
        regs.append(UNSET)
        i = i + 1
    history = []

    state = 0
    immediate_action = False
    last_pos = cur.pos
    steps_without_progress = 0
    dead = False

    while state != NULL_STATE and not dead:
        pos_ch = at(src, cur.pos)
        current_ch = pos_ch
        if immediate_action:
            current_ch = at(src, cur.pos + 1)
        if trace_on:
            _trace_step(tables, state, class_table[code_of(current_ch)], current_ch, current_ch)
        if state < nstates:
            if immediate_action:
                immediate_action = False
                if pos_ch == "\n":
                    cur.line = cur.line + 1
                if cur.pos < n:
                    cur.pos = cur.pos + 1
            here_ch = at(src, cur.pos)
            nxt = table[state][class_table[code_of(here_ch)]]
            if nxt == NULL_STATE:
                dead = True
            else:
                state = nxt
                if nxt >= 0 and nxt < nstates and cur.pos < n:
                    if here_ch == "\n":
                        cur.line = cur.line + 1
                    cur.pos = cur.pos + 1
                else:
                    immediate_action = True
        elif state < nstates + nactions:
            row = action_table[state - nstates]
            here = cur.pos - start
            if row[1] >= registers or (row[2] != NULL_STATE and row[2] >= registers):
                scope.close()
                runtime_error("DFA: register out of range")
            op = row[0]
            if op == Action.SET or op == Action.SET_NEXT:
                value = here
                if op == Action.SET_NEXT:
                    value = here + 1
                regs[row[1]] = value
            elif op == Action.COPY:
                regs[row[1]] = regs[row[2]]
            elif op == Action.APPEND or op == Action.APPEND_NEXT:
                mark = here
                if op == Action.APPEND_NEXT:
                    mark = here + 1
                prev = UNSET
                if row[2] != NULL_STATE:
                    prev = regs[row[2]]
                history.append(HistoryNode(mark, prev))
                regs[row[1]] = len(history) - 1
            else:
                scope.close()
                runtime_error("DFA: unknown action; Report this error to github")
            state = row[3]
        else:
            captures = Captures(src, start, regs, history, start_line)
            semantic_pos = cur.pos
            result = semantic(state - nstates - nactions, captures, start_pos, cur.pos - start, start_line, cur)
            if cur.pos != semantic_pos:
                if cur.pos < semantic_pos:
                    scope.close()
                    runtime_error("DFA: semantic matcher moved the cursor backwards")
                k = semantic_pos
                while k < cur.pos and k < n:
                    if src[k] == "\n":
                        cur.line = cur.line + 1
                    k = k + 1
                immediate_action = False
            state = result.first
            if state == DYNAMIC_FAIL_STATE:
                scope.outcome("dynamic matcher failed")
                scope.close()
                return empty_token
            if state == LEXER_DELETE_STATE:
                if cur.pos < n:
                    cur.pos = cur.pos + 1
                scope.success()
                scope.close()
                return empty_token
            if state == NULL_STATE:
                scope.success()
                scope.close()
                return result.second
            if cur.pos == last_pos:
                steps_without_progress = steps_without_progress + 1
                if steps_without_progress > 100:
                    scope.close()
                    internal_error("DFA: possible zero-width transition cycle at state {}, offset {}",
                                   "Lexer TDFA generation", True,
                                   [int_to_string(state), int_to_string(cur.pos - start)])
            else:
                last_pos = cur.pos
                steps_without_progress = 0
    scope.outcome("DFA found no accepting token")
    scope.close()
    return empty_token
