import io
import json
from pathlib import Path
import subprocess
import tempfile
import sys
import unittest
from contextlib import redirect_stdout, redirect_stderr
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "cli/pkg"))

from ispa_pkg.cli import (PackageError, catalog, generator_version, install, main,
                          parse_source, resolved_source)


class GeneratorVersionTests(unittest.TestCase):
    def test_version_uses_selected_generator(self):
        with patch.dict('os.environ', {'ISPA': '/selected/ispa'}), \
                patch('ispa_pkg.cli.subprocess.run', return_value=subprocess.CompletedProcess(
                    [], 0, 'v1.2.3;\nGenerator description\n')) as run:
            self.assertEqual(generator_version(required=True), 'v1.2.3')
            self.assertEqual(run.call_args.args[0], ['/selected/ispa', '--version'])

    def test_missing_generator_fails_doctor(self):
        with patch.dict('os.environ', {'ISPA': ''}), \
                patch('ispa_pkg.cli.shutil.which', return_value=None), redirect_stderr(io.StringIO()):
            self.assertEqual(main(['doctor']), 1)

    def test_download_tag_and_explicit_sources(self):
        spec = {'github': 'owner/repo', 'ref': 'libraries-{generator}-latest'}
        self.assertEqual(resolved_source(spec, 'v1')['ref'], 'libraries-v1-latest')
        self.assertEqual(resolved_source(spec, 'dev')['ref'], 'main')
        self.assertEqual(spec['ref'], 'libraries-{generator}-latest')
        self.assertEqual(parse_source('owner/repo')['ref'], 'main')
        with self.assertRaises(PackageError):
            resolved_source(spec, 'unknown')

    def test_generator_upgrade_reinstalls_unchanged_sources_and_doctor_is_idempotent(self):
        with tempfile.TemporaryDirectory() as temp, \
                patch('ispa_pkg.cli.generator_version', return_value='v1') as version, \
                redirect_stdout(io.StringIO()):
            base = Path(temp)
            source = base / 'source'
            (source / 'ispalib/grammar').mkdir(parents=True)
            (source / 'ispalib/grammar/x.isc').write_text('original')
            root = base / 'installed'
            install('example', {'path': str(source)}, root)
            marker = root / 'example/.ispa-package.json'
            first = json.loads(marker.read_text())
            version.return_value = 'v2'
            self.assertEqual(main(['--root', str(root), 'doctor']), 0)
            second = json.loads(marker.read_text())
            self.assertEqual(second['generator_version'], 'v2')
            self.assertNotEqual(first['fingerprint'], second['fingerprint'])
            with patch('ispa_pkg.cli.install') as reinstall:
                self.assertEqual(main(['--root', str(root), 'doctor']), 0)
                reinstall.assert_not_called()
            # Legacy markers are repaired too.
            second.pop('generator_version')
            marker.write_text(json.dumps(second))
            self.assertEqual(main(['--root', str(root), 'doctor']), 0)
            self.assertEqual(json.loads(marker.read_text())['generator_version'], 'v2')

    def test_catalog_download_changes_with_generator(self):
        with tempfile.TemporaryDirectory() as temp, \
                patch('ispa_pkg.cli.generator_version', return_value='v1') as version, \
                redirect_stdout(io.StringIO()):
            base = Path(temp)
            source = base / 'source'
            (source / 'ispalib/grammar').mkdir(parents=True)
            (source / 'ispalib/grammar/x.isc').write_text('x')
            root = base / 'installed'
            spec = {'github': 'owner/repo', 'ref': 'libraries-{generator}-latest'}
            with patch('ispa_pkg.cli.source_tree', return_value=(source, 'same-commit')) as fetch:
                install('example', spec, root, from_catalog=True)
                self.assertEqual(fetch.call_args.args[0]['ref'], 'libraries-v1-latest')
                version.return_value = 'v2'
                self.assertEqual(main(['--root', str(root), 'doctor']), 0)
                self.assertEqual(fetch.call_args.args[0]['ref'], 'libraries-v2-latest')
            data = json.loads((root / 'example/.ispa-package.json').read_text())
            self.assertEqual(data['source'], spec)
            self.assertEqual(data['resolved_source']['ref'], 'libraries-v2-latest')

    def test_doctor_repairs_dependencies_before_build(self):
        with tempfile.TemporaryDirectory() as temp, \
                patch('ispa_pkg.cli.generator_version', return_value='v1') as version, \
                redirect_stdout(io.StringIO()):
            base = Path(temp)
            root = base / 'installed'
            for name in ('a-consumer', 'z-dependency'):
                source = base / name
                (source / 'ispalib/grammar').mkdir(parents=True)
                (source / 'ispalib/grammar/x.isc').write_text('x')
            (base / 'a-consumer/ispa-package.yaml').write_text('requires: [z-dependency]\n')
            install('z-dependency', {'path': str(base / 'z-dependency')}, root)
            install('a-consumer', {'path': str(base / 'a-consumer')}, root)
            version.return_value = 'v2'
            built = []
            def build(package, name, root):
                built.append(name)
                if name == 'a-consumer':
                    data = json.loads((root / 'z-dependency/.ispa-package.json').read_text())
                    self.assertEqual(data['generator_version'], 'v2')
            with patch('ispa_pkg.cli.build_cpp_library', side_effect=build):
                self.assertEqual(main(['--root', str(root), 'doctor']), 0)
            self.assertEqual(built, ['z-dependency', 'a-consumer'])

    def test_failed_reinstall_keeps_old_package(self):
        with tempfile.TemporaryDirectory() as temp, \
                patch('ispa_pkg.cli.generator_version', return_value='v1') as version, \
                redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
            base = Path(temp)
            source = base / 'source'
            (source / 'ispalib/grammar').mkdir(parents=True)
            (source / 'ispalib/grammar/x.isc').write_text('x')
            root = base / 'installed'
            install('example', {'path': str(source)}, root)
            marker = root / 'example/.ispa-package.json'
            old = marker.read_bytes()
            version.return_value = 'v2'
            with patch('ispa_pkg.cli.build_cpp_library', side_effect=PackageError('build failed')):
                self.assertEqual(main(['--root', str(root), 'doctor']), 1)
            self.assertEqual(marker.read_bytes(), old)

    def test_cached_official_catalog_migrates_but_custom_catalog_retains_main(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            cached = root / 'catalog.yaml'
            cached.write_text('packages:\n  std:\n    github: Sinfolke/ispa-parser-generator\n    ref: main\n')
            self.assertEqual(catalog(root)['packages']['std']['ref'], 'libraries-{generator}-latest')
            self.assertEqual(catalog(root, cached)['packages']['std']['ref'], 'main')

    def test_explicit_catalog_ref_survives_doctor_and_update(self):
        with tempfile.TemporaryDirectory() as temp, \
                patch('ispa_pkg.cli.generator_version', return_value='v1') as version, \
                redirect_stdout(io.StringIO()):
            base = Path(temp)
            source = base / 'source'
            (source / 'ispalib/grammar').mkdir(parents=True)
            (source / 'ispalib/grammar/x.isc').write_text('x')
            root = base / 'installed'
            with patch('ispa_pkg.cli.source_tree', return_value=(source, 'same-commit')) as fetch:
                self.assertEqual(main(['--root', str(root), 'install', 'std', '--ref', 'pinned']), 0)
                version.return_value = 'v2'
                self.assertEqual(main(['--root', str(root), 'doctor']), 0)
                self.assertEqual(fetch.call_args.args[0]['ref'], 'pinned')
                self.assertEqual(main(['--root', str(root), 'update']), 0)
                self.assertEqual(fetch.call_args.args[0]['ref'], 'pinned')

    def test_install_rebuilds_for_new_generator_without_force(self):
        with tempfile.TemporaryDirectory() as temp, \
                patch('ispa_pkg.cli.generator_version', return_value='v1') as version, \
                redirect_stdout(io.StringIO()):
            base = Path(temp)
            source = base / 'source'
            (source / 'ispalib/grammar').mkdir(parents=True)
            (source / 'ispalib/grammar/x.isc').write_text('x')
            root = base / 'installed'
            spec = {'path': str(source)}
            install('example', spec, root)
            version.return_value = 'v2'
            with patch('ispa_pkg.cli.build_cpp_library') as build:
                install('example', spec, root)
                self.assertEqual(build.call_count, 1)
            self.assertEqual(json.loads((root / 'example/.ispa-package.json').read_text())['generator_version'], 'v2')
