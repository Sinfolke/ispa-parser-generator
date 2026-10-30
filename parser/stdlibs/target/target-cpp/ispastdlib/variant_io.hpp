/**
 * ispastdlib/variant_io.hpp - generic operator<< for std::variant (OPT-IN).
 * Streaming a big Token variant instantiates operator<< for every alternative in the including TU.
 * Prefer a single out-of-line `std::ostream& operator<<(std::ostream&, const Token&)` in one .cpp.
 */
#pragma once
#include <ostream>
#include <variant>
#include <type_traits>

namespace std {
    template<typename T, typename... Ts>
    auto operator<<(
        std::ostream& os,
        const std::variant<T, Ts...>& v
    ) -> std::ostream& {
        std::visit([&os](const auto& arg) {
            using Arg = std::decay_t<decltype(arg)>;

            if constexpr (!std::is_same_v<Arg, std::monostate>) {
                os << arg;
            }
        }, v);

        return os;
    }
}
