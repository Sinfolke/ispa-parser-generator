# ISPA Parser Generator

ISPA turns an `.isc` grammar into a lexer and parser that build a typed AST. It generates C++17 or Python code ahead of time; the generated parser does not need the ISPA compiler at runtime.

**Usage demo**

https://github.com/user-attachments/assets/97d0214b-8807-493c-8a55-3b1fb4ea41aa

## Why ISPA

- **Static generation:** grammars are compiled into lexer tables and target code ahead of time.
- **Small target runtime:** C++ uses the installed ISPA runtime; Python uses the `ispa_std` package.
- **C++ and Python output:** generate parsers in either language from the same grammar. Python currently supports LL generation.
- **Typed ASTs:** generated parsers build typed syntax trees directly during parsing.
- **Reusable grammars:** imports, external parsers, and incremental generation help organize larger grammars.
- **Performance:** see the benchmark comparison below for the measured workloads and limits.

## Install

Install the latest published release for your platform using the command for your OS. The installers are for the current user and do not need administrator access.

**Linux and macOS** (Linux x86_64 and macOS Apple Silicon releases are published):

```sh
curl -fsSL https://raw.githubusercontent.com/Sinfolke/ispa-parser-generator/main/scripts/install.sh | sh
```

**Windows PowerShell**:

```powershell
irm https://raw.githubusercontent.com/Sinfolke/ispa-parser-generator/main/scripts/install.ps1 | iex
```

Open a new terminal if needed, then check that `ispa` is available:

```sh
ispa --help
```

The release also provides `ispa-pkg` for target libraries and `ispadbg` for parser debugging. To update ISPA, run `ispa-update`.

## Install the standard library

Install the `std` package once, then enable its environment exports:

```sh
ispa-pkg install std
ispa-pkg --export-permanent
```

Open a new bash, zsh, or PowerShell session. Package exports set the C++ header and library paths and Python module path. In a shell that was already open, refresh them with `ispa-pkg --export`. The package provides target runtime files such as the C++ headers and the Python `ispa_std` module. Installing `std` builds its C++ runtime archive, so a C++ compiler and archiver must be available.

`ispa-pkg` also supports `list`, `update`, and installing libraries from GitHub or an archive. Run `ispa-pkg --help` for options. The default package root is under the current user's data directory; `ISPA_PKG_ROOT` can select another location.

## Generate and run a parser

Create `grammar.isc`:

```isc
module main -> main;

NUMBER: @ [0-9]+ {@};
main: @ NUMBER {@};
```

The `main` rule is the entry rule. Generate C++ output:

```sh
ispa -l cpp -a LL grammar.isc -o build/Parser
```

This produces `build/Parser.h` and `build/Parser.cpp`. A minimal `main.cpp` can parse a string and print the resulting AST:

```cpp
#include <iostream>
#include <Parser.h>

int main() {
    Parser::Parser parser;
    auto tree = parser.parse("123");
    std::cout << tree << '\n';
    for (const auto& error : parser.getErrors()) {
        std::cerr << error.message << '\n';
    }
}
```

Compile with C++17 and link the installed runtime. With CMake, create `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20)
project(demo LANGUAGES CXX)
include("$ENV{ISPA_TARGETS_CMAKE}")
add_executable(demo main.cpp build/Parser.cpp)
target_link_libraries(demo PRIVATE ISPA::Targets)
```

Then build and run:

```sh
cmake -S . -B build-cmake
cmake --build build-cmake
./build-cmake/demo
```

For a non-CMake build, compile as C++17, add the installed `ispatarget/cpp` include directory, and link the installed ISPA target runtime libraries. `ispa-pkg --export` prints the paths and library names; compiler and linker flags vary by toolchain.

### Generate Python

Generate Python output with the LL algorithm:

```sh
ispa -l python -a LL grammar.isc -o build/Parser
```

The generated module uses `ispa_std` from the installed `std` package. With package exports loaded, run it with:

```sh
python3 -c 'import sys; sys.path.insert(0, "build"); import Parser; print(Parser.Parser().parse("123"))'
```

If Python does not inherit `PYTHONPATH`, set it to include both the generated module directory and the installed `ispatarget/python` directory. `ispa-pkg --export` configures this path for the active shell.

Use `ispa --help` to see all current generation options, including algorithm, output directory, grammar search directories, dependency bindings, and debug instrumentation settings.

## What the generator supports

