// The LR value stack's element type for the core: closed tagged storage (like RawToken, because a large
// std::variant compiles far too slowly) plus the value helpers generated reductions call. Shifted tokens and
// rule results sit inline; only packed captures are shared (the core's LRCaptures).
#pragma once
#include <ispa_core/namespace.hpp>
#include <ispa_core/parser_ll.hpp>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <new>

namespace ISPA_CORE {
namespace core_detail {
// A shifted terminal: the typed node and its text (the text serves char/string targets).
// The text is a view of the input, which outlives the parse (the value stack never outlives it).
template<class N> struct LRTokenBox { N node; std::string_view text; };
}  // namespace core_detail

template<class Token, class... Ts>
class LRValueOf {
public:
    using Caps = std::shared_ptr<core_detail::lo::lr::LRCaptures<LRValueOf>>;
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    template<class T> static constexpr std::size_t index_of() {
        constexpr bool same[] = {std::is_same_v<T, Ts>..., std::is_same_v<T, Caps>};
        for (std::size_t i = 0; i < sizeof...(Ts) + 1; ++i) if (same[i]) return i;
        return npos;
    }
    template<class T> static constexpr bool alternative = index_of<std::remove_cv_t<std::remove_reference_t<T>>>() != npos;

    LRValueOf() noexcept = default;
    template<class T, std::enable_if_t<alternative<T> && !std::is_same_v<std::decay_t<T>, LRValueOf>, int> = 0>
    LRValueOf(T&& value) { emplace<std::decay_t<T>>(std::forward<T>(value)); }
    template<class N, std::enable_if_t<alternative<N>, int> = 0>
    LRValueOf(core_detail::LRTokenBox<N>&& box) : token_(true), text_(std::move(box.text)) { emplace<N>(std::move(box.node)); }
    LRValueOf(const LRValueOf& other) : token_(other.token_), text_(other.text_) { if (other.id_ != none) { ops(other.id_)->copy(data_, other.data_); id_ = other.id_; } }
    LRValueOf(LRValueOf&& other) noexcept : token_(other.token_), text_(std::move(other.text_)) {
        if (other.id_ != none) { ops(other.id_)->move(data_, other.data_); id_ = other.id_; }
    }
    LRValueOf& operator=(const LRValueOf& other) { if (this != &other) { LRValueOf copy(other); *this = std::move(copy); } return *this; }
    LRValueOf& operator=(LRValueOf&& other) noexcept {
        if (this == &other) return *this;
        reset();
        token_ = other.token_; text_ = std::move(other.text_);
        if (other.id_ != none) { ops(other.id_)->move(data_, other.data_); id_ = other.id_; }
        return *this;
    }
    ~LRValueOf() { reset(); }

    bool token() const { return token_; }
    std::string_view text() const { return text_; }
    template<class T> bool holds() const {
        if constexpr (index_of<T>() == npos) return false;
        else return id_ == index_of<T>();
    }
    template<class T> const T& get() const {
        static_assert(index_of<T>() != npos, "LRValueOf::get<T>: not an alternative");
        if (id_ != index_of<T>()) throw std::logic_error("LR value does not hold the requested alternative");
        return *std::launder(reinterpret_cast<const T*>(data_));
    }
    template<class T> T& get() {
        static_assert(index_of<T>() != npos, "LRValueOf::get<T>: not an alternative");
        if (id_ != index_of<T>()) throw std::logic_error("LR value does not hold the requested alternative");
        return *std::launder(reinterpret_cast<T*>(data_));
    }

private:
    static constexpr std::uint16_t none = 0xFFFF;
    static constexpr std::size_t size_ = std::max({sizeof(Ts)..., sizeof(Caps)});
    static constexpr std::size_t align_ = std::max({alignof(Ts)..., alignof(Caps)});
    struct Ops { void (*copy)(void*, const void*); void (*move)(void*, void*) noexcept; void (*destroy)(void*) noexcept; };
    template<class T> static const Ops* ops_of() {
        static const Ops table{
            [](void* to, const void* from) { ::new (to) T(*static_cast<const T*>(from)); },
            [](void* to, void* from) noexcept { ::new (to) T(std::move(*static_cast<T*>(from))); },
            [](void* at) noexcept { static_cast<T*>(at)->~T(); }};
        return &table;
    }
    static const Ops* ops(std::size_t id) {
        static const Ops* const tables[] = {ops_of<Ts>()..., ops_of<Caps>()};
        return tables[id];
    }
    template<class T, class A> void emplace(A&& value) {
        ::new (static_cast<void*>(data_)) T(std::forward<A>(value));
        id_ = static_cast<std::uint16_t>(index_of<T>());
    }
    void reset() noexcept { if (id_ != none) { ops(id_)->destroy(data_); id_ = none; } }

