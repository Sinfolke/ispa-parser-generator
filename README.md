# ISPA Parser Generator

ISPA turns an `.isc` grammar into a lexer and parser that build a **typed AST**. Everything is compiled ahead of time — lexer tables, parser logic, AST shape — into **C++17** or **Python** source you ship, so the generated parser never depends on ISPA at runtime.

**Usage demo**

https://github.com/user-attachments/assets/97d0214b-8807-493c-8a55-3b1fb4ea41aa

## Why ISPA

- **Typed ASTs, no boilerplate.** `@` marks what to capture, `{@}` (or `@{field, ...}`) shapes the result — the generator builds a typed tree during parsing, no visitor/listener layer required afterward.
- **Static generation.** Grammars compile to tables and target code once; nothing is interpreted at runtime.
- **C++ and Python from one grammar.** Python generation currently supports the LL backend.
- **Scales to real grammars.** Imports, external parsers, and incremental generation let you split and reuse grammars instead of maintaining one giant file.

## Coming from ANTLR?

The shape of a `.isc` file will feel familiar — rules, terminals, productions — but a few things work differently:

- **Captures are explicit.** `@` on a token or subrule marks what ends up in the AST; nothing is captured by default, so there's no separate visitor pass to extract data.
- **Output is ahead-of-time and dependency-free.** The generated parser is plain C++17 (or Python using `ispa_std`), not a runtime that interprets your grammar.
- **LL is the stable path.** LL parsing and the tagged-DFA lexer are the production target; LR/LALR exist as an experimental plugin with fewer features.

