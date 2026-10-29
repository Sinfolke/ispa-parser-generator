The Rust emitter is experimental. It can emit and execute typed LangAPI functions,
value classes, owned strings, arrays, borrowing, and control flow. Full grammar to
Rust parser generation is blocked by the shared lowering and runtime contracts
listed below. The converter diagnoses this instead of emitting a parser that
does not compile. Rust runtime unit tests alone do not establish parser support.

All source languages must lower into language-neutral values and operations.
`LangAPI::String::value` contains decoded UTF-8 data; escaping belongs to each
emitter. `StdlibExports::TextOutput` and `RecoveryHandler` represent output streams
and recovery handler construction without C++ type or function names.
Borrowing, cloning, and receiver qualification happen in Rust emission state;
they are never inserted into LangAPI as Rust calls or receiver identifiers.
The structural rewriter only merges scopes and hoists declarations.

Missing or uncertain IR contracts:

- Token-stream operations: pending-token append, clear, EOF insertion, cursor
  creation, and reset use semantic operations. The Rust runtime defines the
  `TokenStream` interface; generated lexers still need to implement it, and
  cursor advancement needs a shared contract.
- Runtime interfaces: lexer callbacks and parser entry points currently depend
  on implicit inherited fields, methods, and virtual dispatch. The IR needs
  explicit receiver/interface bindings and callback signatures. Embedding the
  base as a field and implementing Deref does not implement virtual dispatch.
- Call resolution and types: overloads require resolved declaration identities;
  member calls on arbitrary receivers need receiver types. Inferred exported
  return types require type inference before emission. The emitter rejects
  unresolved overloads and inferred function return types.
- References and lifetimes: borrowed fields and returned borrows need ownership
  and lifetime relationships. Existing borrow/move type wrappers suffice for
  simple function parameters, but do not describe those relationships.
- Character semantics: legacy `Char::escaped` stores source syntax. Source
  lowering must decode this before Rust emission. The Rust cursor now classifies
  individual UTF-8 bytes, matching the DFA tables; text operations at positions
  inside a multibyte character still need an explicit contract.
- Foreign helpers: pseudo-symbols such as `__ispa_python_self`, Python-specific
  collection method enum values, and packed source strings such as
  `std::ostream&` are not neutral operations. They require source lowering into
  semantic instance access, collection operations, and types.
- Exceptions and recovery: the current Rust runtime uses Result while old
  generated throw paths use panics. A consistent propagation/catching contract
  must be implemented before parser recovery can be advertised.

Validation (from the repository root):

```sh
cmake --build cmake-build-release --target ispa tests -j 2
cmake-build-release/tests --gtest_filter='RustEmitter.*'
cargo test --manifest-path parser/stdlibs/target/rust/Cargo.toml --offline
```

The emitter execution test compiles and runs the generated Rust with `rustc`,
including incremental output, and checks that generation leaves its input IR
unchanged.
