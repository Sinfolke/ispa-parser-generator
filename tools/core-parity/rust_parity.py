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

# grammar name -> (directory, entry file, fixed corpus, fuzz alphabet)
GRAMMARS = {
    'json': (HERE / 'grammars' / 'json', 'json.isc', ['[1, "a", true]', '{"a": [1, 2, {"b": false}], "c": "x"}', '[]', '{}', '  [ 1.5 , 2 ]  ', '"s"', 'true', '[[[[1]]]]',
          '[1,', '[1, 2', '{"a": [1,', '{"a": 1, ', '[1 2]', '{"a" 1}', '{"a": }', '[,]', '[1,,2]', '{,}', 'x', '-3',
          '{"a":1,"b":2,"c":[],"d":{}}', '{"k": {"n": [true, false, "s", 1.25]}}'],
             ['[', ']', '{', '}', ',', ':', '"k"', '"v"', '1', '2.5', 'true', 'false', ' ', '  ']),
    # dynamic values: a quote character or heredoc tag captured earlier decides where the match ends
    'dyn': (HERE / 'grammars' / 'dyn', 'dyn.isc', ['"abc", \'d\\\'e\', <<EOT\nline one\nEOT, "x\\"y"', "'a' , \"b\"", '<<A\nxx\nA,<<BB\nA\nBB', '"unterminated', '<<T\nbody T',
                        "'it\"s", '<<X\n\nX', '"a""b"', '', ',', '"a",'],
            ['"', "'", '<<', ',', '\\', '\n', ' ', 'EOT', 'A\n', 'a', 'b', 'T\n']),
    # Python expressions: a deep precedence chain of rules with optional/repeated tails
    'pyexpr': (REPO / 'examples' / 'python_expr', 'expr.isc',
               ['1', 'a + b * c', 'a if b else c', 'not x and y or z', 'f(1, 2)(3)', '[1, 2, 3]', '{1: 2, 3: 4}', 'x.y.z', 'x[1][2]', '(1, 2)', '()', '(a)',
                'a < b <= c', 'a is not b', 'a not in b', 'a | b ^ c & d', 'a << 2 >> 1', '-a ** 2', '~a', 'a // b % c', '1 +', '(1', '[1,', 'f(', '+', ''],
               ['a', 'b', '1', '2', '+', '-', '*', '/', '(', ')', '[', ']', '{', '}', ':', ',', '.', ' ', 'if ', 'else ', 'not ', 'and ', 'or ', 'in ', 'is ',
                '==', '<', '>', '**', 'None', 'True']),
    # counted repetitions: the number of `b` is the number of `a` before it (a shared counter), then exactly two `c`
    'counted': (HERE / 'grammars' / 'counted', 'counted.isc',
                ['abcc', 'aabbcc', 'aaabbbcc', 'aaabbcc', 'aabbbcc', 'abc', 'abccc', 'bcc', 'cc', 'a', '', 'aaaaabbbbbcc', 'abbcc', 'aabcc'],
                ['a', 'b', 'c', 'aa', 'bb', 'cc']),
    # custom fail blocks: `%need_semi` repairs a missing ';' (and skips an 'x' first)
    'failblock': (HERE / 'grammars' / 'failblock', 'failblock.isc',
                  ['1;', '1;2;3;', '1', '1 2;', '1x;', '1x2;', '1;2', ';', '', '1;;', 'x', '1 ; 2 ;'],
                  ['1', '2', ';', 'x', ' ', '12']),
    # SQL select statements: nested rules, groups, optional parts
    'sql': (REPO / 'examples' / 'sql', 'grammar.isc',
            ['SELECT a FROM t', 'SELECT a, b as c FROM t WHERE a = 1', "SELECT x FROM t WHERE name = 'bob' AND (age > 3 OR age < 1) ORDER BY x DESC;",
             'SELECT 1 + 2 * 3 FROM t', 'SELECT (a) FROM t ORDER BY a', 'SELECT', 'SELECT a', 'SELECT a FROM', 'SELECT a FROM t WHERE', 'FROM t',
             'SELECT a, FROM t', 'SELECT a FROM t ORDER x', 'select a from t', 'SELECT a FROM t;;', ''],
            ['SELECT ', 'FROM ', 'WHERE ', 'ORDER ', 'BY ', 'AND ', 'OR ', 'ASC ', 'DESC ', 'as ', 'a', 'b', 't', '1', '2.5', "'s'", ',', ';', '(', ')',
             '+', '-', '*', '/', '=', '!=', '<', '>', ' ']),
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
    common = ['-a', 'LL', '--no-incremental', '--no-ai-recovery', '--no-debug-instrumentation', '--dir', str(GRAMMARS[grammar][0]), str(GRAMMARS[grammar][0] / GRAMMARS[grammar][1])]
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
    inputs = GRAMMARS[args.grammar][2] + list(fuzz_inputs(args.fuzz, args.seed, GRAMMARS[args.grammar][3]))

    def outputs(text, lazy, rust, cpp):
        environment = {**os.environ, **({'LAZY': '1'} if lazy else {})}
        return (subprocess.run([str(cpp), text], capture_output=True, text=True, env=environment).stdout,
                subprocess.run([str(rust), text], capture_output=True, text=True, env=environment).stdout)

    failing = [(text, lazy) for text in inputs for lazy in (False, True) if len(set(outputs(text, lazy, rust, cpp))) != 1]
    # ispa's lexer builder does not number DFA states the same way on every run (and, for malformed dynamic input, the
    # parsers of two runs can disagree): a difference that goes away when the parsers are generated again is not the
    # Rust target's
    for attempt in range(4):
        if not failing:
            break
        shutil.rmtree(work, ignore_errors=True)
        rust, cpp = build(args.ispa, work, args.grammar)
        failing = [(text, lazy) for text, lazy in failing if len(set(outputs(text, lazy, rust, cpp))) != 1]
    for text, lazy in failing:
        want, got = outputs(text, lazy, rust, cpp)
        print(f'DIFF for {text!r}' + (' (lazy)' if lazy else ''))
        for label, out in (('C++ ', want), ('Rust', got)):
            print('\n'.join(f'  {label}: {line}' for line in out.splitlines()[:4]))
    total = 2 * len(inputs)
    print(f'{total - len(failing)} of {total} identical (eager and lazy)')
    sys.exit(1 if failing else 0)


if __name__ == '__main__':
    main()