- **C++17:** LL is the recommended and most complete parser backend.
- **Python:** LL generation is implemented. Generated code imports the `ispa_std` target runtime.
- **Lexing:** token patterns, captures, nested token data, dynamic patterns, and negative matching.
- **AST:** typed AST values are built during parsing and can be traversed in the generated target language.
- **Grammar tools:** grammar imports, incremental generation, and automatic grammar diagnostics.
- **Recovery and actions:** LL supports advanced rules, embedded Python actions, and fail-block recovery strategies.
- **External parsers:** a grammar can call another generated parser and keep its node type. External calls require LL and lazy tokenization. See the short overview below.

## Performance

Lexing and parsing the same 9.7 MB input, compared with flex + bison (LALR(1)) and ANTLR (C++ target). Lower time is faster; the relative column uses flex + bison without an AST as its baseline.

| Configuration | Notes | Time per iteration | Relative |
|---|---|---:|---:|
| flex + bison, no AST | Recognizes input only | 119 ms | 1.0x |
| flex + bison, AST, arena allocator | 3.36 M nodes, bump allocation | 151 ms | 1.3x |
| flex + bison, AST, malloc | 3.36 M nodes; includes tree release | 259 ms | 2.2x |
| **ISPA**, allocations stripped | | 400 ms | 3.4x |
| **ISPA**, full AST | | 1528 ms | 12.8x |
| ANTLR (C++), CST construction disabled | | 18,573 ms | 156x |

In this benchmark, ISPA with a full AST was about 12x faster than ANTLR with CST construction disabled. ISPA was slower than flex + bison, roughly 6–10x when flex + bison built a comparable AST. Results are workload and machine specific; the measurements used a Core i5-12450H laptop, Clang 18 with `-O3`, Linux Mint, ANTLR 4.13.2, flex 2.6.4, and bison 3.8.2. Flex + bison numbers are steady state over 10 iterations; other figures were single runs.

## Limitations

- **Choose LL for applications.** LR-family modes are experimental and support fewer grammar features. Some LL actions and recovery features do not have LR equivalents.
- **Python supports LL only.** Do not assume C++ backend options or features work in generated Python parsers.
- **External parser calls have constraints.** They require LL and lazy tokenization; an eager lexer is rejected when the grammar contains external calls. Each dependency must be generated or supplied, and C++ dependency code must be compiled and linked.
- **Generated code needs a target runtime.** C++ projects need the installed headers and libraries; Python projects need `ispa_std` importable. Installing the compiler alone does not install a project's build dependencies.
- **C++ output requires C++17.** ISPA itself has stricter build requirements described under [Build from source](#build-from-source).
- **Platform availability varies.** Release archives are provided for Linux x86_64, Windows x64, and macOS Apple Silicon. Check the releases for current artifacts; macOS Intel is supported only when an archive is published.
- **Editor extensions vary in maturity.** The JetBrains plugin is the supported, tested editor integration. VS Code, Neovim, and Emacs extensions are not tested at all.

## Editor support

The JetBrains plugin is available on the JetBrains Marketplace and provides `.isc` syntax highlighting, completion, navigation, diagnostics, and fail-block support. Extensions for VS Code, Neovim, and Emacs also exist, but extensions other than the JetBrains plugin have not been tested by this project.

## External parsers

External imports let one grammar reuse a separately generated parser and return its native AST node. ISPA can bind a dependency that is already installed or generate discovered grammar dependencies alongside the host parser. This is useful for splitting grammars into reusable pieces. External calls currently require LL generation and lazy tokenization; C++ consumers must compile and link each dependency. See `ispa --help` for `--dep` and `--no-dep` options.

## Debugging generated parsers

Generated parsers include debug instrumentation by default. `ispadbg` runs a parser with breakpoints, lets you step through matching and AST construction, and can record a JSON trace for replay after the parser exits. For example:

```sh
ispadbg --purpose matching --purpose ast -b grammar.isc:4 \
  --record parser-trace.json -- ./demo 123
ispadbg --replay parser-trace.json
```

Purpose filters include `matching`, `failure`, and `ast`; `failure` is the default. The interactive debugger can step, continue, inspect values, and navigate recorded snapshots. Python parser traces use the same protocol; make sure the installed target runtime and generated parser are on `PYTHONPATH`. Live debugger sessions require POSIX file descriptors, so on Windows use offline trace replay. Pass `--no-debug-instrumentation` to `ispa` if you want to omit generated debug events.

## Build from source

Most users should install a release. Building the generator itself currently requires CMake, vcpkg dependencies, and Clang with libc++ on Linux. GCC is not supported for building ISPA at this time. Windows builds use the MSVC toolchain; the Clang setup is not tested on Windows. See the repository's `vcpkg.json` and CMake toolchain files for build configuration.

For details about grammar syntax, generated APIs, algorithms, and implementation internals, see the [ISPA wiki](https://github.com/Sinfolke/ispa-parser-generator/wiki).
