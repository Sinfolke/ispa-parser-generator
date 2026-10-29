# Language-agnostic ispa_std backend

The C++ runtime (`ispastdlib`) is ported to the restricted Python subset in `ispa_std/`, so the
generator can lower it to any target. What cannot be language-agnostic is in `cpp_residue/`
(see its `MANIFEST.md`).

## Header -> module

| C++ header | Python module |
|---|---|
| lexer.hpp | lexer.py |
| dfa_api.hpp / dfa_run.hpp | dfa_api.py / dfa_run.py |
| parser_base.hpp / parser_ll.hpp | parser_base.py / parser_ll.py |
| ispa_recovery(.hpp, _context.hpp) | recovery.py |
| recovery_scan.hpp / external.hpp / counter.hpp | scan.py / external.py / counter.py |
| node.hpp / raw_token.hpp / assign.hpp / to_string.hpp | node.py / raw_token.py / assign.py / to_string.py |
| ast_printer.hpp / ispa_debug.hpp | ast_printer.py / debug.py |
| lr_machine.hpp / lr_conflicts.hpp | lr.py / lr_glr.py |
| core.hpp errors | failures.py, errors.py |
| helpers (new) | format.py, chars.py, refs.py, sortedmap.py, reflect.py |

## StdlibExports -> implementation

Ported to Python: Node, MatchResult, MatchResultValue, Lexer, LexerMakeTokenParameter, LexerTokenIterator,
LexerLazyIterator, TokenCursor, LazyTokenCursor, ResetTokenStream, Parser, ParserRequired, ParserOptional,
ParserRange, ParserRangeOptional, ParserCounted, ParserFunctionParameter, ParserNodeConstructor, LRParser,
LRGeneralizedParser, LRParse, LRParseGeneralized, LRGet, LRNode, LRPack, LRCaptureAssign, LRToken, DfaState,
DfaTable, DfaClassTable, DfaAcceptTable, DfaLRTable, DfaNullState, DfaDynamicFailState, DfaCaptures,
DfaDynamicMatch, DFADebug, TdfaLayout, ASTPrinter, TextOutput, Concat, Recovery* (Decision, State,
EndOfInput, TokenSet, Policy, Handler), RecoverEncoded, Error, CloneSelfBox, LookaheadKind.

Partly C++ (typed form): TokenNodeConstruct, RawTokenNodeConstruct (`type_id` ints in Python),
VariantIndex, AssignCompatible (`Shape` / `reflect.py` in Python).

Note: the supplied `LangAPI.StdlibExports` has no `StdFail`, so it is older than `prim.py`.

## Contract differences from C++

- Cursors are mutable objects: use `copy()` / `assign()` where C++ copied/assigned iterators.
- Overloads are split by name (`required_token` / `required_rule`, `optional_*`, `range_*`, `counted_*`).
- Source is a String of 8-bit units, offsets are Int; `""` plays NUL. Decode input as latin-1 on hosts with wider strings.
- Sentinels (`NULL_STATE`, `NPOS`, `ABSENT`) are plain ints.
- RAII is replaced by explicit `close()` / `restore()` (`FrameGuard`, `MatchScope`, `CounterScope`, `RuleProbe`).
  Debug scopes unwound by an error close via `debug.unwind_to`.
- `push()` of the end token is `push_eof()`.
- Lexer/parser init: `Lexer_base.init()` and `set_token_names` / `set_space_token` replace constructor-time wiring.
- `try/catch` becomes `prim.attempt(fn, report)`.

## Action required from you: `prim.attempt`

Only `prim.fail` crosses the compiler boundary today. C++ catch sites (NoMatch probing, external parser
errors, recovery) need `attempt(fn, report) -> bool` (`report = [kind, message]`). Add it to
`StdlibExports`, regenerate `prim.py`, and map the new `ErrorKind` values 6-11 (LogicError, RuntimeFailure,
InternalError, ApiError, NoMatch, ExternalParse). `prim_cpython.py` and `cpp_residue/ispastdlib/std_prims.hpp` are reference implementations.

Subset extension used: class inheritance (Parser_base -> LLParser, LRParser -> LRGeneralizedParser, FailHandler -> Handler).
`reflect.py` is host-only and not lowered.

## Optional IR upgrades

`Cast` for `code_of`/`char_of`, `CheckVariant`/`GetVariant` for `RawToken.holds/get`, `ToString` for `to_string`.

## Known limitations

- `ISPA_DEBUG_REGISTERS` macro tracing in dfa_run is dropped.
- Embedded NUL: `""` stands for NUL past the end, so an in-source NUL is distinguishable only by `code_of`.
- File I/O and debug transport are host adapters (cpp_residue).
- Debug scopes abandoned by an error are closed only through `unwind_to`.

## Verification

- `tests/test_diff_cpp.py`: random-input comparison against the ORIGINAL headers for `scan_balanced`,
  `recover`, and `dfa_run` (set `ISPASTDLIB_ORIG` to the dir containing `ispastdlib/`).
- `tests/test_lr.py`, `test_ll.py`, `test_lexer.py`, `test_util.py`: unit/integration tests.
- `python3 tools/check_subset.py ispa_std`: subset linter.
