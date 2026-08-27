module DFA;

import DFA.States;
import DFA.closure;
import hash;
import logging;
import corelib;
import cpuf.op;
import dstd;
import std;

namespace DFA {

    auto DFA::build() -> const States<SingleState>& {
        Tlog::Branch b(logger, "DFA.log");

        utype::unordered_map<Closure, std::size_t> dfa_state_map;
        utype::unordered_map<
            std::pair<Closure, NFA::TransitionKey>,
            Closure
        > closure_cache;

        std::queue<Closure> work;

        utype::unordered_map<
            std::size_t,
            std::vector<std::size_t>
        > nfa_to_dfa;

        std::vector<Closure> dfa_closures;

        states.clear();
        lr_table.clear();
        semantic_table.clear();

        using NextTarget = std::variant<NFA::DFATarget, NFA::ActionTarget, NFA::SemanticTarget>;

        std::map<std::pair<std::size_t, NextTarget>, std::size_t> lr_chain_cache;
        utype::unordered_map<std::pair<std::size_t, std::size_t>, std::size_t> transition_semantic_cache;
        utype::unordered_map<std::pair<std::size_t, std::size_t>, std::size_t> accept_semantic_cache;

        // ================================================================
        // 1. Start subset construction
        // ================================================================

        StateSet start_set = {0};
        Closure start_closure(nfa, start_set);

        const std::size_t start_idx = states.makeNew();

        dfa_state_map.emplace(start_closure, start_idx);
        dfa_closures.push_back(start_closure);
        work.push(start_closure);

        // Helper to check if an action is a terminal/accept-time action
        auto is_terminal_action = [&](std::size_t act_idx) -> bool {
            if (act_idx >= nfa.getActionTable().size()) return false;
            auto act_enum = nfa.getActionTable()[act_idx].action;
            return act_enum == NFA::Action::END || act_enum == NFA::Action::PUSH;
        };

        // ================================================================
        // 1b. Capture entry actions: Action/Semantic epsilon edges reachable
        //     from NFA state 0 with zero characters consumed. The per-symbol
        //     loop below only ever harvests fired actions belonging to a
        //     Closure built by consuming a character (target_closure);
        //     start_closure's own firedActions() were never read anywhere,
        //     so any action reachable purely by the epsilon-closure of the
        //     start state itself was silently dropped from lr_table/
        //     semantic_table entirely -- not misrouted, not stale, just
        //     never created.
        // ================================================================
        {
            std::vector<std::size_t> entry_action_indices;
            std::vector<std::size_t> entry_semantic_indices;

            for (const auto &action : start_closure.firedActions()) {
                if (action.table_type == NFA::TableType::Action) {
                    entry_action_indices.push_back(action.table_index);
                } else if (action.table_type == NFA::TableType::Semantic) {
                    entry_semantic_indices.push_back(action.table_index);
                }
            }

            auto deduplicate_entry = [](std::vector<std::size_t> &indices) {
                std::vector<std::size_t> result;
                result.reserve(indices.size());
                for (const auto index : indices) {
                    if (std::find(result.begin(), result.end(), index) == result.end()) {
                        result.push_back(index);
                    }
                }
                indices = std::move(result);
            };

            deduplicate_entry(entry_action_indices);
            deduplicate_entry(entry_semantic_indices);

            if (!entry_action_indices.empty() || !entry_semantic_indices.empty()) {
                // After the entry chain fires, control falls back to state 0
                // itself to consume the actual first input character
                // normally -- entry_action is a one-time prefix, not a
                // replacement for state 0's own transitions.
                NextTarget current_next = NFA::DFATarget{ start_idx };

                for (const auto &nfa_semantic_idx : entry_semantic_indices) {
                    if (nfa_semantic_idx >= nfa.getSemanticTable().size()) {
                        throw Error("Invalid NFA semantic table index {}", nfa_semantic_idx);
                    }
                    auto semantic_entry = nfa.getSemanticTable().at(nfa_semantic_idx);
                    semantic_entry.next_state = current_next;
                    semantic_entry.nfa_index = nfa_semantic_idx;

                    const std::size_t new_sem_idx = semantic_table.size();
                    semantic_table.push_back(std::move(semantic_entry));
                    current_next = NFA::SemanticTarget{ .id = new_sem_idx };
                }

                for (auto it = entry_action_indices.rbegin(); it != entry_action_indices.rend(); ++it) {
                    const std::size_t nfa_action_idx = *it;
                    auto action_entry = nfa.getActionTable().at(nfa_action_idx);
                    action_entry.next_state = current_next;

                    const std::size_t lr_idx = lr_table.size();
                    lr_table.push_back(std::move(action_entry));
                    current_next = NFA::ActionTarget{ .id = lr_idx };
                }

                states[start_idx].entry_action = current_next;
            }
        }

        // ================================================================
        // 2. Subset construction
        // ================================================================
        const Closure *current_cache;
        while (!work.empty()) {
            Closure current = work.front();
            current_cache = &current;
            work.pop();

            const std::size_t current_dfa_index = dfa_state_map.at(current);

            std::optional<NFA::TokenBinding> best_binding;

            for (const std::size_t nfa_index : current) {
                nfa_to_dfa[nfa_index].push_back(current_dfa_index);

                const auto &nfa_state = nfa.getStates().at(nfa_index);
                auto accept_it = nfa.getAcceptMap().find(nfa_index);

                std::optional<NFA::TokenBinding> binding =
                    accept_it != nfa.getAcceptMap().end()
                        ? std::make_optional(accept_it->second)
                        : nfa_state.accept_binding;

                if (!binding.has_value())
                    continue;

                bool current_has_semantic = binding->target_semantic_state.has_value();
                bool best_has_semantic = best_binding.has_value() && best_binding->target_semantic_state.has_value();

                if (!best_binding.has_value() ||
                    (current_has_semantic && !best_has_semantic) ||
                    (current_has_semantic == best_has_semantic && binding->token_id < best_binding->token_id)) {
                    best_binding = binding;
                }

            }

            states[current_dfa_index].accept_binding = best_binding;

            struct TransitionInfo {
                std::size_t source_nfa_state;
                NFA::TableType table_type;
                std::size_t table_index;
                Closure closure;
            };

            std::vector<std::pair<NFA::TransitionKey, TransitionInfo>> transitions;

            for (const std::size_t nfa_index : current) {
                const auto &nfa_state = nfa.getStates().at(nfa_index);

                for (const auto &[symbol, ids] : nfa_state.transitions) {
                    const auto cache_key = std::make_pair(current, symbol);

                    if (!closure_cache.contains(cache_key)) {
                        closure_cache.emplace(
                            cache_key,
                            Closure(nfa, current.get(), symbol)
                        );
                    }

                    const Closure &closure_set = closure_cache.at(cache_key);
                    if (closure_set.empty())
                        continue;

                    for (const auto &transition : ids) {
                        switch (transition.table_type) {
                            case NFA::TableType::DFA:
                                if (transition.next != NFA::NULL_STATE) {
                                    transitions.emplace_back(symbol, TransitionInfo{
                                        .source_nfa_state = nfa_index,
                                        .table_type = NFA::TableType::DFA,
                                        .table_index = transition.next,
                                        .closure = closure_set
                                    });
                                }
                                break;
                            case NFA::TableType::Action:
                                transitions.emplace_back(symbol, TransitionInfo{
                                    .source_nfa_state = nfa_index,
                                    .table_type = NFA::TableType::Action,
                                    .table_index = transition.next,
                                    .closure = closure_set
                                });
                                break;
                            case NFA::TableType::Semantic:
                                transitions.emplace_back(symbol, TransitionInfo{
                                    .source_nfa_state = nfa_index,
                                    .table_type = NFA::TableType::Semantic,
                                    .table_index = transition.next,
                                    .closure = closure_set
                                });
                                break;
                        }
                    }
                }
            }
            std::map<NFA::TransitionKey, std::vector<TransitionInfo>> grouped_transitions;
            for (auto &&[sym, info] : transitions) {
                grouped_transitions[sym].push_back(std::move(info));
            }

            for (const auto &[symbol, info_list] : grouped_transitions) {
                stdu::vector<std::size_t> merged_nfa_states;
                const auto cache_key = std::make_pair(current, symbol);
                const Closure &target_closure = closure_cache.at(cache_key);
                for (const auto &info : info_list) {
                    for (const std::size_t nfa_st : info.closure) {
                        merged_nfa_states.push_back(nfa_st);
                    }
                }

                std::sort(merged_nfa_states.begin(), merged_nfa_states.end());
                merged_nfa_states.erase(std::unique(merged_nfa_states.begin(), merged_nfa_states.end()), merged_nfa_states.end());

                Closure closure_set(nfa, merged_nfa_states);
                std::size_t target_index;

                auto state_it = dfa_state_map.find(closure_set);
                if (state_it == dfa_state_map.end()) {
                    target_index = states.makeNew();
                    dfa_state_map.emplace(closure_set, target_index);
                    dfa_closures.push_back(closure_set);
                    work.push(closure_set);
                } else {
                    target_index = state_it->second;
                }

                auto &dfa_transition = states[current_dfa_index].transitions[symbol];
                std::vector<std::size_t> action_indices;
                std::vector<std::size_t> semantic_indices;

                // 1. Identify target NFA states in this closure that can consume further characters
                std::unordered_set<std::size_t> char_target_states;
                for (const std::size_t nfa_st : target_closure) {
                    if (nfa_st < nfa.getStates().size() && !nfa.getStates().at(nfa_st).transitions.empty()) {
                        char_target_states.insert(nfa_st);
                    }
                }

                // 1. Identify source NFA states that trigger this specific shift
                std::unordered_set<std::size_t> target_sources;
                for (const auto &info : info_list) {
                    target_sources.insert(info.source_nfa_state);
                }

                // Helper to check if an action leads to any of our target sources
                auto can_reach_target_source = [&](std::size_t action_idx) -> bool {
                    std::unordered_set<std::size_t> visited_act;
                    std::vector<NextTarget> work_q;

                    if (action_idx < nfa.getActionTable().size()) {
                        work_q.push_back(nfa.getActionTable().at(action_idx).next_state);
                    }

                    while (!work_q.empty()) {
                        NextTarget curr = work_q.back();
                        work_q.pop_back();

                        if (std::holds_alternative<NFA::DFATarget>(curr)) {
                            std::size_t nfa_id = std::get<NFA::DFATarget>(curr).id;
                            if (nfa_id == NFA::NULL_STATE) continue;
                            if (target_sources.contains(nfa_id)) return true;

                            // Continue exploring epsilon closures from this NFA state
                            if (nfa_id < nfa.getStates().size()) {
                                for (const auto &edge : nfa.getStates().at(nfa_id).epsilon_transitions) {
                                    if (edge.table_type == NFA::TableType::DFA) {
                                        work_q.push_back(NFA::DFATarget{edge.next});
                                    } else if (edge.table_type == NFA::TableType::Action) {
                                        work_q.push_back(NFA::ActionTarget{edge.next});
                                    } else if (edge.table_type == NFA::TableType::Semantic) {
                                        work_q.push_back(NFA::SemanticTarget{edge.next});
                                    }
                                }
                            }
                        } else if (std::holds_alternative<NFA::ActionTarget>(curr)) {
                            std::size_t act_id = std::get<NFA::ActionTarget>(curr).id;
                            if (act_id < nfa.getActionTable().size() && visited_act.insert(act_id).second) {
                                work_q.push_back(nfa.getActionTable().at(act_id).next_state);
                            }
                        } else if (std::holds_alternative<NFA::SemanticTarget>(curr)) {
                            std::size_t sem_id = std::get<NFA::SemanticTarget>(curr).id;
                            if (sem_id < nfa.getSemanticTable().size() && visited_act.insert(1000000 + sem_id).second) {
                                work_q.push_back(nfa.getSemanticTable().at(sem_id).next_state);
                            }
                        }
                    }
                    return false;
                };

                // All actions reported by the source closure are runtime actions,
                // including END/PUSH.  Do not special-case terminal actions here: the
                // closure is the authoritative record of which epsilon actions fire
                // after this character has been consumed.

                // Walk a chain starting at a discovered root, following each entry's own
                // next_state link (NOT info_list) until it stops being Action/Semantic.
                auto walk_chain = [&](std::size_t start_index, NFA::TableType start_type) {
                    std::size_t idx = start_index;
                    NFA::TableType type = start_type;

                    while (true) {
                        if (type == NFA::TableType::Action) {
                            // Terminal actions are real runtime operations.
                            // They must remain in the transition chain; END/PUSH
                            // may finalize a nested value while the DFA continues
                            // matching the enclosing construct.
                            action_indices.push_back(idx);
                            const auto &entry = nfa.getActionTable().at(idx);
                            if (std::holds_alternative<NFA::ActionTarget>(entry.next_state)) {
                                idx = std::get<NFA::ActionTarget>(entry.next_state).id;
                                type = NFA::TableType::Action;
                                continue;
                            }
                            if (std::holds_alternative<NFA::SemanticTarget>(entry.next_state)) {
                                idx = std::get<NFA::SemanticTarget>(entry.next_state).id;
                                type = NFA::TableType::Semantic;
                                continue;
                            }
                            break;
                        }
                        if (type == NFA::TableType::Semantic) {
                            semantic_indices.push_back(idx);
                            break;
                        }
                        break;
                    }
                };

                for (const auto &info : info_list) {
                    if (info.table_type == NFA::TableType::Action || info.table_type == NFA::TableType::Semantic) {
                        walk_chain(info.table_index, info.table_type);
                    }
                }
                // Extract all actions directly captured during epsilon-closure.
                // END/PUSH are deliberately retained: they are executable runtime
                // actions, not merely accept metadata.
                for (const auto &action : target_closure.firedActions()) {
                    if (action.table_type == NFA::TableType::Action) {
                        action_indices.push_back(action.table_index);
                    } else if (action.table_type == NFA::TableType::Semantic) {
                        semantic_indices.push_back(action.table_index);
                    }
                }
                auto deduplicate = [](std::vector<std::size_t> &indices) {
                    std::vector<std::size_t> result;
                    result.reserve(indices.size());

                    for (const auto index : indices) {
                        if (std::find(result.begin(), result.end(), index) == result.end()) {
                            result.push_back(index);
                        }
                    }

                    indices = std::move(result);
                };

                deduplicate(action_indices);
                deduplicate(semantic_indices);

                // 1. Start with base DFA shift target
                NextTarget current_next = NFA::DFATarget{ target_index };

                // 2. Wrap into Semantic Target if semantic actions exist on transition
                for (const auto &nfa_semantic_idx : semantic_indices) {
                    if (nfa_semantic_idx >= nfa.getSemanticTable().size()) {
                        throw Error("Invalid NFA semantic table index {}", nfa_semantic_idx);
                    }

                    const auto sem_cache_key = std::make_pair(nfa_semantic_idx, target_index);
                    auto sem_cache_it = transition_semantic_cache.find(sem_cache_key);

                    std::size_t new_sem_idx;
                    if (sem_cache_it != transition_semantic_cache.end()) {
                        new_sem_idx = sem_cache_it->second;
                    } else {
                        auto semantic_entry = nfa.getSemanticTable().at(nfa_semantic_idx);
                        semantic_entry.next_state = current_next;
                        semantic_entry.nfa_index = nfa_semantic_idx;

                        new_sem_idx = semantic_table.size();
                        semantic_table.push_back(std::move(semantic_entry));
                        transition_semantic_cache.emplace(sem_cache_key, new_sem_idx);
                    }

                    current_next = NFA::SemanticTarget{ .id = new_sem_idx };
                }

                // 3. Wrap whole target chain inside action chain (if present)

                for (auto it = action_indices.rbegin(); it != action_indices.rend(); ++it) {
                    const std::size_t nfa_action_idx = *it;
                    const auto cache_key = std::make_pair(nfa_action_idx, current_next);

                    auto cache_it = lr_chain_cache.find(cache_key);
                    std::size_t lr_idx;

                    if (cache_it != lr_chain_cache.end()) {
                        lr_idx = cache_it->second;
                    } else {
                        auto action_entry = nfa.getActionTable().at(nfa_action_idx);
                        action_entry.next_state = current_next;

                        lr_idx = lr_table.size();
                        lr_table.push_back(std::move(action_entry));
                        lr_chain_cache.emplace(cache_key, lr_idx);
                    }

                    current_next = NFA::ActionTarget{ .id = lr_idx };
                }

                dfa_transition = current_next;
            }
        }
        // ================================================================
        // 3. Resolve accept/reduce semantic states safely
        // ================================================================

        for (std::size_t dfa_index = 0; dfa_index < states.size(); ++dfa_index) {
            auto &binding = states[dfa_index].accept_binding;

            if (!binding.has_value() || !binding->target_semantic_state.has_value())
                continue;
            // 1. Resolve and cache semantic state index
            const std::size_t nfa_sem_idx = *binding->target_semantic_state;
            if (nfa_sem_idx >= nfa.getSemanticTable().size()) {
                throw Error("DFA state {} references invalid semantic state {}", dfa_index, nfa_sem_idx);
            }

            // Materialize an NFA target chain into DFA table space.  This is
            // deliberately recursive because Semantic -> Semantic and
            // Action -> Semantic chains are legal.  A placeholder is installed
            // before descending, so cycles cannot recurse forever and no NFA
            // index is ever stored in a DFA table.
            std::unordered_map<std::size_t, std::size_t> local_action_cache;
            std::function<NextTarget(const NextTarget&)> materialize_target;
            std::function<NFA::SemanticTarget(std::size_t)> materialize_semantic;
            std::function<NFA::ActionTarget(std::size_t)> materialize_action;

            auto resolve_nfa_state = [&](std::size_t nfa_state) -> std::size_t {
                if (nfa_state == NFA::NULL_STATE)
                    return NFA::NULL_STATE;

                // The normal case is that the continuation is in the closure
                // of the DFA state we're currently resolving (dfa_index).
                // NOTE: this must be dfa_closures[dfa_index], NOT
                // current_cache -- current_cache pointed at a loop-body-local
                // Closure from the main subset-construction loop above,
                // which is out of scope and dangling by the time section 3
                // runs (it's a separate loop, after that one has fully
                // finished). Dereferencing it here was undefined behavior:
                // it could spuriously match against whatever garbage
                // happened to occupy that memory, silently resolving to the
                // wrong dfa_index and baking a corrupted target into the
                // materialized Action/Semantic chain.
                if (dfa_index < dfa_closures.size() &&
                    std::find(dfa_closures[dfa_index].begin(), dfa_closures[dfa_index].end(), nfa_state) != dfa_closures[dfa_index].end())
                    return dfa_index;

                auto mapped = nfa_to_dfa.find(nfa_state);
                if (mapped != nfa_to_dfa.end() && !mapped->second.empty())
                    return mapped->second.front();

                for (std::size_t i = 0; i < dfa_closures.size(); ++i) {
                    if (std::find(dfa_closures[i].begin(), dfa_closures[i].end(), nfa_state) != dfa_closures[i].end())
                        return i;
                }

                throw Error(
                    "Semantic state {} in DFA state {} targets NFA state {} "
                    "which has no corresponding DFA state",
                    nfa_sem_idx, dfa_index, nfa_state
                );
            };

            materialize_target = [&](const TransitionTarget &target) -> NextTarget {
                if (std::holds_alternative<NFA::DFATarget>(target)) {
                    return NFA::DFATarget{
                        resolve_nfa_state(std::get<NFA::DFATarget>(target).id)
                    };
                }
                if (std::holds_alternative<NFA::ActionTarget>(target)) {
                    return materialize_action(std::get<NFA::ActionTarget>(target).id);
                }
                return materialize_semantic(std::get<NFA::SemanticTarget>(target).id);
            };

            materialize_action = [&](std::size_t nfa_action_idx) -> NFA::ActionTarget {
                if (nfa_action_idx >= nfa.getActionTable().size()) {
                    throw Error("Invalid NFA action {} in semantic chain", nfa_action_idx);
                }

                if (auto it = local_action_cache.find(nfa_action_idx); it != local_action_cache.end())
                    return NFA::ActionTarget{it->second};

                const auto dfa_action_idx = lr_table.size();
                local_action_cache.emplace(nfa_action_idx, dfa_action_idx);

                auto action_entry = nfa.getActionTable().at(nfa_action_idx);
                const auto original_next = action_entry.next_state;
                // Reserve the DFA slot before descending so an Action -> Action
                // cycle resolves to the correct DFA index.
                action_entry.next_state = NFA::DFATarget{NFA::NULL_STATE};
                lr_table.push_back(std::move(action_entry));
                lr_table[dfa_action_idx].next_state = materialize_target(original_next);

                return NFA::ActionTarget{dfa_action_idx};
            };

            materialize_semantic = [&](std::size_t nfa_semantic_idx) -> NFA::SemanticTarget {
                if (nfa_semantic_idx >= nfa.getSemanticTable().size()) {
                    throw Error("Invalid NFA semantic state {} in semantic chain", nfa_semantic_idx);
                }

                if (auto it = accept_semantic_cache.find({dfa_index, nfa_semantic_idx});
                    it != accept_semantic_cache.end()) {
                    return NFA::SemanticTarget{it->second};
                }

                const auto dfa_semantic_idx = semantic_table.size();
                accept_semantic_cache.emplace(
                    std::make_pair(dfa_index, nfa_semantic_idx),
                    dfa_semantic_idx
                );

                auto semantic_entry = nfa.getSemanticTable().at(nfa_semantic_idx);
                semantic_entry.nfa_index = nfa_semantic_idx;
                const auto original_next = semantic_entry.next_state;
                // Reserve the DFA slot before descending so Semantic -> Semantic
                // cycles resolve to the correct DFA index.
                semantic_entry.next_state = NFA::DFATarget{NFA::NULL_STATE};
                semantic_table.push_back(std::move(semantic_entry));
                semantic_table[dfa_semantic_idx].next_state = materialize_target(original_next);

                return NFA::SemanticTarget{dfa_semantic_idx};
            };

            const auto dfa_semantic_target = materialize_semantic(nfa_sem_idx);
            const std::size_t dfa_sem_idx = dfa_semantic_target.id;

            // A semantic accept is itself the terminal target.  Its continuation
            // (including END/PUSH, if any) is already represented by the
            // materialized semantic/action chain above.  Do NOT manufacture a
            // second terminal action from reduce_rule_id or firedActions here:
            // doing so makes the same END execute twice when the current input
            // character is reprocessed by the continuation DFA state.
            binding->target_semantic_state = dfa_sem_idx;
            binding->reduce_rule_id.reset();
        }

        // ================================================================
        // 4. Validate
        // ================================================================

        if (states.empty())
            throw Error("DFA cannot be empty");

        // Validate all table references before returning.  This catches an NFA
        // index accidentally leaking into the DFA tables at the exact point it
        // is introduced, instead of much later in the lexer runtime.
        for (std::size_t i = 0; i < states.size(); ++i) {
            const auto &state = states[i];
            if (state.accept_binding && state.accept_binding->target_semantic_state) {
                Assert(*state.accept_binding->target_semantic_state < semantic_table.size(),
                       "DFA state {} has invalid semantic index {}",
                       i, *state.accept_binding->target_semantic_state);
            }
            if (state.entry_action) {
                std::visit([&](const auto &target) {
                    using T = std::decay_t<decltype(target)>;
                    if constexpr (std::is_same_v<T, NFA::ActionTarget>) {
                        Assert(target.id < lr_table.size(),
                               "DFA state {} has invalid entry action {}", i, target.id);
                    } else if constexpr (std::is_same_v<T, NFA::SemanticTarget>) {
                        Assert(target.id < semantic_table.size(),
                               "DFA state {} has invalid entry semantic {}", i, target.id);
                    }
                }, *state.entry_action);
            }
        }

        return states;
    }

