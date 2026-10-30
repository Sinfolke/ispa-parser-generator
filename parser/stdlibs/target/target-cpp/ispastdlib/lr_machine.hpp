#pragma once
#include <ispastdlib/parser_base.hpp>
#include <ispastdlib/ispa_recovery.hpp>
#include <any>
#include <ispastdlib/traits.hpp>
#include <tuple>
#include <limits>
#include <stdexcept>
#include <vector>
#include <sstream>

namespace ISPA_STD {
// The LR value stack's element type, named by generated parsers as LRValueOf<Token, Node<Types::...>...>.
// ispastdlib keeps one dynamic type; a stdlib with closed tagged storage maps the same name onto it.
template<class... Alternatives> using LRValueOf = std::any;
// A region owns its recognition stack and the values produced inside it. A
// continuation identifies where an enclosing semantic program can resume.
// The deterministic parser currently uses one root region; nested region
// transitions require generated semantic callbacks.
enum class LRControlPhase { Decide, Shift, Reduce, Semantic, Recover, Accept };
struct LRControlState {
    LRControlPhase phase = LRControlPhase::Decide;
    std::size_t continuation = 0;
    std::size_t production = 0;
};

struct LRRuleSpan {
    std::size_t begin = 0, end = 0;
    bool leading = false, trailing = false, empty = true;
};

template<class IT>
struct LRRegionFrame {
    IT pos;
    std::vector<std::size_t> states{0};
    std::vector<std::any> values;
    std::vector<LRRuleSpan> spans;
    std::vector<std::size_t> capture_destinations;
    std::size_t cursor = 0;
    bool recovered_prefix = false;
    LRControlState control;

    explicit LRRegionFrame(IT start, std::size_t continuation = 0)
        : pos(std::move(start)) { control.continuation = continuation; }
};

namespace detail {
    template<class G, class IT> using lr_is_space = decltype(G::lr_is_space(*std::declval<IT&>()));
    template<class G> using lr_space = decltype(G::lr_space(std::declval<std::size_t>(), std::declval<std::size_t>()));
    template<class G> using lr_expected = decltype(G::lr_expected(std::declval<std::size_t>()));
    template<class G> using lr_context = decltype(G::lr_context(std::declval<std::size_t>()));
    template<class G> using lr_token_name = decltype(G::lr_token_name(std::declval<std::size_t>()));
    template<class G> using lr_rule_label = decltype(G::lr_rule_label(std::declval<std::size_t>()));
    template<class G, class IT> using lr_recover = decltype(std::declval<G&>().lr_recover(
        std::declval<LRRegionFrame<IT>&>(), std::declval<Recovery::Diagnostic&>()));
    template<class G, class IT> using lr_semantic_reduce = decltype(std::declval<G&>().lr_semantic_reduce(
        std::declval<std::size_t>(), std::declval<LRRegionFrame<IT>&>(),
        std::declval<LRRuleSpan&>(), std::declval<const std::vector<std::any>&>()));
}

// Generated callbacks use a signed action: shift = state+1, reduce = -rule-1,
// accept = 0. Missing cells throw before any stack mutation.
template<class MainNode, class Token>
class LRParser : public Parser_base<MainNode, Token> {
public:
    [[nodiscard]] const std::vector<Recovery::Diagnostic>& getErrors() const noexcept { return errors_; }
protected:
    std::vector<Recovery::Diagnostic> errors_;
    // Synthetic grammar symbols carry captures, never user-visible AST nodes.
    struct LRTokenValue { std::any node; std::string text; };
    template<class T>
    static std::any lr_token(T node) {
        std::string text;
        if (node.start() && node.length() != std::string::npos) text.assign(node.start(), node.length());
        return LRTokenValue{std::any(std::move(node)), std::move(text)};
    }
    struct LRCaptures { std::vector<std::vector<std::any>> columns; bool repeated = false; };

