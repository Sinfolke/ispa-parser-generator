"""parser_ll.hpp: LLParser - the combinators generated LL parsers are built from,
plus local and hierarchical (context) error recovery.

The C++ class is a template over (MainNode, Token) with the combinators
templated on the NodeStruct type being matched. Here a "node struct" is the
Int `type_id` the token alternative list assigns, so `required(pos, type_id,
fail)` replaces `required<NodeStruct>(pos, fail)`, and a rule is an ordinary
callable `rec(pos) -> MatchResult`.

C++ overload sets become separate names:
  required_token / required_rule          required<NodeStruct>(pos, ...)
  optional_token / optional_rule          optional<NodeStruct>(pos, ...)
  range_token / range_rule                range<NodeStruct>(pos, ...)
  range_optional_token / range_optional_rule
  counted_token / counted_rule
`fail` arguments are recovery.FailHandler objects (Handler, ActionFailure or
LegacyFail), which is how C++ distinguished them by signature.

RAII becomes explicit: `recovery_frame(rule)` returns a FrameGuard to
`close()`, `counter_scope()` a CounterScope to `restore()`, and a probe is
`begin_probe()`/`end_probe()`. Every path that leaves a rule must close them,
including the NoMatch path, which is why probing goes through prim.attempt."""
from ispa_std import debug, prim, recovery
from ispa_std.counter import CountSpec
from ispa_std.errors import ErrorKind
from ispa_std.failures import invalid_argument, length_error, logic_error, no_match, out_of_range, rethrow
from ispa_std.format import fmt, int_to_string, replace_all
from ispa_std.lexer import Lexer_base, TokenIterator, is_token_end
from ispa_std.node import MatchResult, Node
from ispa_std.parser_base import Parser_base
from ispa_std.scan import scan_balanced
from ispa_std.sortedmap import StrIntMap
from ispa_std.string import ends_with, substr
from ispa_std.utility import maximum, minimum

MAX_ACTION_LOOKAHEAD = 64
MAX_ACTION_CHECKPOINTS = 32
NPOS = -1


def lookahead_kind(pos) -> int:
    """Recovery::TokenId of the token at pos (EOF_TOKEN at end of input)."""
    if pos.isEnd():
        return recovery.END_OF_INPUT
    return pos.current().kind()


def cursor_position(pos) -> int:
    return pos.distance()


def parse_token_stream(lexer: Lexer_base, tokens: list, rule) -> Node:
    """Run a generated rule against a separate token stream. The returned node
    owns its data; the temporary cursor never escapes with the result."""
    i = 0
    while i < len(tokens):
        lexer.append_token(tokens[i])
        i = i + 1
    lexer.push_eof()
    begin = TokenIterator.begin(lexer)
    result = rule(begin)
    if (not result.status) or (not result.it.isEnd()):
        return Node.blank()
    return result.node


def parse_source_span(lexer: Lexer_base, tokens: list, rule) -> Node:
    """External rules need the original source bytes. Tokens collected from a
    contiguous source region provide its bounds while retaining the source
    positions in the returned AST."""
    from ispa_std.lexer import LazyIterator
    if len(tokens) == 0:
        return Node.blank()
    first = tokens[0].meta()
    last = tokens[len(tokens) - 1].meta()
    if (not first.has_source()) or (not last.has_source()) or last.end_pos() < first.startpos():
        invalid_argument("source stream requires tokens with source spans")
    lexer.setInput(first.source())
    lexer.set_bound(last.end_pos())
    begin = LazyIterator.at_position(lexer, first.startpos())
    result = rule(begin)
    if (not result.status) or (not result.it.isEnd()) or result.it.sourcePosition() > last.end_pos():
        return Node.blank()
    return result.node


class ActionToken:
    """A token as a parser action sees it (LLParser::ActionToken)."""
    raw: object
    text: str
    name: str
    kind: int
    offset: int
    length: int
    line: int
    column: int
    present: bool

    def __init__(self):
        self.raw = 0
        self.text = ""
        self.name = ""
        self.kind = NPOS
        self.offset = 0
        self.length = 0
        self.line = 0
        self.column = 0
        self.present = False

    def empty(self) -> bool:
        return not self.present

    def matches_name(self, expected: str) -> bool:
        if not self.present:
            return False
        normalized = replace_all(expected, "::", ".")
        if self.name == normalized:
            return True
        return len(self.name) > len(normalized) and ends_with(self.name, normalized) \
            and self.name[len(self.name) - len(normalized) - 1] == "."


class TokenBoundary:
    """A sync boundary: source spelling, or a token name when token_name."""
    value: str
    token_name: bool

    def __init__(self, value: str, token_name: bool):
        self.value = value
        self.token_name = token_name


def spelling(value: str) -> TokenBoundary:
    return TokenBoundary(value, False)


def token_boundary(name: str) -> TokenBoundary:
    return TokenBoundary(name, True)


class TokenNames:
    """`const char* (*token_name)(std::size_t)`: id -> dotted token name.
    `lookup(id)` returns "" when the id has no spelling."""
    names: list

    def __init__(self, names: list):
        self.names = names

    def lookup(self, id: int) -> str:
        if id < 0 or id >= len(self.names):
            return ""
        return self.names[id]


NO_TOKEN_NAMES = TokenNames([])


def context_peek(pos, lookahead: int, token_names: TokenNames, ambiguous_id: int) -> ActionToken:
    if lookahead > MAX_ACTION_LOOKAHEAD:
        out_of_range("ctx.peek lookahead exceeds 64 tokens")
    candidate = pos.copy()
    i = 0
    while i < lookahead:
        if candidate.isEnd():
            return ActionToken()
        candidate.next()
        i = i + 1
    if candidate.isEnd():
        return ActionToken()
    token = candidate.current()
    meta = token.meta()
    result = ActionToken()
    result.raw = token
    result.kind = recovery.token_id(token)
    result.offset = meta.startpos()
    result.length = meta.length()
    result.line = meta.line()
    result.column = meta.column()
    result.present = True
    if meta.has_source():
        result.text = meta.text()
    if result.kind != ambiguous_id:
        result.name = token_names.lookup(result.kind)
    return result


def context_prev(pos, start_pos, token_names: TokenNames, ambiguous_id: int) -> ActionToken:
    """Step pos back one token (never before start_pos) and return it."""
    if cursor_position(pos) <= cursor_position(start_pos):
        return ActionToken()
    previous = pos.copy()
    previous.prev()
    if cursor_position(previous) >= cursor_position(pos) or cursor_position(previous) < cursor_position(start_pos):
        return ActionToken()
    pos.assign(previous)
    return context_peek(pos, 0, token_names, ambiguous_id)


