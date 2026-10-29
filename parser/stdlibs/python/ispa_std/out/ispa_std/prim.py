"""Compiler-facing error primitives.

Each declaration lowers to the target runtime's native implementation (see
StdlibExports::Fail, Attempt and FailureMessage); the file is compiled, never
executed. Under plain CPython `prim_cpython` replaces this module (cpython.install()).

fail(kind, message)  raises the target's native error for ErrorKind `kind`; never returns.
attempt(fn)          calls fn(); returns 0 on completion, else the ErrorKind that fn raised.
failure_message()    message of the failure attempt() last caught.
char_code(ch)        code (0..255) of a one-unit String; "" is 0.
char_from_code(n)    the one-unit String of code n in 0..255, "" otherwise.
"""

fail = __ispa_internal__("__ispa_ir__.fail", 2)
attempt = __ispa_internal__("__ispa_ir__.attempt", 1)
failure_message = __ispa_internal__("__ispa_ir__.failure_message", 0)
char_code = __ispa_internal__("__ispa_ir__.char_code", 1)
char_from_code = __ispa_internal__("__ispa_ir__.char_from_code", 1)
