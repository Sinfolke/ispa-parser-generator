"""Host exception boundary for lowered ispa_std code."""
from threading import local


class StdError(RuntimeError):
    def __init__(self, kind: int, message: str):
        super().__init__(message)
        self.kind = kind


_state = local()


def fail(kind: int, message: str):
    raise StdError(kind, message)


def attempt(fn):
    """Catch only failures raised by the shared runtime, preserving host errors."""
    try:
        fn()
    except StdError as error:
        _state.message = str(error)
        return error.kind
    return 0


def failure_message():
    return getattr(_state, 'message', '')
