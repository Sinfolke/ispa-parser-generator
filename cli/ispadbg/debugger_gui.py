"""Tkinter viewer for live and recorded ISPA debugger events."""

import json
import os
from pathlib import Path
import queue
import shlex
import subprocess
import threading
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

from .debugger import describe, events, unwrap


class DebuggerGui:
    def __init__(self, root, program="", input_text=""):
        self.root = root
        root.title("ISPA debugger")
        root.geometry("1100x740")
        self.messages = queue.Queue()
        self.recorded = []
        self.process = None
        self.control = None
        self.paused = False
        self.program = tk.StringVar(value=program)
        self.status = tk.StringVar(value="Ready")
        self.mode = tk.StringVar(value="argument")

        setup = ttk.Frame(root, padding=8)
        setup.pack(fill="x")
        ttk.Label(setup, text="Program and arguments").grid(row=0, column=0, sticky="w")
        ttk.Entry(setup, textvariable=self.program).grid(row=0, column=1, sticky="ew", padx=6)
        ttk.Button(setup, text="Browse…", command=self.browse_program).grid(row=0, column=2)
        setup.columnconfigure(1, weight=1)
        ttk.Label(setup, text="Input string").grid(row=1, column=0, sticky="nw", pady=6)
        self.input_box = tk.Text(setup, height=3, wrap="word")
        self.input_box.grid(row=1, column=1, sticky="ew", padx=6, pady=6)
        self.input_box.insert("1.0", input_text)
        modes = ttk.Frame(setup)
        modes.grid(row=1, column=2, sticky="n")
        ttk.Radiobutton(modes, text="Argument", variable=self.mode, value="argument").pack(anchor="w")
        ttk.Radiobutton(modes, text="Standard input", variable=self.mode, value="stdin").pack(anchor="w")

        toolbar = ttk.Frame(root, padding=(8, 0, 8, 8))
        toolbar.pack(fill="x")
        ttk.Button(toolbar, text="Run", command=self.run).pack(side="left")
        self.controls = {}
        for label, command in (("Step", "s"), ("Next", "n"), ("Finish", "f"), ("Continue", "c")):
            button = ttk.Button(toolbar, text=label, command=lambda c=command: self.resume(c))
            button.pack(side="left", padx=(5, 0))
            self.controls[command] = button
        ttk.Button(toolbar, text="Open trace…", command=self.open_trace).pack(side="left", padx=(18, 0))
        ttk.Button(toolbar, text="Save trace…", command=self.save_trace).pack(side="left", padx=5)
        self.set_paused(False)

        splitter = ttk.Panedwindow(root, orient="horizontal")
        splitter.pack(fill="both", expand=True, padx=8)
        left = ttk.Frame(splitter)
        right = ttk.Frame(splitter)
        splitter.add(left, weight=3)
        splitter.add(right, weight=2)
        ttk.Label(left, text="Trace (select an event to inspect its values)").pack(anchor="w")
        self.trace = ttk.Treeview(left, columns=("location", "event"), show="headings", selectmode="browse")
        self.trace.heading("location", text="Source")
        self.trace.heading("event", text="Event")
        self.trace.column("location", width=190, stretch=False)
        self.trace.column("event", width=470)
        scroll = ttk.Scrollbar(left, orient="vertical", command=self.trace.yview)
        self.trace.configure(yscrollcommand=scroll.set)
        self.trace.pack(side="left", fill="both", expand=True)
        scroll.pack(side="right", fill="y")
        self.trace.bind("<<TreeviewSelect>>", self.show_event)

        ttk.Label(right, text="Variables / AST / context").pack(anchor="w")
        self.values = ttk.Treeview(right, columns=("value",), show="tree headings", selectmode="browse")
        self.values.heading("#0", text="Name")
        self.values.heading("value", text="Value")
        self.values.column("#0", width=180)
        self.values.column("value", width=270)
        self.values.pack(fill="both", expand=True)
        ttk.Label(right, text="Selected event details").pack(anchor="w", pady=(6, 0))
        self.details = tk.Text(right, height=10, wrap="word", state="disabled")
        self.details.pack(fill="both", expand=True)
        ttk.Label(root, textvariable=self.status, padding=8).pack(fill="x")
        root.after(50, self.drain)
        root.protocol("WM_DELETE_WINDOW", self.close)

    def browse_program(self):
        path = filedialog.askopenfilename(title="Choose parser program")
        if path:
            self.program.set(shlex.quote(path))

    def set_paused(self, value):
        self.paused = value
        for button in self.controls.values():
            button.configure(state="normal" if value else "disabled")

    def run(self):
        if self.process is not None and self.process.poll() is None:
            messagebox.showinfo("Parser running", "Continue the current run before starting another.")
            return
        try:
            command = shlex.split(self.program.get())
        except ValueError as error:
            messagebox.showerror("Invalid command", str(error))
            return
        if not command:
            messagebox.showerror("Missing program", "Choose a parser program first.")
            return
        if os.name != "posix":
            messagebox.showerror("Unsupported platform", "Live debugging requires POSIX file descriptors. You can open a saved trace.")
            return
        input_text = self.input_box.get("1.0", "end-1c")
        if self.mode.get() == "argument":
            command.append(input_text)
        self.clear()
        event_read, event_write = os.pipe()
        control_read, control_write = os.pipe()
        env = os.environ.copy()
        env.update(ISPA_DEBUG_FD=str(event_write), ISPA_DEBUG_CONTROL_FD=str(control_read),
                   ISPA_DEBUG_INITIAL_PAUSE="1", ISPA_DEBUG_PURPOSES="matching,failure,ast")
        try:
            self.process = subprocess.Popen(command, env=env, pass_fds=(event_write, control_read),
                                            stdin=subprocess.PIPE if self.mode.get() == "stdin" else subprocess.DEVNULL,
                                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        except OSError as error:
            os.close(event_read)
            os.close(event_write)
            os.close(control_read)
            os.close(control_write)
            messagebox.showerror("Could not run parser", str(error))
            return
        os.close(event_write)
        os.close(control_read)
        self.control = control_write
        self.status.set("Running…")
        if self.mode.get() == "stdin":
            threading.Thread(target=self.send_input, args=(self.process, input_text), daemon=True).start()
        threading.Thread(target=self.read_events, args=(self.process, event_read), daemon=True).start()
        threading.Thread(target=self.read_output, args=(self.process,), daemon=True).start()

    def send_input(self, process, value):
        try:
            process.stdin.write(value.encode("utf-8"))
            process.stdin.close()
        except (BrokenPipeError, OSError):
            pass

    def read_events(self, process, fd):
        try:
            for event in events(fd):
                self.messages.put(("event", event))
        except (OSError, EOFError, ValueError, json.JSONDecodeError) as error:
            self.messages.put(("error", str(error)))
        finally:
            os.close(fd)
            self.messages.put(("exit", process.wait()))

    def read_output(self, process):
        for line in process.stdout:
            self.messages.put(("output", line.decode("utf-8", "replace").rstrip()))

    def drain(self):
        try:
            while True:
                kind, value = self.messages.get_nowait()
                if kind == "event":
                    index = len(self.recorded)
                    self.recorded.append(value)
                    data = value["data"]
                    location = f"{Path(data.get('file', '')).name}:{data.get('line', 0)}" if data.get("file") else ""
                    self.trace.insert("", "end", iid=str(index), values=(location, describe(value)))
                    if data.get("paused"):
                        self.set_paused(True)
                        self.trace.selection_set(str(index))
                        self.trace.see(str(index))
                        self.status.set(f"Paused at {location or value['kind']}")
                elif kind == "exit":
                    self.set_paused(False)
                    if self.control is not None:
                        os.close(self.control)
                        self.control = None
                    self.status.set(f"Exited with status {value} · {len(self.recorded)} events")
                elif kind == "error":
                    self.status.set(f"Trace error: {value}")
                elif kind == "output" and value:
                    self.status.set(f"Parser: {value[:180]}")
        except queue.Empty:
            pass
        self.root.after(50, self.drain)

    def resume(self, command):
        if self.paused and self.control is not None:
            try:
                os.write(self.control, command.encode("ascii"))
                self.set_paused(False)
                self.status.set("Running…")
            except OSError as error:
                self.status.set(f"Could not resume: {error}")

    def clear(self):
        self.recorded.clear()
        self.trace.delete(*self.trace.get_children())
        self.values.delete(*self.values.get_children())
        self.set_details("")
        self.set_paused(False)

    def open_trace(self):
        if self.process is not None and self.process.poll() is None:
            messagebox.showinfo("Parser running", "Finish the current run before opening a trace.")
            return
        path = filedialog.askopenfilename(title="Open ISPA trace", filetypes=[("JSON trace", "*.json"), ("All files", "*")])
        if not path:
            return
        try:
            with open(path, encoding="utf-8") as stream:
                recorded = json.load(stream)
            if not isinstance(recorded, list) or any(not isinstance(item, dict) or "kind" not in item or "data" not in item for item in recorded):
                raise ValueError("Expected a list of debugger events")
        except (OSError, ValueError) as error:
            messagebox.showerror("Could not open trace", str(error))
            return
        self.clear()
        for event in recorded:
            index = len(self.recorded)
            self.recorded.append(event)
            data = event["data"]
            location = f"{Path(data.get('file', '')).name}:{data.get('line', 0)}" if data.get("file") else ""
            self.trace.insert("", "end", iid=str(index), values=(location, describe(event)))
        self.status.set(f"Opened {path} · {len(recorded)} events")

    def save_trace(self):
        if not self.recorded:
            messagebox.showinfo("No trace", "Run a parser or open a trace first.")
            return
        path = filedialog.asksaveasfilename(title="Save ISPA trace", defaultextension=".json", filetypes=[("JSON trace", "*.json")])
        if path:
            try:
                Path(path).write_text(json.dumps(self.recorded, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
                self.status.set(f"Saved {path}")
            except OSError as error:
                messagebox.showerror("Could not save trace", str(error))

    def show_event(self, _event=None):
        selected = self.trace.selection()
        if not selected:
            return
        event = self.recorded[int(selected[0])]
        data = event["data"]
        self.values.delete(*self.values.get_children())
        variables = data.get("variables", {})
        if isinstance(variables, dict):
            for name, value in variables.items():
                self.add_value("", name, unwrap(value))
        elif "data" in data:
            self.add_value("", data.get("name", "value"), unwrap(data["data"]))
        self.set_details(json.dumps(event, indent=2, ensure_ascii=False))

    def add_value(self, parent, name, value):
        if isinstance(value, (dict, list)):
            node = self.values.insert(parent, "end", text=str(name), values=(f"{type(value).__name__} ({len(value)})",))
            pairs = value.items() if isinstance(value, dict) else enumerate(value)
            for key, child in pairs:
                self.add_value(node, key, child)
        else:
            self.values.insert(parent, "end", text=str(name), values=(repr(value),))

    def set_details(self, value):
        self.details.configure(state="normal")
        self.details.delete("1.0", "end")
        self.details.insert("1.0", value)
        self.details.configure(state="disabled")

    def close(self):
        if self.process is not None and self.process.poll() is None:
            self.process.terminate()
        if self.control is not None:
            os.close(self.control)
            self.control = None
        self.root.destroy()


def main(argv=None):
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", default="", help="initial input string")
    parser.add_argument("program", nargs=argparse.REMAINDER, help="optional parser command after --")
    args = parser.parse_args(argv)
    root = tk.Tk()
    DebuggerGui(root, shlex.join(args.program[1:] if args.program[:1] == ["--"] else args.program), args.input)
    root.mainloop()
    return 0
