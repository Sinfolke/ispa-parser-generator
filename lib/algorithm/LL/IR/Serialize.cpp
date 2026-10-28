module;
#include <tsl/ordered_map.h>
module LLIR.Serialize;
import corelib;
import LangAPI.Accessor;
import std;

namespace LLIR::Serialize {
using namespace ParserIR;
namespace detail {

template<class> inline constexpr bool always_false = false;

template<class T> struct is_vector : std::false_type {};
template<class T> struct is_vector<stdu::vector<T>> : std::true_type {};
template<class T> struct is_pair : std::false_type {};
template<class A, class B> struct is_pair<std::pair<A,B>> : std::true_type {};
template<class T> struct is_optional : std::false_type {};
template<class T> struct is_optional<std::optional<T>> : std::true_type {};
template<class T> struct is_shared_ptr : std::false_type {};
template<class T> struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};
template<class T> struct is_variant : std::false_type {};
template<class... T> struct is_variant<std::variant<T...>> : std::true_type {};
template<class T> struct is_ordered_map : std::false_type {};
template<class K, class V> struct is_ordered_map<tsl::ordered_map<K,V>> : std::true_type {};
template<class T> struct is_unordered_map : std::false_type {};
template<class K, class V, class H, class E, class A> struct is_unordered_map<std::unordered_map<K,V,H,E,A>> : std::true_type {};

template<typename T>
concept Reflected = requires(T& value, const T& const_value) {
    LangAPI::MembersAccess::get(value);
    LangAPI::MembersAccess::get(const_value);
};

// These LangAPI reflection tuples omit template arguments. Persistence must
// retain them even though existing hashing/traversal uses the shorter tuples.
template<class T> auto persistedMembers(T& value) {
    using U = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<U, LangAPI::IspaLibFunctionCall>)
        return std::tie(value.symbol, value.args, value.template_parameters);
    else if constexpr (std::is_same_v<U, LangAPI::IspaLibSymbol>)
        return std::tie(value.exports, value.template_parameters, value.Const, value.Reference);
    else if constexpr (std::is_same_v<U, LangAPI::FixedSizeArray>)
        return std::tie(value.values, value.template_parameters);
    else
        return LangAPI::MembersAccess::get(value);
}

// One dispatch function for every type: recursive calls are never affected by
// two-phase lookup / overload declaration order.
template<class T> void write(std::ostream& os, const T& value);
template<class T> void read(std::istream& is, T& value);

inline thread_local bool hit_dfadebug = false;

template<class Variant, std::size_t I = 0>
void read_variant(std::istream& is, Variant& value, std::size_t index) {
    if constexpr (I == std::variant_size_v<Variant>) {
        is.setstate(std::ios::failbit);
    } else if (index == I) {
        using Alt = std::variant_alternative_t<I, Variant>;
        if constexpr (std::is_same_v<Alt, std::monostate>) {
            value = std::monostate{};
        } else {
            Alt item{};
            read(is, item);
            if (is) value = std::move(item);
        }
    } else {
        read_variant<Variant, I + 1>(is, value, index);
    }
}

template<class T> void write(std::ostream& os, const T& value) {
    using U = std::remove_cvref_t<T>;
    // special-cased types: fail the build if they grow
    static_assert(sizeof(LangAPI::Symbol) == sizeof(decltype(LangAPI::Symbol::path)),
                  "Symbol changed: update Serialize");
    if constexpr (std::is_same_v<U, LangAPI::DFADebug>)
        hit_dfadebug = true;   // not fully serializable -> record gets skipped
    if constexpr (std::is_same_v<U, std::string>) {
        const std::size_t length = value.size();
        write(os, length);
        os.write(value.data(), static_cast<std::streamsize>(length));
    } else if constexpr (std::is_enum_v<U>) {
        write(os, static_cast<std::underlying_type_t<U>>(value));
    } else if constexpr (is_vector<U>::value) {
        write(os, value.size());
        for (const auto& item : value) {
            if constexpr (std::is_same_v<typename U::value_type, bool>) write(os, static_cast<bool>(item));
            else write(os, item);
        }
    } else if constexpr (is_pair<U>::value) {
        write(os, value.first); write(os, value.second);
    } else if constexpr (is_shared_ptr<U>::value || is_optional<U>::value) {
        const bool has = static_cast<bool>(value);
        write(os, has);
        if (has) write(os, *value);
    } else if constexpr (is_variant<U>::value) {
        write(os, value.index());
        std::visit([&](const auto& alt) {
            if constexpr (!std::is_same_v<std::remove_cvref_t<decltype(alt)>, std::monostate>)
                write(os, alt);
        }, value);
    } else if constexpr (is_ordered_map<U>::value || is_unordered_map<U>::value) {
        write(os, value.size());
        for (const auto& [key, val] : value) {
            write(os, key); write(os, val);
        }
    } else if constexpr (Reflected<U>) {
        // This assumes members() returns a tuple-like sequence of field REFERENCES.
        std::apply([&](const auto&... fields) { (write(os, fields), ...); }, persistedMembers(value));
    } else if constexpr (std::is_trivially_copyable_v<U> && !std::is_pointer_v<U>) {
        os.write(reinterpret_cast<const char*>(&value), sizeof(U));
    } else {
        static_assert(always_false<U>, "No serializer: implement tuple-of-references members() or add a special case");
    }
}

