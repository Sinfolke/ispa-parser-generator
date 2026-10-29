"""fuzz_dyn.py [N]: dynamic value matching ($var, !($var)*) on grammars/dyn/dyn.isc, original vs --stdlib core."""
import random,subprocess,os,sys
H=os.path.join(os.path.dirname(os.path.abspath(__file__)), 'build')
A,B=H+'/cpp_dyn/ref',H+'/cppcore_dyn/ref'
seeds=["\"abc\", 'd\\'e', <<EOT\nline one\nEOT, \"x\\\"y\"", "'a' , \"b\"", "<<A\nxx\nA,<<BB\nA\nBB", "\"unterminated", "<<T\nbody T", "'it\"s", "<<X\n\nX", "\"a\"\"b\""]
rnd=random.Random(5); junk=list('"\'<,\\\n ')+['<<','EOT','A\n']
def mut(s):
    s=list(s)
    for _ in range(rnd.choice([1,2,3])):
        if not s:break
        k=rnd.randrange(len(s));op=rnd.choice(['del','ins','rep','trunc','dupl'])
        if op=='del':del s[k]
        elif op=='ins':s[k:k]=list(rnd.choice(junk))
        elif op=='rep':s[k]=rnd.choice(junk)[0]
        elif op=='trunc':s=s[:k]
        else:s[k:k]=s[max(0,k-4):k]
    return ''.join(s)
def run(b,s,lazy):
    e=dict(os.environ); 
    if lazy: e['LAZY']='1'
    r=subprocess.run([b,s],env=e,capture_output=True,text=True,timeout=30); return r.returncode,r.stdout,r.stderr
cases=seeds+[mut(rnd.choice(seeds)) for _ in range(int(sys.argv[1]))]
same=diff=ok=0
for s in cases:
  for lazy in (0,1):
    a,b=run(A,s,lazy),run(B,s,lazy)
    if 'exception' not in a[1] and 'ERR' not in a[1]: ok+=1
    if a==b: same+=1
    else:
        diff+=1
        if diff<=4: print('DIFF lazy=%d'%lazy,repr(s)); print(' A',a[1][:300]); print(' B',b[1][:300])
print(f'identical={same} diff={diff} of {2*len(cases)} (clean parses {ok})')
for s in seeds[:3]:
    print(repr(s),'=>',run(A,s,1)[1][:240].replace('\n',' | '))
