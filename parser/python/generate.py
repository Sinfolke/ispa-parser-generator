#!/usr/bin/env python3
"""Regenerate the Python bootstrap while preserving its public flat type names."""
import argparse
import json
from pathlib import Path
import re
import subprocess
import tempfile


def main():
    root = Path(__file__).resolve().parent
    options = argparse.ArgumentParser(description=__doc__)
    options.add_argument("generator", type=Path)
    options.add_argument("--output", type=Path, default=root / "bootstrapped2")
    options.add_argument("--stdlib", help="generate against another stdlib (a directory with ispa-stdlib.yaml, e.g. parser/stdlibs/target/cpp-core)")
    args = options.parse_args()
    generator = args.generator.resolve()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    manifest = (root / "ispa-compat.ispa-export").read_bytes()
    sources = [root / "entry.isc", *sorted((root / "grammar").rglob("*.isc"))]
    # The bootstrap API predates module-qualified types. These modules have
    # unique declarations; flatten only their module/import directives.
    lines = []
    locations = []
    for grammar_source in sources:
        if lines:
            lines.append("")
            locations.append(None)
        for number, line in enumerate(grammar_source.read_text().splitlines(), 1):
            if line.startswith(("module ", "import ")):
                continue
            lines.append(line)
            locations.append((grammar_source.resolve(), number))
    grammar = "\n".join(lines)
    with tempfile.TemporaryDirectory(prefix="ispa-python-") as temporary:
        source = Path(temporary) / "python.isc"
        source.write_text(grammar)
        subprocess.run([str(generator), str(source), "-l", "cpp", "-a", "LL",
                        "--no-incremental", "-o", str(output / "PyRawParser"), "--ai-recovery", str(root / "ai.json"),
                        *(["--stdlib", str(Path(args.stdlib).resolve())] if args.stdlib else [])],
                       cwd=temporary, check=True)
        # The flattened source is temporary. Map debug checkpoints back to the
        # grammar files so breakpoints remain usable after generation.
        pattern = re.compile(re.escape(json.dumps(str(source))) + r", (\d+), (\d+)")
        for artifact in (output / "PyRawParser.cpp", output / "PyRawParser.h"):
            contents = artifact.read_text()

            def restore_location(match):
                index = int(match.group(1)) - 1
                if index < 0 or index >= len(locations) or locations[index] is None:
                    return match.group(0)
                original, line = locations[index]
                return f"{json.dumps(str(original))}, {line}, {match.group(2)}"

            artifact.write_text(pattern.sub(restore_location, contents))
    header = output / "PyRawParser.h"
    suffix = "\n" + (root / "exports.hpp").read_text()
    header_text = header.read_text()
    # Incremental regeneration may leave the previous compatibility suffix in
    # place. Replace it instead of appending duplicate export declarations.
    while header_text.endswith(suffix):
        header_text = header_text[:-len(suffix)]
    header.write_text(header_text + suffix)
    # These entry points use the custom Python lexer and must retain their
    # hand-written external bindings rather than generated raw-lexer bindings.
    (output / "PyRawParser.ispa-export").write_bytes(manifest)


if __name__ == "__main__":
    main()
