module NFA.IR;
import corelib;
import AST.API;
import AST.types;
import std;

// -- capture-anchor storage -------------------------------------------------
namespace {
    std::deque<TokenID> capture_anchor_pool;

    TokenID *makeCaptureAnchor(AST::RuleMember &member,
                               const stdu::vector<std::string> &token_name,
                               std::size_t position_in_token,
                               std::size_t group = NULL_STATE) {
        capture_anchor_pool.push_back(TokenID{
            .member = member,
            .token_name = token_name,
            .position_in_token = position_in_token,
            .group = group,
        });
        return &capture_anchor_pool.back();
    }
}

bool isTopLevel(const AST::UsePlaceTable &use_table, const stdu::vector<std::string> &name) {
    if (!use_table.contains(name))
        return false;
    const auto usage_names = use_table.at(name);
    for (const auto &name_that_use : usage_names) {
        if (corelib::text::isLower(name_that_use.back()))
            return true;
    }
    return false;
}

auto NFA::captureOf(AST::RuleMember &member,
                    const stdu::vector<std::string> &token_name,
                    std::size_t position_in_token) -> stdu::vector<TokenID *> {
    stdu::vector<TokenID *> captures;
    if (!member.prefix.empty())
        captures.push_back(makeCaptureAnchor(member, token_name, position_in_token));
    return captures;
}

namespace NFA {
    stdu::vector<TokenID> InitialNFA::statesAt(AST::RuleMember &member,
                                               const stdu::vector<std::string> &name, std::size_t i) {
        stdu::vector<TokenID> states;
        if (!member.isOp()) {
            states.push_back(TokenID{.member = member, .token_name = name, .position_in_token = i,
                                     .capture = NFA::captureOf(member, name, i)});
            return states;
        }
        for (const auto &option : member.getOp().options) {
            const bool needs_prefix = option->prefix.empty() && !member.prefix.empty();
            const bool needs_quantifier = option->quantifier == '\0' && member.quantifier != '\0';
            AST::RuleMember *alt = (needs_prefix || needs_quantifier)
                ? synthesize(*option,
                             needs_prefix ? std::optional(member.prefix) : std::nullopt,
                             needs_quantifier ? std::optional(member.quantifier) : std::nullopt)
                : option.get();
            states.push_back(TokenID{.member = *alt, .token_name = name, .position_in_token = i,
                                     .capture = NFA::captureOf(*alt, name, i)});
        }
        return states;
    }

    AST::RuleMember *InitialNFA::synthesize(const AST::RuleMember &base,
                                            std::optional<AST::RulePrefix> prefix_override,
                                            std::optional<char> quantifier_override) {
        auto copy = std::make_unique<AST::RuleMember>(base);
        if (prefix_override && copy->prefix.empty())
            copy->prefix = *prefix_override;
        if (quantifier_override && copy->quantifier == '\0')
            copy->quantifier = *quantifier_override;
        synthesized_members.push_back(std::move(copy));
        return synthesized_members.back().get();
    }

