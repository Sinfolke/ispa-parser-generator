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
// Generated mid-rule action reducer: lr_semantic_reduce<IT>(rule, cursor, value stack); lr_semantic_rule(rule) says where it applies.
template<class G, class IT, class V>
using lr_semantic_op = decltype(G::template lr_semantic_reduce<IT>(std::size_t{}, std::declval<IT&>(), std::declval<const std::vector<V>&>()));
// Generated fail-block handler: lr_failure(host, state, cursor, value stack) -> Recovery::Decision.
template<class G, class Host, class IT, class V>
using lr_failure_op = decltype(G::lr_failure(std::declval<Host&>(), std::size_t{}, std::declval<IT&>(), std::declval<const std::vector<V>&>()));
// ispastdlib keeps the action context (ctx in advanced rule bodies) in its LL parser; borrow it for LR actions.
template<class MainNode, class Token>
struct ActionContexts : ISPA_STD::LLParser<MainNode, Token> {
    using Base = ISPA_STD::LLParser<MainNode, Token>;
    using Base::rule_context;
};
// Only the recovery state of the LL parser is used: fail blocks run on it; nothing parses through it.
template<class MainNode, class Token>
struct FailHost : LLParser<MainNode, Token> {
    void parseFromTokens() override {}
    MatchResult<MainNode, typename ISPA_STD::Lexer_base<Token>::lazy_iterator> lazyParse() override { throw std::logic_error("FailHost does not parse"); }
};
constexpr std::size_t lr_absent = std::numeric_limits<std::size_t>::max();
inline std::size_t lr_unsigned(long long value) { return value < 0 ? lr_absent : static_cast<std::size_t>(value); }
inline long long lr_signed(std::size_t value) { return value == lr_absent ? -1 : static_cast<long long>(value); }

// The generated parser's static callbacks as the core's LRGrammar.
template<class Generated, class IT, class Token, class Host>
struct LRAdapter : lo::lr::LRGrammar<typename Generated::LRValue> {
    using V = typename Generated::LRValue;
    Host* host = nullptr;
    static_assert(!ISPA_STD::detail::detected_v<ISPA_STD::detail::lr_recover, Generated, IT>,
                  "the std::any lr_recover(region) hook is not available through the core; fail blocks use the typed one");
    static_assert(!ISPA_STD::detail::detected_v<ISPA_STD::detail::lr_semantic_reduce, Generated, IT>,
                  "the std::any lr_semantic_reduce(region) hook is not available through the core; mid-rule actions use the typed one");

