"""ISPA LL combinators with the return convention of ispastdlib/parser_ll.hpp.

required/optional -> Node; counted/range/range_optional -> list[Node].
Generated rule callbacks return MatchResult(status=..., node=..., it=cursor).
"""
from __future__ import annotations
from dataclasses import dataclass, field
from typing import Any, Callable
from .model import MatchResult, Node, RawToken
from .lexer import Lexer, Cursor
from . import recovery as R
from .recovery_context import (RuleContext, FailContext, custom_failure, custom_failure_decision)
import ispa_debug


class ParseError(RuntimeError):
    """`offset` is a token index; `line`/`column` (1-based) are filled when the lexer is known."""
    def __init__(self, expected: Any, offset: int, line: int | None = None,
                 column: int | None = None):
        where = f'token index {offset}' + (f' (line {line}, column {column})' if line else '')
        super().__init__(f'expected {expected!r} at {where}')
        self.expected, self.offset, self.line, self.column = expected, offset, line, column


class _NoMatch(Exception):
    """A required descendant did not start an optional rule."""


@dataclass(slots=True)
class CountSpec:
    # C++ field order: name, count, minimum.
    name: str = ''
    count: int | None = None
    minimum: int = 0

    @classmethod
    def fixed(cls, count: int, minimum: int = 0) -> 'CountSpec':
        if count < 0:
            raise ValueError('repetition count cannot be negative')
        return cls('', count, max(count, minimum))

    @classmethod
    def named(cls, name: str, minimum: int = 0) -> 'CountSpec':
        return cls(name, None, minimum)

    @property
    def maximum(self) -> int | None:
        return self.count


def _as_matched(node: Node) -> Node:
    """A token that matched must not look like "no match": the cursor has already moved,
    so a payload-less token (absent=True) would make required() raise after consuming it."""
    if not node.absent:
        return node
    return Node(node.kind, node.value, node.startpos, node.length,
                node.line, node.column, node.source, False)


CONTEXT_MAX_SKIP = 64


@dataclass(slots=True)
class Frame:
    rule: str
    expecting: str = ''
    claims: list = field(default_factory=list)


class FrameGuard:
    """Pops the rule's frame. Python has no RAII: generated code closes it in a finally block
    (or uses it as a context manager). Abandoned frames count down the unwind request; the frame
    that is the resume target is the first one that sees it reach zero."""
    __slots__ = ('parser', 'closed')

    def __init__(self, parser: 'LLParser', rule: str) -> None:
        self.parser, self.closed = parser, False
        parser.frames.append(Frame(rule))
        parser.recovery_state.anchors.append([])

    def close(self) -> None:
        if self.closed:
            return
        self.closed = True
        p = self.parser
        p.frames.pop()
        p.recovery_state.anchors.pop()
        if p._unwind:
            p._unwind -= 1

    def __enter__(self) -> 'FrameGuard':
        return self

    def __exit__(self, *_exc) -> None:
        self.close()


class CounterScope:
    def __init__(self, parser):
        self.parser = parser
        self.saved = parser.counters.copy()
        self.closed = False

    def close(self):
        if not self.closed:
            self.parser.counters = self.saved
            self.closed = True