    template<class T, std::size_t... I>
    static bool lr_convert_tuple(T& target, const LRCaptures& captures, std::index_sequence<I...>) {
        return (lr_convert(std::get<I>(target), std::any(LRCaptures{{captures.columns[I]}})) && ...);
    }

    template<class T, std::size_t... I>
    static bool lr_convert_variant(T& target, const std::any& value, std::index_sequence<I...>) {
        return ([&] {
            std::variant_alternative_t<I, T> alternative{};
            if (!lr_convert(alternative, value)) return false;
            target.template emplace<I>(std::move(alternative));
            return true;
        }() || ...);
    }

    static std::any lr_pack(const std::vector<std::any>& values,
                            const std::vector<std::size_t>& indices,
                            const std::vector<std::size_t>& offsets,
                            const std::vector<std::size_t>& splice, std::size_t count, bool repeated = false) {
        LRCaptures result{std::vector<std::vector<std::any>>(count), repeated};
        for (std::size_t i = 0; i < indices.size(); ++i) {
            const auto& value = values.at(indices[i]);
            if (splice[i]) {
                const auto& child = std::any_cast<const LRCaptures&>(value);
                for (std::size_t c = 0; c < child.columns.size(); ++c) {
                    auto& column = result.columns.at(offsets[i] + c);
                    column.insert(column.end(), child.columns[c].begin(), child.columns[c].end());
                }
            } else result.columns.at(offsets[i]).push_back(value);
        }
        return result;
    }
    template<class T>
    static bool lr_convert(T& target, const std::any& value) {
        if (const auto* token = std::any_cast<LRTokenValue>(&value)) {
            if constexpr (std::is_same_v<T, char>) {
                if (token->text.size() != 1) return false;
                target = token->text.front(); return true;
            } else if constexpr (std::is_same_v<T, std::string>) {
                target = token->text; return true;
            } else if constexpr (!is_variant_v<T>) return lr_convert(target, token->node);
            // A variant may contain a literal char/string alternative as well
            // as typed nodes. Keep the token text until alternatives are tried.
        }
        if (const auto* exact = std::any_cast<T>(&value)) { target = *exact; return true; }
        if constexpr (is_node_v<T>) {
            using Storage = typename T::data_type;
            if constexpr (is_shared_ptr<Storage>::value) {
                using Plain = Node<typename is_shared_ptr<Storage>::element_type>;
                if (const auto* plain = std::any_cast<Plain>(&value)) { target = T(*plain); return true; }
            }
        }
        if (const auto* captures = std::any_cast<LRCaptures>(&value)) {
            bool empty = true;
            for (const auto& column : captures->columns) if (!column.empty()) empty = false;
            if (empty) { target = T{}; return true; }
            if constexpr (is_vector_v<T>) {
                target.clear();
                if (captures->repeated && captures->columns.size() > 1) {
                    const auto rows = captures->columns.front().size();
                    for (const auto& column : captures->columns) if (column.size() != rows) return false;
                    for (std::size_t row = 0; row < rows; ++row) {
                        LRCaptures item;
                        for (const auto& column : captures->columns) item.columns.push_back({column[row]});
                        typename T::value_type element{};
                        if (!lr_convert(element, std::any(item))) return false;
                        target.push_back(std::move(element));
                    }
                    return true;
                }
                for (const auto& column : captures->columns) for (const auto& item : column) {
                    typename T::value_type element{};
                    if (lr_convert(element, item)) target.push_back(std::move(element));
                    else {
                        T nested;
                        if (!lr_convert(nested, item)) return false;
                        target.insert(target.end(), nested.begin(), nested.end());
                    }
                }
                return true;
            } else if constexpr (is_tuple_like_v<T> && !is_variant_v<T>) {
                if (captures->columns.size() != std::tuple_size_v<T>) return false;
                return lr_convert_tuple(target, *captures, std::make_index_sequence<std::tuple_size_v<T>>{});
            } else {
                const std::any* selected = nullptr;
                for (const auto& column : captures->columns) for (const auto& item : column) {
                    if (selected) return false;
                    selected = &item;
                }
                if (!selected) { target = T{}; return true; }
                return lr_convert(target, *selected);
            }
        }
        if constexpr (is_variant_v<T>) {
            return lr_convert_variant(target, value, std::make_index_sequence<std::variant_size_v<T>>{});
        }
        return false;
    }
    template<class T>
    static void lr_capture_assign(T& target, const std::vector<std::any>& values,
                                  std::size_t index, std::size_t column) {
        const auto& value = values.at(index);
        if (column == std::numeric_limits<std::size_t>::max()) {
            if (lr_convert(target, value)) return;
        } else {
            const auto& captures = std::any_cast<const LRCaptures&>(value);
            if (lr_convert(target, std::any(LRCaptures{{captures.columns.at(column)}, captures.repeated}))) return;
        }
        throw std::runtime_error("LR capture payload does not match its declared AST field type: " + std::string{typeid(T).name()});
    }
    template<class T>
    static Node<T> lr_get(const std::vector<std::any>& values, std::size_t index) {
        return std::any_cast<Node<T>>(values.at(index));
    }
    template<class T>
    static Node<T> lr_node(T value) {
        Node<T> node;
        node.set_data(std::move(value));
        return node;
    }
    template<class Generated, class IT>
    MatchResult<MainNode, IT> lr_parse(IT pos) {
        errors_.clear();
        LRRegionFrame<IT> region(std::move(pos));
        auto& states = region.states;
        auto& values = region.values;
        auto& spans = region.spans;
        auto& cursor = region.cursor;
        auto& control = region.control;
        auto& position = region.pos;
        auto skip = [](IT& it, std::size_t* offset) {
            if constexpr (detail::detected_v<detail::lr_is_space, Generated, IT>) {
                while (!it.isEnd() && Generated::lr_is_space(*it)) {
                    ++it;
                    if (offset) ++*offset;
                }
            }
        };
        auto gap = [](std::size_t rule, std::size_t index) {
            if constexpr (detail::detected_v<detail::lr_space, Generated>)
                return Generated::lr_space(rule, index);
            else return false;
        };
        for (;;) {
            control.phase = LRControlPhase::Decide;
            skip(position, &cursor);
            auto peek = position;
            std::vector<std::size_t> window;
            for (std::size_t i = 0; i < Generated::lr_k(); ++i) {
                skip(peek, nullptr);
                const bool end = peek.isEnd();
                window.push_back(end ? std::numeric_limits<std::size_t>::max() : (*peek).kind());
                if (!end) ++peek;
            }
            const auto action = Generated::lr_action(states.back(), window);
            if (action == -std::numeric_limits<long long>::max()) {
                Recovery::Diagnostic diagnostic;
                diagnostic.unexpected = window.front();
                if constexpr (detail::detected_v<detail::lr_expected, Generated>)
                    diagnostic.expected = Generated::lr_expected(states.back());
                if constexpr (detail::detected_v<detail::lr_context, Generated>)
                    diagnostic.rule = Generated::lr_context(states.back());
                else diagnostic.rule = "<rule>";
                if (!position.isEnd()) {
                    if constexpr (detail::detected_v<detail::meta_startpos_length, decltype(*position)>) {
                        diagnostic.offset = (*position).meta().startpos();
                        diagnostic.length = (*position).meta().length();
                    }
                }
                std::ostringstream message;
                message << "Unexpected ";
                if constexpr (detail::detected_v<detail::lr_token_name, Generated>)
                    message << Generated::lr_token_name(diagnostic.unexpected);
                else message << "token " << diagnostic.unexpected;
                if (diagnostic.offset != std::numeric_limits<std::size_t>::max())
                    message << " at offset " << diagnostic.offset;
                message << " in rule " << diagnostic.rule;
                if (!diagnostic.expected.empty()) {
                    message << "; expected ";
                    for (std::size_t i = 0; i < diagnostic.expected.size(); ++i) {
                        if (i) message << ", ";
                        if constexpr (detail::detected_v<detail::lr_token_name, Generated>)
                            message << Generated::lr_token_name(diagnostic.expected[i]);
                        else message << "token " << diagnostic.expected[i];
                    }
                }
                diagnostic.message = message.str();
                diagnostic.code = "lr-unexpected-token";
                errors_.push_back(std::move(diagnostic));
                if constexpr (detail::detected_v<detail::lr_recover, Generated, IT>) {
                    control.phase = LRControlPhase::Recover;
                    const auto old_cursor = cursor;
                    const auto old_stack_size = states.size();
                    const auto old_state = states.back();
                    if (static_cast<Generated&>(*this).lr_recover(region, errors_.back())) {
                        if (cursor == old_cursor && states.size() == old_stack_size &&
                            states.back() == old_state)
                            throw std::runtime_error("LR recovery handler made no progress");
                        if (cursor != old_cursor) {
                            if (spans.empty()) region.recovered_prefix = true;
                            else spans.back().trailing = true;
                        }
                        continue;
                    }
                }
                throw std::runtime_error(errors_.back().message);
            }
            if (action == 0) {
                control.phase = LRControlPhase::Accept;
                if (values.size() != 1 || !position.isEnd())
                    throw std::runtime_error("Invalid LR accept stack");
                if ((spans.back().begin && !spans.back().leading && !region.recovered_prefix) ||
                    (cursor > spans.back().end && !spans.back().trailing))
                    throw std::runtime_error("Whitespace is not allowed at the input boundary");
                return {true, std::any_cast<Node<MainNode>>(std::move(values.back())), position};
            }
            if (action > 0) {
                control.phase = LRControlPhase::Shift;
                if (position.isEnd()) throw std::runtime_error("LR shift past end of input");
                values.push_back(Generated::lr_shift(*position));
                spans.push_back(LRRuleSpan{cursor, cursor + 1, false, false, false});
                ++cursor;
                states.push_back(static_cast<std::size_t>(action - 1));
                ++position;
            } else {
                const auto rule = static_cast<std::size_t>(-action - 1);
                control.phase = LRControlPhase::Reduce;
                control.production = rule;
                const auto count = Generated::lr_length(rule);
                if (count > values.size()) throw std::runtime_error("LR stack underflow");
                const auto begin = values.size() - count;
                std::vector<std::any> rhs;
                rhs.reserve(count);
                for (auto i = begin; i < values.size(); ++i) rhs.push_back(std::move(values[i]));
                LRRuleSpan span{cursor, cursor};
                bool allowed = gap(rule, 0);
                for (std::size_t i = 0; i < count; ++i) {
                    const auto& child = spans[begin + i];
                    if (child.empty) allowed = allowed || child.leading || child.trailing;
                    else {
                        if (span.empty) {
                            span.begin = child.begin;
                            span.leading = allowed || child.leading;
                        } else if (child.begin > span.end && !(allowed || child.leading)) {
                            throw std::runtime_error("Whitespace is not allowed between these rule members");
                        }
                        span.empty = false;
                        span.end = child.end;
                        allowed = child.trailing;
                    }
                    allowed = allowed || gap(rule, i + 1);
                }
                span.trailing = allowed;
                if (span.empty) span.leading = allowed;
                std::any value;
                if constexpr (detail::detected_v<detail::lr_semantic_reduce, Generated, IT>) {
                    // Semantic reductions run at the ACTION table's exact
                    // reduce point. The callback may update the cursor and
                    // span, and may enter an independent nested LR region.
                    control.phase = LRControlPhase::Semantic;
                    value = static_cast<Generated&>(*this).lr_semantic_reduce(rule, region, span, rhs);
                } else value = Generated::lr_reduce(rule, rhs);
                values.resize(begin);
                spans.resize(begin);
                states.resize(begin + 1);
                const auto next = Generated::lr_goto(states.back(), Generated::lr_lhs(rule));
                values.push_back(std::move(value));
                spans.push_back(span);
                states.push_back(next);
            }
        }
    }
};
}
