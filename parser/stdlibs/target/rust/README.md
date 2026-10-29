# ispa_std for the Rust target

The runtime crate generated Rust parsers (`ispa -l rust`) are written against. It is the Python runtime
(`parser/stdlibs/python/ispa_std`, package `target-core`) **lowered to Rust by ispa itself**, plus a small typed layer:

| file | what it is |
|---|---|
| `src/lowered.rs` | the lowered runtime, `python::ispa_std::*` (generated: `python3 regen.py --ispa PATH`). Combinators, error recovery, frames, DFA interpreter, contexts. |
| `src/shared.rs` | `Shared<T>`: object identity for reference-semantic classes (an `Rc` plus a pointer to the part of the allocation the handle means, so a handle to a subclass converts to its base by pointer arithmetic). Borrows are unchecked: Python objects alias freely. |
| `src/pycoll.rs` | Python list/dict/`in`/index semantics, `PyAny`, casts, `AssignFrom`-style helpers used by lowered code. |
| `src/facade.rs` | typed `Node`, `RawToken`, `MatchResult`, `LexerBase` (dispatch slots for the generated lexer), token iterators (eager and lazy), `TokenStream`, `skip_spaces`. |
| `src/bridge.rs` | typed cursors/tokens/metadata that are *subclasses of lowered classes* (fn-pointer slots, see below): how a typed iterator is a core `TokenCursor`, a typed token a core `RawToken`, and a typed node travels through the core inside a core `Node`. |
| `src/parser.rs` | `LLParser<Main, Token>` (the base of generated parsers) and `LLParserApi`/`LLParserSpaced`: `required`/`optional`/`range`/`counted`... in token and rule (`_rule`) form, `recovery::handler`, `recovery_at`/`recover_choice`, fail blocks (`FailContext`, `custom_failure`), counters, entry points `parse_tokens`/`parse_text`, `diagnostics`. |
| `src/lexer.rs` | running a generated lexer's DFA tables through the lowered interpreter (`lookup`), `set_input`/`make_tokens`/`begin_lazy`. |
| `src/printer.rs` | `ASTPrinter`, `PrintValue`, `ToText`/`concat` (port of ispastdlib's ast_printer/to_string). |

## Virtual dispatch

Python methods are virtual; Rust structs are not. The backend (`lib/converter/rust`) gives every class that is
overridden (found structurally from the first-base chain) a function-pointer slot `__vt_<m>` in the class that
introduces the method, a `__base_<m>` body, and a dispatcher `m`; an overriding class has its body as `__impl_<m>`,
a dispatcher, and a `__thunk_<m>` it registers in its `Default`. `base_0` is always the first field and structs are
`repr(C)`, so a pointer to the base is a pointer to the object. A few lowered classes (`TokenCursor`, `RawToken`,
`RawTokenMeta`, `TokenNames`) get slots for *all* their methods because hand-written code subclasses them
(`tests/subclass.rs`, `src/bridge.rs`).

## Tests

```
cargo test                      # unit tests + tests/lowered.rs (the lowered runtime executing), subclass.rs, bridge.rs, parser_api.rs
python3 tools/core-parity/rust_parity.py --grammar json|dyn|sql|pyexpr|counted|failblock [--fuzz N --seed S]
```

`rust_parity.py` generates the parser for a grammar with `-l rust` and with `-l cpp`, and requires the same tree, the
same failure text and the same diagnostics for a corpus plus random token soup, with tokens made up front and on
demand. Tests are single-threaded (`.cargo/config.toml`): namespace variables are `Lazy` statics.

## Not yet

LR parsers (`-a LALR|SLR|...`), `--static`, grammars whose actions embed C++ (examples/yaml), and the
(`ISPA_RUST_EXPERIMENTAL=1` lifts the converter's refusal of LR parsers, for development.)
