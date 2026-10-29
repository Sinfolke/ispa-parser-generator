"""Reference runtime for the primitives in prim_table.py, for plain CPython."""
from ispa_std.errors import ErrorKind


class StdError(Exception):
    kind = 0


class OutOfRange(StdError, IndexError):
    kind = ErrorKind.OutOfRange


class InvalidArgument(StdError, ValueError):
    kind = ErrorKind.InvalidArgument


class LengthError(StdError, ValueError):
    kind = ErrorKind.LengthError


class BadOptionalAccess(StdError):
    kind = ErrorKind.BadOptionalAccess


class KeyNotFound(StdError, KeyError):
    kind = ErrorKind.KeyNotFound


class LogicError(StdError):
    kind = ErrorKind.LogicError


class RuntimeFailure(StdError, RuntimeError):
    kind = ErrorKind.RuntimeFailure


class InternalError(StdError):
    kind = ErrorKind.InternalError


class ApiError(StdError):
    kind = ErrorKind.ApiError


class NoMatch(StdError):
    kind = ErrorKind.NoMatch


class ExternalParseError(RuntimeFailure):
    kind = ErrorKind.ExternalParse


_KIND_TO_EXC = {
    ErrorKind.OutOfRange: OutOfRange,
    ErrorKind.InvalidArgument: InvalidArgument,
    ErrorKind.LengthError: LengthError,
    ErrorKind.BadOptionalAccess: BadOptionalAccess,
    ErrorKind.KeyNotFound: KeyNotFound,
    ErrorKind.LogicError: LogicError,
    ErrorKind.RuntimeFailure: RuntimeFailure,
    ErrorKind.InternalError: InternalError,
    ErrorKind.ApiError: ApiError,
    ErrorKind.NoMatch: NoMatch,
    ErrorKind.ExternalParse: ExternalParseError,
}


def fail(kind, message):
    raise _KIND_TO_EXC[kind](message)


_last_message = [""]


def attempt(fn):
    """Call fn(). Returns 0 when it completed. When it raised one of the std
    errors above, returns that error kind and remembers its message for
    failure_message(). Other exceptions propagate."""
    try:
        fn()
    except StdError as error:
        _last_message[0] = str(error)
        return error.kind
    return 0


def failure_message():
    return _last_message[0]
