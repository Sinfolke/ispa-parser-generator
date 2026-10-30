"""Build one grammar twice (the C++ ispastdlib, and `--stdlib core`) for parity checks.

build_cpp(NAME, ENTRY, SRCDIR, stdlib=False, algorithm='LL') returns the path of a binary that parses argv[1] (LAZY=1: lazily) and
prints the AST dump / diagnostics. Binaries and generated sources go under ./build/. Sources are regenerated on each build;
ISPA=<executable> selects the generator, CXX the compiler."""
import json, os, subprocess, sys, shlex
from cmake_build import build_cpp as cmake_build_cpp
HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, '..', '..'))
BUILD = os.path.join(HERE, 'build')
ISPA = os.environ.get('ISPA') or os.path.join(REPO, 'cmake-build-release', 'ispa')
FACADE = REPO + '/parser/stdlibs/target/target-cpp'
ENV = {k: v for k, v in os.environ.items() if k != 'LD_LIBRARY_PATH'}

def build_cpp(name, entry, srcdir, stdlib=False, algorithm='LL'):
    d = os.path.join(BUILD, ('cppcore_' if stdlib else 'cpp_') + name); os.makedirs(d, exist_ok=True)
    r = subprocess.run([ISPA, '-l', 'cpp', '-a', algorithm, '--no-incremental', '--no-ai-recovery',
                        '--stdlib', FACADE if stdlib else 'ispastdlib', *shlex.split(os.environ.get('SUITE_GEN', '')), '--dir', *srcdir.split(':'), entry, '-o', os.path.join(d, name)], env=ENV, capture_output=True, text=True)
    if r.returncode: sys.exit('cpp generation failed:\n' + r.stdout[-1500:] + r.stderr[-500:])
    main = open(HERE + '/cppref/main.cpp').read().replace('"json.h"', f'"{name}.h"').replace('json::', name + '::')
    open(os.path.join(d, 'main.cpp'), 'w').write(main)
    for f in ('dump.hpp',): open(os.path.join(d, f), 'w').write(open(HERE + '/cppref/' + f).read())
    return str(cmake_build_cpp(d, 'ref',
        [f'{d}/{name}.cpp', f'{d}/main.cpp', *shlex.split(os.environ.get('SUITE_SRC', ''))],
        [d, *shlex.split(os.environ.get('SUITE_INC', ''))], core=stdlib,
        definitions=['ISPA_COUNT_CONSTRUCT'] if os.environ.get('COUNT') else []))
