# ISPA browser playground

Write an ISPA grammar, write (or keep the default) Python script, and run both in the browser.
Nothing is sent to a server: the real ISPA generator runs as WebAssembly, and the parser it
generates runs in Pyodide.

```
grammar (*.isc) ──► ispa.wasm (Emscripten) ──► Generated.py ──► Pyodide ──► script.py ──► result
```

Everything runs in a Web Worker (`worker.js`), so the page never blocks; **Stop** terminates and
restarts the worker.

## Python script

`script.py` runs after the grammar compiles. It sees:

| name      | meaning                                         |
|-----------|--------------------------------------------------|
| `Parser`  | the generated parser class                       |
| `grammar` | the generated module                             |
| `text`    | the test input                                   |
| `to_json` | converts parser values to JSON-friendly data     |

Assign what you want to display to `result`; `print()` goes to the Console tab. The default
script is just `result = Parser().parse(text).data()`.

## Building

```sh
source /path/to/emsdk/emsdk_env.sh         # Emscripten 6.0.13
export VCPKG_ROOT=/path/to/vcpkg
scripts/build-wasm.sh                      # builds ispa.wasm and fills playground/runtime/
node scripts/test-wasm.mjs playground      # generator smoke test
node scripts/test-playground.mjs playground  # drives the page in headless Chrome
python3 -m http.server 8000 --directory playground
```

`playground/runtime/` (generator + Python runtime) is generated and git-ignored.

## Deployment

`.github/workflows/playground.yml` rebuilds the WebAssembly generator and redeploys GitHub Pages on
every push to `main` that touches the generator, its runtimes or the playground. In the repository
settings, set **Pages → Source** to **GitHub Actions** once.

## Limits of the browser build

- Only the LL algorithm and the Python target are included (LR/LALR and the C++/Rust targets are
  native-only for now).
- `size_t` is 32-bit in WebAssembly, so internal rule/token ids are 32-bit hashes; generated code is
  otherwise identical to the native generator's.
- Pyodide is loaded from the jsDelivr CDN on first run.