    void DFA::optimizeRegistersAndLRTable() {
        std::vector<bool> used(lr_table.size(), false);

        auto mark_action = [&](auto self, std::size_t idx) -> void {
            if (idx >= lr_table.size() || used[idx]) return;
            used[idx] = true;
            if (std::holds_alternative<NFA::ActionTarget>(lr_table[idx].next_state)) {
                self(self, std::get<NFA::ActionTarget>(lr_table[idx].next_state).id);
            }
        };

        for (const auto &state : states) {
            // FIX: Prevent entry actions from being garbage collected
            if (state.entry_action.has_value() && std::holds_alternative<NFA::ActionTarget>(*state.entry_action)) {
                mark_action(mark_action, std::get<NFA::ActionTarget>(*state.entry_action).id);
            }

            // FIX: reduce_rule_id can reference the head of a materialized
            // action chain (section 3's END/PUSH -> ... -> Semantic
            // chaining) that's reachable ONLY through this field -- not
            // through any transition, entry_action, or semantic_table
            // next_state. Without marking it here it's silently compacted
            // away as "unused", leaving reduce_rule_id dangling.
            if (state.accept_binding && state.accept_binding->reduce_rule_id.has_value()) {
                mark_action(mark_action, *state.accept_binding->reduce_rule_id);
            }

            for (const auto &[symbol, target] : state.transitions) {
                if (std::holds_alternative<NFA::ActionTarget>(target)) {
                    mark_action(mark_action, std::get<NFA::ActionTarget>(target).id);
                }
            }
        }
        for (const auto &state : semantic_table) {
            if (std::holds_alternative<NFA::ActionTarget>(state.next_state)) {
                mark_action(mark_action, std::get<NFA::ActionTarget>(state.next_state).id);
            }
        }
        std::vector<NFA::ActionState> deduplicated_lr_table;
        std::unordered_map<std::size_t, std::size_t> lr_index_remap;

        bool merged_any = true;
        while (merged_any) {
            merged_any = false;
            deduplicated_lr_table.clear();
            lr_index_remap.clear();

            for (std::size_t i = 0; i < lr_table.size(); ++i) {
                if (!used[i]) continue;
                const auto &entry = lr_table[i];
                std::size_t canonical_idx = NFA::NULL_STATE;

                for (std::size_t j = 0; j < deduplicated_lr_table.size(); ++j) {
                    if (deduplicated_lr_table[j].action == entry.action &&
                        deduplicated_lr_table[j].variable == entry.variable &&
                        deduplicated_lr_table[j].next_state == entry.next_state) {
                        canonical_idx = j;
                        break;
                    }
                }

                if (canonical_idx == NFA::NULL_STATE) {
                    canonical_idx = deduplicated_lr_table.size();
                    deduplicated_lr_table.push_back(entry);
                } else {
                    merged_any = true;
                }
                lr_index_remap[i] = canonical_idx;
            }

            for (auto &entry : deduplicated_lr_table) {
                if (std::holds_alternative<NFA::ActionTarget>(entry.next_state)) {
                    auto &act = std::get<NFA::ActionTarget>(entry.next_state);
                    if (auto it = lr_index_remap.find(act.id); it != lr_index_remap.end()) {
                        act.id = it->second;
                    }
                }
            }

            lr_table = std::move(deduplicated_lr_table);
            used.assign(lr_table.size(), true);

            // CRITICAL: rewrite every external reference into lr_table at the
            // end of EVERY round, not just once after the loop exits.
            // lr_index_remap only maps *this round's* starting index space to
            // the freshly-compacted one. Since this round's lr_table is
            // exactly the previous round's compacted output, external refs
            // that were already advanced by the previous round's pass are in
            // exactly the right space for this round's map to apply cleanly --
            // that's what keeps them perpetually in sync. Applying only the
            // LAST round's map (as before) leaves refs that predate an
            // earlier round stuck in stale, pre-dedup index space: numbers
            // that made sense before compaction but point nowhere sane in the
            // final, much smaller table -- exactly how a runtime ends up
            // invoking an action index that no longer exists.
            for (auto &state : states) {
                // FIX: Update entry action IDs after compaction shifts them
                if (state.entry_action.has_value() && std::holds_alternative<NFA::ActionTarget>(*state.entry_action)) {
                    auto &act = std::get<NFA::ActionTarget>(*state.entry_action);
                    if (auto it = lr_index_remap.find(act.id); it != lr_index_remap.end()) {
                        act.id = it->second;
                    }
                }

                if (state.accept_binding && state.accept_binding->reduce_rule_id) {
                    auto &id = *state.accept_binding->reduce_rule_id;
                    if (auto it = lr_index_remap.find(id); it != lr_index_remap.end()) {
                        id = it->second;
                    }
                }

                for (auto &[symbol, target] : state.transitions) {
                    if (std::holds_alternative<NFA::ActionTarget>(target)) {
                        auto &act = std::get<NFA::ActionTarget>(target);
                        if (auto it = lr_index_remap.find(act.id); it != lr_index_remap.end()) {
                            act.id = it->second;
                        }
                    }
                }
            }
            for (auto &sem : semantic_table) {
                if (std::holds_alternative<NFA::ActionTarget>(sem.next_state)) {
                    auto &act = std::get<NFA::ActionTarget>(sem.next_state);
                    if (auto it = lr_index_remap.find(act.id); it != lr_index_remap.end()) {
                        act.id = it->second;
                    }
                }
            }
        }
    }