See [Performance](#performance) below for a measured comparison against ANTLR's C++ target.

Use `ispa-pkg tool antlr-to-ispa` to install the compiled ANTLR 4 migration tool, and
`ispa-pkg tool flex-to-ispa` and `ispa-pkg tool bison-to-ispa` for Flex and Bison grammars. The install scripts try all three and skip
them silently if no matching release asset exists.
It preserves left recursion and emits a report of required manual changes.
See [ANTLR conversion](docs/ANTLR_TO_ISPA.md) for usage and translation limits.

## Install

```sh
# Linux / macOS (Apple Silicon)
curl -fsSL https://raw.githubusercontent.com/Sinfolke/ispa-parser-generator/main/scripts/install.sh | sh

# Windows PowerShell
irm https://raw.githubusercontent.com/Sinfolke/ispa-parser-generator/main/scripts/install.ps1 | iex
```

Install the grammar standard library, then the runtime for each output language you use:

```sh
ispa-pkg install std
ispa-pkg install cpp-target    # for C++ output
ispa-pkg install cpp-core      # for C++ output with --stdlib core (independent of cpp-target)
ispa-pkg install python-target # for Python output
ispa-pkg --export-permanent
```

`std` contains `.isc` grammar modules and their Python helpers. It does not
install a generated parser runtime. `cpp-target` supplies the C++ headers and
library; `python-target` supplies the Python runtime modules. Install either
target package or both, according to the code you generate. If a C++ toolchain
is unavailable during installation, run `ispa-pkg --build cpp-target` after
setting it up.

`ispa --help` lists generation options; `ispa-pkg --help` covers package management. The [Getting Started](https://github.com/Sinfolke/ispa-parser-generator/wiki/Getting-Started) wiki page walks through the rest, including non-CMake builds.

## Quick example

```ispa
module main -> main;

NUMBER: @ [0-9]+ {@};
main: @ NUMBER {@};
```

```sh
ispa -l cpp -a LL grammar.isc -o build/Parser
```

```cpp
#include <Parser.h>

Parser::Parser parser;
auto tree = parser.parse("123");
```

Same grammar, Python target:

```sh
ispa -l python -a LL grammar.isc -o build/Parser
```

Full walkthroughs — CMake setup, linking, the Python runtime path — are in [Getting Started](https://github.com/Sinfolke/ispa-parser-generator/wiki/Getting-Started) and [Your First Grammar](https://github.com/Sinfolke/ispa-parser-generator/wiki/Your-First-Grammar).

# Dynamic pattern match

This is done in release v1.1.5 and allows you to match dynamic string from variable.
This can be done by capturing it before or even assigning in advanced rules.

A practical example:
```ispa
module main -> main;
STRING<QUOTE>:
    &q ${QUOTE} @ !($q | [\n\r])* $q
    
    {@}
;
QUOTE: '"' | '\'';
main: 
    @ STRING<QUOTE>
    {@}
;
```
- `&q` captures a variable with pattern matched
- `${QUOTE}` matches compile-time template,
- `@ !($q | [\n\r])*` match anything but quote, newline characters or `\0`.
- `$q` match closing quote 


## What's implemented

- **C++17:** LL is the recommended, most complete backend.
- **Python:** LL generation only, via the `ispa_std` runtime.
- **Rust:** experimental typed IR emission; full parser generation awaits [neutral IR and runtime contracts](lib/converter/rust/README.md).
- **Lexing:** token patterns, captures, nested token data, dynamic patterns, negative matching.
- **Recovery and actions:** advanced rules, embedded Python actions, fail-block recovery — all on LL.
- **External parsers:** a grammar can call another generated parser and keep its native node type (LL + lazy tokenization only).
- **Tooling:** grammar imports, incremental generation, automatic grammar diagnostics, and a parser debugger (`ispadbg`) with breakpoints and trace replay.

Grammar syntax, the CLI, generated-code layout, and the runtime API are documented in the wiki (linked below) rather than here.

## Performance

The performance measured on state state ispa has no error recovery.
I did not measure latest release speed, and performance is a work in progress.

Lexing and parsing the same 9.7 MB input, against flex + bison (LALR(1)) and ANTLR (C++ target). Lower is faster; relative is against flex + bison without an AST.

| Configuration | Notes | Time/iteration | Relative |
|---|---|---:|---:|
| flex + bison, no AST | recognition only | 119 ms | 1.0x |
| flex + bison, AST, arena allocator | 3.36M nodes, bump allocation | 151 ms | 1.3x |
| flex + bison, AST, malloc | 3.36M nodes, includes tree release | 259 ms | 2.2x |
| **ISPA**, allocations stripped | | 400 ms | 3.4x |
| **ISPA**, full AST | | 1528 ms | 12.8x |
| ANTLR (C++), CST construction disabled | | 18,573 ms | 156x |

With a full typed AST, ISPA was about 12x faster than ANTLR with CST construction disabled, and roughly 6–10x slower than flex + bison when flex + bison built a comparable AST. Measured on a Core i5-12450H, Clang 18 `-O3`, Linux Mint, ANTLR 4.13.2, flex 2.6.4, bison 3.8.2. flex + bison figures are steady-state over 10 runs; others are single runs.

## Limitations

- **Choose LL for applications.** LR-family modes are experimental, support fewer grammar features, and some LL actions/recovery have no LR equivalent.
- **Python supports LL only** — don't assume C++-backend options apply there.
- **External parser calls** require LL and lazy tokenization; an eager lexer is rejected if the grammar uses them.
- **C++ output requires C++17**; building ISPA itself has stricter requirements (Clang + libc++ on Linux, MSVC on Windows — GCC isn't supported yet).
- **Release artifacts:** Linux x86_64, Windows x64, macOS Apple Silicon. Check [Releases](https://github.com/Sinfolke/ispa-parser-generator/releases) for current platforms.
- **Editor support:** the [JetBrains plugin](https://plugins.jetbrains.com/) is the tested integration (highlighting, completion, diagnostics, fail-block support). VS Code, Neovim, and Emacs extensions exist but aren't tested by this project.

## Learn more

- [Wiki](https://github.com/Sinfolke/ispa-parser-generator/wiki) — grammar syntax, CLI reference, generated code and runtime API, error recovery, modules, package manager, debugging, building from source.
- [ispa-ai](docs/ISPA_AI.md) — interactive AI suggestions for LL diagnostics and recovery in `ai.json`.
- [Discussions](https://github.com/Sinfolke/ispa-parser-generator/discussions) and [Issues](https://github.com/Sinfolke/ispa-parser-generator/issues) for questions and bug reports.

## License

Apache-2.0

Flex and Bison converter usage and limitations: [migration guide](docs/FLEX_BISON_TO_ISPA.md).
