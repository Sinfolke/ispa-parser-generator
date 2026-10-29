"""lexer.hpp: Lexer_base and its two token cursors.

Subclass Lexer_base and override `makeToken` (plus whichever of init, clone,
checkpoint, restoreCheckpoint, externalAdvance, hasExternalRules, on_token,
on_eof, push, push_eof the grammar needs). Names match the C++ members.
Differences forced by the language-neutral model:
  * the source is a native String of 8-bit units and positions are Int offsets;
    `""` past the end stands for the NUL terminator;
  * `makeToken(cur)` receives a Cursor (position + line) instead of `const char*&`;
  * C++ overloads are split: push(token) / push_eof() (was push()),
    makeTokens() / makeTokensFromText(text); file loading is the host's job
    (cpp_residue/ispastdlib/file_io.hpp);
  * cursors are mutable objects: use copy() where C++ copies an iterator and
    assign() where it assigns one."""
from typing import Callable, ClassVar

from ispa_std.chars import at as char_at, is_space
from ispa_std.dfa_api import Captures, LEXER_DELETE_STATE, NULL_STATE, SourceText
from ispa_std.utility import Pair
from ispa_std.dfa_run import DfaTables, run
from ispa_std.failures import api_error, internal_error, logic_error, out_of_range, runtime_error
from ispa_std.format import int_to_string
from ispa_std.raw_token import AMBIGUOUS_ID, RawToken, RawTokenMeta
from ispa_std.refs import Cursor
from ispa_std.string import substr


class LexerDecision:
    NONE: ClassVar[int] = 0
    DELETE: ClassVar[int] = 1
    INSERT: ClassVar[int] = 2
    ABORT: ClassVar[int] = 3


class LexerFailureContext:
    src: str
    cursor: int
    diagnostics: list[str]
    rule: str
    failed_rule: str
    decision: int

    def __init__(self, src: str, cursor: int, diagnostics: list[str], rule: str, failed_rule: str):
        self.src = src
        self.cursor = cursor
        self.diagnostics = diagnostics
        self.rule = rule
        self.failed_rule = failed_rule
        self.decision = LexerDecision.NONE

    def at(self, value: str, offset: int) -> bool:
        """Is the unit `offset` positions ahead exactly `value` ("" = end)?"""
        p = self.cursor
        left = offset
        while left > 0 and p < len(self.src):
            p = p + 1
            left = left - 1
        return value == char_at(self.src, p)

    def error(self, message: str) -> None:
        self.diagnostics.append(message)

    def erase(self) -> None:
        self.decision = LexerDecision.DELETE

    def skip(self) -> None:
        self.decision = LexerDecision.DELETE

    def insert(self) -> None:
        self.decision = LexerDecision.INSERT

    def abort(self) -> None:
        self.decision = LexerDecision.ABORT


def is_token_end(token: RawToken) -> bool:
    """End-of-stream: an empty token that is not the ambiguous marker."""
    return token.empty() and not token.isAmbiguous()


class _SharedState:
    pos: int
    eof_processed: bool
    checkpoints: list[object]

    def __init__(self, pos: int):
        self.pos = pos
        self.eof_processed = False
        self.checkpoints = []


