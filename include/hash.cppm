export module hash;
import std;
import dstd;
export inline void hash_combine(std::size_t& seed) {}

export template <typename T, typename... Rest>
inline void hash_combine(std::size_t& seed, const T& v, const Rest&... rest) {
    seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    if constexpr (sizeof...(rest) > 0) {
        hash_combine(seed, rest...);
    }
}

template<typename T>
concept is_Hashable = requires(T a) {
    { std::hash<T>{}(a) } -> std::convertible_to<std::size_t>;
};
// Helper to apply a function to each element of a tuple
template <typename Tuple, typename Func, std::size_t... I>
void for_each_in_tuple_impl(const Tuple& t, Func f, std::index_sequence<I...>) {
    (f(std::get<I>(t)), ...);  // Fold expression over comma operator (C++17)
}

template <typename Tuple, typename Func>
void for_each_in_tuple(const Tuple& t, Func f) {
    for_each_in_tuple_impl(t, f, std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

export struct uhash {
    // hash for optional and variant
    template <typename T>
    std::size_t operator()(const std::optional<T>& opt) const {
        if (!opt.has_value()) return 0;
        return uhash{}(*opt);
    }
    template <typename... Ts>
    std::size_t operator()(const std::variant<Ts...>& v) const {
        std::size_t h = std::hash<std::size_t>{}(v.index()); // include the index in the hash
        std::visit([&](const auto& value) {
            hash_combine(h, uhash{}(value));
        }, v);
        return h;
    }
    template<typename T>
    std::size_t operator()(const std::unordered_set<T>& s) const {
        std::size_t hash = 0;
        for (const T& item : s) {
            hash ^= uhash{}(item) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        return hash;
    }

    std::size_t operator()(const std::monostate&) const {
        return 0;
    }
    template<is_Hashable T>
    std::size_t operator()(const T& value) const {
        return std::hash<T>{}(value);
    }
    template<typename... Ts>
    std::size_t operator()(const std::tuple<Ts...>& t) const {
        std::size_t h = 0;
        for_each_in_tuple(t, [&](const auto& value) {
            hash_combine(h, uhash{}(value));
        });
        return h;
    }
    // Fallback for pairs
    template <typename T1, typename T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const {
        std::size_t h = 0;
        hash_combine(h, uhash{}(p.first), uhash{}(p.second));
        return h;
    }

    template<typename T>
    requires (!is_Hashable<T> && requires(T a) { a.begin(); a.end(); })
    std::size_t operator()(const T& container) const {
        std::size_t h = 0;
        for (const auto& item : container) {
            std::size_t item_hash = uhash{}(item);
            hash_combine(h, item_hash);
        }
        return h;
    }
    template<typename T>
    requires (!is_Hashable<T> && !requires(T a) {a.begin(); a.end();} && requires(T a) { a.members(); })
    std::size_t operator()(const T& structure) const {
        std::size_t h = 0;
        auto members = structure.members();
        for_each_in_tuple(members, [&](const auto& member) {
            std::size_t member_hash = uhash{}(member);
            hash_combine(h, member_hash);
        });
        return h;
    }
    // Fallback or static_assert for unsupported types
    template<typename T>
    requires (!is_Hashable<T> && !requires(T a) { a.begin(); a.end(); } && !requires(T a) { a.members(); } && sizeof(T) != 0)
    std::size_t operator()(const T& structure) const {
        static_assert(std::tuple_size<decltype(structure.members())>::value >= 0, "members() must return a tuple");
        static_assert(sizeof(T) == 0, "uhash: cannot hash type: not default hashable, not container and does not provide members method");
        return 0;
    }
    template<typename T>
    std::size_t operator()(const std::shared_ptr<T>& ptr) const {
        if (!ptr) return 0;
        return uhash{}(*ptr);
    }
};
export template<typename T>
bool operator==(std::unordered_set<T> a, std::unordered_set<T> b) {
    if (a.size() != b.size()) return false;
    for (const auto& item : a) {
        if (!b.contains(item))
            return false;
    }
    return true;
}
export struct uequal {
    template<typename T>
    bool operator()(const T& a, const T& b) const {
        return a == b;
    }
};
// exports
export namespace utype {
    template<typename Key, typename Value>
    using unordered_map = std::unordered_map<Key, Value, uhash, uequal>;
    template<typename T>
    using unordered_set = std::unordered_set<T, uhash, uequal>;
}
export struct EmptyHashable {
protected:
    friend struct ::uhash;
    auto members() const  {
        return std::tuple<> {};
    }
};