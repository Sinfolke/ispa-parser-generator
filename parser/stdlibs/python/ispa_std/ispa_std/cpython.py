"""Run the library under plain CPython.

Call install() BEFORE importing any other ispa_std module. It swaps the
compiler-facing ispa_std.prim (IR constructors) for the strict reference
runtime, so every `from ispa_std import prim` resolves to executable code.
"""
import sys


def install():
    from ispa_std import prim_cpython
    sys.modules["ispa_std.prim"] = prim_cpython
    import ispa_std
    ispa_std.prim = prim_cpython
