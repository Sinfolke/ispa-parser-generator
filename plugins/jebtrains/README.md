# ISPA Grammar for JetBrains IDEs

Recognizes `.isc` files with semantic colors for rule and token declarations, references, parameters, and captures. Module directives, features, comments, character classes, and embedded Python have tolerant highlighting while editing. Python identifiers, attributes, calls, builtins, keywords, strings, numbers, comments, operators, and punctuation have distinct colors in direct blocks and unbraced expressions. The current grammar uses direct Python in `fail` and advanced rule blocks, action expressions, and `${...}` patterns; legacy `p{...}` wrappers are rejected by the parser. Native parser errors appear in the editor, including the expected symbols and where parsing stopped. Token highlighting remains available after parsing fails; UTF-8 native positions are translated to UTF-16 editor positions.

Completion suggests rule and token declarations from the current file and directly imported `.isc` files. Grammar import paths offer nearby modules. Inside Python actions, variable targets offer local variables, captures, parameters, and `ctx`; expression positions also offer functions, modules, and builtins. Python import paths offer nearby sidecar modules and common standard library modules, while `from ... import` and member access read nearby Python sidecars. Type annotations offer type names. Completion items carry symbol icons. Go to Declaration works for matching local grammar references. The editor model remains available while the grammar is incomplete.

List and dict variables offer their methods after `.` when their type comes from an annotation, a literal, a constructor, or a nearby assignment. Python expression completion includes the functions and signatures bundled from `parser/stdlibs/python/stdfun.py`.

`ctx.shared.` and `ctx.ast.` completion scans field declarations and uses in the current grammar and directly imported `.isc` files. These fields are dynamic at compile time, so completion shows fields visible in those sources rather than a fixed schema. Typed shared or AST fields also offer list and dict methods after another `.`.

Native parser validation remains local to the current file. Import (including `import extern python ispa_python as python;`) and detach directives are checked without resolving external modules. Direct embedded Python is validated by the bundled Python parser and highlighted while editing. Editor semantic checks currently cover duplicate rule declarations, invalid `[skip]` targets (GRM102), suspicious `@TOKEN` captures (G305), invalid `ctx` members (PY261), and literal assignment type mismatches (PY245/PY210). These checks follow `src/Semantic.cppm` where the necessary information is available; full AST checks and general Python type inference still run in the compiler. No Python IDE plugin is required. The native helper has a two-second timeout; lexical highlighting remains available if it cannot run.

Live local errors for unfinished rules, strings, comments, character classes, and Python blocks appear through a regular IntelliJ annotator. Native parser diagnostics add deeper syntax errors. These editor errors do not require a locally installed `ispa` compiler.

Malformed Python numeric tokens are marked at their exact source location during editing. The bundled Python parser also reports the offending line of an advanced-rule action instead of anchoring every failure at the beginning of the action.

For saved files, the plugin also runs the local `ispa --check-only -l cpp` command in a background annotator and shows the full compiler semantic diagnostics. It discovers `ispa` under `cmake-build-release`, `cmake-build-debug`, or `build` in the project root. Set `ISPA_COMPILER` or the `ispa.compiler.path` JVM property to use another binary. Results are cached by file contents and compiler build time, and the command has a 1.5-second limit. Unsaved edits use the bundled parser and editor checks immediately. A compiler without `--check-only` is ignored.

## Build and install

Build with JDK 21, CMake 3.22+, and a C++23 compiler compatible with the parser. A local build contains a native helper for the host OS and architecture. The GitHub Actions release assembles Linux x64, Windows x64, macOS x64, and macOS arm64 helpers into one ZIP. The plugin selects the matching helper at runtime. Compiler and linker setup is taken from the environment by CMake.

Example matching the ISPA Clang/libc++ setup on this machine:

```sh
CC=/opt/llvm-23.1.2-icx/bin/clang \
CFLAGS='-stdlib=libc++ -I/opt/llvm-23.1.2-icx/include/c++/v1' \
CXX=/opt/llvm-23.1.2-icx/bin/clang++ \
CXXFLAGS='-stdlib=libc++ -I/opt/llvm-23.1.2-icx/include/c++/v1' \
LD=ld.lld \
LDFLAGS='-stdlib=libc++ -L/opt/llvm-23.1.2-icx/lib -L/home/ura/intel/oneapi/compiler/2026.1/lib' \
./gradlew buildPlugin
```

Use compiler/runtime paths appropriate to your own machine. The packaged native helper must be able to locate its C++ runtime libraries when the IDE launches it.

Install the current `build/distributions/ispa-grammar-<version>.zip` from **Settings → Plugins → ⚙ → Install Plugin from Disk**, then restart the IDE. `./gradlew runIde` starts a sandbox IDE for testing.

## Releases

Changing `version` in `build.gradle.kts` on `main` triggers `.github/workflows/jetbrains-plugin-release.yml`. It builds the four native helpers, runs plugin tests, checks that all helpers are in the ZIP, and uploads that ZIP to a `jetbrains-v<version>` GitHub Release. A manual workflow run can rebuild a release without changing the version.

To publish version updates to JetBrains Marketplace, first submit the plugin through the Marketplace web interface and add a repository Actions secret named `JETBRAINS_MARKETPLACE_TOKEN`. The workflow then runs Gradle's `publishPlugin` task on version changes. Manual workflow runs leave Marketplace publishing off unless selected. If the secret is absent, the GitHub Release is still created and the Marketplace step is skipped. Use a new version for each Marketplace update.

The native build links `ISPA::Parser` from `parser/CMakeLists.txt`, using the current bootstrap in `../../parser/ispa/bootstrapped_)` and linking its bundled Python dependency. A standalone CMake build can override the parser with `-DISPA_BOOTSTRAPPED_DIR=/path/to/generated/parser`. Gradle tracks generated ISPA and Python parser sources and the shared standard library for rebuilds.

Run `./gradlew test` for lexer restart, Unicode protocol, and packaged helper tests. Run `python3 tests/bridge_test.py build/native-build/ispa-grammar-bridge` for native parsing and error-position tests.
