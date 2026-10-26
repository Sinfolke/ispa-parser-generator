"""Small, dependency-free language server for ISPA grammar files."""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from urllib.parse import unquote, urlparse


DECLARATION = re.compile(r"(?m)^\s*([A-Za-z_][\w]*)\s*(?:\([^\n;]*\))?\s*:")
EARLY_ERROR = re.compile(r"(?m)^(error|warning|info)\[([^]]+)\]:\s*(.*)$")
EARLY_LOCATION = re.compile(r"(?m)^\s*-->\s*(.*):(\d+):(\d+)\s*$")


def path_from_uri(uri):
    parsed = urlparse(uri)
    path = unquote(parsed.path)
    if os.name == "nt" and re.match(r"^/[A-Za-z]:", path):
        path = path[1:]
    return Path(path)


def utf16_length(value):
    return len(value.encode("utf-16-le")) // 2


def position_at(text, offset):
    offset = max(0, min(offset, len(text)))
    prefix = text[:offset]
    return {"line": prefix.count("\n"), "character": utf16_length(prefix.rsplit("\n", 1)[-1])}


def offset_at(text, position):
    lines = text.splitlines(keepends=True)
    line = min(max(position.get("line", 0), 0), len(lines))
    start = sum(map(len, lines[:line]))
    content = lines[line] if line < len(lines) else ""
    units = 0
    for index, char in enumerate(content):
        next_units = units + utf16_length(char)
        if next_units > position.get("character", 0):
            return start + index
        units = next_units
    return start + len(content.rstrip("\r\n"))


def declarations(text):
    return [(match.group(1), match.start(1), match.end(1)) for match in DECLARATION.finditer(text)]


def word_at(text, offset):
    for match in re.finditer(r"[A-Za-z_][\w]*", text):
        if match.start() <= offset <= match.end():
            return match.group(), match.start(), match.end()
    return None


def diagnostic_range(text, offset, length):
    start = position_at(text, offset)
    end = position_at(text, offset + max(length, 1))
    return {"start": start, "end": end}


def compiler_diagnostics(text, path, compiler, timeout):
    if not compiler:
        return [{"range": diagnostic_range(text, 0, 1), "severity": 2, "source": "ispa-lsp",
                 "message": "ISPA compiler not found; configure --ispa or ISPA_COMPILER"}]
    # Keep the snapshot next to the source so relative grammar imports resolve.
    # The unique name prevents two open documents from overwriting each other.
    snapshot = None
    try:
        with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", suffix=".isc",
                                     prefix=".ispa-lsp-", dir=path.parent,
                                     delete=False) as stream:
            snapshot = Path(stream.name)
            stream.write(text)
        command = [compiler, "--check-only", "-l", "cpp", "-d", str(path.parent), str(snapshot)]
        result = subprocess.run(command, capture_output=True, text=True,
                                timeout=timeout, cwd=path.parent)
        output = result.stdout + "\n" + result.stderr
        payload = None
        for line in result.stdout.splitlines():
            try:
                candidate = json.loads(line)
            except json.JSONDecodeError:
                continue
            if isinstance(candidate, dict) and "diagnostics" in candidate:
                payload = candidate
        found = []
        if payload is not None:
            for entry in payload["diagnostics"]:
                source_file = entry.get("file", "")
                if source_file and Path(source_file).resolve() != snapshot.resolve():
                    continue
                byte_offset = entry.get("offset", -1)
                if byte_offset >= 0:
                    start = len(text.encode("utf-8")[:byte_offset].decode("utf-8", "ignore"))
                    end = len(text.encode("utf-8")[:byte_offset + max(entry.get("length", 1), 1)].decode("utf-8", "ignore"))
                    span = diagnostic_range(text, start, end - start)
                else:
                    line_no = max(entry.get("line", 1) - 1, 0)
                    column = max(entry.get("column", 1) - 1, 0)
                    pos = {"line": line_no, "character": column}
                    span = {"start": pos, "end": {"line": line_no, "character": column + 1}}
                found.append({"range": span, "severity": {"error": 1, "warning": 2, "info": 3}.get(entry.get("severity"), 1),
                              "code": entry.get("code"), "source": "ispa", "message": entry.get("message", "")})
        if found or payload is not None:
            return found
        # Parse errors occur before the compiler reaches its JSON path.
        errors = list(EARLY_ERROR.finditer(output))
        locations = list(EARLY_LOCATION.finditer(output))
        for index, match in enumerate(errors):
            location = locations[index] if index < len(locations) else None
            if location and Path(location.group(1)).resolve() != snapshot.resolve():
                continue
            line_no = max(int(location.group(2)) - 1, 0) if location else 0
            column = max(int(location.group(3)) - 1, 0) if location else 0
            position = {"line": line_no, "character": column}
            found.append({"range": {"start": position, "end": {"line": line_no, "character": column + 1}},
                          "severity": {"error": 1, "warning": 2, "info": 3}[match.group(1)],
                          "code": match.group(2), "source": "ispa", "message": match.group(3)})
        return list({(item["range"]["start"]["line"], item["range"]["start"]["character"],
                      item.get("code"), item["message"]): item for item in found}.values())
    except (OSError, subprocess.TimeoutExpired) as error:
        return [{"range": diagnostic_range(text, 0, 1), "severity": 2, "source": "ispa-lsp",
                 "message": f"Compiler check failed: {error}"}]
    finally:
        if snapshot is not None:
            snapshot.unlink(missing_ok=True)


