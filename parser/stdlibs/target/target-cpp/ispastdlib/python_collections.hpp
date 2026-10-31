// Python collection operations used by the language-neutral ArrayMethodCall lowering.
#pragma once
#include <algorithm>
#include <any>
#include <iterator>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <map>
#include <variant>
#include <unordered_map>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace ISPA_STD {
// Character primitives of the 8-bit-unit string model (prim.char_code / prim.char_from_code).
inline long long python_char_code(const std::string& ch) { return ch.empty() ? 0 : static_cast<unsigned char>(ch.front()); }
inline std::string python_char_from_code(long long code) { return code < 0 || code > 255 ? std::string() : std::string(1, static_cast<char>(code)); }
// Cast with Python meaning: an object held as std::any comes back as its dynamic type; other values convert.
template<typename T> T python_cast(const std::any& value) { return std::any_cast<T>(value); }
// A closed value variant (or a type derived from one) yields the alternative named by T; so does a
// RawToken-style tagged storage (member get<T>()), which compiles far faster than a large variant.
namespace cast_detail {
template<class T, class V, class = void> struct has_std_get : std::false_type {};
template<class T, class V> struct has_std_get<T, V, std::void_t<decltype(std::get<T>(std::declval<V>()))>> : std::true_type {};
template<class T, class V, class = void> struct has_member_get : std::false_type {};
template<class T, class V> struct has_member_get<T, V, std::void_t<decltype(std::declval<V>().template get<T>())>> : std::true_type {};
}
template<typename T, typename V, std::enable_if_t<!std::is_same_v<std::decay_t<V>, std::any> && cast_detail::has_std_get<T, V>::value, int> = 0>
T python_cast(V&& value) { return std::get<T>(std::forward<V>(value)); }
template<typename T, typename V, std::enable_if_t<!std::is_same_v<std::decay_t<V>, std::any> && !cast_detail::has_std_get<T, V>::value &&
                                                     cast_detail::has_member_get<T, V>::value, long> = 0>
T python_cast(V&& value) { return std::forward<V>(value).template get<T>(); }
template<typename T, typename V, std::enable_if_t<!std::is_same_v<std::decay_t<V>, std::any> && !cast_detail::has_std_get<T, V>::value &&
                                                     !cast_detail::has_member_get<T, V>::value, char> = 0>