def context_at(pos, text: str, lookahead: int, token_names: TokenNames, match_name: bool,
               ambiguous_id: int) -> bool:
    if lookahead > MAX_ACTION_LOOKAHEAD:
        out_of_range("ctx.at lookahead exceeds 64 tokens")
    candidate = pos.copy()
    i = 0
    while i < lookahead:
        if candidate.isEnd():
            return False
        candidate.next()
        i = i + 1
    if candidate.isEnd():
        return False
    meta = candidate.current().meta()
    if not match_name:
        return meta.has_source() and meta.text() == text
    if recovery.token_id(candidate.current()) == ambiguous_id:
        return False
    name = replace_all(text, "::", ".")
    actual = token_names.lookup(recovery.token_id(candidate.current()))
    if actual == name:
        return True
    return len(actual) > len(name) and ends_with(actual, name) \
        and actual[len(actual) - len(name) - 1] == "."


class ActionCheckpoint:
    """Named cursor checkpoints a rule or fail action can mark and restore.
    Contexts hold the live cursor, so restoring writes through to it."""
    pos: object
    names: list
    saved: list

    def __init__(self, pos):
        self.pos = pos
        self.names = []
        self.saved = []

    def mark(self, name: str) -> None:
        if len(name) == 0:
            invalid_argument("checkpoint name cannot be empty")
        if len(self.names) >= MAX_ACTION_CHECKPOINTS:
            length_error("action checkpoint limit (32) exceeded")
        self.names.append(name)
        self.saved.append(self.pos.copy())

    def _find_last(self, name: str) -> int:
        i = len(self.names) - 1
        while i >= 0:
            if self.names[i] == name:
                return i
            i = i - 1
        return -1

    def restore(self, name: str) -> None:
        index = self._find_last(name)
        if index == -1:
            logic_error("unknown action checkpoint '" + name + "'")
        self.pos.assign(self.saved[index])
        while len(self.names) > index + 1:
            self.names.pop()
            self.saved.pop()

    def commit(self, name: str) -> None:
        index = self._find_last(name)
        if index == -1:
            logic_error("unknown action checkpoint '" + name + "'")
        while len(self.names) > index:
            self.names.pop()
            self.saved.pop()


class RuleContext(ActionCheckpoint):
    """`ctx` inside a rule action. Cursor and AST changes are immediately
    visible to subsequent matches and to the rule's caller."""
    ast: object
    start_pos: object
    rule: str
    token_names: TokenNames
    ambiguous_id: int

    def __init__(self, pos, ast, start_pos, rule: str, token_names: TokenNames, ambiguous_id: int):
        ActionCheckpoint.__init__(self, pos)
        self.ast = ast
        self.start_pos = start_pos
        self.rule = rule
        self.token_names = token_names
        self.ambiguous_id = ambiguous_id

    def at(self, text: str, lookahead: int) -> bool:
        return context_at(self.pos, text, lookahead, self.token_names, False, self.ambiguous_id)

    def at_token(self, name: str, lookahead: int) -> bool:
        return context_at(self.pos, name, lookahead, self.token_names, True, self.ambiguous_id)

    def peek(self, lookahead: int) -> ActionToken:
        return context_peek(self.pos, lookahead, self.token_names, self.ambiguous_id)

    def take(self) -> ActionToken:
        token = self.peek(0)
        if not token.empty():
            self.pos.next()
        return token

    def prev(self) -> ActionToken:
        return context_prev(self.pos, self.start_pos, self.token_names, self.ambiguous_id)

    def collect_braced(self) -> list:
        """Collect a braced region's contents after its opening token has been
        consumed. The closing token is consumed but not included; nested braces
        are preserved."""
        collected = []
        if self.pos.has_source():
            begin = self.pos.sourcePosition()
            origin = self.pos.sourceOrigin()
            scanned = scan_balanced(origin, begin, ["}"], len(origin) - begin)
            if scanned.exhausted or scanned.end >= len(origin) or origin[scanned.end] != "}":
                from ispa_std.failures import runtime_error
                runtime_error("unterminated braced region in parser action")
            while (not self.pos.isEnd()) and self.pos.sourcePosition() < scanned.end:
                collected.append(self.pos.current())
                self.pos.next()
            if self.pos.isEnd() or self.pos.sourcePosition() != scanned.end or not self.at("}", 0):
                from ispa_std.failures import runtime_error
                runtime_error("braced region boundary is not a token")
            self.pos.next()
            return collected
        depth = 1
        while not self.pos.isEnd():
            if self.at("{", 0):
                depth = depth + 1
            elif self.at("}", 0):
                depth = depth - 1
                if depth == 0:
                    self.pos.next()
                    return collected
            collected.append(self.pos.current())
            self.pos.next()
        from ispa_std.failures import runtime_error
        runtime_error("unterminated braced region in parser action")
        return collected


