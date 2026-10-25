"""ISPA debug transport v2 for generated Python parsers."""

from contextlib import contextmanager
from dataclasses import fields, is_dataclass
import json
import os
import struct
import threading


KINDS = {
    "trace": 1, "value": 2, "rule_enter": 3, "rule_exit": 4,
    "diagnostic": 5, "breakpoint": 6, "lexer_enter": 7,
    "lexer_leave": 8, "lexer_capture": 9, "line": 10,
    "production_enter": 11, "production_leave": 12,
}
MAX_PAYLOAD = 1048576
_lock = threading.Lock()
_state = threading.local()


def _fd(name):
    try:
        return int(os.environ[name])
    except (KeyError, ValueError):
        return -1


def enabled():
    return _fd("ISPA_DEBUG_FD") >= 0


def _current():
    if not hasattr(_state, "depth"):
        _state.depth = 0
        _state.location = ("", 0, 0)
        _state.stepping = bool(os.environ.get("ISPA_DEBUG_INITIAL_PAUSE"))
        _state.target_depth = -1
        _state.breaks = set()
        _state.failure_seen = False
    return _state


def _write(kind, data):
    fd = _fd("ISPA_DEBUG_FD")
    if fd < 0:
        return
    payload = json.dumps(data, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
    if len(payload) > MAX_PAYLOAD:
        payload = json.dumps({"name": "debug payload exceeded 1 MiB"}).encode("utf-8")
        kind = "diagnostic"
    header = struct.pack("<4sBBHI", b"ISDB", 2, KINDS[kind], 0, len(payload))
    with _lock:
        frame = header + payload
        while frame:
            written = os.write(fd, frame)
            if written <= 0:
                return
            frame = frame[written:]


def _command(state):
    fd = _fd("ISPA_DEBUG_CONTROL_FD")
    if fd < 0:
        return
    while True:
        action = os.read(fd, 1)
        if not action:
            return
        if action == b"b":
            place = bytearray()
            while len(place) < 8192:
                ch = os.read(fd, 1)
                if not ch or ch == b"\n":
                    break
                place.extend(ch)
            text = place.decode("utf-8", "replace")
            file, _, row = text.rpartition(":")
            if file and row.isdecimal():
                state.breaks.add((file, int(row)))
            continue
        if action in (b"s", b"c", b"n", b"f"):
            state.stepping = action != b"c"
            state.target_depth = (state.depth if action == b"n" else
                                  state.depth - 1 if action == b"f" else -1)
            return


def _typed(value, seen=None, depth=0):
    if seen is None:
        seen = set()
    typename = f"{type(value).__module__}.{type(value).__qualname__}"
    if value is None:
        return {"type": "null", "value": None}
    if isinstance(value, (bool, int, float, str)):
        return {"type": type(value).__name__, "value": value}
    if isinstance(value, bytes):
        return {"type": "bytes", "value": value.hex()}
    if type(value).__name__ == "Cursor" and hasattr(value, "index") and hasattr(value, "lexer"):
        return {"type": "ISPA.Cursor", "value": {"token_index": value.index}}
    if depth >= 16:
        return {"type": typename, "value": "<depth limit>"}
    identity = id(value)
    if identity in seen:
        return {"type": typename, "value": "<cycle>"}
    seen.add(identity)
    try:
        if typename.startswith("python.model.") and all(
                hasattr(value, field) for field in ("startpos", "length", "kind", "value", "absent")):
            start = value.startpos
            size = value.length
            source = value.source
            content = {"kind": _typed(value.kind, seen, depth + 1),
                       "value": _typed(value.value, seen, depth + 1),
                       "startpos": _typed(start, seen, depth + 1),
                       "length": _typed(size, seen, depth + 1),
                       "line": _typed(value.line, seen, depth + 1),
                       "column": _typed(value.column, seen, depth + 1),
                       "absent": _typed(value.absent, seen, depth + 1)}
            if isinstance(source, str) and isinstance(start, int) and isinstance(size, int) and start >= 0:
                content["text"] = _typed(source[start:start + min(size, 256)], seen, depth + 1)
        elif isinstance(value, (list, tuple, set)):
            content = [_typed(item, seen, depth + 1) for item in list(value)[:256]]
        elif isinstance(value, dict):
            content = [[_typed(key, seen, depth + 1), _typed(item, seen, depth + 1)]
                       for key, item in list(value.items())[:256]]
            return {"type": "map", "value": content}
        elif is_dataclass(value):
            content = {item.name: _typed(getattr(value, item.name), seen, depth + 1)
                       for item in fields(value) if not item.name.startswith("_")}
        elif hasattr(value, "__dict__"):
            content = {name: _typed(item, seen, depth + 1)
                       for name, item in vars(value).items() if not name.startswith("_")}
        elif hasattr(type(value), "__slots__"):
            slots = type(value).__slots__
            if isinstance(slots, str):
                slots = (slots,)
            content = {name: _typed(getattr(value, name), seen, depth + 1)
                       for name in slots if not name.startswith("_") and hasattr(value, name)}
        else:
            content = "<unavailable>"
        return {"type": typename if not isinstance(value, (list, tuple, set)) else "list",
                "value": content}
    finally:
        seen.remove(identity)


def line(file, row, column, variables=None, phase="before"):
    if not enabled():
        return
    state = _current()
    state.failure_seen = False
    hit = state.stepping and (state.target_depth < 0 or state.depth <= state.target_depth)
    hit = hit or (file, row) in state.breaks
    paused = hit and _fd("ISPA_DEBUG_CONTROL_FD") >= 0
    _write("line", {"file": file, "line": row, "column": column, "depth": state.depth,
                    "phase": phase, "paused": paused,
                    "variables": {name: _typed(value) for name, value in (variables or {}).items()}})
    if paused:
        _command(state)


@contextmanager
def scope(kind, name, file="", row=0, column=0):
    state = _current()
    previous = state.location
    state.location = (file, row, column)
    _write("rule_enter" if kind == "rule" else "production_enter",
           {"name": name, "file": file, "line": row, "column": column})
    state.depth += 1
    try:
        yield
    except Exception as error:
        if type(error).__name__ not in {"_NoMatch", "ExternalParseError"}:
            breakpoint("uncaught parser failure")
        raise
    finally:
        state.depth -= 1
        _write("rule_exit" if kind == "rule" else "production_leave",
               {"name": name, "file": file, "line": row, "column": column})
        state.location = previous


def event(kind, name, **data):
    if not enabled():
        return
    file, row, column = _current().location
    _write(kind, {"name": name, "file": file, "line": row, "column": column, **data})


def capture(name, value):
    if enabled():
        event("lexer_capture", name, data=_typed(value))


def value(name, item):
    if enabled():
        event("value", name, data=_typed(item))


def match(name, actual, success, reason=""):
    event("lexer_leave", name, actual=str(actual), success=bool(success), reason=reason)


def breakpoint(label=""):
    if not enabled():
        return
    state = _current()
    if state.failure_seen:
        return
    state.failure_seen = True
    paused = _fd("ISPA_DEBUG_CONTROL_FD") >= 0
    event("breakpoint", label, paused=paused)
    state.stepping = True
    state.target_depth = -1
    if paused:
        _command(state)
