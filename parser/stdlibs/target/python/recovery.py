"""ISPA error recovery runtime for the Python target.

Port of ispastdlib/ispa_recovery.hpp, ispa_recovery_context.hpp and recovery_scan.hpp.

Token ids are whatever `token.kind` holds (ints for generated lexers). `EndOfInput` is the same
sentinel the C++ runtime uses (`std::numeric_limits<size_t>::max()`), so LR windows and LL
recovery agree on what "no more tokens" looks like.
"""
from __future__ import annotations
import enum
from dataclasses import dataclass, field
from typing import Any, Callable, Iterable

ABSENT = (1 << 64) - 1          # end of input / "no offset"
EndOfInput = ABSENT
eof_token = ABSENT


class Mode(enum.Enum):
    PanicUntil = 0    # skipped input up to a synchronization token; caller handles construct
    Insert = 1        # synthesize the expected terminal; do NOT consume the current token
    Delete = 2        # consumed one unexpected terminal, then retry the required match
    Replace = 3       # consumed the wrong terminal, synthesize the expected one
    Retry = 4         # explicit custom-handler request, only after changing state
    Synchronize = 5   # already at a viable continuation boundary; keep its token
    Propagate = 6     # current construct cannot recover: ask the parent rule to recover
    Abort = 7         # error budget exceeded or explicitly requested


@dataclass(slots=True)
class Decision:
    mode: Mode = Mode.Propagate
    synthesized: Any = eof_token
    unexpected: Any = eof_token
    skipped: int = 0


@dataclass(slots=True)
class Diagnostic:
    rule: str = ''
    unexpected: Any = eof_token
    expected: list = field(default_factory=list)
    decision: Decision = field(default_factory=Decision)
    expected_text: str = ''
    offset: int = ABSENT
    length: int = 0
    message: str = ''
    code: str = ''
    notes: list = field(default_factory=list)


@dataclass(slots=True)
class State:
    max_errors: int = 32
    max_total_skipped: int = 2048
    total_skipped: int = 0
    diagnostics: list = field(default_factory=list)
    # Active call-site suffixes, including terminals inside nested productions.
    anchors: list = field(default_factory=list)

    def exhausted(self) -> bool:
        return len(self.diagnostics) >= self.max_errors or self.total_skipped >= self.max_total_skipped


@dataclass(slots=True)
class Plan:
    rule: str = ''
    expected: list = field(default_factory=list)
    continuation: list = field(default_factory=list)   # FIRST of the viable continuation at this site
    follow: list = field(default_factory=list)         # rule-call contextual FOLLOW if available
    explicit_sync: list = field(default_factory=list)
    allow_insertion: bool = True
    allow_deletion: bool = True
    allow_replacement: bool = True
    max_skip: int = 64


def contains(tokens: Iterable, token_id: Any) -> bool:
    return token_id in tokens


def decode(encoded: str) -> list[int]:
    """Comma/space separated unsigned decimal token ids (error-path-only encoding)."""
    result: list[int] = []
    for part in encoded.replace(',', ' ').split():
        if not part.isdigit():
            raise ValueError('ISPA recovery: malformed token set')
        value = int(part)
        if value >= eof_token:
            raise ValueError('ISPA recovery: token ID out of range')
        result.append(value)
    return result


def token_id(token: Any) -> Any:
    """Default token access: the token's `kind`."""
    kind = getattr(token, 'kind', None)
    if kind is None and hasattr(token, 'name'):
        kind = token.name() if callable(token.name) else token.name
    if isinstance(kind, int) and not isinstance(kind, bool):
        return int(kind)
    return kind


def _describe(token: Any) -> str:
    length = int(getattr(token, 'length', 0) or 0)
    text = token.text() if length and hasattr(token, 'text') else ''
    return f"'{text}'" if text else ''


