# core-parity

Checks that `--stdlib core` (the Python runtime lowered to C++, `parser/stdlibs/target/target-cpp`) behaves exactly like the
hand-written C++ `ispastdlib`: same AST, same diagnostics, same exception text, including error recovery.

Everything builds into `./build` (ignored). Needs a built `ispa` (`ISPA=` or `cmake-build-release/ispa`), CMake and a C++23 compiler (`CXX=`). The harness builds the generated sources and runtime together using CMake. `CXXFLAGS` applies to both, so a libc++ parser cannot accidentally link a runtime built with libstdc++. `ISPA_BENCH_JOBS` controls build parallelism (default 2).

| script | what it covers |
|---|---|
| `python3 build_ispa.py orig` / `core` | builds the ispa grammar parser (`parser/ispa`) both ways |
| `python3 fuzz_bin.py 600` | mutates `parser/ispa/grammar/*.isc` (recovery paths) and diffs the two parsers |
| `python3 -c "import suite; suite.build_cpp('dyn', 'grammars/dyn/dyn.isc', 'grammars/dyn')"` (+ `stdlib=True`), `python3 fuzz_dyn.py 400` | dynamic value matching |
| `suite.build_cpp('lrjson', 'grammars/json/json.isc', 'grammars/json', algorithm='LALR')` (+ `stdlib=True`), `python3 fuzz_lr.py 300` | LR (LALR) driver in the core |
| `counted_parity.cpp` | counted repetitions, named counters, scopes (`-DCORE` for the facade) |

`counted_parity.cpp` prints its results; build it once with the C++ stdlib include path and once with `-DCORE -I.../target-cpp`, then `diff` the output.

After changing `parser/stdlibs/python/ispa_std`, run `parser/stdlibs/target/target-cpp/regen.py` first.

For comparable timings on a grammar supported by all targets:

```sh
python3 tools/core-parity/benchmark_json.py --targets rust cpp python --sizes 8192 32768 131072
python3 tools/core-parity/benchmark_json.py --targets rust --sizes 10485760 --timeout 60
```

`--text-view` (C++ only) generates the parser with token text as non-owning views of the input (`ISPA_STD::Text`; the input must outlive the AST, grammar actions that need an owning `std::string` must use the default copy mode).

The script builds optimized binaries, generates compact single-line JSON, and reports lexing and parsing separately as JSON lines. File reads, compilation and AST printing are outside the timers. Each parse has a timeout, and errors or diagnostics fail the run. Increasing input sizes exercises source-position lookup, which previously rescanned long lines for every Rust token and caused quadratic parsing time.

These are **JSON grammar** timings. The actual ISPA grammar imports the external `PyRawParser` binding, currently implemented in C++; Python/Rust generation needs corresponding bindings before it can be benchmarked. Do not report substitute JSON timings as ISPA grammar results.

For the actual C++ ISPA grammar, build the existing runner through CMake:

```sh
cmake -S parser -B /tmp/ispa-benchmark -DCMAKE_BUILD_TYPE=Release -DISPA_BUILD_EXECUTABLES=ON
cmake --build /tmp/ispa-benchmark --target isparun --parallel 1
/tmp/ispa-benchmark/isparun --file /path/to/input.isc
```

The runner prints parse time before writing the AST. Use the same compiler and flags for the entire CMake build; avoid linking its archives with an unrelated ad-hoc compiler invocation.
