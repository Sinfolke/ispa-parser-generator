"""Eager and lazy token sources with independently copyable parser cursors."""
from __future__ import annotations
from dataclasses import dataclass
from typing import Any, Callable, Generic, TypeVar
from .model import Position, RawToken

T = TypeVar('T')


class LexError(RuntimeError):
    """Lexing failed at `offset`; carries 1-based line/column and the offending character."""
    def __init__(self, offset: int, line: int, column: int, char: str):
        super().__init__(f'lexer made no progress at line {line}, column {column} '
                         f'(offset {offset}, character {char!r})')
        self.offset, self.line, self.column, self.char = offset, line, column, char


class Lexer(Generic[T]):
    def __init__(self, source: str = '') -> None:
        self._reset(source)

    def _reset(self, source: str) -> None:
        self.source = source
        self.tokens: list[T] = []
        self._cursor = Position(source)
        self._finished = False
        self._token_offsets: list[int] = []
        self._boundary: int | None = None
        self.line = 1

    def set_input(self, source: str) -> None:
        # Re-initialises lexer state only; never re-runs a generated subclass __init__
        # (which may take different arguments or hold extra state).
        self._reset(source)

    def setExternalBoundary(self, enabled: bool) -> None:
        self._boundary = len(self.source) if enabled else None

    def makeToken(self, pos: Position) -> T | None:
        raise NotImplementedError('A generated lexer must implement makeToken')

    def lookup(self, table, class_table, action_table, layout, semantic,
               debug_array=(), debug_index=None, pos: Position | None = None) -> T | None:
        # Uninstrumented generated lexers pass only the cursor after semantic.
        # Instrumented lexers additionally pass debug_array and debug_index.
        if pos is None and isinstance(debug_array, Position):
            pos, debug_array = debug_array, ()
        if pos is None:
            raise TypeError('lookup requires a source position')
        from .dfa import TdfaLayout, run_tdfa
        import ispa_debug
        if pos.at_end():
            return None
        if layout is None:
            registers = 0
            for action, dst, src, _next in action_table:
                registers = max(registers, dst + 1)
                if action == 3 and src >= 0:
                    registers = max(registers, src + 1)
            layout = TdfaLayout(registers, 0)
        tracing = ispa_debug.enabled()
        if tracing:
            ispa_debug.event('lexer_enter', 'lexer scan')
        token = None
        actual = self.source[pos.index:pos.index + 1]
        try:
            token, self.line = run_tdfa(self.source, pos, table, class_table,
                                        action_table, layout, semantic, self.line,
                                        debug_array, debug_index)
        finally:
            if tracing:
                ispa_debug.match('lexer scan', actual, token is not None,
                                 'matched' if token is not None else 'DFA found no accepting token')
        return token

    def lookup_dfa(self, index: int, pos: Position) -> T | None:
        table = getattr(self, f'dfa_table_{index}', None)
        if table is None:
            raise NotImplementedError(f'ISPA DFA #{index}: missing generated table')
        # The producer must provide the full TDFA table bundle (not just transitions).
        if not isinstance(table, tuple) or len(table) != 5:
            raise NotImplementedError('Legacy nested DFA tables must be lowered before Python execution')
        classes, actions, layout, semantic = table[1:]
        return self.lookup(table[0], classes, actions, layout, semantic, (), {}, pos)

    def checkpoint(self) -> Any:
        return None

    def restore(self, _state: Any) -> None:
        pass

    def push(self) -> None:
        """Override to inject EOF tokens/dedents. Called exactly once."""
        pass

    def append_token(self, token: T) -> None:
        self.tokens.append(token)

    def _ensure(self, index: int) -> None:
        while index >= len(self.tokens) and not self._finished:
            if self._cursor.index >= len(self.source) or (
                self._boundary is not None and self._cursor.index >= self._boundary
            ):
                self._finished = True
                self.push()
                return
            before = self._cursor.index
            token = self.makeToken(self._cursor)
            if self._cursor.index <= before:
                if self._boundary is not None:
                    self._finished = True
                    return
                import ispa_debug
                ispa_debug.breakpoint('lexer made no progress')
                raise LexError(before, self._line(self._cursor, before),
                               self._column(self._cursor, before),
                               self.source[before:before + 1])
            if not self._is_skip(token):
                self._token_offsets.append(before)
                if getattr(self, '_use_on_token', False):
                    self.on_token(token)
                else:
                    self.tokens.append(token)
            # else: a lexical skip may return None; progress without a token is fine.

    @staticmethod
    def _is_skip(token: Any) -> bool:
        """None or a default-constructed (zero-width, payload-less) token means "skipped input".
        A payload-less token that *covers input* (e.g. punctuation) is a real token."""
        if token is None:
            return True
        return bool(hasattr(token, 'empty') and token.empty() and getattr(token, 'length', 0) == 0)

    def makeTokens(self, source: str | None = None) -> list[T]:
        if source is not None:
            self.set_input(source)
        while not self._finished:
            self._ensure(len(self.tokens))
        return self.tokens

    def hasTokens(self) -> bool:
        return bool(self.tokens)

    def hasInput(self) -> bool:
        return bool(self.source)

    def iterator(self) -> 'Cursor[T]':
        self.makeTokens()
        return Cursor(self)

    def lazy_iterator(self) -> 'Cursor[T]':
        return Cursor(self)

    # Single-underscore on purpose: double-underscore names are mangled per class and
    # therefore unreachable from the generated Lexer subclass.
    def _line(self, pos: Position | int, index: int | None = None) -> int:
        at = (pos.index if isinstance(pos, Position) else pos) if index is None else index
        return self.source.count('\n', 0, at) + 1

    def _column(self, pos: Position | int, index: int | None = None) -> int:
        at = (pos.index if isinstance(pos, Position) else pos) if index is None else index
        return at - self.source.rfind('\n', 0, at)

    def _next_char(self, pos: Position | int) -> str:
        at = pos.index if isinstance(pos, Position) else pos
        return self.source[at:at + 1]

    def _line_prefix(self, pos: Position | int) -> str:
        at = pos.index if isinstance(pos, Position) else pos
        return self.source[self.source.rfind('\n', 0, at) + 1:at]

    def panic_mode(self, pos: Position) -> None:
        """Lexer-level recovery: drop one character so lexing can make progress."""
        if not pos.at_end():
            pos.advance()

    def skip_spaces(self, pos: Position) -> int:
        before = pos.index
        while not pos.at_end() and pos.peek().isspace():
            pos.advance()
        return pos.index - before


