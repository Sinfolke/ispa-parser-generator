#!/usr/bin/env python3
"""CLI directory entry point for the graphical ISPA debugger."""

from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent / "pkg"))
from ispa_pkg.debugger_gui import main

sys.exit(main())
