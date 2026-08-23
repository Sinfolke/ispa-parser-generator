module DFA.functionality;

import DFA.API;
import DFA.Base;
import DFA;
import corelib;
import logging;
import hash;
import std;

void DFA::mergeTwoNFA(
    NFA &first,
    NFA &second,
    std::size_t rule_idx
) {
    if (second.getStates().empty()) {
        second.build(true);
    }

    if (second.getStates().empty()) {
        return;
    }

    const std::size_t state_offset = first.getStates().size();
    const std::size_t action_offset = first.getActionTable().size();
    const std::size_t semantic_offset = first.getSemanticTable().size();

    auto &first_states = first.getStates();
    const auto &second_states = second.getStates();

    /*
     * ------------------------------------------------------------
     * Copy states with table-type aware rebasing
     * ------------------------------------------------------------
     */
    for (const auto &state : second_states) {
        auto new_state = state;

        // Symbol transitions: Rebase depending on target table type
        for (auto &[symbol, target_ids] : new_state.transitions) {
            for (auto &target : target_ids) {
                if (target.next != NFA::NULL_STATE) {
                    if (target.table_type == NFA::TableType::Action) {
                        target.next += action_offset;
                    } else if (target.table_type == NFA::TableType::Semantic) {
                        target.next += semantic_offset;
                    } else {
                        target.next += state_offset;
                    }
                }
            }
        }

        // Epsilon transitions: Rebase depending on target table type
        utype::unordered_set<NFA::TransitionValue> rebased_epsilon;
        for (auto target : new_state.epsilon_transitions) {
            if (target.next != NFA::NULL_STATE) {
                if (target.table_type == NFA::TableType::Action) {
                    target.next += action_offset;
                } else if (target.table_type == NFA::TableType::Semantic) {
                    target.next += semantic_offset;
                } else {
                    target.next += state_offset;
                }
            }
            rebased_epsilon.insert(target);
        }
        new_state.epsilon_transitions = std::move(rebased_epsilon);

        // Accept binding target state & rule rebasing
        if (new_state.accept_binding.has_value()) {
            auto &binding = *new_state.accept_binding;

            if (binding.target_semantic_state.has_value() && *binding.target_semantic_state != NFA::NULL_STATE) {
                *binding.target_semantic_state += semantic_offset;
            }
            if (rule_idx != NFA::NULL_STATE) {
                binding.reduce_rule_id = rule_idx;
            }
        }

        first_states.emplace_back(std::move(new_state));
    }

    /*
     * ------------------------------------------------------------
     * Merge start states (State 0 -> state_offset via Epsilon)
     * ------------------------------------------------------------
     */
    first_states[0].epsilon_transitions.insert({
        state_offset,
        NFA::TableType::DFA
    });

    /*
     * ------------------------------------------------------------
     * Merge LR / Action table
     * ------------------------------------------------------------
     */
    {
        auto &first_actions = first.getActionTable();
        auto second_actions = second.getActionTable();

        for (auto &action : second_actions) {
            // Rebase next NFA state reference
            if (action.next_nfa_state != NFA::NULL_STATE) {
                action.next_nfa_state += state_offset;
            }

            // Rebase variant target state
            std::visit([&](auto &&target) {
                using T = std::decay_t<decltype(target)>;
                if constexpr (std::is_same_v<T, NFA::DFATarget>) {
                    if (target.id != NFA::NULL_STATE) {
                        target.id += state_offset;
                    }
                } else if constexpr (std::is_same_v<T, NFA::ActionTarget>) {
                    if (target.id != NFA::NULL_STATE) {
                        target.id += action_offset;
                    }
                } else if constexpr (std::is_same_v<T, NFA::SemanticTarget>) {
                    if (target.id != NFA::NULL_STATE) {
                        target.id += semantic_offset;
                    }
                }
            }, action.next_state);
        }

        first_actions.insert(
            first_actions.end(),
            std::make_move_iterator(second_actions.begin()),
            std::make_move_iterator(second_actions.end())
        );
    }

    /*
     * ------------------------------------------------------------
     * Merge Semantic table
     * ------------------------------------------------------------
     */
    {
        auto &first_semantic = first.getSemanticTable();
        auto second_semantic = second.getSemanticTable();

        for (auto &semantic : second_semantic) {
            // Rebase NFA index reference
            if (semantic.nfa_index != NFA::NULL_STATE) {
                semantic.nfa_index += state_offset;
            }

            // Rebase variant target state
            std::visit([&](auto &&target) {
                using T = std::decay_t<decltype(target)>;
                if constexpr (std::is_same_v<T, NFA::DFATarget>) {
                    if (target.id != NFA::NULL_STATE) {
                        target.id += state_offset;
                    }
                } else if constexpr (std::is_same_v<T, NFA::ActionTarget>) {
                    if (target.id != NFA::NULL_STATE) {
                        target.id += action_offset;
                    }
                } else if constexpr (std::is_same_v<T, NFA::SemanticTarget>) {
                    if (target.id != NFA::NULL_STATE) {
                        target.id += semantic_offset;
                    }
                }
            }, semantic.next_state);
        }

        first_semantic.insert(
            first_semantic.end(),
            std::make_move_iterator(second_semantic.begin()),
            std::make_move_iterator(second_semantic.end())
        );
    }
}

// functionality_2.cpp in DFA::mergeNFAS
auto DFA::mergeNFAS(
    const stdu::vector<NFA> &nfas
) -> std::pair<NFA, std::size_t> {
    NFA merged = nfas[0];

    if (merged.getStates().empty()) {
        merged.build(true);
    }

    std::size_t max_registers_count = merged.getRegistersCount();
    for (std::size_t i = 1; i < nfas.size(); ++i) {
        NFA next = nfas[i];
        if (next.getStates().empty()) {
            next.build(true);
        }

        mergeTwoNFA(
            merged,
            next,
            i
        );
        max_registers_count = std::max(max_registers_count, next.getRegistersCount());
    }
    merged.buildAcceptMap();
    return std::make_pair(merged, max_registers_count);
}
auto DFA::build(const AST::Tree &ast, const NFA &nfa) -> DFA {
    auto mutable_nfa = nfa;
    DFA dfa(&mutable_nfa);
    dfa.build();
    dfa.minimize();
    return dfa;
}
auto DFA::build(const AST::Tree &ast, const stdu::vector<NFA> &nfa_collection) -> std::tuple<ClassifiedDFA, stdu::vector<NFA::ActionState>, stdu::vector<NFA::SemanticState>, std::size_t> {
    auto [mergedNFA, max_registers_count] = mergeNFAS(nfa_collection);
    // construct tables
    auto dfa = DFA(&mergedNFA);

    Tlog::Branch b(logger, "DFA-build.log");
    // All state-local accept bindings have now been rebased.
    // Reconstruct the global accept map from them.
    logger.log(
        "Merged NFA action table: {}",
        mergedNFA.getActionTable().size()
    );

    for (std::size_t i = 0;
         i < mergedNFA.getActionTable().size();
         ++i) {

        const auto &action =
            mergedNFA.getActionTable()[i];

        logger.log(
            "  action[{}]: {} {}",
            i,
            static_cast<int>(action.action),
            action.variable.name
        );
         }

    dfa.build();
    dfa.minimize();
    return std::make_tuple(dfa.classify(), dfa.getLR(), dfa.getSemantic(), max_registers_count);
}