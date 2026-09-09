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
    NFA &second
) {
    if (second.getStates().empty()) {
        second.build(true);
    }

    if (second.getStates().empty()) {
        return;
    }

    const std::size_t state_offset = first.getStates().size();

    auto &first_states = first.getStates();
    const auto &second_states = second.getStates();

    /*
     * ------------------------------------------------------------
     * Copy states with rebasing (Transitions, Actions & Bindings)
     * ------------------------------------------------------------
     */
    for (const auto &state : second_states) {
        auto new_state = state;

        // Symbol transitions: Rebase target state IDs
        for (auto &[symbol, target_ids] : new_state.transitions) {
            for (auto &target : target_ids) {
                if (target.next != NFA::NULL_STATE) {
                    target.next += state_offset;
                }
            }
        }

        // Epsilon transitions: Rebase target state IDs
        utype::unordered_set<NFA::TransitionValue> rebased_epsilon;
        for (auto target : new_state.epsilon_transitions) {
            if (target.next != NFA::NULL_STATE) {
                target.next += state_offset;
            }
            rebased_epsilon.insert(target);
        }
        new_state.epsilon_transitions = std::move(rebased_epsilon);

        // Actions: Rebase any embedded NFA state target references
        for (auto &act_var : new_state.actions) {
            std::visit([&](auto &act) {
                using T = std::decay_t<decltype(act)>;
                if constexpr (std::is_same_v<T, NFA::ActionState>) {
                    if (act.next_nfa_state != NFA::NULL_STATE) {
                        act.next_nfa_state += state_offset;
                    }
                    if (std::holds_alternative<NFA::DFATarget>(act.next_state)) {
                        auto &target = std::get<NFA::DFATarget>(act.next_state);
                        if (target.id != NFA::NULL_STATE) {
                            target.id += state_offset;
                        }
                    }
                } else if constexpr (std::is_same_v<T, NFA::SemanticState>) {
                    if (act.nfa_index != NFA::NULL_STATE) {
                        act.nfa_index += state_offset;
                    }
                    if (std::holds_alternative<NFA::DFATarget>(act.next_state)) {
                        auto &target = std::get<NFA::DFATarget>(act.next_state);
                        if (target.id != NFA::NULL_STATE) {
                            target.id += state_offset;
                        }
                    }
                }
            }, act_var);
        }

        // Accept binding target state rebase
        if (new_state.accept_binding.has_value()) {
            auto &binding = *new_state.accept_binding;
            if (binding.target_semantic_state.has_value() && *binding.target_semantic_state != NFA::NULL_STATE) {
                *binding.target_semantic_state += state_offset;
            }
        }

        first_states.emplace_back(std::move(new_state));
    }

    /*
     * ------------------------------------------------------------
     * Attach the second NFA to the synthetic root (State 0).
     * ------------------------------------------------------------
     */
    first_states[0].epsilon_transitions.insert(NFA::TransitionValue{ .next = state_offset });
}

auto DFA::mergeNFAS(
    const stdu::vector<NFA> &nfas
) -> std::pair<NFA, std::size_t> {
    NFA merged = nfas[0];

    if (merged.getStates().empty()) {
        merged.build(true);
    }

    // ------------------------------------------------------------
    // The merged NFA needs a genuinely synthetic start state.
    //
    // The old implementation used NFA #0's state 0 as the common root
    // and added epsilon edges from it to every later NFA.  That is wrong
    // when state 0 owns actions: those actions become a shared prefix of
    // every terminal in the merged automaton.  In a multi-terminal lexer
    // this makes the first terminal's BEGIN/PUSH/etc. leak into all other
    // terminals and corrupts capture boundaries.
    //
    // Make state 0 a pure dispatcher instead:
    //
    //       synthetic root
    //          /  |  \
    //         v   v   v
    //        NFA0 NFA1 NFA2
    //
    // Rebase the original NFA #0 by one state so all of its embedded NFA
    // references remain valid.
    // ------------------------------------------------------------
    {
        auto &states = merged.getStates();

        states.insert(states.begin(), NFA::state{});

        for (std::size_t old_id = states.size(); old_id-- > 1;) {
            auto &state = states[old_id];

            for (auto &[symbol, targets] : state.transitions) {
                for (auto &target : targets) {
                    if (target.next != NFA::NULL_STATE)
                        ++target.next;
                }
            }

            utype::unordered_set<NFA::TransitionValue> rebased_epsilon;
            for (auto target : state.epsilon_transitions) {
                if (target.next != NFA::NULL_STATE)
                    ++target.next;
                rebased_epsilon.insert(target);
            }
            state.epsilon_transitions =
                std::move(rebased_epsilon);

            for (auto &act_var : state.actions) {
                std::visit([&](auto &act) {
                    using T = std::decay_t<decltype(act)>;

                    if constexpr (std::is_same_v<T, NFA::ActionState>) {
                        if (act.next_nfa_state != NFA::NULL_STATE)
                            ++act.next_nfa_state;

                        if (std::holds_alternative<NFA::DFATarget>(
                                act.next_state)) {
                            auto &target =
                                std::get<NFA::DFATarget>(act.next_state);

                            if (target.id != NFA::NULL_STATE)
                                ++target.id;
                        }
                    }
                    else if constexpr (
                        std::is_same_v<T, NFA::SemanticState>) {
                        if (act.nfa_index != NFA::NULL_STATE)
                            ++act.nfa_index;

                        if (std::holds_alternative<NFA::DFATarget>(
                                act.next_state)) {
                            auto &target =
                                std::get<NFA::DFATarget>(act.next_state);

                            if (target.id != NFA::NULL_STATE)
                                ++target.id;
                        }
                    }
                }, act_var);
            }

            if (state.accept_binding.has_value()) {
                auto &binding = *state.accept_binding;

                if (binding.target_semantic_state.has_value() &&
                    *binding.target_semantic_state != NFA::NULL_STATE) {
                    ++*binding.target_semantic_state;
                }
            }
        }

        // The original NFA #0 starts at old state 0, now state 1.
        states[0].epsilon_transitions.insert(
            NFA::TransitionValue{.next = 1}
        );
    }

    std::size_t max_registers_count = merged.getRegistersCount();
    for (std::size_t i = 1; i < nfas.size(); ++i) {
        NFA next = nfas[i];
        if (next.getStates().empty()) {
            next.build(true);
        }

        mergeTwoNFA(
            merged,
            next
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
    std::cout << "Merged NFA contains " << mergedNFA.getStates().size() << " states and " << max_registers_count << " registers." << std::endl;
    auto dfa = DFA(&mergedNFA);

    Tlog::Branch b(logger, "DFA-build.log");

    for (std::size_t i = 0; i < mergedNFA.getStates().size(); ++i) {
        const auto &st = mergedNFA.getStates()[i];
        if (!st.actions.empty()) {
            logger.log("  state[{}]: {} state-hosted action(s)", i, st.actions.size());
        }
    }

    dfa.build();
    dfa.minimize();
    std::cout << "DFA contains " << dfa.get().size() << " states, " << dfa.getLR().size() << " LR items, and " << dfa.getSemantic().size() << " semantic states." << std::endl;
    return std::make_tuple(dfa.classify(), dfa.getLR(), dfa.getSemantic(), max_registers_count);
}