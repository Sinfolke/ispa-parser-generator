// std_prims.hpp - C++ implementation of the `prim` boundary used by ispa_std.
// Mirrors ispa_std/errors.py (ErrorKind) and prim_cpython.py (fail / attempt).
// Use it when ispa_std's Python-subset modules are lowered to C++ by the generator.
#pragma once
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace ispa_std_prims {
enum ErrorKind : int {
    OutOfRange = 1, InvalidArgument = 2, LengthError = 3, BadOptionalAccess = 4, KeyNotFound = 5,
    LogicError = 6, RuntimeFailure = 7, InternalError = 8, ApiError = 9, NoMatch = 10, ExternalParse = 11
};

// One carrier for every kind so `attempt` can report [kind, message] uniformly.
struct StdError : std::runtime_error {
    int kind;
    StdError(int k, const std::string& m) : std::runtime_error(m), kind(k) {}
};

[[noreturn]] inline void fail(int kind, const std::string& message) { throw StdError(kind, message); }

// Runs fn. true = completed. false = a StdError was thrown; report = {kind-as-text, message}.
// Any other exception propagates (matches prim_cpython.attempt).
inline bool attempt(const std::function<void()>& fn, std::vector<std::string>& report) {
    try { fn(); }
    catch (const StdError& e) {
        report.clear();
        report.push_back(std::to_string(e.kind));
        report.push_back(e.what());
        return false;
    }
    return true;
}
} // namespace ispa_std_prims
