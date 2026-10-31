"""Real-world benchmark: the ISPA Python grammar (parser/python) parsing Python source, on the C++, Rust and Python targets, LL and LALR.

    python3 tools/core-parity/benchmark_python.py --targets cpp rust python --algorithms LL LALR --sizes 32768 262144

Everything is built under --work (default /tmp/ispabench). The corpus is real Python (the standard library), restricted to the files the
grammar accepts so that every run parses valid input. Each configuration runs --runs times; the median is reported."""
import argparse, ast, json, os, shutil, statistics, subprocess, sys
from pathlib import Path
from cmake_build import build_cpp

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
GRAMMAR = REPO / 'parser/python'
REUSE = False
CORE = REPO / 'parser/stdlibs/target/target-cpp'

CPP_MAIN = r'''
#include "generated.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
static std::string slurp(const char* path) { std::ifstream f(path, std::ios::binary); return {std::istreambuf_iterator<char>(f), {}}; }
int main(int argc, char** argv) {
    if (argc < 2) return 2;
    if (std::string(argv[1]) == "--check") {  // one OK/FAIL line per file
        for (int i = 2; i < argc; ++i) {
            try {
                auto text = slurp(argv[i]);
                generated::Lexer lexer; lexer.setInput(text.c_str()); lexer.makeTokens();
                generated::Parser parser; parser.parse(lexer);
                std::cout << (parser.getErrors().empty() ? "OK " : "FAIL ") << argv[i] << "\n";
                if (!parser.getErrors().empty()) std::cerr << argv[i] << ": " << parser.getErrors().front().message << " @" << parser.getErrors().front().offset << "\n";
            } catch (const std::exception& e) { std::cout << "FAIL " << argv[i] << "\n"; std::cerr << argv[i] << ": " << e.what() << "\n";
            } catch (...) { std::cout << "FAIL " << argv[i] << "\n"; }
        }
        return 0;
    }
    auto text = slurp(argv[1]);
    const auto start = std::chrono::steady_clock::now();
    generated::Lexer lexer; lexer.setInput(text.c_str()); lexer.makeTokens();
    const auto lex_end = std::chrono::steady_clock::now();
    generated::Parser parser; parser.parse(lexer);
    const auto parse_end = std::chrono::steady_clock::now();
    if (!parser.getErrors().empty()) { std::cerr << "parse errors\n"; return 1; }
    std::cout << "{\"lex_ms\":" << std::chrono::duration<double, std::milli>(lex_end - start).count()
              << ",\"parse_ms\":" << std::chrono::duration<double, std::milli>(parse_end - lex_end).count() << "}\n";
}
'''
PY_MAIN = '''import json, sys, time
from pathlib import Path
from generated import Parser
text = Path(sys.argv[1]).read_text()
start = time.perf_counter()
lexer = Parser.lexer_type(text)
lexer.makeTokens()
lex_ms = (time.perf_counter() - start) * 1000
start = time.perf_counter()
parser = Parser()
parser.parse(lexer)
parse_ms = (time.perf_counter() - start) * 1000
assert not parser.getErrors(), 'parse diagnostics'
print(json.dumps(dict(lex_ms=lex_ms, parse_ms=parse_ms)))
'''


def run(command, **kwargs):
    result = subprocess.run(command, capture_output=True, text=True, **kwargs)
    if result.returncode:
        raise RuntimeError(f'{command[0]} failed ({result.returncode}):\n' + (result.stdout + result.stderr)[-4000:])
    return result.stdout


def generate(ispa, target, algorithm, out, env, extra=()):
    out.mkdir(parents=True, exist_ok=True)
    run([ispa, '-l', target, *(['--stdlib', str(CORE)] if target == 'cpp' else []), '-a', algorithm, '--no-incremental',
         '--no-debug-instrumentation', '--no-ai-recovery', '--dir', str(GRAMMAR / 'grammar'), str(GRAMMAR / 'entry.isc'),
         '-o', str(out / 'generated'), *extra], env=env)


