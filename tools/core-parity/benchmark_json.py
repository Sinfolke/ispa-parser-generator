"""Comparable JSON timings for supported targets; never substitute for ISPA grammar timings.

python3 tools/core-parity/benchmark_json.py --targets rust cpp python --sizes 8192 32768 131072
Each execution has a timeout; failed parses are errors, never successful timings.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

from cmake_build import build_cpp

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]

CPP_MAIN = r'''
#include "generated.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
int main(int argc, char** argv) {
    if (argc != 2) return 1;
    std::ifstream file(argv[1], std::ios::binary);
    if (!file) return 1;
    std::string text((std::istreambuf_iterator<char>(file)), {});
    const auto start = std::chrono::steady_clock::now();
    generated::Lexer lexer;
    lexer.setInput(text.c_str());
    lexer.makeTokens();
    const auto lex_end = std::chrono::steady_clock::now();
    generated::Parser parser;
    parser.parse(lexer);
    const auto parse_end = std::chrono::steady_clock::now();
    if (!parser.getErrors().empty()) return 1;
    if ([](auto& p) { if constexpr (requires { p.getContextErrors(); }) return !p.getContextErrors().empty(); else return false; }(parser)) return 1;
    std::cout << "{\"lex_ms\":" << std::chrono::duration<double, std::milli>(lex_end-start).count()
              << ",\"parse_ms\":" << std::chrono::duration<double, std::milli>(parse_end-lex_end).count() << "}\n";
}
'''

PY_MAIN = '''import json, sys, time
from pathlib import Path
from generated import Lexer, Parser
text = Path(sys.argv[1]).read_text()
start = time.perf_counter()
lexer = Lexer()
lexer.makeTokens(text)
lex_ms = (time.perf_counter() - start) * 1000
start = time.perf_counter()
parser = Parser()
parser.parse(lexer)
parse_ms = (time.perf_counter() - start) * 1000
assert not parser.getErrors() and not parser.getContextErrors(), 'parse diagnostics'
print(json.dumps(dict(lex_ms=lex_ms, parse_ms=parse_ms)))
'''


def run(command, **kwargs):
    result = subprocess.run(command, capture_output=True, text=True, **kwargs)
    if result.returncode:
        raise RuntimeError(f'{command[0]} failed ({result.returncode}):\n' + (result.stdout + result.stderr)[-5000:])
    return result.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ispa', default=os.environ.get('ISPA') or str(REPO / 'cmake-build-release/ispa'))
    parser.add_argument('--targets', nargs='+', choices=('rust', 'cpp', 'python'), default=['rust'])
    parser.add_argument('--sizes', nargs='+', type=int, default=[8192, 32768, 131072, 10485760])
    parser.add_argument('--timeout', type=float, default=60)
    parser.add_argument('--text-view', action='store_true', help='C++ target: generate with --text-view (token text as views of the input)')
    parser.add_argument('--algorithm', default='LL', help='ispa -a value, e.g. LL or LALR (C++ target for LR)')
    parser.add_argument('--grammar', choices=('json', 'expr'), default='json')
    parser.add_argument('--work', type=Path, default=None)
    args = parser.parse_args()
    if any(size < 2 for size in args.sizes) or args.timeout <= 0:
        parser.error('sizes must be >= 2 and timeout must be positive')
    work = (args.work or HERE / ('build/benchmark-json' if args.grammar == 'json' else 'build/benchmark-' + args.grammar)).resolve()
    work.mkdir(parents=True, exist_ok=True)
    env = {key: value for key, value in os.environ.items() if key != 'LD_LIBRARY_PATH'}
    grammar = HERE / f'grammars/{args.grammar}/{args.grammar}.isc'
    commands = {}
    for target in args.targets:
        generated = work / target
        generated.mkdir(exist_ok=True)
        print(f'Building JSON benchmark: {target}', file=sys.stderr, flush=True)
        run([args.ispa, '-l', target, *(['--stdlib', str(REPO / 'parser/stdlibs/target/target-cpp')] if target == 'cpp' else []), '-a', args.algorithm, '--no-incremental', '--no-debug-instrumentation',
             *(['--text-view'] if args.text_view and target == 'cpp' else []), '--no-ai-recovery', '--dir', str(grammar.parent), str(grammar), '-o', str(generated / 'generated')], env=env)
        if target == 'rust':
            (generated / 'src').mkdir(exist_ok=True)
            runtime = REPO / 'parser/stdlibs/target/rust'
            (generated / 'Cargo.toml').write_text('[package]\nname="json_bench"\nversion="0.1.0"\nedition="2021"\n'
                                                '[dependencies]\nispa_std={path=' + json.dumps(str(runtime)) + '}\n')
            (generated / 'src/lib.rs').write_text('include!("../generated.rs");\npub use generated::*;\n')
            shutil.copy(HERE / 'rust/bench.rs', generated / 'src/main.rs')
            run(['cargo', 'build', '--release', '--offline'], cwd=generated)
            commands[target] = [str(generated / 'target/release' / ('json_bench.exe' if os.name == 'nt' else 'json_bench'))]
        elif target == 'cpp':
            (generated / 'main.cpp').write_text(CPP_MAIN)
            commands[target] = [str(build_cpp(generated, 'benchmark', [generated / 'generated.cpp', generated / 'main.cpp'], [generated]))]
        else:
            (generated / 'bench.py').write_text(PY_MAIN)
            commands[target] = [sys.executable, str(generated / 'bench.py')]
    env['PYTHONPATH'] = os.pathsep.join(filter(None, [str(REPO / 'parser/stdlibs/target'), env.get('PYTHONPATH', '')]))
    # Compact, single-line input exposes quadratic source-column scans. No file IO or AST printing is timed.
    row = '{"id":1,"name":"item","value":123.45,"active":true,"tags":["a","b","c"]}'
    unit = '(12+34*56-78/9)*(3+(4-5)*6)+'
    for size in args.sizes:
        if args.grammar == 'json':
            count = max(1, (size - 1) // (len(row) + 1))
            text = '[' + ','.join([row] * count) + ']'
        else:
            text = unit * max(1, (size - 2) // len(unit)) + '1'
        path = work / f'input-{size}.txt'
        path.write_text(text)
        for target, command in commands.items():
            try:
                timing = json.loads(run([*command, str(path)], env=env, timeout=args.timeout))
                print(json.dumps(dict(grammar=args.grammar, target=target, bytes=len(text), **timing)), flush=True)
            except subprocess.TimeoutExpired:
                raise RuntimeError(f'{target}: JSON {len(text)} bytes timed out after {args.timeout}s')


if __name__ == '__main__':
    main()
