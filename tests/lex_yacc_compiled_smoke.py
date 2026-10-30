"""Exercise grammar conversion in each compiled release executable."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
from test_lex_yacc_to_ispa import FLEX, BISON

kind, executable = sys.argv[1:]
with tempfile.TemporaryDirectory() as temp:
    source = Path(temp) / ('example.l' if kind == 'flex' else 'example.y')
    source.write_text(FLEX if kind == 'flex' else BISON)
    subprocess.run([str(Path(executable).resolve()), str(source)], check=True)
    assert 'NUMBER' in source.with_suffix('.isc').read_text()
    assert json.loads(source.with_suffix('.migration.json').read_text())['format'] == kind
