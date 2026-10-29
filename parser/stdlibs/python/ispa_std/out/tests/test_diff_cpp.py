"""Differential tests: the Python port against the ORIGINAL C++ headers.

Set ISPASTDLIB_ORIG to the directory that contains the original `ispastdlib/`
folder (the headers as supplied). Skipped when g++ or the headers are missing."""
import os
import random
import shutil
import subprocess

import pytest

from ispa_std import recovery
from ispa_std.dfa_api import Action, Captures, LEXER_DELETE_STATE, NULL_STATE, TdfaLayout
from ispa_std.dfa_run import DfaTables, run
from ispa_std.dfa_api import DebugIndex
from ispa_std.refs import Cursor
from ispa_std.scan import scan_balanced
from ispa_std.utility import Pair

HERE = os.path.dirname(os.path.abspath(__file__))
ORIG = os.environ.get("ISPASTDLIB_ORIG", "")
BIN = os.path.join(HERE, ".bin")

pytestmark = pytest.mark.skipif(
    not ORIG or not shutil.which("g++") or not os.path.isdir(os.path.join(ORIG, "ispastdlib")),
    reason="needs g++ and ISPASTDLIB_ORIG=<dir containing ispastdlib/>")


def build(name):
    os.makedirs(BIN, exist_ok=True)
    exe = os.path.join(BIN, name)
    src = os.path.join(HERE, "cpp_diff", name + ".cpp")
    if not os.path.exists(exe) or os.path.getmtime(exe) < os.path.getmtime(src):
        subprocess.run(["g++", "-std=c++17", "-O1", "-I", ORIG, src, "-o", exe], check=True)
    return exe


def run_exe(name, lines):
    out = subprocess.run([build(name)], input="\n".join(lines) + "\n", capture_output=True, text=True, check=True)
    return out.stdout.splitlines()


def hexs(s):
    return "".join("%02x" % ord(c) for c in s)


# --- scan_balanced ------------------------------------------------------------
def test_scan_balanced_matches_cpp():
    rnd = random.Random(1)
    alphabet = "()[]{}\"'#\\\n;a ,x"
    cases = []
    for _ in range(600):
        src = "".join(rnd.choice(alphabet) for _ in range(rnd.randint(0, 40)))
        begin = rnd.randint(0, len(src))
        budget = rnd.choice([0, 1, 3, 10, 4096])
        bounds = rnd.choice([["}"], [";"], [",", ")"], ["}", ";"]])
        cases.append((src, begin, budget, bounds))
    lines = ["%s %d %d %d %s" % (hexs(s), bu, b, len(bs), " ".join(hexs(x) for x in bs)) if s else
             "%s %d %d %d %s" % ("", bu, b, len(bs), " ".join(hexs(x) for x in bs)) for s, b, bu, bs in cases]
    # an empty source would break the token split; give it one placeholder unit
    fixed = []
    pycases = []
    for (s, b, bu, bs), line in zip(cases, lines):
        if not s:
            s = "a"
            line = "%s %d %d %d %s" % (hexs(s), bu, 0, len(bs), " ".join(hexs(x) for x in bs))
            b = 0
        fixed.append(line)
        pycases.append((s, b, bu, bs))
    expected = run_exe("scan_diff", fixed)
    for (s, b, bu, bs), exp in zip(pycases, expected):
        r = scan_balanced(s, b, bs, bu)
        assert "%d %d" % (r.end, 1 if r.exhausted else 0) == exp, (s, b, bu, bs)


# --- Recovery::recover --------------------------------------------------------
class PyTok:
    def __init__(self, kind, pos):
        from ispa_std.raw_token import RawToken, RawTokenMeta
        self.t = RawToken.make(RawTokenMeta(pos, "", False, 1, 1, 1, kind), 0)


class PyIt:
    def __init__(self, toks, i=0):
        self.toks, self.i = toks, i

    def isEnd(self):
        return self.i >= len(self.toks)

    def current(self):
        return self.toks[self.i]

    def next(self):
        self.i += 1
        return self

    def copy(self):
        return PyIt(self.toks, self.i)


def vec(v):
    return "%d %s" % (len(v), " ".join(str(x) for x in v))


