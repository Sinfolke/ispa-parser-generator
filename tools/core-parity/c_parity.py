"""C target vs C++ ispastdlib: the parser generated for a grammar with `-l c` (built on the Python runtime lowered to C,
parser/stdlibs/target/c) must print the same tree and report the same failure as the C++ parser, including error recovery.

    python3 c_parity.py [--grammar json|dyn|counted|failblock|...] [--ispa PATH] [--fuzz N] [--seed S] [--sanitize]

Needs a C11 compiler, CMake and a C++23 compiler; builds into ./build/c (ignored). Same grammars and corpora as rust_parity.py;
trees and diagnostics are compared with the tokens made up front and made on demand."""
import argparse, os, re, shutil, subprocess, sys
from pathlib import Path
import rust_parity
from rust_parity import GRAMMARS, LR_ALGORITHMS, fuzz_inputs, run
from cmake_build import build_cpp

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
RUNTIME = REPO / 'parser' / 'stdlibs' / 'target' / 'c'
LIBRARY = REPO / 'parser' / 'stdlibs' / 'python' / 'ispa_std'
MODULES = ('parser_ll', 'lr_conflicts', 'debug', 'external', 'optional', 'map', 'span', 'vector', 'algorithm', 'ast_printer', 'to_string')


def build(ispa, work, grammar, sanitize):
    env = {key: value for key, value in os.environ.items() if key != 'LD_LIBRARY_PATH'}
    gen, gen_cpp = work / 'gen_c', work / 'gen_cpp'
    gen.mkdir(parents=True, exist_ok=True)
    gen_cpp.mkdir(parents=True, exist_ok=True)
    directory, entry = GRAMMARS[grammar][0], GRAMMARS[grammar][1]
    global WRAPPED
    WRAPPED = False
    algorithm = LR_ALGORITHMS[grammar][1] if grammar in LR_ALGORITHMS else 'LL'
    common = ['-a', algorithm, '--no-incremental', '--no-ai-recovery', '--no-debug-instrumentation']
    # the application is lowered together with the library it runs on (the library itself is compiled separately)
    imports = ''.join(f'import py ispa_std.{name};\n' for name in MODULES)
    # sibling grammar files (token lists) are merged in front of the entry; a leading `module ...;` line stays first
    body = ''.join(f.read_text() for f in sorted(directory.glob('*.isc')) if f.name != entry) + (directory / entry).read_text()
    head, _, rest = body.partition('\n')
    # a plain grammar is declared as the root module, so the imports do not add a wrapper `main` rule above it
    (gen / 'parser.isc').write_text(head + '\n' + imports + rest if head.startswith('module ') else 'module main -> main;\n' + imports + body)
    run([ispa, '-l', 'c', *common, '--dir', str(LIBRARY), str(gen / 'parser.isc'), '-o', str(gen / 'parser')], env=env)
    # the reference is the same module-wrapped grammar without the library imports, so both sides have the same root structure
    (gen_cpp / 'ref.isc').write_text(head + '\n' + rest if head.startswith('module ') else 'module main -> main;\n' + body)
    run([ispa, '-l', 'cpp', *common, str(gen_cpp / 'ref.isc'), '-o', str(gen_cpp / 'generated')], env=env)
    flags = ['-std=c11', '-O1', '-g', '-w'] + (['-fsanitize=address,undefined'] if sanitize else [])
    sources = [gen / 'parser.c', HERE / 'c' / 'run.c', RUNTIME / 'ispa_lexer.c', RUNTIME / 'ispa_print.c', RUNTIME / 'ispa_c.c', RUNTIME / 'lowered' / 'ispa_std.c']
    binary = work / 'run_c'
    run(['cc', *flags, f'-DNS=parser', f'-I{gen}', f'-I{RUNTIME}', f'-I{RUNTIME / "lowered"}', *map(str, sources), '-o', str(binary)])
    shutil.copy(HERE / 'rust' / 'ref_main.cpp', work / 'ref_main.cpp')
    reference = build_cpp(work, 'ref', [gen_cpp / 'generated.cpp', work / 'ref_main.cpp'], [gen_cpp], core=True)
    return binary, reference


WRAPPED = True      # False for grammars that declare their own `module` (no wrapper root)


def normalize(out):
    """The import lines make the grammar a module: rules are named modules::parser::x, and the root is a wrapper
    `main -> value -> <the user's main rule>`. Strip both so the output reads like the plain C++ parser's."""
    for prefix in ('parser', 'main'):
        out = out.replace(f'modules::{prefix}::', '').replace(f'modules_{prefix}_', '').replace(f'modules.{prefix}.', '')
    lines = out.splitlines(keepends=True)
    if WRAPPED and len(lines) > 2 and lines[0] == 'main\n' and lines[1] == '\u2514\u2500\u2500 value\n' and lines[2].startswith('    '):
        body = [line[8:] if line[:1] in ' \u2502' and not line.startswith('ERR') else line for line in lines[2:]]
        return ''.join(body)
    return out


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--ispa', default=os.environ.get('ISPA') or str(REPO / 'cmake-build-release' / 'ispa'))
    parser.add_argument('--grammar', choices=sorted(GRAMMARS), default='json')
    parser.add_argument('--fuzz', type=int, default=150)
    parser.add_argument('--seed', type=int, default=1)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    work = HERE / 'build' / 'c' / args.grammar
    ours, cpp = build(args.ispa, work, args.grammar, args.sanitize)
    inputs = GRAMMARS[args.grammar][2] + list(fuzz_inputs(args.fuzz, args.seed, GRAMMARS[args.grammar][3]))

    def outputs(text, lazy):
        environment = {**os.environ, **({'LAZY': '1'} if lazy else {})}
        # the `import py` lines that bring the library in make the grammar a module: rules are named modules::parser::x
        got = subprocess.run([str(ours), text], capture_output=True, text=True, env=environment).stdout
        return normalize(subprocess.run([str(cpp), text], capture_output=True, text=True, env=environment).stdout), normalize(got)

    failing = [(text, lazy) for text in inputs for lazy in (False, True) if len(set(outputs(text, lazy))) != 1]
    for text, lazy in failing:
        want, got = outputs(text, lazy)
        print(f'DIFF for {text!r}' + (' (lazy)' if lazy else ''))
        for label, out in (('C++', want), ('C  ', got)):
            print('\n'.join(f'  {label}: {line}' for line in out.splitlines()[:int(os.environ.get("DIFF_LINES", 4))]))
    total = 2 * len(inputs)
    print(f'{total - len(failing)} of {total} identical (eager and lazy)')
    sys.exit(1 if failing else 0)


if __name__ == '__main__':
    main()
