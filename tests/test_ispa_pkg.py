import io
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tarfile
import tempfile
import unittest
import sys
import zipfile
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "cli" / "pkg"))
from ispa_pkg.cli import (PackageError, default_root, export_variables, install, main,
                          permanent_export, shell_exports, unpack, write_export_files)


class PackageTests(unittest.TestCase):
    @unittest.skipUnless(os.name != 'nt' and shutil.which('c++') and shutil.which('ar')
                         and shutil.which('bash'), 'requires a POSIX C++ toolchain')
    def test_install_builds_static_library_and_exports_it_for_linking(self):
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp).resolve()
            source = base / 'source'
            cpp = source / 'ispatarget' / 'cpp'
            (cpp / 'src').mkdir(parents=True)
            (cpp / 'example.hpp').write_text('int example_value();\n')
            (cpp / 'src' / 'example.cpp').write_text(
                '#include <example.hpp>\nint example_value() { return 42; }\n')
            root = base / 'libraries'
            install('example', {'path': str(source)}, root)
            archive = root / 'example' / 'ispatarget' / 'cpp' / 'lib' / 'libexample.a'
            self.assertTrue(archive.is_file())

            output, manifest = write_export_files(root)
            self.assertIn(str(archive), manifest['cpp']['libraries'])
            self.assertIn(str(archive), (output / 'targets.cmake').read_text())
            self.assertNotIn('target_sources(', (output / 'targets.cmake').read_text())
            variables = export_variables(root, output, manifest)
            script = output / 'env.sh'
            script.write_text(shell_exports(variables, 'bash', manifest))
            consumer = base / 'consumer.cpp'
            consumer.write_text('#include <example.hpp>\n'
                                'int main() { return example_value() == 42 ? 0 : 1; }\n')
            program = base / 'consumer'
            result = subprocess.run(
                ['bash', '-c', '. "$1"; c++ -std=c++17 "$2" -lexample -o "$3"',
                 'bash', str(script), str(consumer), str(program)],
                capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            subprocess.run([str(program)], check=True)

            archive.unlink()
            entry = Path(__file__).resolve().parents[1] / 'cli' / 'pkg' / 'ispa_pkg_entry.py'
            exported = subprocess.run(
                [sys.executable, str(entry), '--root', str(root), '--export', '--export-path'],
                capture_output=True, text=True, check=True)
            self.assertEqual(exported.stdout.strip(), str(script))
            self.assertIn('compiling src/example.cpp', exported.stderr)
            self.assertTrue(archive.is_file())

            (cpp / 'src' / 'example.cpp').write_text(
                '#include <example.hpp>\nint example_value() { return 43; }\n')
            install('example', {'path': str(source)}, root)
            consumer.write_text('#include <example.hpp>\n'
                                'int main() { return example_value() == 43 ? 0 : 1; }\n')
            result = subprocess.run(
                ['bash', '-c', '. "$1"; c++ -std=c++17 "$2" -lexample -o "$3"',
                 'bash', str(script), str(consumer), str(program)],
                capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            subprocess.run([str(program)], check=True)

    @unittest.skipIf(os.name == 'nt' or shutil.which('bash') is None,
                     'requires a POSIX shell')
    def test_exports_reach_compiler_linker_and_python_runtime(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp).resolve() / 'libraries'
            package = root / 'example'
            cpp = package / 'ispatarget' / 'cpp'
            python = package / 'ispatarget' / 'python'
            (cpp / 'include').mkdir(parents=True)
            (cpp / 'lib').mkdir()
            python.mkdir(parents=True)
            (package / '.ispa-package.json').write_text('{}')
            (cpp / 'include' / 'example.hpp').write_text('')
            (cpp / 'lib' / 'libexample.so').write_text('')
            (python / 'runtime.py').write_text('value = 7\n')
            (python.parent / 'ispa_std.py').write_text('value = 8\n')
            output, manifest = write_export_files(root)
            variables = export_variables(root, output, manifest)
            script = output / 'env.sh'
            script.write_text(shell_exports(variables, 'bash', manifest))
            loader = 'DYLD_LIBRARY_PATH' if sys.platform == 'darwin' else 'LD_LIBRARY_PATH'
            env = dict(os.environ, CPATH='', CPLUS_INCLUDE_PATH='/existing/include',
                       LIBRARY_PATH='/existing/lib', PYTHONPATH='/existing/python')
            env[loader] = '/existing/runtime'
            command = ['bash', '-c', '. "$1"; . "$1"; python3 -c "import json, os; print(json.dumps({key: os.environ.get(key) for key in (\'CPATH\', \'CPLUS_INCLUDE_PATH\', \'LIBRARY_PATH\', \'LD_LIBRARY_PATH\', \'DYLD_LIBRARY_PATH\', \'PYTHONPATH\')}))"',
                       'bash', str(script)]
            result = subprocess.run(command, env=env, capture_output=True, text=True, check=True)
            exported = json.loads(result.stdout)
            self.assertEqual(exported['CPLUS_INCLUDE_PATH'].split(os.pathsep),
                             [str(cpp), str(cpp / 'include'), '/existing/include'])
            self.assertEqual(exported['CPATH'].split(os.pathsep),
                             [str(cpp), str(cpp / 'include')])
            self.assertEqual(exported['LIBRARY_PATH'].split(os.pathsep),
                             [str(cpp / 'lib'), '/existing/lib'])
            self.assertEqual(exported[loader].split(os.pathsep),
                             [str(cpp / 'lib'), '/existing/runtime'])
            self.assertEqual(exported['PYTHONPATH'].split(os.pathsep),
                             [str(python.parent), str(python), '/existing/python'])
            self.assertIn(str(cpp / 'lib' / 'libexample.so'), (output / 'targets.cmake').read_text())

    def test_permanent_export_writes_same_environment_script(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / 'libraries'
            python = root / 'example' / 'ispatarget' / 'python'
            python.mkdir(parents=True)
            (python.parent.parent / '.ispa-package.json').write_text('{}')
            output, manifest = write_export_files(root)
            variables = export_variables(root, output, manifest)
            with patch('pathlib.Path.home', return_value=Path(temp)):
                profile, script = permanent_export(root, 'bash', variables, manifest)
            self.assertEqual(script.read_text(), shell_exports(variables, 'bash', manifest))
            self.assertIn(str(script), profile.read_text())

    @unittest.skipIf(os.name == 'nt' or shutil.which('bash') is None,
                     'requires a POSIX shell')
    def test_export_function_updates_current_shell_without_eval(self):
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp).resolve()
            root = base / 'library root'
            cpp = root / 'example' / 'ispatarget' / 'cpp'
            cpp.mkdir(parents=True)
            (cpp.parent.parent / '.ispa-package.json').write_text('{}')
            output, manifest = write_export_files(root)
            variables = export_variables(root, output, manifest)

            executable = base / 'bin' / 'ispa-pkg'
            executable.parent.mkdir()
            entry = Path(__file__).resolve().parents[1] / 'cli' / 'pkg' / 'ispa_pkg_entry.py'
            executable.write_text(f'#!/bin/sh\nexec {shlex.quote(sys.executable)} '
                                  f'{shlex.quote(str(entry))} "$@"\n')
            executable.chmod(0o755)
            env = dict(os.environ, PATH=str(executable.parent) + os.pathsep + os.environ['PATH'])
            for shell in ('bash', 'zsh'):
                if shutil.which(shell) is None:
                    continue
                with self.subTest(shell=shell), patch('pathlib.Path.home', return_value=base):
                    profile, _ = permanent_export(root, shell, variables, manifest)
                    options = ['--noprofile', '--norc'] if shell == 'bash' else ['-f']
                    result = subprocess.run(
                        [shell, *options, '-c',
                         '. "$1"; unset CPATH; ispa-pkg --root "$2" --export; printf "%s" "$CPATH"',
                         shell, str(profile), str(root)],
                        env=env, capture_output=True, text=True, check=True)
                    self.assertEqual(result.stdout, str(cpp))

    def test_installed_executable_selects_installation_library_root(self):
        with tempfile.TemporaryDirectory() as temp:
            executable = Path(temp) / "versions" / "v1" / "ispa-pkg"
            with patch.dict("os.environ", {"ISPA_PKG_ROOT": "", "ISPA_INSTALL_DIR": ""}), \
                    patch("sys.executable", str(executable)):
                self.assertEqual(default_root(), (Path(temp) / "libraries").resolve())

    def test_install_dir_override_selects_library_root(self):
        with tempfile.TemporaryDirectory() as temp:
            with patch.dict("os.environ", {"ISPA_PKG_ROOT": "", "ISPA_INSTALL_DIR": temp}):
                self.assertEqual(default_root(), Path(temp) / "libraries")

    def test_local_layout_and_revision(self):
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp)
            source = base / "source"
            (source / "release" / "grammar-renamed").mkdir(parents=True)
            (source / "release" / "grammar-renamed" / "hello.isc").write_text("first")
            (source / "ispa-package.yaml").write_text(
                "layout:\n  root: release\n  ispalib:\n    grammar: grammar-renamed\n"
            )
            root = base / "installed"
            install("example", {"path": str(source)}, root)
            result = root / "example" / "ispalib" / "grammar" / "hello.isc"
            self.assertEqual(result.read_text(), "first")
            revision = json.loads((root / "example" / ".ispa-package.json").read_text())["revision"]
            install("example", {"path": str(source)}, root)
            self.assertEqual(json.loads((root / "example" / ".ispa-package.json").read_text())["revision"], revision)
            (source / "release" / "grammar-renamed" / "hello.isc").write_text("second")
            install("example", {"path": str(source)}, root)
            self.assertEqual(result.read_text(), "second")

    def test_install_script_requires_opt_in(self):
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp)
            source = base / "source"
            (source / "ispalib" / "grammar").mkdir(parents=True)
            (source / "ispalib" / "grammar" / "x.isc").write_text("x")
            (source / "ispa-package.yaml").write_text("install: [build.py]\n")
            (source / "build.py").write_text("import pathlib, sys\n(pathlib.Path(sys.argv[1]) / 'built').write_text('yes')\n")
            with self.assertRaises(PackageError):
                install("example", {"path": str(source)}, base / "installed")
            install("example", {"path": str(source)}, base / "installed", allow_scripts=True)
            self.assertEqual((base / "installed" / "example" / "built").read_text(), "yes")

    def test_archive_rejects_traversal(self):
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp)
            buffer = io.BytesIO()
            with zipfile.ZipFile(buffer, "w") as archive:
                archive.writestr("../escape", "bad")
            with self.assertRaises(PackageError):
                unpack(buffer.getvalue(), "bad.zip", base / "zip")

            buffer = io.BytesIO()
            with tarfile.open(fileobj=buffer, mode="w:gz") as archive:
                info = tarfile.TarInfo("../escape")
                info.size = 3
                archive.addfile(info, io.BytesIO(b"bad"))
            with self.assertRaises(PackageError):
                unpack(buffer.getvalue(), "bad.tar.gz", base / "tar")

    def test_zip_archive_source(self):
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp)
            archive_path = base / "example.zip"
            with zipfile.ZipFile(archive_path, "w") as archive:
                archive.writestr("release/ispalib/grammar/module.isc", "module demo;")
            install("example", {"archive": str(archive_path)}, base / "installed")
            self.assertEqual(
                (base / "installed/example/ispalib/grammar/module.isc").read_text(),
                "module demo;",
            )

    def test_repository_config_can_select_nested_archive_and_file(self):
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp)
            source = base / "source"
            source.mkdir()
            with zipfile.ZipFile(source / "payload.zip", "w") as archive:
                archive.writestr("payload/grammar/module.isc", "module demo;")
                archive.writestr("payload/target/ispa_std.py", "name = 'std'\n")
            (source / "ispa-package.yaml").write_text(
                "archive: payload.zip\nlayout:\n  ispalib:\n    grammar: grammar\n"
                "  files:\n    ispatarget/ispa_std.py: target/ispa_std.py\n"
            )
            root = base / "installed"
            install("example", {"path": str(source)}, root)
            self.assertEqual((root / "example/ispatarget/ispa_std.py").read_text(), "name = 'std'\n")
            first = json.loads((root / "example/.ispa-package.json").read_text())["fingerprint"]
            install("example", {"path": str(source)}, root)
            self.assertEqual(json.loads((root / "example/.ispa-package.json").read_text())["fingerprint"], first)

    def test_update_keeps_direct_source_even_if_name_is_in_catalog(self):
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp)
            source = base / "source"
            (source / "ispalib/grammar").mkdir(parents=True)
            (source / "ispalib/grammar/x.isc").write_text("x")
            root = base / "installed"
            self.assertEqual(main(["--root", str(root), "install", str(source), "--name", "std"]), 0)
            self.assertEqual(main(["--root", str(root), "update", "std"]), 0)

    def test_github_commit_controls_update(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / "installed"
            commit = ["a" * 40]

            def fake_request(url):
                if "/commits/" in url:
                    return json.dumps({"sha": commit[0]}).encode()
                buffer = io.BytesIO()
                with tarfile.open(fileobj=buffer, mode="w:gz") as archive:
                    data = commit[0].encode()
                    info = tarfile.TarInfo("repo/ispalib/grammar/x.isc")
                    info.size = len(data)
                    archive.addfile(info, io.BytesIO(data))
                return buffer.getvalue()

            with patch("ispa_pkg.cli.request", side_effect=fake_request):
                spec = {"github": "owner/repo", "ref": "main"}
                install("example", spec, root)
                first = json.loads((root / "example/.ispa-package.json").read_text())["fingerprint"]
                install("example", spec, root)
                self.assertEqual(json.loads((root / "example/.ispa-package.json").read_text())["fingerprint"], first)
                commit[0] = "b" * 40
                install("example", spec, root)
                self.assertEqual((root / "example/ispalib/grammar/x.isc").read_text(), "b" * 40)


if __name__ == "__main__":
    unittest.main()
