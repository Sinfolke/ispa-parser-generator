module DFA.functionality;

import DFA.API;
import DFA.Base;
import DFA;
import corelib;
import logging;
import hash;
import std;

auto DFA::mergeTwoNFA(
    NFA &first,
    NFA &second
) -> void {
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
     * Find a priority range that is completely outside the priorities
     * already used by `first`.
     *
     * Priorities are local to an NFA before merging. After merging,
     * DFA construction may compare priorities coming from different
     * source NFAs, so their ranges must not overlap.
     * ------------------------------------------------------------
     */
    std::size_t priority_base = 0;

    for (const auto &state : first_states) {
        for (const auto &[symbol, targets] : state.transitions) {
            for (const auto &target : targets) {
                if (target.priority != NFA::NULL_STATE) {
                    priority_base =
                        std::max(priority_base, target.priority + 1);
                }
            }
        }

        for (const auto &target : state.epsilon_transitions) {
            if (target.priority != NFA::NULL_STATE) {
                priority_base =
                    std::max(priority_base, target.priority + 1);
            }
        }
    }

    /*
     * Reserve one priority range for all transitions belonging to
     * `second`.
     *
     * We deliberately preserve the relative ordering:
     *
     *     p0 < p1 < p2
     *
     * becomes:
     *
     *     base+p0 < base+p1 < base+p2
     *
     * so local NFA priority semantics are unchanged.
     */
    std::size_t second_priority_max = 0;

    for (const auto &state : second_states) {
        for (const auto &[symbol, targets] : state.transitions) {
            for (const auto &target : targets) {
                if (target.priority != NFA::NULL_STATE) {
                    second_priority_max =
                        std::max(second_priority_max, target.priority);
                }
            }
        }

        for (const auto &target : state.epsilon_transitions) {
            if (target.priority != NFA::NULL_STATE) {
                second_priority_max =
                    std::max(second_priority_max, target.priority);
            }
        }
    }

    const std::size_t second_priority_range =
        second_priority_max == NFA::NULL_STATE
            ? 1
            : second_priority_max + 1;

    /*
     * ------------------------------------------------------------
     * Copy states with rebasing.
     *
     * State IDs are rebased independently from priorities.
     * ------------------------------------------------------------
     */
    for (const auto &state : second_states) {
        auto new_state = state;

        /*
         * Symbol transitions:
         *
         *     state IDs -> +state_offset
         *     priorities -> +priority_base
         */
        for (auto &[symbol, target_ids] : new_state.transitions) {
            for (auto &target : target_ids) {
                if (target.next != NFA::NULL_STATE) {
                    target.next += state_offset;
                }

                if (target.priority != NFA::NULL_STATE) {
                    target.priority += priority_base;
                }
            }
        }

        /*
         * Epsilon transitions:
         *
         *     state IDs -> +state_offset
         *     priorities -> +priority_base
         */
        utype::unordered_set<NFA::TransitionValue> rebased_epsilon;

        for (auto target : new_state.epsilon_transitions) {
            if (target.next != NFA::NULL_STATE) {
                target.next += state_offset;
            }

            if (target.priority != NFA::NULL_STATE) {
                target.priority += priority_base;
            }

            rebased_epsilon.insert(std::move(target));
        }

        new_state.epsilon_transitions =
            std::move(rebased_epsilon);

        /*
         * Accept binding semantic state IDs are state IDs in the NFA,
         * so they need state rebasing, but NOT priority rebasing.
         */
        if (new_state.accept_binding.has_value()) {
            auto &binding = *new_state.accept_binding;

            if (binding.target_semantic_state.has_value() &&
                *binding.target_semantic_state != NFA::NULL_STATE) {
                *binding.target_semantic_state += state_offset;
            }
        }

        first_states.emplace_back(std::move(new_state));
    }

    /*
     * ------------------------------------------------------------
     * Attach the second NFA to the merged root.
     *
     * The root transition gets a priority AFTER the entire second-NFA
     * range. This keeps root dispatch deterministic.
     * ------------------------------------------------------------
     */
    const std::size_t root_priority =
        priority_base + second_priority_range;

    first_states[0].epsilon_transitions.insert(
        NFA::TransitionValue{
            .next = state_offset,
            .priority = root_priority
        }
    );
}

