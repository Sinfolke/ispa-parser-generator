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

ISPA builds a typed AST while parsing and keeps error-recovery bookkeeping, so it trades some raw speed for that. All numbers below were measured on a Core i5-12450H (plugged in, Linux, Clang `-O3`, Rust `--release` with `codegen-units=1`), as the median of several runs. A browser was open during the runs, so expect a few percent of noise. Lower is faster.

### Python grammar (real-world input)

The input is a corpus of top-level statements taken from the Python standard library, normalized with `ast.unparse` and filtered to the statements the bundled ISPA Python grammar accepts (4068 of 5402). Times are parse time in ms; lexing is reported separately.

| Target | Algorithm | 8 KB | 32 KB | 262 KB |
|---|---|---:|---:|---:|
| C++ | LL | 63.7 | 243.6 | 1751 |
| C++ | LALR (generalized) | 796.5 | 7788 | n/a |
| Rust | LL | 79.8 | 296.2 | 2129 |
| Rust | LALR (generalized) | 395.5 | 3316 | n/a |
| Python | LL | 424.6 | 1642 | 12483 |
| Python | LALR (generalized) | 855.7 | n/a | n/a |
| ANTLR 4 (C++), no tree | | 9.9 | 28.4 | 74.7 |
| ANTLR 4 (C++), parse tree | | 10.0 | 29.1 | 83.6 |

Lexing time at 8 KB / 32 KB / 262 KB: C++ 0.8 / 5.5 / 294 ms, Rust 2.4 / 5.5 / 44 ms, Python 85 / 324 / 3231 ms, ANTLR 98 / 363 / 2799 ms.

Caveats:
- ANTLR parses faster than ISPA here, and its lexer is slower. ISPA's figures include building the typed AST and recovery bookkeeping; the ANTLR figures use the full Python 3 grammar from grammars-v4, ISPA's is a subset.
- The Python grammar has LALR conflicts, so LALR uses the generalized parser (runtime branching). It hits its resource limit above ~32 KB ("n/a"). Use LL for this grammar.
- Rust and Python targets are not optimized yet.
- **Known bottleneck: exception-based backtracking.** A `perf` profile of the C++ LL parser on the 262 KB input shows about 60% of the time spent in C++ exception unwinding (`ISPA_STD::fail` throws on every failed alternative; the unwinder and personality routines dominate). The Python grammar fails alternatives constantly because statements and expressions share long prefixes, while JSON rarely does, which is why ISPA is close to ANTLR on JSON but far behind on Python. Returning failure by status instead of throwing, and rejecting alternatives by first-token lookahead, are planned.
- No bison grammar for Python exists that we could use, so bison is compared on JSON below.

Reproduce: `python3 tools/core-parity/benchmark_python.py --targets cpp rust python --algorithms LL LALR`.

### JSON, 10 MB (bison comparison)

| Configuration | Lex | Parse |
|---|---:|---:|
| flex + bison, recognition only | | 40 ms |
| ANTLR 4 (C++), no tree | | 972 ms |
| ANTLR 4 (C++), parse tree | | 1209 ms |
| **ISPA** C++ LL | 768 ms | 1019 ms |
| **ISPA** C++ LALR | 756 ms | 1984 ms |
| **ISPA** Rust LL | 2064 ms | 5892 ms |
| **ISPA** Rust LALR | 2084 ms | 3622 ms |

flex + bison is far faster, about 25x the C++ LL parse, but it only recognizes the input and builds no AST. ISPA's C++ LL parse builds a full typed AST. On this grammar LR is about 2x slower than LL in C++ and faster than LL in Rust. Sources: [`parser/ispa/bench`](parser/ispa/bench) and `tools/core-parity/benchmark_json.py`.

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