    void DFA::optimizeSemanticTable() {
        std::vector<bool> used(semantic_table.size(), false);

        auto mark_semantic = [&](std::size_t idx) {
            if (idx < used.size()) used[idx] = true;
        };

        auto resolve_target = [&](auto self, const TransitionTarget &target) -> void {
            if (std::holds_alternative<NFA::SemanticTarget>(target)) {
                std::size_t sem_id = std::get<NFA::SemanticTarget>(target).id;
                if (sem_id < semantic_table.size() && !used[sem_id]) {
                    used[sem_id] = true; // Mark as used
                    self(self, semantic_table[sem_id].next_state);
                }
            } else if (std::holds_alternative<NFA::ActionTarget>(target)) {
                std::size_t act_id = std::get<NFA::ActionTarget>(target).id;
                if (act_id < lr_table.size()) {
                    // RECURSE through action chains to reach downstream semantic states!
                    self(self, lr_table[act_id].next_state);
                }
            }
        };

        for (const auto &state : states) {
            if (state.accept_binding && state.accept_binding->target_semantic_state) {
                // Implicit conversion from NFA::SemanticTarget to NextTarget now works
                resolve_target(resolve_target, NFA::SemanticTarget{*state.accept_binding->target_semantic_state});
            }

            if (state.entry_action.has_value()) {
                resolve_target(resolve_target, *state.entry_action);
            }

            for (const auto &[symbol, target] : state.transitions) {
                resolve_target(resolve_target, target);
            }
        }

        for (const auto &state : lr_table) {
            resolve_target(resolve_target, state.next_state);
        }

        std::unordered_map<std::size_t, std::size_t> remap;
        std::vector<NFA::SemanticState> compacted;

        // 2. Compact and deduplicate used entries
        for (std::size_t i = 0; i < semantic_table.size(); ++i) {
            if (!used[i]) continue;
            const auto &entry = semantic_table[i];
            std::size_t canonical_idx = NFA::NULL_STATE;

            for (std::size_t j = 0; j < compacted.size(); ++j) {
                if (compacted[j].next_state == entry.next_state &&
                    compacted[j].statements == entry.statements &&
                    compacted[j].instance_value == entry.instance_value &&
                    compacted[j].nfa_index == entry.nfa_index)
                {
                    canonical_idx = j;
                    break;
                }
            }

            if (canonical_idx == NFA::NULL_STATE) {
                canonical_idx = compacted.size();
                compacted.push_back(entry);
            }
            remap[i] = canonical_idx;
        }

        // Helper to safely remap SemanticTarget IDs or clear them if dropped
        auto update_sem_target = [&](NFA::SemanticTarget &sem) {
            if (auto it = remap.find(sem.id); it != remap.end()) {
                sem.id = it->second;
            } else {
                // FIX 3: Reset unmapped target to NULL_STATE instead of leaving stale index
                sem.id = NFA::NULL_STATE;
            }
        };

        // 3. Remap all internal and external references
        for (auto &entry : compacted) {
            if (std::holds_alternative<NFA::SemanticTarget>(entry.next_state)) {
                update_sem_target(std::get<NFA::SemanticTarget>(entry.next_state));
            }
        }

        for (auto &act : lr_table) {
            if (std::holds_alternative<NFA::SemanticTarget>(act.next_state)) {
                update_sem_target(std::get<NFA::SemanticTarget>(act.next_state));
            }
        }

        for (auto &state : states) {
            if (state.accept_binding && state.accept_binding->target_semantic_state) {
                if (auto it = remap.find(*state.accept_binding->target_semantic_state); it != remap.end()) {
                    state.accept_binding->target_semantic_state = it->second;
                } else {
                    state.accept_binding->target_semantic_state.reset();
                }
            }

            if (state.entry_action.has_value() && std::holds_alternative<NFA::SemanticTarget>(*state.entry_action)) {
                update_sem_target(std::get<NFA::SemanticTarget>(*state.entry_action));
            }

            for (auto &[symbol, target] : state.transitions) {
                if (std::holds_alternative<NFA::SemanticTarget>(target)) {
                    update_sem_target(std::get<NFA::SemanticTarget>(target));
                }
            }
        }

        semantic_table = std::move(compacted);
    }

