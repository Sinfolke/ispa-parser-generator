"""Error kinds understood by the target runtime's `fail` primitive.

Plain int constants so they lower as Enum/Int IR without any class machinery.
Kinds 1-5 are the original set; 6-11 cover the C++ runtime's exception types.
"""

from typing import ClassVar


class ErrorKind:
    OutOfRange: ClassVar[int] = 1
    InvalidArgument: ClassVar[int] = 2
    LengthError: ClassVar[int] = 3
    BadOptionalAccess: ClassVar[int] = 4
    KeyNotFound: ClassVar[int] = 5
    # std::logic_error
    LogicError: ClassVar[int] = 6
    # std::runtime_error
    RuntimeFailure: ClassVar[int] = 7
    # ISPA_STD::IspaInternalExcept (a bug in the generator or the library)
    InternalError: ClassVar[int] = 8
    # ISPA_STD::IspaAPIExcept (a misuse by the caller)
    ApiError: ClassVar[int] = 9
    # LLParser::NoMatch: control flow, an absent optional rule. Only
    # prim.attempt() ever catches it.
    NoMatch: ClassVar[int] = 10
    # ISPA_STD::ExternalParseError
    ExternalParse: ClassVar[int] = 11
