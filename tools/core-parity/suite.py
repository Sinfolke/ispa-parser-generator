"""Build one grammar twice (the C++ ispastdlib, and `--stdlib core`) for parity checks.

build_cpp(NAME, ENTRY, SRCDIR, stdlib=False) returns the path of a binary that parses argv[1] (LAZY=1: lazily) and
prints the AST dump / diagnostics. Binaries and generated sources go under ./build/. REBUILD=1 forces a rebuild;
ISPA=<executable> selects the generator, CXX the compiler."""
import json, os, subprocess, sys
HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, '..', '..'))
BUILD = os.path.join(HERE, 'build')
ISPA = os.environ.get('ISPA') or os.path.join(REPO, 'cmake-build-release', 'ispa')
CORE = REPO + '/parser/stdlibs/target/cpp'
FACADE = CORE + '-core'
CXX = os.environ.get('CXX') or 'clang++'
ENV = {k: v for k, v in os.environ.items() if k != 'LD_LIBRARY_PATH'}

def build_cpp(name, entry, srcdir, stdlib=False):
    d = os.path.join(BUILD, ('cppcore_' if stdlib else 'cpp_') + name); os.makedirs(d, exist_ok=True)
    binary = os.path.join(d, 'ref')
    if os.path.exists(binary) and not os.environ.get('REBUILD'):
        return binary
    r = subprocess.run([ISPA, '-l', 'cpp', '-a', 'LL', '--no-incremental', '--no-ai-recovery',
                        *(['--stdlib', 'core'] if stdlib else []), *os.environ.get('SUITE_GEN', '').split(), '--dir', *srcdir.split(':'), entry, '-o', os.path.join(d, name)], env=ENV, capture_output=True, text=True)
    if r.returncode: sys.exit('cpp generation failed:\n' + r.stdout[-1500:] + r.stderr[-500:])
    main = open(HERE + '/cppref/main.cpp').read().replace('"json.h"', f'"{name}.h"').replace('json::', name + '::')
    open(os.path.join(d, 'main.cpp'), 'w').write(main)
    for f in ('dump.hpp',): open(os.path.join(d, f), 'w').write(open(HERE + '/cppref/' + f).read())
    extra = []
    if stdlib:
        lowered = FACADE
        obj = os.path.join(BUILD, 'ispa_core_lowered.o')
        source = lowered + '/src/ispa_core_lowered.cpp'
        if not os.path.exists(obj) or os.path.getmtime(obj) < os.path.getmtime(source):
            r = subprocess.run([CXX, '-std=c++23', '-O2', '-c', '-I' + CORE, '-I' + lowered, source, '-o', obj],
                               capture_output=True, text=True)
            if r.returncode: sys.exit('lowered core build failed:\n' + '\n'.join(l for l in r.stderr.splitlines() if 'error' in l)[:2500])
        extra = ['-I' + FACADE, obj]
    r = subprocess.run([CXX, '-std=c++23', '-O2', *(['-DISPA_COUNT_CONSTRUCT'] if os.environ.get('COUNT') else []), '-rdynamic', *extra, '-I' + CORE, f'{d}/{name}.cpp', f'{d}/main.cpp', CORE + '/src/core.cpp', *os.environ.get('SUITE_SRC', '').split(), *['-I' + i for i in os.environ.get('SUITE_INC', '').split()],
                        '-o', binary], capture_output=True, text=True)
    if r.returncode: sys.exit('cpp build failed:\n' + '\n'.join(l for l in r.stderr.splitlines() if 'error' in l)[:2500])
    return binary