    InitialNFA::Fragment InitialNFA::expandMember(AST::RuleMember &member,
                                                   TokenID *prev_leaf,
                                                   const stdu::vector<std::string> &token_name,
                                                   std::size_t position_in_token,
                                                   std::size_t &site_counter,
                                                   Token &token,
                                                   const stdu::vector<TransitionValue> &next,
                                                   stdu::vector<TokenID *> active_captures,
                                                   const stdu::vector<std::size_t> &alt_path
                                                   ) {
        if (member.quantifier != '\0') {
            auto core_copy = std::make_unique<AST::RuleMember>(member);
            core_copy->quantifier = '\0';
            synthesized_members.push_back(std::move(core_copy));
            AST::RuleMember *core = synthesized_members.back().get();

            Fragment core_fragment = expandMember(*core, prev_leaf, token_name, position_in_token, site_counter, token, next, active_captures, alt_path);

            if (member.quantifier == '*' || member.quantifier == '+') {
                for (const auto &tail : core_fragment.tails) {
                    auto &tail_values = token.transitions.at(tail);
                    for (const auto &e : core_fragment.entries) {
                        if (std::find(tail_values.begin(), tail_values.end(), e) == tail_values.end())
                            tail_values.push_back(e);
                    }
                }
            }

            Fragment result{core_fragment.entries, core_fragment.tails};
            if (member.quantifier == '*' || member.quantifier == '?') {
                result.entries.insert(result.entries.end(), next.begin(), next.end());
            }
            return result;
        }

        if (member.isOp()) {
            Fragment result;
            std::size_t alt_num = 0;
            for (const auto &option : member.getOp().options) {
                AST::RuleMember *alt = (option->prefix.empty() && !member.prefix.empty())
                    ? synthesize(*option, member.prefix, std::nullopt)
                    : option.get();
                // Every alternative gets its own path (outer path + its index) so
                // the leaves it produces are distinct TokenID keys. Building the
                // path once outside the loop pinned every alternative to index 0.
                auto path = alt_path;
                path.push_back(alt_num);
                Fragment sub = expandMember(*alt, prev_leaf, token_name, position_in_token, site_counter,
                                            token, next, active_captures, path);
                result.entries.insert(result.entries.end(), sub.entries.begin(), sub.entries.end());
                result.tails.insert(result.tails.end(), sub.tails.begin(), sub.tails.end());
                ++alt_num;
            }
            return result;
        }

        if (member.isGroup()) {
            auto &group = member.getGroup();

            if (group.values.size() == 1) {
                AST::RuleMember *child = &*group.values[0];
                const bool group_captures = !member.prefix.empty();
                const bool child_captures = !child->prefix.empty();

                if (group_captures && !child_captures) {
                    // Nothing distinguishes the group from its single child — fold
                    // the group's prefix onto it instead of adding a boundary.
                    child = synthesize(*child, member.prefix, std::nullopt);
                    return expandMember(*child, prev_leaf, token_name, position_in_token, site_counter, token, next, active_captures, alt_path);
                }

                stdu::vector<TokenID *> captures = active_captures;
                if (group_captures) {
                    // Group and child both capture independently — the group needs
                    // its own boundary in addition to the child's.
                    captures.push_back(makeCaptureAnchor(member, token_name, position_in_token, site_counter++));
                }
                return expandMember(*child, prev_leaf, token_name, position_in_token, site_counter, token, next, captures, alt_path);
            }
            if (!member.prefix.empty())
                active_captures.push_back(makeCaptureAnchor(member, token_name, position_in_token, site_counter++));

            stdu::vector<Fragment> parts(group.values.size());
            stdu::vector<TransitionValue> cursor_next = next;
            for (std::size_t i = group.values.size(); i-- > 0;) {
                TokenID *element_prev = (i == 0) ? prev_leaf : nullptr; // filled in below for i > 0
                parts[i] = expandMember(*group.values[i], element_prev, token_name, position_in_token, site_counter, token, cursor_next, active_captures, alt_path);
                cursor_next = parts[i].entries;
            }
            // Now that every element's fragment exists, wire prev forward: element i's
            // entries were built with prev_leaf = nullptr (except i == 0); patch them
            // to point at element (i-1)'s tail(s).
            for (std::size_t i = 1; i < parts.size(); ++i) {
                for (auto &entry : parts[i].entries) {
                    // needs to update entry.prev and reflect that in token.transitions' key
                }
            }
            return Fragment{parts.front().entries, parts.back().tails};
        }

        if (!member.prefix.empty())
            active_captures.push_back(makeCaptureAnchor(member, token_name, position_in_token, site_counter));

        TokenID leaf_id{
            .member = member,
            .token_name = token_name,
            .position_in_token = position_in_token,
            .group = site_counter++,
            .alt = alt_path,
            .capture = active_captures,
            .prev = prev_leaf,
        };
        token.transitions[leaf_id] = next;
        return Fragment{{leaf_id}, {leaf_id}};
    }

