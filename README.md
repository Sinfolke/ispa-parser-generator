# ISPA Parser Generator

ISPA turns an `.isc` grammar into a lexer and parser that build a **typed AST**. Everything is generated ahead of time — lexer tables, parser logic, AST shape — as **C++17**, **Python** or **Rust** source you ship. The generated parser never depends on ISPA at runtime.

**Usage demo**

https://github.com/user-attachments/assets/97d0214b-8807-493c-8a55-3b1fb4ea41aa

## Why ISPA

- **Typed ASTs, no boilerplate.** `@` marks what to capture and `{@}` (or `@{field, ...}`) shapes the result. The tree is built while parsing, so you don't need a visitor or listener pass.
- **Static generation.** Grammars compile to tables and target code once. Nothing is interpreted at runtime.
- **One grammar, several languages.** C++17, Python and Rust targets share the same `.isc` source.
- **LL and LR.** LL is the full-featured default. LR0/LALR (deterministic and generalized) is available when you want it.
- **Built for real grammars.** Imports, external parsers and incremental generation let you split and reuse grammars.
- **Good tooling.** Language server, JetBrains plugin, a parser debugger (`ispadbg`) and a browser playground.

## Install

```sh
# Linux / macOS (Apple Silicon)
curl -fsSL https://raw.githubusercontent.com/Sinfolke/ispa-parser-generator/main/scripts/install.sh | sh

# Windows PowerShell
irm https://raw.githubusercontent.com/Sinfolke/ispa-parser-generator/main/scripts/install.ps1 | iex
```

Then install the standard library and the runtime for each language you generate:

```sh
ispa-pkg install std
ispa-pkg install target-cpp     # C++ output
ispa-pkg install python-target  # Python output
ispa-pkg install rust-target    # Rust output
ispa-pkg --export-permanent
```

- `std` holds the `.isc` grammar modules and their Python helpers. See [standard library](docs/standard-library.md).
- `target-cpp` builds `libispa-core.a` and the C++ headers. Run `ispa-pkg --build target-cpp` to rebuild it, and `eval "$(ispa --export)"` to export include and library paths into your shell.
- `python-target` and `rust-target` provide the runtimes for those languages.

`ispa --help` lists generation options. `ispa-pkg --help` covers package management.

## Quick start

Save as `grammar.isc`:

```ispa
module main -> main;

NUMBER: @ [0-9]+ {@};
main: @ NUMBER {@};
```

Generate a parser:

```sh
ispa -l cpp    -a LL grammar.isc -o build/Parser   # C++
ispa -l python -a LL grammar.isc -o build/Parser   # Python
ispa -l rust   -a LL grammar.isc -o build/Parser   # Rust
```

Use it from C++:

```cpp
#include <Parser.h>

Parser::Parser parser;
auto tree = parser.parse("123");
```

Switch `-a LL` to `-a LALR` (or `LR(0)`) to generate an LR parser from the same grammar.