    auto DFA::sameAcceptBinding(
        const SingleState &a,
        const SingleState &b
    ) -> bool {
        if (a.accept_binding.has_value() != b.accept_binding.has_value())
            return false;

        if (!a.accept_binding)
            return true;

        const auto &lhs = *a.accept_binding;
        const auto &rhs = *b.accept_binding;

        return
            lhs.token_id == rhs.token_id &&
            lhs.is_unique_representation == rhs.is_unique_representation &&
            lhs.reduce_rule_id == rhs.reduce_rule_id &&
            lhs.target_semantic_state == rhs.target_semantic_state;
    }

    auto DFA::initialClass(const SingleState &s) -> std::size_t {
        std::size_t h = 0;

        if (s.accept_binding.has_value()) {
            const auto &binding = *s.accept_binding;

            hash_combine(h, binding.token_id);
            hash_combine(h, binding.is_unique_representation);

            if (binding.reduce_rule_id.has_value())
                hash_combine(h, *binding.reduce_rule_id);
            else
                hash_combine(h, NFA::NULL_STATE);

            if (binding.target_semantic_state.has_value())
                hash_combine(h, *binding.target_semantic_state);
            else
                hash_combine(h, NFA::NULL_STATE);
        } else {
            hash_combine(h, 0xDEADBEEF);
        }

        return h;
    }

