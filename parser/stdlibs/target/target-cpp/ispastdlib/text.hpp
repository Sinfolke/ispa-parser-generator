/**
 * ispastdlib/text.hpp - Text: a non-owning view of source text (generated code with --text-view)
 * Part of the split ISPA std lib; include <ispastdlib.hpp> for everything.
 *
 * Token and rule fields hold a pointer and a length into the lexer's input instead of a copy. The input must stay alive
 * as long as the AST; an owning std::string is made only where one is demanded (conversion operator below).
 */
#pragma once
#include <cstddef>
#include <functional>
#include <ostream>
#include <string>
#include <string_view>

namespace ISPA_STD {
    class Text {
        const char* _data = "";
        std::size_t _size = 0;
    public:
        constexpr Text() = default;
        constexpr Text(const char* data, std::size_t size) : _data(data), _size(size) {}
        constexpr Text(std::string_view view) : _data(view.data()), _size(view.size()) {}
        Text(const char* literal) : _data(literal ? literal : ""), _size(literal ? std::char_traits<char>::length(literal) : 0) {}
        // A temporary or a local owner would leave the view dangling: such code needs the owning (copy) mode.
        Text(const std::string&) = delete;
        Text(std::string&&) = delete;
        Text& operator=(const std::string&) = delete;
        Text& operator=(std::string&&) = delete;

        constexpr const char* data() const { return _data; }
        constexpr std::size_t size() const { return _size; }
        constexpr std::size_t length() const { return _size; }
        constexpr bool empty() const { return _size == 0; }
        constexpr const char* begin() const { return _data; }
        constexpr const char* end() const { return _data + _size; }
        constexpr char operator[](std::size_t i) const { return _data[i]; }
        constexpr std::string_view view() const { return {_data, _size}; }
        constexpr operator std::string_view() const { return view(); }
        // the copy: made only where an owning string is required
        operator std::string() const { return std::string(_data, _size); }
        std::string str() const { return std::string(_data, _size); }

        friend bool operator==(const Text& a, const Text& b) { return a.view() == b.view(); }
        friend bool operator!=(const Text& a, const Text& b) { return a.view() != b.view(); }
        friend bool operator<(const Text& a, const Text& b) { return a.view() < b.view(); }
        friend bool operator==(const Text& a, std::string_view b) { return a.view() == b; }
        friend bool operator!=(const Text& a, std::string_view b) { return a.view() != b; }
        friend bool operator==(std::string_view a, const Text& b) { return a == b.view(); }
        friend bool operator!=(std::string_view a, const Text& b) { return a != b.view(); }
        friend bool operator==(const Text& a, const std::string& b) { return a.view() == b; }
        friend bool operator==(const std::string& a, const Text& b) { return a == b.view(); }
        friend bool operator==(const Text& a, const char* b) { return a.view() == b; }
        friend std::ostream& operator<<(std::ostream& os, const Text& value) { return os << value.view(); }
        friend std::string operator+(const Text& a, const Text& b) { return a.str() + b.str(); }
        friend std::string operator+(const std::string& a, const Text& b) { return a + b.str(); }
        friend std::string operator+(const Text& a, const std::string& b) { return a.str() + b; }
        friend std::string operator+(const char* a, const Text& b) { return a + b.str(); }
        friend std::string operator+(const Text& a, const char* b) { return a.str() + b; }
    };

    inline std::string to_string_helper(const Text& value) { return value.str(); }
}

namespace std {
    template<> struct hash<ISPA_STD::Text> {
        std::size_t operator()(const ISPA_STD::Text& value) const noexcept { return std::hash<std::string_view>{}(value.view()); }
    };
}