CMake setup, linking and the Python and Rust paths are covered in [Getting Started](https://github.com/Sinfolke/ispa-parser-generator/wiki/Getting-Started) and [Your First Grammar](https://github.com/Sinfolke/ispa-parser-generator/wiki/Your-First-Grammar).

## Features

| Area | What you get |
|---|---|
| **Targets** | C++17, Python (`ispa_std` runtime), Rust |
| **Algorithms** | LL (most complete), LR0/LALR with generalized parsing |
| **Lexing** | Tagged-DFA lexer, captures, nested token data, [dynamic patterns](#dynamic-patterns), negative matching |
| **Error recovery** | Fail blocks (`ctx.error`, `skip`, `insert`, `replace`, `retry`, `abort`) and synchronization |
| **Actions** | Embedded Python-like actions and `match` statements in advanced rules |
| **Native bindings** | Declare functions in plain Python and implement them in the host language ([guide](docs/native-bindings.md)) |
| **External parsers** | A grammar can call another generated parser and keep its node type |
| **Modules** | Grammar imports and incremental generation |
| **Debugging** | `ispadbg` with breakpoints and trace replay |
| **Browser** | WASM build and a playground |

### Dynamic patterns

Match a string that depends on something captured earlier, such as a closing quote that must equal the opening one:

```ispa
module main -> main;

QUOTE: '"' | '\'';

STRING<QUOTE>:
    &q ${QUOTE} @ !($q | [\n\r])* $q
    {@}
;

main: @ STRING<QUOTE> {@};
```

- `&q` captures the matched text into a variable.
- `${QUOTE}` matches a compile-time template argument.
- `!($q | [\n\r])*` matches anything except the captured quote or a line break.
- `$q` matches the closing quote.

## Coming from other tools

**ANTLR, Flex or Bison?** Converters migrate existing grammars and report what needs manual work:

```sh
ispa-pkg tool antlr-to-ispa
ispa-pkg tool flex-to-ispa
ispa-pkg tool bison-to-ispa
```

See [ANTLR conversion](docs/ANTLR_TO_ISPA.md) and [Flex/Bison conversion](docs/FLEX_BISON_TO_ISPA.md).

Key differences from ANTLR:

- **Captures are explicit.** Only tokens and subrules marked with `@` end up in the AST.
- **Output is ahead of time.** The generated parser is plain source, not a runtime that interprets your grammar.
- **Left recursion is preserved** by the converter.

## Performance

ISPA trades some raw speed for a typed AST built during parsing. The figures below come from one earlier measurement on a 9.7 MB input, comparing against flex + bison (LALR(1)) and ANTLR's C++ target. They were taken before error recovery was added and have not been re-run on the latest release, so treat them as indicative. Lower is faster; "relative" is against flex + bison without an AST.

| Configuration | Notes | Time/iteration | Relative |
|---|---|---:|---:|
| flex + bison, no AST | recognition only | 119 ms | 1.0x |
| flex + bison, AST, arena allocator | 3.36M nodes, bump allocation | 151 ms | 1.3x |
| flex + bison, AST, malloc | 3.36M nodes, includes tree release | 259 ms | 2.2x |
| **ISPA**, allocations stripped | | 400 ms | 3.4x |
| **ISPA**, full AST | | 1528 ms | 12.8x |
| ANTLR (C++), CST construction disabled | | 18,573 ms | 156x |

With a full typed AST, ISPA was about 12x faster than ANTLR (CST construction disabled) and roughly 6–10x slower than flex + bison building a comparable AST. Setup: Core i5-12450H, Clang 18 `-O3`, Linux Mint, ANTLR 4.13.2, flex 2.6.4, bison 3.8.2. flex + bison numbers are steady-state over 10 runs; the others are single runs.

Benchmark sources live in [`parser/ispa/bench`](parser/ispa/bench).

## Limitations

- **Prefer LL for applications.** LR modes support fewer grammar features, and some LL actions and recovery have no LR equivalent.
- **External parser calls** need LL and lazy tokenization. An eager lexer is rejected if the grammar uses them.
- **Rust and Python** do not yet match every C++ backend option.
- **Generated C++** requires C++17. Building ISPA itself needs Clang + libc++ on Linux or MSVC on Windows (GCC is not supported yet).
- **Release artifacts** cover Linux x86_64, Windows x64 and macOS Apple Silicon. See [Releases](https://github.com/Sinfolke/ispa-parser-generator/releases).
- **Editors:** the [JetBrains plugin](https://plugins.jetbrains.com/) is the tested integration (highlighting, completion, diagnostics, fail-block support). VS Code, Neovim and Emacs extensions exist but are not tested by this project.

## Learn more

- [Wiki](https://github.com/Sinfolke/ispa-parser-generator/wiki): grammar syntax, CLI reference, generated code and runtime API, error recovery, modules, package manager, debugging, building from source.
- [Changelog](CHANGELOG.md)
- [Library versioning](docs/LIBRARY_VERSIONING.md) and `ispa-pkg doctor`
- [Discussions](https://github.com/Sinfolke/ispa-parser-generator/discussions) and [Issues](https://github.com/Sinfolke/ispa-parser-generator/issues) for questions and bug reports.

## License

Apache-2.0
