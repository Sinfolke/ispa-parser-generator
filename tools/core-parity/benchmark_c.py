"""JSON timings of the C target alone (the other targets' numbers are not re-measured here).

python3 tools/core-parity/benchmark_c.py --ispa PATH --sizes 8192 32768 262144 10485760
Builds the LL JSON parser the way c_parity.py does, then times lexing and parsing of generated compact JSON."""
import argparse
import json
import os
from pathlib import Path
import subprocess

import c_parity
from c_parity import HERE, RUNTIME, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ispa', default=os.environ.get('ISPA'))
    parser.add_argument('--sizes', nargs='+', type=int, default=[8192, 32768, 262144, 10485760])
    parser.add_argument('--repeat', type=int, default=5)
    parser.add_argument('--timeout', type=float, default=300)
    args = parser.parse_args()
    work = HERE / 'build' / 'c-bench'
    work.mkdir(parents=True, exist_ok=True)
    gen = work / 'gen_c'
    c_parity.build(args.ispa, work, 'json', False, link=False)
    binary = work / 'bench_c'
    sources = [gen / 'parser.c', HERE / 'c' / 'bench.c', RUNTIME / 'ispa_lexer.c', RUNTIME / 'ispa_print.c', RUNTIME / 'ispa_c.c', RUNTIME / 'lowered' / 'ispa_std.c']
    run(['cc', '-std=c11', '-O2', '-w', '-DNS=parser', f'-I{gen}', f'-I{RUNTIME}', f'-I{RUNTIME / "lowered"}', *map(str, sources), '-o', str(binary)])
    row = '{"id":1,"name":"item","value":123.45,"active":true,"tags":["a","b","c"]}'
    for size in args.sizes:
        count = max(1, (size - 1) // (len(row) + 1))
        path = work / f'input-{size}.txt'
        path.write_text('[' + ','.join([row] * count) + ']')
        runs = [json.loads(subprocess.run([str(binary), str(path)], capture_output=True, text=True, check=True, timeout=args.timeout).stdout) for _ in range(args.repeat)]
        med = lambda key: sorted(r[key] for r in runs)[len(runs) // 2]
        print(json.dumps(dict(grammar='json', target='c', bytes=path.stat().st_size, lex_ms=med('lex_ms'), parse_ms=med('parse_ms'))), flush=True)


if __name__ == '__main__':
    main()
