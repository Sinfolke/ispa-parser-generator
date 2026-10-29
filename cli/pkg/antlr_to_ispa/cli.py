"""Convert ANTLR 4 grammars to ISPA, with a migration report."""
import argparse
import json
from pathlib import Path
import sys
from .actions import TARGETS, canonical_language
from .analysis import load
from .emitter import Emitter
from .companions import convert_companions


def main(argv=None):
    parser = argparse.ArgumentParser(prog='antlr-to-ispa', description=__doc__)
    parser.add_argument('grammar', type=Path, nargs='?')
    parser.add_argument('-o', '--output', type=Path)
    parser.add_argument('-I', '--include', type=Path, action='append', default=[])
    parser.add_argument('--language', type=canonical_language, choices=tuple(TARGETS))
    parser.add_argument('--report', type=Path, help='JSON report (default: OUTPUT.migration.json)')
    parser.add_argument('--list-targets', action='store_true')
    parser.add_argument('--code', type=Path, action='append', default=[], metavar='FILE',
                        help='Convert a companion source file and import its Python module (repeatable)')
    parser.add_argument('--overwrite-code', action='store_true',
                        help='Allow replacing existing companion Python output files')
    args = parser.parse_args(argv)
    if args.list_targets:
        print('\n'.join(TARGETS))
        return 0
    if args.grammar is None:
        parser.error('a .g4 grammar is required')
    output = args.output or args.grammar.with_suffix('.isc')
    report_path = args.report or output.with_suffix('.migration.json')
    if len({args.grammar.resolve(), output.resolve(), report_path.resolve()}) != 3:
        parser.error('input, output and report must be different files')
    try:
        grammar = load(args.grammar, args.include)
        companions = convert_companions(args.code, args.language or grammar.options.get('language', 'Java')) if args.code else None
        text, report = Emitter(grammar, args.language, companions).emit()
        # Keep complete dependency source in the report for manual migration.
        paths = {rule.span.source for rule in grammar.rules} | {t.span.source for t in grammar.declarations} | {str(args.grammar.resolve())}
        paths.update(str(p.resolve()) for p in args.code)
        report['sources'] = {p: Path(p).read_text(encoding='utf-8-sig') for p in sorted(paths)}
        files = {output: text, report_path: json.dumps(report, indent=2, ensure_ascii=False) + '\n'}
        if companions:
            for name, code in companions.files.items():
                destination = output.parent / name
                if destination.resolve() in {output.resolve(), report_path.resolve()}:
                    raise ValueError(f'Companion output collides with grammar/report: {destination}')
                if destination.exists() and destination.read_text(encoding='utf-8') != code and not args.overwrite_code:
                    raise ValueError(f'Companion output exists: {destination}; use --overwrite-code to replace it')
                files[destination] = code
        if any(str(destination.resolve()) in paths for destination in files):
            raise ValueError('An output path would overwrite an input grammar or companion source')
        for destination, contents in files.items():
            destination.write_text(contents, encoding='utf-8')
    except (OSError, ValueError) as error:
        print(f'antlr-to-ispa: {error}', file=sys.stderr)
        return 1
    print(f'Wrote {output} and {report_path}')
    if companions:
        for name in companions.files:
            print(f'Wrote {output.parent / name}')
    for diagnostic in report['diagnostics']:
        span = diagnostic['span']
        location = f"{span['source']}:{span['line']}:{span['column']}: " if span else ''
        print(f"{location}warning [{diagnostic['code']}]: {diagnostic['message']}", file=sys.stderr)
    if report['diagnostics']:
        print('\nChanges required before running this grammar:', file=sys.stderr)
        for remedy in dict.fromkeys(d['remedy'] for d in report['diagnostics']):
            print(f'  - {remedy}', file=sys.stderr)
    else:
        print('No migration warnings.')
    return 0
