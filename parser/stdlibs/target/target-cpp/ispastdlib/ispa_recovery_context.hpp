/**
 * ispastdlib/ispa_recovery_context.hpp
 * Types for hierarchical (context) error recovery used by LLParser.
 * Kept separate from ispa_recovery.hpp so it does not depend on the layout of
 * Recovery::Diagnostic / Recovery::State.
 */
#pragma once
#include <cstddef>
#include <limits>
#include <string>
#include <string_view>

namespace ISPA_STD::Recovery {

    enum class ContextKind {
        Mismatch,   // required member missing; skipped to a resume point
        Misplaced,  // token starts a construct owned by an outer frame
        Stray       // token after a loop that no active frame can accept
    };

    // A production (or terminal) that an active frame could start / continue
    // with at the current position. `matches` receives a `const Token*`.
    struct ContextClaim {
        std::string_view name;
        bool (*matches)(const void*) = nullptr;
    };

    struct ContextDiagnostic {
        ContextKind kind = ContextKind::Mismatch;
        std::string message;
        std::size_t position = 0;        // cursor position where the error was detected
        std::size_t offset = std::numeric_limits<std::size_t>::max(); // source byte offset, when available
        std::size_t length = 0;          // offending token length
        std::size_t skipped = 0;         // tokens skipped to reach the resume point
        std::string rule;                // innermost rule when the error was detected
        std::string resume_rule;         // frame that resumes parsing
        std::string claim;               // production/terminal that resumes (empty at EOF)
    };

} // namespace ISPA_STD::Recovery
