/**
 * ispastdlib/assign.hpp - assign_compatible
 * Part of the split ISPA std lib; include <ispastdlib.hpp> for everything.
 */
#pragma once
#include <ispastdlib/traits.hpp>
#include <ispastdlib/core.hpp>

namespace ISPA_STD {
    template<typename To, typename From>
    void assign_compatible(To& target, const From& source);

    template<std::size_t D, std::size_t S, class To, class From>
    constexpr bool tuple_omitted_strings_compatible() {
        if constexpr (D == std::tuple_size_v<To>) {
            return S == std::tuple_size_v<From>;
        } else if constexpr (std::is_same_v<std::tuple_element_t<D, To>, std::string>) {
            return tuple_omitted_strings_compatible<D + 1, S, To, From>();
        } else if constexpr (S == std::tuple_size_v<From>) {
            return false;
        } else {
            using Target = std::tuple_element_t<D, To>;
            using Source = std::tuple_element_t<S, From>;
            return (std::is_constructible_v<Target, const Source&> ||
                    std::is_assignable_v<Target&, const Source&>) &&
                   tuple_omitted_strings_compatible<D + 1, S + 1, To, From>();
        }
    }

    template<class To, class From> struct tuple_omitted_strings : std::false_type {};
    template<class... To, class... From>
    struct tuple_omitted_strings<std::tuple<To...>, std::tuple<From...>>
        : std::bool_constant<(sizeof...(To) > sizeof...(From)) &&
                             tuple_omitted_strings_compatible<0, 0,
                                 std::tuple<To...>, std::tuple<From...>>()> {};

    template<std::size_t D = 0, std::size_t S = 0, class To, class From>
    void assign_tuple_omitted_strings(To& target, const From& source) {
        if constexpr (D < std::tuple_size_v<To>) {
            using Element = std::tuple_element_t<D, To>;
            if constexpr (std::is_same_v<Element, std::string>) {
                assign_tuple_omitted_strings<D + 1, S>(target, source);
            } else {
                assign_compatible(std::get<D>(target), std::get<S>(source));
                assign_tuple_omitted_strings<D + 1, S + 1>(target, source);
            }
        }
    }

    // Index of the alternative of variant V that is exactly T, or variant_npos. Pure std::is_same: no overload
    // resolution and no conversion-constructor instantiation, so it is O(N) cheap checks for an N-alternative variant.
    template<typename V, typename T, std::size_t... Is>
    constexpr std::size_t variant_exact_index_impl(std::index_sequence<Is...>) {
        std::size_t result = std::variant_npos;
        (void) ((std::is_same_v<std::variant_alternative_t<Is, V>, T> && result == std::variant_npos ? (result = Is) : result), ...);
        return result;
    }
    template<typename V, typename T>
    inline constexpr std::size_t variant_exact_index_v =
        variant_exact_index_impl<V, T>(std::make_index_sequence<std::variant_size_v<V>>{});

    template<typename To, typename From, std::size_t I = 0>
    bool assign_variant_alternative(To& target, const From& source);

    template<typename To, typename From>
    void assign_compatible(To& target, const From& source);
    template<typename To, typename From>
    void assign_compatible(To& target, const From& source) {
        // Recovered required captures may be absent. Leave their destination
        // default-initialized rather than dereferencing a missing node.
        if constexpr (is_node_v<From>) {
            if (source.empty()) return;
        }
        if constexpr (is_match_result_v<From>) {
            // MatchResult<Rules, T, IT> -> Node<Rules, T>
            // Then recursively convert to the destination type.
            assign_compatible(target, source.node);

        } else if constexpr (is_vector_v<To> && is_vector_v<From>) {
            target.clear();
            target.reserve(source.size());

            using Element = typename To::value_type;

            for (const auto& item : source) {
                Element converted{};
                assign_compatible(converted, item);
                target.push_back(std::move(converted));
            }

        } else if constexpr (tuple_omitted_strings<To, From>::value) {
            assign_tuple_omitted_strings(target, source);

        } else if constexpr (is_variant_v<To> && is_variant_v<From>) {
            std::visit(
                [&](const auto& value) {
                    if (!assign_variant_alternative(target, value)) {
                        throw IspaInternalExcept("assign_compatible: incompatible variant alternative", "Parser", true);
                    }
                },
                source
            );

        } else if constexpr (std::is_assignable_v<To&, const From&>) {
            target = source;

        } else if constexpr (std::is_constructible_v<To, const From&>) {
            target = To(source);

        } else if constexpr (is_variant_v<From>) {
            std::visit([&](const auto& value) {
                using Value = std::decay_t<decltype(value)>;
                if constexpr (!std::is_same_v<Value, std::monostate>)
                    assign_compatible(target, value);
            }, source);

        } else if constexpr (detail::detected_v<detail::data_value, From>) {
            assign_compatible(target, source.data().value);

        } else if constexpr (std::is_same_v<To, char> &&
                             detail::detected_v<detail::start_length, From>) {
            if (source.length() != 1 || !source.start())
                throw IspaInternalExcept("assign_compatible: expected one character", "Parser", true);
            target = *source.start();

        } else if constexpr (std::is_same_v<To, std::string> &&
                             detail::detected_v<detail::start_length, From>) {
            target.assign(source.start(), source.length());

        } else {
            static_assert(
                std::is_constructible_v<To, const From&>,
                "assign_compatible: unsupported conversion"
            );
        }
    }
    template<typename To, typename From, std::size_t I>
    bool assign_variant_alternative(To& target, const From& source) {
        if constexpr (I == 0) {
            // Fast path: the source alternative type is literally one of the target's alternatives.
            // Without it every source alternative runs is_constructible against every target alternative,
            // i.e. N*M Node converting-constructor overload resolutions for two big Token variants.
            constexpr std::size_t exact = variant_exact_index_v<To, std::decay_t<From>>;
            if constexpr (exact != std::variant_npos) {
                target.template emplace<exact>(source);
                return true;
            }
        }
        if constexpr (I == std::variant_size_v<To>) {
            static_assert(
                std::is_constructible_v<To, const From&> || std::is_assignable_v<To&, const From&>,
                "assign_variant_alternative: no compatible alternative found"
            );
            return false;
        } else {
            using Alternative = std::variant_alternative_t<I, To>;

            if constexpr (std::is_constructible_v<Alternative, From>) {
                target.template emplace<I>(source);
                return true;
            } else if constexpr (tuple_omitted_strings<Alternative, From>::value) {
                auto& value = target.template emplace<I>();
                assign_tuple_omitted_strings(value, source);
                return true;
            } else {
                return assign_variant_alternative<To, From, I + 1>(target, source);
            }
        }
    }
} // namespace ISPA_STD