class FailContext(ActionCheckpoint):
    """`ctx` inside a `fail` block: the same queries plus the repair requests."""
    ast: object
    start_pos: object
    rule: str
    failed_rule: str
    recovery_state: recovery.State
    source: str
    has_source_text: bool
    diagnostic_begin: int
    token_names: TokenNames
    ambiguous_id: int
    requested: int
    has_request: bool
    skipped: int

    def __init__(self, pos, ast, start_pos, rule: str, failed_rule: str, state: recovery.State,
                 source: str, has_source_text: bool, diagnostic_begin: int, token_names: TokenNames,
                 ambiguous_id: int):
        ActionCheckpoint.__init__(self, pos)
        self.ast = ast
        self.start_pos = start_pos
        self.rule = rule
        self.failed_rule = failed_rule
        self.recovery_state = state
        self.source = source
        self.has_source_text = has_source_text
        self.diagnostic_begin = diagnostic_begin
        self.token_names = token_names
        self.ambiguous_id = ambiguous_id
        self.requested = recovery.Mode.Propagate
        self.has_request = False
        self.skipped = 0

    def at(self, text: str, lookahead: int) -> bool:
        return context_at(self.pos, text, lookahead, self.token_names, False, self.ambiguous_id)

    def at_token(self, name: str, lookahead: int) -> bool:
        return context_at(self.pos, name, lookahead, self.token_names, True, self.ambiguous_id)

    def peek(self, lookahead: int) -> ActionToken:
        return context_peek(self.pos, lookahead, self.token_names, self.ambiguous_id)

    def take(self) -> ActionToken:
        token = self.peek(0)
        if not token.empty():
            self.pos.next()
        return token

    def prev(self) -> ActionToken:
        return context_prev(self.pos, self.start_pos, self.token_names, self.ambiguous_id)

    def error(self, message: str, code: str, source_offset: int, source_length: int) -> None:
        """Custom diagnostics share the collection with automatic repairs.
        Pass source_offset = -1 to take the span from the current token."""
        if (source_offset < 0) != (source_length < 0):
            invalid_argument("ctx.error source span requires both offset and length")
        if self.recovery_state.exhausted():
            self.abort()
            return
        diagnostic = recovery.Diagnostic()
        diagnostic.rule = self.rule
        diagnostic.expected_text = self.failed_rule
        diagnostic.message = message
        diagnostic.code = code
        if self.pos.has_source():
            # Do not force host tokenization of a broken foreign payload.
            p = self.pos.sourcePosition()
            if p >= 0:
                diagnostic.offset = p
                diagnostic.length = 0
                if p < len(self.pos.sourceOrigin()):
                    diagnostic.length = 1
        elif not self.pos.isEnd():
            diagnostic.unexpected = recovery.token_id(self.pos.current())
            diagnostic.offset = self.pos.current().meta().startpos()
            diagnostic.length = self.pos.current().meta().length()
        elif self.has_source_text:
            diagnostic.offset = len(self.source)
        if source_offset >= 0:
            input_length = 0
            if self.has_source_text:
                input_length = len(self.source)
            if source_offset > input_length or source_length > input_length - source_offset:
                out_of_range("ctx.error source span is outside parser input")
            diagnostic.offset = source_offset
            diagnostic.length = source_length
        self.recovery_state.diagnostics.append(diagnostic)

    def note(self, message: str) -> None:
        if len(self.recovery_state.diagnostics) <= self.diagnostic_begin:
            logic_error("ctx.note requires a preceding ctx.error in this fail block")
        self.recovery_state.diagnostics[len(self.recovery_state.diagnostics) - 1].notes.append(message)

    def sync(self, boundaries: list) -> None:
        """Skip source text to one of `boundaries` (Strings). Lazy cursors only."""
        if len(boundaries) == 0:
            logic_error("ctx.sync requires at least one boundary")
        if self.recovery_state.exhausted() or (self.has_request and self.requested == recovery.Mode.Abort):
            self.abort()
            return
        i = 0
        while i < len(boundaries):
            if len(boundaries[i]) == 0:
                invalid_argument("ctx.sync requires nonempty boundaries")
            i = i + 1
        if not self.pos.has_source():
            logic_error("ctx.sync requires a lazy source cursor")
            return
        begin = self.pos.sourcePosition()
        budget = minimum(4096, self.recovery_state.max_total_skipped - self.recovery_state.total_skipped)
        scanned = scan_balanced(self.pos.sourceOrigin(), begin, boundaries, budget)
        count = scanned.end - begin
        if count > 0:
            self.pos.consumeExternal(scanned.end)
        self.skipped = self.skipped + count
        self.recovery_state.total_skipped = self.recovery_state.total_skipped + count
        if scanned.exhausted or self.recovery_state.exhausted():
            self.abort()

    def sync_tokens(self, boundaries: list) -> None:
        """Skip whole tokens to one of `boundaries` (TokenBoundary), preserving
        token-stream structure (including synthetic indentation)."""
        if len(boundaries) == 0:
            logic_error("ctx.sync_tokens requires at least one boundary")
        i = 0
        while i < len(boundaries):
            if len(boundaries[i].value) == 0:
                invalid_argument("ctx.sync_tokens requires nonempty boundaries")
            i = i + 1
        while True:
            if self.recovery_state.exhausted() or (self.has_request and self.requested == recovery.Mode.Abort):
                self.abort()
                return
            if self.pos.isEnd():
                return
            j = 0
            while j < len(boundaries):
                stop = boundaries[j]
                if stop.token_name:
                    if self.at_token(stop.value, 0):
                        return
                elif self.at(stop.value, 0):
                    return
                j = j + 1
            self.skip()

    def skip(self) -> None:
        if self.recovery_state.exhausted():
            self.abort()
            return
        if not self.pos.isEnd():
            self.pos.next()
            self.skipped = self.skipped + 1
            self.recovery_state.total_skipped = self.recovery_state.total_skipped + 1

    def insert(self) -> None:
        if not (self.has_request and self.requested == recovery.Mode.Abort):
            self.requested = recovery.Mode.Insert
            self.has_request = True

    def replace(self) -> None:
        self.skip()
        self.insert()

    def retry(self) -> None:
        if not (self.has_request and self.requested == recovery.Mode.Abort):
            self.requested = recovery.Mode.Retry
            self.has_request = True

    def abort(self) -> None:
        self.requested = recovery.Mode.Abort
        self.has_request = True

    def finish(self, before) -> recovery.Decision:
        begin = cursor_position(before)
        end = cursor_position(self.pos)
        moved = 0
        if end > begin:
            moved = end - begin
        # Also account for handlers that change ctx.pos directly.
        if moved > self.skipped:
            self.recovery_state.total_skipped = self.recovery_state.total_skipped + (moved - self.skipped)
        decision = recovery.Decision.default()
        if self.recovery_state.exhausted():
            decision.mode = recovery.Mode.Abort
        elif self.has_request:
            decision.mode = self.requested
        elif begin == end:
            decision.mode = recovery.Mode.Insert
        else:
            decision.mode = recovery.Mode.Retry
        if decision.mode == recovery.Mode.Insert and moved > 0:
            decision.mode = recovery.Mode.Replace
        decision.skipped = maximum(moved, self.skipped)
        i = self.diagnostic_begin
        while i < len(self.recovery_state.diagnostics):
            diagnostic = self.recovery_state.diagnostics[i]
            diagnostic.decision = decision.copy()
            diagnostic.decision.unexpected = diagnostic.unexpected
            i = i + 1
        return decision


class Frame:
    """One rule invocation's recovery frame."""
    rule: str
    expecting: str
    claims: list

    def __init__(self, rule: str):
        self.rule = rule
        self.expecting = ""
        self.claims = []


class FrameGuard:
    parser: object
    open: bool

    def __init__(self, parser, rule: str):
        self.parser = parser
        self.open = True
        parser.frames_.append(Frame(rule))
        parser.recovery_state_.anchors.append([])

    def close(self) -> None:
        # Abandoned frames count down the unwind request; the frame that is the
        # resume target is the first one that sees it reach zero.
        if not self.open:
            return
        self.open = False
        self.parser.frames_.pop()
        self.parser.recovery_state_.anchors.pop()
        if self.parser.unwind_ != 0:
            self.parser.unwind_ = self.parser.unwind_ - 1


