// ISPA_CORE::LRParser<MainNode, Token>: the ispastdlib LR interface (generated static callbacks, value helpers)
// whose driver runs in the lowered core (python::ispa_std::lr). The generated class names its stack element type
// as LRValue; the core's driver is generic over it, so no value crosses the boundary as std::any unless the
// stdlib maps LRValueOf onto std::any.
#pragma once
#include <ispastdlib/lr_machine.hpp>
#include <ispa_core/namespace.hpp>
#include <ispa_core/parser_ll.hpp>
#include <ispa_core/lr_value.hpp>
#include <limits>

namespace ISPA_CORE {
namespace core_detail {
template<class G> using lr_conflict_op = decltype(G::lr_conflict(std::size_t{}, std::vector<std::size_t>{}));
template<class P, class V, class G, class C>
using lr_generalized_op = decltype(std::declval<P&>().template lr_parse_generalized<V>(std::declval<G&>(), std::declval<C&>()));
constexpr std::size_t lr_absent = std::numeric_limits<std::size_t>::max();
inline std::size_t lr_unsigned(long long value) { return value < 0 ? lr_absent : static_cast<std::size_t>(value); }
inline long long lr_signed(std::size_t value) { return value == lr_absent ? -1 : static_cast<long long>(value); }

// The generated parser's static callbacks as the core's LRGrammar.
template<class Generated, class IT, class Token>
struct LRAdapter : lo::lr::LRGrammar<typename Generated::LRValue> {
    using V = typename Generated::LRValue;
    static_assert(!ISPA_STD::detail::detected_v<ISPA_STD::detail::lr_recover, Generated, IT>,
                  "LR recovery handlers (lr_recover) are not available through the core yet");
    static_assert(!ISPA_STD::detail::detected_v<ISPA_STD::detail::lr_semantic_reduce, Generated, IT>,
                  "semantic reductions (lr_semantic_reduce) are not available through the core yet");

