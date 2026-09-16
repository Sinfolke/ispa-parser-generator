module NFA.IR;
import corelib;
import AST.API;
import AST.types;
import std;

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
auto tranfsformOP(const AST::RuleMemberOp member, const stdu::vector<std::string> &name, std::size_t i) -> stdu::vector<TransitionValue> {
    stdu::vector<TransitionValue> transition_values;
    for (const auto &option : member.options) {
        TransitionValue transition_value{TokenID {.member = &*option, .token_name = name, .position_in_token = i}};
        transition_values.push_back(transition_value);
    }
    return transition_values;
}
namespace NFA {
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
                                                   const stdu::vector<std::string> &token_name,
                                                   std::size_t position_in_token,
                                                   std::size_t &site_counter,
                                                   Token &token,
                                                   const stdu::vector<TransitionValue> &next) {
        if (member.quantifier != '\0') {
            // Build the unquantified core once (quantifier stripped on a clone), then patch
            // in loop-back / bypass behavior around it rather than guessing at compound semantics.
            auto core_copy = std::make_unique<AST::RuleMember>(member);
            core_copy->quantifier = '\0';
            synthesized_members.push_back(std::move(core_copy));
            AST::RuleMember *core = synthesized_members.back().get();

            Fragment core_fragment = expandMember(*core, token_name, position_in_token, site_counter, token, next);

            if (member.quantifier == '*' || member.quantifier == '+') {
                // Loop back: finishing one repetition can start another.
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
                // Zero occurrences allowed: whoever reaches this slot can also skip straight past it.
                result.entries.insert(result.entries.end(), next.begin(), next.end());
            }
            return result;
        }

        if (member.isOp()) {
            Fragment result;
            for (const auto &option : member.getOp().options) {
                Fragment sub = expandMember(*option, token_name, position_in_token, site_counter, token, next);
                result.entries.insert(result.entries.end(), sub.entries.begin(), sub.entries.end());
                result.tails.insert(result.tails.end(), sub.tails.begin(), sub.tails.end());
            }
            return result;
        }

        if (member.isGroup()) {
            auto &group = member.getGroup();

            if (group.values.size() == 1) {
                // Transparent wrapper: recurse into the sole child, carrying this group's own
                // `@` prefix marker down if the child doesn't already have one.
                AST::RuleMember *child = &*group.values[0];
                if (child->prefix.empty() && !member.prefix.empty())
                    child = synthesize(*child, member.prefix, std::nullopt);
                return expandMember(*child, token_name, position_in_token, site_counter, token, next);
            }

            // Real multi-member sequence: spell it out as a chain of individual states
            // (member[0] -> member[1] -> ... -> member[n-1] -> next) instead of leaving
            // the whole group as one opaque blob.
            stdu::vector<Fragment> parts(group.values.size());
            stdu::vector<TransitionValue> cursor_next = next;
            for (std::size_t i = group.values.size(); i-- > 0;) {
                parts[i] = expandMember(*group.values[i], token_name, position_in_token, site_counter, token, cursor_next);
                cursor_next = parts[i].entries;
            }
            return Fragment{parts.front().entries, parts.back().tails};
        }

        // True terminal: literal, char class, name reference, etc. -- a single leaf state,
        // tagged with this call site so repeated unroll occurrences stay distinct.
        TokenID leaf_id{&member, token_name, position_in_token, .group = site_counter++};
        token.transitions[leaf_id] = next;
        return Fragment{{leaf_id}, {leaf_id}};
    }
    stdu::vector<AST::RuleMember *> InitialNFA::resolveTransparent(AST::RuleMember &member) {
        if (member.isGroup()) {
            auto &group = member.getGroup();
            if (group.values.size() != 1)
                return {&member}; // real multi-member sequence group: left opaque for now

            auto resolved = resolveTransparent(*group.values[0]); // recurse through nested transparent groups/alternations

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
            // An alternation reached while unwrapping a transparent group is not
            // itself a single state — each option is its own leaf, same as if
            // it had appeared directly as a group value. Mirrors tranfsformOP.
            stdu::vector<AST::RuleMember *> result;
            for (const auto &option : member.getOp().options) {
                auto resolved = resolveTransparent(*option);
                result.insert(result.end(), resolved.begin(), resolved.end());
            }
            return result;
        }

        return {&member}; // true terminal: literal, char class, name reference, etc.
    }
    void InitialNFA::build(bool addStoreActions) {
        for (const auto &[name, rule] : *tree) {
            if (corelib::text::isLower(name.back())) {
                continue;
            }
            Token token;
            token.name = name;
            token.top_level = isTopLevel(tree->getUsePlacesTable(), name);

            for (std::size_t i = 1; i < rule.rule_members.size(); ++i) {
                auto &prev_member = *rule.rule_members[i - 1];
                auto &member = *rule.rule_members[i];
                stdu::vector<TokenID> transition_symbols = prev_member.isOp() ?
                    tranfsformOP(prev_member.getOp(), name, i - 1) : stdu::vector<TokenID>{TokenID{&prev_member, name, i - 1}};
                stdu::vector<TokenID> transition_values = member.isOp() ?
                    tranfsformOP(member.getOp(), name, i) : stdu::vector<TokenID>{TokenID{&member, name, i}};
                for (const auto &transition_symbol : transition_symbols) {
                    token.transitions[transition_symbol] = transition_values;
                }
            }
            auto &last_member = *rule.rule_members.back();
            std::size_t i = rule.rule_members.size() - 1;
            stdu::vector<TokenID> transition_symbols = last_member.isOp() ?
                    tranfsformOP(last_member.getOp(), name, i) : stdu::vector<TokenID>{TokenID{&last_member, name, i}};
            TransitionValue transition_value{TokenID {}};
            for (const auto &sym : transition_symbols) {
                token.transitions[sym].push_back(transition_value);
            }
            tokens.emplace(name, token);
        }
    }

    void InitialNFA::unrollGroups() {
        for (auto &[name, token] : tokens) {
            struct Reference {
                TokenID ref;
                stdu::vector<TransitionValue> continuation;
            };
            stdu::vector<Reference> references;

            for (const auto &[token_id, transition_values] : token.transitions) {
                if (token_id.member != nullptr && token_id.member->isGroup())
                    references.push_back(Reference{token_id, transition_values});
            }

            std::size_t site_counter = 0;
            for (auto &reference : references) {
                Fragment fragment = expandMember(*reference.ref.member, name, reference.ref.position_in_token,
                                                  site_counter, token, reference.continuation);

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
                if (token_id.member != nullptr && token_id.member->quantifier != '\0')
                    quantified.push_back(token_id);
            }

            for (const auto &state : quantified) {
                const char quantifier = state.member->quantifier;
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
                    bool found_predecessor = false;
                    for (auto &[other_id, other_values] : token.transitions) {
                        if (other_id == state)
                            continue;
                        if (std::find(other_values.begin(), other_values.end(), state) == other_values.end())
                            continue;
                        for (const auto &c : continuation) {
                            if (std::find(other_values.begin(), other_values.end(), c) == other_values.end())
                                other_values.push_back(c);
                        }
                        found_predecessor = true;
                    }
                    if (!found_predecessor) {
                        // TODO: `state` has no in-token predecessor -> it's only
                        // reachable as a start alternative (position 0). Bypassing
                        // it means `continuation` needs to become a start
                        // alternative too. I don't know how start states are
                        // recorded/consumed downstream, so left unhandled rather
                        // than guessed at.
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
                    if (token_id.member == nullptr)
                        continue;
                    const auto &member = *token_id.member;
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
                    // PRESERVE original rule_name (e.g., rule::CSEQUENCE::SYMBOL);
                    // only attach the call site counter to disambiguate the instance.
                    remapped_id.call = site_counter;

                    stdu::vector<TransitionValue> remapped_values;
                    for (auto remapped_value : nested_transition_values) {
                        if (remapped_value.member == nullptr) {
                            remapped_values.insert(remapped_values.end(),
                                                    continuation.begin(), continuation.end());
                            continue;
                        }
                        if (remapped_value.token_name == nested_token.name) {
                            // PRESERVE original rule_name for internal transitions
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
