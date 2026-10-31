#!/usr/bin/env python3
"""Copy a finished WebAssembly build of ispa and the Python runtime into playground/runtime.

usage: package-playground.py <wasm-build-dir> [<playground-dir>]
"""
import json
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def main() -> int:
    if len(sys.argv) < 2:
        print(__doc__, file=sys.stderr)
        return 2
    build = Path(sys.argv[1])
    playground = Path(sys.argv[2]) if len(sys.argv) > 2 else ROOT / "playground"
    runtime = playground / "runtime"
    if runtime.exists():
        shutil.rmtree(runtime)
    runtime.mkdir(parents=True)
    for name in ("ispa.mjs", "ispa.wasm"):
        shutil.copy2(build / name, runtime / name)

    # What generated Python parsers import: ispa_std, ispa_debug and the `python` package behind them.
    target = ROOT / "parser" / "stdlibs" / "target"
    sources = {}
    for path in [target / "ispa_std.py", target / "ispa_debug.py", *sorted((target / "python").glob("*.py"))]:
        if path.name.startswith("test_"):
            continue
        sources[path.relative_to(target).as_posix()] = path.read_text(encoding="utf-8")
    (runtime / "python-runtime.json").write_text(json.dumps(sources), encoding="utf-8")
    print(f"packaged {len(sources)} Python files, {(runtime / 'ispa.wasm').stat().st_size // 1024} KiB wasm -> {runtime}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
