#!/usr/bin/env python3
"""Source checkout entry point for the ISPA debugger."""

from pathlib import Path
import runpy

runpy.run_path(str(Path(__file__).resolve().parent / "pkg" / "ispa_pkg" / "debugger.py"), run_name="__main__")
