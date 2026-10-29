"""Named wrappers over prim.fail, one per C++ exception type the runtime throws.

Every std error in the library goes through here, so the compiler boundary
stays the single `prim.fail` (plus `prim.attempt` for the three places the C++
code catches an exception)."""
from ispa_std import prim
from ispa_std.errors import ErrorKind
from ispa_std.format import fmt


def logic_error(message: str) -> None:
    prim.fail(ErrorKind.LogicError, message)


def runtime_error(message: str) -> None:
    prim.fail(ErrorKind.RuntimeFailure, message)


def out_of_range(message: str) -> None:
    prim.fail(ErrorKind.OutOfRange, message)


def invalid_argument(message: str) -> None:
    prim.fail(ErrorKind.InvalidArgument, message)


def length_error(message: str) -> None:
    prim.fail(ErrorKind.LengthError, message)


def api_error(message: str, args: list) -> None:
    # IspaAPIExcept: `message` uses sequential {} placeholders.
    prim.fail(ErrorKind.ApiError, fmt(message, args))


def internal_error(message: str, issue: str, ispa_side: bool, args: list) -> None:
    # IspaInternalExcept carried (issue, ispa_side) next to the text; the
    # primitive carries one string, so the classification is a bracketed prefix.
    origin = "Generated parser"
    if ispa_side:
        origin = "ISPA standard library"
    prim.fail(ErrorKind.InternalError, "[" + issue + " | " + origin + "] " + fmt(message, args))


def no_match() -> None:
    prim.fail(ErrorKind.NoMatch, "no match")


def external_parse_error(message: str) -> None:
    prim.fail(ErrorKind.ExternalParse, message)


def rethrow(kind: int, message: str) -> None:
    prim.fail(kind, message)
