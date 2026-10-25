#!/usr/bin/env python3
"""Run an instrumented ISPA parser and inspect its recorded execution."""

import argparse
import json
import os
from pathlib import Path
import struct
import subprocess
import sys


KINDS = {
    1: "trace", 2: "value", 3: "rule_enter", 4: "rule_exit",
    5: "diagnostic", 6: "breakpoint", 7: "lexer_enter", 8: "lexer_leave",
    9: "lexer_capture", 10: "line", 11: "production_enter",
    12: "production_leave", 13: "snapshot", 14: "end",
}
PURPOSES = {
    "matching": {"lexer_enter", "lexer_leave", "lexer_capture", "rule_enter", "rule_exit", "production_enter", "production_leave", "line", "breakpoint"},
    "failure": {"diagnostic", "breakpoint", "lexer_leave", "rule_exit", "production_leave", "line"},
    "ast": {"value", "lexer_capture", "rule_exit", "production_leave", "line", "breakpoint"},
}


def read_exact(fd, size):
    data = bytearray()
    while len(data) < size:
        chunk = os.read(fd, size - len(data))
        if not chunk:
            if data:
                raise EOFError("truncated ISPA debug frame")
            return None
        data.extend(chunk)
    return bytes(data)


def events(fd):
    while (header := read_exact(fd, 12)) is not None:
        magic, version, kind, reserved, length = struct.unpack("<4sBBHI", header)
        if magic != b"ISDB" or version != 2 or reserved or length > 1048576:
            raise ValueError("invalid ISPA debug v2 frame")
        payload = read_exact(fd, length)
        if payload is None:
            raise EOFError("truncated ISPA debug frame")
        value = json.loads(payload)
        if not isinstance(value, dict):
            raise ValueError("ISPA debug payload must be an object")
        yield {"kind": KINDS.get(kind, f"unknown_{kind}"), "data": value}


def unwrap(value):
    if not isinstance(value, dict) or "type" not in value or "value" not in value:
        return value
    kind, data = value["type"], value["value"]
    if kind == "list":
        return [unwrap(item) for item in data]
    if kind == "map":
        return [[unwrap(key), unwrap(item)] for key, item in data]
    if kind == "variant":
        return {"index": data["index"], "value": unwrap(data["item"])}
    if kind == "optional":
        return unwrap(data)
    if isinstance(data, dict):
        return {key: unwrap(item) for key, item in data.items()}
    return data


def describe(event):
    kind, data = event["kind"], event["data"]
    place = f"{data.get('file', '?')}:{data.get('line', 0)}"
    name = data.get("name", "")
    if kind == "line":
        return f"[{kind} {data.get('phase', 'before')}] {place} depth={data.get('depth', 0)}"
    if kind in {"value", "lexer_capture"}:
        return f"[{kind}] {name}={unwrap(data.get('data'))!r} {place}"
    details = data.get("reason") or data.get("outcome") or ""
    return f"[{kind}] {name} {place} {details}".rstrip()


def snapshot(event, name=None):
    values = event["data"].get("variables", {})
    if name is None:
        selected = values
    elif name == "ctx" and name not in values:
        selected = {key[4:]: value for key, value in values.items() if key.startswith("ctx.")}
        if not selected:
            print("'ctx' is not available at this checkpoint")
            return
    elif name not in values:
        print(f"{name!r} is not available at this checkpoint")
        return
    else:
        selected = values[name]
    if isinstance(selected, dict) and "type" not in selected:
        selected = {key: unwrap(value) for key, value in selected.items()}
    else:
        selected = unwrap(selected)
    print(json.dumps(selected, indent=2, ensure_ascii=False))


def pause(event, history, command_fd):
    cursor = len(history) - 1
    while True:
        try:
            entry = input("(ispadbg) ").strip()
        except EOFError:
            entry = "c"
        operation, _, argument = entry.partition(" ")
        if operation in {"", "s", "step", "n", "next", "f", "finish", "c", "continue"}:
            if cursor != len(history) - 1:
                print("Returning to the live pause before resuming")
            command = {"": "s", "step": "s", "next": "n", "finish": "f", "continue": "c"}.get(operation, operation)
            os.write(command_fd, command.encode("ascii"))
            return
        if operation in {"back", "reverse", "rb"}:
            if cursor > 0:
                cursor -= 1
                print(f"history {cursor}: {describe(history[cursor])}")
            else:
                print("At the oldest snapshot")
        elif operation in {"forward", "fw"}:
            if cursor < len(history) - 1:
                cursor += 1
                print(f"history {cursor}: {describe(history[cursor])}")
            else:
                print("At the live pause")
        elif operation == "live":
            cursor = len(history) - 1
            print(describe(history[cursor]))
        elif operation in {"p", "print", "ctx"}:
            name = "ctx" if operation == "ctx" else argument or None
            snapshot(history[cursor], name)
        elif operation in {"b", "break"}:
            if not argument or ":" not in argument or not argument.rsplit(":", 1)[1].isdigit():
                print("Usage: b file:line")
            else:
                os.write(command_fd, b"b" + argument.encode("utf-8") + b"\n")
                print(f"Breakpoint added: {argument}")
        elif operation in {"where", "w"}:
            print(describe(history[cursor]))
        elif operation in {"help", "h", "?"}:
            print("s step, n next, f finish, c continue, back, forward, live, p [name], ctx, b file:line, where")
        else:
            print("Unknown command; enter help for commands")


