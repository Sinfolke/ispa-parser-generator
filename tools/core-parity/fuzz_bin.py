"""fuzz_bin.py [N]: mutate the ispa grammar files and compare the two Parser binaries (build_ispa.py orig / core)."""
import glob,random,re,subprocess,sys,os
H=os.path.join(os.path.dirname(os.path.abspath(__file__)), 'build')
A,B=H+'/cpp_Parser/ref',H+'/cppcore_Parser/ref'
N=int(sys.argv[1]) if len(sys.argv)>1 else 400
seeds=[]
for f in sorted(glob.glob(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'parser', 'ispa', 'grammar', '*.isc'))):
    t=re.sub(r'(?m)^\s*(module|import) .*\n','',open(f).read()).lstrip('\n'); seeds.append(t)
rnd=random.Random(77)
junk=list(',;()[]{}:+-*/<>=!"\'.#@|?& ')+['@{','{@}','#[','->','<<','$']
def mut(s):
    s=list(s)
    for _ in range(rnd.choice([1,1,2,3])):
        if not s:break
        k=rnd.randrange(len(s));op=rnd.choice(['del','ins','rep','trunc','dupl'])
        if op=='del':del s[k]
        elif op=='ins':s[k:k]=list(rnd.choice(junk))
        elif op=='rep':s[k]=rnd.choice(junk)[0]
        elif op=='trunc':s=s[:k]
        else:s[k:k]=s[max(0,k-5):k]
    return ''.join(s)
def run(b,s):
    try:
        r=subprocess.run([b,s],env=dict(os.environ,LAZY='1'),capture_output=True,text=True,timeout=30)
        return r.returncode,r.stdout,r.stderr
    except subprocess.TimeoutExpired: return 'timeout',
same=diff=errs=0
for i in range(N):
    s=mut(rnd.choice(seeds))
    a,b=run(A,s),run(B,s)
    if 'ERR' in a[1] or 'exception' in a[1]: errs+=1
    if a==b: same+=1
    else:
        diff+=1
        if diff<=3:
            print('DIFF',repr(s[:150]));
            la,lb=str(a).splitlines(),str(b).splitlines()
            print(' A:',str(a)[:400]);print(' B:',str(b)[:400])
print(f'identical={same} diff={diff} of {N}; cases with diagnostics/exception={errs}')
