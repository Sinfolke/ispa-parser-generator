/**
 * ISPA LL recovery runtime (C++17).
 * Only invoked after a committed required match fails.  Never invoke from
 * optional/probing/backtracking alternatives.
 */
#pragma once
#include <ispastdlib/node.hpp>

#include <cstdint>
#include <cstddef>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>
#include <algorithm>

namespace ISPA_STD::Recovery {

using TokenId = std::size_t;
constexpr TokenId eof_token = std::numeric_limits<TokenId>::max();

enum class Mode : std::uint8_t {
    PanicUntil,    // Skip input to synchronization set; caller handles construct
    Insert,        // Synthesize expected terminal; do NOT consume current token
    Delete,        // Consume one unexpected terminal, then retry required match
    Replace,       // Consume wrong terminal, synthesize expected terminal
    Retry,         // Explicit custom-handler request, only after changing state
    Synchronize,   // Already at a viable continuation boundary; keep its token
    Propagate,     // Current construct cannot recover: ask parent rule to recover
    Abort          // Error budget exceeded or explicitly requested
};

struct Decision {
    Mode mode = Mode::Propagate;
    TokenId synthesized = eof_token;
    TokenId unexpected = eof_token;
    std::size_t skipped = 0;
};

struct Diagnostic {
    std::string rule;
    TokenId unexpected = eof_token;
    std::vector<TokenId> expected;
    Decision decision;
    std::string expected_text; // supplied by typed handler; optional for raw API
    std::size_t offset = std::numeric_limits<std::size_t>::max();
    std::size_t length = 0;
    std::string message;
    std::string code;
    std::vector<std::string> notes;
};

struct State {
    std::size_t max_errors = 32;
    std::size_t max_total_skipped = 2048;
    std::size_t total_skipped = 0;
    std::vector<Diagnostic> diagnostics;
    // Active call-site suffixes, including terminals inside nested productions.
    std::vector<std::vector<TokenId>> anchors;

