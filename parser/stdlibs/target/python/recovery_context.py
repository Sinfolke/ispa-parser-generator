"""Rule / fail contexts handed to user actions (port of LLParser::RuleContext / FailContext)."""
from __future__ import annotations
from dataclasses import dataclass
from typing import Any, Callable

from . import recovery as R
from .lexer import LexError

MAX_ACTION_LOOKAHEAD = 64
MAX_ACTION_CHECKPOINTS = 32


@dataclass(frozen=True)
class TokenBoundary:
    name: str


def context_at(candidate, spelling: str, lookahead: int,
               token_name: Callable[[Any], str] | None, match_name: bool = False) -> bool:
    if lookahead > MAX_ACTION_LOOKAHEAD:
        raise IndexError('ctx.at lookahead exceeds 64 tokens')
    cursor = candidate.copy()
    for _ in range(lookahead):
        if cursor.isEnd():
            return False
        cursor.advance()
    if cursor.isEnd():
        return False
    token = cursor.peek()
    if not match_name:
        return bool(getattr(token, 'length', 0)) and token.text() == spelling
    if token_name is None:
        return False
    name = spelling.replace('::', '.')
    actual = token_name(R.token_id(token))
    return actual == name or (len(actual) > len(name) and actual.endswith(name)
                              and actual[len(actual) - len(name) - 1] == '.')


class ActionCheckpoint:
    def __init__(self, pos) -> None:
        self.pos = pos
        self.checkpoints: list[tuple[str, Any]] = []

    def mark(self, name: str) -> None:
        if not name:
            raise ValueError('checkpoint name cannot be empty')
        if len(self.checkpoints) >= MAX_ACTION_CHECKPOINTS:
            raise OverflowError('action checkpoint limit (32) exceeded')
        self.checkpoints.append((name, self.pos.copy()))

    def _find(self, name: str) -> int:
        for i in range(len(self.checkpoints) - 1, -1, -1):
            if self.checkpoints[i][0] == name:
                return i
        raise LookupError(f"unknown action checkpoint '{name}'")

    def restore(self, name: str) -> None:
        index = self._find(name)
        self.pos.commit(self.checkpoints[index][1])
        del self.checkpoints[index + 1:]

    def commit(self, name: str) -> None:
        del self.checkpoints[self._find(name):]

    def at_token(self, name: str, lookahead: int = 0) -> bool:
        return context_at(self.pos, name, lookahead, self.token_name, True)

    @staticmethod
    def token_boundary(name: str) -> TokenBoundary:
        return TokenBoundary(name)


class RuleContext(ActionCheckpoint):
    """Borrows invocation-local storage: cursor and AST changes are immediately visible."""

    def __init__(self, pos, ast, start, rule: str,
                 token_name: Callable[[Any], str] | None = None) -> None:
        super().__init__(pos)
        self.ast, self.start_pos, self.rule, self.token_name = ast, start, rule, token_name

    def at(self, spelling: str, lookahead: int = 0) -> bool:
        return context_at(self.pos, spelling, lookahead, self.token_name)

    def collect_braced(self) -> list:
        """Tokens of a braced region after its opening token was consumed. The closing token is
        consumed but not returned; nested braces are preserved."""
        pos, collected = self.pos, []
        lexer = pos.lexer
        if pos.isEnd():
            raise RuntimeError('unterminated braced region in parser action')
        begin = pos.sourcePosition()
        end, exhausted = R.scan_balanced(lexer.source, begin, ['}'], len(lexer.source) - begin)
        if not exhausted and end < len(lexer.source) and lexer.source[end] == '}':
            while not pos.isEnd() and pos.sourcePosition() < end:
                collected.append(pos.peek())
                pos.advance()
            if pos.isEnd() or pos.sourcePosition() != end or not self.at('}'):
                raise RuntimeError('braced region boundary is not a token')
            pos.advance()
            return collected
        depth = 1
        while not pos.isEnd():
            if self.at('{'):
                depth += 1
            elif self.at('}'):
                depth -= 1
                if depth == 0:
                    pos.advance()
                    return collected
            collected.append(pos.peek())
            pos.advance()
        raise RuntimeError('unterminated braced region in parser action')


