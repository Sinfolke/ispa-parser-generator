"""ispa_recovery.hpp + ispa_recovery_context.hpp: LL error recovery.

Only invoked after a committed required match fails. Never invoke it from
optional/probing/backtracking alternatives. Token ids are Ints; EOF_TOKEN
(-1) is "end of input" (C++: size_t max). Tokens expose kind(); cursors expose
isEnd(), current(), next(), copy()."""
from ispa_std.failures import invalid_argument, logic_error
from ispa_std.format import int_to_string, join
from typing import Callable

from ispa_std.lexer import TokenCursor
from ispa_std.node import Node
from ispa_std.raw_token import RawToken

EOF_TOKEN = -1
END_OF_INPUT: int = EOF_TOKEN
NO_OFFSET = -1


class Mode:
    PanicUntil = 0
    Insert = 1
    Delete = 2
    Replace = 3
    Retry = 4
    Synchronize = 5
    Propagate = 6
    Abort = 7


class Decision:
    mode: int
    synthesized: int
    unexpected: int
    skipped: int

    def __init__(self, mode: int, synthesized: int, unexpected: int, skipped: int):
        self.mode = mode
        self.synthesized = synthesized
        self.unexpected = unexpected
        self.skipped = skipped

    @staticmethod
    def default() -> "Decision":
        return Decision(Mode.Propagate, EOF_TOKEN, EOF_TOKEN, 0)

    def copy(self) -> "Decision":
        return Decision(self.mode, self.synthesized, self.unexpected, self.skipped)


class Diagnostic:
    rule: str
    unexpected: int
    expected: list
    decision: Decision
    expected_text: str
    offset: int
    length: int
    message: str
    code: str
    notes: list

    def __init__(self):
        self.rule = ""
        self.unexpected = EOF_TOKEN
        self.expected = []
        self.decision = Decision.default()
        self.expected_text = ""
        self.offset = NO_OFFSET
        self.length = 0
        self.message = ""
        self.code = ""
        self.notes = []


class State:
    max_errors: int
    max_total_skipped: int
    total_skipped: int
    diagnostics: list
    # Active call-site suffixes, including terminals inside nested productions.
    anchors: list

    def __init__(self):
        self.max_errors = 32
        self.max_total_skipped = 2048
        self.total_skipped = 0
        self.diagnostics = []
        self.anchors = []

    def exhausted(self) -> bool:
        return len(self.diagnostics) >= self.max_errors or self.total_skipped >= self.max_total_skipped


class Plan:
    rule: str
    expected: list
    continuation: list
    follow: list
    explicit_sync: list
    allow_insertion: bool
    allow_deletion: bool
    allow_replacement: bool
    max_skip: int

    def __init__(self, rule: str, expected: list, continuation: list, follow: list, explicit_sync: list,
                 allow_insertion: bool, allow_deletion: bool, allow_replacement: bool, max_skip: int):
        self.rule = rule
        self.expected = expected
        self.continuation = continuation
        self.follow = follow
        self.explicit_sync = explicit_sync
        self.allow_insertion = allow_insertion
        self.allow_deletion = allow_deletion
        self.allow_replacement = allow_replacement
        self.max_skip = max_skip


def contains(tokens: list, id: int) -> bool:
    i = 0
    while i < len(tokens):
        if tokens[i] == id:
            return True
        i = i + 1
    return False


def decode(encoded: str) -> list:
    """Comma/space separated unsigned decimal token ids (LangAPI fail wrappers)."""
    result = []
    i = 0
    n = len(encoded)
    while i < n:
        while i < n and (encoded[i] == "," or encoded[i] == " "):
            i = i + 1
        if i < n:
            value = 0
            has_digit = False
            while i < n and encoded[i] >= "0" and encoded[i] <= "9":
                has_digit = True
                value = value * 10 + _digit(encoded[i])
                i = i + 1
            if (not has_digit) or (i < n and encoded[i] != "," and encoded[i] != " "):
                invalid_argument("ISPA recovery: malformed token set")
                return result
            result.append(value)
    return result


def _digit(c: str) -> int:
    d = 0
    while d < 10 and "0123456789"[d] != c:
        d = d + 1
    return d


def token_id(token: RawToken) -> int:
    return token.kind()


def _kind(pos: TokenCursor) -> int:
    if pos.isEnd():
        return EOF_TOKEN
    return token_id(pos.current())