def build(ispa, target, algorithm, work, env):
    out = work / f'{target}-{algorithm}'
    ready = {'cpp': out / 'cmake-benchmark/build/benchmark', 'rust': out / 'target/release/json_bench', 'python': out / 'bench.py'}[target]
    if REUSE and ready.exists():  # --reuse: keep an earlier build (a Rust build of this grammar takes ~12 minutes)
        return [sys.executable, str(ready)] if target == 'python' else [str(ready)]
    print(f'building {target} {algorithm}', file=sys.stderr, flush=True)
    generate(ispa, target, algorithm, out, env)
    if target == 'cpp':
        (out / 'main.cpp').write_text(CPP_MAIN)
        return [str(build_cpp(out, 'benchmark', [out / 'generated.cpp', out / 'main.cpp'], [out]))]
    if target == 'rust':
        (out / 'src').mkdir(exist_ok=True)
        (out / 'Cargo.toml').write_text('[package]\nname="json_bench"\nversion="0.1.0"\nedition="2021"\n[dependencies]\nispa_std={path='
                                        + json.dumps(str(REPO / 'parser/stdlibs/target/rust')) + '}\n')
        (out / 'src/lib.rs').write_text('include!("../generated.rs");\npub use generated::*;\n')
        shutil.copy(HERE / 'rust/bench.rs', out / 'src/main.rs')
        # one codegen unit and one job: rustc needs ~5 GB for this grammar and more with parallel codegen
        run(['cargo', 'build', '--release', '--offline'], cwd=out, env={**env, 'CARGO_TARGET_DIR': str(out / 'target'),
             'CARGO_PROFILE_RELEASE_CODEGEN_UNITS': '1', 'CARGO_BUILD_JOBS': '1'})
        return [str(out / 'target/release/json_bench')]
    (out / 'bench.py').write_text(PY_MAIN)
    return [sys.executable, str(out / 'bench.py')]


class Subset(ast.NodeTransformer):
    # Docstrings go; strings keep their shape but lose quotes, escapes and non-ASCII (ast.unparse then always writes '...').
    def _body(self, node):
        if node.body and isinstance(node.body[0], ast.Expr) and isinstance(getattr(node.body[0], 'value', None), ast.Constant) \
                and isinstance(node.body[0].value.value, str):
            node.body = node.body[1:] or [ast.Pass()]
        return self.generic_visit(node)
    visit_Module = visit_FunctionDef = visit_AsyncFunctionDef = visit_ClassDef = _body
    def visit_Constant(self, node):
        if isinstance(node.value, str):
            node.value = ''.join(c for c in node.value if c.isascii() and c.isprintable() and c not in '\'"\\')
        elif isinstance(node.value, bytes):
            return ast.copy_location(ast.Constant('b'), node)
        return node
    def visit_JoinedStr(self, node):
        return ast.copy_location(ast.Constant('f'), node)