class Lexer_base:
    _src: SourceText
    _has_input: bool
    tokens: list[RawToken]
    errors: list[str]
    line: int
    external_boundary: bool
    # End of the source this lexer may read (-1: whole input). Replaces C++'s
    # BoundedSourceLexer wrapper; see parse_source_span.
    bound: int

    def __init__(self):
        self._src = SourceText("")
        self._has_input = False
        self.tokens = []
        self.errors = []
        self.line = 1
        self.external_boundary = False
        self.bound = -1

    # --- overridable ---------------------------------------------------------
    def makeToken(self, cur: Cursor) -> RawToken:
        logic_error("Lexer_base.makeToken is abstract")
        return self.empty_token()

    def init(self) -> None:
        return

    def empty_token(self) -> RawToken:
        """The default-constructed token (what C++ spells `Token{}`)."""
        return RawToken.empty_token()

    def ambiguous_token(self, before: int, after: int) -> RawToken:
        """Marker for one source unit an external rule may still claim."""
        meta = RawTokenMeta(before, self._src.text, True, 1, self.line_at(before), self.column_at(before), AMBIGUOUS_ID)
        return RawToken.ambiguous(meta)

    def clone(self) -> "Lexer_base":
        logic_error("External parser handoff requires a cloneable lexer")
        return self

    def checkpoint(self) -> object:
        return 0

    def restoreCheckpoint(self, state: object) -> None:
        return

    def externalAdvance(self, begin: int, end: int) -> None:
        return

    def hasExternalRules(self) -> bool:
        return False

    def on_token(self, token: RawToken) -> None:
        self.tokens.append(token)

    def on_eof(self) -> None:
        self.tokens.append(self.empty_token())

    def push(self, token: RawToken) -> None:
        self.on_token(token)

    def push_eof(self) -> None:
        self.on_eof()

    def append_token(self, token: RawToken) -> None:
        self.tokens.append(token)

    def has_buffered_tokens(self) -> bool:
        return len(self.tokens) > 0

    def pop_buffered_token(self) -> RawToken:
        return self.tokens.pop(0)

    # --- state ---------------------------------------------------------------
    def setExternalBoundary(self, enabled: bool) -> None:
        self.external_boundary = enabled

    def set_bound(self, end: int) -> None:
        """Limit lexing to source[:end] and make hitting it an external boundary."""
        self.bound = end
        self.external_boundary = True

    def hasInput(self) -> bool:
        return self._has_input

    def hasTokens(self) -> bool:
        return len(self.tokens) > 0

    def source(self) -> str:
        return self._src.text

    def setInput(self, text: str) -> "Lexer_base":
        self.errors.clear()
        self._src = SourceText(text)
        self._has_input = True
        return self

    def getTokens(self) -> list[RawToken]:
        return self.tokens

    def clearTokens(self) -> None:
        self.tokens.clear()

    def getErrors(self) -> list[str]:
        return self.errors

    def copy_base_into(self, other: "Lexer_base") -> None:
        """For clone() implementations: copy the base-class state."""
        other._src = self._src
        other._has_input = self._has_input
        other.tokens = []
        other.tokens.extend(self.tokens)
        other.errors = []
        other.errors.extend(self.errors)
        other.line = self.line
        other.external_boundary = self.external_boundary
        other.bound = self.bound

    def assign_from(self, other: "Lexer_base") -> None:
        """Lexer_base::operator=: tokens, errors and input (not line/flags)."""
        self.tokens = []
        self.tokens.extend(other.tokens)
        self.errors = []
        self.errors.extend(other.errors)
        self._src = other._src
        self._has_input = other._has_input

    def equals(self, other: "Lexer_base") -> bool:
        if len(self.tokens) != len(other.tokens):
            return False
        i = 0
        while i < len(self.tokens):
            if not _token_equal(self.tokens[i], other.tokens[i]):
                return False
            i = i + 1
        return True

    # --- position helpers ----------------------------------------------------
    def getCurrentPos(self, pos: int) -> int:
        return pos

    def skip_spaces(self, cur: Cursor) -> int:
        prev = cur.pos
        while is_space(char_at(self._src.text, cur.pos)):
            cur.pos = cur.pos + 1
        return cur.pos - prev

    def line_at(self, pos: int) -> int:
        if not self._has_input or pos < 0:
            logic_error("Lexer line lookup has no source position")
        if pos > len(self._src.text):
            out_of_range("Lexer line lookup exceeds source input")
        count = 1
        i = 0
        while i < pos:
            if self._src.text[i] == "\n":
                count = count + 1
            i = i + 1
        return count

    def column_at(self, pos: int) -> int:
        if not self._has_input or pos < 0:
            logic_error("Lexer column lookup has no source position")
        if pos > len(self._src.text):
            out_of_range("Lexer column lookup exceeds source input")
        count = 1
        i = 0
        while i < pos:
            if self._src.text[i] == "\n":
                count = 0
            else:
                count = count + 1
            i = i + 1
        return count

    def line_prefix(self, pos: int) -> str:
        if not self._has_input or pos < 0:
            logic_error("Lexer line prefix has no source position")
        begin = pos
        while begin > 0 and char_at(self._src.text, begin - 1) != "\n" and char_at(self._src.text, begin - 1) != "\r":
            begin = begin - 1
        return substr(self._src.text, begin, pos - begin)

    def next_char(self, pos: int) -> str:
        if pos < 0:
            logic_error("Lexer next character has no source position")
        return char_at(self._src.text, pos)

    def panic_mode(self, cur: Cursor) -> None:
        if cur.pos < len(self._src.text):
            cur.pos = cur.pos + 1

    def lexer_failure_context(self, cursor: int, rule: str, failed_rule: str) -> LexerFailureContext:
        return LexerFailureContext(self._src.text, cursor, self.errors, rule, failed_rule)

    def lexer_failure_result(self, ctx: LexerFailureContext, inserted: RawToken) -> Pair[int, RawToken]:
        """(state, token) for a lexer fail handler; see DFA run()."""
        if ctx.decision == LexerDecision.DELETE:
            return Pair[int, RawToken](LEXER_DELETE_STATE, self.empty_token())
        if ctx.decision == LexerDecision.INSERT:
            return Pair[int, RawToken](NULL_STATE, inserted)
        if len(self.errors) == 0:
            runtime_error("unmatched lexer input")
        else:
            runtime_error(self.errors[len(self.errors) - 1])
        return Pair[int, RawToken](NULL_STATE, inserted)

    def lookup(self, tables: DfaTables, semantic: Callable[[int, Captures, int, int, int, Cursor], Pair[int, RawToken]], cur: Cursor) -> RawToken:
        """Run the DFA for one token at cur (empty token at end of input)."""
        if cur.pos >= len(self._src.text):
            return self.empty_token()
        # `line` is shared with the scan: copy in, copy out.
        scan = Cursor(cur.pos, self.line)
        token = run(self._src, scan, self.getCurrentPos(cur.pos), tables, semantic, self.empty_token())
        cur.pos = scan.pos
        cur.line = scan.line
        self.line = scan.line  # the lexer's own `line` is the one that persists
        return token

    # --- eager tokenization ----------------------------------------------------
    def makeTokens(self) -> list[RawToken]:
        if self.hasExternalRules():
            logic_error("AMBIGUOUS eager token stream: use lazy tokenization with external parsers")
        if not self._has_input:
            api_error("can't lex because no input provided. ", [])
        cur = Cursor(0, self.line)
        n = len(self._src.text)
        while cur.pos < n:
            before = cur.pos
            token = self.makeToken(cur)
            if cur.pos == before:
                internal_error("no matches at position {}, on character {}, and no exception can handle it",
                               "Lexer TDFA generation", True,
                               [int_to_string(cur.pos), char_at(self._src.text, cur.pos)])
            if not is_token_end(token):
                self.push(token)
        self.push_eof()
        return self.tokens

    def makeTokensFromText(self, text: str) -> list[RawToken]:
        self.setInput(text)
        return self.makeTokens()


