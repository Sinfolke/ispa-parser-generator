module NFA.IR;
import corelib;
import AST.API;
import AST.types;
import std;

// -- capture-anchor storage -------------------------------------------------
namespace {
    std::deque<TokenID> capture_anchor_pool;

    bool hasVariableCapture(const AST::RuleMember& member) {
        return std::ranges::any_of(member.features, [](const auto& feature) {
            return std::holds_alternative<AST::VariableCapture>(feature);
        });
    }

    TokenID *makeCaptureAnchor(AST::RuleMember &member,
                               const AST::RuleMember& original_member,
                               const stdu::vector<std::string> &token_name,
                               std::size_t position_in_token,
                               std::size_t group = NULL_STATE) {
        capture_anchor_pool.push_back(TokenID{
            .member = member,
            .original_member = original_member,
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
                    const AST::RuleMember& original_member,
                    const stdu::vector<std::string> &token_name,
                    std::size_t position_in_token) -> stdu::vector<TokenID *> {
    stdu::vector<TokenID *> captures;
    if (!member.prefix.empty())
        captures.push_back(makeCaptureAnchor(member, original_member, token_name, position_in_token));
    else if (hasVariableCapture(member))
        captures.push_back(makeCaptureAnchor(member, original_member, token_name, position_in_token));
    return captures;
}

namespace NFA {
    stdu::vector<TokenID> InitialNFA::statesAt(AST::RuleMember &member,
                                               const AST::RuleMember& original_member,
                                               const stdu::vector<std::string> &name, std::size_t i) {
        stdu::vector<TokenID> states;
        if (!member.isOp()) {
            states.push_back(TokenID{.member = member, .original_member = original_member, .token_name = name, .position_in_token = i,
                                     .capture = NFA::captureOf(member, original_member, name, i)});
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
            states.push_back(TokenID{.member = *alt, .original_member = *option, .token_name = name, .position_in_token = i,
                                     .capture = NFA::captureOf(*alt, *option, name, i)});
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
                                                   const AST::RuleMember& original_member,
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

            Fragment core_fragment = expandMember(*core, original_member, prev_leaf, token_name, position_in_token, site_counter, token, next, active_captures, alt_path);

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
                Fragment sub = expandMember(*alt, original_member, prev_leaf, token_name, position_in_token, site_counter,
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
                const bool group_captures = !member.prefix.empty() || hasVariableCapture(member);
                const bool child_captures = !child->prefix.empty();

                // Folding the prefix onto the child is only valid for a leaf or
                // a plain sequence. An op child would spread the prefix over its
                // options as separate anchors, splitting one logical capture
                // (e.g. `@ (('\\' .) | [^'\\])*`) into unrelated ones.
                if (group_captures && !hasVariableCapture(member) && !child_captures && !child->isOp()) {
                    child = synthesize(*child, member.prefix, std::nullopt);
                    return expandMember(*child, original_member, prev_leaf, token_name,
                                        position_in_token, site_counter, token, next,
                                        active_captures, alt_path);
                }

                // One anchor for the whole group, inherited by every leaf under
                // the child, including every option of an op.
                stdu::vector<TokenID *> captures = active_captures;
                if (group_captures) {
                    captures.push_back(makeCaptureAnchor(member, original_member, token_name,
                                                         position_in_token, site_counter++));
                }
                return expandMember(*child, original_member, prev_leaf, token_name,
                                    position_in_token, site_counter, token, next,
                                    captures, alt_path);
            }

            if (!member.prefix.empty() || hasVariableCapture(member)) {
                active_captures.push_back(makeCaptureAnchor(member, original_member, token_name,
                                                            position_in_token, site_counter++));
            }

            // Build back to front so each element knows its continuation.
            stdu::vector<Fragment> parts(group.values.size());
            stdu::vector<TransitionValue> cursor_next = next;
            for (std::size_t i = group.values.size(); i-- > 0;) {
                TokenID *element_prev = (i == 0) ? prev_leaf : nullptr;
                parts[i] = expandMember(*group.values[i], original_member, element_prev,
                                        token_name, position_in_token, site_counter, token,
                                        cursor_next, active_captures, alt_path);
                cursor_next = parts[i].entries;
            }
            // NOTE: the empty "patch prev forward" loop was removed. `prev` stays
            // nullptr for elements i > 0. That is only safe while TokenID's
            // operator==/hash ignore `prev`; if they use it, set it here AND
            // re-key token.transitions.
            return Fragment{parts.front().entries, parts.back().tails};
        }

        if (!member.prefix.empty() || hasVariableCapture(member)) {
            active_captures.push_back(makeCaptureAnchor(member, original_member, token_name,
                                                        position_in_token, site_counter));
        }

        TokenID leaf_id{
            .member = member,
            .original_member = original_member,
            .token_name = token_name,          // <- comma was missing
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
            if (rule.external_token || rule.isTemplateDefinition || corelib::text::isLower(name.back())) {
                continue;
            }
            Token token;
            token.name = name;
            token.top_level = tree->getTreeMap().at(name).isTopLevel || isTopLevel(tree->getUsePlacesTable(), name);
            token.data_block = &rule.data_block;
            stdu::vector<stdu::vector<TokenID>> states;
            for (std::size_t i = 0; i < rule.rule_members.size(); ++i)
                states.push_back(statesAt(*rule.rule_members[i], *rule.rule_members[i], name, i));

            if (!states.empty()) {
                // Inter-position transitions
                for (std::size_t i = 1; i < states.size(); ++i) {
                    for (const auto &from : states[i - 1]) {
                        token.transitions[from] = states[i];
                    }
                }
                // Terminal transitions for trailing states in the rule
                TokenID terminal_marker{.original_member = {}, .token_name = name, .position_in_token = states.size()};
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
            for (std::size_t ri = 0; ri < references.size(); ++ri) {
                auto &reference = references[ri];
                Fragment fragment = expandMember(reference.ref.member, reference.ref.original_member, reference.ref.prev, name, reference.ref.position_in_token,
                                                  site_counter, token, reference.continuation, {}, alt_path);

                // Substitute in the live map, same as before.
                for (auto &[token_id, transition_values] : token.transitions) {
                    if (token_id == reference.ref) continue;
                    stdu::vector<TransitionValue> rebuilt;
                    bool touched = false;
                    for (auto &v : transition_values) {
                        if (v == reference.ref) { rebuilt.insert(rebuilt.end(), fragment.entries.begin(), fragment.entries.end()); touched = true; }
                        else rebuilt.push_back(v);
                    }
                    if (touched) transition_values = std::move(rebuilt);
                }

                // NEW: also patch every remaining reference's *own* cached
                // continuation, since those are independent snapshots that the
                // live-map loop above can never reach. Without this, a
                // not-yet-processed reference whose original continuation
                // pointed at `reference.ref` bakes in a dangling TokenID the
                // moment IT is expanded, later, from stale data.
                for (std::size_t rj = ri + 1; rj < references.size(); ++rj) {
                    stdu::vector<TransitionValue> rebuilt;
                    bool touched = false;
                    for (auto &v : references[rj].continuation) {
                        if (v == reference.ref) { rebuilt.insert(rebuilt.end(), fragment.entries.begin(), fragment.entries.end()); touched = true; }
                        else rebuilt.push_back(v);
                    }
                    if (touched) references[rj].continuation = std::move(rebuilt);
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
                    // Completion markers retain the original named member, but
                    // are zero-width boundaries, NOT nested references to expand.
                    // Expanding one recreates itself indefinitely.
                    if (token_id.member.empty() || token_id.nested_completion)
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

                // 1. Create a consistent mapping of original IDs to remapped IDs.
                utype::unordered_map<TokenID, TokenID> remapped;
                remapped.reserve(nested_token.transitions.size());

                for (const auto& [id, transitions] : nested_token.transitions) {
                    TokenID new_id = id;
                    new_id.call = site_counter;

                    remapped.emplace(id, std::move(new_id));
                }

                // 2. Rebuild transitions using the remapped IDs.
                for (const auto& [id, transitions] : nested_token.transitions) {
                    const TokenID& new_id = remapped.at(id);

                    stdu::vector<TransitionValue> new_transitions;

                    for (const auto& next : transitions) {
                        if (next.member.empty()) {
                            // Keep the call boundary instead of splicing the nested
                            // accept directly into the host continuation.
                            TokenID completion = ref_id;
                            completion.call = site_counter;
                            completion.nested_completion = true;
                            token.transitions[completion] = continuation;
                            new_transitions.push_back(completion);
                        } else {
                            auto it = remapped.find(next);

                            if (it != remapped.end()) {
                                new_transitions.push_back(it->second);
                            } else {
                                // A target outside the copied token graph.
                                new_transitions.push_back(next);
                            }
                        }
                    }

                    auto [it, inserted] =
                        token.transitions.try_emplace(new_id, new_transitions);

                    if (!inserted) {
                        it->second.insert(
                            it->second.end(),
                            new_transitions.begin(),
                            new_transitions.end()
                        );
                    }

                    if (id.position_in_token == 0) {
                        start_states.push_back(new_id);
                    }
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

        // Keep nested token metadata available for semantic reduction. The
        // TNFA builder will only expose top-level tokens as entry points.
        // (No erase: nested token definitions provide their data blocks.)
    }
}
