/**
 * ispastdlib/to_string.hpp - to_string_helper / concat
 * Part of the split ISPA std lib; include <ispastdlib.hpp> for everything.
 */
#pragma once
#include <tuple>
#include <ispastdlib/node.hpp>
#include <ispastdlib/raw_token.hpp>
namespace ISPA_STD {
    template<class A, class B>
    std::string to_string_helper(const std::pair<A, B>& value);
    template<class... Ts>
    std::string to_string_helper(const std::tuple<Ts...>& value);


    template<typename Tokens, typename Type>
    std::string to_string_helper(
        const ISPA_STD::Node<Tokens, Type>& node
    );

    template<typename... Ts>
    std::string to_string_helper(
        const std::variant<Ts...>& v
    );
    template<typename Tokens, typename Type>
    std::string to_string_helper(
        const ISPA_STD::Node<Tokens, Type>& node
    );

    template<typename... Ts>
    std::string to_string_helper(
        const std::variant<Ts...>& v
    );

    template<typename T, typename Allocator>
    std::string to_string_helper(
        const std::vector<T, Allocator>& values
    );
    inline std::string to_string_helper(const std::string& val) {
        return val;
    }

    inline std::string to_string_helper(std::string_view val) {
        return std::string(val);
    }

    inline std::string to_string_helper(const char* val) {
        return val ? std::string(val) : std::string();
    }

    // Characters
    inline std::string to_string_helper(char val) {
        return std::string(1, val);
    }

    // Booleans
    inline std::string to_string_helper(bool val) {
        return val ? "true" : "false";
    }

    // Integers
    inline std::string to_string_helper(short val) {
        return std::to_string(val);
    }

    inline std::string to_string_helper(unsigned short val) {
        return std::to_string(val);
    }

    inline std::string to_string_helper(int val) {
        return std::to_string(val);
    }

    inline std::string to_string_helper(unsigned int val) {
        return std::to_string(val);
    }

    inline std::string to_string_helper(long val) {
        return std::to_string(val);
    }

    inline std::string to_string_helper(unsigned long val) {
        return std::to_string(val);
    }

    inline std::string to_string_helper(long long val) {
        return std::to_string(val);
    }

    inline std::string to_string_helper(unsigned long long val) {
        return std::to_string(val);
    }

    // Floating-point
    inline std::string to_string_helper(float val) {
        return std::to_string(val);
    }

    inline std::string to_string_helper(double val) {
        return std::to_string(val);
    }

    inline std::string to_string_helper(long double val) {
        return std::to_string(val);
    }

    // Empty variant alternative
    inline std::string to_string_helper(std::monostate) {
        return "";
    }

    // Node overload
    template<typename Tokens, typename Type>
    std::string to_string_helper(
        const ISPA_STD::Node<Tokens, Type>& node
    ) {
        return node.to_string();
    }
    template <typename EnumT, typename... Ts>
    std::string to_string_helper(const ISPA_STD::RawToken<EnumT, Ts...>& token) {
        return token.to_string();
    }
    // Variant overload
    template<typename... Ts>
    std::string to_string_helper(const std::variant<Ts...>& v) {
        return std::visit(
            [](const auto& arg) -> std::string {
                return to_string_helper(arg);
            },
            v
        );
    }
    template<typename T, typename Allocator>
    std::string to_string_helper(
        const std::vector<T, Allocator>& values
    ) {
        std::string result;

        for (const auto& value : values) {
            result += to_string_helper(value);
        }

        return result;
    }
    template<class A, class B>
    std::string to_string_helper(const std::pair<A, B>& value) {
        return to_string_helper(value.first) + to_string_helper(value.second);
    }
    template<class... Ts>
    std::string to_string_helper(const std::tuple<Ts...>& value) {
        return std::apply([](const auto&... items) {
            return (std::string{} + ... + to_string_helper(items));
        }, value);
    }
    // Base case: zero arguments
    inline std::string concat() {
        return {};
    }

    // Variadic concatenation using C++17 fold expressions
    template <typename... Args>
    std::string concat(Args&&... args) {
        return (to_string_helper(std::forward<Args>(args)) + ...);
    }
} // namespace ISPA_STD