def test_recover_matches_cpp():
    from ispa_std.raw_token import RawToken, RawTokenMeta
    rnd = random.Random(7)
    lines, cases = [], []
    for _ in range(1500):
        n = rnd.randint(0, 8)
        kinds = [rnd.randint(0, 5) for _ in range(n)]

        def ids():
            return [rnd.randint(0, 5) for _ in range(rnd.choice([0, 1, 1, 2, 3]))]
        expected, cont, follow, sync = ids(), ids(), ids(), ids()
        flags = [rnd.randint(0, 1) for _ in range(3)] + [rnd.choice([1, 2, 64])]
        anchors = [ids() for _ in range(rnd.randint(0, 3))]
        errs, skipped = rnd.choice([0, 0, 31, 3]), rnd.choice([0, 0, 2047, 5])
        line = "%d %s %s %s %s %s %s %d %s %d %d 32 2048" % (
            n, " ".join(map(str, kinds)), vec(expected), vec(cont), vec(follow), vec(sync),
            " ".join(map(str, flags)), len(anchors), " ".join(vec(a) for a in anchors), errs, skipped)
        lines.append(line)
        cases.append((kinds, expected, cont, follow, sync, flags, anchors, errs, skipped))
    expected_out = run_exe("recover_diff", lines)
    for (kinds, expected, cont, follow, sync, flags, anchors, errs, skipped), exp in zip(cases, expected_out):
        toks = [RawToken.make(RawTokenMeta(i * 2, "", False, 1, 1, 1, k), 0) for i, k in enumerate(kinds)]
        it = PyIt(toks)
        st = recovery.State()
        st.anchors = [list(a) for a in anchors]
        st.anchor_count = len(st.anchors)
        for _ in range(errs):
            st.diagnostics.append(recovery.Diagnostic())
        st.total_skipped = skipped
        plan = recovery.Plan("r", expected, cont, follow, sync, bool(flags[0]), bool(flags[1]), bool(flags[2]), flags[3])
        d = recovery.recover_default(it, st, plan)
        got = "%d %d %d %d %d %d %d" % (d.mode, d.synthesized, d.unexpected, d.skipped, it.i,
                                        st.total_skipped, len(st.diagnostics))
        assert got == exp, (kinds, expected, cont, follow, sync, flags, anchors, errs, skipped)


# --- DFA::run -----------------------------------------------------------------
def make_tables():
    N = NULL_STATE
    cls = [0] * 256
    for c in range(ord("a"), ord("z") + 1):
        cls[c] = 1
    for c in range(ord("0"), ord("9") + 1):
        cls[c] = 2
    cls[ord(" ")] = 3
    cls[ord("\n")] = 3
    cls[ord("+")] = 4
    table = [[N, 5, 7, 3, 4], [6, 1, 1, 6, 6], [9, 9, 2, 9, 9], [10, 10, 10, 3, 10], [11, 11, 11, 11, 11]]
    actions = [[Action.SET, 0, N, 1], [Action.SET, 1, N, 8], [Action.SET, 0, N, 2]]
    return DfaTables(table, cls, actions, TdfaLayout(3, 2), [], DebugIndex())


def semantic(idx, cap, start_pos, length, start_line, cur):
    if idx == 0:
        return Pair(NULL_STATE, "ID:%s@%d+%d" % (cap.text(0), start_pos, length))
    if idx == 1:
        return Pair(NULL_STATE, "NUM@%d+%d" % (start_pos, length))
    if idx == 2:
        return Pair(NULL_STATE, "")
    return Pair(NULL_STATE, "PLUS")


def test_dfa_run_matches_cpp():
    rnd = random.Random(3)
    alphabet = "ab1 2+!\nz9"
    srcs = ["".join(rnd.choice(alphabet) for _ in range(rnd.randint(1, 30))) for _ in range(500)]
    expected = run_exe("dfa_diff", [hexs(s) for s in srcs])
    tables = make_tables()
    for s, exp in zip(srcs, expected):
        pos, line, out = 0, 1, ""
        while pos < len(s):
            before = pos
            cur = Cursor(pos, line)
            tok = run(s, cur, pos, tables, semantic, "")
            pos, line = cur.pos, cur.line
            out += "[%s|%d|%d]" % (tok, pos, line)
            if pos == before:
                out += "STUCK"
                break
        assert out == exp, s