auto DFA::mergeNFAS(
    const stdu::vector<NFA> &nfas
) -> std::pair<NFA, std::size_t> {
    if (nfas.empty())
    throw Error("Cannot merge an empty collection of NFAs.");

    if (nfas.size() == 1)
        return {nfas[0], nfas[0].getRegistersCount()};

    /*
     * Build every NFA first. This makes their local priority spaces
     * explicit before we start assigning global priority ranges.
     */
    stdu::vector<NFA> built_nfas;
    built_nfas.reserve(nfas.size());

    for (const auto &source : nfas) {
        NFA copy = source;

        if (copy.getStates().empty())
            copy.build(true);

        built_nfas.push_back(std::move(copy));
    }

    /*
     * Start from an entirely synthetic NFA.
     *
     * This avoids inheriting state 0, actions, bindings or priorities
     * from any particular lexer rule.
     */
    NFA merged = built_nfas[0];

    /*
     * ------------------------------------------------------------
     * Create a genuinely synthetic root.
     * ------------------------------------------------------------
     */
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

            utype::unordered_set<NFA::TransitionValue>
                rebased_epsilon;

            for (auto target : state.epsilon_transitions) {
                if (target.next != NFA::NULL_STATE)
                    ++target.next;

                rebased_epsilon.insert(std::move(target));
            }

            state.epsilon_transitions =
                std::move(rebased_epsilon);

            if (state.accept_binding.has_value()) {
                auto &binding = *state.accept_binding;

                if (binding.target_semantic_state.has_value() &&
                    *binding.target_semantic_state != NFA::NULL_STATE) {
                    ++*binding.target_semantic_state;
                }
            }
        }

        states[0].epsilon_transitions.insert(
            NFA::TransitionValue{
                .next = 1,
                .priority = 0
            }
        );
    }

    std::size_t max_registers_count =
        built_nfas[0].getRegistersCount();

    /*
     * ------------------------------------------------------------
     * Merge remaining NFAs.
     *
     * mergeTwoNFA() gives every incoming NFA a disjoint priority
     * range, so priorities become globally comparable.
     * ------------------------------------------------------------
     */
    for (std::size_t i = 1; i < built_nfas.size(); ++i) {
        mergeTwoNFA(
            merged,
            built_nfas[i]
        );

        max_registers_count =
            std::max(
                max_registers_count,
                built_nfas[i].getRegistersCount()
            );
    }

    merged.buildAcceptMap();

    return {
        std::move(merged),
        max_registers_count
    };

}

auto DFA::buildTokenDFA(const AST::Tree &ast, const NFA &nfa) -> DFA {
    auto mutable_nfa = nfa;
    DFA dfa(&mutable_nfa);
    dfa.build();
    dfa.minimize();
    return dfa;
}
auto DFA::build(const AST::Tree &ast, NFA &nfa) -> std::tuple<ClassifiedDFA, stdu::vector<NFA::ActionState>, stdu::vector<NFA::SemanticState>, std::size_t> {
    auto dfa = DFA(&nfa);
    dfa.build();
    dfa.minimize();
    std::cout << "DFA contains " << dfa.get().size() << " states, " << dfa.getLR().size() << " LR items, and " << dfa.getSemantic().size() << " semantic states." << std::endl;
    return std::make_tuple(dfa.classify(), dfa.getLR(), dfa.getSemantic(), nfa.getRegistersCount());
}
auto DFA::build(const AST::Tree &ast, const stdu::vector<NFA> &nfa_collection) -> std::tuple<ClassifiedDFA, stdu::vector<NFA::ActionState>, stdu::vector<NFA::SemanticState>, std::size_t> {
    auto [mergedNFA, max_registers_count] = mergeNFAS(nfa_collection);
    std::cout << "Merged NFA contains " << mergedNFA.getStates().size() << " states and " << max_registers_count << " registers." << std::endl;
    auto dfa = DFA(&mergedNFA);

    dfa.build();
    dfa.minimize();
    std::cout << "DFA contains " << dfa.get().size() << " states, " << dfa.getLR().size() << " LR items, and " << dfa.getSemantic().size() << " semantic states." << std::endl;
    return std::make_tuple(dfa.classify(), dfa.getLR(), dfa.getSemantic(), max_registers_count);
}