    static const Token& token_of(const std::shared_ptr<lo::raw_token::RawToken>& raw) {
        return static_cast<TypedRawToken<Token, void>&>(*raw).token;
    }
    auto lr_k() -> long long override { return static_cast<long long>(Generated::lr_k()); }
    auto lr_action(long long state, const std::vector<long long>& window) -> long long override {
        thread_local std::vector<std::size_t> converted;  // lr_action does not re-enter: one buffer for every decision
        converted.clear();
        for (auto id : window) converted.push_back(lr_unsigned(id));
        return Generated::lr_action(static_cast<std::size_t>(state), converted);
    }
    auto lr_shift(std::shared_ptr<lo::raw_token::RawToken> token) -> V override { return Generated::lr_shift(token_of(token)); }
    auto lr_length(long long rule) -> long long override { return static_cast<long long>(Generated::lr_length(static_cast<std::size_t>(rule))); }
    auto lr_reduce(long long rule, const std::vector<V>& rhs) -> V override { return Generated::lr_reduce(static_cast<std::size_t>(rule), rhs); }
    auto has_recover() -> bool override { return ISPA_STD::detail::detected_v<lr_failure_op, Generated, Host, IT, V>; }
    // A syntax error where a `%handler` member was expected: run the fail block, then repair the parse stack.
    auto lr_recover(std::shared_ptr<lo::lr::LRRegionFrame<V>> region, std::shared_ptr<lo::recovery::Diagnostic> diagnostic) -> bool override {
        if constexpr (ISPA_STD::detail::detected_v<lr_failure_op, Generated, Host, IT, V>) {
            const auto state = static_cast<std::size_t>(region->states.back());
            const auto expected = Generated::lr_failure_token(state);
            if (expected == lr_absent) return false;
            auto& cursor = static_cast<TypedCursor<IT, Token, void>&>(*region->pos);
            const auto reported = host->getErrors().size();
            const auto decision = Generated::lr_failure(*host, state, cursor.it, region->values);
            if (host->getErrors().size() > reported) diagnostic->code = "lr-superseded";
            using Mode = ISPA_STD::Recovery::Mode;
            if (decision.mode == Mode::Abort) return false;
            if (decision.mode != Mode::Insert && decision.mode != Mode::Replace) return true;
            if (expected == lr_absent - 1) return false;  // the member is a rule, only a token can be inserted
            std::vector<long long> window{lr_signed(expected)};
            auto peek = region->pos->copy();
            for (long long i = 1; i < lr_k(); ++i) {
                while (!peek->isEnd() && has_is_space() && lr_is_space(peek->current())) peek->next();
                if (peek->isEnd()) window.push_back(-1);
                else { window.push_back(peek->current()->kind()); peek->next(); }
            }
            const auto action = lr_action(static_cast<long long>(state), window);
            if (action <= 0) return false;
            region->values.push_back(Generated::lr_synthesize(expected));
            region->span_begin.push_back(region->cursor); region->span_end.push_back(region->cursor);
            region->span_leading.push_back(false); region->span_trailing.push_back(false); region->span_empty.push_back(true);
            region->states.push_back(action - 1);
            return true;
        } else return false;
    }
    auto has_semantic_reduce() -> bool override { return ISPA_STD::detail::detected_v<lr_semantic_op, Generated, IT, V>; }
    auto lr_semantic_reduce(long long rule, std::shared_ptr<lo::lr::LRRegionFrame<V>> region, std::shared_ptr<lo::lr::LRRuleSpan>,
                            const std::vector<V>& rhs) -> V override {
        if constexpr (ISPA_STD::detail::detected_v<lr_semantic_op, Generated, IT, V>) {
            if (!Generated::lr_semantic_rule(static_cast<std::size_t>(rule))) return Generated::lr_reduce(static_cast<std::size_t>(rule), rhs);
            auto& cursor = static_cast<TypedCursor<IT, Token, void>&>(*region->pos);
            return Generated::template lr_semantic_reduce<IT>(static_cast<std::size_t>(rule), cursor.it, region->values);
        } else return Generated::lr_reduce(static_cast<std::size_t>(rule), rhs);
    }
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
template<class Generated, class MainNode, class Token, class IT, class CoreParser, class Host>
ISPA_STD::MatchResult<MainNode, IT> lr_core_run(CoreParser& parser, Host& host, std::vector<ISPA_STD::Recovery::Diagnostic>& errors, IT pos, bool generalized) {
    using V = typename Generated::LRValue;
    using Cursor = TypedCursor<IT, Token, void>;
    auto cursor = counted<Cursor>(std::move(pos));
    auto grammar = counted<LRAdapter<Generated, IT, Token, Host>>();
    grammar->host = &host;
    host.reset_recovery();
    auto publish = [&] {
        errors.clear();
        for (const auto& diagnostic : parser->getErrors()) errors.push_back(lr_diagnostic(*diagnostic));
        for (const auto& diagnostic : host.getErrors()) errors.push_back(diagnostic);  // from ctx.error in fail blocks
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
    core_detail::FailHost<MainNode, Token> failure_host_;  // owns the recovery state that fail blocks (ctx.error, ctx.skip, ...) work on
    // Value helpers the generated reductions call, over the parser's closed LRValue instead of std::any.
    template<class T>
    static auto lr_token(T node) {
        std::string_view text;
        if (node.start() && node.length() != std::string::npos) text = {node.start(), node.length()};
        return core_detail::LRTokenBox<T>{std::move(node), text};
    }
    template<class V>
    static V lr_pack(const std::vector<V>& values, const std::vector<std::size_t>& indices,
                     const std::vector<std::size_t>& offsets, const std::vector<std::size_t>& splice,
                     std::size_t count, bool repeated = false) {
        std::vector<std::vector<V>> columns(count);
        for (std::size_t i = 0; i < indices.size(); ++i) {
            const auto& value = values.at(indices[i]);
            if (splice[i]) {
                auto& caps = value.template get<typename V::Caps>();
                // The driver moves a reduction's right-hand side off the stack: the only owner of these captures is this
                // vector, so the (long, left-recursive) repetition column is taken over instead of copied per element.
                const bool owned = caps.use_count() == 1;
                auto& child = *caps;
                for (std::size_t c = 0; c < child.columns.size(); ++c) {
                    auto& column = columns.at(offsets[i] + c);
                    if (owned && column.empty()) column = std::move(const_cast<std::vector<V>&>(child.columns[c]));
                    else column.insert(column.end(), child.columns[c].begin(), child.columns[c].end());
                }
            } else columns.at(offsets[i]).push_back(std::move(const_cast<V&>(value)));  // rhs is the reduction's own copy, used once
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

    // ctx of the Python statements in advanced rules (pure cursor queries; no recovery state is involved)
    template<class IT, class AST>
    static auto rule_context(IT& pos, AST& ast, IT start, std::string rule, const char* (*token_name)(std::size_t) = nullptr) {
        return core_detail::ActionContexts<MainNode, Token>::rule_context(pos, ast, std::move(start), std::move(rule), token_name);
    }

    // ctx of a fail block, made by the parser object the failure is reported to
    template<class Host, class IT, class AST>
    static auto fail_context(Host& host, IT& pos, AST& ast, IT start, std::string rule, std::string failed_rule,
                             const char* (*token_name)(std::size_t) = nullptr) {
        return host.fail_context(pos, ast, std::move(start), std::move(rule), std::move(failed_rule), token_name);
    }
    template<class Host, class IT, class Ctx>
    static auto custom_failure_decision(Host&, const IT& before, IT& after, Ctx& ctx) {
        return Host::custom_failure_decision(before, after, ctx);
    }

    template<class Generated, class IT>
    ISPA_STD::MatchResult<MainNode, IT> lr_parse(IT pos) {
        if (!core_) { ISPA_STD::HeapScope heap; core_ = ISPA_STD::construct<core_detail::lo::lr::LRParser>(); }
        return core_detail::lr_core_run<Generated, MainNode, Token>(core_, failure_host_, this->errors_, std::move(pos), false);
    }
};
}  // namespace ISPA_CORE