class CounterScope:
    """Bindings belong to an invocation; callees see their caller's bindings."""
    parser: object
    saved: StrIntMap
    open: bool

    def __init__(self, parser):
        self.parser = parser
        self.saved = parser.counters_.copy()
        self.open = True

    def restore(self) -> None:
        if not self.open:
            return
        self.open = False
        self.parser.counters_.assign(self.saved)


class RuleProbe:
    """`optional(pos, rec)` and custom fail actions probe a rule: if a required
    descendant never started, the probe absorbs the NoMatch."""
    parser: object
    previous: object
    has_previous: bool
    start: int
    committed: bool
    open: bool

    def __init__(self, parser, start: int):
        self.parser = parser
        self.previous = parser.probe_
        self.has_previous = parser.has_probe_
        self.start = start
        self.committed = False
        self.open = True
        parser.probe_ = self
        parser.has_probe_ = True

    def close(self) -> None:
        if not self.open:
            return
        self.open = False
        self.parser.probe_ = self.previous
        self.parser.has_probe_ = self.has_previous


class Resume:
    skip: int
    frame: int
    claim: recovery.ContextClaim
    has_claim: bool

    def __init__(self, skip: int, frame: int, claim: recovery.ContextClaim, has_claim: bool):
        self.skip = skip
        self.frame = frame
        self.claim = claim
        self.has_claim = has_claim