    static const Token& token_of(const std::shared_ptr<lo::raw_token::RawToken>& raw) {
        return static_cast<TypedRawToken<Token, void>&>(*raw).token;
    }
    auto lr_k() -> long long override { return static_cast<long long>(Generated::lr_k()); }
    auto lr_action(long long state, const std::vector<long long>& window) -> long long override {
        std::vector<std::size_t> converted;
        converted.reserve(window.size());
        for (auto id : window) converted.push_back(lr_unsigned(id));
        return Generated::lr_action(static_cast<std::size_t>(state), converted);
    }
    auto lr_shift(std::shared_ptr<lo::raw_token::RawToken> token) -> V override { return Generated::lr_shift(token_of(token)); }
    auto lr_length(long long rule) -> long long override { return static_cast<long long>(Generated::lr_length(static_cast<std::size_t>(rule))); }
    auto lr_reduce(long long rule, const std::vector<V>& rhs) -> V override { return Generated::lr_reduce(static_cast<std::size_t>(rule), rhs); }
    auto lr_lhs(long long rule) -> long long override { return static_cast<long long>(Generated::lr_lhs(static_cast<std::size_t>(rule))); }
    auto lr_goto(long long state, long long lhs) -> long long override {
        return static_cast<long long>(Generated::lr_goto(static_cast<std::size_t>(state), static_cast<std::size_t>(lhs)));
    }
    auto has_is_space() -> bool override { return ISPA_STD::detail::detected_v<ISPA_STD::detail::lr_is_space, Generated, IT>; }
    auto lr_is_space(std::shared_ptr<lo::raw_token::RawToken> token) -> bool override {
        if constexpr (ISPA_STD::detail::detected_v<ISPA_STD::detail::lr_is_space, Generated, IT>) return Generated::lr_is_space(token_of(token));
        else return false;
    }
    auto has_space() -> bool override { return ISPA_STD::detail::detected_v<ISPA_STD::detail::lr_space, Generated>; }
    auto lr_space(long long rule, long long index) -> bool override {
        if constexpr (ISPA_STD::detail::detected_v<ISPA_STD::detail::lr_space, Generated>)
            return Generated::lr_space(static_cast<std::size_t>(rule), static_cast<std::size_t>(index));
        else return false;
    }
    auto has_expected() -> bool override { return ISPA_STD::detail::detected_v<ISPA_STD::detail::lr_expected, Generated>; }
    auto lr_expected(long long state) -> std::vector<long long> override {
        std::vector<long long> out;
        if constexpr (ISPA_STD::detail::detected_v<ISPA_STD::detail::lr_expected, Generated>)
            for (auto id : Generated::lr_expected(static_cast<std::size_t>(state))) out.push_back(lr_signed(id));
        return out;
    }
    auto has_context() -> bool override { return ISPA_STD::detail::detected_v<ISPA_STD::detail::lr_context, Generated>; }
    auto lr_context(long long state) -> std::string override {
        if constexpr (ISPA_STD::detail::detected_v<ISPA_STD::detail::lr_context, Generated>)
            return Generated::lr_context(static_cast<std::size_t>(state));
        else return {};
    }
    auto has_token_name() -> bool override { return ISPA_STD::detail::detected_v<ISPA_STD::detail::lr_token_name, Generated>; }
    auto lr_token_name(long long id) -> std::string override {
        if constexpr (ISPA_STD::detail::detected_v<ISPA_STD::detail::lr_token_name, Generated>) return Generated::lr_token_name(lr_unsigned(id));
        else return {};
    }
    auto has_rule_label() -> bool override { return ISPA_STD::detail::detected_v<ISPA_STD::detail::lr_rule_label, Generated>; }
    auto lr_rule_label(long long rule) -> std::string override {
        if constexpr (ISPA_STD::detail::detected_v<ISPA_STD::detail::lr_rule_label, Generated>)
            return Generated::lr_rule_label(static_cast<std::size_t>(rule));
        else return {};
    }
    auto lr_conflict(long long state, const std::vector<long long>& window) -> std::vector<long long> override {
        if constexpr (ISPA_STD::detail::detected_v<lr_conflict_op, Generated>) {
            std::vector<std::size_t> converted;
            for (auto id : window) converted.push_back(lr_unsigned(id));
            std::vector<long long> out;
            for (auto action : Generated::lr_conflict(static_cast<std::size_t>(state), converted)) out.push_back(static_cast<long long>(action));
            return out;
        } else return {};
    }
};

inline ISPA_STD::Recovery::Diagnostic lr_diagnostic(const lo::recovery::Diagnostic& core) {
    ISPA_STD::Recovery::Diagnostic out;
    out.rule = core.rule;
    out.unexpected = lr_unsigned(core.unexpected);
    for (auto id : core.expected) out.expected.push_back(lr_unsigned(id));
    out.expected_text = core.expected_text;
    out.offset = lr_unsigned(core.offset);
    out.length = core.length < 0 ? 0 : static_cast<std::size_t>(core.length);
    out.message = core.message;
    out.code = core.code;
    out.notes = core.notes;
    return out;
}

// Runs the core driver over a typed iterator and brings the typed result back.
template<class Generated, class MainNode, class Token, class IT, class CoreParser>
ISPA_STD::MatchResult<MainNode, IT> lr_core_run(CoreParser& parser, std::vector<ISPA_STD::Recovery::Diagnostic>& errors, IT pos, bool generalized) {
    using V = typename Generated::LRValue;
    using Cursor = TypedCursor<IT, Token, void>;
    auto cursor = counted<Cursor>(std::move(pos));
    auto grammar = counted<LRAdapter<Generated, IT, Token>>();
    auto publish = [&] {
        errors.clear();
        for (const auto& diagnostic : parser->getErrors()) errors.push_back(lr_diagnostic(*diagnostic));
    };
    std::shared_ptr<lo::match_result::MatchResult> result;
    try {
        if constexpr (ISPA_STD::detail::detected_v<lr_generalized_op, std::remove_reference_t<decltype(*parser)>, V, decltype(grammar), decltype(cursor)>) {
            result = generalized ? parser->template lr_parse_generalized<V>(grammar, cursor)
                                 : parser->template lr_parse<V>(grammar, cursor);
        } else result = parser->template lr_parse<V>(grammar, cursor);
    } catch (...) { publish(); throw; }
    publish();
    auto at = std::static_pointer_cast<Cursor>(result->it)->it;
    const std::any& data = result->node->data();
    if constexpr (std::is_same_v<V, std::any>) return {true, std::any_cast<ISPA_STD::Node<MainNode>>(data), std::move(at)};
    else return {true, ISPA_STD::python_cast<ISPA_STD::Node<MainNode>>(std::any_cast<const V&>(data)), std::move(at)};
}
}  // namespace core_detail

template<typename MainNode, typename Token>
class LRParser : public ISPA_STD::LRParser<MainNode, Token> {
    std::shared_ptr<core_detail::lo::lr::LRParser> core_;
protected:
    // Value helpers the generated reductions call, over the parser's closed LRValue instead of std::any.
    template<class T>
    static auto lr_token(T node) {
        std::string text;
        if (node.start() && node.length() != std::string::npos) text.assign(node.start(), node.length());
        return core_detail::LRTokenBox<T>{std::move(node), std::move(text)};
    }
    template<class V>
    static V lr_pack(const std::vector<V>& values, const std::vector<std::size_t>& indices,
                     const std::vector<std::size_t>& offsets, const std::vector<std::size_t>& splice,
                     std::size_t count, bool repeated = false) {
        std::vector<std::vector<V>> columns(count);
        for (std::size_t i = 0; i < indices.size(); ++i) {
            const auto& value = values.at(indices[i]);
            if (splice[i]) {
                const auto& child = *value.template get<typename V::Caps>();
                for (std::size_t c = 0; c < child.columns.size(); ++c) {
                    auto& column = columns.at(offsets[i] + c);
                    column.insert(column.end(), child.columns[c].begin(), child.columns[c].end());
                }
            } else columns.at(offsets[i]).push_back(value);
        }
        return core_detail::lr_captures_value<V>(std::move(columns), repeated);
    }
    template<class T, class V>
    static void lr_capture_assign(T& target, const std::vector<V>& values, std::size_t index, std::size_t column) {
        const auto& value = values.at(index);
        if (column == std::numeric_limits<std::size_t>::max()) {
            if (core_detail::lr_convert<V>(target, value)) return;
        } else {
            const auto& captures = *value.template get<typename V::Caps>();
            if (core_detail::lr_convert<V>(target, core_detail::lr_captures_value<V>({captures.columns.at(column)}, captures.repeated))) return;
        }
        throw std::runtime_error("LR capture payload does not match its declared AST field type: " + std::string{typeid(T).name()});
    }
    template<class T, class V>
    static ISPA_STD::Node<T> lr_get(const std::vector<V>& values, std::size_t index) {
        return values.at(index).template get<ISPA_STD::Node<T>>();
    }

    template<class Generated, class IT>
    ISPA_STD::MatchResult<MainNode, IT> lr_parse(IT pos) {
        if (!core_) { ISPA_STD::HeapScope heap; core_ = ISPA_STD::construct<core_detail::lo::lr::LRParser>(); }
        return core_detail::lr_core_run<Generated, MainNode, Token>(core_, this->errors_, std::move(pos), false);
    }
};
}  // namespace ISPA_CORE
