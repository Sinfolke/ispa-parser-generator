#pragma once
#include <algorithm>
#include <string_view>
#include <vector>

namespace ISPA_STD::Recovery {
struct ScanResult {
    const char* end;
    bool exhausted = false;
};

// Scan opaque Python payloads without asking the host lexer to tokenize them.
// The budget is in source bytes, including strings and comments.
template<class Boundaries>
inline ScanResult scanBalanced(const char* begin, const Boundaries& boundaries,
                               std::size_t budget) {
    const char* p = begin;
    std::vector<char> closers;
    char quote = 0;
    bool triple = false, comment = false;
    auto advance = [&](std::size_t n) {
        if (n > budget - static_cast<std::size_t>(p - begin)) return false;
        p += n;
        return true;
    };
    while (*p) {
        if (static_cast<std::size_t>(p - begin) >= budget) return {p, true};
        const char c = *p;
        if (comment) {
            if (c == '\n' || c == '\r') comment = false;
        } else if (quote) {
            if (c == '\\' && p[1]) {
                if (!advance(2)) return {p, true};
                continue;
            }
            if (c == quote && (!triple || (p[1] == quote && p[2] == quote))) {
                if (!advance(triple ? 3 : 1)) return {p, true};
                quote = 0;
                continue;
            }
            // A broken single-line string must not swallow later declarations.
            if (!triple && (c == '\n' || c == '\r')) return {p, true};
        } else {
            for (auto boundary : boundaries) {
                bool matches = true;
                for (std::size_t i = 0; i < boundary.size(); ++i)
                    if (!p[i] || p[i] != boundary[i]) { matches = false; break; }
                if ((closers.empty() || boundary == ";") && matches) return {p};
            }
            if (c == ')' || c == ']' || c == '}') {
                if (closers.empty() || closers.back() != c) return {p};
                closers.pop_back();
            } else if (c == '(') closers.push_back(')');
            else if (c == '[') closers.push_back(']');
            else if (c == '{') closers.push_back('}');
            else if (c == '#') comment = true;
            else if (c == '\'' || c == '"') {
                quote = c;
                triple = p[1] == c && p[2] == c;
                if (!advance(triple ? 3 : 1)) return {p, true};
                continue;
            }
        }
        if (!advance(1)) return {p, true};
    }
    return {p, quote != 0};
}
}