def recover(pos, state: State, plan: Plan, token_kind: Callable[[Any], Any] = token_id) -> Decision:
    """Local repair of a committed required match that failed at `pos` (a Cursor)."""
    def kind(cursor) -> Any:
        return eof_token if cursor.isEnd() else token_kind(cursor.peek())

    def anchor(ident: Any) -> bool:
        if ident in plan.explicit_sync:
            return True
        return any(ident in tokens for tokens in state.anchors)

    at = kind(pos)
    offset, length, actual = ABSENT, 0, 'end of input'
    if at != eof_token:
        token = pos.peek()
        offset = int(getattr(token, 'startpos', ABSENT))
        length = int(getattr(token, 'length', 0))
        actual = _describe(token) or f'token {at}'
    decision = Decision(Mode.Propagate, unexpected=at)

    if state.exhausted():
        decision.mode = Mode.Abort
    elif at in plan.expected:
        # An explicit custom repair may have changed the input before this call.
        decision.mode = Mode.Retry
    else:
        nxt = eof_token
        if at != eof_token:
            look = pos.copy()
            look.advance()
            nxt = kind(look)
        expected_one = len(plan.expected) == 1
        # Do not delete enclosing delimiters / caller-owned follow tokens.
        protected = at in plan.follow or anchor(at)

        if plan.allow_deletion and not protected and at != eof_token and nxt in plan.expected:
            pos.advance()
            decision.mode, decision.skipped = Mode.Delete, 1
        elif plan.allow_insertion and expected_one and (at in plan.continuation or at in plan.follow):
            decision.mode = Mode.Insert
            decision.synthesized = plan.expected[0]
        elif (plan.allow_replacement and expected_one and not protected and at != eof_token
              and nxt in plan.continuation):
            pos.advance()
            decision.mode, decision.synthesized, decision.skipped = Mode.Replace, plan.expected[0], 1
        elif at in plan.follow or at == eof_token:
            decision.mode = Mode.Propagate       # let the enclosing rule consume this boundary
        elif at in plan.continuation or anchor(at):
            decision.mode = Mode.Synchronize     # no input consumed; the parent handles it
        else:
            # Panic: consume malformed tokens but never consume the boundary.
            while (kind(pos) != eof_token and decision.skipped < plan.max_skip
                   and state.total_skipped + decision.skipped < state.max_total_skipped):
                ident = kind(pos)
                if (ident in plan.expected or ident in plan.continuation
                        or ident in plan.follow or anchor(ident)):
                    break
                pos.advance()
                decision.skipped += 1
            here = kind(pos)
            if here in plan.expected:
                decision.mode = Mode.Retry
            elif here == eof_token or decision.skipped == 0:
                decision.mode = Mode.Propagate
            elif here in plan.continuation or here in plan.follow or anchor(here):
                decision.mode = Mode.PanicUntil   # a boundary was located, NOT a successful match
            else:
                decision.mode = Mode.Propagate

    state.total_skipped += decision.skipped
    diagnostic = Diagnostic(plan.rule, at, list(plan.expected), decision, '', offset, length,
                            f'unexpected {actual} in {plan.rule}')
    state.diagnostics.append(diagnostic)
    if state.exhausted():
        decision.mode = Mode.Abort
    return decision


class Policy(tuple):
    """(allow_insertion, allow_deletion, allow_replacement, max_skip)"""
    def __new__(cls, allow_insertion=True, allow_deletion=True, allow_replacement=True, max_skip=64):
        if max_skip <= 0:
            raise ValueError('Recovery.Policy max_skip must be positive')
        return super().__new__(cls, (bool(allow_insertion), bool(allow_deletion),
                                     bool(allow_replacement), int(max_skip)))


class Handler:
    """Typed generated failure handler (Recovery::Handler). Called as handler(pos, state)."""
    _ispa_recovering = True

    def __init__(self, expected, continuation, follow, synchronization, policy, rule, expected_text):
        self.expected, self.continuation = list(expected), list(continuation)
        self.follow, self.synchronization = list(follow), list(synchronization)
        self.policy = Policy(*policy)
        self.rule, self.expected_text = rule, expected_text

    def __call__(self, pos, state: State) -> Decision:
        ins, dele, repl, max_skip = self.policy
        decision = recover(pos, state, Plan(self.rule, self.expected, self.continuation, self.follow,
                                            self.synchronization, ins, dele, repl, max_skip))
        if state.diagnostics:
            diagnostic = state.diagnostics[-1]
            diagnostic.expected_text = self.expected_text
            diagnostic.message += f'; expected {self.expected_text}'
            suffix = {Mode.Insert: '; inserted missing token',
                      Mode.Replace: '; replaced unexpected token',
                      Mode.Delete: '; deleted unexpected token',
                      Mode.Abort: '; recovery limit reached'}.get(decision.mode)
            if suffix:
                diagnostic.message += suffix
            if decision.skipped > 1:
                diagnostic.message += f'; skipped {decision.skipped} tokens'
        return decision