    alignas(align_) unsigned char data_[size_];
    std::uint16_t id_ = none;
    bool token_ = false;
    std::string_view text_;
};

namespace core_detail {
// --- capture conversion over the closed value type (port of ispastdlib's std::any lr_convert) ---------------
template<class V> V lr_captures_value(std::vector<std::vector<V>> columns, bool repeated) {
    // The lowered constructor takes its columns by const reference: construct empty, then move them in (no copy of the lists).
    auto captures = ISPA_STD::construct<lo::lr::LRCaptures<V>>(std::vector<std::vector<V>>{}, repeated);
    captures->columns = std::move(columns);
    return V(std::move(captures));
}
template<class V, class T> bool lr_convert(T& target, const V& value);

template<class V, class T, std::size_t... I>
bool lr_convert_tuple(T& target, const lo::lr::LRCaptures<V>& captures, std::index_sequence<I...>) {
    return (lr_convert<V>(std::get<I>(target), lr_captures_value<V>({captures.columns[I]}, false)) && ...);
}
template<class V, class T, std::size_t... I>
bool lr_convert_variant(T& target, const V& value, std::index_sequence<I...>) {
    return ([&] {
        std::variant_alternative_t<I, T> alternative{};
        if (!lr_convert<V>(alternative, value)) return false;
        target.template emplace<I>(std::move(alternative));
        return true;
    }() || ...);
}
template<class V, class T> bool lr_convert_value(T& target, const V& value) {
    if constexpr (V::template index_of<T>() != V::npos)
        if (value.template holds<T>()) { target = value.template get<T>(); return true; }
    if constexpr (ISPA_STD::is_node_v<T>) {
        using Storage = typename T::data_type;
        if constexpr (ISPA_STD::is_shared_ptr<Storage>::value) {
            using Plain = ISPA_STD::Node<typename ISPA_STD::is_shared_ptr<Storage>::element_type>;
            if constexpr (V::template index_of<Plain>() != V::npos)
                if (value.template holds<Plain>()) { target = T(value.template get<Plain>()); return true; }
        }
    }
    if (value.template holds<typename V::Caps>()) {
        const auto& captures = *value.template get<typename V::Caps>();
        bool empty = true;
        for (const auto& column : captures.columns) if (!column.empty()) empty = false;
        if (empty) { target = T{}; return true; }
        if constexpr (ISPA_STD::is_vector_v<T>) {
            target.clear();
            if (captures.repeated && captures.columns.size() > 1) {
                const auto rows = captures.columns.front().size();
                for (const auto& column : captures.columns) if (column.size() != rows) return false;
                for (std::size_t row = 0; row < rows; ++row) {
                    std::vector<std::vector<V>> item;
                    for (const auto& column : captures.columns) item.push_back({column[row]});
                    typename T::value_type element{};
                    if (!lr_convert<V>(element, lr_captures_value<V>(std::move(item), false))) return false;
                    target.push_back(std::move(element));
                }
                return true;
            }
            for (const auto& column : captures.columns) for (const auto& item : column) {
                typename T::value_type element{};
                if (lr_convert<V>(element, item)) target.push_back(std::move(element));
                else {
                    T nested;
                    if (!lr_convert<V>(nested, item)) return false;
                    target.insert(target.end(), nested.begin(), nested.end());
                }
            }
            return true;
        } else if constexpr (ISPA_STD::is_tuple_like_v<T> && !ISPA_STD::is_variant_v<T>) {
            if (captures.columns.size() != std::tuple_size_v<T>) return false;
            return lr_convert_tuple<V>(target, captures, std::make_index_sequence<std::tuple_size_v<T>>{});
        } else {
            const V* selected = nullptr;
            for (const auto& column : captures.columns) for (const auto& item : column) {
                if (selected) return false;
                selected = &item;
            }
            if (!selected) { target = T{}; return true; }
            return lr_convert<V>(target, *selected);
        }
    }
    if constexpr (ISPA_STD::is_variant_v<T>) return lr_convert_variant<V>(target, value, std::make_index_sequence<std::variant_size_v<T>>{});
    return false;
}
template<class V, class T> bool lr_convert(T& target, const V& value) {
    if (value.token()) {
        if constexpr (std::is_same_v<T, char>) {
            if (value.text().size() != 1) return false;
            target = value.text().front(); return true;
        } else if constexpr (std::is_same_v<T, std::string>) {
            target = std::string{value.text()}; return true;
        } else if constexpr (!ISPA_STD::is_variant_v<T>) return lr_convert_value<V>(target, value);
        // A variant may contain a literal char/string alternative as well as typed nodes: try them all.
    }
    return lr_convert_value<V>(target, value);
}
}  // namespace core_detail
}  // namespace ISPA_CORE
