"""Imports a generated parser and runs the user's Python script against it.

Executed inside Pyodide by worker.js. serialize.py is imported alongside it.
The script sees: grammar (the generated module), Parser, text, to_json.
It publishes its outcome by assigning to `result`; print() goes to the console.
"""
import importlib
import io
import json
import sys
import traceback
from contextlib import redirect_stderr, redirect_stdout

import serialize

DEFAULT_SCRIPT = '''\
# Runs after your grammar compiles.
# Available: Parser, grammar (the generated module), text (the test input), to_json.
# Assign what you want to see to `result`; print() output goes to the console.
tree = Parser().parse(text).data()
result = tree
'''


def run(module_name, text, script):
    sys.modules.pop(module_name, None)
    importlib.invalidate_caches()
    try:
        grammar = importlib.import_module(module_name)
    except Exception:
        return json.dumps({"ok": False, "stage": "import", "error": traceback.format_exc(limit=-6)})
    scope = {"__name__": "__playground__", "grammar": grammar, "Parser": grammar.Parser,
             "text": text, "to_json": serialize.to_json_value, "result": None}
    captured = io.StringIO()
    try:
        with redirect_stdout(captured), redirect_stderr(captured):
            exec(compile(script, "<script>", "exec"), scope)
        return json.dumps({"ok": True, "ast": serialize.to_json_value(scope["result"]), "printed": captured.getvalue()})
    except Exception as error:
        return json.dumps({"ok": False, "stage": "script", "error": str(error) or type(error).__name__,
                           "trace": traceback.format_exc(limit=-6), "printed": captured.getvalue()})
