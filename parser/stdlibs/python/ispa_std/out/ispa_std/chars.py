"""Character-code helpers for the 8-bit-unit source model.

The C++ runtime walks `const char*` and indexes its tables with the unsigned byte
value. Here a source is a native String whose elements are 8-bit units
(for text read as bytes, decode it as latin-1 first). `code_of` maps a one-unit
String to its code through the `prim` character primitives (native per target).
The empty String "" stands for the NUL terminator / end of input.
"""

from ispa_std import prim


def code_of(ch: str) -> int:
    # "" (end of input) behaves like the NUL byte, as it does in the C++ walker.
    code = prim.char_code(ch)
    if code > 255:
        return 255
    return code


def char_of(code: int) -> str:
    if code < 0 or code > 255:
        return ""
    return prim.char_from_code(code)



def at(s: str, index: int) -> str:
    """s[index], or "" outside the string (the C++ NUL terminator)."""
    if index < 0 or index >= len(s):
        return ""
    return s[index]


def is_space(ch: str) -> bool:
    # C isspace() in the "C" locale.
    if len(ch) == 0:
        return False
    return ch == " " or ch == "\t" or ch == "\n" or ch == "\r" or ch == "\x0b" or ch == "\x0c"


def is_digit(ch: str) -> bool:
    if len(ch) == 0:
        return False
    return ch >= "0" and ch <= "9"