def _token_equal(a: RawToken, b: RawToken) -> bool:
    if a.empty() != b.empty():
        return False
    if a.empty():
        return a.kind() == b.kind()
    return a.kind() == b.kind() and a.index() == b.index() and a.meta().startpos() == b.meta().startpos() \
        and a.meta().length() == b.meta().length()


class TokenCursor:
    """What the parsers see of a token position. LazyIterator lexes on demand
    from source; TokenIterator walks tokens that already exist. Both keep their
    state in these fields so assign() works through the base type."""
    owner: Lexer_base
    state: _SharedState
    token_index: int
    distance_base: int
    owned_owner: Lexer_base
    has_owned: bool

    def __init__(self):
        self.owner = Lexer_base()
        self.state = _SharedState(0)
        self.token_index = 0
        self.distance_base = 0
        self.owned_owner = self.owner
        self.has_owned = False

    def copy(self) -> "TokenCursor":
        logic_error("TokenCursor.copy is abstract")
        return self

    def assign(self, other: "TokenCursor") -> None:
        self.owner = other.owner
        self.state = other.state
        self.token_index = other.token_index
        self.distance_base = other.distance_base
        self.owned_owner = other.owned_owner
        self.has_owned = other.has_owned

    def has_source(self) -> bool:
        return False

    def sourceOrigin(self) -> str:
        return ""

    def getErrors(self) -> list[str]:
        return self.owner.getErrors()

    def sourcePosition(self) -> int:
        return -1

    def consumeExternal(self, end: int) -> None:
        logic_error("TokenCursor.consumeExternal needs a lazy cursor")

    def isEnd(self) -> bool:
        logic_error("TokenCursor.isEnd is abstract")
        return True

    def next(self) -> "TokenCursor":
        logic_error("TokenCursor.next is abstract")
        return self

    def prev(self) -> "TokenCursor":
        logic_error("TokenCursor.prev is abstract")
        return self

    def advance_by(self, count: int) -> "TokenCursor":
        logic_error("TokenCursor.advance_by is abstract")
        return self

    def plus(self, count: int) -> "TokenCursor":
        out = self.copy()
        out.advance_by(count)
        return out

    def current(self) -> RawToken:
        logic_error("TokenCursor.current is abstract")
        return self.owner.empty_token()

    def minus(self, other: "TokenCursor") -> int:
        return self.distance() - other.distance()

    def distance(self) -> int:
        return self.distance_base + self.token_index

    def resumePosition(self) -> int:
        return -1

    def advanceCharacters(self, count: int) -> bool:
        runtime_error("advanceCharacters needs a lazy cursor (external parser integration)")
        return False


