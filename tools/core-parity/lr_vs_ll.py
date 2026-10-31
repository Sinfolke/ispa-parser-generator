"""lr_vs_ll.py: an LR (LALR) parser must behave like the LL parser of the same grammar for what LR supports:
Python statements in advanced rules, `match ... -> name` bindings and fail blocks (`%handler` markers).

    python3 lr_vs_ll.py     (needs the C++ toolchain; builds under ./build)

Compared: AST dump and diagnostics for inputs the fail blocks repair. Not compared: inputs LL repairs automatically
(LR has no generic recovery) and trailing input (LL's eager parse tolerates it, LR requires the end of input)."""
import os, subprocess, sys
import suite
H = os.path.dirname(os.path.abspath(__file__))
CASES = {
    'failblock': ['1;', '1;2;3;', '1', '1 2;', '1x;', '1x2;', '1;2', '1 ; 2 ;'],
    'lraction': ['1+2;', '1+2;3+4;', '1+2;3+4;5+6;'],
}
def run(binary, text, lazy):
    env = dict(os.environ); env.pop('LAZY', None)
    if lazy: env['LAZY'] = '1'
    return subprocess.run([binary, text], env=env, capture_output=True, text=True).stdout
bad = total = 0
for name, corpus in CASES.items():
    path = f'grammars/{name}/{name}.isc'
    ll = suite.build_cpp('ll_' + name, path, f'grammars/{name}', stdlib=True, algorithm='LL')
    lr = suite.build_cpp('lr_' + name, path, f'grammars/{name}', stdlib=True, algorithm='LALR')
    for text in corpus:
        for lazy in (0, 1):
            total += 1
            a, b = run(ll, text, lazy), run(lr, text, lazy)
            if a != b:
                bad += 1; print('DIFF', name, repr(text), 'lazy' if lazy else 'eager'); print(' LL', a[:300]); print(' LR', b[:300])
print(f'{total - bad} of {total} identical')
sys.exit(1 if bad else 0)