    auto DFA::refinementKey(
        const SingleState &s,
        const std::unordered_map<std::size_t, std::size_t> &partition_of
    ) -> std::vector<TransitionKeyExt> {

        std::vector<TransitionKeyExt> key;
        key.reserve(s.transitions.size());

        for (const auto &[symbol, target] : s.transitions) {
            std::visit([&](auto &&arg) {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, NFA::DFATarget>) {
                    key.push_back(TransitionKeyExt{
                        .symbol = symbol,
                        .table_type = NFA::TableType::DFA,
                        .action = NFA::Action::UNDEF,
                        .action_id = NFA::NULL_STATE,
                        .target_partition = partition_of.at(arg.id)
                    });
                }
                else if constexpr (std::is_same_v<T, NFA::ActionTarget>) {
                    std::size_t curr_act = arg.id;
                    bool in_semantic = false;
                    std::size_t chain_hash = 0;
                    std::size_t target_part = NFA::NULL_STATE;

                    // curr_act can be reassigned mid-loop to an index into a
                    // DIFFERENT table (lr_table -> semantic_table or vice
                    // versa) once the chain crosses table types. Track which
                    // table is currently active and check bounds/index into
                    // THAT table -- otherwise, once the chain crosses into
                    // semantic_table, the loop keeps checking/indexing
                    // lr_table, silently reading an unrelated entry and
                    // corrupting the equivalence hash used for state merging.
                    while (true) {
                        if (!in_semantic) {
                            if (curr_act >= lr_table.size()) break;
                            const auto &act_entry = lr_table.at(curr_act);

                            hash_combine(chain_hash, uhash {} (act_entry.action));
                            hash_combine(chain_hash, uhash {} (act_entry.variable));

                            if (std::holds_alternative<NFA::ActionTarget>(act_entry.next_state)) {
                                curr_act = std::get<NFA::ActionTarget>(act_entry.next_state).id;
                            } else if (std::holds_alternative<NFA::SemanticTarget>(act_entry.next_state)) {
                                curr_act = std::get<NFA::SemanticTarget>(act_entry.next_state).id;
                                in_semantic = true;
                            } else if (std::holds_alternative<NFA::DFATarget>(act_entry.next_state)) {
                                std::size_t st = std::get<NFA::DFATarget>(act_entry.next_state).id;
                                target_part = (st != NFA::NULL_STATE) ? partition_of.at(st) : NFA::NULL_STATE;
                                break;
                            } else {
                                break;
                            }
                        } else {
                            if (curr_act >= semantic_table.size()) break;
                            const auto &sem_entry = semantic_table.at(curr_act);

                            hash_combine(chain_hash, uhash {} (sem_entry.next_state));
                            hash_combine(chain_hash, uhash {} (sem_entry.instance_value));
                            hash_combine(chain_hash, uhash {} (sem_entry.statements));

                            if (std::holds_alternative<NFA::ActionTarget>(sem_entry.next_state)) {
                                curr_act = std::get<NFA::ActionTarget>(sem_entry.next_state).id;
                                in_semantic = false;
                            } else if (std::holds_alternative<NFA::SemanticTarget>(sem_entry.next_state)) {
                                curr_act = std::get<NFA::SemanticTarget>(sem_entry.next_state).id;
                            } else if (std::holds_alternative<NFA::DFATarget>(sem_entry.next_state)) {
                                std::size_t st = std::get<NFA::DFATarget>(sem_entry.next_state).id;
                                target_part = (st != NFA::NULL_STATE) ? partition_of.at(st) : NFA::NULL_STATE;
                                break;
                            } else {
                                break;
                            }
                        }
                    }

                    key.push_back(TransitionKeyExt{
                        .symbol = symbol,
                        .table_type = NFA::TableType::Action,
                        .action = NFA::Action::UNDEF,
                        .action_id = chain_hash,
                        .target_partition = target_part
                    });
                }
                else if constexpr (std::is_same_v<T, NFA::SemanticTarget>) {
                    std::size_t curr_act = arg.id;
                    std::size_t chain_hash = 0;
                    std::size_t target_part = NFA::NULL_STATE;
                    bool in_semantic = true;

                    while (true) {
                        if (in_semantic) {
                            if (curr_act >= semantic_table.size()) break;
                            const auto &sem_entry = semantic_table.at(curr_act);

                            hash_combine(chain_hash, uhash {} (sem_entry.next_state));
                            hash_combine(chain_hash, uhash {} (sem_entry.instance_value));
                            hash_combine(chain_hash, uhash {} (sem_entry.statements));

                            if (std::holds_alternative<NFA::ActionTarget>(sem_entry.next_state)) {
                                curr_act = std::get<NFA::ActionTarget>(sem_entry.next_state).id;
                                in_semantic = false;
                            } else if (std::holds_alternative<NFA::SemanticTarget>(sem_entry.next_state)) {
                                curr_act = std::get<NFA::SemanticTarget>(sem_entry.next_state).id;
                            } else if (std::holds_alternative<NFA::DFATarget>(sem_entry.next_state)) {
                                std::size_t st = std::get<NFA::DFATarget>(sem_entry.next_state).id;
                                target_part = (st != NFA::NULL_STATE) ? partition_of.at(st) : NFA::NULL_STATE;
                                break;
                            } else {
                                break;
                            }
                        } else {
                            if (curr_act >= lr_table.size()) break;
                            const auto &act_entry = lr_table.at(curr_act);

                            hash_combine(chain_hash, uhash {} (act_entry.action));
                            hash_combine(chain_hash, uhash {} (act_entry.variable));

                            if (std::holds_alternative<NFA::ActionTarget>(act_entry.next_state)) {
                                curr_act = std::get<NFA::ActionTarget>(act_entry.next_state).id;
                            } else if (std::holds_alternative<NFA::SemanticTarget>(act_entry.next_state)) {
                                curr_act = std::get<NFA::SemanticTarget>(act_entry.next_state).id;
                                in_semantic = true;
                            } else if (std::holds_alternative<NFA::DFATarget>(act_entry.next_state)) {
                                std::size_t st = std::get<NFA::DFATarget>(act_entry.next_state).id;
                                target_part = (st != NFA::NULL_STATE) ? partition_of.at(st) : NFA::NULL_STATE;
                                break;
                            } else {
                                break;
                            }
                        }
                    }

                    key.push_back(TransitionKeyExt{
                        .symbol = symbol,
                        .table_type = NFA::TableType::Semantic,
                        .action = NFA::Action::UNDEF,
                        .action_id = chain_hash,
                        .target_partition = target_part
                    });
                }
            }, target);
        }

