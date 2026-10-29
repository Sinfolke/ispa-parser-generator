"""Rust target vs C++ ispastdlib: the parser generated for a grammar with `-l rust` (built on the Python runtime lowered to
Rust, parser/stdlibs/target/rust) must print the same tree and report the same failure as the C++ parser, including
error recovery on malformed input.

    python3 rust_parity.py [--ispa PATH] [--fuzz N] [--seed S]

Needs cargo and clang++ (C++23). Builds into ./build/rust (ignored). `ISPA_RUST_EXPERIMENTAL=1` is set for the generator
(the Rust parser backend is still behind that switch)."""
import argparse, os, random, re, shutil, subprocess, sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
RUNTIME = REPO / 'parser' / 'stdlibs' / 'target' / 'rust'
CPP_STDLIB = REPO / 'parser' / 'stdlibs' / 'target' / 'cpp'

# grammar name -> (entry file, fixed corpus, fuzz alphabet)
GRAMMARS = {
    'json': ('json.isc', ['[1, "a", true]', '{"a": [1, 2, {"b": false}], "c": "x"}', '[]', '{}', '  [ 1.5 , 2 ]  ', '"s"', 'true', '[[[[1]]]]',
          '[1,', '[1, 2', '{"a": [1,', '{"a": 1, ', '[1 2]', '{"a" 1}', '{"a": }', '[,]', '[1,,2]', '{,}', 'x', '-3',
          '{"a":1,"b":2,"c":[],"d":{}}', '{"k": {"n": [true, false, "s", 1.25]}}'],
             ['[', ']', '{', '}', ',', ':', '"k"', '"v"', '1', '2.5', 'true', 'false', ' ', '  ']),
    # dynamic values: a quote character or heredoc tag captured earlier decides where the match ends
    'dyn': ('dyn.isc', ['"abc", \'d\\\'e\', <<EOT\nline one\nEOT, "x\\"y"', "'a' , \"b\"", '<<A\nxx\nA,<<BB\nA\nBB', '"unterminated', '<<T\nbody T',
                        "'it\"s", '<<X\n\nX', '"a""b"', '', ',', '"a",'],
            ['"', "'", '<<', ',', '\\', '\n', ' ', 'EOT', 'A\n', 'a', 'b', 'T\n']),
}


def run(command, **kwargs):
    result = subprocess.run(command, capture_output=True, text=True, **kwargs)
    if result.returncode:
        sys.exit(f'{command[0]} failed:\n' + (result.stdout + result.stderr)[-2500:])
    return result


def build(ispa, work, grammar):
    env = {key: value for key, value in os.environ.items() if key != 'LD_LIBRARY_PATH'}
    env['ISPA_RUST_EXPERIMENTAL'] = '1'
    gen, gen_cpp = work / 'gen_rust', work / 'gen_cpp'
    gen.mkdir(parents=True, exist_ok=True)
    gen_cpp.mkdir(parents=True, exist_ok=True)
    common = ['-a', 'LL', '--no-incremental', '--no-ai-recovery', '--no-debug-instrumentation', '--dir', str(HERE / 'grammars' / grammar), str(HERE / 'grammars' / grammar / GRAMMARS[grammar][0])]
    run([ispa, '-l', 'rust', *common, '-o', str(gen / 'generated')], env=env)
    run([ispa, '-l', 'cpp', *common, '-o', str(gen_cpp / 'generated')], env=env)
    # the generated Rust file is a module root: its crate-level attributes move to the including crate
    source = re.sub(r'#!\[allow\(.*?\)\]\n', '', (gen / 'generated.rs').read_text(), count=1, flags=re.S)
    crate = work / 'crate'
    (crate / 'src' / 'bin').mkdir(parents=True, exist_ok=True)
    (crate / 'Cargo.toml').write_text(f'[package]\nname = "parity"\nversion = "0.1.0"\nedition = "2021"\n[dependencies]\nispa_std = {{ path = "{RUNTIME}" }}\n')
    (crate / 'src' / 'lib.rs').write_text('#![allow(warnings)]\npub use self::generated::*;\ninclude!("generated.rs");\n')
    (crate / 'src' / 'generated.rs').write_text(source.replace('pub mod generated', 'pub mod generated', 1))
    shutil.copy(HERE / 'rust' / 'run.rs', crate / 'src' / 'bin' / 'run.rs')
    run(['cargo', 'build', '--quiet', '--bin', 'run'], cwd=crate)
    reference = work / 'ref'
    shutil.copy(HERE / 'rust' / 'ref_main.cpp', work / 'ref_main.cpp')
    run([os.environ.get('CXX', 'clang++'), '-std=c++23', '-O1', '-I', str(CPP_STDLIB), '-I', str(gen_cpp), str(gen_cpp / 'generated.cpp'),
         str(work / 'ref_main.cpp'), str(CPP_STDLIB / 'src' / 'core.cpp'), '-o', str(reference)])
    return crate / 'target' / 'debug' / 'run', reference


def fuzz_inputs(count, seed, pieces):
    rng = random.Random(seed)
    for _ in range(count):
        yield ''.join(rng.choice(pieces) for _ in range(rng.randint(1, 14)))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--ispa', default=os.environ.get('ISPA') or str(REPO / 'cmake-build-release' / 'ispa'))
    parser.add_argument('--grammar', choices=sorted(GRAMMARS), default='json')
    parser.add_argument('--fuzz', type=int, default=150)
    parser.add_argument('--seed', type=int, default=1)
    args = parser.parse_args()
    work = HERE / 'build' / 'rust' / args.grammar
    rust, cpp = build(args.ispa, work, args.grammar)
    different = 0
    inputs = GRAMMARS[args.grammar][1] + list(fuzz_inputs(args.fuzz, args.seed, GRAMMARS[args.grammar][2]))
    for text in inputs:
        want = subprocess.run([str(cpp), text], capture_output=True, text=True).stdout
        got = subprocess.run([str(rust), text], capture_output=True, text=True).stdout
        if want != got:
            different += 1
            print(f'DIFF for {text!r}')
            for label, out in (('C++ ', want), ('Rust', got)):
                print('\n'.join(f'  {label}: {line}' for line in out.splitlines()[:4]))
    print(f'{len(inputs) - different} of {len(inputs)} identical')
    sys.exit(1 if different else 0)


if __name__ == '__main__':
    main()
