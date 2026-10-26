# ISPA Parser Generator

**ISPA** is a static parser and lexer generator written from scratch in modern C++ (C++23). You describe a grammar once; ISPA compiles it ahead of time into a lexer and parser that build a fully typed AST. Generated C++ targets C++17; Python output is also available.

The [wiki](https://github.com/Sinfolke/ispa-parser-generator/wiki) covers setup, grammar syntax, generated APIs, algorithms, recovery, external parsers, packages, and debugging.

**Usage demo**

https://github.com/user-attachments/assets/97d0214b-8807-493c-8a55-3b1fb4ea41aa

It is built around one idea: **easy to use, without giving up performance, extensibility, or features.** In benchmarks it is about 12x faster than ANTLR; see [Performance](#-performance).

---

## Why ISPA

1. **Static generation.** Everything is computed at generation time. The grammar is compiled to DFA tables and plain code. Nothing is generated, interpreted, or compiled at runtime.
2. **Small target runtime.** Generated C++ uses headers and `core.cpp` from `parser/stdlibs`; generated Python imports `ispa_std`. Package exports can put these on your build or module path.
3. **Multi-language output.** Code generation goes through **LangAPI**, a language-agnostic IR. **C++17 and Python** emitters are implemented; Python currently uses LL generation.
4. **Extensible grammar descriptions.** Rules describe language structure and connect to generated parser code.
5. **Scales from trivial to larger projects.** Incremental generation and grammar imports help organize and rebuild shared grammars. Feature-gated and parametrized module work remains on the roadmap.
6. **Works out of the box.** Generate, include, call. The generated API returns a typed AST with no post-parsing step or callbacks required.

---

## 🚀 Current State

| Area | Status |
|---|---|
| Compiler (frontend + backend) | Implemented; builds on **Windows** and **Linux** |
| Bootstrap | **Done.** ISPA's own grammar is parsed by an ISPA-generated parser, which fully replaced the hand-written one |
| Lexer (TDFA, AST-building tokens) | **Done** |
| Parser (LL) | Production target; actively growing, with further work and testing ongoing |
| Parser (LR-family modes) | Less capable and experimental for now; see [LR backend status](docs/LR_STATUS.md) |
| Incremental generation | Implemented (regenerates changed parts only) |
| C++17 target | Implemented; output compiles and runs |
| Python target | Implemented for LL generation; generated modules use `ispa_std` from the target runtime |
| Dynamic regex (back-references, counts from earlier captures) | Syntax parsed, **not yet lowered to output** |
| Grammar syntax error reporting | Automatic LL recovery and source diagnostics; syntax errors stop generation |

The backends lower grammar structure and AST captures to C++ or Python. Support for advanced actions and recovery differs by algorithm; see [LR backend status](docs/LR_STATUS.md).

## Debugging generated parsers

For `.isc` editor diagnostics and navigation, see the [ISPA language server](docs/LSP.md),
the [VS Code extension](plugins/vscode/README.md), and the
[JetBrains plugin](plugins/jebtrains/README.md).

Generate and build a parser with the current generator, then run its existing
executable under `ispadbg`:

```sh
./ispadbg --purpose matching --purpose ast -b grammar.isc:12 \
  --record parser-trace.json -- ./my-parser input.txt
./ispadbg --replay parser-trace.json
```

For a graphical view, run `./ispadbg-gui` (or the installed `ispadbg-gui`).
Choose a parser program, enter an input string, then click **Run**. The input
is passed as one command-line argument by default, as in `examples/main.cpp`;
choose **Standard input** for parsers that read stdin. Use **Step**, **Next**,
**Finish**, and **Continue** while paused. Select any trace row to inspect its
variables, including `ctx` and AST values, in the expandable tree. **Open
trace** and **Save trace** use the same JSON format as `ispadbg --replay` and
`--record`. The GUI uses Python's Tkinter and needs a desktop display.

Debug instrumentation is enabled by default when generating parsers. Pass
`--no-debug-instrumentation` to `ispa` to omit generated source checkpoints,
scope events, and value serialization. Pass `--debug-instrumentation` to
explicitly enable them.

Purpose IDs are `matching`, `failure`, and `ast`; `failure` is the default.
The debugger prints the selected IDs at startup. The interactive commands are
`s` (step), `n` (next), `f` (finish), `c` (continue), `b file:line` (breakpoint),
`p [name]` (inspect a value), `ctx`, `back`, `forward`, and `live`. `back` and
`forward` inspect recorded snapshots while the parser is paused; execution
resumes from the live pause. Use `--no-interactive` to record without pausing.

C++ generated code emits rule and production scopes, match outcomes, typed
captures, source checkpoints for converted Python statements, and typed AST
snapshots. The Python target uses the same v2 event protocol through
`parser/stdlibs/target/ispa_debug.py`; include that directory in `PYTHONPATH`
alongside `ispa_std.py` when running a generated Python parser. Traces are JSON
and can be replayed after the parser exits.
Live parser sessions currently require POSIX file descriptors; the Windows
`ispadbg` executable supports offline `--replay` and help.

---

## 🔁 Bootstrap

ISPA is self-hosting for its own grammar. The `.isc` grammar that describes ISPA's input language was compiled by ISPA into the parser that ISPA now uses to read grammars, replacing the original hand-written parser. A Python-subset parser and a small module-scanner parser are generated the same way.

The generated bootstrap parser is committed under `parser/ispa/bootstrapped_0` and is built with ISPA before the generator processes other grammars.

<!-- TODO: optional, add the generated grammar's size (rules, tokens, lines) -->

## External parsers

An external import calls a separately generated parser and captures its original
node type:

```isc
import extern python ispa_python as python;
expr: @ python.expr {@};
```

Use `--dep NAME PREFIX` to select an existing parser. `PREFIX` has no extension;
ISPA reads `PREFIX.ispa-export` for exported symbols and uses `PREFIX.h` as its
public header for C++ output or `PREFIX.py` for Python output. For example:

```sh
ispa -l cpp -a LL --dir parser/ispa/grammar parser/ispa/entry.isc \
  --dep python parser/python/bootstrapped/PyRawParser -o build/Parser
```

The generated C++ includes `<PyRawParser.h>` and uses `PyRawParser::Types`
directly. Add the dependency directory to the compiler's include path and link
its implementation as a static or shared library. The host does not generate
copies of the imported parser's productions or types. Export entry declarations
live in the dependency header; their definitions live in its `.cpp` file.

With `--no-dep`, ISPA discovers external grammar modules in the supplied `--dir`
directories and generates each dependency beside the host, using the external
dependency name as its output prefix. C++ dependencies remain separate `.h`/`.cpp`
files and must be compiled and linked by the build system. Nested dependencies
are generated before their consumers; dependency cycles are diagnosed. Explicit
`--dep` bindings may supply existing libraries while `--no-dep` builds the rest.

External parser calls currently require LL generation and lazy tokenization.
An eager lexer rejects grammars containing external calls with an `AMBIGUOUS`
diagnostic. Lazy calls re-lex the imported span with the target lexer so token
payloads match the target parser, then resume the host lexer at the returned
source position.

For the Python target, generate with `-l python -a LL`. Put the generated
modules and `parser/stdlibs/target` on `PYTHONPATH` so `ispa_std` is available:

```sh
PYTHONPATH=build:parser/stdlibs/target python3 -c 'import Parser; print(Parser.Parser().parse("input"))'
```

Python external imports use the generated dependency module (for example,
`import Foreign`) and its exported parser bindings. Keep dependencies on
`PYTHONPATH` beside the host parser. `--dep` and `--no-dep` work as above.

---

## ⚡ Performance

Lex + parse of the same input, compared against flex + bison (LALR(1)) and ANTLR (C++ target). Lower is better; the multiplier is relative to flex + bison without an AST.

| Configuration | Notes | Time per iteration | Relative |
|---|---|---|---|
| flex + bison, no AST | Lex + LALR(1) parse, recognizes only | 119 ms | 1.0x |
| flex + bison, AST, arena allocator | 3.36 M nodes, bump allocation | 151 ms | 1.3x |
| flex + bison, AST, malloc | 3.36 M nodes; parse 219 ms + tree release 40 ms | 259 ms | 2.2x |
| **ISPA**, allocations stripped | | 400 ms | 3.4x |
| **ISPA**, full AST | | 1528 ms | 12.8x |
| ANTLR (C++), CST construction disabled | | 18,573 ms | 156x |

What this shows:
- ISPA with a full AST is about **12x faster than ANTLR**, which was benchmarked with CST construction disabled.
- ISPA is currently **slower than flex + bison**, roughly 6-10x when the latter builds a comparable AST. Closing that gap is ongoing work.

Flex + bison figures are steady state over 10 iterations.

- input: 9.7 MB
- machine processor: intel core i5 12450h laptop
- compiler: clang18, -O3
- OS: GNU Linux – Linux Mint
- ANTLR: 4.13.2
- flex: 2.6.4
- bison: 3.8.2
- single run
---

## Lexer Capabilities
- Matches tokens with a compact **TDFA** (tagged DFA) table, which keeps captures and actions inside a single automaton
- Builds AST-like tokens natively during lexing. **Capture only what you need**
- Matches tokens inside other tokens without losing structure, preserving each capture's data type
- Matches non-terminals inside terminals during lexing, separately from parsing *(not yet implemented)*

## Parser Capabilities
- LL is the production parser target and is actively growing; alternative LR-family modes currently support fewer grammar features and remain experimental
- Builds a fully typed, easy-to-traverse AST during parsing. No post-parsing pass or callbacks required

## Advanced rules and fail blocks

See the separate [Advanced Rules and Fail Blocks guide](concept/ADVANCED_RULES_AND_FAIL_BLOCKS.md).

---

## Quick start

Install ISPA from a release, install its standard library package, and enable
the package exports for future shells:

```sh
ispa-pkg install std
ispa-pkg --export-permanent
```

Open a new shell. The installed shell helper exposes the include paths and
CMake targets; run `ispa-pkg --export` in that shell whenever you want to
refresh them. Then create `grammar.isc`:

```isc
module main -> demo;

NUMBER: @ [0-9]+ {@};
main: @ NUMBER {@};
```

Generate the C++ parser:

```sh
ispa -l cpp -a LL grammar.isc -o generated/Parser
```

Use the generated parser in your application and link the installed targets
through CMake. In `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20)
project(demo LANGUAGES CXX)
include("$ENV{ISPA_TARGETS_CMAKE}")
add_executable(demo main.cpp generated/Parser.cpp)
target_link_libraries(demo PRIVATE ISPA::Targets)
```

This target supplies the installed ISPA headers and runtime libraries. See the
[wiki](https://github.com/Sinfolke/ispa-parser-generator/wiki/Getting-Started)
for a complete installed-user walkthrough, including Python output.

---

## 🔮 Future Roadmap

- [ ] **Lexer/LR actions and dynamic regex** in generated code
- [x] **Grammar syntax diagnostics with LL recovery** (further improvements ongoing)
- [ ] **PLL algorithm**: finalize the custom Predictive LL algorithm to resolve left recursion in LL parsers while keeping structural parity with LR parsers
- [ ] **Multi-target code generation**
  - [x] Python target emitter for LL
- [ ] **Full LR parser stabilization**: LR(1), LALR, and related modes
- [ ] **Modules**: feature-gated modules, `disable feature X`, parametrized modules
- [ ] **Grammar standard library**: JSON, XML, math expressions, C-like statements
- [ ] Features described in **concepts/**

## Install ISPA

ISPA has prebuilt GitHub Release archives for Linux x86_64, Windows x64, and macOS Apple Silicon (ARM64). macOS Intel is supported by the installer **only if** its release archive exists. All commands install for the current user, with no administrator privileges needed.

### Library packages (`ispa-pkg`)

Release installers provide `ispa-pkg` and `ispadbg` alongside `ispa`. For a
source checkout, install both Python CLIs with `python3 -m pip install .`
(Python 3.9+ and PyYAML). Tagged releases also publish standalone Nuitka builds
named `ispa-pkg-<platform>` and `ispadbg-<platform>` (`.tar.gz`, or `.zip` on
Windows) for the four supported Linux, Windows, and macOS targets. The ISPA
platform archives include both executables, which the installers and updaters
activate.
The CLI lives in `cli/pkg`. Libraries are installed under
`~/.local/share/ispa/libraries` by default on Linux and macOS, or under the
ISPA installation directory when launched from an installed release; use
`ISPA_PKG_ROOT` to choose another location for both commands (`ispa-pkg --root`
changes only the package command). `ispa` searches the matching directory for
grammar and `import py` libraries automatically.
It also loads its bundled algorithm and converter plugins beside the executable.
The installed package has `ispalib/grammar`,
`ispalib/python`, `ispatarget/<language>`, and `ispaalgorithm` directories.
When compiling generated target code, add its `ispatarget/<language>` directory
to your compiler or interpreter search path.
Installed external parser manifests (`NAME.ispa-export` beside `NAME.h` or
`NAME.py`) also provide `--dep` bindings automatically. A package with one
manifest can be referenced by its package name; an explicit `--dep` wins.

```sh
ispa-pkg install std
ispa-pkg install owner/repository --ref v1.2.0
ispa-pkg install https://example.org/mylib.tar.gz --name mylib
ispa-pkg update
ispa-pkg list
ispa-pkg catalog update
ispa-pkg catalog list
```

Run `ispa-pkg --export-permanent` once, then open a new bash, zsh, or PowerShell
session. This installs a small shell function and loads the exports on startup.
In that shell, `ispa-pkg --export` refreshes the exports in the current session
without an `eval` command. The standalone executable prints shell code when the
function is not loaded, because a child process cannot change its parent shell.
Exports add C++ header and library directories to compiler and
linker search paths, shared library directories to the runtime loader path,
and Python target directories to `PYTHONPATH`. Existing search paths are
preserved. The generated `ISPA_TARGETS_CMAKE` file defines `ISPA::Targets`
with installed headers and libraries for CMake projects. During installation,
`ispa-pkg` compiles each package's `ispatarget/cpp/**/*.cpp` files as C++17
and archives them in `ispatarget/cpp/lib` (`lib<name>.a`, or `<name>.lib` with
MSVC). This requires a C++ compiler and archiver; set `CXX`, `CXXFLAGS`, and
`AR` to select the toolchain. Export also builds archives missing from older
installations. The archive directory is added to `LIBRARY_PATH` (or `LIB` on
Windows), and the archive path appears in `ISPA_CPP_LIBRARIES` and
`ISPA_TARGETS_CMAKE`.
For GCC and Clang, the C++ header search variables are `CPATH` and
`CPLUS_INCLUDE_PATH`; `CPLUSPLUS_PATH` is not recognized. Generated C++ code
and the target runtime build with C++17, for example
`clang++ -std=c++17 -c Parser.cpp` after loading the exports. An already
installed `ispa-pkg` executable and `std` library package must be updated to
receive changes made in this source checkout.

The catalog is [`ispa-libraries.yaml`](ispa-libraries.yaml) at the repository
root. `catalog update` fetches the current file from GitHub into the local
package root; `--catalog FILE` selects another YAML catalog. A catalog entry
specifies `github: owner/repository` with optional `ref`, or `archive: URL`.
Installed GitHub packages update when their commit changes. Archives and local
directories update when their contents change. `--force` reinstalls a package.

Library repositories may provide `ispa-package.yaml` at their root. The
[`ispa-package.yaml`](ispa-package.yaml) in this repository demonstrates the
standard library: its source is `parser/stdlibs`, with renamed folders mapped
to their standard destinations. Without a config, package sources are read
from `ispalib/grammar`, `ispalib/python`, `ispatarget/cpp`,
`ispatarget/python`, and `ispaalgorithm`. A config can also use `layout.root`
for a `release` directory, or `archive: release/library.zip` to unpack an
archive from the repository before applying the layout. An `install` list of
repository relative `.py` or shell script paths runs after copying files;
scripts receive the staged destination as their first argument. Pass
`--allow-scripts` to install or update a package that declares scripts.
Use `layout.files` to map individual files such as `target/ispa_std.py`.
LL and LR algorithm plugins are bundled with ISPA; they are not part of `std`.

### Linux and macOS

Review [scripts/install.sh](scripts/install.sh), then install with:

```sh
curl -fsSL https://raw.githubusercontent.com/Sinfolke/ispa-parser-generator/main/scripts/install.sh | sh
```

This downloads the newest **published** GitHub Release for your OS and architecture. The binary is linked as `~/.local/bin/ispa`. Make sure `~/.local/bin` is on your `PATH`, then run:

```sh
ispa --help
```

For an exact version:

```sh
ISPA_VERSION=v1.1.0 sh -c "$(curl -fsSL https://raw.githubusercontent.com/Sinfolke/ispa-parser-generator/main/scripts/install.sh)"
```

### Windows (PowerShell)

Review [scripts/install.ps1](scripts/install.ps1), then run:

```powershell
irm https://raw.githubusercontent.com/Sinfolke/ispa-parser-generator/main/scripts/install.ps1 | iex
```

Open a new terminal and run `ispa --help`. The installer creates `%LOCALAPPDATA%\Programs\ISPA\bin\ispa.cmd` and adds that directory to your **user** PATH.

To install a particular published tag:

```powershell
$env:ISPA_VERSION = 'v1.1.0'
irm https://raw.githubusercontent.com/Sinfolke/ispa-parser-generator/main/scripts/install.ps1 | iex
```

### Change install location

On Linux/macOS, set `ISPA_INSTALL_DIR` and/or `ISPA_BIN_DIR` before running the installer. On Windows, set the same environment variables in PowerShell before executing the script.

### Updating and removing
Update:
```sh
ispa-update
```

To remove ISPA on Linux/macOS (with default paths):

```sh
rm -f "$HOME/.local/bin/ispa"
rm -rf "${XDG_DATA_HOME:-$HOME/.local/share}/ispa"
```

On Windows (with default paths), remove `%LOCALAPPDATA%\Programs\ISPA`, and delete that folder's `bin` entry from your user PATH.

---

## 🛠️ Build & Requirements
1. Clang with libc++ on the latest Ubuntu
2. MSVC `clang-cl` on Windows
3. CMake 4.5.0-rc1, plus dependencies in `vcpkg.json`

**Note: you will not be able to build ISPA with GCC yet.**
**Note: the Clang build is not tested on Windows.**

### Build on Ubuntu
Configure. **Change `ISPA_SOURCE_DIR` to your local path:**
```sh
export ISPA_SOURCE_DIR=/path/to/ispa-parser
export CC=clang-20
export CXX=clang++-20
export CFLAGS="-stdlib=libc++"
export CXXFLAGS="-stdlib=libc++"
export LDFLAGS="-stdlib=libc++"
cmake -B cmake-build-release \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=$ISPA_SOURCE_DIR/cmake/toolchains/clang-libcxx.cmake \
  -DVCPKG_TARGET_TRIPLET=x64-linux-libcxx \
  -DVCPKG_OVERLAY_TRIPLETS=$ISPA_SOURCE_DIR/cmake/triplets
```
Build:
```sh
cmake --build cmake-build-release
```

### Build on Windows
- Change `CMAKE_TOOLCHAIN_FILE` to your vcpkg installation path if this does not work
- You might need to set `CMAKE_CXX_COMPILER` to `cl.exe`
- You might need to configure other variables MSVC requires
```bat
cmake -B cmake-build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="%VCPKG_INSTALLATION_ROOT%/scripts/buildsystems/vcpkg.cmake"
cmake --build cmake-build-release
```