template<class T> void read(std::istream& is, T& value) {
    using U = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<U, std::string>) {
        std::size_t length{};
        read(is, length);
        if (!is || length > value.max_size() || length > static_cast<std::size_t>(is.rdbuf()->in_avail())) { is.setstate(std::ios::failbit); return; }
        value.resize(length);
        if (length) is.read(value.data(), static_cast<std::streamsize>(length));
    } else if constexpr (std::is_enum_v<U>) {
        std::underlying_type_t<U> underlying{};
        read(is, underlying);
        if (is) value = static_cast<U>(underlying);
    } else if constexpr (is_vector<U>::value) {
        std::size_t n{};
        read(is, n);
        if (!is || n > value.max_size() || n > 1000000) { is.setstate(std::ios::failbit); return; }
        value.clear(); value.reserve(n);
        for (std::size_t i = 0; i < n && is; ++i) {
            typename U::value_type item{};
            read(is, item);
            if (is) value.push_back(std::move(item));
        }
    } else if constexpr (is_pair<U>::value) {
        read(is, value.first); read(is, value.second);
    } else if constexpr (is_shared_ptr<U>::value || is_optional<U>::value) {
        bool has{};
        read(is, has);
        if (!is) return;
        if (!has) { value.reset(); return; }
        if constexpr (is_shared_ptr<U>::value) {
            using Pointee = typename U::element_type;
            auto ptr = std::make_shared<Pointee>();
            read(is, *ptr);
            if (is) value = std::move(ptr);
        } else {
            typename U::value_type item{};
            read(is, item);
            if (is) value = std::move(item);
        }
    } else if constexpr (is_variant<U>::value) {
        std::size_t index{};
        read(is, index);
        if (is) read_variant(is, value, index);
    } else if constexpr (is_ordered_map<U>::value || is_unordered_map<U>::value) {
        std::size_t n{};
        read(is, n);
        if (!is || n > 1000000) { is.setstate(std::ios::failbit); return; }
        value.clear();
        for (std::size_t i = 0; i < n && is; ++i) {
            typename U::key_type key{};
            typename U::mapped_type val{};
            read(is, key); read(is, val);
            if (is) value.try_emplace(std::move(key), std::move(val));
        }
    } else if constexpr (Reflected<U>) {
        auto fields = persistedMembers(value);

        std::apply([&](auto&... field) {
            static_assert(
                ((!std::is_const_v<
                    std::remove_reference_t<decltype(field)>
                >) && ...),
                "members() returns const references during deserialization"
            );

            (read(is, field), ...);
        }, fields);
    } else if constexpr (std::is_trivially_copyable_v<U> && !std::is_pointer_v<U>) {
        is.read(reinterpret_cast<char*>(&value), sizeof(U));
    } else {
        static_assert(always_false<U>, "No deserializer: implement mutable tuple-of-references members() or add a special case");
    }
}

} // namespace detail

auto writeProductionRecord(std::ostream& os, const Production& p) -> bool {
    detail::hit_dfadebug = false;
    std::ostringstream body;
    detail::write(body, p);
    if (detail::hit_dfadebug || !body) return false;
    const std::string bytes = body.str();
    detail::write(os, bytes.size());
    os.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    return static_cast<bool>(os);
}

auto readProductionRecord(std::istream& is) -> std::optional<Production> {
    std::size_t length{};
    detail::read(is, length);
    if (!is || length > std::string{}.max_size()) return std::nullopt;
    const auto start = is.tellg();
    is.seekg(0, std::ios::end);
    const auto end = is.tellg();
    is.seekg(start);
    if (!is || start < 0 || end < start || length > static_cast<std::size_t>(end - start))
        return std::nullopt;
    std::string bytes(length, '\0');
    is.read(bytes.data(), static_cast<std::streamsize>(length));
    if (!is) return std::nullopt;
    try {
        std::istringstream body(bytes);
        Production p;
        detail::read(body, p);
        if (!body || body.peek() != std::char_traits<char>::eof()) return std::nullopt;
        return p;
    } catch (...) { return std::nullopt; }
}

} // namespace LLIR::Serialize
