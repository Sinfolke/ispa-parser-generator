# ISPA C target (work in progress, branch `c/target`)

Native C lowering of the ISPA runtime and generated parsers: `ispa -l c`.

* `ispa_c.h` / `ispa_c.c` - C11 runtime (arenas, byte strings, vectors). `cc -std=c11 ispa_c.c tests/runtime.c` runs its test.
* `lib/converter/c` - the emitter (module `C.Init`, loaded as `libispa-converter-c`).

## Conventions
* Namespaces are flattened into prefixes: `python::ispa_std::lexer::TokenCursor` -> `python_ispa_std_lexer_TokenCursor`.
* A class's first member is its base struct, so a derived pointer is a valid base pointer.
* Virtual methods are function-pointer slots (`vt`) in the introducing class; overriding classes install thunks.
* Per class X: `<ns>_get_X_handler(ispa_arena*, ...)` constructs, `<ns>_free_X_handler(X*)` releases.
* Errors are values (`ctx->err` + early return), not exceptions.

## Memory
Two arenas per parser handle. RESULT: AST, tokens, diagnostics (detachable, outlives the parser). SCRATCH: contexts,
probes, recovery, failed alternatives (reset per parse). A result object may only point into the result arena.
Mark/rewind of scratch on backtracking is a later optimisation, only for allocations that provably do not escape.

## Status
Declaration layer only: structs, vtable structs, handlers, prototypes. Function bodies are `ISPA_UNIMPLEMENTED`
placeholders and counted ("function bodies pending"). Next: statements/expressions, monomorphization, vtable
registration, then the typed LL facade and a `c_parity.py` against C++ (see the Rust target for the model).