class LLParser(Parser_base):
    recovery_state_: recovery.State
    counters_: StrIntMap
    frames_: list
    unwind_: int
    last_resume_pos_: int
    recovering_: bool
    context_max_skip_: int
    context_errors_: list
    probe_: object
    has_probe_: bool
    token_names_: TokenNames
    ambiguous_id_: int
    space_token_: int
    has_space_token_: bool

    def __init__(self):
        Parser_base.__init__(self)
        self.recovery_state_ = recovery.State()
        self.counters_ = StrIntMap()
        self.frames_ = []
        self.unwind_ = 0
        # progress guard
        self.last_resume_pos_ = NPOS
        # suppress cascading reports
        self.recovering_ = False
        self.context_max_skip_ = 64
        self.context_errors_ = []
        self.probe_ = 0
        self.has_probe_ = False
        self.token_names_ = NO_TOKEN_NAMES
        self.ambiguous_id_ = NPOS
        # The token id generated code passes as `SpaceToken`; -1 = none.
        self.space_token_ = NPOS
        self.has_space_token_ = False

    def set_token_names(self, names: TokenNames, ambiguous_id: int) -> None:
        self.token_names_ = names
        self.ambiguous_id_ = ambiguous_id

    def set_space_token(self, type_id: int) -> None:
        self.space_token_ = type_id
        self.has_space_token_ = True

    # Diagnostics from the current/last parse, including one that failed.
    # A new parse clears them; callers control how they are displayed.
    def getErrors(self) -> list:
        return self.recovery_state_.diagnostics

    # Diagnostics produced by hierarchical (context) recovery.
    def getContextErrors(self) -> list:
        return self.context_errors_

    def reset_recovery(self) -> None:
        self.counters_.clear()
        self.recovery_state_.diagnostics.clear()
        self.recovery_state_.total_skipped = 0
        self.recovery_state_.anchors.clear()
        self.frames_.clear()
        self.unwind_ = 0
        self.last_resume_pos_ = NPOS
        self.recovering_ = False
        self.context_errors_.clear()

    def parse_tokens(self):
        self.reset_recovery()
        return Parser_base.parse_tokens(self)

    def parseLazy(self):
        self.reset_recovery()
        if not self.has_text:
            from ispa_std.failures import api_error
            api_error("Can't parse because no input text is provided", [])
        result = self.lazyParse()
        at_end = result.it.isEnd()
        messages = result.it.getErrors()
        i = 0
        while i < len(messages):
            diagnostic = recovery.Diagnostic()
            diagnostic.rule = "lexer"
            diagnostic.message = messages[i]
            self.recovery_state_.diagnostics.append(diagnostic)
            i = i + 1
        if (not result.status) or (not at_end):
            diagnostic = recovery.Diagnostic()
            diagnostic.rule = "main"
            if result.status:
                diagnostic.message = "unexpected trailing input"
            else:
                diagnostic.message = "could not match main rule"
            if not at_end:
                token = result.it.current()
                diagnostic.offset = token.meta().startpos()
                diagnostic.length = token.meta().length()
            else:
                diagnostic.offset = len(self.text)
            self.recovery_state_.diagnostics.append(diagnostic)
        if result.status:
            self.tree = result.node.data()
        return self.tree

    # --- counters ------------------------------------------------------------
    def counter_scope(self) -> CounterScope:
        return CounterScope(self)

    def bind_counter_count(self, name: str, count: int) -> None:
        if not self.counters_.try_emplace(name, count):
            if self.counters_.get(name) != count:
                logic_error("Shared counter '" + name + "' has a different count")

    def bind_counter_list(self, name: str, values: list) -> None:
        self.bind_counter_count(name, len(values))

    def bind_counter_node(self, name: str, node: Node) -> None:
        count = 0
        if not node.empty():
            count = 1
        self.bind_counter_count(name, count)

    # --- probes --------------------------------------------------------------
    def begin_probe(self, pos) -> RuleProbe:
        return RuleProbe(self, cursor_position(pos))

    def reject_absent_rule(self) -> None:
        """Raise NoMatch when we are inside an uncommitted probe: the rule never
        started, so the caller's `optional` should simply not match."""
        if self.has_probe_ and not self.probe_.committed:
            no_match()

    def commit_rule(self, pos) -> None:
        probe = self.probe_
        active = self.has_probe_
        while active:
            if cursor_position(pos) != probe.start:
                probe.committed = True
            active = probe.has_previous
            probe = probe.previous

    def note_match(self) -> None:
        self.recovering_ = False

    # --- debug ---------------------------------------------------------------
    def debug_actual(self, pos) -> str:
        if not debug.enabled():
            return ""
        if pos.isEnd():
            return "<end of input>"
        actual = "token #" + int_to_string(recovery.token_id(pos.current()))
        meta = pos.current().meta()
        if meta.has_source() and meta.length() > 0:
            actual = actual + " '" + substr(meta.source(), meta.startpos(), minimum(meta.length(), 80)) + "'"
        return actual

    def debug_recovery_mode(self, mode: int) -> str:
        if mode == recovery.Mode.PanicUntil:
            return "panic until synchronization"
        if mode == recovery.Mode.Insert:
            return "insert"
        if mode == recovery.Mode.Delete:
            return "delete and retry"
        if mode == recovery.Mode.Replace:
            return "replace"
        if mode == recovery.Mode.Retry:
            return "retry"
        if mode == recovery.Mode.Synchronize:
            return "synchronize"
        if mode == recovery.Mode.Propagate:
            return "propagate to parent"
        if mode == recovery.Mode.Abort:
            return "abort"
        return "unknown"

    # --- frames --------------------------------------------------------------
    def recovery_frame(self, rule: str) -> FrameGuard:
        return FrameGuard(self, rule)

    def recovery_at(self, expecting: str, claims: list) -> None:
        """Emitted before each child member: the claims of this frame here (the
        member itself if repeated, plus the suffix up to the first required
        member). `claims` is a list of recovery.ContextClaim."""
        if len(self.frames_) == 0:
            return
        frame = self.frames_[len(self.frames_) - 1]
        frame.expecting = expecting
        frame.claims = claims
        anchors = self.recovery_state_.anchors[len(self.recovery_state_.anchors) - 1]
        anchors.clear()
        i = 0
        while i < len(claims):
            anchors.extend(claims[i].tokens.values())
            i = i + 1

    def unwinding(self) -> bool:
        return self.unwind_ != 0

    def describe_token(self, pos) -> str:
        if pos.isEnd():
            return "end of input"
        meta = pos.current().meta()
        if meta.has_source() and meta.startpos() != NPOS:
            return "'" + meta.text() + "'"
        return "<token>"

    def breadcrumb(self) -> str:
        out = ""
        i = 0
        while i < len(self.frames_):
            if len(out) > 0:
                out = out + " > "
            out = out + self.frames_[i].rule
            i = i + 1
        return out

    def resolve_context(self, pos, max_skip: int) -> Resume:
        """Earliest token (within max_skip) claimed by any frame; ties go to the
        innermost frame. If an identical spot already failed, the innermost
        frame may not claim the current token again (guarantees progress).
        Returns a Resume with frame == -1 when there is none."""
        repeat_spot = cursor_position(pos) == self.last_resume_pos_
        candidate = pos.copy()
        k = 0
        while k <= max_skip and not candidate.isEnd():
            token = candidate.current()
            f = len(self.frames_)
            while f > 0:
                f = f - 1
                if not (k == 0 and repeat_spot and f + 1 == len(self.frames_)):
                    claims = self.frames_[f].claims
                    c = 0
                    while c < len(claims):
                        if claims[c].tokens.matches(token):
                            return Resume(k, f, claims[c], True)
                        c = c + 1
            k = k + 1
            candidate.next()
        if candidate.isEnd() and len(self.frames_) > 0:
            return Resume(k, 0, recovery.ContextClaim("", recovery.empty_token_set()), False)
        return Resume(0, -1, recovery.ContextClaim("", recovery.empty_token_set()), False)

    def report_context(self, pos, kind: int, r: Resume) -> None:
        inner = self.frames_[len(self.frames_) - 1]
        target = self.frames_[r.frame]
        d = recovery.ContextDiagnostic()
        d.kind = kind
        d.position = cursor_position(pos)
        if pos.has_source():
            at = pos.sourcePosition()
            if at >= 0:
                d.offset = at
                d.length = 0
                if at < len(pos.sourceOrigin()):
                    d.length = 1
        elif not pos.isEnd():
            d.offset = pos.current().meta().startpos()
            d.length = pos.current().meta().length()
        if d.offset == recovery.NO_OFFSET and pos.isEnd() and self.has_text:
            d.offset = len(self.text)
        d.skipped = r.skip
        d.rule = inner.rule
        d.resume_rule = target.rule
        if r.has_claim:
            d.claim = r.claim.name
        tok = self.describe_token(pos)
        where = self.breadcrumb()
        expected = ""
        if len(inner.expecting) > 0:
            expected = "; expected " + inner.expecting
        if kind == recovery.ContextKind.Misplaced:
            d.message = ("unexpected " + tok + " while parsing '" + d.rule + "'" + expected
                         + "; continuing at '" + d.claim + "' in enclosing rule '" + d.resume_rule + "'")
        elif kind == recovery.ContextKind.Stray:
            tail = "; reached end of input"
            if r.has_claim:
                tail = "; resuming at '" + d.claim + "' of '" + d.resume_rule + "'"
            d.message = ("unexpected " + tok + " in " + where
                         + "; no enclosing rule accepts it here; skipped "
                         + int_to_string(r.skip) + " token(s)" + tail)
        else:
            d.message = "unexpected " + tok + " in " + where + expected
            if r.skip != 0:
                d.message = d.message + "; skipped " + int_to_string(r.skip) + " token(s)"
            if r.has_claim:
                d.message = d.message + "; resuming at '" + d.claim + "' of '" + d.resume_rule + "'"
            else:
                d.message = d.message + "; reached end of input"
        self.context_errors_.append(d)

    def recover_at_context(self, pos, kind: int) -> bool:
        """Local repair already failed (or does not apply). Find a recovery
        point in the enclosing frames, skip to it, and request unwinding."""
        if len(self.frames_) == 0 or self.recovery_state_.exhausted():
            return False
        remaining = self.recovery_state_.max_total_skipped - self.recovery_state_.total_skipped
        r = self.resolve_context(pos, minimum(self.context_max_skip_, remaining))
        if r.frame == -1:
            return False
        effective = kind
        innermost = r.frame + 1 == len(self.frames_)
        if (not innermost) and r.skip == 0 and r.has_claim:
            effective = recovery.ContextKind.Misplaced
        if (not self.recovering_) or effective == recovery.ContextKind.Misplaced:
            self.report_context(pos, effective, r)
        i = 0
        while i < r.skip and not pos.isEnd():
            pos.next()
            i = i + 1
        self.recovery_state_.total_skipped = self.recovery_state_.total_skipped + r.skip
        self.last_resume_pos_ = cursor_position(pos)
        self.unwind_ = len(self.frames_) - 1 - r.frame
        self.recovering_ = True
        return True

    def sync_loop(self, pos) -> None:
        """A loop ended on a token that no active frame can accept: report it
        here, where the mistake is, rather than at the next required member."""
        if pos.isEnd() or len(self.frames_) == 0 or self.unwinding():
            return
        candidate = pos.copy()
        if self.has_space_token_:
            self.skip_spaces(candidate)
        if candidate.isEnd():
            return
        # External FIRST sets are predicates over source text; a boundary token
        # cannot be classified by the host recovery token sets.
        if candidate.current().isAmbiguous():
            return
        token = candidate.current()
        any_claims = False
        i = 0
        while i < len(self.frames_):
            claims = self.frames_[i].claims
            c = 0
            while c < len(claims):
                any_claims = True
                if claims[c].tokens.matches(token):
                    # someone owns it
                    return
                c = c + 1
            i = i + 1
        if not any_claims:
            return
        pos.assign(candidate)
        self.recover_at_context(pos, recovery.ContextKind.Stray)

    def recover_choice(self, pos, rule: str, expected: str, first_sets: list, inner_sets: list) -> int:
        """Called only after normal LL(k) selection failed. Prefer an
        unambiguous FIRST token, then an unambiguous terminal inside an
        alternative (shared interior terminals do not identify a branch).
        `first_sets[i]` / `inner_sets[i]` are the two sets of alternative i
        (the C++ even/odd template pack). Returns the alternative, or -1."""
        self.reject_absent_rule()
        if len(first_sets) != len(inner_sets):
            logic_error("recover_choice: one FIRST set and one interior set per alternative")
        diagnostic = recovery.Diagnostic()
        diagnostic.rule = rule
        diagnostic.expected_text = expected
        diagnostic.message = ("unexpected " + self.describe_token(pos) + " in " + rule
                              + "; expected one of: " + expected)
        if not pos.isEnd():
            diagnostic.unexpected = recovery.token_id(pos.current())
            diagnostic.offset = pos.current().meta().startpos()
            diagnostic.length = pos.current().meta().length()
        i = 0
        while i < len(first_sets):
            diagnostic.expected.extend(first_sets[i].values())
            i = i + 1
        selected = -1
        skipped = 0
        if not self.recovery_state_.exhausted():
            scanning = True
            while scanning and not pos.isEnd():
                ident = recovery.token_id(pos.current())
                phase = 0
                while phase < 2 and selected == -1:
                    sets = first_sets
                    if phase == 1:
                        sets = inner_sets
                    candidate = -1
                    ambiguous = False
                    j = 0
                    while j < len(sets):
                        if recovery.contains(sets[j].values(), ident):
                            if candidate != -1:
                                ambiguous = True
                            candidate = j
                        j = j + 1
                    if ambiguous:
                        phase = 2
                    else:
                        selected = candidate
                        phase = phase + 1
                if selected != -1:
                    scanning = False
                else:
                    boundary = False
                    a = 0
                    while a < len(self.recovery_state_.anchors):
                        if recovery.contains(self.recovery_state_.anchors[a], ident):
                            boundary = True
                        a = a + 1
                    if boundary or skipped >= self.context_max_skip_ \
                            or self.recovery_state_.total_skipped + skipped >= self.recovery_state_.max_total_skipped:
                        scanning = False
                    else:
                        pos.next()
                        skipped = skipped + 1
        self.recovery_state_.total_skipped = self.recovery_state_.total_skipped + skipped
        diagnostic.decision.skipped = skipped
        if selected == -1:
            diagnostic.decision.mode = recovery.Mode.Propagate
            diagnostic.message = diagnostic.message + "; no unambiguous alternative at recovery boundary"
        else:
            diagnostic.decision.mode = recovery.Mode.Synchronize
            diagnostic.message = diagnostic.message + "; continuing in alternative " + int_to_string(selected + 1)
        if skipped != 0:
            diagnostic.message = diagnostic.message + "; skipped " + int_to_string(skipped) + " tokens"
        self.recovery_state_.diagnostics.append(diagnostic)
        if self.recovery_state_.exhausted():
            last = self.recovery_state_.diagnostics[len(self.recovery_state_.diagnostics) - 1]
            last.decision.mode = recovery.Mode.Abort
            last.message = last.message + "; recovery limit reached"
            return -1
        return selected

    def raise_error(self, token, message: str) -> None:
        self.reject_absent_rule()
        diagnostic = recovery.Diagnostic()
        if len(self.frames_) == 0:
            diagnostic.rule = "<rule>"
        else:
            diagnostic.rule = self.frames_[len(self.frames_) - 1].rule
        diagnostic.message = message
        diagnostic.offset = token.meta().startpos()
        diagnostic.length = token.meta().length()
        self.recovery_state_.diagnostics.append(diagnostic)

    # --- required ------------------------------------------------------------
    def required_recovering(self, pos, type_id: int, fail: recovery.FailHandler) -> recovery.RequiredResult:
        if self.unwinding():
            return recovery.RequiredResult.boundary_only()
        retries = 0
        while True:
            if pos.current().holds(type_id):
                node = pos.current().to_node(type_id)
                pos.next()
                self.commit_rule(pos)
                self.note_match()
                return recovery.RequiredResult(recovery.RequiredStatus.Matched, True, node,
                                               recovery.Decision.default())
            self.reject_absent_rule()
            decision = fail.run(pos, self.recovery_state_)
            retry = False
            if decision.mode == recovery.Mode.Delete:
                retries = retries + 1
                retry = retries <= 2
            elif decision.mode == recovery.Mode.Retry:
                retries = retries + 1
                retry = retries <= 1
            elif decision.mode == recovery.Mode.Insert or decision.mode == recovery.Mode.Replace:
                return recovery.RequiredResult(recovery.RequiredStatus.Missing, False, Node.blank(), decision)
            elif decision.mode == recovery.Mode.Abort:
                return recovery.RequiredResult(recovery.RequiredStatus.Aborted, False, Node.blank(), decision)
            if not retry:
                # Local repair did not work: look for a recovery point in the
                # enclosing frames (may skip tokens and request unwinding).
                if decision.mode == recovery.Mode.Propagate:
                    self.recover_at_context(pos, recovery.ContextKind.Mismatch)
                return recovery.RequiredResult(recovery.RequiredStatus.Boundary, False, Node.blank(), decision)

    def _attempt_rule(self, rec, attempt) -> MatchResult:
        """Run `rec(attempt)` inside a probe, absorbing NoMatch and
        ExternalParse so an explicitly bound parent handler can run (C++
        `catch (const NoMatch&)` / `catch (const ExternalParseError&)`)."""
        holder = []
        probe = self.begin_probe(attempt)
        scope = debug.scope_depth()
        failed = prim.attempt(lambda: holder.append(rec(attempt)))
        probe.close()
        if failed == 0:
            return holder[0]
        debug.unwind_to(scope)
        if failed == ErrorKind.NoMatch or failed == ErrorKind.ExternalParse:
            return MatchResult.failed(attempt)
        rethrow(failed, prim.failure_message())
        return MatchResult.failed(attempt)

    def required_rule_recovering(self, pos, rec, fail: recovery.FailHandler) -> recovery.RequiredResult:
        if self.unwinding():
            return recovery.RequiredResult.boundary_only()
        retries = 0
        while True:
            # Recursive alternatives may advance their input argument before
            # failing. Never let a failed probe mutate pos.
            attempt = pos.copy()
            if fail.is_custom():
                match = self._attempt_rule(rec, attempt)
            else:
                match = rec(attempt)
            if match.status:
                pos.assign(match.it)
                self.commit_rule(pos)
                self.note_match()
                return recovery.RequiredResult(recovery.RequiredStatus.Matched, True, match.node,
                                               recovery.Decision.default())
            if self.unwinding():
                pos.assign(match.it)
                # The child abandoned itself for an outer frame; do not repair
                # here, just propagate.
                return recovery.RequiredResult(recovery.RequiredStatus.Boundary, False, Node.blank(),
                                               recovery.Decision.default())
            self.reject_absent_rule()
            decision = fail.run(pos, self.recovery_state_)
            # We cannot synthesize an arbitrary nonterminal from one token.
            again = False
            if decision.mode == recovery.Mode.Retry or decision.mode == recovery.Mode.Delete:
                retries = retries + 1
                again = retries <= 2
            if not again:
                if decision.mode == recovery.Mode.Propagate:
                    self.recover_at_context(pos, recovery.ContextKind.Mismatch)
                status = recovery.RequiredStatus.Boundary
                if decision.mode == recovery.Mode.Abort:
                    status = recovery.RequiredStatus.Aborted
                elif decision.mode == recovery.Mode.Insert or decision.mode == recovery.Mode.Replace:
                    status = recovery.RequiredStatus.Missing
                return recovery.RequiredResult(status, False, Node.blank(), decision)

    def _required_outcome(self, result: recovery.RequiredResult, pos, scope: debug.MatchScope,
                          what: str) -> Node:
        """Shared tail of required_token / required_rule: a repaired or unwound
        match yields an empty Node; anything else is unrecoverable."""
        if result.has_node:
            scope.success()
            scope.close()
            return result.node
        if result.status != recovery.RequiredStatus.Matched:
            scope.outcome("recovery: " + self.debug_recovery_mode(result.recovery.mode)
                          + ", skipped " + int_to_string(result.recovery.skipped))
        scope.close()
        if result.status != recovery.RequiredStatus.Aborted:
            if result.status == recovery.RequiredStatus.Missing or self.unwinding() \
                    or ((not pos.isEnd()) and result.recovery.mode != recovery.Mode.Propagate):
                return Node.blank()
        debug.breakpoint(what + " failed")
        if len(self.recovery_state_.diagnostics) == 0:
            logic_error("ISPA required(): " + what + " could not be recovered")
        else:
            logic_error(self.recovery_state_.diagnostics[len(self.recovery_state_.diagnostics) - 1].message)
        return Node.blank()

    def required_token(self, pos, type_id: int, fail: recovery.FailHandler) -> Node:
        scope = debug.MatchScope(self.token_names_.lookup(type_id), self.debug_actual(pos))
        if fail.is_recovering():
            result = self.required_recovering(pos, type_id, fail)
            return self._required_outcome(result, pos, scope, "required token")
        if pos.current().holds(type_id):
            node = pos.current().to_node(type_id)
            pos.next()
            self.commit_rule(pos)
            scope.success()
            scope.close()
            return node
        self.reject_absent_rule()
        fail.run(pos, self.recovery_state_)
        scope.close()
        debug.breakpoint("required token failed")
        if len(self.recovery_state_.diagnostics) == 0:
            logic_error("ISPA required(): required token could not be recovered")
        else:
            logic_error(self.recovery_state_.diagnostics[len(self.recovery_state_.diagnostics) - 1].message)
        return Node.blank()

    def required_rule(self, pos, rule_name: str, rec, fail: recovery.FailHandler) -> Node:
        scope = debug.MatchScope(rule_name, self.debug_actual(pos))
        if fail.is_recovering():
            result = self.required_rule_recovering(pos, rec, fail)
            return self._required_outcome(result, pos, scope, "required rule")
        match = rec(pos)
        if match.status:
            pos.assign(match.it)
            self.commit_rule(pos)
            scope.success()
            scope.close()
            return match.node
        self.reject_absent_rule()
        fail.run(pos, self.recovery_state_)
        scope.close()
        debug.breakpoint("required rule failed")
        if len(self.recovery_state_.diagnostics) == 0:
            logic_error("ISPA required(): required rule could not be recovered")
        else:
            logic_error(self.recovery_state_.diagnostics[len(self.recovery_state_.diagnostics) - 1].message)
        return Node.blank()

    # --- optional ------------------------------------------------------------
    def optional_token(self, pos, type_id: int) -> Node:
        scope = debug.MatchScope(self.token_names_.lookup(type_id), self.debug_actual(pos))
        if pos.current().holds(type_id):
            node = pos.current().to_node(type_id)
            pos.next()
            self.commit_rule(pos)
            scope.success()
            scope.close()
            return node
        scope.close()
        return Node.blank()

    def optional_rule(self, pos, rec) -> Node:
        """NOTE: a rule that abandons itself during unwinding must still return
        status == True (a partial match) or the committed-probe check throws."""
        attempt = pos.copy()
        holder = []
        probe = self.begin_probe(pos)
        scope = debug.scope_depth()
        failed = prim.attempt(lambda: holder.append(rec(attempt)))
        if failed == 0:
            match = holder[0]
            if match.status:
                pos.assign(match.it)
                self.commit_rule(pos)
                probe.close()
                return match.node
            committed = probe.committed
            probe.close()
            if committed:
                logic_error("ISPA optional(): rule failed after consuming input")
            return Node.blank()
        probe.close()
        debug.unwind_to(scope)
        if failed != ErrorKind.NoMatch:
            rethrow(failed, prim.failure_message())
        # A required descendant did not start. Only the optional caller handles
        # this signal; recovery has not run.
        return Node.blank()

    # --- repetition ----------------------------------------------------------
    def skip_spaces(self, pos) -> None:
        if not self.has_space_token_:
            return
        if pos.current().holds(self.space_token_):
            pos.next()

    def skip_external_spaces(self, pos) -> None:
        from ispa_std.chars import at, is_space
        if pos.has_source():
            origin = pos.sourceOrigin()
            begin = pos.sourcePosition()
            end = begin
            while is_space(at(origin, end)):
                end = end + 1
            if end != begin:
                pos.consumeExternal(end)
        else:
            self.skip_spaces(pos)

    def counted_impl(self, pos, spec: CountSpec, match) -> list:
        has_limit = spec.has_count
        limit = spec.count
        if len(spec.name) > 0:
            if self.counters_.contains(spec.name):
                limit = self.counters_.get(spec.name)
                has_limit = True
        nodes = []
        cursor = pos.copy()
        matching = True
        while matching and ((not has_limit) or len(nodes) < limit):
            attempt = cursor.copy()
            if self.has_space_token_ and len(nodes) > 0:
                self.skip_spaces(attempt)
            before = cursor_position(attempt)
            node = match(attempt)
            if node.empty():
                matching = False
            else:
                if (not has_limit) and cursor_position(attempt) == before:
                    logic_error("Unbound counted repetition matched without consuming input")
                nodes.append(node)
                cursor.assign(attempt)
        if (has_limit and len(nodes) != limit) or len(nodes) < spec.minimum:
            self.reject_absent_rule()
            diagnostic = recovery.Diagnostic()
            if len(self.frames_) == 0:
                diagnostic.rule = "<rule>"
            else:
                diagnostic.rule = self.frames_[len(self.frames_) - 1].rule
            wanted = spec.minimum
            if has_limit:
                wanted = maximum(limit, spec.minimum)
            label = ""
            if len(spec.name) > 0:
                label = " '" + spec.name + "'"
            diagnostic.message = ("Counted repetition" + label + " expected " + int_to_string(wanted)
                                  + " matches, got " + int_to_string(len(nodes)))
            if not cursor.isEnd():
                diagnostic.unexpected = recovery.token_id(cursor.current())
                diagnostic.offset = cursor.current().meta().startpos()
            message = diagnostic.message
            self.recovery_state_.diagnostics.append(diagnostic)
            logic_error(message)
        if len(spec.name) > 0:
            self.counters_.try_emplace(spec.name, len(nodes))
        pos.assign(cursor)
        self.commit_rule(pos)
        return nodes

    def counted_token(self, pos, type_id: int, spec: CountSpec) -> list:
        return self.counted_impl(pos, spec, lambda attempt: self.optional_token(attempt, type_id))

    def counted_rule(self, pos, rec, spec: CountSpec) -> list:
        return self.counted_impl(pos, spec, lambda attempt: self.optional_rule(attempt, rec))

    def repeat_optional(self, pos, match, separated: bool) -> Node:
        """Separators belong to a successful next iteration, not to the end of
        the list: leave pos (including whitespace) intact on absence."""
        attempt = pos.copy()
        if self.has_space_token_ and separated:
            self.skip_spaces(attempt)
        start = cursor_position(attempt)
        node = match(attempt)
        if node.empty() or cursor_position(attempt) == start:
            return Node.blank()
        pos.assign(attempt)
        return node

    def range_token(self, pos, type_id: int, fail: recovery.FailHandler) -> list:
        result = []
        if fail.is_recovering():
            first = self.required_recovering(pos, type_id, fail)
            if not first.has_node:
                # repaired/unwound: nothing consumed
                return result
            result.append(first.node)
        else:
            result.append(self.required_token(pos, type_id, fail))
        if self.unwinding():
            return result
        repeating = True
        while repeating and (not pos.isEnd()) and not self.unwinding():
            node = self.repeat_optional(pos, lambda attempt: self.optional_token(attempt, type_id), True)
            if node.empty():
                repeating = False
            else:
                result.append(node)
        self.sync_loop(pos)
        return result

    def range_rule(self, pos, rule_name: str, rec, fail: recovery.FailHandler) -> list:
        start = cursor_position(pos)
        result = []
        if fail.is_recovering():
            first = self.required_rule_recovering(pos, rec, fail)
            if not first.has_node:
                return result
            result.append(first.node)
        else:
            result.append(self.required_rule(pos, rule_name, rec, fail))
        if cursor_position(pos) == start or self.unwinding():
            return result
        repeating = True
        while repeating and (not pos.isEnd()) and not self.unwinding():
            node = self.repeat_optional(pos, lambda attempt: self.optional_rule(attempt, rec), True)
            if node.empty():
                repeating = False
            else:
                result.append(node)
        self.sync_loop(pos)
        return result

    def range_optional_token(self, pos, type_id: int) -> list:
        result = []
        repeating = True
        while repeating and (not pos.isEnd()) and not self.unwinding():
            node = self.repeat_optional(pos, lambda attempt: self.optional_token(attempt, type_id),
                                        len(result) > 0)
            if node.empty():
                repeating = False
            else:
                result.append(node)
        self.sync_loop(pos)
        return result

    def range_optional_rule(self, pos, rec) -> list:
        result = []
        repeating = True
        while repeating and (not pos.isEnd()) and not self.unwinding():
            node = self.repeat_optional(pos, lambda attempt: self.optional_rule(attempt, rec),
                                        len(result) > 0)
            if node.empty():
                repeating = False
            else:
                result.append(node)
        self.sync_loop(pos)
        return result

    # --- contexts ------------------------------------------------------------
    def rule_context(self, pos, ast, start, rule: str) -> RuleContext:
        return RuleContext(pos, ast, start, rule, self.token_names_, self.ambiguous_id_)

    def fail_context(self, pos, ast, start, rule: str, failed_rule: str) -> FailContext:
        return FailContext(pos, ast, start, rule, failed_rule, self.recovery_state_, self.text,
                           self.has_text, len(self.recovery_state_.diagnostics), self.token_names_,
                           self.ambiguous_id_)

    def custom_failure_decision(self, before, after) -> recovery.Decision:
        decision = recovery.Decision.default()
        if cursor_position(before) == cursor_position(after):
            decision.mode = recovery.Mode.Insert
        else:
            decision.mode = recovery.Mode.Retry
        return decision

    def custom_failure_decision_ctx(self, before, ctx: FailContext) -> recovery.Decision:
        return ctx.finish(before)

    def custom_failure(self, action) -> recovery.ActionFailure:
        return recovery.ActionFailure(action)

    def action_result(self, ast, pos) -> MatchResult:
        node = Node.blank()
        node.set_data(ast)
        return MatchResult(True, node, pos.copy())
