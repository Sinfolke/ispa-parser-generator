"""Compiler-facing error primitives.

Each declaration lowers to the target runtime's native implementation (see
StdlibExports::Fail, Attempt and FailureMessage); the file is compiled, never
executed. Under plain CPython `prim_cpython` replaces this module (cpython.install()).

fail(kind, message)  raises the target's native error for ErrorKind `kind`; never returns.
attempt(fn)          calls fn(); returns 0 on completion, else the ErrorKind that fn raised.
failure_message()    message of the failure attempt() last caught.
"""

fail = __ispa_internal__("__ispa_ir__.fail", 2)
attempt = __ispa_internal__("__ispa_ir__.attempt", 1)
failure_message = __ispa_internal__("__ispa_ir__.failure_message", 0)
