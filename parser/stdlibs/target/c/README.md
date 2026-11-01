# ISPA C target (work in progress, branch `c/target`)

Native C lowering of the ISPA runtime and generated parsers: `ispa -l c`.

* `ispa_c.h` / `ispa_c.c` - C11 runtime (arenas, byte strings, vectors). `cc -std=c11 ispa_c.c tests/runtime.c` runs its test.
* `lib/converter/c` - the emitter (module `C.Init`, loaded as `libispa-converter-c`).

## Conventions
* Namespaces are flattened into prefixes: `python::ispa_std::lexer::TokenCursor` -> `python_ispa_std_lexer_TokenCursor`.
* A class's first member is its base struct, so a derived pointer is a valid base pointer.
* Methods are static calls (`<Class>_<method>(self, ...)`) unless a class redefines a method that an ancestor defines; see Dispatch.
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

## Dispatch
Python marks every method virtual; the C target deliberately does not. Direct calls are faster (inlinable, no load) and the
generated API stays plain C functions. The rule is structural:

* A method is **virtual iff some class in the lowered tree redefines a method an ancestor defines** (`__init__` and static
  methods never are). The topmost definer owns the vtable slot.
* Only such methods get a `<Root>_vt` slot, a dispatcher `<Root>_<method>_v(self, ...)`, and a per-class vtable instance that
  the class's `get_<X>_handler` installs. Every other call is `<Owner>_<method>(self, ...)`.
* Call through the dispatcher (`..._v`) whenever you hold a base pointer to something that may be a subclass.
* `Class.method(self, ...)` (explicit class, e.g. base `__init__`) is always a static call, like `super()`.

Limits: the analysis sees the declarations being lowered. A method that only native code or a *later* compilation unit
overrides is not virtual (no slot exists yet); to extend a library class that way it has to be redefined by some class in the
same lowering. Overrides inside monomorphized generic instances are not analysed. One vtable chain per class (no multiple
inheritance of virtual groups).

Test: `cc -std=c11 -I. -Ilowered tests/dispatch.c lowered/ispa_std.c ispa_c.c && ./a.out`.