def recover(pos: TokenCursor, state: State, plan: Plan, token_kind: int) -> Decision:
    """Pick and perform a repair at `pos`. `token_kind(token) -> id`; pos is
    advanced in place when tokens are skipped."""
    at = EOF_TOKEN
    if not pos.isEnd():
        at = token_kind(pos.current())
    offset = NO_OFFSET
    length = 0
    actual = "end of input"
    if at != EOF_TOKEN:
        meta = pos.current().meta()
        offset = meta.startpos()
        length = meta.length()
        if meta.has_source():
            actual = "'" + meta.text() + "'"
        else:
            actual = "token " + int_to_string(at)
    decision = Decision(Mode.Propagate, EOF_TOKEN, at, 0)

    if state.exhausted():
        decision.mode = Mode.Abort
    elif contains(plan.expected, at):
        # An explicit custom repair may have changed the input before this call.
        decision.mode = Mode.Retry
    else:
        nxt = EOF_TOKEN
        if at != EOF_TOKEN:
            lookahead = pos.copy()
            lookahead.next()
            if not lookahead.isEnd():
                nxt = token_kind(lookahead.current())
        expected_one = len(plan.expected) == 1
        # Do not delete enclosing delimiters / caller-owned follow tokens.
        protected_token = contains(plan.follow, at) or _anchor(plan, state, at)
        if plan.allow_deletion and (not protected_token) and at != EOF_TOKEN and contains(plan.expected, nxt):
            pos.next()
            decision.mode = Mode.Delete
            decision.skipped = 1
        elif plan.allow_insertion and expected_one and (contains(plan.continuation, at) or contains(plan.follow, at)):
            decision.mode = Mode.Insert
            decision.synthesized = plan.expected[0]
        elif plan.allow_replacement and expected_one and (not protected_token) and at != EOF_TOKEN \
                and contains(plan.continuation, nxt):
            pos.next()
            decision.mode = Mode.Replace
            decision.synthesized = plan.expected[0]
            decision.skipped = 1
        elif contains(plan.follow, at) or at == EOF_TOKEN:
            # Let the enclosing rule consume this boundary token.
            decision.mode = Mode.Propagate
        elif contains(plan.continuation, at) or _anchor(plan, state, at):
            # No input consumed; the enclosing rule handles the incomplete construct.
            decision.mode = Mode.Synchronize
        else:
            # Panic: consume malformed tokens but never consume the boundary.
            scanning = True
            while scanning and not pos.isEnd() and decision.skipped < plan.max_skip \
                    and state.total_skipped + decision.skipped < state.max_total_skipped:
                ident = token_kind(pos.current())
                if contains(plan.expected, ident) or contains(plan.continuation, ident) \
                        or contains(plan.follow, ident) or _anchor(plan, state, ident):
                    scanning = False
                else:
                    pos.next()
                    decision.skipped = decision.skipped + 1
            here = EOF_TOKEN
            if not pos.isEnd():
                here = token_kind(pos.current())
            if contains(plan.expected, here):
                decision.mode = Mode.Retry
            elif here == EOF_TOKEN or decision.skipped == 0:
                decision.mode = Mode.Propagate
            elif contains(plan.continuation, here) or contains(plan.follow, here) or _anchor(plan, state, here):
                # PanicUntil means a boundary was located, NOT a successful match.
                decision.mode = Mode.PanicUntil
            else:
                decision.mode = Mode.Propagate

    state.total_skipped = state.total_skipped + decision.skipped
    entry = Diagnostic()
    entry.rule = plan.rule
    entry.unexpected = at
    entry.expected = plan.expected
    entry.decision = decision.copy()
    entry.offset = offset
    entry.length = length
    entry.message = "unexpected " + actual + " in " + plan.rule
    state.diagnostics.append(entry)
    if len(state.diagnostics) >= state.max_errors or state.total_skipped >= state.max_total_skipped:
        decision.mode = Mode.Abort
    entry.decision = decision.copy()
    return decision


def _anchor(plan: Plan, state: State, id: int) -> bool:
    if contains(plan.explicit_sync, id):
        return True
    i = 0
    while i < len(state.anchors):
        if contains(state.anchors[i], id):
            return True
        i = i + 1
    return False


def recover_default(pos: TokenCursor, state: State, plan: Plan) -> Decision:
    return recover(pos, state, plan, token_id)


def recover_encoded(pos: TokenCursor, state: State, rule: str, expected: str, continuation: str, follow: str,
                    explicit_sync: str, allow_insertion: bool, allow_deletion: bool,
                    allow_replacement: bool, max_skip: int) -> Decision:
    return recover_default(pos, state, Plan(rule, decode(expected), decode(continuation), decode(follow),
                                           decode(explicit_sync), allow_insertion, allow_deletion,
                                           allow_replacement, max_skip))


class TokenSet:
    """Recovery::Tokens<...>: a fixed set of token ids."""
    _values: list

    def __init__(self, values: list):
        self._values = values

    def values(self) -> list:
        return self._values

    def matches(self, token: RawToken) -> bool:
        return contains(self._values, token_id(token))


def empty_token_set() -> TokenSet:
    return TokenSet([])


class Policy:
    allow_insertion: bool
    allow_deletion: bool
    allow_replacement: bool
    max_skip: int

    def __init__(self, allow_insertion: bool, allow_deletion: bool, allow_replacement: bool, max_skip: int):
        self.allow_insertion = allow_insertion
        self.allow_deletion = allow_deletion
        self.allow_replacement = allow_replacement
        if max_skip <= 0:
            invalid_argument("Recovery::Policy MaxSkip must be positive")
        self.max_skip = max_skip

    @staticmethod
    def default() -> "Policy":
        return Policy(True, True, True, 64)


