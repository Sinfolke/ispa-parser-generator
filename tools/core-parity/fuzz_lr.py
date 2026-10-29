"""fuzz_lr.py [N]: LR (LALR) parsing of grammars/json/json.isc, original vs --stdlib core (the lowered LR driver).
Builds both first: python3 -c "import suite; suite.build_cpp('lrjson', 'grammars/json/json.isc', 'grammars/json', algorithm='LALR')" (+ stdlib=True)."""
import random, subprocess, os, sys
H = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'build')
A, B = H + '/cpp_lrjson/ref', H + '/cppcore_lrjson/ref'
seeds = ['[1, 2.5, "x", true, false]', '{"a": [1, {"b": null}], "c": "d"}', '[]', '{}', '[[[1],[2]],[3]]',
         '{"k": {"n": {"m": [1,2,3]}}}', '[1,', '{"a" 1}', '["unterminated', '[1 2]', '', '  [ 1 , 2 ]  ']
rnd = random.Random(11); junk = list('[]{},:"\' 0a') + ['true', 'null', '1.5', ',,']
def mut(s):
    s = list(s)
    for _ in range(rnd.choice([1, 2, 3])):
        if not s: break
        k = rnd.randrange(len(s)); op = rnd.choice(['del', 'ins', 'rep', 'trunc', 'dupl'])
        if op == 'del': del s[k]
        elif op == 'ins': s[k:k] = list(rnd.choice(junk))
        elif op == 'rep': s[k] = rnd.choice(junk)[0]
        elif op == 'trunc': s = s[:k]
        else: s[k:k] = s[max(0, k - 4):k]
    return ''.join(s)
def run(b, s, lazy):
    e = dict(os.environ)
    if lazy: e['LAZY'] = '1'
    r = subprocess.run([b, s], env=e, capture_output=True, text=True, timeout=30); return r.returncode, r.stdout, r.stderr
cases = seeds + [mut(rnd.choice(seeds)) for _ in range(int(sys.argv[1]))]
same = diff = ok = 0
for s in cases:
    for lazy in (0, 1):
        a, b = run(A, s, lazy), run(B, s, lazy)
        if 'exception' not in a[1] and 'ERR' not in a[1]: ok += 1
        if a == b: same += 1
        else:
            diff += 1
            if diff <= 4: print('DIFF lazy=%d' % lazy, repr(s)); print(' A', a[1][:300]); print(' B', b[1][:300])
print(f'identical={same} diff={diff} of {2 * len(cases)} (clean parses {ok})')
