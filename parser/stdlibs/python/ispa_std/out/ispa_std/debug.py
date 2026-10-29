"""ispa_debug.hpp: ISPA debug transport v2, minus the operating-system parts.

Structured events are JSON carried inside length-framed ISDB records. This
module builds the records and runs the stepping state machine; WHERE records go
is a host concern: call `configure(sink, control)` with
  sink.write(text) -> bool        receives each framed record (8-bit units)
  control.read_byte() -> int      next command byte, -1 at end   (may be omitted)
The C++ side (ISPA_DEBUG_FD / ISPA_DEBUG_CONTROL_FD, getenv, write(2), mutex)
stays C++: cpp_residue/ispastdlib/debug_transport.hpp. With nothing configured
every call is a no-op, like the C++ build without ISPA_DEBUG_FD.

Where C++ used RAII (RuleScope, ProductionScope, MatchScope) call `close()` on
every exit path. A scope abandoned by an exception is closed by
`unwind_to(depth)`, which the parser calls where it catches NoMatch."""
from ispa_std import reflect
from ispa_std.chars import char_of, code_of
from ispa_std.format import int_to_string, join
from ispa_std.string import substr
from ispa_std.utility import Pair


class Kind:
    trace = 1
    value = 2
    enter_rule = 3
    exit_rule = 4
    diagnostic = 5
    breakpoint = 6
    lexer_enter = 7
    lexer_leave = 8
    lexer_capture = 9
    source_line = 10
    production_enter = 11
    production_leave = 12
    snapshot = 13
    session_end = 14


MAX_PAYLOAD = 1048576


class Location:
    file: str
    line: int
    column: int

    def __init__(self, file: str, line: int, column: int):
        self.file = file
        self.line = line
        self.column = column

    @staticmethod
    def none() -> "Location":
        return Location("", 0, 0)

    def copy(self) -> "Location":
        return Location(self.file, self.line, self.column)


class DebugState:
    sink: object
    has_sink: bool
    control: object
    has_control: bool
    stepping: bool
    target_depth: int
    frame_depth: int
    break_files: list
    break_lines: list
    failure_seen: bool
    location: Location
    scopes: list
    serial: int

    def __init__(self):
        self.sink = 0
        self.has_sink = False
        self.control = 0
        self.has_control = False
        self.stepping = False
        self.target_depth = -1
        self.frame_depth = 0
        self.break_files = []
        self.break_lines = []
        self.failure_seen = False
        self.location = Location.none()
        self.scopes = []
        self.serial = 0


_S: DebugState = DebugState()


def configure(sink, control) -> None:
    _S.sink = sink
    _S.has_sink = True
    _S.control = control
    _S.has_control = True


def configure_sink_only(sink) -> None:
    _S.sink = sink
    _S.has_sink = True
    _S.has_control = False


def reset() -> None:
    """Back to the unconfigured state (also clears stepping and scopes)."""
    _S.has_sink = False
    _S.has_control = False
    _S.stepping = False
    _S.target_depth = -1
    _S.frame_depth = 0
    _S.break_files.clear()
    _S.break_lines.clear()
    _S.failure_seen = False
    _S.location = Location.none()
    _S.scopes.clear()


def enabled() -> bool:
    return _S.has_sink


def initial_pause() -> None:
    _S.stepping = True


def active_location() -> Location:
    return _S.location


