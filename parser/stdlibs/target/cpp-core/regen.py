"""Regenerate ispa_core_lowered.{h,cpp}: the Python runtime (target-core) lowered to C++ by ispa itself.

    python3 regen.py [--ispa PATH_TO_ISPA]

The facade in ispa_core/ is the native residue; everything else the core does comes from this lowering."""
import argparse, os, shutil, subprocess, sys, tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
LIBRARY = HERE.parents[1] / 'python' / 'ispa_std' / 'out'
MODULES = ('parser_ll', 'lr_conflicts', 'debug', 'external', 'optional', 'map', 'span', 'vector', 'algorithm',
           'ast_printer', 'to_string')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--ispa', default=os.environ.get('ISPA') or shutil.which('ispa'))
    args = parser.parse_args()
    if not args.ispa:
        sys.exit('regen.py: ispa executable not found (use --ispa)')
    env = {key: value for key, value in os.environ.items() if key != 'LD_LIBRARY_PATH'}
    with tempfile.TemporaryDirectory(prefix='ispa-core-') as temp:
        work = Path(temp)
        entry = work / 'c.isc'
        entry.write_text(''.join(f'import py ispa_std.{name};\n' for name in MODULES) + "main: 'x';\n")
        subprocess.run([args.ispa, '-l', 'cpp', '-a', 'LL', '--no-incremental', '--no-debug-instrumentation',
                        '--dir', str(LIBRARY), '--dir', str(work), str(entry), '-o', str(work / 'o')],
                       check=True, env=env)
        header = (work / 'o.h').read_text().replace('"o.h"', '"ispa_core_lowered.h"')
        source = (work / 'o.cpp').read_text().replace('"o.h"', '"ispa_core_lowered.h"')
    (HERE / 'ispa_core_lowered.h').write_text(header)
    (HERE / 'src' / 'ispa_core_lowered.cpp').write_text(source)
    print('regenerated ispa_core_lowered.h and src/ispa_core_lowered.cpp')


if __name__ == '__main__':
    main()
