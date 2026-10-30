"""Build generated C++ and its runtime together, with one CMake toolchain/ABI."""
import os
from pathlib import Path
import shlex
import subprocess

RUNTIME = Path(__file__).resolve().parents[2] / 'parser/stdlibs/target/target-cpp'


def quote(value):
    return '"' + str(value).replace('\\', '/').replace('"', '\\"').replace(';', '\\;') + '"'


def build_cpp(work, name, sources, includes=(), core=True, definitions=()):
    work = Path(work).resolve()
    project = work / ('cmake-' + name)
    project.mkdir(parents=True, exist_ok=True)
    lines = ['cmake_minimum_required(VERSION 3.22)', 'project(ispa_reference LANGUAGES CXX)',
             f'add_subdirectory({quote(RUNTIME)} runtime)', f'add_executable({name}',
             *(quote(Path(source).resolve()) for source in sources), ')',
             f'target_compile_features({name} PRIVATE cxx_std_23)',
             f'target_link_libraries({name} PRIVATE ' + ('ispa_core' if core else 'ispastdlib') + ')',
             f'target_include_directories({name} PRIVATE',
             *(quote(Path(include).resolve()) for include in includes), ')']
    if definitions:
        lines.append(f'target_compile_definitions({name} PRIVATE ' + ' '.join(definitions) + ')')
    (project / 'CMakeLists.txt').write_text('\n'.join(lines) + '\n')
    configure = ['cmake', '-S', str(project), '-B', str(project / 'build'), '-DCMAKE_BUILD_TYPE=Release']
    compiler = shlex.split(os.environ.get('CXX', ''))
    if compiler:
        configure += ['-DCMAKE_CXX_COMPILER=' + compiler[0]]
        if len(compiler) > 1:
            configure += ['-DCMAKE_CXX_FLAGS=' + shlex.join(compiler[1:] + shlex.split(os.environ.get('CXXFLAGS', '')))]
    for command in (configure, ['cmake', '--build', str(project / 'build'), '--config', 'Release',
                                '--target', name, '--parallel', os.environ.get('ISPA_BENCH_JOBS', '2')]):
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError('CMake reference build failed:\n' + (result.stdout + result.stderr)[-6000:])
    binary = name + ('.exe' if os.name == 'nt' else '')
    for candidate in (project / 'build' / binary, project / 'build/Release' / binary):
        if candidate.is_file():
            return candidate
    raise RuntimeError(f'CMake did not produce {binary}')