class FailContext(ActionCheckpoint):
    def __init__(self, pos, ast, start, rule: str, failed_rule: str, recovery: R.State,
                 source: str | None, diagnostic_begin: int,
                 token_name: Callable[[Any], str] | None = None) -> None:
        super().__init__(pos)
        self.ast, self.start_pos, self.rule, self.failed_rule = ast, start, rule, failed_rule
        self.recovery, self.source, self.diagnostic_begin = recovery, source, diagnostic_begin
        self.token_name = token_name
        self.requested: R.Mode | None = None
        self.skipped = 0

    def at(self, spelling: str, lookahead: int = 0) -> bool:
        return context_at(self.pos, spelling, lookahead, self.token_name)

    # Custom diagnostics use the same collection as automatic repairs.
    def error(self, message: str, code: str = '', source_offset: int | None = None,
              source_length: int | None = None) -> None:
        if (source_offset is None) != (source_length is None):
            raise ValueError('ctx.error source span requires both offset and length')
        if self.recovery.exhausted():
            self.abort()
            return
        d = R.Diagnostic(rule=self.rule, expected_text=self.failed_rule, message=message, code=code)
        pos = self.pos
        if not pos.isEnd():
            try:
                token = pos.peek()
                d.unexpected = R.token_id(token)
                d.offset, d.length = int(token.startpos), int(token.length)
            except LexError:
                # A broken foreign payload cannot be tokenized: report the raw position.
                d.offset, d.length = pos.lexer._cursor.index, 1
        elif self.source is not None:
            d.offset = len(self.source)
        if source_offset is not None:
            size = len(self.source) if self.source is not None else 0
            if source_offset < 0 or source_offset > size or source_length < 0 or source_length > size - source_offset:
                raise IndexError('ctx.error source span is outside parser input')
            d.offset, d.length = source_offset, source_length
        self.recovery.diagnostics.append(d)

    def note(self, message: str) -> None:
        if len(self.recovery.diagnostics) <= self.diagnostic_begin:
            raise RuntimeError('ctx.note requires a preceding ctx.error in this fail block')
        self.recovery.diagnostics[-1].notes.append(message)

    def sync(self, *boundaries: str) -> None:
        """Skip source text (balanced, string/comment aware) up to one of the boundaries."""
        if not boundaries:
            raise TypeError('ctx.sync requires at least one boundary')
        if self.recovery.exhausted() or self.requested == R.Mode.Abort:
            self.abort()
            return
        if any(not stop for stop in boundaries):
            raise ValueError('ctx.sync requires nonempty boundaries')
        pos = self.pos
        begin = pos.sourcePosition()
        budget = min(4096, self.recovery.max_total_skipped - self.recovery.total_skipped)
        end, exhausted = R.scan_balanced(pos.lexer.source, begin, boundaries, budget)
        count = end - begin
        if count:
            pos.consume_external(end)
        self.skipped += count
        self.recovery.total_skipped += count
        if exhausted or self.recovery.exhausted():
            self.abort()

    def sync_tokens(self, *boundaries: str | TokenBoundary) -> None:
        """Skip tokens, preserving token-stream structure (e.g. synthetic indentation)."""
        if not boundaries:
            raise TypeError('ctx.sync_tokens requires at least one boundary')
        if any(not (stop.name if isinstance(stop, TokenBoundary) else stop) for stop in boundaries):
            raise ValueError('ctx.sync_tokens requires nonempty boundaries')
        while True:
            if self.recovery.exhausted() or self.requested == R.Mode.Abort:
                self.abort()
                return
            if self.pos.isEnd():
                return
            if any(self.at_token(stop.name) if isinstance(stop, TokenBoundary) else self.at(stop)
                   for stop in boundaries):
                return
            self.skip()

    def skip(self) -> None:
        if self.recovery.exhausted():
            self.abort()
            return
        if not self.pos.isEnd():
            self.pos.advance()
            self.skipped += 1
            self.recovery.total_skipped += 1

    def insert(self) -> None:
        if self.requested != R.Mode.Abort:
            self.requested = R.Mode.Insert

    def replace(self) -> None:
        self.skip()
        self.insert()

    def retry(self) -> None:
        if self.requested != R.Mode.Abort:
            self.requested = R.Mode.Retry

    def abort(self) -> None:
        self.requested = R.Mode.Abort

    def finish(self, before) -> R.Decision:
        begin, end = before.index, self.pos.index
        moved = end - begin if end > begin else 0
        # Also account for legacy handlers that change ctx.pos directly.
        if moved > self.skipped:
            self.recovery.total_skipped += moved - self.skipped
        if self.recovery.exhausted():
            mode = R.Mode.Abort
        elif self.requested is not None:
            mode = self.requested
        else:
            mode = R.Mode.Insert if begin == end else R.Mode.Retry
        if mode == R.Mode.Insert and moved:
            mode = R.Mode.Replace
        decision = R.Decision(mode, skipped=max(moved, self.skipped))
        for diagnostic in self.recovery.diagnostics[self.diagnostic_begin:]:
            diagnostic.decision = R.Decision(decision.mode, decision.synthesized,
                                             diagnostic.unexpected, decision.skipped)
        return decision


class ActionFailure:
    """custom_failure(action): wraps a user fail block so required() runs the recovering path."""
    custom_action = True
    _ispa_recovering = True

    def __init__(self, action: Callable) -> None:
        self.action = action

    def __call__(self, pos, state: R.State):
        return self.action(pos, state)


def custom_failure(action: Callable) -> ActionFailure:
    return ActionFailure(action)


def custom_failure_decision(before, after, ctx: FailContext | None = None) -> R.Decision:
    if ctx is not None:
        return ctx.finish(before)
    return R.Decision(R.Mode.Insert if before.index == after.index else R.Mode.Retry)