# --- framing -----------------------------------------------------------------
def frame(kind: int, payload: str) -> str:
    n = len(payload)
    header = "ISDB" + char_of(2) + char_of(kind) + char_of(0) + char_of(0)
    header = header + char_of(n - (n // 256) * 256)
    n1 = n // 256
    header = header + char_of(n1 - (n1 // 256) * 256)
    n2 = n1 // 256
    header = header + char_of(n2 - (n2 // 256) * 256)
    n3 = n2 // 256
    header = header + char_of(n3 - (n3 // 256) * 256)
    return header + payload


def emit(kind: int, payload: str) -> bool:
    if not _S.has_sink or len(payload) > MAX_PAYLOAD:
        return False
    return _S.sink.write(frame(kind, payload))


# --- JSON building -----------------------------------------------------------
def quote(s: str) -> str:
    out = '"'
    hexd = "0123456789abcdef"
    i = 0
    while i < len(s):
        c = s[i]
        code = code_of(c)
        if c == '"':
            out = out + '\\"'
        elif c == "\\":
            out = out + "\\\\"
        elif c == "\n":
            out = out + "\\n"
        elif c == "\r":
            out = out + "\\r"
        elif c == "\t":
            out = out + "\\t"
        elif code < 32:
            out = out + "\\u00" + hexd[code // 16] + hexd[code - (code // 16) * 16]
        else:
            out = out + c
        i = i + 1
    return out + '"'


def obj(type_name: str, data: str) -> str:
    return '{"type":' + quote(type_name) + ',"value":' + data + "}"


def encode(value, depth: int) -> str:
    """Typed JSON for a value; objects expose debug_type()/debug_fields()."""
    kind = reflect.kind_of(value)
    if kind == reflect.NONE:
        return obj("null", "null")
    if kind == reflect.BOOL:
        if value:
            return obj("bool", "true")
        return obj("bool", "false")
    if kind == reflect.INT:
        return obj("int", int_to_string(value))
    if kind == reflect.FLOAT:
        return obj("double", reflect.float_to_string(value))
    if kind == reflect.STR:
        return obj("string", quote(value))
    if kind == reflect.LIST or kind == reflect.TUPLE:
        if depth >= 16:
            return obj("list", '"<depth limit>"')
        parts = []
        i = 0
        while i < len(value) and i < 256:
            parts.append(encode(value[i], depth + 1))
            i = i + 1
        return obj("list", "[" + join(parts, ",") + "]")
    if reflect.has_method(value, "debug_fields"):
        name = value.debug_type()
        if depth >= 16:
            return obj(name, '"<depth limit>"')
        fields = value.debug_fields()
        parts = []
        i = 0
        while i < len(fields):
            pair = fields[i]
            parts.append(quote(pair.first) + ":" + encode(pair.second, depth + 1))
            i = i + 1
        return obj(name, "{" + join(parts, ",") + "}")
    return obj("opaque", '"<unavailable: expose debug_fields>"')


def location_json(loc: Location) -> str:
    return '"file":' + quote(loc.file) + ',"line":' + int_to_string(loc.line) + ',"column":' + int_to_string(loc.column)


def event(kind: int, loc: Location, name: str, extra: str) -> bool:
    body = "{" + location_json(loc) + ',"name":' + quote(name)
    if len(extra) > 0:
        body = body + "," + extra
    return emit(kind, body + "}")


def trace(text: str) -> bool:
    return event(Kind.trace, Location.none(), text, "")


def diagnostic(text: str) -> bool:
    return event(Kind.diagnostic, Location.none(), text, "")


def enter_rule(name: str, loc: Location) -> bool:
    return event(Kind.enter_rule, loc, name, "")


def exit_rule(name: str, loc: Location) -> bool:
    return event(Kind.exit_rule, loc, name, "")


def enter_production(name: str, loc: Location) -> bool:
    return event(Kind.production_enter, loc, name, "")


def leave_production(name: str, loc: Location) -> bool:
    return event(Kind.production_leave, loc, name, "")


def lexer_enter(token: str, loc: Location) -> bool:
    return event(Kind.lexer_enter, loc, token, "")


def lexer_result(token: str, success: bool, actual: str, reason: str, loc: Location) -> bool:
    flag = "false"
    if success:
        flag = "true"
    return event(Kind.lexer_leave, loc, token,
                 '"success":' + flag + ',"actual":' + quote(actual) + ',"reason":' + quote(reason))


def value(name: str, data, loc: Location) -> bool:
    return event(Kind.value, loc, name, '"data":' + encode(data, 0))


def lexer_capture(name: str, data, loc: Location) -> bool:
    return event(Kind.lexer_capture, loc, name, '"data":' + encode(data, 0))


class Snapshot:
    _values: list

    def __init__(self):
        self._values = []

    def add(self, name: str, data) -> "Snapshot":
        self._values.append(quote(name) + ":" + encode(data, 0))
        return self

    def json(self) -> str:
        return "{" + join(self._values, ",") + "}"


# --- stepping ----------------------------------------------------------------
def command() -> bool:
    """Block on the control channel for a stepping command. False when there is none."""
    if not _S.has_control:
        return False
    while True:
        b = _S.control.read_byte()
        if b < 0:
            return False
        c = char_of(b)
        if c == "s":
            _S.stepping = True
            _S.target_depth = -1
            return True
        if c == "c":
            _S.stepping = False
            _S.target_depth = -1
            return True
        if c == "n":
            _S.stepping = True
            _S.target_depth = _S.frame_depth
            return True
        if c == "f":
            _S.stepping = True
            _S.target_depth = _S.frame_depth - 1
            return True
        if c == "b":
            text = ""
            reading = True
            while reading and len(text) < 8192:
                nb = _S.control.read_byte()
                if nb < 0 or char_of(nb) == "\n":
                    reading = False
                else:
                    text = text + char_of(nb)
            split = -1
            j = len(text) - 1
            while j >= 0 and split == -1:
                if text[j] == ":":
                    split = j
                j = j - 1
            if split != -1:
                digits = substr(text, split + 1, -1)
                number = 0
                valid = len(digits) > 0
                k = 0
                while k < len(digits):
                    d = code_of(digits[k]) - 48
                    if d < 0 or d > 9:
                        valid = False
                    else:
                        number = number * 10 + d
                    k = k + 1
                if valid:
                    _S.break_files.append(substr(text, 0, split))
                    _S.break_lines.append(number)
    return False


def line(loc: Location, snap: Snapshot, phase: str) -> bool:
    _S.failure_seen = False
    hit = _S.stepping and (_S.target_depth < 0 or _S.frame_depth <= _S.target_depth)
    i = 0
    while i < len(_S.break_files):
        if _S.break_files[i] == loc.file and _S.break_lines[i] == loc.line:
            hit = True
        i = i + 1
    can_pause = _S.has_control
    paused = "false"
    if hit and can_pause:
        paused = "true"
    payload = (location_json(loc) + ',"depth":' + int_to_string(_S.frame_depth) + ',"paused":' + paused
               + ',"phase":' + quote(phase) + ',"variables":' + snap.json())
    ok = emit(Kind.source_line, "{" + payload + "}")
    if hit and can_pause:
        command()
    return ok


def rule_depth_enter() -> None:
    _S.frame_depth = _S.frame_depth + 1


def rule_depth_leave() -> None:
    _S.frame_depth = _S.frame_depth - 1


def breakpoint(label: str) -> None:
    if _S.failure_seen:
        return
    _S.failure_seen = True
    can_pause = _S.has_control
    flag = "false"
    if can_pause:
        flag = "true"
    event(Kind.breakpoint, Location.none(), label, '"paused":' + flag)
    _S.stepping = True
    _S.target_depth = -1
    if can_pause:
        command()


# --- scopes (the RAII classes) ------------------------------------------------
def scope_depth() -> int:
    return len(_S.scopes)


def unwind_to(depth: int) -> None:
    """Close scopes opened after `depth`, innermost first (what C++ destructors
    do while an exception propagates)."""
    while len(_S.scopes) > depth:
        _S.scopes[len(_S.scopes) - 1].close()


def _next_serial() -> int:
    _S.serial = _S.serial + 1
    return _S.serial


def _forget(scope) -> None:
    i = len(_S.scopes) - 1
    while i >= 0:
        if _S.scopes[i].serial == scope.serial:
            _S.scopes.pop(i)
            return
        i = i - 1


class RuleScope:
    name: str
    loc: Location
    previous: Location
    open: bool
    serial: int

    def __init__(self, name: str, loc: Location):
        self.name = name
        self.loc = loc
        self.previous = _S.location
        self.open = True
        self.serial = _next_serial()
        _S.location = loc
        enter_rule(name, loc)
        rule_depth_enter()
        _S.scopes.append(self)

    def close(self) -> None:
        if not self.open:
            return
        self.open = False
        _forget(self)
        rule_depth_leave()
        exit_rule(self.name, self.loc)
        _S.location = self.previous


class ProductionScope:
    name: str
    loc: Location
    previous: Location
    open: bool
    serial: int

    def __init__(self, name: str, loc: Location):
        self.name = name
        self.loc = loc
        self.previous = _S.location
        self.open = True
        self.serial = _next_serial()
        _S.location = loc
        enter_production(name, loc)
        rule_depth_enter()
        _S.scopes.append(self)

    def close(self) -> None:
        if not self.open:
            return
        self.open = False
        _forget(self)
        rule_depth_leave()
        leave_production(self.name, self.loc)
        _S.location = self.previous


class MatchScope:
    expected: str
    actual: str
    reason: str
    loc: Location
    matched: bool
    active: bool
    open: bool
    serial: int

    def __init__(self, want: str, got: str):
        self.expected = want
        self.actual = got
        self.reason = "no match"
        self.loc = _S.location
        self.matched = False
        self.active = enabled()
        self.open = True
        self.serial = _next_serial()
        if self.active:
            lexer_enter(want, self.loc)
            _S.scopes.append(self)

    def success(self) -> None:
        self.matched = True
        self.reason = "matched"

    def outcome(self, why: str) -> None:
        self.reason = why

    def close(self) -> None:
        if not self.open:
            return
        self.open = False
        if self.active:
            _forget(self)
            lexer_result(self.expected, self.matched, self.actual, self.reason, self.loc)