    // Custom handlers can return Abort without modifying State.
    bool exhausted() const noexcept {
        return diagnostics.size() >= max_errors ||
               total_skipped >= max_total_skipped;
    }
};

struct Plan {
    std::string_view rule;
    std::vector<TokenId> expected;
    std::vector<TokenId> continuation; // FIRST of viable continuation at this site
    std::vector<TokenId> follow;       // rule-call contextual FOLLOW if available
    std::vector<TokenId> explicit_sync;
    bool allow_insertion = true;
    bool allow_deletion = true;
    bool allow_replacement = true;
    std::size_t max_skip = 64;
};

inline bool contains(const std::vector<TokenId>& tokens, TokenId id) {
    return std::find(tokens.begin(), tokens.end(), id) != tokens.end();
}

// A compact error-path-only encoding used by LangAPI generated fail wrappers.
// Accepts a comma-separated list of unsigned decimal token IDs.
inline std::vector<TokenId> decode(std::string_view encoded) {
    std::vector<TokenId> result;
    std::size_t i = 0;
    while (i < encoded.size()) {
        while (i < encoded.size() && (encoded[i] == ',' || encoded[i] == ' ')) ++i;
        if (i == encoded.size()) break;
        std::uint64_t value = 0;
        bool has_digit = false;
        while (i < encoded.size() && encoded[i] >= '0' && encoded[i] <= '9') {
            has_digit = true;
            value = value * 10 + static_cast<unsigned>(encoded[i++] - '0');
            if (value >= eof_token)
                throw std::invalid_argument("ISPA recovery: token ID out of range");
        }
        if (!has_digit || (i < encoded.size() && encoded[i] != ',' && encoded[i] != ' '))
            throw std::invalid_argument("ISPA recovery: malformed token set");
        result.push_back(static_cast<TokenId>(value));
    }
    return result;
}

// Default token access for RawToken and Node-based token streams.
// A custom token_id function can instead be passed to recover(...).
template <class Token>
TokenId tokenId(const Token& token) {
    if constexpr (has_meta_name_v<Token>) {
        return static_cast<TokenId>(token.meta().name());
    } else if constexpr (has_name_v<Token>) {
        return static_cast<TokenId>(token.name());
    } else if constexpr (has_index_v<Token>) {
        return static_cast<TokenId>(token.index());
    } else {
        static_assert(sizeof(Token) == 0,
            "ISPA recovery: token must have meta().name(), name(), or index()");
    }
}

template<class IT, class TokenKind>
Decision recover(IT& pos, State& state, const Plan& plan, TokenKind&& token_kind) {
    const auto kind = [&](const IT& p) -> TokenId {
        if constexpr (detail::detected_v<detail::is_end, IT>)
            return p.isEnd() ? eof_token : token_kind(*p);
        else
            return p->isEnd() ? eof_token : token_kind(*p);
    };
    const TokenId at = kind(pos);
    const auto anchor = [&](TokenId id) {
        if (contains(plan.explicit_sync, id)) return true;
        for (const auto& tokens : state.anchors)
            if (contains(tokens, id)) return true;
        return false;
    };
    std::size_t offset = std::numeric_limits<std::size_t>::max(), length = 0;
    std::string actual = "end of input";
    if (at != eof_token) {
        if constexpr (detail::detected_v<detail::meta_startpos_length, decltype(*pos)>) {
            offset = (*pos).meta().startpos();
            length = (*pos).meta().length();
        }
        if constexpr (detail::detected_v<detail::meta_start_length, decltype(*pos)>) {
            if ((*pos).meta().start())
                actual = "'" + std::string((*pos).meta().start(), (*pos).meta().length()) + "'";
            else actual = "token " + std::to_string(at);
        } else actual = "token " + std::to_string(at);
    }
    Decision decision;
    decision.mode = Mode::Propagate;
    decision.unexpected = at;

    if (state.exhausted()) {
        decision.mode = Mode::Abort;
    } else if (contains(plan.expected, at)) {
        // An explicit custom repair may have changed the input before this call.
        decision.mode = Mode::Retry;
    } else {
        TokenId next = eof_token;
        if (at != eof_token) {
            IT lookahead = pos;
            ++lookahead;
            next = kind(lookahead);
        }

        const bool expected_one = plan.expected.size() == 1;
        // Do not delete enclosing delimiters / caller-owned follow tokens.
        const bool protected_token =
            contains(plan.follow, at) || anchor(at);

        if (plan.allow_deletion && !protected_token && at != eof_token &&
            contains(plan.expected, next)) {
            ++pos;
            decision.mode = Mode::Delete;
            decision.skipped = 1;
        } else if (plan.allow_insertion && expected_one &&
                   (contains(plan.continuation, at) || contains(plan.follow, at))) {
            decision.mode = Mode::Insert;
            decision.synthesized = plan.expected.front();
        } else if (plan.allow_replacement && expected_one && !protected_token &&
                   at != eof_token && contains(plan.continuation, next)) {
            ++pos;
            decision.mode = Mode::Replace;
            decision.synthesized = plan.expected.front();
            decision.skipped = 1;
        } else if (contains(plan.follow, at) || at == eof_token) {
            // Let the enclosing rule consume this boundary token.
            decision.mode = Mode::Propagate;
        } else if (contains(plan.continuation, at) ||
                   anchor(at)) {
            // No input consumed; enclosing rule handles the incomplete construct.
            decision.mode = Mode::Synchronize;
        } else {
            // Panic: consume malformed tokens but never consume the boundary.
            while (kind(pos) != eof_token && decision.skipped < plan.max_skip &&
                   state.total_skipped + decision.skipped < state.max_total_skipped) {
                const auto id = kind(pos);
                if (contains(plan.expected, id) ||
                    contains(plan.continuation, id) || contains(plan.follow, id) ||
                    anchor(id))
                    break;
                ++pos;
                ++decision.skipped;
            }
            if (contains(plan.expected, kind(pos))) {
                decision.mode = Mode::Retry;
            } else if (kind(pos) == eof_token || decision.skipped == 0) {
                decision.mode = Mode::Propagate;
            } else if (contains(plan.continuation, kind(pos)) ||
                       contains(plan.follow, kind(pos)) || anchor(kind(pos))) {
                decision.mode = Mode::PanicUntil;
                // PanicUntil means a boundary was located, NOT a successful match.
            } else {
                decision.mode = Mode::Propagate;
            }
        }
    }

    state.total_skipped += decision.skipped;
    state.diagnostics.push_back(Diagnostic{
        std::string(plan.rule), at, plan.expected, decision, {}, offset, length
    });
    state.diagnostics.back().message = "unexpected " + actual + " in " + std::string(plan.rule);
    if (state.diagnostics.size() >= state.max_errors ||
        state.total_skipped >= state.max_total_skipped)
        decision.mode = Mode::Abort;
    state.diagnostics.back().decision = decision;
    return decision;
}

template<class IT>
Decision recover(IT& pos, State& state, const Plan& plan) {
    return recover(pos, state, plan, [](const auto& token) {
        return tokenId(token);
    });
}

template<class IT>
Decision recoverEncoded(
    IT& pos, State& state, std::string_view rule,
    std::string_view expected, std::string_view continuation,
    std::string_view follow, std::string_view explicit_sync,
    bool allow_insertion = true, bool allow_deletion = true,
    bool allow_replacement = true, std::size_t max_skip = 64
) {
    return recover(pos, state, Plan{
        rule, decode(expected), decode(continuation), decode(follow),
        decode(explicit_sync), allow_insertion, allow_deletion,
        allow_replacement, max_skip
    });
}

// Typed, generated failure-handler API. These templates correspond to the
// LangAPI expression emitted by ParserBase::recoveryHandler(). Token values
// are numeric (potentially scoped enums), not C++ types.
inline constexpr TokenId EndOfInput = eof_token;

template<auto... Values>
struct Tokens {
    static auto values() -> std::vector<TokenId> {
        return {static_cast<TokenId>(Values)...};
    }
    template<class Token>
    static bool matches(const Token& token) {
        return ((tokenId(token) == static_cast<TokenId>(Values)) || ...);
    }
};

using EmptyTokens = Tokens<>;

template<bool AllowInsertion = true,
         bool AllowDeletion = true,
         bool AllowReplacement = true,
         std::size_t MaxSkip = 64>
struct Policy {
    static_assert(MaxSkip > 0, "Recovery::Policy MaxSkip must be positive");
    static constexpr bool allow_insertion = AllowInsertion;
    static constexpr bool allow_deletion = AllowDeletion;
    static constexpr bool allow_replacement = AllowReplacement;
    static constexpr std::size_t max_skip = MaxSkip;
};

template<class Expected, class Continuation, class Follow,
         class Synchronization, class RecoveryPolicy>
struct Handler {
    std::string rule;
    std::string expected_text;

