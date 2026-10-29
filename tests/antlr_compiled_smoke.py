"""Ensure every dynamically loaded parser survives Nuitka onefile packaging."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
from test_antlr_to_ispa import SAMPLES
from test_antlr_companions import SOURCES

executable = str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory() as temporary:
    path = Path(temporary) / 'Smoke.g4'
    for target, action in SAMPLES.items():
        path.write_text('grammar Smoke; options { language=' + target + '; }\nmain: ID {' + action + '}; ID: [a-z]+;', encoding='utf-8')
        subprocess.run([executable, str(path)], check=True)
        report = json.loads(path.with_suffix('.migration.json').read_text())
        assert report['actions'][0]['integrated'], (target, report)
    for filename, source in SOURCES.items():
        directory = Path(temporary) / Path(filename).suffix[1:]
        directory.mkdir()
        helper = directory / filename
        helper.write_text(source, encoding='utf-8')
        grammar = directory / 'Companion.g4'
        grammar.write_text("grammar Companion; main: 'x';", encoding='utf-8')
        subprocess.run([executable, str(grammar), '--code', str(helper)], check=True)
        report = json.loads(grammar.with_suffix('.migration.json').read_text())
        assert report['companions'][0]['declarations'][0]['converted'], (filename, report)