class LazyIterator(TokenCursor):
    """Lexer_base::lazy_iterator. Copies made with copy() share the token
    cache and source cursor but keep independent logical indices."""

    def __init__(self, lexer: Lexer_base, state: _SharedState, token_index: int, distance_base: int):
        self.owner = lexer
        self.state = state
        self.token_index = token_index
        self.distance_base = distance_base
        self.owned_owner = lexer
        self.has_owned = False

    @staticmethod
    def begin(lexer: Lexer_base) -> "LazyIterator":
        """lazy_iterator(lexer, in): start at the lexer's own input."""
        return LazyIterator.at_position(lexer, 0)

    @staticmethod
    def at_position(lexer: Lexer_base, start: int) -> "LazyIterator":
        """lazy_iterator(lexer, origin, start)."""
        it = LazyIterator(lexer, _SharedState(start), 0, 0)
        if start > 0:
            lexer.line = lexer.line_at(start)
        return it

    def copy(self) -> "TokenCursor":
        out = LazyIterator(self.owner, self.state, self.token_index, self.distance_base)
        out.owned_owner = self.owned_owner
        out.has_owned = self.has_owned
        return out

    def has_source(self) -> bool:
        return True

    def ensure_token(self) -> None:
        owner = self.owner
        state = self.state
        running = True
        while running and len(owner.tokens) <= self.token_index:
            if state.eof_processed:
                running = False
            else:
                if not owner.hasInput():
                    logic_error("Lazy lexer has no source input")
                if state.pos >= len(owner.source()):
                    state.eof_processed = True
                    owner.push_eof()
                    running = False
                else:
                    before = state.pos
                    checkpoint = owner.checkpoint()
                    cur = Cursor(state.pos, owner.line)
                    raw: RawToken = owner.empty_token()
                    if owner.bound < 0 or state.pos < owner.bound:
                        raw = owner.makeToken(cur)
                    state.pos = cur.pos
                    if raw.empty() and owner.external_boundary:
                        state.pos = before
                        state.eof_processed = True
                        owner.push_eof()
                        running = False
                    else:
                        if raw.empty() and not raw.isAmbiguous() and owner.hasExternalRules():
                            state.pos = before + 1
                            raw = owner.ambiguous_token(before, state.pos)
                        if state.pos == before:
                            runtime_error("Lazy lexer did not advance source position")
                        if not is_token_end(raw):
                            owner.push(raw)
                        while len(state.checkpoints) < len(owner.tokens):
                            state.checkpoints.append(checkpoint)

    def sourceOrigin(self) -> str:
        return self.owner.source()

    def getErrors(self) -> list[str]:
        return self.owner.getErrors()

    def sourcePosition(self) -> int:
        if self.token_index < len(self.owner.tokens):
            token = self.owner.tokens[self.token_index]
            if not is_token_end(token) and token.meta().has_source():
                return token.meta().startpos()
        return self.state.pos

    def consumeExternal(self, end: int) -> None:
        begin = self.sourcePosition()
        origin = self.owner.source()
        if end < begin or end > len(origin):
            logic_error("External parser returned an invalid source cursor")
        branch = self.owner.clone()
        if self.token_index < len(self.state.checkpoints):
            branch.restoreCheckpoint(self.state.checkpoints[self.token_index])
        branch.tokens.clear()
        branch._src = SourceText(origin)
        branch._has_input = True
        branch.line = branch.line_at(end)
        branch.externalAdvance(begin, end)
        extra = 0
        if end != begin:
            extra = 1
        self.distance_base = self.distance() + extra
        self.token_index = 0
        self.owned_owner = branch
        self.has_owned = True
        self.owner = branch
        self.state = _SharedState(end)

    def isEnd(self) -> bool:
        self.ensure_token()
        if self.token_index >= len(self.owner.tokens):
            return True
        return is_token_end(self.owner.tokens[self.token_index])

    def next(self) -> "TokenCursor":
        """operator++"""
        if not self.isEnd():
            self.token_index = self.token_index + 1
        return self

    def prev(self) -> "TokenCursor":
        """operator--"""
        if self.token_index > 0:
            self.token_index = self.token_index - 1
        return self

    def advance_by(self, count: int) -> "TokenCursor":
        """operator+="""
        left = count
        while left > 0 and not self.isEnd():
            self.next()
            left = left - 1
        return self

    def current(self) -> RawToken:
        """operator* / operator->"""
        self.ensure_token()
        if self.token_index >= len(self.owner.tokens):
            out_of_range("lazy_iterator: dereference beyond EOF")
        return self.owner.tokens[self.token_index]

    def resumePosition(self) -> int:
        """Source position just after the current token (-1 when unknown)."""
        self.ensure_token()
        if self.token_index >= len(self.owner.tokens):
            return self.state.pos
        token = self.owner.tokens[self.token_index]
        if is_token_end(token):
            return self.state.pos
        return token.meta().end_pos()

    def advanceCharacters(self, count: int) -> bool:
        """Resume lexing `count` source units after the current token's start,
        invalidating cached tokens from here on (for external parsers)."""
        self.ensure_token()
        if self.token_index >= len(self.owner.tokens):
            return False
        current = self.owner.tokens[self.token_index]
        if is_token_end(current) or not current.meta().has_source():
            return False
        target = current.meta().startpos()
        n = len(self.owner.source())
        i = 0
        while i < count:
            if target >= n:
                return False
            target = target + 1
            i = i + 1
        while len(self.owner.tokens) > self.token_index:
            self.owner.tokens.pop()
        self.state.pos = target
        self.state.eof_processed = False
        self.ensure_token()
        return True


