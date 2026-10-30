#!/usr/bin/env python3
"""
ispa_deps.py - scan the ISPA std library headers and write a dependency configuration.

For every header under <root>/ispastdlib/ (plus <root>/ispastdlib.hpp) it records
  * includes     direct project #includes            (system ones are listed separately)
  * closure      transitive project includes
  * defines      namespace-scope symbols the file defines / forward declares
  * uses         symbols from OTHER library files that the file's code names
  * depends_on   the files those symbols live in
and a global symbol table  qualified name -> defining file.

Output
  deps.json   the configuration (schema version 1)
  deps.inc    optional: the same JSON as C++ raw string literal(s), for `#include` in LibraryDeps.hpp
  --check     fail (exit 1) when a file uses a symbol whose file is not reachable through its includes

Backend: a self-contained C++ lexer + namespace-scope declaration scanner (no third-party packages).
It is deliberately small: it understands comments, strings, preprocessor lines, namespaces, templates, classes,
enums, aliases, variables and functions at namespace scope, and treats class / function bodies as opaque token
ranges that are only scanned for names. The scanner is isolated in `scan_file()`; a tree-sitter backend can replace
it as long as it returns the same `FileScan`.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

SCHEMA_VERSION = 1
KEYWORDS_BEFORE_PAREN = {"decltype", "noexcept", "alignas", "sizeof", "alignof", "if", "while", "for", "switch",
                         "return", "static_assert", "requires", "typeof"}
CLASS_KEYS = {"class", "struct", "union"}
PUNCT2 = {"::", "->", "...", "<<", ">>", "==", "!=", "<=", ">=", "&&", "||", "+=", "-=", "*=", "/=", "++", "--"}
# `<<` / `>>` are split back into single characters: angle brackets in templates must count one by one.
SPLIT = {"<<": ["<", "<"], ">>": [">", ">"]}


# ------------------------------------------------------------------------------------------------ lexer
@dataclass
class Tok:
    kind: str   # id | num | p
    text: str


@dataclass
class FileScan:
    path: str
    includes: list[str] = field(default_factory=list)          # every #include, as written
    macros: set[str] = field(default_factory=set)
    defines: dict[str, str] = field(default_factory=dict)       # qualified name -> kind (class/enum/alias/function/...)
    declares: dict[str, str] = field(default_factory=dict)      # forward declarations only
    chains: list[tuple[tuple[str, ...], tuple[str, ...]]] = field(default_factory=list)  # (namespace, chain) per use
    macro_uses: set[str] = field(default_factory=set)


def strip_and_lex(text: str, fs: FileScan, project_root: Path) -> list[Tok]:
    """Removes comments / literals / preprocessor lines (recording #include and #define), returns tokens."""
    # join continued lines first so multi-line #defines are one logical line
    text = text.replace("\\\r\n", " ").replace("\\\n", " ")
    out: list[Tok] = []
    i, n = 0, len(text)
    at_line_start = True
    while i < n:
        c = text[i]
        if c == "\n":
            at_line_start = True
            i += 1
        elif c in " \t\r":
            i += 1
        elif text.startswith("//", i):
            while i < n and text[i] != "\n":
                i += 1
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            i = n if j < 0 else j + 2
        elif c == "#" and at_line_start:
            j = i
            while j < n and text[j] != "\n":
                if text.startswith("/*", j):          # comment inside a directive
                    k = text.find("*/", j + 2)
                    j = n if k < 0 else k + 2
                elif text.startswith("//", j):
                    while j < n and text[j] != "\n":
                        j += 1
                else:
                    j += 1
            line = text[i + 1:j].strip()
            m = re.match(r'include\s*([<"])([^>"]+)[>"]', line)
            if m:
                fs.includes.append(m.group(2))      # project vs system is decided in analyse() by file existence
            m = re.match(r'define\s+([A-Za-z_]\w*)', line)
            if m:
                fs.macros.add(m.group(1))
            i = j
        else:
            at_line_start = False
            if c == '"' or c == "'":
                q = c
                i += 1
                while i < n and text[i] != q:
                    i += 2 if text[i] == "\\" else 1
                i += 1
                out.append(Tok("num", q))                 # literal placeholder (never a name)
            elif c.isalpha() or c == "_":
                j = i
                while j < n and (text[j].isalnum() or text[j] == "_"):
                    j += 1
                out.append(Tok("id", text[i:j]))
                i = j
            elif c.isdigit():
                j = i
                while j < n and (text[j].isalnum() or text[j] in "._'"):
                    j += 1
                out.append(Tok("num", text[i:j]))
                i = j
            else:
                three, two = text[i:i + 3], text[i:i + 2]
                if three == "...":
                    out.append(Tok("p", "..."))
                    i += 3
                elif two in SPLIT:
                    for ch in SPLIT[two]:
                        out.append(Tok("p", ch))
                    i += 2
                elif two in PUNCT2:
                    out.append(Tok("p", two))
                    i += 2
                else:
                    out.append(Tok("p", c))
                    i += 1
    return out


# ------------------------------------------------------------------------------------------------ scanner
class Scanner:
    def __init__(self, toks: list[Tok], fs: FileScan):
        self.t = toks
        self.fs = fs
        self.ns: list[str] = []
        self.pops: list[int] = []                             # group sizes: how many names each open `{` added
        self.tok_ns: list[tuple[str, ...]] = [()] * len(toks)

    # helpers -----------------------------------------------------------------------------------
    def is_(self, i: int, text: str) -> bool:
        return i < len(self.t) and self.t[i].kind != "num" and self.t[i].text == text

    def mark(self, a: int, b: int) -> None:
        cur = tuple(self.ns)
        for k in range(a, min(b, len(self.t))):
            self.tok_ns[k] = cur

    def skip_balanced(self, i: int, open_: str, close: str) -> int:
        """i points at `open_`; returns index after the matching `close`."""
        depth = 0
        while i < len(self.t):
            if self.is_(i, open_):
                depth += 1
            elif self.is_(i, close):
                depth -= 1
                if depth == 0:
                    return i + 1
            i += 1
        return i

    def skip_template_head(self, i: int) -> int:
        """i at `template`; returns index after the closing `>` of its parameter list."""
        i += 1
        if not self.is_(i, "<"):
            return i
        angle = paren = 0
        while i < len(self.t):
            if self.is_(i, "("):
                paren += 1
            elif self.is_(i, ")"):
                paren -= 1
            elif paren == 0 and self.is_(i, "<"):
                angle += 1
            elif paren == 0 and self.is_(i, ">") and not self.is_(i - 1, "-"):
                angle -= 1
                if angle == 0:
                    return i + 1
            i += 1
        return i

    def qual(self, name: str) -> str:
        return "::".join(self.ns + [name])

    def define(self, name: str, kind: str) -> None:
        self.fs.defines.setdefault(self.qual(name), kind)

    def declare(self, name: str, kind: str) -> None:
        self.fs.declares.setdefault(self.qual(name), kind)

    # main walk ---------------------------------------------------------------------------------
    def run(self) -> None:
        i, n = 0, len(self.t)
        while i < n:
            start = i
            if self.is_(i, "}"):                              # closes `namespace A::B {` (pops both) / extern "C" {
                if self.pops:
                    self.mark(i, i + 1)
                    for _ in range(self.pops.pop()):
                        self.ns.pop()
                i += 1
            elif self.is_(i, ";"):
                i += 1
            elif self.is_(i, "namespace"):
                i = self.namespace(i)
            elif self.is_(i, "extern") and i + 2 < n and self.t[i + 1].kind == "num" and self.is_(i + 2, "{"):
                self.mark(i, i + 3)                           # extern "C" { ... } is transparent
                self.pops.append(0)
                i += 3
            elif self.is_(i, "using") and self.is_(i + 1, "namespace"):
                while i < n and not self.is_(i, ";"):
                    i += 1
                self.mark(start, i + 1)
                i += 1
            else:
                i = self.declaration(i)
            if i == start:                                    # safety: always make progress
                i += 1

    def namespace(self, i: int) -> int:
        start = i
        i += 1
        names: list[str] = []
        while i < len(self.t) and not self.is_(i, "{") and not self.is_(i, ";") and not self.is_(i, "="):
            if self.t[i].kind == "id" and self.t[i].text not in ("inline",):
                names.append(self.t[i].text)
            i += 1
        if self.is_(i, "{"):
            self.mark(start, i + 1)
            if not names:
                names = ["(anonymous)"]
            self.ns.extend(names)                             # one `}` later pops all of them
            self.pops.append(len(names))
            return i + 1
        while i < len(self.t) and not self.is_(i, ";"):   # namespace alias
            i += 1
        self.mark(start, i + 1)
        return i + 1

    def declaration(self, i: int) -> int:
        t, n = self.t, len(self.t)
        start = i
        is_template = False
        if self.is_(i, "template") and self.is_(i + 1, "<"):
            is_template = True
            i = self.skip_template_head(i)
        head_start = i

        # statement-like things that define nothing
        if self.is_(i, "static_assert") or self.is_(i, "typedef"):
            while i < n and not self.is_(i, ";"):
                i += 1
            self.mark(start, i + 1)
            return i + 1
        if self.is_(i, "using"):
            return self.using(start, i)

        # collect the head up to `;` or `{` at paren/bracket depth 0
        paren = 0
        j = i
        while j < n:
            if self.is_(j, "(") or self.is_(j, "["):
                paren += 1
            elif self.is_(j, ")") or self.is_(j, "]"):
                paren -= 1
            elif paren == 0 and (self.is_(j, ";") or self.is_(j, "{")):
                break
            j += 1
        head = t[head_start:j]
        term = t[j].text if j < n else ";"

        kind = self.classify(head, term)
        end = j
        if term == "{":
            body_end = self.skip_balanced(j, "{", "}")
            end = body_end
            if kind[0] in ("class", "enum", "var"):        # `struct X { ... } x;`  /  `T x{...};`
                while end < n and not self.is_(end, ";"):
                    end += 1
                end += 1
        else:
            end = j + 1
        if kind[0] != "none":
            if kind[0] == "forward":
                self.declare(kind[1], kind[2])
            else:
                self.define(kind[1], kind[2])
        self.mark(start, end)
        return end

    def using(self, start: int, i: int) -> int:
        t, n = self.t, len(self.t)
        j = i + 1
        # `using Name = ...;`
        if j + 1 < n and t[j].kind == "id" and self.is_(j + 1, "="):
            self.define(t[j].text, "alias")
        k = i
        while k < n and not self.is_(k, ";"):
            k += 1
        self.mark(start, k + 1)
        return k + 1

    def classify(self, head: list[Tok], term: str) -> tuple[str, str, str]:
        """-> (what, name, kind); what in none | forward | def"""
        if not head:
            return ("none", "", "")
        txt = [h.text if h.kind != "num" else "<lit>" for h in head]
        # attributes / specifiers in front
        k = 0
        while k < len(txt) and txt[k] in ("inline", "constexpr", "static", "extern", "consteval", "constinit",
                                          "typename", "friend"):
            k += 1
        if k < len(txt) and txt[k] == "enum":
            k += 1
            if k < len(txt) and txt[k] in ("class", "struct"):
                k += 1
            if k < len(txt) and head[k].kind == "id":
                return ("def", txt[k], "enum")
            return ("none", "", "")
        if k < len(txt) and txt[k] in CLASS_KEYS:
            k += 1
            while k < len(txt) and txt[k] in ("alignas",):
                k += 1
            if k < len(txt) and head[k].kind == "id":
                name = txt[k]
                nxt = txt[k + 1] if k + 1 < len(txt) else ""
                if nxt == "<":                              # partial / explicit specialization: no new symbol
                    return ("none", "", "")
                if term == ";":
                    return ("forward", name, "class")
                return ("def", name, "class")
            return ("none", "", "")
        # operator functions: `operator<<`, `operator()`, ...
        for idx, w in enumerate(txt):
            if w == "operator" and head[idx].kind == "id":
                m = idx + 1
                op = ""
                if m + 1 < len(txt) and txt[m] == "(" and txt[m + 1] == ")":
                    op, m = "()", m + 2
                else:
                    while m < len(txt) and txt[m] != "(":
                        op += txt[m]
                        m += 1
                if idx >= 1 and txt[idx - 1] == "::":
                    return ("none", "", "")                 # out-of-class member definition
                return ("def", "operator" + op, "function")
        depth = 0
        for idx, w in enumerate(txt):
            if w in ("(", "["):
                if depth == 0 and w == "(":
                    if idx == 0:
                        return ("none", "", "")
                    prev = idx - 1
                    if txt[prev] in KEYWORDS_BEFORE_PAREN or head[prev].kind != "id":
                        # skip e.g. `decltype(...)`: continue past the group
                        pass
                    else:
                        if prev >= 1 and txt[prev - 1] == "::":
                            return ("none", "", "")         # out-of-class member definition
                        return ("def", txt[prev], "function")
                depth += 1
            elif w in (")", "]"):
                depth -= 1
            elif w == "=" and depth == 0:
                if idx >= 1 and head[idx - 1].kind == "id":
                    return ("def", txt[idx - 1], "variable")
                return ("none", "", "")
        # `T x;` / `T x{...}` - a plain variable declaration
        if term in (";", "{") and len(head) >= 2 and head[-1].kind == "id" and head[-1].text not in CLASS_KEYS:
            return ("def", txt[-1], "variable")
        return ("none", "", "")


def collect_chains(toks: list[Tok], tok_ns: list[tuple[str, ...]], fs: FileScan) -> None:
    """Every `a::b::c` name in the file with the namespace it appears in. Member accesses are skipped."""
    n = len(toks)
    i = 0
    while i < n:
        t = toks[i]
        if t.kind == "id":
            prev = toks[i - 1] if i else None
            prev_text = prev.text if prev and prev.kind != "num" else ""
            after_scope = prev_text == "::"
            if prev_text in (".", "->"):
                i += 1
                continue
            if after_scope:                                   # continuation of a chain handled at its head
                # a leading `::name` (global) starts a chain too
                before = toks[i - 2] if i >= 2 else None
                if before is not None and (before.kind == "id" or before.text == ">"):
                    i += 1
                    continue
            chain = [t.text]
            j = i + 1
            while j + 1 < n and toks[j].kind != "num" and toks[j].text == "::" and toks[j + 1].kind == "id":
                chain.append(toks[j + 1].text)
                j += 2
            if t.text in fs.macros:
                fs.macro_uses.add(t.text)
            fs.chains.append((tok_ns[i], tuple(chain)))
            i = j
        else:
            i += 1


def scan_file(path: Path, rel: str, project_root: Path) -> FileScan:
    fs = FileScan(path=rel)
    toks = strip_and_lex(path.read_text(encoding="utf-8"), fs, project_root)
    sc = Scanner(toks, fs)
    sc.run()
    collect_chains(toks, sc.tok_ns, fs)
    return fs


# ------------------------------------------------------------------------------------------------ analysis
def analyse(root: Path, namespace: str) -> dict:
    files = sorted(root.glob("ispastdlib/**/*.hpp")) + [root / "ispastdlib.hpp"]
    files = [f for f in files if f.exists()]
    rels = {f.relative_to(root).as_posix() for f in files}
    scans = {f.relative_to(root).as_posix(): scan_file(f, f.relative_to(root).as_posix(), root) for f in files}

    # includes: project vs system
    direct: dict[str, list[str]] = {}
    system: dict[str, list[str]] = {}
    for rel, fs in scans.items():
        proj, sysl = [], []
        for inc in dict.fromkeys(fs.includes):
            (proj if inc in rels else sysl).append(inc)
        direct[rel] = [p for p in proj if p != rel]
        system[rel] = sysl

    def closure(rel: str) -> list[str]:
        seen: dict[str, None] = {}
        stack = list(direct[rel])
        while stack:
            cur = stack.pop()
            if cur in seen:
                continue
            seen[cur] = None
            stack.extend(direct[cur])
        return sorted(seen)

    # symbol table: definitions win over forward declarations
    symbols: dict[str, tuple[str, str]] = {}
    declared_only: dict[str, tuple[str, str]] = {}
    macro_owner: dict[str, str] = {}
    for rel, fs in sorted(scans.items()):
        if rel == "ispastdlib.hpp":
            continue                                        # the umbrella defines nothing itself
        for name, kind in fs.defines.items():
            symbols.setdefault(name, (rel, kind))
        for name, kind in fs.declares.items():
            declared_only.setdefault(name, (rel, kind))
        for m in fs.macros:
            macro_owner.setdefault(m, rel)
    for name, (rel, kind) in declared_only.items():
        symbols.setdefault(name, (rel, kind))
    for m, rel in macro_owner.items():
        symbols.setdefault(m, (rel, "macro"))
    # operators are overloaded across files and are only ever found through ADL: nothing can name them in text
    symbols = {n: v for n, v in symbols.items() if not n.split("::")[-1].startswith("operator")}
    by_last: dict[str, list[str]] = {}
    for name in symbols:
        by_last.setdefault(name.split("::")[-1], []).append(name)

    def resolve(ns: tuple[str, ...], chain: tuple[str, ...]) -> str | None:
        if chain[0] == "std":
            return None
        if chain[-1] not in by_last:
            return None
        for length in range(len(ns), -1, -1):               # innermost scope first, like C++ lookup
            full = "::".join(list(ns[:length]) + list(chain))
            if full in symbols:
                return full
        return None

    result_files = []
    problems: list[str] = []
    for rel in sorted(scans):
        fs = scans[rel]
        uses: dict[str, str] = {}
        for ns, chain in fs.chains:
            sym = resolve(ns, chain)
            # a name the file itself declares (e.g. a forward declaration) is not a dependency on its definition
            if sym and symbols[sym][0] != rel and sym not in fs.declares and sym not in fs.defines:
                uses[sym] = symbols[sym][0]
        for m in fs.macro_uses:
            if m in macro_owner and macro_owner[m] != rel:
                uses[m] = macro_owner[m]
        cl = closure(rel)
        for sym, owner in sorted(uses.items()):
            if owner not in cl and rel != "ispastdlib.hpp":
                problems.append(f"{rel}: uses {sym} (from {owner}) but does not include it (directly or transitively)")
        result_files.append({
            "path": rel,
            "includes": sorted(direct[rel]),
            "system_includes": sorted(system[rel]),
            "closure": cl,
            "uses": sorted(uses),
            "depends_on": sorted(set(uses.values())),
            "defines": sorted(n for n, (r, _) in symbols.items() if r == rel and n not in macro_owner),
        })
    return {
        "version": SCHEMA_VERSION,
        "default_namespace": namespace,
        "umbrella": "ispastdlib.hpp",
        "files": result_files,
        "symbols": [{"name": n, "kind": k, "file": r} for n, (r, k) in sorted(symbols.items())],
        "_problems": problems,
    }


# ------------------------------------------------------------------------------------------------ output
def dump_json(doc: dict) -> str:
    """Stable layout: one line per file / symbol (keeps the raw-string chunks of the .inc small and diffs readable)."""
    d = json.dumps
    lines = ["{", f'  "version": {doc["version"]},', f'  "default_namespace": {d(doc["default_namespace"])},',
             f'  "umbrella": {d(doc["umbrella"])},', '  "files": [']
    lines += ["    " + d(f, ensure_ascii=True) + ("," if i + 1 < len(doc["files"]) else "") for i, f in enumerate(doc["files"])]
    lines += ["  ],", '  "symbols": [']
    lines += ["    " + d(s, ensure_ascii=True) + ("," if i + 1 < len(doc["symbols"]) else "") for i, s in enumerate(doc["symbols"])]
    lines += ["  ]", "}", ""]
    return "\n".join(lines)


def dump_inc(json_text: str, limit: int = 12000) -> str:
    """JSON as adjacent raw string literals (MSVC rejects a single literal over ~16 KB)."""
    chunks: list[str] = []
    cur = ""
    for line in json_text.splitlines(keepends=True):
        if cur and len(cur) + len(line) > limit:
            chunks.append(cur)
            cur = ""
        cur += line
    if cur:
        chunks.append(cur)
    return "// generated by tools/ispa_deps.py - do not edit\n" + "\n".join(f'R"ispajson({c})ispajson"' for c in chunks) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--root", default=".", help="directory containing ispastdlib.hpp and ispastdlib/")
    ap.add_argument("--out", default="ispastdlib_deps.json")
    ap.add_argument("--emit-inc", default=None, help="also write the JSON as C++ raw string literal(s) to this path")
    ap.add_argument("--namespace", default="ISPA_STD", help="namespace unqualified converter names are tried in")
    ap.add_argument("--check", action="store_true", help="exit 1 if a file uses a symbol it does not include")
    args = ap.parse_args()

    root = Path(args.root).resolve()
    doc = analyse(root, args.namespace)
    problems = doc.pop("_problems")
    text = dump_json(doc)
    Path(args.out).write_text(text, encoding="utf-8")
    if args.emit_inc:
        Path(args.emit_inc).write_text(dump_inc(text), encoding="utf-8")
    print(f"{len(doc['files'])} files, {len(doc['symbols'])} symbols -> {args.out}")
    for p in problems:
        print("MISSING INCLUDE:", p, file=sys.stderr)
    return 1 if (args.check and problems) else 0


if __name__ == "__main__":
    sys.exit(main())