def corpus(checker, sources, work):
    """Real top-level Python statements (standard library, normalised to the grammar's subset) that the grammar accepts."""
    listing = work / 'corpus.json'
    if listing.exists():
        return json.loads(listing.read_text())
    pieces = work / 'statements'
    pieces.mkdir(exist_ok=True)
    names = []
    for source in sources:
        for p in sorted(Path(source).glob('*.py')):
            try: tree = Subset().visit(ast.parse(p.read_text()))
            except (SyntaxError, ValueError, UnicodeDecodeError, RecursionError): continue
            for node in tree.body:
                try: text = ast.unparse(node) + '\n'
                except RecursionError: continue
                if len(text) > 30000: continue
                target = pieces / f'{len(names):06d}.py'
                target.write_text(text)
                names.append(str(target))
    accepted = []
    for start in range(0, len(names), 200):
        batch = names[start:start + 200]
        result = subprocess.run([*checker, '--check', *batch], capture_output=True, text=True)
        accepted += [line.split(' ', 1)[1] for line in result.stdout.splitlines() if line.startswith('OK ')]
        seen = {line.split(' ', 1)[1] for line in result.stdout.splitlines() if line.startswith(('OK ', 'FAIL '))}
        for name in batch:  # a crash lost the rest of the batch: retry those one by one
            if name not in seen:
                one = subprocess.run([*checker, '--check', name], capture_output=True, text=True)
                if one.stdout.startswith('OK '): accepted.append(name)
    listing.write_text(json.dumps(accepted))
    print(f'{len(accepted)} of {len(names)} statements accepted', file=sys.stderr)
    return accepted


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ispa', default=os.environ.get('ISPA') or str(REPO / 'cmake-build-release/ispa'))
    parser.add_argument('--targets', nargs='+', choices=('cpp', 'rust', 'python'), default=['cpp'])
    parser.add_argument('--algorithms', nargs='+', default=['LL', 'LALR'])
    parser.add_argument('--sizes', nargs='+', type=int, default=[32768, 262144])
    parser.add_argument('--runs', type=int, default=5)
    parser.add_argument('--timeout', type=float, default=1800)
    parser.add_argument('--source', nargs='+', default=['/usr/lib/python3.12'])
    parser.add_argument('--reuse', action='store_true', help='keep builds already present under --work')
    parser.add_argument('--antlr', type=Path, help='ANTLR benchmark binary (tools/core-parity README: ANTLR section); timed as lexing + parsing without and with a parse tree')
    parser.add_argument('--limit', nargs='*', default=[], metavar='TARGET:ALGORITHM=BYTES', help='largest input for a configuration (slow ones)')
    parser.add_argument('--work', type=Path, default=Path('/tmp/ispabench'))
    args = parser.parse_args()
    global REUSE
    REUSE = args.reuse
    work = args.work.resolve(); work.mkdir(parents=True, exist_ok=True)
    env = {k: v for k, v in os.environ.items() if k != 'LD_LIBRARY_PATH'}
    limits = {k: int(v) for k, v in (item.split('=') for item in args.limit)}
    env['PYTHONPATH'] = os.pathsep.join(filter(None, [str(REPO / 'parser/stdlibs/target'), env.get('PYTHONPATH', '')]))
    checker = build(args.ispa, 'cpp', 'LL', work, env)
    files = corpus(checker, args.source, work)
    print(f'corpus: {len(files)} accepted files', file=sys.stderr, flush=True)
    inputs = {}
    for size in args.sizes:
        text, parts = '', []
        for name in files:
            piece = Path(name).read_text(errors='replace')
            if not piece.endswith('\n'): piece += '\n'
            if len(text) + len(piece) > size: continue
            text += piece
        path = work / f'input-{size}.py'; path.write_text(text); inputs[size] = path
    for target in args.targets:
        for algorithm in args.algorithms:
            command = checker if (target, algorithm) == ('cpp', 'LL') else build(args.ispa, target, algorithm, work, env)
            for size, path in inputs.items():
                if size > limits.get(f'{target}:{algorithm}', 1 << 60): continue
                lex, parse = [], []
                for _ in range(args.runs if target != 'python' else min(args.runs, 3)):
                    try:
                        timing = json.loads(run([*command, str(path)], env=env, timeout=args.timeout))
                    except subprocess.TimeoutExpired:
                        timing = None; break
                    except RuntimeError as error:
                        print(json.dumps(dict(target=target, algorithm=algorithm, bytes=size, error=str(error)[-300:])), flush=True); timing = None; break
                    lex.append(timing['lex_ms']); parse.append(timing['parse_ms'])
                if lex:
                    print(json.dumps(dict(grammar='python', target=target, algorithm=algorithm, bytes=len(path.read_bytes()),
                                          runs=len(lex), lex_ms=round(statistics.median(lex), 2), parse_ms=round(statistics.median(parse), 2))), flush=True)

    if args.antlr:
        for tree in ('', 'tree'):
            for size, path in inputs.items():
                lex, parse = [], []
                for _ in range(args.runs):
                    timing = json.loads(run([str(args.antlr), str(path), *([tree] if tree else [])], env=env, timeout=args.timeout))
                    lex.append(timing['lex_ms']); parse.append(timing['parse_ms'])
                print(json.dumps(dict(grammar='python', target='antlr', algorithm='ALL(*)' + (' + parse tree' if tree else ''),
                                      bytes=len(path.read_bytes()), runs=len(lex), lex_ms=round(statistics.median(lex), 2),
                                      parse_ms=round(statistics.median(parse), 2))), flush=True)


if __name__ == '__main__':
    main()