class TokenIterator(TokenCursor):
    """Lexer_base::iterator over already-accumulated tokens (run makeTokens() first)."""

    def __init__(self, owner: Lexer_base, index: int):
        self.owner = owner
        self.state = _SharedState(0)
        self.token_index = index
        self.distance_base = 0
        self.owned_owner = owner
        self.has_owned = False

    @staticmethod
    def begin(owner: Lexer_base) -> "TokenCursor":
        return TokenIterator(owner, 0)

    def copy(self) -> "TokenCursor":
        return TokenIterator(self.owner, self.token_index)

    def next(self) -> "TokenCursor":
        self.token_index = self.token_index + 1
        return self

    def prev(self) -> "TokenCursor":
        if self.token_index != 0:
            self.token_index = self.token_index - 1
        return self

    def advance_by(self, count: int) -> "TokenCursor":
        self.token_index = self.token_index + count
        return self

    def isEnd(self) -> bool:
        if self.token_index >= len(self.owner.tokens):
            return True
        return is_token_end(self.owner.tokens[self.token_index])

    def current(self) -> RawToken:
        if self.token_index >= len(self.owner.tokens):
            out_of_range("token iterator: dereference beyond the token list")
        return self.owner.tokens[self.token_index]

    def resumePosition(self) -> int:
        """Source position after the current token, -1 when it has none.
        (The C++ body referenced members that do not exist and was never
        instantiated; this is its documented intent.)"""
        if self.token_index >= len(self.owner.tokens):
            return -1
        token = self.owner.tokens[self.token_index]
        if is_token_end(token) or not token.meta().has_source():
            return -1
        return token.meta().end_pos()
