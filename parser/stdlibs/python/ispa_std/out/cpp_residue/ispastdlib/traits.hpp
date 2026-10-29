/**
 * ispastdlib/traits.hpp - is_shared_ptr / is_vector / is_variant / is_match_result / has_ast_print
 * Part of the split ISPA std lib; include <ispastdlib.hpp> for everything.
 */
#pragma once
#include <ispastdlib/core.hpp>

namespace ISPA_STD {
    template<typename DataStorageType, typename IT>
    struct MatchResult;
    template<typename DataStorageType, typename Enable>
    class Node;
    template<typename... Ts>
    class RawToken;
    template<typename T>
    struct is_shared_ptr {
        static constexpr bool value = false;
        using element_type = void;
    };

    template<typename T>
    struct is_shared_ptr<std::shared_ptr<T>> {
        static constexpr bool value = true;
        using element_type = T;
    };
    template<typename T>
    struct is_vector : std::false_type {};

    template<typename T, typename Alloc>
    struct is_vector<std::vector<T, Alloc>> : std::true_type {};

    template<typename T>
    struct is_variant : std::false_type {};

    template<typename... Ts>
    struct is_variant<std::variant<Ts...>> : std::true_type {};

    template<typename T>
    inline constexpr bool is_vector_v =
        is_vector<std::decay_t<T>>::value;

    template<typename T>
    inline constexpr bool is_variant_v =
        is_variant<std::decay_t<T>>::value;
    template<typename T>
    struct is_match_result : std::false_type {};

    template<typename Data, typename IT>
    struct is_match_result<MatchResult<Data, IT>>
        : std::true_type {};

    template<typename T>
    inline constexpr bool is_match_result_v =
        is_match_result<std::decay_t<T>>::value;
    template<typename T, typename Printer, typename = void>
    struct has_ast_print : std::false_type {};

    template<typename T, typename Printer>
    struct has_ast_print<
        T,
        Printer,
        std::void_t<
            decltype(
                std::declval<const T&>().print(
                    std::declval<Printer&>()
                )
            )
        >
    > : std::true_type {};
    template<typename T, typename Printer, typename = void>
    struct has_write_to_output : std::false_type {};

    template<typename T, typename Printer>
    struct has_write_to_output<
        T,
        Printer,
        std::void_t<
            decltype(
                std::declval<const T&>().write_to_output(
                    std::declval<Printer&>()
                )
            )
        >
    > : std::true_type {};
    template<typename T>
    struct is_node : std::false_type {};

    template<typename DataStorageType, typename Enable>
    struct is_node<Node<DataStorageType, Enable>> : std::true_type {};

    template<typename T>
    inline constexpr bool is_node_v = is_node<std::decay_t<T>>::value;

    template<typename T>
    struct is_raw_token : std::false_type {};

    template<typename... Ts>
    struct is_raw_token<RawToken<Ts...>> : std::true_type {};

    template<typename T>
    inline constexpr bool is_raw_token_v = is_raw_token<std::decay_t<T>>::value;
    // Helper detection traits for C++17 (place at namespace or class scope)
    template <typename T, typename = void>
    struct has_to_string : std::false_type {};

    template <typename T>
    struct has_to_string<T, std::void_t<decltype(std::declval<const T&>().to_string())>> : std::true_type {};

    template <typename T, typename = void>
    struct is_streamable : std::false_type {};

    template <typename T>
    struct is_streamable<T, std::void_t<decltype(std::declval<std::ostringstream&>() << std::declval<const T&>())>> : std::true_type {};

    // Primary template: defaults to false
    template <typename T, typename = void>
    struct is_tuple_like : std::false_type {};

    // Specialization: matching when std::tuple_size<T>::value is valid
    template <typename T>
    struct is_tuple_like<T, std::void_t<decltype(std::tuple_size<T>::value)>> : std::true_type {};

    // Helper variable template
    template <typename T>
    inline constexpr bool is_tuple_like_v = is_tuple_like<T>::value;

    template <typename T, typename = void>
    struct has_meta_name : std::false_type {};

    template <typename T>
    struct has_meta_name<T, std::void_t<decltype(std::declval<const T&>().meta().name())>> : std::true_type {};

    template <typename T>
    inline constexpr bool has_meta_name_v = has_meta_name<T>::value;


    // Checks if `token.name()` is valid
    template <typename T, typename = void>
    struct has_name : std::false_type {};

    template <typename T>
    struct has_name<T, std::void_t<decltype(std::declval<const T&>().name())>> : std::true_type {};

    template <typename T>
    inline constexpr bool has_name_v = has_name<T>::value;


    // Checks if `token.index()` is valid
    template <typename T, typename = void>
    struct has_index : std::false_type {};

    template <typename T>
    struct has_index<T, std::void_t<decltype(std::declval<const T&>().index())>> : std::true_type {};

    template <typename T>
    inline constexpr bool has_index_v = has_index<T>::value;

    namespace detail {
        template<template<class...> class Op, class, class... Args>
        struct detector : std::false_type {};

        template<template<class...> class Op, class... Args>
        struct detector<Op, std::void_t<Op<Args...>>, Args...> : std::true_type {};

        template<template<class...> class Op, class... Args>
        inline constexpr bool detected_v = detector<Op, void, Args...>::value;

        template<class T> using data_value = decltype(std::declval<const T&>().data().value);
        template<class T> using start_length = decltype(std::declval<const T&>().start(),
                                                         std::declval<const T&>().length());
        template<class T> using meta_start_length = decltype(std::declval<const T&>().meta().start(),
                                                              std::declval<const T&>().meta().length());
        template<class T> using meta_startpos_length = decltype(std::declval<const T&>().meta().startpos(),
                                                                 std::declval<const T&>().meta().length());
        template<class T> using meta_startpos = decltype(std::declval<const T&>().meta().startpos());
        template<class T> using is_end = decltype(std::declval<const T&>().isEnd());
        template<class T> using size = decltype(std::declval<const T&>().size());
        template<class T> using distance = decltype(std::declval<const T&>().distance());
        template<class T> using decrement = decltype(--std::declval<T&>());
        template<class T> using source_position = decltype(std::declval<const T&>().sourcePosition());
        template<class T> using source_origin_position = decltype(std::declval<const T&>().sourceOrigin(),
                                                                   std::declval<const T&>().sourcePosition());
        template<class T> using consume_external = decltype(std::declval<T&>().consumeExternal(
            std::declval<const T&>().sourcePosition()));
        template<class T> using is_ambiguous = decltype(std::declval<const T&>().isAmbiguous());
        template<class T> using ambiguous_id = decltype(T::ambiguous_id);
        template<class T> using custom_action = decltype(T::custom_action);
    }

} // namespace ISPA_STD
