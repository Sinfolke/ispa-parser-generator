#pragma once
#include <ispastdlib/lr_machine.hpp>
#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>

namespace ISPA_STD {
// Instantiated only by generated parsers with unresolved conflict cells. Stacks
// share immutable prefixes and reductions share a packed forest. No semantic
// callbacks run until the complete input has exactly one surviving derivation.
template<class MainNode, class Token>
class LRGeneralizedParser : public LRParser<MainNode, Token> {
protected:
    template<class Generated, class IT>
    MatchResult<MainNode, IT> lr_parse_generalized(IT pos) {
        constexpr auto conflict_action = std::numeric_limits<long long>::max();
        constexpr auto invalid_action = -std::numeric_limits<long long>::max();
        constexpr auto absent = std::numeric_limits<std::size_t>::max();
        constexpr std::size_t node_limit = 250000, work_limit = 2000000;
        struct BranchSpan {
            std::size_t begin = 0, end = 0;
            bool leading = false, trailing = false, empty = true;
        };
        struct Alternative {
            std::size_t rule;
            std::vector<std::size_t> children;
            bool operator==(const Alternative& other) const {
                return rule == other.rule && children == other.children;
            }
        };
        struct Forest {
            BranchSpan span;
            std::optional<IT> token;
            std::vector<Alternative> alternatives;
        };
        struct Stack { std::size_t previous, state, forest; };
        using ForestKey = std::tuple<bool, std::size_t, std::size_t, std::size_t, bool, bool, bool>;
        std::map<ForestKey, std::size_t> forest_index;
        std::map<std::tuple<std::size_t, std::size_t, std::size_t>, std::size_t> stack_index;
        std::vector<Forest> forest;
        std::vector<Stack> stacks{{absent, 0, absent}};
        std::size_t work = 0, cursor = 0;
        auto tick = [&] {
            if (++work > work_limit || forest.size() + stacks.size() > node_limit)
                throw std::runtime_error("LR automatic conflict resolution resource limit exceeded");
        };
        auto intern_forest = [&](bool terminal, std::size_t symbol, BranchSpan span, std::optional<IT> token) {
            tick();
            ForestKey key{terminal, symbol, span.begin, span.end, span.leading, span.trailing, span.empty};
            auto [it, inserted] = forest_index.emplace(key, forest.size());
            if (inserted) forest.push_back(Forest{span, std::move(token), {}});
            return it->second;
        };
        auto push = [&](std::size_t previous, std::size_t state, std::size_t value) {
            tick();
            auto [it, inserted] = stack_index.emplace(std::make_tuple(previous, state, value), stacks.size());
            if (inserted) stacks.push_back(Stack{previous, state, value});
            return it->second;
        };
        auto skip = [](IT& it, std::size_t* offset) {
            if constexpr (detail::detected_v<detail::lr_is_space, Generated, IT>) {
                while (!it.isEnd() && Generated::lr_is_space(*it)) { ++it; if (offset) ++*offset; }
            }
        };
        auto gap = [](std::size_t rule, std::size_t index) {
            if constexpr (detail::detected_v<detail::lr_space, Generated>) return Generated::lr_space(rule, index);
            else return false;
        };
        std::set<std::size_t> active{0}, roots;
        for (;;) {
            skip(pos, &cursor);
            auto peek = pos;
            std::vector<std::size_t> window;
            for (std::size_t i = 0; i < Generated::lr_k(); ++i) {
                skip(peek, nullptr);
                const bool end = peek.isEnd();
                window.push_back(end ? absent : (*peek).kind());
                if (!end) ++peek;
            }
            std::vector<std::size_t> pending(active.begin(), active.end());
            std::set<std::size_t> closure = active, shifted;
            for (std::size_t next = 0; next < pending.size(); ++next) {
                tick();
                const auto top_id = pending[next];
                const auto top = stacks[top_id];
                const auto selected = Generated::lr_action(top.state, window);
                std::vector<long long> actions;
                if (selected == conflict_action) actions = Generated::lr_conflict(top.state, window);
                else if (selected != invalid_action) actions.push_back(selected);
                for (const auto action : actions) {
                    tick();
                    if (action == 0) {
                        if (!pos.isEnd() || top.forest == absent || top.previous != 0) continue;
                        const auto& span = forest[top.forest].span;
                        if ((span.begin && !span.leading) || (cursor > span.end && !span.trailing)) continue;
                        roots.insert(top.forest);
                    } else if (action > 0) {
                        if (pos.isEnd()) continue;
                        const auto leaf = intern_forest(true, (*pos).kind(), BranchSpan{cursor, cursor + 1, false, false, false}, pos);
                        shifted.insert(push(top_id, static_cast<std::size_t>(action - 1), leaf));
                    } else {
                        const auto rule = static_cast<std::size_t>(-action - 1);
                        const auto count = Generated::lr_length(rule);
                        std::vector<std::size_t> children;
                        auto base = top_id;
                        for (std::size_t i = 0; i < count && base != 0; ++i) {
                            children.push_back(stacks[base].forest);
                            base = stacks[base].previous;
                        }
                        if (children.size() != count) continue;
                        std::reverse(children.begin(), children.end());
                        BranchSpan span{cursor, cursor};
                        bool allowed = gap(rule, 0), valid = true;
                        for (std::size_t i = 0; i < children.size(); ++i) {
                            const auto& child = forest[children[i]].span;
                            if (child.empty) allowed = allowed || child.leading || child.trailing;
                            else {
                                if (span.empty) { span.begin = child.begin; span.leading = allowed || child.leading; }
                                else if (child.begin > span.end && !(allowed || child.leading)) valid = false;
                                span.empty = false;
                                span.end = child.end;
                                allowed = child.trailing;
                            }
                            allowed = allowed || gap(rule, i + 1);
                        }
                        if (!valid) continue;
                        span.trailing = allowed;
                        if (span.empty) span.leading = allowed;
                        const auto lhs = Generated::lr_lhs(rule);
                        const auto value = intern_forest(false, lhs, span, std::nullopt);
                        auto& alternatives = forest[value].alternatives;
                        Alternative alternative{rule, std::move(children)};
                        if (std::find(alternatives.begin(), alternatives.end(), alternative) == alternatives.end())
                            alternatives.push_back(std::move(alternative));
                        const auto dest = push(base, Generated::lr_goto(stacks[base].state, lhs), value);
                        if (closure.insert(dest).second) pending.push_back(dest);
                    }
                }
            }
            if (pos.isEnd()) break;
            if (shifted.empty()) throw std::runtime_error("Unexpected input during LR automatic conflict resolution at token " + std::to_string(cursor));
            active = std::move(shifted);
            ++pos;
            ++cursor;
        }
        if (roots.empty()) throw std::runtime_error("Unexpected end of input during LR automatic conflict resolution");
        std::vector<unsigned char> visiting(forest.size(), 0), ways(forest.size(), 0);
        std::size_t ambiguous = absent;
        struct Visit { std::size_t node, alternative = 0, child = 0; };
        std::vector<std::size_t> order;
        for (auto root : roots) {
            if (visiting[root] == 2) continue;
            std::vector<Visit> pending{{root}};
            visiting[root] = 1;
            while (!pending.empty() && ambiguous == absent) {
                tick();
                auto& frame = pending.back();
                const auto& node = forest[frame.node];
                if (frame.alternative == node.alternatives.size()) {
                    visiting[frame.node] = 2;
                    order.push_back(frame.node);
                    pending.pop_back();
                } else {
                    const auto& alternative = node.alternatives[frame.alternative];
                    if (frame.child == alternative.children.size()) { ++frame.alternative; frame.child = 0; }
                    else {
                        const auto child = alternative.children[frame.child++];
                        if (visiting[child] == 1) ambiguous = child; // productive nullable cycle
                        else if (!visiting[child]) { visiting[child] = 1; pending.push_back(Visit{child}); }
                    }
                }
            }
            if (ambiguous != absent) break;
        }
        unsigned total = 0;
        if (ambiguous != absent) total = 2;
        else {
            for (auto id : order) {
                unsigned n = forest[id].token ? 1 : 0;
                for (const auto& alternative : forest[id].alternatives) {
                    unsigned product = 1;
                    for (auto child : alternative.children) product = std::min(2u, product * ways[child]);
                    n = std::min(2u, n + product);
                }
                ways[id] = static_cast<unsigned char>(n);
                if (n > 1 && ambiguous == absent) ambiguous = id;
            }
            for (auto root : roots) total = std::min(2u, total + ways[root]);
        }
        if (total != 1) {
            std::string message = "Ambiguous LR input: multiple complete parses";
            if (ambiguous != absent) {
                const auto& node = forest[ambiguous];
                message += " at tokens " + std::to_string(node.span.begin) + ".." + std::to_string(node.span.end);
                for (const auto& alternative : node.alternatives) {
                    if constexpr (detail::detected_v<detail::lr_rule_label, Generated>)
                        message += "\n  " + Generated::lr_rule_label(alternative.rule);
                    else message += "\n  production " + std::to_string(alternative.rule);
                }
            }
            throw std::runtime_error(message);
        }
        // Evaluate the chosen tree after recognition. Do not memoize semantic
        // results: two occurrences of a shared epsilon subtree still require
        // two action invocations. Iterative traversal also handles deep trees.
        struct Evaluation { std::size_t node, child = 0; std::vector<std::any> rhs; };
        std::vector<Evaluation> pending{{*roots.begin()}};
        std::any result;
        while (!pending.empty()) {
            tick();
            auto& frame = pending.back();
            const auto& node = forest[frame.node];
            if (!node.token && frame.child < node.alternatives.front().children.size()) {
                const auto child = node.alternatives.front().children[frame.child++];
                pending.push_back(Evaluation{child});
                continue;
            }
            auto value = node.token ? Generated::lr_shift(**node.token)
                : Generated::lr_reduce(node.alternatives.front().rule, frame.rhs);
            pending.pop_back();
            if (pending.empty()) result = std::move(value);
            else pending.back().rhs.push_back(std::move(value));
        }
        return {true, std::any_cast<Node<MainNode>>(std::move(result)), pos};
    }
};
}