    stdu::vector<AST::RuleMember *> InitialNFA::resolveTransparent(AST::RuleMember &member) {
        if (member.isGroup()) {
            auto &group = member.getGroup();
            if (group.values.size() != 1)
                return {&member};

            auto resolved = resolveTransparent(*group.values[0]);

            stdu::vector<AST::RuleMember *> result;
            for (auto *leaf : resolved) {
                const bool needs_prefix = leaf->prefix.empty() && !member.prefix.empty();
                const bool needs_quantifier = leaf->quantifier == '\0' && member.quantifier != '\0';
                if (needs_prefix || needs_quantifier) {
                    leaf = synthesize(*leaf,
                                       needs_prefix ? std::optional(member.prefix) : std::nullopt,
                                       needs_quantifier ? std::optional(member.quantifier) : std::nullopt);
                }
                result.push_back(leaf);
            }
            return result;
        }

        if (member.isOp()) {
            stdu::vector<AST::RuleMember *> result;
            for (const auto &option : member.getOp().options) {
                auto resolved = resolveTransparent(*option);
                result.insert(result.end(), resolved.begin(), resolved.end());
            }
            return result;
        }

        return {&member};
    }

    void InitialNFA::build(bool addStoreActions) {
        // Reset static anchor storage arena per build run
        capture_anchor_pool.clear();

        for (const auto &[name, rule] : *tree) {
            if (corelib::text::isLower(name.back())) {
                continue;
            }
            Token token;
            token.name = name;
            token.top_level = tree->getTreeMap().at(name).isTopLevel || isTopLevel(tree->getUsePlacesTable(), name);
            token.data_block = &rule.data_block;
            stdu::vector<stdu::vector<TokenID>> states;
            for (std::size_t i = 0; i < rule.rule_members.size(); ++i)
                states.push_back(statesAt(*rule.rule_members[i], name, i));

            if (!states.empty()) {
                // Inter-position transitions
                for (std::size_t i = 1; i < states.size(); ++i) {
                    for (const auto &from : states[i - 1]) {
                        token.transitions[from] = states[i];
                    }
                }
                // Terminal transitions for trailing states in the rule
                TokenID terminal_marker{.token_name = name, .position_in_token = states.size()};
                for (const auto &last : states.back()) {
                    token.transitions[last] = {terminal_marker};
                }
            }

            tokens.emplace(name, token);
        }

        // Execute structural unrolling passes
        unrollGroups();
        unrollQuantifiers();
        unrollNestedTokens();
    }

    void InitialNFA::unrollGroups() {
        for (auto &[name, token] : tokens) {
            struct Reference {
                TokenID ref;
                stdu::vector<TransitionValue> continuation;
            };
            stdu::vector<Reference> references;

            for (const auto &[token_id, transition_values] : token.transitions) {
                if (!token_id.member.empty() && (token_id.member.isGroup() || token_id.member.quantifier != '\0'))
                    references.push_back(Reference{token_id, transition_values});
            }

            std::size_t site_counter = 0;
            stdu::vector<std::size_t> alt_path;
            for (auto &reference : references) {
                Fragment fragment = expandMember(reference.ref.member, reference.ref.prev, name, reference.ref.position_in_token,
                                                  site_counter, token, reference.continuation, {}, alt_path);

                for (auto &[token_id, transition_values] : token.transitions) {
                    if (token_id == reference.ref)
                        continue;
                    stdu::vector<TransitionValue> rebuilt;
                    bool touched = false;
                    for (auto &v : transition_values) {
                        if (v == reference.ref) {
                            rebuilt.insert(rebuilt.end(), fragment.entries.begin(), fragment.entries.end());
                            touched = true;
                        } else {
                            rebuilt.push_back(v);
                        }
                    }
                    if (touched)
                        transition_values = std::move(rebuilt);
                }

                token.transitions.erase(reference.ref);
            }
        }
    }