@dataclass(slots=True)
class Cursor(Generic[T]):
    lexer: Lexer[T]
    index: int = 0

    def copy(self) -> 'Cursor[T]':
        return Cursor(self.lexer, self.index)

    def commit(self, other: 'Cursor[T]') -> None:
        if other.lexer is not self.lexer:
            raise ValueError('Cannot commit cursor from a different lexer')
        self.index = other.index

    def peek(self, offset: int = 0) -> T | None:
        i = self.index + offset
        if i < 0:
            return None
        self.lexer._ensure(i)
        return self.lexer.tokens[i] if i < len(self.lexer.tokens) else None

    def advance(self, count: int = 1) -> 'Cursor[T]':
        self.index += count
        return self

    def isEnd(self) -> bool:
        return self.peek() is None

    def sourcePosition(self) -> int:
        token = self.peek()
        if token is not None and hasattr(token, 'startpos'):
            return token.startpos
        return self.lexer._cursor.index

    def consume_external(self, end: int) -> None:
        """Skip source up to `end` without tokenizing it (recovery `ctx.sync`).
        Cached tokens that start inside the skipped span are dropped; if the lexer has not
        finished, everything cached from this cursor on is dropped and lexing resumes at `end`."""
        lex = self.lexer
        if not lex._finished:
            del lex.tokens[self.index:]
            del lex._token_offsets[self.index:]
            lex._cursor.index = max(lex._cursor.index, 0)
            lex._cursor.index = end
            lex.line = lex._line(end)
        else:
            while self.index < len(lex.tokens) and 0 <= lex.tokens[self.index].startpos < end:
                del lex.tokens[self.index]

    def distance(self, other: 'Cursor[T]') -> int:
        return self.index - other.index

    def __iadd__(self, n: int) -> 'Cursor[T]':
        return self.advance(n)

    def __eq__(self, other: object) -> bool:
        return isinstance(other, Cursor) and other.lexer is self.lexer and other.index == self.index
