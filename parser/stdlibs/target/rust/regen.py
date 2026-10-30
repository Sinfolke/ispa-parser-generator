"""Regenerate src/lowered.rs: the Python runtime (target-core) lowered to Rust by ispa itself.

    python3 regen.py [--ispa PATH_TO_ISPA]

lib.rs includes the result at the crate root as `python::ispa_std::*`; the typed facade in src/ is the native residue."""
import argparse, os, re, shutil, subprocess, sys, tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
LIBRARY = HERE.parents[1] / 'python' / 'ispa_std'
MODULES = ('parser_ll', 'lr_conflicts', 'debug', 'external', 'optional', 'map', 'span', 'vector', 'algorithm',
           'ast_printer', 'to_string')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--ispa', default=os.environ.get('ISPA') or shutil.which('ispa'))
    args = parser.parse_args()
    if not args.ispa:
        sys.exit('regen.py: ispa executable not found (use --ispa)')
    env = {key: value for key, value in os.environ.items() if key != 'LD_LIBRARY_PATH'}
    with tempfile.TemporaryDirectory(prefix='ispa-rust-core-') as temp:
        work = Path(temp)
        entry = work / 'c.isc'
        entry.write_text(''.join(f'import py ispa_std.{name};\n' for name in MODULES) + "main: 'x';\n")
        subprocess.run([args.ispa, '-l', 'rust', '-a', 'LL', '--no-incremental', '--no-debug-instrumentation', '--library-only',
                        '--dir', str(LIBRARY), '--dir', str(work), str(entry), '-o', str(work / 'o')],
                       check=True, env=env)
        source = (work / 'o.rs').read_text()
    (HERE / 'src' / 'lowered.rs').write_text(source)
    print('regenerated src/lowered.rs')


if __name__ == '__main__':
    main()