def replay(path):
    with open(path, encoding="utf-8") as stream:
        recorded = json.load(stream)
    history = [event for event in recorded if event["kind"] == "line"]
    print(f"{len(recorded)} events; {len(history)} source snapshots")
    cursor = len(history) - 1
    if history:
        print(f"history {cursor}: {describe(history[cursor])}")
    while True:
        try:
            entry = input("(ispadbg replay) ").strip()
        except EOFError:
            return
        operation, _, argument = entry.partition(" ")
        if operation in {"q", "quit", "exit"}:
            return
        if operation in {"back", "reverse", "rb"} and cursor > 0:
            cursor -= 1
        elif operation in {"forward", "fw"} and cursor + 1 < len(history):
            cursor += 1
        elif operation in {"goto", "g"}:
            try:
                next_cursor = int(argument)
                if not 0 <= next_cursor < len(history):
                    raise ValueError
                cursor = next_cursor
            except ValueError:
                print("Usage: goto <snapshot number>")
                continue
        elif operation in {"p", "print", "ctx"} and history:
            snapshot(history[cursor], "ctx" if operation == "ctx" else argument or None)
            continue
        elif operation in {"event", "e"}:
            try:
                print(json.dumps(recorded[int(argument)], indent=2, ensure_ascii=False))
            except (ValueError, IndexError):
                print("Usage: event <event number>")
            continue
        elif operation not in {"where", "w", "back", "reverse", "rb", "forward", "fw", "goto", "g"}:
            print("Commands: back, forward, goto N, p [name], ctx, event N, where, q")
            continue
        if history:
            print(f"history {cursor}: {describe(history[cursor])}")


def run(program, purposes, breakpoints, record, interactive):
    event_read, event_write = os.pipe()
    control_read, control_write = os.pipe()
    environment = os.environ.copy()
    environment["ISPA_DEBUG_FD"] = str(event_write)
    environment["ISPA_DEBUG_CONTROL_FD"] = str(control_read) if interactive else ""
    environment["ISPA_DEBUG_PURPOSES"] = ",".join(purposes)
    environment["ISPA_DEBUG_INITIAL_PAUSE"] = "1" if interactive else ""
    print("[ispadbg] purpose: " + ", ".join(purposes), flush=True)
    process = subprocess.Popen(program, env=environment, pass_fds=(event_write, control_read))
    os.close(event_write)
    os.close(control_read)
    recorded, history = [], []
    try:
        for event in events(event_read):
            recorded.append(event)
            if event["kind"] == "line":
                history.append(event)
            if event["kind"] in set.union(*(PURPOSES[purpose] for purpose in purposes)):
                print(describe(event), flush=True)
            if interactive and event["data"].get("paused"):
                for breakpoint in breakpoints:
                    os.write(control_write, b"b" + breakpoint.encode("utf-8") + b"\n")
                breakpoints.clear()
                if event["kind"] == "line":
                    pause(event, history, control_write)
                else:
                    print("Stopped on parser failure")
                    if history:
                        pause(event, history, control_write)
                    else:
                        os.write(control_write, b"c")
    finally:
        os.close(event_read)
        os.close(control_write)
        status = process.wait()
        if record:
            Path(record).write_text(json.dumps(recorded, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
        print(f"[ispadbg] exit={status}, events={len(recorded)}, snapshots={len(history)}", file=sys.stderr)
    return status


def main(argv=None):
    parser = argparse.ArgumentParser(prog="ispadbg", description=__doc__)
    parser.add_argument("--purpose", action="append", choices=PURPOSES, help="debug purpose ID; may repeat")
    parser.add_argument("-b", "--breakpoint", action="append", default=[], metavar="FILE:LINE")
    parser.add_argument("--record", metavar="FILE", help="write complete JSON event history")
    parser.add_argument("--replay", metavar="FILE", help="inspect an existing JSON history")
    parser.add_argument("--no-interactive", action="store_true", help="record without pausing")
    parser.add_argument("program", nargs=argparse.REMAINDER, help="program and arguments after --")
    args = parser.parse_args(argv)
    if args.replay:
        replay(args.replay)
        return 0
    program = args.program[1:] if args.program[:1] == ["--"] else args.program
    if not program:
        parser.error("a parser program is required after --")
    if os.name != "posix":
        parser.error("running a program currently requires POSIX file descriptors")
    for breakpoint in args.breakpoint:
        if ":" not in breakpoint or not breakpoint.rsplit(":", 1)[1].isdigit():
            parser.error(f"invalid breakpoint: {breakpoint!r}; expected file:line")
    return run(program, args.purpose or ["failure"], set(args.breakpoint), args.record, not args.no_interactive)


if __name__ == "__main__":
    sys.exit(main())
