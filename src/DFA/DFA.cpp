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

        // ================================================================
        // 2. Subset construction
        // ================================================================

        while (!work.empty()) {
            Closure current = work.front();
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

                // Walk a chain starting at a discovered root, following each entry's own
                // next_state link (NOT info_list) until it stops being Action/Semantic.
                auto walk_chain = [&](std::size_t start_index, NFA::TableType start_type) {
                    std::size_t idx = start_index;
                    NFA::TableType type = start_type;

                    while (true) {
                        if (type == NFA::TableType::Action) {
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
                            break; // DFATarget or nothing further: chain ends here
                        }
                        if (type == NFA::TableType::Semantic) {
                            semantic_indices.push_back(idx);
                            // Per your semantic model, Semantic is chain-terminal
                            // (SemanticState::next_state is a plain DFA index, not a variant).
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
                // Extract actions directly captured during epsilon-closure!
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

            const std::size_t nfa_semantic_index = *binding->target_semantic_state;
            if (nfa_semantic_index >= nfa.getSemanticTable().size()) {
                throw Error("DFA state {} references invalid semantic state {}", dfa_index, nfa_semantic_index);
            }

            const auto &nfa_semantic = nfa.getSemanticTable().at(nfa_semantic_index);

            std::size_t raw_next_nfa_state = NFA::NULL_STATE;
            if (std::holds_alternative<NFA::DFATarget>(nfa_semantic.next_state)) {
                raw_next_nfa_state = std::get<NFA::DFATarget>(nfa_semantic.next_state).id;
            } else {
                throw Error("Accept-time semantic state {} unexpectedly chains into Action/Semantic at NFA-build time", nfa_semantic_index);
            }

            std::size_t next_dfa_state = NFA::NULL_STATE;

            if (raw_next_nfa_state != NFA::NULL_STATE) {
                const auto it = nfa_to_dfa.find(raw_next_nfa_state);
                if (it == nfa_to_dfa.end() || it->second.empty()) {
                    throw Error("Cannot resolve semantic next NFA state {} to DFA state", raw_next_nfa_state);
                }

                bool resolved = false;
                for (const std::size_t candidate_dfa : it->second) {
                    if (dfa_closures.at(candidate_dfa).contains(raw_next_nfa_state)) {
                        next_dfa_state = candidate_dfa;
                        resolved = true;
                        break;
                    }
                }

                if (!resolved) {
                    next_dfa_state = it->second.front();
                }
            }

            const auto cache_key = std::make_pair(nfa_semantic_index, next_dfa_state);
            auto sem_cache_it = accept_semantic_cache.find(cache_key);

            std::size_t new_sem_idx;
            if (sem_cache_it != accept_semantic_cache.end()) {
                new_sem_idx = sem_cache_it->second;
            } else {
                auto semantic_copy = nfa_semantic;
                semantic_copy.next_state = NFA::DFATarget{next_dfa_state};
                semantic_copy.nfa_index = nfa_semantic_index;

                new_sem_idx = semantic_table.size();
                semantic_table.push_back(std::move(semantic_copy));
                accept_semantic_cache.emplace(cache_key, new_sem_idx);
            }
            binding->target_semantic_state = new_sem_idx;

            // IMPORTANT: do NOT rewrite DFA transitions to SemanticTarget here.
            //
            // A semantic accept action is metadata of the accepting DFA state;
            // it is not a DFA edge. Rewriting every incoming edge to this state
            // changes the DFA graph into a DFA/semantic graph and can create
            // cycles such as:
            //
            //     state -> Semantic -> state
            //
            // The minimizer follows action/semantic chains while constructing
            // transition signatures, so such a cycle can make refinement never
            // terminate. The semantic state remains reachable through
            // accept_binding->target_semantic_state instead.
        }

        // ================================================================
        // 4. Validate
        // ================================================================

        if (states.empty())
            throw Error("DFA cannot be empty");

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

        auto resolve_target = [&](auto self, const auto &target) -> void {
            if (std::holds_alternative<NFA::SemanticTarget>(target)) {
                std::size_t act_id = std::get<NFA::SemanticTarget>(target).id;
                if (act_id < semantic_table.size()) {
                    // Mark this entry itself as used -- previously only the
                    // accept_binding-rooted marks below ever set `used`, so any
                    // semantic entry reachable only through a transition or an
                    // action chain was invisible here and got silently dropped
                    // (or worse, left as a now-stale index into the compacted
                    // table) by the compaction pass further down.
                    mark_semantic(act_id);
                    self(self, semantic_table[act_id].next_state);
                }
            } else if (std::holds_alternative<NFA::ActionTarget>(target)) {
                std::size_t act_id = std::get<NFA::ActionTarget>(target).id;
                if (act_id < lr_table.size()) {
                    self(self, lr_table[act_id].next_state);
                }
            }
        };

        for (const auto &state : states) {
            if (state.accept_binding && state.accept_binding->target_semantic_state) {
                mark_semantic(*state.accept_binding->target_semantic_state);
            }
            for (const auto &[symbol, target] : state.transitions) {
                resolve_target(resolve_target, target);
            }
        }
        for (const auto &state : lr_table) {
            if (std::holds_alternative<NFA::SemanticTarget>(state.next_state)) {
                resolve_target(resolve_target, state.next_state);
            }
        }
        std::unordered_map<std::size_t, std::size_t> remap;
        std::vector<NFA::SemanticState> compacted;

        for (std::size_t i = 0; i < semantic_table.size(); ++i) {
            if (!used[i]) continue;
            const auto &entry = semantic_table[i];
            std::size_t canonical_idx = NFA::NULL_STATE;

            for (std::size_t j = 0; j < compacted.size(); ++j) {
                // Dedup must include nfa_index. Structural equality of
                // (next_state, statements, instance_value) alone isn't
                // sufficient: intermediate push-entries created for nested
                // token references (e.g. a rule member that's itself another
                // token) are generic ("push the matched Node, continue") and
                // can end up textually identical across completely unrelated
                // rules -- especially once minimize() has already merged
                // their next_state targets too. Without nfa_index to
                // disambiguate, two entries from different original NFA
                // accept points collapse into one shared compacted entry, and
                // every DFA transition remapped onto it silently executes
                // reduce logic spliced together from two unrelated origins.
                // nfa_index does still allow the safe case -- the SAME
                // original NFA accept point reached via multiple DFA paths --
                // to merge, since those share nfa_index by construction.
                if (compacted[j].next_state == entry.next_state &&
                    compacted[j].statements == entry.statements &&
                    compacted[j].instance_value == entry.instance_value &&
                    compacted[j].nfa_index == entry.nfa_index
                    ) {
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

        // Fix up internal chain references: a semantic entry's own next_state
        // can itself be a SemanticTarget pointing at another semantic entry
        // (e.g. an inner nested-token reduce chaining into the outer reduce
        // that consumes its pushed value). Each entry was copied into
        // `compacted` as-is above, so any such internal reference still holds
        // its PRE-compaction index. Only external references (lr_table,
        // states) were being fixed up below -- this internal one was never
        // touched, leaving a stale index baked directly into the entries that
        // survive compaction.
        for (auto &entry : compacted) {
            if (std::holds_alternative<NFA::SemanticTarget>(entry.next_state)) {
                auto &sem = std::get<NFA::SemanticTarget>(entry.next_state);
                if (auto it = remap.find(sem.id); it != remap.end()) {
                    sem.id = it->second;
                }
            }
        }

        for (auto &act : lr_table) {
            if (std::holds_alternative<NFA::SemanticTarget>(act.next_state)) {
                auto &sem = std::get<NFA::SemanticTarget>(act.next_state);
                if (auto it = remap.find(sem.id); it != remap.end()) {
                    sem.id = it->second;
                }
            }
        }

        for (auto &state : states) {
            if (state.accept_binding && state.accept_binding->target_semantic_state) {
                if (auto it = remap.find(*state.accept_binding->target_semantic_state); it != remap.end()) {
                    state.accept_binding->target_semantic_state = it->second;
                }
            }
            for (auto &[symbol, target] : state.transitions) {
                if (std::holds_alternative<NFA::SemanticTarget>(target)) {
                    auto &sem = std::get<NFA::SemanticTarget>(target);
                    if (auto it = remap.find(sem.id); it != remap.end()) {
                        sem.id = it->second;
                    }
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
                    std::size_t chain_hash = 0;
                    std::size_t target_part = NFA::NULL_STATE;

                    while (curr_act < lr_table.size()) {
                        const auto &act_entry = lr_table.at(curr_act);

                        hash_combine(chain_hash, uhash {} (act_entry.action));
                        hash_combine(chain_hash, uhash {} (act_entry.variable));

                        if (std::holds_alternative<NFA::ActionTarget>(act_entry.next_state)) {
                            curr_act = std::get<NFA::ActionTarget>(act_entry.next_state).id;
                        } else if (std::holds_alternative<NFA::SemanticTarget>(act_entry.next_state)) {
                            curr_act = std::get<NFA::SemanticTarget>(act_entry.next_state).id;
                        } else if (std::holds_alternative<NFA::DFATarget>(act_entry.next_state)) {
                            std::size_t st = std::get<NFA::DFATarget>(act_entry.next_state).id;
                            target_part = (st != NFA::NULL_STATE) ? partition_of.at(st) : NFA::NULL_STATE;
                            break;
                        } else {
                            break;
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

                    while (curr_act < semantic_table.size()) {
                        const auto &sem_entry = semantic_table.at(curr_act);

                        hash_combine(chain_hash, uhash {} (sem_entry.next_state));
                        hash_combine(chain_hash, uhash {} (sem_entry.instance_value));
                        hash_combine(chain_hash, uhash {} (sem_entry.statements));

                        if (std::holds_alternative<NFA::ActionTarget>(sem_entry.next_state)) {
                            curr_act = std::get<NFA::ActionTarget>(sem_entry.next_state).id;
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

        std::unordered_map<std::size_t, std::size_t> partition_of;
        std::unordered_map<std::size_t, std::size_t> initial_hash_to_class;
        std::size_t class_count = 0;

        for (std::size_t i = 0; i < n; ++i) {
            const std::size_t hash = initialClass(input[i]);
            auto [it, inserted] = initial_hash_to_class.emplace(hash, class_count);
            if (inserted) ++class_count;
            partition_of[i] = it->second;
        }

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

        std::unordered_map<std::size_t, std::size_t> class_to_new_index;
        States<SingleState> output(&nfa);

        for (std::size_t i = 0; i < n; ++i) {
            const std::size_t cls = partition_of.at(i);
            if (!class_to_new_index.contains(cls)) {
                class_to_new_index.emplace(cls, output.makeNew());
            }
        }

        std::unordered_set<std::size_t> constructed_classes;

        for (std::size_t i = 0; i < n; ++i) {
            const std::size_t cls = partition_of.at(i);
            const std::size_t new_idx = class_to_new_index.at(cls);

            if (!constructed_classes.insert(cls).second)
                continue;

            const auto &source = input[i];
            auto &destination = output[new_idx];

            destination.accept_binding = source.accept_binding;

            for (const auto &[symbol, target] : source.transitions) {
                std::visit([&](auto &&arg) {
                    using T = std::decay_t<decltype(arg)>;

                    if constexpr (std::is_same_v<T, NFA::DFATarget>) {
                        const auto target_class = partition_of.at(arg.id);
                        destination.transitions[symbol] = NFA::DFATarget{ class_to_new_index.at(target_class) };
                    }
                    else if constexpr (std::is_same_v<T, NFA::ActionTarget>) {
                        destination.transitions[symbol] = arg;
                    }
                    else if constexpr (std::is_same_v<T, NFA::SemanticTarget>) {
                        destination.transitions[symbol] = arg;
                    }
                }, target);
            }
        }

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
                    while (curr_act < lr_table.size()) {
                        const auto &act_entry = lr_table.at(curr_act);
                        if (std::holds_alternative<NFA::ActionTarget>(act_entry.next_state)) {
                            curr_act = std::get<NFA::ActionTarget>(act_entry.next_state).id;
                        } else if (std::holds_alternative<NFA::SemanticTarget>(act_entry.next_state)) {
                            curr_act = std::get<NFA::SemanticTarget>(act_entry.next_state).id;
                        } else {
                            const std::size_t old_st = std::get<NFA::DFATarget>(act_entry.next_state).id;
                            if (old_st != NFA::NULL_STATE && old_st < partition_of.size()) {
                                const std::size_t target_cls = partition_of.at(old_st);
                                next_st = class_to_new_index.at(target_cls);
                            }
                            break;
                        }
                    }
                } else if (std::holds_alternative<NFA::SemanticTarget>(target)) {
                    std::size_t curr_act = std::get<NFA::SemanticTarget>(target).id;
                    while (curr_act < semantic_table.size()) {
                        const auto &act_entry = semantic_table.at(curr_act);
                        if (std::holds_alternative<NFA::ActionTarget>(act_entry.next_state)) {
                            curr_act = std::get<NFA::ActionTarget>(act_entry.next_state).id;
                        } else if (std::holds_alternative<NFA::SemanticTarget>(act_entry.next_state)) {
                            curr_act = std::get<NFA::SemanticTarget>(act_entry.next_state).id;
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

                if (next_st != NFA::NULL_STATE && next_st < reachable.size() && !reachable[next_st]) {
                    reachable[next_st] = true;
                    q.push(next_st);
                }
            }
        }

        States<SingleState> reachable_output(&nfa);
        std::vector<std::size_t> state_remap(output.size(), NFA::NULL_STATE);

        for (std::size_t i = 0; i < output.size(); ++i) {
            if (reachable[i]) {
                state_remap[i] = reachable_output.makeNew();
            }
        }

        auto resolve_state = [&](std::size_t old_dfa_idx) -> std::size_t {
            if (old_dfa_idx == NFA::NULL_STATE) return NFA::NULL_STATE;
            auto p_it = partition_of.find(old_dfa_idx);
            if (p_it == partition_of.end()) return NFA::NULL_STATE;
            std::size_t cls = p_it->second;
            auto c_it = class_to_new_index.find(cls);
            if (c_it == class_to_new_index.end()) return NFA::NULL_STATE;
            return state_remap[c_it->second];
        };

        for (auto &act : lr_table) {
            if (std::holds_alternative<NFA::DFATarget>(act.next_state)) {
                std::size_t old_st = std::get<NFA::DFATarget>(act.next_state).id;
                act.next_state = NFA::DFATarget{ resolve_state(old_st) };
            }
        }

        for (auto &semantic : semantic_table) {
            if (std::holds_alternative<NFA::DFATarget>(semantic.next_state)) {
                semantic.next_state = NFA::DFATarget {resolve_state(std::get<NFA::DFATarget>(semantic.next_state).id)};
            }
        }

        for (std::size_t i = 0; i < output.size(); ++i) {
            if (!reachable[i]) continue;

            const std::size_t new_idx = state_remap[i];
            auto &destination = reachable_output[new_idx];
            const auto &source = output[i];

            destination.accept_binding = source.accept_binding;

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

        // Fixed-point iterative co-optimization of tables
        bool changes = true;
        while (changes) {
            std::size_t prev_sem = semantic_table.size();
            std::size_t prev_lr = lr_table.size();

            optimizeSemanticTable();
            optimizeRegistersAndLRTable();

            if (semantic_table.size() == prev_sem && lr_table.size() == prev_lr) {
                changes = false;
            }
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

            // Default terminal transition when no character shift exists in state i
            TransitionValue default_trans{ NULL_STATE, NFA::TableType::DFA, NULL_STATE };
            if (states[i].accept_binding.has_value()) {
                const auto &binding = *states[i].accept_binding;
                if (binding.target_semantic_state.has_value()) {
                    default_trans = TransitionValue{
                        NULL_STATE,
                        NFA::TableType::Semantic,
                        *binding.target_semantic_state
                    };
                } else if (binding.reduce_rule_id.has_value()) {
                    default_trans = TransitionValue{
                        NULL_STATE,
                        NFA::TableType::Action,
                        *binding.reduce_rule_id
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
                        Assert(states.size() > next, "Out of bound transition {} in state {}", next, index);
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