class FailHandler:
    """What a `fail` argument of the required/range combinators is.
    run(pos, state) returns a Decision. is_custom() marks a user action that
    may itself raise NoMatch (C++ `custom_action`); is_recovering() is False
    for the legacy `fail(pos)` form that never repairs."""

    def run(self, pos: TokenCursor, state: State) -> Decision:
        logic_error("FailHandler.run is abstract")
        return Decision.default()

    def is_custom(self) -> bool:
        return False

    def is_recovering(self) -> bool:
        return True


class Handler(FailHandler):
    rule: str
    expected_text: str
    expected: TokenSet
    continuation: TokenSet
    follow: TokenSet
    synchronization: TokenSet
    policy: Policy

    def __init__(self, rule: str, expected_text: str, expected: TokenSet, continuation: TokenSet,
                 follow: TokenSet, synchronization: TokenSet, policy: Policy):
        self.rule = rule
        self.expected_text = expected_text
        self.expected = expected
        self.continuation = continuation
        self.follow = follow
        self.synchronization = synchronization
        self.policy = policy

    def run(self, pos: TokenCursor, state: State) -> Decision:
        plan = Plan(self.rule, self.expected.values(), self.continuation.values(), self.follow.values(),
                    self.synchronization.values(), self.policy.allow_insertion, self.policy.allow_deletion,
                    self.policy.allow_replacement, self.policy.max_skip)
        decision = recover_default(pos, state, plan)
        if len(state.diagnostics) > 0:
            diagnostic = state.diagnostics[len(state.diagnostics) - 1]
            diagnostic.expected_text = self.expected_text
            diagnostic.message = diagnostic.message + "; expected " + self.expected_text
            if decision.mode == Mode.Insert:
                diagnostic.message = diagnostic.message + "; inserted missing token"
            elif decision.mode == Mode.Replace:
                diagnostic.message = diagnostic.message + "; replaced unexpected token"
            elif decision.mode == Mode.Delete:
                diagnostic.message = diagnostic.message + "; deleted unexpected token"
            elif decision.mode == Mode.Abort:
                diagnostic.message = diagnostic.message + "; recovery limit reached"
            if decision.skipped > 1:
                diagnostic.message = diagnostic.message + "; skipped " + int_to_string(decision.skipped) + " tokens"
        return decision


class ActionFailure(FailHandler):
    """A user-written fail action: action(pos, state) -> Decision."""
    action: Callable[[TokenCursor, State], Decision]

    def __init__(self, action: Callable[[TokenCursor, State], Decision]):
        self.action = action

    def run(self, pos: TokenCursor, state: State) -> Decision:
        return self.action(pos, state)

    def is_custom(self) -> bool:
        return True


class LegacyFail(FailHandler):
    """The pre-recovery `fail(pos)` form: reports and never repairs."""
    action: Callable[[TokenCursor], None]

    def __init__(self, action: Callable[[TokenCursor], None]):
        self.action = action

    def run(self, pos: TokenCursor, state: State) -> Decision:
        self.action(pos)
        return Decision.default()

    def is_recovering(self) -> bool:
        return False


class RequiredStatus:
    Matched = 0
    Missing = 1
    Boundary = 2
    Aborted = 3


class RequiredResult:
    """A required match outcome. `node` is only meaningful when has_node."""
    status: int
    has_node: bool
    node: Node
    recovery: Decision

    def __init__(self, status: int, has_node: bool, node: Node, recovery: Decision):
        self.status = status
        self.has_node = has_node
        self.node = node
        self.recovery = recovery

    @staticmethod
    def boundary_only() -> "RequiredResult":
        """The C++ default-constructed result (status Boundary, no node)."""
        return RequiredResult(RequiredStatus.Boundary, False, Node.blank(), Decision.default())


# --- context recovery ---------------------------------------------------------
class ContextKind:
    # required member missing; skipped to a resume point
    Mismatch = 0
    # token starts a construct owned by an outer frame
    Misplaced = 1
    # token after a loop that no active frame can accept
    Stray = 2


class ContextClaim:
    """A production (or terminal) that an active frame could start or continue
    with at the current position."""
    name: str
    tokens: TokenSet

    def __init__(self, name: str, tokens: TokenSet):
        self.name = name
        self.tokens = tokens


class ContextDiagnostic:
    kind: int
    message: str
    position: int
    offset: int
    length: int
    skipped: int
    rule: str
    resume_rule: str
    claim: str

    def __init__(self):
        self.kind = ContextKind.Mismatch
        self.message = ""
        self.position = 0
        self.offset = NO_OFFSET
        self.length = 0
        self.skipped = 0
        self.rule = ""
        self.resume_rule = ""
        self.claim = ""


def describe_ids(ids: list) -> str:
    parts = []
    i = 0
    while i < len(ids):
        parts.append(int_to_string(ids[i]))
        i = i + 1
    return join(parts, ", ")
