"""Regenerate src/lowered.rs: the Python runtime (target-core) lowered to Rust by ispa itself.

    python3 regen.py [--ispa PATH_TO_ISPA]

lib.rs includes the result at the crate root as `python::ispa_std::*`; the typed facade in src/ is the native residue."""
import argparse, os, re, shutil, subprocess, sys, tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
LIBRARY = HERE.parents[1] / 'python' / 'ispa_std'
MODULES = ('parser_ll', 'lr_conflicts', 'debug', 'external', 'optional', 'map', 'span', 'vector', 'algorithm',
           'ast_printer', 'to_string')


def native_substr(source):
    """`string.substr` builds its result one unit at a time (`out = out + s[i]`): a clone of the growing String per unit.
    The runtime crate has the same function over bytes (`pycoll::substr_units`); the lowered one delegates to it."""
    start = source.index('pub fn substr(s: &String, mut pos: i64, mut count: i64) -> String {')
    end = source.index('pub fn compare(', start)
    return (source[:start] + 'pub fn substr(s: &String, pos: i64, count: i64) -> String { ::ispa_std::pycoll::substr_units(s, pos, count) }\n\t\t\t'
            + source[end:])


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
    source = native_substr(source)
    (HERE / 'src' / 'lowered.rs').write_text(source)
    print('regenerated src/lowered.rs')


if __name__ == '__main__':
    main()