    template<class IT>
    Decision operator()(IT& pos, State& state) const {
        Plan plan{
            rule,
            Expected::values(),
            Continuation::values(),
            Follow::values(),
            Synchronization::values(),
            RecoveryPolicy::allow_insertion,
            RecoveryPolicy::allow_deletion,
            RecoveryPolicy::allow_replacement,
            RecoveryPolicy::max_skip
        };
        auto decision = recover(pos, state, plan);
        if (!state.diagnostics.empty())
            state.diagnostics.back().expected_text = expected_text;
        if (!state.diagnostics.empty()) {
            auto& diagnostic = state.diagnostics.back();
            diagnostic.message += "; expected " + expected_text;
            switch (decision.mode) {
            case Mode::Insert: diagnostic.message += "; inserted missing token"; break;
            case Mode::Replace: diagnostic.message += "; replaced unexpected token"; break;
            case Mode::Delete: diagnostic.message += "; deleted unexpected token"; break;
            case Mode::Abort: diagnostic.message += "; recovery limit reached"; break;
            default: break;
            }
            if (decision.skipped > 1)
                diagnostic.message += "; skipped " + std::to_string(decision.skipped) + " tokens";
        }
        return decision;
    }
};

template<class Expected, class Continuation, class Follow,
         class Synchronization, class RecoveryPolicy>
auto handler(std::string_view rule, std::string_view expected_text) {
    return Handler<Expected, Continuation, Follow,
                   Synchronization, RecoveryPolicy>{
        std::string(rule), std::string(expected_text)
    };
}

// Unlike normal optional/match results, recovered required values can lack a
// concrete Node.  The caller must handle Missing/Boundary rather than using
// a default Node as if it were valid.
enum class RequiredStatus { Matched, Missing, Boundary, Aborted };

template<class Node>
struct RequiredResult {
    RequiredStatus status = RequiredStatus::Boundary;
    std::optional<Node> node;
    Decision recovery;
};

} // namespace ISPA_STD::Recovery
