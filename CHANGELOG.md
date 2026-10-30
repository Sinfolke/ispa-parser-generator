# Changelog

## Unreleased

### Native bindings

- **Native declarations in plain Python** — In a Python file brought in with `import py`, a function or method whose whole body is `raise NotImplementedError` (after an optional docstring) is a declaration only. The C++ target emits a prototype and expects you to define it; the Python target emits a stub that raises until the host assigns an implementation; the Rust target rejects it. See `docs/native-bindings.md`.
- **`depends` removed** — The `depends` keyword and `forward_declaration` grammar were never consumed by the generator. `import py` is the only link between grammar Python code and declarations. Regenerate the bootstrapped parser (`parser/ispa/bootstrapped`) after pulling.

### Lexer

- **Dynamic pattern matching** — Lexer tokens can now use dynamic patterns whose structure depends on previously captured values or parser state.
- **Negative matching** — Lexer tokens support negative matching to reject unwanted alternatives during tokenization.
- **Variable capture** — Lexer rules can bind captured values to variables for use in subsequent rules and actions.

### Parser & Error Recovery

- **Fail blocks** — Required LL parser matches can declare named fail blocks with `ctx.error`, `ctx.skip`, `ctx.insert`, `ctx.replace`, `ctx.retry`, and `ctx.abort` for in-grammar error recovery.
- **Advanced rules** — LL C++ parser rules support `match` statements, embedded Python-like action statements, `ctx.pos` checkpointing (mark/restore/commit), `ctx.collect_braced`, and `ctx.sync`/`ctx.sync_tokens` for source and token-stream synchronization.
- **Dynamic regex** — Back-references and counts from earlier captures are now lowered to output for both lexer and parser.
- **Bootstrapped parsers** — ISPA's own grammar and Python-subset parser are fully bootstrapped; generated parsers replace hand-written ones.

### Tooling

- **LSP support** — Language server (`cli/pkg/ispa_pkg/lsp.py`) provides `.isc` diagnostics and editor features.
- **Editor plugins** — VS Code, Neovim, and Emacs extensions updated with syntax highlighting and language features for `.isc` files.
- **JetBrains plugin** — Moved to JetBrains Marketplace. Provides semantic highlighting, completion, go-to-declaration, native parser diagnostics, and background `ispa --check-only` annotation. GitHub Actions release workflow builds and packages native helpers for Linux x64, Windows x64, macOS x64, and macOS ARM64.

### Build & CI

- Fixed JetBrains plugin release gradlew compilation.
- Fixed JetBrains plugin Windows compilation.
- Updated bootstrapped parser sources (`bootstrapped0` and `bootstrapped_old`).

### Documentation

- Moved the `concept/` documents into the [wiki](https://github.com/Sinfolke/ispa-parser-generator/wiki), as requested; feature and design documentation now lives there.
- Updated `README.md` current-state table: LL parser, C++17 target, dynamic regex, lexer dynamic patterns, variable capture, advanced rules, and fail blocks are marked **Done**.