class Server:
    def __init__(self, compiler, timeout):
        self.compiler = compiler
        self.timeout = timeout
        self.documents = {}
        self.shutdown = False

    def send(self, message):
        encoded = json.dumps(message, separators=(",", ":"), ensure_ascii=False).encode("utf-8")
        sys.stdout.buffer.write(f"Content-Length: {len(encoded)}\r\n\r\n".encode("ascii") + encoded)
        sys.stdout.buffer.flush()

    def publish(self, uri):
        if uri not in self.documents:
            return
        document = self.documents[uri]
        diagnostics = compiler_diagnostics(document["text"], path_from_uri(uri), self.compiler, self.timeout)
        self.send({"jsonrpc": "2.0", "method": "textDocument/publishDiagnostics",
                   "params": {"uri": uri, "version": document["version"], "diagnostics": diagnostics}})

    def handle(self, message):
        method = message.get("method")
        params = message.get("params") or {}
        request_id = message.get("id")
        result = None
        if method == "initialize":
            result = {"capabilities": {"textDocumentSync": {"openClose": True, "change": 2, "save": {"includeText": True}},
                                       "documentSymbolProvider": True, "definitionProvider": True,
                                       "completionProvider": {"triggerCharacters": ["@"]}},
                      "serverInfo": {"name": "ispa-lsp", "version": "0.1.0"}}
        elif method == "shutdown":
            self.shutdown = True
        elif method == "exit":
            return False
        elif method == "textDocument/didOpen":
            doc = params["textDocument"]
            self.documents[doc["uri"]] = {"text": doc["text"], "version": doc.get("version")}
            self.publish(doc["uri"])
        elif method == "textDocument/didChange":
            uri = params["textDocument"]["uri"]
            if uri in self.documents:
                text = self.documents[uri]["text"]
                for change in params.get("contentChanges", []):
                    if "range" in change:
                        span = change["range"]
                        start, end = offset_at(text, span["start"]), offset_at(text, span["end"])
                        text = text[:start] + change["text"] + text[end:]
                    else:
                        text = change["text"]
                self.documents[uri] = {"text": text, "version": params["textDocument"].get("version")}
                self.publish(uri)
        elif method == "textDocument/didSave":
            uri = params["textDocument"]["uri"]
            if uri in self.documents:
                if "text" in params:
                    self.documents[uri]["text"] = params["text"]
                self.publish(uri)
        elif method == "textDocument/didClose":
            uri = params["textDocument"]["uri"]
            self.documents.pop(uri, None)
            self.send({"jsonrpc": "2.0", "method": "textDocument/publishDiagnostics",
                       "params": {"uri": uri, "diagnostics": []}})
        elif method in ("textDocument/documentSymbol", "textDocument/completion", "textDocument/definition"):
            uri = params["textDocument"]["uri"]
            text = self.documents.get(uri, {}).get("text", "")
            symbols = declarations(text)
            if method == "textDocument/documentSymbol":
                result = [{"name": name, "kind": 13 if name.isupper() else 12,
                           "range": diagnostic_range(text, start, end - start),
                           "selectionRange": diagnostic_range(text, start, end - start)}
                          for name, start, end in symbols]
            elif method == "textDocument/completion":
                result = [{"label": name, "kind": 6 if name.isupper() else 3}
                          for name in sorted({item[0] for item in symbols})]
            else:
                current = word_at(text, offset_at(text, params["position"]))
                result = [{"uri": uri, "range": diagnostic_range(text, start, end - start)}
                          for name, start, end in symbols if current and name == current[0]][:1]
        if request_id is not None:
            self.send({"jsonrpc": "2.0", "id": request_id, "result": result})
        return True

    def run(self):
        while True:
            headers = {}
            while line := sys.stdin.buffer.readline():
                if line in (b"\r\n", b"\n"):
                    break
                key, _, value = line.partition(b":")
                headers[key.strip().lower()] = value.strip()
            if not headers:
                return
            try:
                length = int(headers[b"content-length"])
                message = json.loads(sys.stdin.buffer.read(length))
                if self.handle(message) is False:
                    return
            except (KeyError, ValueError, json.JSONDecodeError) as error:
                print(f"ispa-lsp: invalid message: {error}", file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(description="ISPA grammar language server (stdio)")
    parser.add_argument("--ispa", default=os.environ.get("ISPA_COMPILER", "ispa"), help="path to the ISPA compiler")
    parser.add_argument("--timeout", type=float, default=5.0, help="compiler timeout in seconds")
    args = parser.parse_args()
    compiler = shutil.which(args.ispa)
    if compiler is not None:
        compiler = str(Path(compiler).resolve())
    if compiler is None:
        print(f"ispa-lsp: compiler not found: {args.ispa}", file=sys.stderr)
    Server(compiler, args.timeout).run()


if __name__ == "__main__":
    main()