    void InitialNFA::unrollQuantifiers() {
        for (auto &[name, token] : tokens) {
            stdu::vector<TokenID> quantified;
            for (const auto &[token_id, transition_values] : token.transitions) {
                if (!token_id.member.empty() && token_id.member.quantifier != '\0')
                    quantified.push_back(token_id);
            }

            for (const auto &state : quantified) {
                const char quantifier = state.member.quantifier;
                auto &own_transitions = token.transitions.at(state);
                const auto continuation = own_transitions;

                if (quantifier == '*' || quantifier == '+') {
                    for (const auto &[other_id, unused] : token.transitions) {
                        if (other_id.token_name != state.token_name || other_id.position_in_token != state.position_in_token)
                            continue;
                        if (std::find(own_transitions.begin(), own_transitions.end(), other_id) == own_transitions.end())
                            own_transitions.push_back(other_id);
                    }
                }

                if (quantifier == '*' || quantifier == '?') {
                    for (auto &[other_id, other_values] : token.transitions) {
                        if (other_id == state)
                            continue;
                        if (std::find(other_values.begin(), other_values.end(), state) == other_values.end())
                            continue;
                        for (const auto &c : continuation) {
                            if (std::find(other_values.begin(), other_values.end(), c) == other_values.end())
                                other_values.push_back(c);
                        }
                    }
                }
            }
        }
    }

    void InitialNFA::unrollNestedTokens() {
        utype::unordered_set<stdu::vector<std::string>> unrolled_to_remove;

        for (auto &[name, token] : tokens) {
            std::size_t site_counter = 0;
            bool changed = true;

            while (changed) {
                changed = false;

                TokenID ref_id;
                stdu::vector<TransitionValue> continuation;
                const Token *nested_token_ptr = nullptr;

                for (const auto &[token_id, transition_values] : token.transitions) {
                    if (token_id.member.empty())
                        continue;
                    const auto &member = token_id.member;
                    if (!member.isName())
                        continue;
                    const auto nested_name = member.getName().name;
                    if (nested_name == name || corelib::text::isLower(nested_name.back()))
                        continue;
                    const auto nested_it = tokens.find(nested_name);
                    if (nested_it == tokens.end())
                        continue;

                    ref_id = token_id;
                    continuation = transition_values;
                    nested_token_ptr = &nested_it->second;
                    changed = true;
                    break;
                }

                if (!changed)
                    break;

                const auto &nested_token = *nested_token_ptr;
                stdu::vector<TransitionValue> start_states;

                for (const auto &[nested_token_id, nested_transition_values] : nested_token.transitions) {
                    TokenID remapped_id = nested_token_id;
                    remapped_id.call = site_counter;

                    stdu::vector<TransitionValue> remapped_values;
                    for (auto remapped_value : nested_transition_values) {
                        if (remapped_value.member.empty()) {
                            remapped_values.insert(remapped_values.end(),
                                                   continuation.begin(), continuation.end());
                            continue;
                        }
                        if (remapped_value.token_name == nested_token.name) {
                            remapped_value.call = site_counter;
                        }
                        remapped_values.push_back(remapped_value);
                    }

                    auto [it, inserted] = token.transitions.try_emplace(remapped_id, remapped_values);
                    if (!inserted)
                        it->second.insert(it->second.end(), remapped_values.begin(), remapped_values.end());

                    if (nested_token_id.position_in_token == 0)
                        start_states.push_back(remapped_id);
                }

                for (auto &[token_id, transition_values] : token.transitions) {
                    if (token_id == ref_id)
                        continue;
                    stdu::vector<TransitionValue> rebuilt;
                    bool touched = false;
                    for (auto &v : transition_values) {
                        if (v == ref_id) {
                            rebuilt.insert(rebuilt.end(), start_states.begin(), start_states.end());
                            touched = true;
                        } else {
                            rebuilt.push_back(v);
                        }
                    }
                    if (touched)
                        transition_values = std::move(rebuilt);
                }

                token.transitions.erase(ref_id);

                if (!nested_token.top_level)
                    unrolled_to_remove.insert(nested_token.name);

                ++site_counter;
            }
        }

        for (const auto &name : unrolled_to_remove) {
            tokens.erase(name);
        }
    }
}