/*
 * ispastdlib/failure.hpp - native side of the lowered core's error primitives
 * (ispa_std/prim.py: fail, attempt, failure_message). ErrorKind numbers mirror ispa_std/errors.py.
 * Standard kinds throw the matching std exception so callers outside the lowered core see the
 * same exception types as before; the remaining kinds travel as StdError.
 */
#pragma once
#include <exception>
#include <functional>
#include <new>
#include <stdexcept>
#include <string>
#include <optional>

namespace ISPA_STD {
enum ErrorKindCode : int {
    EK_OutOfRange = 1, EK_InvalidArgument = 2, EK_LengthError = 3, EK_BadOptionalAccess = 4, EK_KeyNotFound = 5,
    EK_LogicError = 6, EK_RuntimeFailure = 7, EK_InternalError = 8, EK_ApiError = 9, EK_NoMatch = 10, EK_ExternalParse = 11
};

// Carrier for the kinds that have no std exception (control flow and ISPA-specific failures).
struct StdError : std::runtime_error {
    int kind;
    StdError(int k, const std::string& m) : std::runtime_error(m), kind(k) {}
};

[[noreturn]] inline void fail(long long kind, const std::string& message) {
    switch (kind) {
        case EK_OutOfRange: throw std::out_of_range(message);
        case EK_InvalidArgument: throw std::invalid_argument(message);
        case EK_LengthError: throw std::length_error(message);
        case EK_LogicError: throw std::logic_error(message);
        case EK_RuntimeFailure: throw std::runtime_error(message);
        default: throw StdError(static_cast<int>(kind), message);
    }
}

namespace detail {
inline std::string& last_failure_message() { static thread_local std::string message; return message; }
}

// Runs fn. Returns 0 on completion, else the ErrorKind of what fn raised (message kept for failure_message()).
inline long long attempt(const std::function<void()>& fn) {
    try { fn(); return 0; }
    catch (const StdError& e) { detail::last_failure_message() = e.what(); return e.kind; }
    catch (const std::out_of_range& e) { detail::last_failure_message() = e.what(); return EK_OutOfRange; }
    catch (const std::invalid_argument& e) { detail::last_failure_message() = e.what(); return EK_InvalidArgument; }
    catch (const std::length_error& e) { detail::last_failure_message() = e.what(); return EK_LengthError; }
    catch (const std::bad_optional_access& e) { detail::last_failure_message() = e.what(); return EK_BadOptionalAccess; }
    catch (const std::logic_error& e) { detail::last_failure_message() = e.what(); return EK_LogicError; }
    catch (const std::runtime_error& e) { detail::last_failure_message() = e.what(); return EK_RuntimeFailure; }
}

inline std::string failure_message() { return detail::last_failure_message(); }
}
