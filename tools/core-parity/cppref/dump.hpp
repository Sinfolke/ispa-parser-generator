// Runtime-independent rendering of an AST, matching compare.py's dump() on the Python side.
#pragma once
#include <ostream>
#include <string>
#include <variant>
#include <memory>
#include <sstream>
#include <optional>
#include <vector>
#include <map>
#include <algorithm>
#include <type_traits>
#include <ispastdlib.hpp>

template<class T> void dump(std::ostream& os, const T& v);

template<class T> struct is_node : std::false_type {};
template<class T> struct is_node<ISPA_STD::Node<T>> : std::true_type {};
template<class T> struct is_vec : std::false_type {};
template<class T> struct is_vec<std::vector<T>> : std::true_type {};
template<class T> struct is_opt : std::false_type {};
template<class T> struct is_opt<std::optional<T>> : std::true_type {};
template<class T> struct is_var : std::false_type {};
template<class... T> struct is_var<std::variant<T...>> : std::true_type {};
template<class T> struct is_pair : std::false_type {};
template<class A, class B> struct is_pair<std::pair<A, B>> : std::true_type {};

template<class T> struct is_ptr : std::false_type {};
template<class T> struct is_ptr<std::shared_ptr<T>> : std::true_type {};
template<class T> struct is_ptr<std::unique_ptr<T>> : std::true_type {};

template<class T> concept HasMembers = requires(const T& t) { t.members(); T::members_str(); };

template<class T> void dump(std::ostream& os, const T& v) {
    if constexpr (std::is_same_v<T, bool>) os << (v ? "True" : "False");
    else if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>) os << v;
    else if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, ISPA_STD::Text>) {
        os << "'";
        for (char c : v) {
            switch (c) { case '\\': os << "\\\\"; break; case '\'': os << "\\'"; break; case '\n': os << "\\n"; break;
                         case '\r': os << "\\r"; break; case '\t': os << "\\t"; break; default: os << c; }
        }
        os << "'";
    }
    else if constexpr (is_node<T>::value) {
        if (v.empty()) { os << "None"; return; }
        long long start = v.startpos() == std::string::npos ? -1 : (long long)v.startpos();
        os << "Node@" << start << "+" << v.length() << "(";
        dump(os, v.data());
        os << ")";
    } else if constexpr (is_vec<T>::value) {
        os << "[";
        for (std::size_t i = 0; i < v.size(); ++i) { if (i) os << ", "; dump(os, v[i]); }
        os << "]";
    } else if constexpr (is_opt<T>::value) { if (v) dump(os, *v); else os << "None"; }
    else if constexpr (is_ptr<T>::value) { if (v) dump(os, *v); else os << "None"; }
    else if constexpr (is_var<T>::value) std::visit([&](const auto& x) { dump(os, x); }, v);
    else if constexpr (is_pair<T>::value) { os << "["; dump(os, v.first); os << ", "; dump(os, v.second); os << "]"; }
    else if constexpr (HasMembers<T>) {
        std::string name = typeid(T).name();
        os << "{";
        constexpr auto names = T::members_str();
        std::vector<std::pair<std::string, std::string>> fields;
        std::size_t i = 0;
        std::apply([&](const auto&... m) {
            ((fields.emplace_back(std::string(names[i++]), [&] { std::ostringstream s; dump(s, m); return s.str(); }())), ...);
        }, v.members());
        std::sort(fields.begin(), fields.end());
        for (std::size_t k = 0; k < fields.size(); ++k) os << (k ? ", " : "") << fields[k].first << "=" << fields[k].second;
        os << "}";
    } else os << "?";
}
