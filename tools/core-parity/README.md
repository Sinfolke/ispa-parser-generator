# core-parity

Checks that `--stdlib core` (the Python runtime lowered to C++, `parser/stdlibs/target/cpp-core`) behaves exactly like the
hand-written C++ `ispastdlib`: same AST, same diagnostics, same exception text, including error recovery.

Everything builds into `./build` (ignored). Needs a built `ispa` (`ISPA=` or `cmake-build-release/ispa`) and clang++ with C++23 (`CXX=`); the facade itself only needs C++17 (`parser/CMakeLists.txt` builds it at `cxx_std_17`), the harness just compiles its generated test binaries at C++23 regardless of variant.

| script | what it covers |
|---|---|
| `python3 build_ispa.py orig` / `core` | builds the ispa grammar parser (`parser/ispa`) both ways |
| `python3 fuzz_bin.py 600` | mutates `parser/ispa/grammar/*.isc` (recovery paths) and diffs the two parsers |
| `python3 -c "import suite; suite.build_cpp('dyn', 'grammars/dyn/dyn.isc', 'grammars/dyn')"` (+ `stdlib=True`), `python3 fuzz_dyn.py 400` | dynamic value matching |
| `suite.build_cpp('lrjson', 'grammars/json/json.isc', 'grammars/json', algorithm='LALR')` (+ `stdlib=True`), `python3 fuzz_lr.py 300` | LR (LALR) driver in the core |
| `counted_parity.cpp` | counted repetitions, named counters, scopes (`-DCORE` for the facade) |

`counted_parity.cpp` prints its results; build it once with the C++ stdlib include path and once with `-DCORE -I.../cpp-core`, then `diff` the output.

After changing `parser/stdlibs/python/ispa_std/out`, run `parser/stdlibs/target/cpp-core/regen.py` first.
