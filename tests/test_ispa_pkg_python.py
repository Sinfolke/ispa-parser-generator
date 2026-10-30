import sys
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "cli" / "pkg"))
from ispa_pkg import cli


class PythonCommandTests(unittest.TestCase):
    def test_source_uses_current_interpreter(self):
        with patch.object(cli.sys, 'frozen', False, create=True):
            self.assertEqual(cli.python_command(), [sys.executable])

    def test_nuitka_uses_system_python(self):
        with patch.dict(cli.__dict__, {'__compiled__': object()}), patch.object(
                cli.shutil, 'which', return_value='/usr/bin/python3'):
            self.assertEqual(cli.python_command(), ['/usr/bin/python3'])

    def test_frozen_missing_python_reports_actionable_error(self):
        with patch.object(cli.sys, 'frozen', True, create=True), patch.object(
                cli.shutil, 'which', return_value=None):
            with self.assertRaisesRegex(cli.PackageError, 'Python 3 is required'):
                cli.python_command()