        std::sort(key.begin(), key.end());
        return key;
    }

    auto DFA::minimize() -> States<SingleState> {
        Tlog::Branch b(logger, "DFA/minimize.log");

        const auto &input = states;
        const std::size_t n = input.size();

        if (n == 0) {
            return States<SingleState>(&nfa);
        }

        // 1. Initial equivalence classes based on state hashing
        std::unordered_map<std::size_t, std::size_t> partition_of;
        std::unordered_map<std::size_t, std::size_t> initial_hash_to_class;
        std::size_t class_count = 0;

        for (std::size_t i = 0; i < n; ++i) {
            const std::size_t hash = initialClass(input[i]);
            auto [it, inserted] = initial_hash_to_class.emplace(hash, class_count);
            if (inserted) ++class_count;
            partition_of[i] = it->second;
        }

        // 2. Refinement loop until fixed point
        bool changed = true;
        while (changed) {
            changed = false;
            std::unordered_map<std::size_t, std::size_t> new_partition;
            std::map<std::pair<std::size_t, std::vector<TransitionKeyExt>>, std::size_t> signature_to_class;
            std::size_t new_class_count = 0;

            for (std::size_t i = 0; i < n; ++i) {
                const auto key = refinementKey(input[i], partition_of);
                const std::size_t accept_hash = initialClass(input[i]);
                const auto signature = std::make_pair(accept_hash, key);

                auto [it, inserted] = signature_to_class.emplace(signature, new_class_count);
                if (inserted) ++new_class_count;

                new_partition[i] = it->second;
                if (new_partition[i] != partition_of[i])
                    changed = true;
            }

            partition_of = std::move(new_partition);
        }

        // 3. Map partition classes to intermediate state indices
        std::unordered_map<std::size_t, std::size_t> class_to_new_index;
        States<SingleState> output(&nfa);

        for (std::size_t i = 0; i < n; ++i) {
            const std::size_t cls = partition_of.at(i);
            if (!class_to_new_index.contains(cls)) {
                class_to_new_index.emplace(cls, output.makeNew());
            }
        }

        // 4. Construct minimized output states
        std::unordered_set<std::size_t> constructed_classes;

        for (std::size_t i = 0; i < n; ++i) {
            const std::size_t cls = partition_of.at(i);
            const std::size_t new_idx = class_to_new_index.at(cls);

            if (!constructed_classes.insert(cls).second)
                continue;

            const auto &source = input[i];
            auto &destination = output[new_idx];

            destination.accept_binding = source.accept_binding;
            destination.entry_action = source.entry_action;

            for (const auto &[symbol, target] : source.transitions) {
                std::visit([&](auto &&arg) {
                    using T = std::decay_t<decltype(arg)>;

                    if constexpr (std::is_same_v<T, NFA::DFATarget>) {
                        const auto target_class = partition_of.at(arg.id);
                        destination.transitions[symbol] = NFA::DFATarget{ class_to_new_index.at(target_class) };
                    } else if constexpr (std::is_same_v<T, NFA::ActionTarget>) {
                        destination.transitions[symbol] = arg;
                    } else if constexpr (std::is_same_v<T, NFA::SemanticTarget>) {
                        destination.transitions[symbol] = arg;
                    }
                }, target);
            }
        }

        // 5. Reachability analysis to prune dead/unreachable states
        std::vector<bool> reachable(output.size(), false);
        std::queue<std::size_t> q;

        if (!output.empty()) {
            reachable[0] = true;
            q.push(0);
        }

        while (!q.empty()) {
            const std::size_t current = q.front();
            q.pop();

            for (const auto &[symbol, target] : output[current].transitions) {
                std::size_t next_st = NFA::NULL_STATE;

                if (std::holds_alternative<NFA::DFATarget>(target)) {
                    next_st = std::get<NFA::DFATarget>(target).id;
                } else if (std::holds_alternative<NFA::ActionTarget>(target)) {
                    std::size_t curr_act = std::get<NFA::ActionTarget>(target).id;
                    bool in_semantic = false;

                    while (true) {
                        if (!in_semantic) {
                            if (curr_act >= lr_table.size()) break;
                            const auto &act_entry = lr_table.at(curr_act);
                            if (std::holds_alternative<NFA::ActionTarget>(act_entry.next_state)) {
                                curr_act = std::get<NFA::ActionTarget>(act_entry.next_state).id;
                            } else if (std::holds_alternative<NFA::SemanticTarget>(act_entry.next_state)) {
                                curr_act = std::get<NFA::SemanticTarget>(act_entry.next_state).id;
                                in_semantic = true;
                            } else {
                                const std::size_t old_st = std::get<NFA::DFATarget>(act_entry.next_state).id;
                                if (old_st != NFA::NULL_STATE && old_st < partition_of.size()) {
                                    const std::size_t target_cls = partition_of.at(old_st);
                                    next_st = class_to_new_index.at(target_cls);
                                }
                                break;
                            }
                        } else {
                            if (curr_act >= semantic_table.size()) break;
                            const auto &sem_entry = semantic_table.at(curr_act);
                            if (std::holds_alternative<NFA::ActionTarget>(sem_entry.next_state)) {
                                curr_act = std::get<NFA::ActionTarget>(sem_entry.next_state).id;
                                in_semantic = false;
                            } else if (std::holds_alternative<NFA::SemanticTarget>(sem_entry.next_state)) {
                                curr_act = std::get<NFA::SemanticTarget>(sem_entry.next_state).id;
                            } else {
                                const std::size_t old_st = std::get<NFA::DFATarget>(sem_entry.next_state).id;
                                if (old_st != NFA::NULL_STATE && old_st < partition_of.size()) {
                                    const std::size_t target_cls = partition_of.at(old_st);
                                    next_st = class_to_new_index.at(target_cls);
                                }
                                break;
                            }
                        }
                    }
                } else if (std::holds_alternative<NFA::SemanticTarget>(target)) {
                    std::size_t curr_act = std::get<NFA::SemanticTarget>(target).id;
                    bool in_semantic = true;
                    while (true) {
                        if (in_semantic) {
                            if (curr_act >= semantic_table.size()) break;
                            const auto &sem_entry = semantic_table.at(curr_act);
                            if (std::holds_alternative<NFA::ActionTarget>(sem_entry.next_state)) {
                                curr_act = std::get<NFA::ActionTarget>(sem_entry.next_state).id;
                                in_semantic = false;
                            } else if (std::holds_alternative<NFA::SemanticTarget>(sem_entry.next_state)) {
                                curr_act = std::get<NFA::SemanticTarget>(sem_entry.next_state).id;
                            } else {
                                const std::size_t old_st = std::get<NFA::DFATarget>(sem_entry.next_state).id;
                                if (old_st != NFA::NULL_STATE && old_st < partition_of.size()) {
                                    const std::size_t target_cls = partition_of.at(old_st);
                                    next_st = class_to_new_index.at(target_cls);
                                }
                                break;
                            }
                        } else {
                            if (curr_act >= lr_table.size()) break;
                            const auto &act_entry = lr_table.at(curr_act);
                            if (std::holds_alternative<NFA::ActionTarget>(act_entry.next_state)) {
                                curr_act = std::get<NFA::ActionTarget>(act_entry.next_state).id;
                            } else if (std::holds_alternative<NFA::SemanticTarget>(act_entry.next_state)) {
                                curr_act = std::get<NFA::SemanticTarget>(act_entry.next_state).id;
                                in_semantic = true;
                            } else {
                                const std::size_t old_st = std::get<NFA::DFATarget>(act_entry.next_state).id;
                                if (old_st != NFA::NULL_STATE && old_st < partition_of.size()) {
                                    const std::size_t target_cls = partition_of.at(old_st);
                                    next_st = class_to_new_index.at(target_cls);
                                }
                                break;
                            }
                        }
                    }
                }

                if (next_st != NFA::NULL_STATE && next_st < reachable.size() && !reachable[next_st]) {
                    reachable[next_st] = true;
                    q.push(next_st);
                }
            }
        }

        // 6. Compact reachable states into the final state set
        States<SingleState> reachable_output(&nfa);
        std::vector<std::size_t> state_remap(output.size(), NFA::NULL_STATE);
        for (std::size_t i = 0; i < output.size(); ++i) {
            if (reachable[i]) {
                state_remap[i] = reachable_output.makeNew();
            }
        }
        // Direct remapper: converts an old DFATarget to its post-minimization, pruned index
        auto remap_dfa_target = [&](NFA::DFATarget &dfa_tgt) {
            if (dfa_tgt.id == NFA::NULL_STATE) return;

            auto p_it = partition_of.find(dfa_tgt.id);
            if (p_it == partition_of.end()) {
                dfa_tgt.id = NFA::NULL_STATE;
                return;
            }

            auto c_it = class_to_new_index.find(p_it->second);
            if (c_it == class_to_new_index.end() || c_it->second >= state_remap.size()) {
                dfa_tgt.id = NFA::NULL_STATE;
                return;
            }

            // Write the final remapped index directly back into the target object
            dfa_tgt.id = state_remap[c_it->second];
        };

        // Helper to inspect any NextTarget variant and apply the remap if it holds a DFATarget
        auto resolve_target_val = [&](TransitionTarget &target) {
            if (auto *dfa_tgt = std::get_if<NFA::DFATarget>(&target)) {
                remap_dfa_target(*dfa_tgt);
            }
        };

        // Update all embedded DFATarget references across tables in place
        for (auto &act : lr_table) {
            resolve_target_val(act.next_state);
        }
        for (auto &sem : semantic_table) {
            resolve_target_val(sem.next_state);
        }
        for (auto &state : output) {
            if (state.entry_action) {
                resolve_target_val(*state.entry_action);
            }
        }

        // 8. Copy transitions for reachable states
        for (std::size_t i = 0; i < output.size(); ++i) {
            if (!reachable[i]) continue;

            const std::size_t new_idx = state_remap[i];
            auto &destination = reachable_output[new_idx];
            const auto &source = output[i];

            destination.accept_binding = source.accept_binding;
            destination.entry_action = source.entry_action;

            for (const auto &[symbol, target] : source.transitions) {
                std::visit([&](auto &&arg) {
                    using T = std::decay_t<decltype(arg)>;

                    if constexpr (std::is_same_v<T, NFA::DFATarget>) {
                        std::size_t final_target = (arg.id < state_remap.size()) ? state_remap[arg.id] : NFA::NULL_STATE;
                        if (final_target != NFA::NULL_STATE) {
                            destination.transitions[symbol] = NFA::DFATarget{ final_target };
                        }
                    } else if constexpr (std::is_same_v<T, NFA::ActionTarget>) {
                        destination.transitions[symbol] = arg;
                    } else if constexpr (std::is_same_v<T, NFA::SemanticTarget>) {
                        destination.transitions[symbol] = arg;
                    }
                }, target);
            }
        }

        this->states = std::move(reachable_output);

        // 9. Fixed-point iterative co-optimization of tables
        bool changes = true;
        while (changes) {
            const auto prev_sem = semantic_table;
            const auto prev_lr = lr_table;

            optimizeSemanticTable();
            optimizeRegistersAndLRTable();

            bool semantic_changed = prev_sem.size() != semantic_table.size();
            if (!semantic_changed) {
                for (std::size_t i = 0; i < semantic_table.size(); ++i) {
                    const auto &a = prev_sem[i];
                    const auto &b = semantic_table[i];
                    if (a.next_state != b.next_state ||
                        a.instance_value != b.instance_value ||
                        a.statements != b.statements ||
                        a.nfa_index != b.nfa_index) {
                        semantic_changed = true;
                        break;
                    }
                }
            }

            bool lr_changed = prev_lr.size() != lr_table.size();
            if (!lr_changed) {
                for (std::size_t i = 0; i < lr_table.size(); ++i) {
                    const auto &a = prev_lr[i];
                    const auto &b = lr_table[i];
                    if (a.action != b.action ||
                        a.variable != b.variable ||
                        a.next_state != b.next_state) {
                        lr_changed = true;
                        break;
                    }
                }
            }

            changes = semantic_changed || lr_changed;
        }

        return this->states;
    }

    auto DFA::classify() -> ClassifiedDFA {
        if (!nfa.isCharNfa()) {
            throw Error("classify() only applies to character-keyed (CharMachineDFA) automata");
        }

        constexpr std::size_t ALPHABET_SIZE = 256;
        const std::size_t n = states.size();

        using Signature = std::vector<std::pair<NFA::TableType, std::size_t>>;

        std::unordered_map<Signature, std::size_t, uhash> class_of_signature;
        CharClassTable table;

        for (std::size_t c = 0; c < ALPHABET_SIZE; ++c) {
            Signature sig;
            sig.reserve(n);
            NFA::TransitionKey key{static_cast<char>(c)};

            for (std::size_t i = 0; i < n; ++i) {
                auto it = states[i].transitions.find(key);
                if (it == states[i].transitions.end()) {
                    sig.emplace_back(NFA::TableType::DFA, NULL_STATE);
                } else {
                    std::visit([&](auto &&target) {
                        using T = std::decay_t<decltype(target)>;
                        if constexpr (std::is_same_v<T, NFA::DFATarget>) {
                            sig.emplace_back(NFA::TableType::DFA, target.id);
                        } else if constexpr (std::is_same_v<T, NFA::ActionTarget>) {
                            sig.emplace_back(NFA::TableType::Action, target.id);
                        } else if constexpr (std::is_same_v<T, NFA::SemanticTarget>) {
                            sig.emplace_back(NFA::TableType::Semantic, target.id);
                        }
                    }, it->second);
                }
            }
            auto [it, inserted] = class_of_signature.try_emplace(sig, class_of_signature.size());
            table.char_to_class[c] = it->second;
        }
        table.num_classes = class_of_signature.size();

        auto resolve_final_dfa_state = [&](const auto &start_target) -> std::size_t {
            using NextTarget = std::variant<NFA::DFATarget, NFA::ActionTarget, NFA::SemanticTarget>;
            NextTarget curr = start_target;
            const std::size_t max_steps = lr_table.size() + semantic_table.size() + 1;
            std::size_t steps = 0;

            while (steps++ < max_steps) {
                if (std::holds_alternative<NFA::DFATarget>(curr)) {
                    return std::get<NFA::DFATarget>(curr).id;
                } else if (std::holds_alternative<NFA::ActionTarget>(curr)) {
                    const std::size_t idx = std::get<NFA::ActionTarget>(curr).id;
                    if (idx >= lr_table.size()) break;
                    curr = lr_table[idx].next_state;
                } else if (std::holds_alternative<NFA::SemanticTarget>(curr)) {
                    const std::size_t idx = std::get<NFA::SemanticTarget>(curr).id;
                    if (idx >= semantic_table.size()) break;
                    curr = semantic_table[idx].next_state;
                } else {
                    break;
                }
            }
            return NULL_STATE;
        };

        States<State<ClassTransitions>> output(&nfa);
        for (std::size_t i = 0; i < n; ++i) {
            auto new_idx = output.makeNew();
            output[new_idx].accept_binding = states[i].accept_binding;
            output[new_idx].entry_action = states[i].entry_action;

            // Default terminal transition when no character shift exists in state i
            TransitionValue default_trans{ NULL_STATE, NFA::TableType::DFA, NULL_STATE };
            if (states[i].accept_binding.has_value()) {
                const auto &binding = *states[i].accept_binding;
                if (binding.reduce_rule_id.has_value()) {
                    default_trans = TransitionValue{
                        NULL_STATE,
                        NFA::TableType::Action,
                        *binding.reduce_rule_id
                    };
                } else if (binding.target_semantic_state.has_value()) {
                    default_trans = TransitionValue{
                        NULL_STATE,
                        NFA::TableType::Semantic,
                        *binding.target_semantic_state
                    };
                } else {
                    default_trans = TransitionValue{
                        NULL_STATE,
                        NFA::TableType::DFA,
                        binding.token_id
                    };
                }
            }

            output[new_idx].transitions.assign(table.num_classes, default_trans);

            for (const auto &[symbol, value] : states[i].transitions) {
                if (!std::holds_alternative<char>(symbol)) continue;
                unsigned char c = static_cast<unsigned char>(std::get<char>(symbol));
                std::size_t cls = table.char_to_class[c];

                std::visit([&](auto &&target) {
                    using T = std::decay_t<decltype(target)>;

                    if constexpr (std::is_same_v<T, NFA::DFATarget>) {
                        output[new_idx].transitions[cls] = TransitionValue{
                            target.id,
                            NFA::TableType::DFA,
                            NULL_STATE
                        };
                    } else if constexpr (std::is_same_v<T, NFA::ActionTarget>) {
                        output[new_idx].transitions[cls] = TransitionValue{
                            resolve_final_dfa_state(target),
                            NFA::TableType::Action,
                            target.id
                        };
                    } else if constexpr (std::is_same_v<T, NFA::SemanticTarget>) {
                        output[new_idx].transitions[cls] = TransitionValue{
                            resolve_final_dfa_state(target),
                            NFA::TableType::Semantic,
                            target.id
                        };
                    }
                }, value);
            }
        }

        return ClassifiedDFA{std::move(table), std::move(output)};
    }

    auto DFA::clear() -> void {
        states.clear();
        lr_table.clear();
        semantic_table.clear();
    }

    auto DFA::getType() const -> DfaType {
        return nfa.isCharNfa() ? DfaType::Char : DfaType::Token;
    }

    auto DFA::check_dfa() -> void {
        std::size_t index = 0;
        try {
            for (const auto &state : states) {
                for (const auto &[sym, transitions] : state.transitions) {
                    if (std::holds_alternative<stdu::vector<std::string>>(sym)) {
                        const auto &nested_name = std::get<stdu::vector<std::string>>(sym);
                        AssertNe(nested_name.empty(), "Empty nested_name in state {}", index);
                    }
                    if (std::holds_alternative<NFA::DFATarget>(transitions)) {
                        const auto next = std::get<NFA::DFATarget>(transitions).id;
                        Assert(next == NFA::NULL_STATE || states.size() > next,
                               "Out of bound transition {} in state {}", next, index);
                    } else if (std::holds_alternative<NFA::ActionTarget>(transitions)) {
                        const auto act_idx = std::get<NFA::ActionTarget>(transitions).id;
                        Assert(lr_table.size() > act_idx, "Out of bound action index {} in state {}", act_idx, index);
                    } else if (std::holds_alternative<NFA::SemanticTarget>(transitions)) {
                        const auto sem_idx = std::get<NFA::SemanticTarget>(transitions).id;
                        Assert(semantic_table.size() > sem_idx, "Out of bound semantic index {} in state {}", sem_idx, index);
                    }
                }
                ++index;
            }
        } catch (Error &e) {
            std::cout << "[MDFA] Check Failed > " << e.what() << '\n';
            std::abort();
        }
    }

    auto operator<<(std::ostream& os, const DFA& dfa) -> std::ostream& {
        (void)dfa;
        os << "<DFA stream dump unavailable for current transition model>\n";
        return os;
    }

    auto operator<<(std::ostream& os, const ClassifiedDFA& dfa) -> std::ostream& {
        os << dfa.table.num_classes << " equivalence classes\n";
        for (std::size_t i = 0; i < dfa.table.num_classes; ++i) {
            os << "Class " << i << ": ";
            for (std::size_t j = 0; j < dfa.table.char_to_class.size(); ++j) {
                if (dfa.table.char_to_class[j] != i) continue;
                if (std::isprint(static_cast<unsigned char>(j))) {
                    os << static_cast<char>(j) << ' ';
                } else {
                    os << corelib::text::getEscapedFromChar(static_cast<char>(j)) << ' ';
                }
            }
            os << '\n';
        }

        os << "--- DFA ---\n";
        std::size_t index = 0;
        for (const auto &state : dfa.states) {
            os << "State " << index << ": \n";
            for (std::size_t cls = 0; cls < state.transitions.size(); ++cls) {
                const auto &t = state.transitions[cls];
                if (t.next == NULL_STATE) continue;
                os << "\t" << cls << " -> " << t.next;
                if (t.accept_index != NULL_STATE) {
                    os << " [accept: " << t.accept_index << "]";
                }
                os << '\n';
            }
            ++index;
        }
        return os;
    }
}