T python_cast(V&& value) { return static_cast<T>(std::forward<V>(value)); }
// `needle in container`: vectors, strings and associative containers (C++17 overloads).
template<class T, class A, class V>
bool python_contains(const std::vector<T, A>& values, const V& needle) {
    return std::find(values.begin(), values.end(), needle) != values.end();
}
template<class K, class T, class H, class E, class A, class V>
bool python_contains(const std::unordered_map<K, T, H, E, A>& values, const V& key) { return values.find(key) != values.end(); }
template<class K, class T, class C, class A, class V>
bool python_contains(const std::map<K, T, C, A>& values, const V& key) { return values.find(key) != values.end(); }
inline bool python_contains(std::string_view text, std::string_view needle) { return text.find(needle) != std::string_view::npos; }
inline std::ptrdiff_t python_bound(std::ptrdiff_t index, std::ptrdiff_t size) {
    if (index < 0) index += size;
    return std::clamp(index, std::ptrdiff_t{0}, size);
}
template<class T, class A, class V>
void python_append(std::vector<T, A>& values, V&& value) { values.push_back(std::forward<V>(value)); }
template<class T, class A, class Range>
void python_extend(std::vector<T, A>& values, const Range& other) {
    // Extending a list with itself is valid Python: snapshot only in that case.
    if constexpr (std::is_same_v<Range, std::vector<T, A>>) {
        if (static_cast<const void*>(&other) == static_cast<const void*>(&values)) {
            std::vector<T, A> copy(other);
            values.insert(values.end(), copy.begin(), copy.end());
            return;
        }
    }
    values.insert(values.end(), std::begin(other), std::end(other));
}
template<class T, class A, class V>
void python_insert(std::vector<T, A>& values, std::ptrdiff_t index, V&& value) {
    values.insert(values.begin() + python_bound(index, values.size()), std::forward<V>(value));
}
template<class T, class A, class V>
void python_remove(std::vector<T, A>& values, const V& value) {
    auto found = std::find(values.begin(), values.end(), value);
    if (found == values.end()) throw std::invalid_argument("list.remove: value is not in list");
    values.erase(found);
}
template<class T, class A>
T python_pop(std::vector<T, A>& values, std::ptrdiff_t index = -1) {
    if (index < 0) index += static_cast<std::ptrdiff_t>(values.size());
    if (index < 0 || static_cast<std::size_t>(index) >= values.size()) throw std::out_of_range("list.pop: index out of range");
    T value = std::move(values[index]);
    values.erase(values.begin() + index);
    return value;
}
template<class C> void python_clear(C& values) { values.clear(); }
template<class C> C python_copy(const C& values) { return values; }
template<class T, class A, class V>
std::ptrdiff_t python_count(const std::vector<T, A>& values, const V& value) {
    return std::count(values.begin(), values.end(), value);
}
template<class T, class A, class V>
std::ptrdiff_t python_index(const std::vector<T, A>& values, const V& value,
    std::ptrdiff_t start = 0, std::ptrdiff_t stop = PTRDIFF_MAX) {
    start = python_bound(start, values.size()); stop = python_bound(stop, values.size());
    for (auto i = start; i < stop; ++i) if (values[i] == value) return i;
    throw std::invalid_argument("list.index: value is not in list");
}
template<class T, class A>
void python_reverse(std::vector<T, A>& values) { std::reverse(values.begin(), values.end()); }
template<class T, class A>
void python_sort(std::vector<T, A>& values, bool reverse = false) {
    std::stable_sort(values.begin(), values.end(), [reverse](const T& a, const T& b) { return reverse ? b < a : a < b; });
}
// Decorate once before sorting, as Python evaluates key exactly once per item.
template<class T, class A, class Key>
void python_sort(std::vector<T, A>& values, bool reverse, Key key) {
    using SortKey = std::decay_t<decltype(key(std::declval<const T&>()))>;
    std::vector<std::pair<SortKey, T>> decorated;
    decorated.reserve(values.size());
    for (const auto& value : values) decorated.emplace_back(key(value), value);
    std::stable_sort(decorated.begin(), decorated.end(), [reverse](const auto& a, const auto& b) {
        return reverse ? b.first < a.first : a.first < b.first;
    });
    for (std::size_t i = 0; i < values.size(); ++i) values[i] = std::move(decorated[i].second);
}
template<class Map, class Key>
auto python_pop(Map& values, const Key& key) -> typename Map::mapped_type {
    auto found = values.find(key);
    if (found == values.end()) throw std::out_of_range("dict.pop: missing key");
    auto value = found->second; values.erase(found); return value;
}
template<class Map, class Key, class Default>
auto python_pop(Map& values, const Key& key, const Default& fallback) -> typename Map::mapped_type {
    auto found = values.find(key);
    if (found == values.end()) return typename Map::mapped_type(fallback);
    auto value = found->second; values.erase(found); return value;
}
template<class Map, class Key, class Default>
auto python_get(const Map& values, const Key& key, const Default& fallback) -> typename Map::mapped_type {
    auto found = values.find(key);
    return found == values.end() ? typename Map::mapped_type(fallback) : found->second;
}
template<class Map, class Key, class Default>
auto python_setdefault(Map& values, const Key& key, const Default& fallback) -> typename Map::mapped_type {
    return values.emplace(key, fallback).first->second;
}
template<class Map> void python_update(Map&) {}
template<class Map, class K, class V, class... Pairs>
void python_update(Map& values, const std::tuple<K, V>& item, const Pairs&... rest) {
    values.insert_or_assign(std::get<0>(item), std::get<1>(item));
    python_update(values, rest...);
}
template<class Map, class K, class V, class... Pairs>
void python_update(Map& values, const std::pair<K, V>& item, const Pairs&... rest) {
    values.insert_or_assign(std::get<0>(item), std::get<1>(item));
    python_update(values, rest...);
}
template<class Map, class Range, class... Pairs>
void python_update(Map& values, const Range& other, const Pairs&... rest) {
    for (const auto& item : other) values.insert_or_assign(std::get<0>(item), std::get<1>(item));
    python_update(values, rest...);
}
template<class Map>
auto python_keys(const Map& values) -> std::vector<typename Map::key_type> {
    std::vector<typename Map::key_type> result;
    for (const auto& item : values) result.push_back(item.first); return result;
}
template<class Map>
auto python_values(const Map& values) -> std::vector<typename Map::mapped_type> {
    std::vector<typename Map::mapped_type> result;
    for (const auto& item : values) result.push_back(item.second); return result;
}
template<class Map>
auto python_items(const Map& values) -> std::vector<std::tuple<typename Map::key_type, typename Map::mapped_type>> {
    std::vector<std::tuple<typename Map::key_type, typename Map::mapped_type>> result;
    for (const auto& item : values) result.emplace_back(item.first, item.second); return result;
}
template<class Range, class V>
auto python_fromkeys(const Range& keys, const V& value) {
    using Value = std::conditional_t<std::is_convertible_v<const V&, std::string_view>, std::string, std::decay_t<V>>;
    std::unordered_map<typename Range::value_type, Value> result;
    for (const auto& key : keys) result.emplace(key, value); return result;
}
template<class Map, class Range, class V>
Map python_fromkeys(const Map&, const Range& keys, const V& value) {
    Map result; for (const auto& key : keys) result.emplace(key, value); return result;
}
} // namespace ISPA_STD