def handler(expected=(), continuation=(), follow=(), synchronization=(), policy=(True, True, True, 64),
            rule: str = '', expected_text: str = '') -> Handler:
    return Handler(expected, continuation, follow, synchronization, policy, rule, expected_text)


def recover_encoded(pos, state: State, rule: str, expected: str, continuation: str, follow: str,
                    explicit_sync: str, allow_insertion=True, allow_deletion=True,
                    allow_replacement=True, max_skip=64) -> Decision:
    return recover(pos, state, Plan(rule, decode(expected), decode(continuation), decode(follow),
                                    decode(explicit_sync), allow_insertion, allow_deletion,
                                    allow_replacement, max_skip))


class RequiredStatus(enum.Enum):
    Matched = 0
    Missing = 1
    Boundary = 2
    Aborted = 3


@dataclass(slots=True)
class RequiredResult:
    """A recovered required value can lack a concrete Node: handle Missing/Boundary explicitly."""
    status: RequiredStatus = RequiredStatus.Boundary
    node: Any = None
    recovery: Decision = field(default_factory=Decision)


# ---------------------------------------------------------------- hierarchical (context) recovery

class ContextKind(enum.Enum):
    Mismatch = 0    # required member missing; skipped to a resume point
    Misplaced = 1   # token starts a construct owned by an outer frame
    Stray = 2       # token after a loop that no active frame can accept


@dataclass(slots=True)
class ContextClaim:
    """A production (or terminal) an active frame could start/continue with. `matches(token)`."""
    name: str = ''
    matches: Callable[[Any], bool] | None = None


@dataclass(slots=True)
class ContextDiagnostic:
    kind: ContextKind = ContextKind.Mismatch
    message: str = ''
    position: int = 0           # cursor position where the error was detected
    offset: int = ABSENT        # source offset, when available
    length: int = 0
    skipped: int = 0            # tokens skipped to reach the resume point
    rule: str = ''              # innermost rule when the error was detected
    resume_rule: str = ''       # frame that resumes parsing
    claim: str = ''             # production/terminal that resumes (empty at end of input)


# ---------------------------------------------------------------- opaque payload scanning

def scan_balanced(text: str, begin: int, boundaries, budget: int) -> tuple[int, bool]:
    """Scan opaque payloads without asking the host lexer to tokenize them.
    Returns (end_index, exhausted). The budget counts source characters, strings/comments included."""
    n = len(text)

    def at(i: int) -> str:
        return text[i] if i < n else '\0'

    p = begin
    closers: list[str] = []
    quote, triple, comment = '', False, False

    def advance(k: int) -> bool:
        nonlocal p
        if k > budget - (p - begin):
            return False
        p += k
        return True

    while at(p) != '\0':
        if p - begin >= budget:
            return p, True
        c = text[p]
        if comment:
            if c in '\n\r':
                comment = False
        elif quote:
            if c == '\\' and at(p + 1) != '\0':
                if not advance(2):
                    return p, True
                continue
            if c == quote and (not triple or (at(p + 1) == quote and at(p + 2) == quote)):
                if not advance(3 if triple else 1):
                    return p, True
                quote = ''
                continue
            # A broken single-line string must not swallow later declarations.
            if not triple and c in '\n\r':
                return p, True
        else:
            for boundary in boundaries:
                matches = all(at(p + i) != '\0' and at(p + i) == boundary[i] for i in range(len(boundary)))
                if (not closers or boundary == ';') and matches:
                    return p, False
            if c in ')]}':
                if not closers or closers[-1] != c:
                    return p, False
                closers.pop()
            elif c == '(':
                closers.append(')')
            elif c == '[':
                closers.append(']')
            elif c == '{':
                closers.append('}')
            elif c == '#':
                comment = True
            elif c in '\'"':
                quote = c
                triple = at(p + 1) == c and at(p + 2) == c
                if not advance(3 if triple else 1):
                    return p, True
                continue
        if not advance(1):
            return p, True
    return p, bool(quote)