class LLParser:
    def __init__(self, lexer: Lexer | None = None) -> None:
        self.lexer = lexer
        self.tree: Any = None
        self.errors: list[ParseError] = []
        self.counters: dict[str, int] = {}
        self._probes: list[list[int | bool]] = []
        self._reset_recovery()

    def _reset_recovery(self) -> None:
        self.counters.clear()
        state = getattr(self, 'recovery_state', None)
        if state is None:
            self.recovery_state = R.State()
        else:   # limits (max_errors / max_total_skipped) belong to the caller and survive a reset
            state.diagnostics.clear()
            state.anchors.clear()
            state.total_skipped = 0
        self.frames: list[Frame] = []
        self._unwind = 0
        self._last_resume_pos: int | None = None
        self._recovering = False
        self.context_errors: list[R.ContextDiagnostic] = []

    def getErrors(self) -> list:
        """Diagnostics of the current/last parse, including a parse that threw."""
        return self.recovery_state.diagnostics

    def getContextErrors(self) -> list:
        """Diagnostics produced by hierarchical (context) recovery."""
        return self.context_errors

    def counter_scope(self):
        return CounterScope(self)

    def bind_counter(self, name, value):
        count = len(value) if isinstance(value, (list, tuple)) else int(not value.empty())
        if name in self.counters and self.counters[name] != count:
            raise RuntimeError(f"Shared counter '{name}' has a different count")
        self.counters[name] = count

    def parse(self, source: Lexer | str, rule: Callable[[Cursor], MatchResult] | None = None) -> Any:
        lex = source if isinstance(source, Lexer) else self._make_lexer(source)
        self.lexer = lex
        self.errors.clear()
        self._probes.clear()
        self._reset_recovery()
        start = lex.lazy_iterator()
        match = (rule or getattr(self, 'main'))(start)
        end = match.it if match.it is not None else start
        if not match.status:
            raise self._error('complete input', end)
        self.skip_spaces(end)
        if not end.isEnd():
            raise self._error('complete input', end)
        self.tree = match.node
        return self.tree

    def _error(self, expected: Any, pos: Cursor | None) -> ParseError:
        """Build a ParseError, resolving line/column from the failing token when possible."""
        index = 0 if pos is None else pos.index
        line = column = None
        lex = self.lexer
        if pos is not None and lex is not None:
            try:
                offset = pos.sourcePosition()
                line, column = lex._line(offset), lex._column(offset)
            except Exception:  # never let diagnostics mask the real parse error
                pass
        return ParseError(expected, index, line, column)

    def _make_lexer(self, source: str) -> Lexer:
        lexer_cls = getattr(type(self), 'lexer_type', None)
        if lexer_cls is None:
            raise TypeError('The generated Parser must set lexer_type = Lexer')
        return lexer_cls(source)

    # ------------------------------------------------------------------ hierarchical recovery
    # Every rule owns a frame (recovery_frame). Before each child member generated code calls
    # recovery_at(sets, expecting, *names), recording what the rule could still accept. On a
    # committed failure local repair cannot fix, the parser scans ahead for the earliest token
    # claimed by any active frame (innermost first), skips to it and unwinds to the owner.

    def recovery_frame(self, rule: str) -> 'FrameGuard':
        return FrameGuard(self, rule)

    def recovery_at(self, sets=(), expecting: str = '', *names: str) -> None:
        if isinstance(sets, str) or not self.frames:   # str: pre-recovery generated code
            return
        frame = self.frames[-1]
        frame.expecting = expecting
        frame.claims = []
        anchors = self.recovery_state.anchors[-1]
        anchors.clear()
        for i, ids in enumerate(sets):
            ids = frozenset(ids)
            anchors.extend(ids)
            frame.claims.append(R.ContextClaim(
                names[i] if i < len(names) else '',
                lambda token, ids=ids: R.token_id(token) in ids))

    def unwinding(self) -> bool:
        return self._unwind != 0

    @staticmethod
    def describe_token(pos: Cursor) -> str:
        if pos.isEnd():
            return 'end of input'
        token = pos.peek()
        if getattr(token, 'length', 0) and hasattr(token, 'text'):
            return "'" + token.text() + "'"
        return '<token>'

    def breadcrumb(self) -> str:
        return ' > '.join(f.rule for f in self.frames)

    def _resolve_context(self, pos: Cursor, max_skip: int):
        """Earliest token (within max_skip) claimed by any frame; ties go to the innermost frame.
        If the identical spot already failed, the innermost frame may not claim it again."""
        repeat_spot = pos.index == self._last_resume_pos
        cursor, k = pos.copy(), 0
        while k <= max_skip and not cursor.isEnd():
            token = cursor.peek()
            for f in range(len(self.frames) - 1, -1, -1):
                if k == 0 and repeat_spot and f + 1 == len(self.frames):
                    continue
                for claim in self.frames[f].claims:
                    if claim.matches is not None and claim.matches(token):
                        return k, f, claim
            k += 1
            cursor.advance()
        if cursor.isEnd() and self.frames:
            return k, 0, None
        return None

    def _report_context(self, pos: Cursor, kind: R.ContextKind, skip: int, frame: int,
                        claim: R.ContextClaim | None) -> None:
        inner, target = self.frames[-1], self.frames[frame]
        d = R.ContextDiagnostic(kind=kind, position=pos.index, skipped=skip, rule=inner.rule,
                                resume_rule=target.rule, claim=claim.name if claim else '')
        if not pos.isEnd():
            token = pos.peek()
            d.offset, d.length = int(token.startpos), int(token.length)
        elif self.lexer is not None:
            d.offset = len(self.lexer.source)
        tok, where = self.describe_token(pos), self.breadcrumb()
        expected = f'; expected {inner.expecting}' if inner.expecting else ''
        if kind == R.ContextKind.Misplaced:
            d.message = (f"unexpected {tok} while parsing '{d.rule}'{expected}; continuing at "
                         f"'{d.claim}' in enclosing rule '{d.resume_rule}'")
        elif kind == R.ContextKind.Stray:
            d.message = (f'unexpected {tok} in {where}; no enclosing rule accepts it here; '
                         f'skipped {skip} token(s)'
                         + (f"; resuming at '{d.claim}' of '{d.resume_rule}'" if claim
                            else '; reached end of input'))
        else:
            d.message = f'unexpected {tok} in {where}{expected}'
            if skip:
                d.message += f'; skipped {skip} token(s)'
            d.message += (f"; resuming at '{d.claim}' of '{d.resume_rule}'" if claim
                          else '; reached end of input')
        self.context_errors.append(d)

    def recover_at_context(self, pos: Cursor, kind: R.ContextKind) -> bool:
        """Local repair failed (or does not apply): find a recovery point in the enclosing
        frames, skip to it and request unwinding."""
        state = self.recovery_state
        if not self.frames or state.exhausted():
            return False
        remaining = state.max_total_skipped - state.total_skipped
        found = self._resolve_context(pos, min(CONTEXT_MAX_SKIP, remaining))
        if found is None:
            return False
        skip, frame, claim = found
        if frame + 1 != len(self.frames) and skip == 0 and claim is not None:
            kind = R.ContextKind.Misplaced
        if not self._recovering or kind == R.ContextKind.Misplaced:
            self._report_context(pos, kind, skip, frame, claim)
        for _ in range(skip):
            if pos.isEnd():
                break
            pos.advance()
        state.total_skipped += skip
        self._last_resume_pos = pos.index
        self._unwind = len(self.frames) - 1 - frame
        self._recovering = True
        return True

    def sync_loop(self, pos: Cursor) -> None:
        """A loop ended on a token no active frame can accept: report where the mistake is."""
        if pos.isEnd() or not self.frames or self.unwinding():
            return
        candidate = pos.copy()
        ws = getattr(type(self), 'parser_ws_type', None)
        if ws is not None:
            while not candidate.isEnd():
                token = candidate.peek()
                if isinstance(token, RawToken) and token.holds(ws):
                    candidate.advance()
                else:
                    break
        if candidate.isEnd():
            return
        token = candidate.peek()
        # External FIRST sets are predicates over source text: such a token cannot be classified.
        ambiguous = getattr(token, 'isAmbiguous', None)
        if callable(ambiguous) and ambiguous():
            return
        any_claims = False
        for frame in self.frames:
            for claim in frame.claims:
                any_claims = True
                if claim.matches is not None and claim.matches(token):
                    return   # someone owns it
        if not any_claims:
            return
        pos.commit(candidate)
        self.recover_at_context(pos, R.ContextKind.Stray)

    def recover_choice(self, pos: Cursor, rule: str, expected: str,
                       alternatives: list[tuple[Any, ...]]) -> int:
        """Called only after normal LL(k) selection failed. Prefer an unambiguous FIRST token,
        then an unambiguous terminal inside an alternative. Returns the alternative or -1."""
        if len(alternatives) % 2:
            raise ValueError(f'invalid recovery alternatives for {rule}')
        self.reject_absent_rule()
        state = self.recovery_state
        d = R.Diagnostic(rule=rule, expected_text=expected,
                         message=f'unexpected {self.describe_token(pos)} in {rule}; '
                                 f'expected one of: {expected}')
        if not pos.isEnd():
            token = pos.peek()
            d.unexpected = R.token_id(token)
            d.offset, d.length = int(token.startpos), int(token.length)
        for i in range(0, len(alternatives), 2):
            d.expected.extend(alternatives[i])
        sets = [tuple(R.ABSENT if x is None else x for x in alt) for alt in alternatives]
        selected, skipped = -1, 0
        if not state.exhausted():
            while not pos.isEnd():
                ident = R.token_id(pos.peek())
                for phase in (0, 1):
                    if selected != -1:
                        break
                    matches = [i // 2 for i in range(phase, len(sets), 2) if ident in sets[i]]
                    if len(matches) > 1:
                        break
                    if matches:
                        selected = matches[0]
                if selected != -1:
                    break
                boundary = any(ident in anchors for anchors in state.anchors)
                if (boundary or skipped >= CONTEXT_MAX_SKIP
                        or state.total_skipped + skipped >= state.max_total_skipped):
                    break
                pos.advance()
                skipped += 1
        state.total_skipped += skipped
        d.decision.skipped = skipped
        d.decision.mode = R.Mode.Propagate if selected == -1 else R.Mode.Synchronize
        d.message += ('; no unambiguous alternative at recovery boundary' if selected == -1
                      else f'; continuing in alternative {selected + 1}')
        if skipped:
            d.message += f'; skipped {skipped} tokens'
        state.diagnostics.append(d)
        if state.exhausted():
            d.decision.mode = R.Mode.Abort
            d.message += '; recovery limit reached'
            return -1
        return selected

    def reject_absent_rule(self) -> None:
        if self._probes and not self._probes[-1][1]:
            raise _NoMatch()

    def note_match(self) -> None:
        self._recovering = False

    def fail_context(self, pos: Cursor, ast: Any, start: Cursor, rule: str, failed_rule: str,
                     token_name: Callable | None = None) -> FailContext:
        return FailContext(pos, ast, start, rule, failed_rule, self.recovery_state,
                           self.lexer.source if self.lexer is not None else None,
                           len(self.recovery_state.diagnostics), token_name)

    @staticmethod
    def rule_context(pos: Cursor, ast: Any, start: Cursor, rule: str,
                     token_name: Callable | None = None) -> RuleContext:
        return RuleContext(pos, ast, start, rule, token_name)

    custom_failure = staticmethod(custom_failure)
    custom_failure_decision = staticmethod(custom_failure_decision)

    @staticmethod
    def action_result(value: Any, pos: Cursor) -> MatchResult:
        return MatchResult(False, Node(value=value), pos)

    def _skip_ws(self, pos: Cursor, expected: Any, callback: Callable | None) -> Cursor:
        """Skip whitespace tokens (committing them into `pos`) and return a trial cursor."""
        trial = pos.copy()
        whitespace = getattr(type(self), 'parser_ws_type', None)
        if whitespace is not None and expected is not whitespace:
            while True:
                if callback is not None and trial.index >= len(trial.lexer.tokens):
                    offset = trial.lexer._cursor.index
                    if offset >= len(trial.lexer.source) or not trial.lexer.source[offset].isspace():
                        break
                token = trial.peek()
                if not isinstance(token, RawToken) or not token.holds(whitespace):
                    break
                trial.advance()
                # An external callback owns the next source span; the host
                # lexer may be unable to tokenize its first character.
                if callback is not None and trial.index >= len(trial.lexer.tokens):
                    break
            pos.commit(trial)
        return trial

    def _match(self, pos: Cursor, expected: Any, callback: Callable | None) -> Node:
        if not ispa_debug.enabled():
            return self._match_impl(pos, expected, callback)
        name = getattr(expected, '__name__', str(expected))
        ispa_debug.event('lexer_enter', name)
        result = None
        try:
            result = self._match_impl(pos, expected, callback)
            return result
        finally:
            success = result is not None and not result.empty()
            ispa_debug.match(name, f'token index {pos.index}', success,
                             'matched' if success else 'expected token or rule was absent')

    def _match_impl(self, pos: Cursor, expected: Any, callback: Callable | None) -> Node:
        trial = self._skip_ws(pos, expected, callback)
        if callback is not None:
            result = callback(trial)
            if isinstance(result, MatchResult):
                if result.status:
                    pos.commit(result.it if result.it is not None else trial)
                    self._commit_probes(pos)
                    return result.node
                return Node()
            if result is None or result is False:
                return Node()
            pos.commit(trial)
            self._commit_probes(pos)
            return result
        token = trial.peek()
        if token is None:
            return Node()
        if isinstance(expected, type):
            matches = token.holds(expected) if isinstance(token, RawToken) else isinstance(token, expected)
        else:
            kind = token.name() if hasattr(token, 'name') else getattr(token, 'kind', None)
            matches = kind == expected
        if not matches:
            return Node()
        trial.advance()
        pos.commit(trial)
        self._commit_probes(pos)
        node = token.to_node(expected if isinstance(expected, type) else None) \
            if isinstance(token, RawToken) else token
        return _as_matched(node)

    def _commit_probes(self, pos: Cursor) -> None:
        for probe in self._probes:
            if pos.index != probe[0]:
                probe[1] = True

    @staticmethod
    def _split_fail(callback: Callable | None, fail: Callable | None):
        """`required(pos, Node, fail)`: a recovery handler passed in the callback slot."""
        if fail is None and getattr(callback, '_ispa_recovering', False):
            return None, callback
        return callback, fail

    def _record(self, message: str, rule: str = '', pos: Cursor | None = None) -> None:
        d = R.Diagnostic(rule=rule or (self.frames[-1].rule if self.frames else '<rule>'), message=message)
        if pos is not None and not pos.isEnd():
            try:
                token = pos.peek()
                d.unexpected, d.offset, d.length = R.token_id(token), int(token.startpos), int(token.length)
            except Exception:  # diagnostics must never mask the real error
                pass
        self.recovery_state.diagnostics.append(d)

    # Use only at committed required matches: unlike the legacy path they report an explicit
    # Missing/Boundary result when the input was repaired.
    def required_recovering(self, pos: Cursor, expected: Any, callback: Callable | None,
                            fail: Callable) -> R.RequiredResult:
        if self.unwinding():
            return R.RequiredResult()
        retries = 0
        custom = getattr(fail, 'custom_action', False)
        while True:
            if callback is None:
                node = self._match(pos, expected, None)
                if node is not None and not node.empty():
                    self.note_match()
                    return R.RequiredResult(R.RequiredStatus.Matched, node)
            else:
                # A recursive alternative may advance its argument before failing:
                # a failed probe must never mutate pos.
                attempt = self._skip_ws(pos, expected, callback)
                result = None
                if custom:
                    probe: list[int | bool] = [attempt.index, False]
                    self._probes.append(probe)
                    try:
                        result = callback(attempt)
                    except _NoMatch:
                        pass   # invoke the explicitly bound parent handler
                    finally:
                        self._probes.pop()
                else:
                    result = callback(attempt)
                if not isinstance(result, MatchResult):
                    result = MatchResult(False, Node(), attempt) if result is None or result is False \
                        else MatchResult(True, result, attempt)
                if result.status:
                    pos.commit(result.it if result.it is not None else attempt)
                    self._commit_probes(pos)
                    self.note_match()
                    return R.RequiredResult(R.RequiredStatus.Matched, result.node)
                if self.unwinding():
                    # The child abandoned itself for an outer frame: do not repair, propagate.
                    pos.commit(result.it if result.it is not None else attempt)
                    return R.RequiredResult(R.RequiredStatus.Boundary)
            self.reject_absent_rule()
            decision = fail(pos, self.recovery_state)
            mode = decision.mode
            if mode in (R.Mode.Retry, R.Mode.Delete):
                retries += 1
                # A rule cannot be synthesized from one token; tokens may be retried once/twice.
                if retries <= (1 if (mode == R.Mode.Retry and callback is None) else 2):
                    continue
            elif mode in (R.Mode.Insert, R.Mode.Replace) and callback is None:
                return R.RequiredResult(R.RequiredStatus.Missing, None, decision)
            elif mode == R.Mode.Abort and callback is None:
                return R.RequiredResult(R.RequiredStatus.Aborted, None, decision)
            # Local repair did not work: look for a recovery point in the enclosing frames.
            if mode == R.Mode.Propagate:
                self.recover_at_context(pos, R.ContextKind.Mismatch)
            status = (R.RequiredStatus.Aborted if mode == R.Mode.Abort
                      else R.RequiredStatus.Missing if mode in (R.Mode.Insert, R.Mode.Replace)
                      else R.RequiredStatus.Boundary)
            return R.RequiredResult(status, None, decision)

    def _recovered_value(self, pos: Cursor, result: R.RequiredResult, what: str) -> Node:
        if result.node is not None:
            return result.node
        if result.status != R.RequiredStatus.Aborted and (
                result.status == R.RequiredStatus.Missing or self.unwinding()
                or (not pos.isEnd() and result.recovery.mode != R.Mode.Propagate)):
            return Node()
        raise ParseError(what, pos.index)

    def required(self, pos: Cursor, expected: Any, callback: Callable | None = None,
                 fail: Callable | None = None) -> Node:
        callback, fail = self._split_fail(callback, fail)
        if getattr(fail, '_ispa_recovering', False):
            result = self.required_recovering(pos, expected, callback, fail)
            return self._recovered_value(pos, result, 'rule' if callback is not None else 'token')
        if getattr(callback, '_ispa_custom_failure', False):
            fail, callback = callback, None
        node = self._match(pos, expected, callback)
        if node is not None and not node.empty():
            return node
        if fail is not None and not getattr(fail, '_ispa_custom_failure', False):
            fail(pos)
        if self._probes and not self._probes[-1][1]:
            raise _NoMatch()
        err = self._error(expected, pos)
        self.errors.append(err)
        self._record(str(err), pos=pos)
        raise err

    def optional(self, pos: Cursor, expected: Any, callback: Callable | None = None) -> Node:
        if callback is None:
            return self._match(pos, expected, None)
        probe: list[int | bool] = [pos.index, False]
        self._probes.append(probe)
        try:
            node = self._match(pos, expected, callback)
            if node.empty() and probe[1]:
                raise RuntimeError('ISPA optional(): rule failed after consuming input')
            return node
        except _NoMatch:
            return Node()
        finally:
            self._probes.pop()

    def counted(self, pos: Cursor, spec: CountSpec | tuple[int, int | None] | int,
                expected: Any, callback: Callable | None = None, space=None) -> list[Node]:
        if isinstance(spec, int):
            spec = CountSpec.fixed(spec)
        elif isinstance(spec, tuple):
            spec = CountSpec('', spec[1], spec[0])
        # A named counter that was already captured demands exactly that many items;
        # otherwise `minimum`..`count` is an inclusive range (count=None means unbounded).
        recorded = self.counters.get(spec.name) if spec.name else None
        limit = recorded if recorded is not None else spec.count
        minimum = max(recorded, spec.minimum) if recorded is not None else spec.minimum
        cursor = pos.copy()
        matches: list[Node] = []
        while limit is None or len(matches) < limit:
            attempt = cursor.copy()
            if matches and space is not None:
                self.skip_spaces(attempt, space)
            before = attempt.index
            item = self.optional(attempt, expected, callback)
            if item.empty():
                break
            if limit is None and attempt.index == before:
                raise RuntimeError('Zero-length match in repetition')
            matches.append(item)
            cursor.commit(attempt)
        if len(matches) < minimum:
            self.reject_absent_rule()
            err = self._error(f'{minimum}..{limit} instances of {expected!r}', pos)
            self.errors.append(err)
            self._record(str(err), pos=pos)
            raise err
        if spec.name:
            self.counters.setdefault(spec.name, len(matches))
        pos.commit(cursor)
        self.note_match()
        return matches

    def _repeat(self, pos: Cursor, expected: Any, callback: Callable | None,
                items: list[Node]) -> None:
        while not pos.isEnd() and not self.unwinding():
            before = pos.index
            item = self.optional(pos, expected, callback)
            if item.empty():
                break
            if before == pos.index:
                raise RuntimeError('Zero-width match in range')
            items.append(item)
        self.sync_loop(pos)

    def range(self, pos: Cursor, expected: Any, callback: Callable | None = None,
              fail: Callable | None = None) -> list[Node]:
        callback, fail = self._split_fail(callback, fail)
        start = pos.index
        if getattr(fail, '_ispa_recovering', False):
            result = self.required_recovering(pos, expected, callback, fail)
            if result.node is None:
                return []   # repaired / unwound / unrecoverable: nothing consumed
            items = [result.node]
        else:
            items = [self.required(pos, expected, callback, fail)]
        if self.unwinding() or (callback is not None and pos.index == start):
            return items
        self._repeat(pos, expected, callback, items)
        return items

    def range_optional(self, pos: Cursor, expected: Any,
                       callback: Callable | None = None) -> list[Node]:
        items: list[Node] = []
        self._repeat(pos, expected, callback, items)
        return items

    def skip_spaces(self, pos: Cursor, expected: Any = None) -> None:
        """Use the whitespace token type or provide parser_ws_type on the class."""
        expected = expected if expected is not None else getattr(self, 'parser_ws_type', None)
        if expected is None:
            return
        while not self.optional(pos, expected).empty():
            pass

    def raise_error(self, message: str, pos: Cursor | None = None) -> None:
        err = self._error(message, pos)
        self.errors.append(err)
        self._record(message, pos=pos)
        raise err


def parse_token_stream(tokens: list, rule: Callable[[Cursor], MatchResult], lexer_type: type[Lexer]) -> Any:
    lexer = lexer_type()
    lexer.tokens = list(tokens)
    lexer._finished = True
    result = rule(lexer.lazy_iterator())
    return result.node if result.status and result.it.isEnd() else None
