"""Command line interfaces for Flex and Bison migration."""
import argparse
import json
from pathlib import Path
import sys
from .converter import convert


def main(kind, argv=None):
    parser = argparse.ArgumentParser(prog=kind + '-to-ispa', description='Convert ' + kind + ' grammar to ISPA with a migration report.')
    parser.add_argument('grammar', type=Path)
    parser.add_argument('-o', '--output', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args(argv)
    output = args.output or args.grammar.with_suffix('.isc')
    report = args.report or output.with_suffix('.migration.json')
    if len({p.resolve() for p in (args.grammar, output, report)}) != 3:
        parser.error('input, output and report must be different files')
    try:
        text, details = convert(args.grammar.read_text(encoding='utf-8-sig'), kind)
        output.write_text(text, encoding='utf-8')
        report.write_text(json.dumps(details, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    except (OSError, ValueError) as error:
        print(f'{kind}-to-ispa: {error}', file=sys.stderr)
        return 1
    print(f'Wrote {output} and {report}')
    for diagnostic in details['diagnostics']:
        print(f"warning [{diagnostic['code']}]: {diagnostic['message']}", file=sys.stderr)
    return 0
