"""Convert C++ source files to ISPA-compatible Python with Tree-sitter."""
import argparse
import json
from pathlib import Path
import sys

from .companions import convert_companions


def main(argv=None):
    parser = argparse.ArgumentParser(prog='cpp-to-ispa', description=__doc__)
    parser.add_argument('source', type=Path, nargs='+')
    parser.add_argument('-o', '--output-dir', type=Path, default=Path('.'))
    parser.add_argument('--report', type=Path, help='Write diagnostics and preserved source as JSON')
    args = parser.parse_args(argv)
    if any(path.suffix.lower() not in {'.cpp', '.cc', '.cxx', '.h', '.hpp', '.hxx'} for path in args.source):
        parser.error('all input files must be C++ source or header files')
    try:
        result = convert_companions(args.source, 'Cpp')
        destinations = {args.output_dir / name: contents for name, contents in result.files.items()}
        sources = {path.resolve() for path in args.source}
        if any(path.resolve() in sources for path in destinations):
            raise ValueError('output would overwrite an input source file')
        for path, contents in destinations.items():
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(contents, encoding='utf-8')
        if args.report:
            report = {
                'sources': {str(unit.path): unit.source for unit in result.units},
                'diagnostics': [diagnostic.to_dict() for diagnostic in result.diagnostics],
            }
            args.report.write_text(json.dumps(report, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    except (OSError, ValueError) as error:
        print(f'cpp-to-ispa: {error}', file=sys.stderr)
        return 1
    for path in destinations:
        print(f'Wrote {path}')
    for diagnostic in result.diagnostics:
        span = diagnostic.span
        location = f'{span.source}:{span.line}:{span.column}: ' if span else ''
        print(f'{location}warning [{diagnostic.code}]: {diagnostic.message}', file=sys.stderr)
    return 0
