"""Real installs, generation and consumers: ISPA_PKG_INTEGRATION=1 ISPA=/path/to/ispa
python3 -m unittest discover -s tests -p test_ispa_pkg_exports_integration.py -v

Requires bash, CMake, a C++23 compiler, Cargo and rustc. CXX/CXXFLAGS select
the same toolchain for installation and the C++ consumer. No network is needed.
"""
import json
import os
from pathlib import Path
import shutil
import shlex
import subprocess
import sys
import tempfile
import unittest


REPO = Path(os.environ.get('ISPA_EXPORT_TEST_REPO', Path(__file__).resolve().parents[1]))
ENTRY = REPO / 'cli/pkg/ispa_pkg_entry.py'


@unittest.skipUnless(os.environ.get('ISPA_PKG_INTEGRATION') == '1',
                     'set ISPA_PKG_INTEGRATION=1 to install and compile the real runtimes')
class InstalledRuntimeExportTests(unittest.TestCase):
    @classmethod
    def run_command(cls, command, *, cwd=None, env=None):
        result = subprocess.run([str(arg) for arg in command], cwd=cwd or cls.work,
                                env=env if env is not None else cls.env,
                                capture_output=True, text=True, timeout=600)
        if result.returncode:
            raise AssertionError(f'{command!r} exited {result.returncode}:\n'
                                 f'{result.stdout}\n{result.stderr}')
        return result.stdout

    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix='ispa export integration ')
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.work = Path(cls.temporary.name)
        cls.root = cls.work / 'installed libraries'
        cls.env = dict(os.environ)
        # An existing permanent export must not make the tests pass accidentally.
        for key in list(cls.env):
            if key.startswith('ISPA_TARGET') or key.startswith('ISPA_CPP_') or key in (
                    'PYTHONPATH', 'CPATH', 'CPLUS_INCLUDE_PATH', 'LIBRARY_PATH',
                    'CMAKE_INCLUDE_PATH', 'CMAKE_LIBRARY_PATH', 'ISPA_PYTHON_INTERPRETER_DIRS'):
                cls.env.pop(key)
        generator = cls.env.get('ISPA') or shutil.which('ispa')
        if not generator:
            raise AssertionError('Set ISPA to the generator under test')
        cls.generator = str(Path(generator).resolve())
        cls.env.update(ISPA=cls.generator, ISPA_PKG_ROOT=str(cls.root), CARGO_NET_OFFLINE='true')
        packages = ('target-core', 'python-target', 'rust-target', 'target-cpp')
        catalog = cls.work / 'catalog.yaml'
        catalog.write_text('packages:\n' + ''.join(
            f'  {name}:\n    path: {json.dumps(str(REPO))}\n'
            f'    manifest: ispa-package.{name}.yaml\n' for name in packages))
        for package in packages:
            cls.run_command([sys.executable, ENTRY, '--root', cls.root,
                             '--catalog', catalog, 'install', package])
        # Exercise the public CLI and source exactly the shell code it prints.
        script = cls.run_command([sys.executable, ENTRY, '--root', cls.root,
                                  '--export', '--shell', 'bash'])
        exports = cls.work / 'exports.sh'
        exports.write_text(script)
        output = cls.run_command(['bash', '--noprofile', '--norc', '-c',
                                 '. "$1"; "$2" -c "import os,json; print(json.dumps(dict(os.environ)))"',
                                 'export-test', exports, sys.executable])
        cls.exported = json.loads(output)
        cls.manifest = json.loads(Path(cls.exported['ISPA_TARGETS_MANIFEST']).read_text())
        cls.grammar = cls.work / 'smoke.isc'
        cls.grammar.write_text("main: 'x';\n")

    def generate(self, language):
        work = self.work / language
        work.mkdir()
        self.run_command([self.generator, '-l', language, '-a', 'LL', '--no-incremental',
                          '--no-debug-instrumentation', '--dir', self.work, self.grammar,
                          '-o', work / 'generated'] +
                         (['--stdlib', 'core'] if language == 'cpp' else []),
                         env=self.exported)
        return work

    def test_python_generated_parser_imports_only_installed_runtime(self):
        work = self.generate('python')
        (work / 'consume.py').write_text('''import os
from pathlib import Path
import ispa_std
import generated
runtime = Path(ispa_std.__file__).resolve()
assert runtime.is_relative_to(Path(os.environ['ISPA_PKG_ROOT']))
parser = generated.Parser()
tree = parser.parse('x')
assert not tree.empty()
assert isinstance(tree.data(), generated.Types.main)
assert 'main' in ispa_std.format_ast(tree)
assert not parser.getErrors()
print('installed Python runtime OK')
''')
        result = self.run_command([sys.executable, work / 'consume.py'], cwd=work, env=self.exported)
        self.assertIn('installed Python runtime OK', result)
        self.assertIn(str(self.root / 'python-target/ispatarget'),
                      self.exported['PYTHONPATH'].split(os.pathsep))

    def test_cpp_generated_parser_links_installed_compiled_core_via_cmake_export(self):
        work = self.generate('cpp')
        libraries = self.exported['ISPA_CPP_CORE_LIBRARIES'].split(os.pathsep)
        self.assertTrue(libraries)
        for library in libraries:
            self.assertTrue(Path(library).is_file(), library)
            self.assertTrue(Path(library).is_relative_to(self.root))
        # Source-free linkage proves that the archive, not a rebuilt runtime, works.
        self.assertEqual(self.manifest['cpp']['sources'], [])
        (work / 'consume.cpp').write_text('''#include "generated.h"
#include <iostream>
int main() {
    generated::Parser parser;
    generated::Lexer lexer;
    lexer.setInput("x");
    lexer.makeTokens();
    auto& tree = parser.parse(lexer);
    if (!parser.getErrors().empty()) return 1;
    tree.print(std::cout);
}
''')
        (work / 'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.22)
project(export_consumer LANGUAGES CXX)
include("$ENV{ISPA_TARGETS_CMAKE}")
add_executable(consume consume.cpp generated.cpp)
target_compile_features(consume PRIVATE cxx_std_23)
target_link_libraries(consume PRIVATE ISPA::Targets)
''')
        self.run_command(['cmake', '-S', work, '-B', work / 'build'], env=self.exported)
        self.run_command(['cmake', '--build', work / 'build', '--parallel', '2'], env=self.exported)
        self.assertIn('main', self.run_command([work / 'build/consume'], env=self.exported))
        # Also verify the compiler/linker environment paths, without -I or -L.
        compiler = shlex.split(self.exported.get('CXX', 'c++'))
        flags = shlex.split(self.exported.get('CXXFLAGS', ''))
        self.run_command([*compiler, *flags, '-std=c++23', work / 'consume.cpp',
                          work / 'generated.cpp', '-lispa-core', '-o', work / 'direct'],
                         env=self.exported)
        self.assertIn('main', self.run_command([work / 'direct'], env=self.exported))

    def test_rust_generated_parser_links_installed_compiled_ispa_std(self):
        work = self.generate('rust')
        # Rust's export is the target directory. Use its already-built rlib;
        # rustc must not rebuild the runtime from the repository or Cargo sources.
        runtime, = self.exported['ISPA_TARGET_RUST_DIRS'].split(os.pathsep)
        runtime = Path(runtime)
        self.assertTrue(runtime.is_relative_to(self.root))
        self.assertEqual(self.manifest['targets']['rust'], [str(runtime)])
        library = runtime / 'target/release/libispa_std.rlib'
        self.assertTrue(library.is_file(), library)
        (work / 'consume.rs').write_text('''include!("generated.rs");
fn main() {
    let mut parser = generated::Parser::default();
    parser.parse_text("x");
    assert!(parser.diagnostics().is_empty());
    let mut tree = String::new();
    parser.tree.print(&mut tree);
    assert!(tree.contains("main"), "{tree}");
    println!("installed Rust runtime OK");
}
''')
        self.run_command(['rustc', '--edition=2021', '-Awarnings', work / 'consume.rs',
                          '--extern', f'ispa_std={library}', '-L',
                          f'dependency={library.parent / "deps"}', '-o', work / 'consume'],
                         env=self.exported)
        self.assertIn('installed Rust runtime OK', self.run_command([work / 'consume'], env=self.exported))


if __name__ == '__main__':
    unittest.main()
