import json
import subprocess
import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "cli" / "pkg"))
from ispa_pkg.lsp import offset_at, position_at  # noqa: E402


def frame(message):
    body = json.dumps(message).encode()
    return f"Content-Length: {len(body)}\r\n\r\n".encode() + body


def responses(output):
    result = []
    while output:
        header, output = output.split(b"\r\n\r\n", 1)
        length = int(header.split(b":", 1)[1].strip())
        result.append(json.loads(output[:length]))
        output = output[length:]
    return result


class LspTest(unittest.TestCase):
    def test_utf16_positions(self):
        text = "😀\nmain: @ TOKEN;"
        self.assertEqual(position_at(text, 1), {"line": 0, "character": 2})
        self.assertEqual(offset_at(text, {"line": 1, "character": 4}), 6)

    def test_stdio_open_change_and_navigation(self):
        uri = (ROOT / "tests" / "sample.isc").as_uri()
        messages = [
            {"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {}},
            {"jsonrpc": "2.0", "method": "textDocument/didOpen", "params": {"textDocument": {
                "uri": uri, "version": 1, "text": "main: @ TOKEN;\nTOKEN: @ 'a';\n"}}},
            {"jsonrpc": "2.0", "id": 2, "method": "textDocument/definition", "params": {
                "textDocument": {"uri": uri}, "position": {"line": 0, "character": 8}}},
            {"jsonrpc": "2.0", "method": "textDocument/didChange", "params": {
                "textDocument": {"uri": uri, "version": 2}, "contentChanges": [{
                    "range": {"start": {"line": 1, "character": 0}, "end": {"line": 1, "character": 5}},
                    "text": "OTHER"}]}},
            {"jsonrpc": "2.0", "id": 3, "method": "textDocument/documentSymbol", "params": {
                "textDocument": {"uri": uri}}},
            {"jsonrpc": "2.0", "id": 4, "method": "shutdown"},
            {"jsonrpc": "2.0", "method": "exit"},
        ]
        process = subprocess.run([sys.executable, "-m", "ispa_pkg.lsp", "--ispa", "nonexistent-ispa"],
                                 input=b"".join(map(frame, messages)), capture_output=True,
                                 cwd=ROOT, env={"PYTHONPATH": str(ROOT / "cli" / "pkg")}, timeout=10)
        self.assertEqual(process.returncode, 0, process.stderr.decode())
        output = responses(process.stdout)
        self.assertEqual(output[2]["result"][0]["range"]["start"], {"line": 1, "character": 0})
        self.assertEqual([entry["name"] for entry in output[4]["result"]], ["main", "OTHER"])


if __name__ == "__main__":
    unittest.main()
