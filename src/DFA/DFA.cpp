module DFA;

import DFA.States;
import DFA.closure;
import hash;
import logging;
import corelib;
import cpuf.printf;
import dstd;
import std;

namespace DFA {
auto DFA::build() -> const States<StateWithActions> & {
    Tlog::Branch b(logger, "DFA.log");

    namespace TNFA = NFA::TNFA;

    using RawAction = TNFA::RawAction;
    using NextTarget = std::variant<
        TNFA::DFATarget,
        ActionSequence
    >;

    /*
     * ------------------------------------------------------------------
     * Helpers
     * ------------------------------------------------------------------
     */

    std::map<std::vector<std::size_t>, std::size_t> dfa_state_map;
    std::queue<std::size_t> work_queue;
    std::vector<Closure> dfa_closures;

    states_with_actions.clear();
    action_table.clear();
    semantic_table.clear();


    /*
     * ------------------------------------------------------------------
     * Debug provenance
     * ------------------------------------------------------------------
     */

    auto collect_debug_origins =
        [](const std::vector<const NFA::IR::TokenID*> &sources)
            -> TNFA::DebugOrigins {

        TNFA::DebugOrigins out;

        for (const auto &source : sources) {
            if (std::ranges::none_of(
                    out,
                    [&](const NFA::IR::TokenID &existing) {
                        return existing == *source;
                    })) {

                out.push_back(*source);
            }
        }

        return out;
    };

    // Mirrors collect_debug_origins, but for the per-character grammar
    // site (CharOrigin) rather than the owning TokenID. Kept as its own
    // vector - same shape as ActionSequence::char_origin - so every
    // contributing candidate's site survives folding, not just one.
    auto collect_char_origins =
        [](const std::vector<std::optional<TNFA::CharOrigin>> &sources)
            -> stdu::vector<std::optional<TNFA::CharOrigin>> {

        stdu::vector<std::optional<TNFA::CharOrigin>> out;

        for (const auto &source : sources) {
            if (std::ranges::none_of(
                    out,
                    [&](const std::optional<TNFA::CharOrigin> &existing) {
                        return existing == source;
                    })) {

                out.push_back(source);
            }
        }

        return out;
    };


    /*
     * ------------------------------------------------------------------
     * Raw-action equality
     * ------------------------------------------------------------------
     *
     * The complete TNFA action is significant.
     *
     * In particular ActionState::next_state is part of its identity.
     */

    auto append_unique_action =
        [](stdu::vector<RawAction> &out,
           const RawAction &raw) {

        const bool exists =
            std::ranges::any_of(
                out,
                [&](const RawAction &existing) {

                    if (existing.index() != raw.index())
                        return false;

                    return std::visit(
                        [&](const auto &lhs,
                            const auto &rhs) -> bool {

                            using L =
                                std::decay_t<decltype(lhs)>;

                            using R =
                                std::decay_t<decltype(rhs)>;

                            if constexpr (
                                std::is_same_v<L, R>
                            ) {
                                return lhs == rhs;
                            }

                            return false;
                        },
                        existing,
                        raw
                    );
                }
            );

        if (!exists)
            out.push_back(raw);
    };


    /*
     * ------------------------------------------------------------------
     * Build ActionSequence
     * ------------------------------------------------------------------
     */

    auto make_action_sequence =
        [](const stdu::vector<RawAction> &actions,
           const std::size_t terminal_dfa_target,
           TNFA::DebugOrigins debug,
           stdu::vector<std::optional<TNFA::CharOrigin>> char_origin) -> NextTarget {

        /*
         * A consuming transition always has a real DFA destination.
         *
         * NULL_STATE is reserved for accept_action, which has no
         * following DFA state.
         */
        Assert(
            terminal_dfa_target != TNFA::NULL_STATE,
            "DFA::build: consuming transition has NULL target"
        );

        if (actions.empty()) {
            return TNFA::DFATarget{
                .id = terminal_dfa_target,
                .debug = std::move(debug.back()),
                .char_origin = char_origin.empty()
                    ? std::nullopt
                    : char_origin.back()
            };
        }

        return ActionSequence{
            .actions = actions,
            .terminal_dfa_target = terminal_dfa_target,
            .debug = std::move(debug),
            .char_origin = std::move(char_origin)
        };
    };


    /*
     * ------------------------------------------------------------------
     * Start DFA state
     * ------------------------------------------------------------------
     */

    std::vector<std::size_t> entries;

    for (const auto &[_, entry] : nfa.getTokenEntries())
        entries.push_back(entry);

    Closure start_closure(
        &nfa,
        entries
    );

    const std::vector<std::size_t> start_subset =
        start_closure.get();

    const std::size_t start_idx =
        states_with_actions.makeNew();

    dfa_state_map.emplace(
        start_subset,
        start_idx
    );

    dfa_closures.push_back(
        std::move(start_closure)
    );

    work_queue.push(start_idx);


    /*
     * ------------------------------------------------------------------
     * Subset construction
     * ------------------------------------------------------------------
     */

    while (!work_queue.empty()) {
        const std::size_t current_dfa_index =
            work_queue.front();

        work_queue.pop();

        const Closure &current_closure =
            dfa_closures.at(current_dfa_index);

        const std::vector<std::size_t> current_subset =
            current_closure.get();


        /*
         * ==============================================================
         * A. Resolve top-level accepting binding
         * ==============================================================
         * Therefore the binding only determines WHICH token wins.
         */

        std::optional<TNFA::TokenBinding> best_binding;
        std::size_t best_binding_nfa_index =
            TNFA::NULL_STATE;

        for (const std::size_t nfa_index : current_subset) {
            const auto &nfa_state =
                nfa.getStates().at(nfa_index);

            auto accept_it =
                nfa.getAcceptMap().find(nfa_index);

            std::optional<TNFA::TokenBinding> binding =
                accept_it != nfa.getAcceptMap().end()
                    ? std::make_optional(accept_it->second)
                    : nfa_state.accept_binding;

            if (!binding.has_value())
                continue;

            if (!best_binding.has_value() ||
                binding->token_id >
                    best_binding->token_id) {

                best_binding = binding;
                best_binding_nfa_index = nfa_index;
            }
        }


        /*
         * ==============================================================
         * B. Accepting action sequence
         * ==============================================================
         *
         * The complete epsilon closure path to the accepting NFA state
         * already contains the SemanticState if one exists.
         *
         * No semantic table lookup is performed here.
         */

        if (best_binding.has_value()) {
            states_with_actions[current_dfa_index].accept_binding =
                best_binding;

            const ActionPath &accept_path =
                current_closure.getActionsForState(
                    best_binding_nfa_index
                );

            const ActionPath &terminal_path =
                current_closure.getTerminalActionsForState(
                    best_binding_nfa_index
                );

            stdu::vector<RawAction> accept_actions;

            for (const auto &fired : accept_path)
                append_unique_action(
                    accept_actions,
                    fired.raw
                );

            for (const auto &fired : terminal_path)
                append_unique_action(
                    accept_actions,
                    fired.raw
                );

            if (!accept_actions.empty()) {
                states_with_actions[current_dfa_index]
                    .accept_action =
                    ActionSequence{
                        .actions = std::move(accept_actions),
                        .terminal_dfa_target =
                            TNFA::NULL_STATE,
                        .debug = {}
                    };
            }
        }


        /*
         * ==============================================================
         * C. Collect consuming transition candidates
         * ==============================================================
         */

        struct TransitionCandidate {
            std::size_t priority = 0;

            std::size_t target = TNFA::NULL_STATE;
            std::size_t source = TNFA::NULL_STATE;

            const NFA::IR::TokenID* origin;
            // The per-character grammar site this specific edge came
            // from. Distinct from `origin` (the TokenID/rule the edge
            // belongs to) - this is what lets a debugger point at the
            // exact character literal, and it must survive subset
            // construction the same way `origin` does.
            std::optional<TNFA::CharOrigin> char_origin;
        };


        std::map<
            TNFA::TransitionKey,
            std::vector<TransitionCandidate>
        > transition_candidates;


        for (const std::size_t nfa_index : current_subset) {
            const auto &nfa_state =
                nfa.getStates().at(nfa_index);

            /*
             * Actions needed to reach nfa_index within the CURRENT
             * DFA state already fired as part of the transition that
             * produced this DFA state - they must not be re-added
             * here. Only the consuming edge + everything epsilon-
             * reachable after it (collected below via
             * next_closure.getTransitionActions()) belongs to THIS
             * transition.
             */

            for (const auto &[symbol, targets] :
                 nfa_state.transitions) {

                for (const auto &target : targets) {
                    if (target.next == TNFA::NULL_STATE)
                        continue;

                    transition_candidates[symbol].push_back(
                        TransitionCandidate{
                            .priority = target.priority,
                            .target = target.next,
                            .source = nfa_index,
                            .origin = &target.source,
                            .char_origin = target.char_origin
                        }
                    );
                }
            }
        }


        /*
         * ==============================================================
         * D. Build DFA transitions
         * ==============================================================
         */

        for (auto &[symbol, candidates] :
             transition_candidates) {

            if (candidates.empty())
                continue;


            /*
             * ----------------------------------------------------------
             * Priority
             * ----------------------------------------------------------
             */

            std::ranges::stable_sort(
                candidates,
                [](const TransitionCandidate &a,
                   const TransitionCandidate &b) {

                    if (a.priority != b.priority)
                        return a.priority < b.priority;

                    return a.source < b.source;
                }
            );


            /*
             * ----------------------------------------------------------
             * Construct destination closure
             * ----------------------------------------------------------
             *
             * Closure is responsible for following:
             *
             *   consuming edge
             *       ↓
             *   epsilon transitions
             *       ↓
             *   ActionState
             *       ↓
             *   SemanticState
             *       ↓
             *   ...
             *
             * and recording that path in ActionPath.
             */

            Closure next_closure(
                &nfa,
                current_subset,
                symbol
            );

            const std::vector<std::size_t> next_subset =
                next_closure.get();

            if (next_subset.empty())
                continue;

            /*
             * Read the transition's actions NOW, before next_closure can
             * be moved into dfa_closures below. Reading them afterwards
             * used a moved-from Closure, so every transition into a
             * newly discovered DFA state silently lost its actions.
             */
            stdu::vector<RawAction> actions;

            for (const auto &fired : next_closure.getTransitionActions())
                actions.push_back(fired.raw);


            /*
             * ----------------------------------------------------------
             * Destination DFA state
             * ----------------------------------------------------------
             */

            std::size_t target_dfa_index;

            auto dfa_it =
                dfa_state_map.find(next_subset);

            if (dfa_it == dfa_state_map.end()) {
                target_dfa_index =
                    states_with_actions.makeNew();

                dfa_state_map.emplace(
                    next_subset,
                    target_dfa_index
                );

                Assert(
                    dfa_closures.size() ==
                        target_dfa_index,
                    "DFA::build: closure/state index mismatch"
                );

                dfa_closures.push_back(
                    std::move(next_closure)
                );

                work_queue.push(
                    target_dfa_index
                );
            }
            else {
                target_dfa_index =
                    dfa_it->second;
            }


            /*
             * ----------------------------------------------------------
             * Debug origins
             * ----------------------------------------------------------
             */
            std::vector<const NFA::IR::TokenID*> origins;
            origins.reserve(candidates.size());

            std::vector<std::optional<TNFA::CharOrigin>> char_origins_raw;
            char_origins_raw.reserve(candidates.size());

            for (const auto &candidate : candidates) {
                origins.push_back(candidate.origin);
                char_origins_raw.push_back(candidate.char_origin);
            }

            const TNFA::DebugOrigins debug_origins =
                collect_debug_origins(origins);

            auto char_origins =
                collect_char_origins(char_origins_raw);


            /*
             * ----------------------------------------------------------
             * Construct transition action sequence
             * ----------------------------------------------------------
             *
             * Closure owns action-path construction.
             *
             * next_closure.getTransitionActions() is the flat, in-
             * order list of every action chain actually fired while
             * building next_closure: the consuming edge's own
             * actions, PLUS every epsilon-edge action reachable after
             * it (nested ActionState / SemanticState included). Each
             * edge in that DFS is traversed at most once, so this is
             * already exactly-once per firing action - no manual
             * per-target lookup, and no manual dedup needed.
             *
             * In particular:
             *
             *   - do not use candidate.target to look up actions;
             *     candidate.target is the raw, pre-epsilon consuming
             *     destination and getActionsForState() on it returns
             *     only the bare consuming-edge action, dropping every
             *     epsilon-chained action after it
             *   - do not reorder
             */



            /*
             * ----------------------------------------------------------
             * Emit transition
             * ----------------------------------------------------------
             */

            Assert(
                target_dfa_index != TNFA::NULL_STATE,
                "DFA::build: NULL DFA target for consuming transition"
            );

            states_with_actions[current_dfa_index]
                .transitions[symbol] =
                    make_action_sequence(
                        actions,
                        target_dfa_index,
                        debug_origins,
                        std::move(char_origins)
                    );
        }
    }
    utype::unordered_set<RawAction> seen;
    for (const auto &state : states_with_actions) {
        for (const auto [i, next] : state.transitions) {
            if (std::holds_alternative<ActionSequence>(next)) {
                auto seq = std::get<ActionSequence>(next);
                for (const auto &act : seq.actions) {
                    if (!seen.contains(act)) {
                        seen.insert(act);
                    }
                }
            }
        }
    }
    std::cout << "Actions: " << seen.size() << " = " << seen << std::endl;
    /*
     * ------------------------------------------------------------------
     * Validate DFA
     * ------------------------------------------------------------------
     */

    for (std::size_t dfa_index = 0;
         dfa_index < states_with_actions.size();
         ++dfa_index) {

        const auto &state =
            states_with_actions.get().at(dfa_index);

        for (const auto &[symbol, transition] :
             state.transitions) {

            std::visit(
                [&](const auto &target) {

                    using T =
                        std::decay_t<decltype(target)>;

                    if constexpr (
                        std::is_same_v<
                            T,
                            TNFA::DFATarget
                        >
                    ) {
                        Assert(
                            target.id != TNFA::NULL_STATE,
                            "DFA::build: NULL DFA target"
                        );

                        Assert(
                            target.id <
                                states_with_actions.size(),
                            "DFA::build: DFA target out of range"
                        );
                    }
                    else if constexpr (
                        std::is_same_v<
                            T,
                            ActionSequence
                        >
                    ) {
                        /*
                         * A consuming ActionSequence must always have
                         * a DFA target.
                         */
                        Assert(
                            target.terminal_dfa_target !=
                                TNFA::NULL_STATE,
                            "DFA::build: consuming ActionSequence "
                            "has NULL target"
                        );

                        Assert(
                            target.terminal_dfa_target <
                                states_with_actions.size(),
                            "DFA::build: ActionSequence target "
                            "out of range"
                        );
                    }

                },
                transition
            );
        }


        /*
         * accept_action is different:
         *
         * it executes after the token has been recognized and therefore
         * intentionally has no DFA continuation.
         */
        if (state.accept_action.has_value()) {
            const auto &accept =
                *state.accept_action;

            Assert(
                accept.terminal_dfa_target ==
                    TNFA::NULL_STATE,
                "DFA::build: accepting action has DFA target"
            );
        }
    }


    return states_with_actions;
}
void DFA::optimizeRegistersAndLRTable() {
  std::cout << "optimizeRegistersAndLRTable: action_table.size(): "
            << action_table.size() << std::endl;
  std::vector<bool> used(action_table.size(), false);

  auto mark_action = [&](auto self, std::size_t idx) -> void {
    if (idx >= action_table.size() || used[idx])
      return;
    used[idx] = true;

    if (std::holds_alternative<NFA::ActionTarget>(
            action_table[idx].next_state)) {
      self(self, std::get<NFA::ActionTarget>(action_table[idx].next_state).id);
    }
  };

  for (const auto &state : states) {
    if (state.accept_binding &&
        state.accept_binding->reduce_rule_id.has_value()) {
      mark_action(mark_action, *state.accept_binding->reduce_rule_id);
    }
    for (const auto &[symbol, target] : state.transitions) {
      if (std::holds_alternative<NFA::ActionTarget>(target)) {
        mark_action(mark_action, std::get<NFA::ActionTarget>(target).id);
      }
    }
  }

  for (const auto &sem : semantic_table) {
    if (std::holds_alternative<NFA::ActionTarget>(sem.next_state)) {
      mark_action(mark_action, std::get<NFA::ActionTarget>(sem.next_state).id);
    }
  }

  auto dedup_hash = [](const NFA::ActionState &entry) -> std::size_t {
    std::size_t hash = 0;
    auto combine = [](std::size_t &seed, std::size_t value) {
      seed ^= value + static_cast<std::size_t>(0x9e3779b9) + (seed << 6) +
              (seed >> 2);
    };
    combine(hash, uhash{}(entry.action));
    combine(hash, uhash{}(entry.variable));
    return hash;
  };

  std::vector<std::size_t> canonical(action_table.size(), NFA::NULL_STATE);
  std::vector<NFA::ActionState> dedup_table;
  std::unordered_map<std::size_t, std::vector<std::size_t>> buckets;
  buckets.reserve(action_table.size() * 2);
  dedup_table.reserve(action_table.size());

  auto canonicalize = [&](auto self, std::size_t idx) -> std::size_t {
    if (canonical[idx] != NFA::NULL_STATE)
      return canonical[idx];

    NFA::ActionState entry = action_table[idx];

    if (std::holds_alternative<NFA::ActionTarget>(entry.next_state)) {
      auto &t = std::get<NFA::ActionTarget>(entry.next_state);
      t.id = self(self, t.id);
    }

    const std::size_t key = dedup_hash(entry);
    auto &bucket = buckets[key];
    for (std::size_t j : bucket) {
      if (dedup_table[j].action == entry.action &&
          dedup_table[j].variable == entry.variable &&
          dedup_table[j].next_state == entry.next_state) {
        return canonical[idx] = j;
      }
    }

    const std::size_t new_idx = dedup_table.size();
    dedup_table.push_back(std::move(entry));
    bucket.push_back(new_idx);
    return canonical[idx] = new_idx;
  };

  for (std::size_t i = 0; i < action_table.size(); ++i)
    if (used[i])
      canonicalize(canonicalize, i);

  action_table = std::move(dedup_table);

  // Single remap sweep, using `canonical` (only valid for indices that were
  // `used`).
  // Semantic entries can chain into the action table ([semantic, END, ...]);
  // their ActionTarget ids must follow the same compaction.
  for (auto &sem : semantic_table) {
    if (std::holds_alternative<NFA::ActionTarget>(sem.next_state)) {
      auto &t = std::get<NFA::ActionTarget>(sem.next_state);
      if (t.id < canonical.size() && canonical[t.id] != NFA::NULL_STATE)
        t.id = canonical[t.id];
    }
  }
  for (auto &state : states) {
    if (state.accept_binding && state.accept_binding->reduce_rule_id) {
      auto &id = *state.accept_binding->reduce_rule_id;
      if (id < canonical.size() && canonical[id] != NFA::NULL_STATE) {
        id = canonical[id];
      }
    }
    for (auto &[symbol, target] : state.transitions) {
      if (std::holds_alternative<NFA::ActionTarget>(target)) {
        auto &act = std::get<NFA::ActionTarget>(target);
        if (act.id < canonical.size() && canonical[act.id] != NFA::NULL_STATE) {
          act.id = canonical[act.id];
        }
      }
    }
  }
}
void DFA::optimizeSemanticTable() {
  std::vector<bool> used(semantic_table.size(), false);

  auto mark_semantic = [&](std::size_t idx) {
    if (idx < used.size())
      used[idx] = true;
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
      if (act_id < action_table.size()) {
        // RECURSE through action chains to reach downstream semantic states!
        self(self, action_table[act_id].next_state);
      }
    }
  };

  for (const auto &state : states) {
    if (state.accept_binding && state.accept_binding->target_semantic_state) {
      // Implicit conversion from NFA::SemanticTarget to NextTarget now works
      resolve_target(
          resolve_target,
          NFA::SemanticTarget{*state.accept_binding->target_semantic_state});
    }

    for (const auto &[symbol, target] : state.transitions) {
      resolve_target(resolve_target, target);
    }
  }

  for (const auto &state : action_table) {
    resolve_target(resolve_target, state.next_state);
  }

  std::unordered_map<std::size_t, std::size_t> remap;
  std::vector<NFA::SemanticState> compacted;

  // 2. Compact and deduplicate used entries
  for (std::size_t i = 0; i < semantic_table.size(); ++i) {
    if (!used[i])
      continue;
    const auto &entry = semantic_table[i];
    std::size_t canonical_idx = NFA::NULL_STATE;

    for (std::size_t j = 0; j < compacted.size(); ++j) {
      if (compacted[j].next_state == entry.next_state &&
          compacted[j].statements == entry.statements &&
          compacted[j].instance_value == entry.instance_value &&
          compacted[j].nfa_index == entry.nfa_index) {
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
      // FIX 3: Reset unmapped target to NULL_STATE instead of leaving stale
      // index
      sem.id = NFA::NULL_STATE;
    }
  };

  // 3. Remap all internal and external references
  for (auto &entry : compacted) {
    if (std::holds_alternative<NFA::SemanticTarget>(entry.next_state)) {
      update_sem_target(std::get<NFA::SemanticTarget>(entry.next_state));
    }
  }

  for (auto &act : action_table) {
    if (std::holds_alternative<NFA::SemanticTarget>(act.next_state)) {
      update_sem_target(std::get<NFA::SemanticTarget>(act.next_state));
    }
  }

  for (auto &state : states) {
    if (state.accept_binding && state.accept_binding->target_semantic_state) {
      if (auto it = remap.find(*state.accept_binding->target_semantic_state);
          it != remap.end()) {
        state.accept_binding->target_semantic_state = it->second;
      } else {
        state.accept_binding->target_semantic_state.reset();
      }
    }

    for (auto &[symbol, target] : state.transitions) {
      if (std::holds_alternative<NFA::SemanticTarget>(target)) {
        update_sem_target(std::get<NFA::SemanticTarget>(target));
      }
    }
  }

  semantic_table = std::move(compacted);
}

auto DFA::sameAcceptBinding(const State<> &a, const State<> &b) -> bool {
  if (a.accept_binding.has_value() != b.accept_binding.has_value())
    return false;

  if (!a.accept_binding)
    return true;

  const auto &lhs = *a.accept_binding;
  const auto &rhs = *b.accept_binding;

  return lhs.token_id == rhs.token_id &&
         lhs.is_unique_representation == rhs.is_unique_representation &&
         lhs.reduce_rule_id == rhs.reduce_rule_id &&
         lhs.target_semantic_state == rhs.target_semantic_state;
}

auto DFA::initialClass(const StateWithActions &s) -> std::size_t {
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

auto DFA::minimize() -> States<State<>> {
  Tlog::Branch b(logger, "DFA/minimize.log");

  // ================================================================
  // IMPORTANT ARCHITECTURE
  //
  // states_with_actions contains the intermediate DFA produced by
  // build().
  //
  // It contains:
  //
  //     DFA targets
  //     ordered ActionSequence objects
  //
  // It does NOT contain ActionTable/SemanticTable indices.
  //
  // Therefore minimization operates entirely on semantic structure,
  // and table materialization happens only once, after minimization.
  // ================================================================

  const auto &input = states_with_actions;
  const std::size_t n = input.size();

  States<State<>> empty_result(&nfa);

  if (n == 0) {
    states.clear();
    action_table.clear();
    semantic_table.clear();
    return empty_result;
  }
  auto extract_debug_origins = [&](const NFA::TNFA::DebugOrigins &debug) {
      return debug.empty() ? NFA::IR::TokenID {} : debug.back();
  };
  // ================================================================
  // Helper: hash one raw action.
  //
  // This hashes the actual action contents, NOT an eventual table
  // index. This is important because table indices don't exist yet.
  // ================================================================

  auto hash_raw_action = [&](const NFA::ActionState &action,
                             std::size_t &hash) {
    hash_combine(hash, uhash{}(action.action));

    hash_combine(hash, uhash{}(action.variable));

    // Do NOT hash action.next_state here.
    //
    // The intermediate action sequence owns the control flow.
    // The next action is simply the next element of the vector.
  };

  auto hash_raw_semantic = [&](const NFA::SemanticState &semantic,
                               std::size_t &hash) {
    hash_combine(hash, uhash{}(semantic.instance_value));

    hash_combine(hash, uhash{}(semantic.statements));

    // next_state is deliberately not used here.
    //
    // In the new architecture, sequence ordering represents
    // the chain and terminal_dfa_target represents its final
    // destination.
  };

  // ================================================================
  // Helper: produce a deterministic hash for an ActionSequence.
  //
  // ORDER MATTERS.
  //
  // [BEGIN, PUSH, END]
  //
  // is different from:
  //
  // [BEGIN, END, PUSH]
  //
  // even if they contain exactly the same individual actions.
  // ================================================================

  auto hash_action_sequence =
      [&](const ActionSequence &sequence) -> std::size_t {
    std::size_t hash = 0;

    // Include sequence length so that:
    //
    // [A, B]
    //
    // does not accidentally behave like:
    //
    // [A, B, ...]
    //
    hash_combine(hash, uhash{}(sequence.actions.size()));

    for (const auto &raw : sequence.actions) {
      std::visit(
          [&](const auto &action) {
            using T = std::decay_t<decltype(action)>;

            if constexpr (std::is_same_v<T, NFA::ActionState>) {
              // Distinguish ActionState from
              // SemanticState even if their contents
              // happen to hash similarly.
              hash_combine(hash, std::size_t{0});

              hash_raw_action(action, hash);
            } else {
              hash_combine(hash, std::size_t{1});

              hash_raw_semantic(action, hash);
            }
          },
          raw);
    }

    return hash;
  };

  // ================================================================
  // Initial state class.
  //
  // This must describe properties intrinsic to the state itself.
  // Transition destinations are refined later.
  // ================================================================

  auto state_base_hash = [&](const StateWithActions &state) -> std::size_t {
    std::size_t hash = 0;

    // Preserve the old initialClass semantics for accepting
    // information.
    //
    // We deliberately still use initialClass here because it
    // already contains the token/acceptance semantics that
    // define an initial partition.
    hash_combine(hash, initialClass(state));
    hash_combine(hash, std::size_t{0});

    // --------------------------------------------------------
    // Accept action sequence is also intrinsic state behavior.
    // --------------------------------------------------------

    if (state.accept_action.has_value()) {
      hash_combine(hash, std::size_t{1});
      hash_combine(hash, hash_action_sequence(*state.accept_action));
    } else {
      hash_combine(hash, std::size_t{0});
    }

    return hash;
  };

  // ================================================================
  // Helper: construct the refinement key of one transition.
  //
  // This replaces the old refinementKey() entirely.
  //
  // IMPORTANT:
  //     ActionSequence is compared in its original order.
  //
  // The target DFA state is represented by its CURRENT partition.
  // ================================================================

  struct RefinedTransition {
    NFA::TransitionKey symbol;

    std::size_t kind = 0;
    // 0 = direct DFA transition
    // 1 = action sequence

    std::size_t action_hash = 0;
    std::vector<std::size_t> action_identity;

    std::size_t target_partition = NFA::NULL_STATE;
    // Debug provenance carried straight from the TNFA target (TokenID) and
    // the per-character site (CharOrigin). This is compared UNCONDITIONALLY:
    // when the TNFA was built without debug tracking these are empty/nullopt
    // everywhere, so including them in the key has no discriminating effect -
    // equivalent to branching on "is debug present". When present, two
    // otherwise-identical transitions that came from different grammar sites
    // are (correctly) kept distinct.
    NFA::DebugOrigins debug;
    // One entry per folded candidate, same shape as `debug` above -
    // a plain DFATarget's single char_origin is wrapped into a
    // one-element vector so both branches compare uniformly. Actually
    // populated below now (was previously hardcoded to empty/nullopt,
    // which meant char_origin never distinguished two otherwise-
    // identical transitions during partition refinement).
    std::vector<std::optional<NFA::TNFA::CharOrigin>> origin;
    bool operator<(const RefinedTransition &other) const {
      return std::tie(symbol, kind, action_hash, action_identity,
                      target_partition, debug, origin) <
             std::tie(other.symbol, other.kind, other.action_hash,
                      other.action_identity, other.target_partition,
                      other.debug, other.origin);
    }

    bool operator==(const RefinedTransition &other) const {
      return symbol == other.symbol && kind == other.kind &&
             action_hash == other.action_hash &&
             action_identity == other.action_identity &&
             target_partition == other.target_partition &&
             debug == other.debug &&
             origin == other.origin;
    }
  };

  auto make_transition_key =
      [&](const StateWithActions &state,
          const std::unordered_map<std::size_t, std::size_t> &partition_of)
      -> std::vector<RefinedTransition> {
    std::vector<RefinedTransition> key;

    key.reserve(state.transitions.size());

    for (const auto &[symbol, target] : state.transitions) {

      std::visit(
          [&](const auto &arg) {
            using T = std::decay_t<decltype(arg)>;

            // ------------------------------------------------
            // Direct DFA transition
            // ------------------------------------------------

            if constexpr (std::is_same_v<T, NFA::DFATarget>) {
              const std::size_t target_partition =
                  arg.id == NFA::NULL_STATE ? NFA::NULL_STATE
                                            : partition_of.at(arg.id);

              // A plain DFATarget only ever carries a single debug
              // origin (arg.debug). Wrap it into a DebugOrigins of one
              // element so it compares uniformly with the ActionSequence
              // branch below: when TNFA was built without debug
              // tracking, arg.debug is a default/empty TokenID on every
              // transition, so this element is identical everywhere and
              // has no discriminating effect - equivalent to leaving it
              // out entirely. When debug tracking is on, transitions
              // reached from different grammar sites are (correctly)
              // kept in separate partitions instead of being merged.
              NFA::DebugOrigins single_origin;
              single_origin.push_back(arg.debug);

              key.push_back(
                  RefinedTransition{.symbol = symbol,
                                    .kind = 0,
                                    .action_hash = 0,
                                    .action_identity = {},
                                    .target_partition = target_partition,
                                    .debug = std::move(single_origin),
                                    .origin = {arg.char_origin}});
            }

            // ------------------------------------------------
            // Ordered action sequence
            // ------------------------------------------------

            else if constexpr (std::is_same_v<T, ActionSequence>) {
              std::vector<std::size_t> action_identity;

              action_identity.reserve(arg.actions.size() * 3);

              std::size_t action_hash = 0;

              hash_combine(action_hash, uhash{}(arg.actions.size()));

              for (const auto &raw : arg.actions) {

                std::visit(
                    [&](const auto &action) {
                      using A = std::decay_t<decltype(action)>;

                      if constexpr (std::is_same_v<A, NFA::ActionState>) {
                        // Type marker.
                        action_identity.push_back(0);

                        hash_combine(action_hash, std::size_t{0});

                        hash_raw_action(action, action_hash);

                        // Keep an explicit
                        // structural identity as
                        // well as the hash.
                        action_identity.push_back(uhash{}(action.action));

                        action_identity.push_back(uhash{}(action.variable));
                      } else {
                        // Type marker.
                        action_identity.push_back(1);

                        hash_combine(action_hash, std::size_t{1});

                        hash_raw_semantic(action, action_hash);

                        action_identity.push_back(
                            uhash{}(action.instance_value));

                        action_identity.push_back(uhash{}(action.statements));
                      }
                    },
                    raw);
              }

              const std::size_t target_partition =
                  arg.terminal_dfa_target == NFA::NULL_STATE
                      ? NFA::NULL_STATE
                      : partition_of.at(arg.terminal_dfa_target);

              // arg.debug is already a full DebugOrigins list here (one
              // origin per NFA transition candidate that fed this
              // ActionSequence) - carry it straight through, same
              // unconditional-by-emptiness reasoning as the DFATarget
              // branch above: empty everywhere when debug tracking is
              // off, so it changes nothing in that mode.
              key.push_back(RefinedTransition{
                  .symbol = symbol,
                  .kind = 1,
                  .action_hash = action_hash,
                  .action_identity = std::move(action_identity),
                  .target_partition = target_partition,
                  .debug = arg.debug,
                  .origin = std::vector<std::optional<NFA::TNFA::CharOrigin>>(
                      arg.char_origin.begin(), arg.char_origin.end())});
            }
          },
          target);
    }

    std::sort(key.begin(), key.end());

    return key;
  };

  // ================================================================
  // 1. Initial partition
  // ================================================================

  std::unordered_map<std::size_t, std::size_t> partition_of;

  std::map<std::size_t, std::size_t> initial_hash_to_class;

  std::size_t class_count = 0;

  for (std::size_t i = 0; i < n; ++i) {
    const std::size_t hash = state_base_hash(input[i]);

    auto [it, inserted] = initial_hash_to_class.emplace(hash, class_count);

    if (inserted)
      ++class_count;

    partition_of[i] = it->second;
  }

  // ================================================================
  // 2. Partition refinement
  // ================================================================

  while (true) {
    std::unordered_map<std::size_t, std::size_t> new_partition;

    std::map<std::pair<std::size_t, std::vector<RefinedTransition>>,
             std::size_t>
        signature_to_class;

    std::size_t new_class_count = 0;

    for (std::size_t i = 0; i < n; ++i) {
      const std::size_t base_hash = state_base_hash(input[i]);

      const auto transition_key = make_transition_key(input[i], partition_of);

      const auto signature = std::make_pair(base_hash, transition_key);

      auto [it, inserted] =
          signature_to_class.emplace(signature, new_class_count);

      if (inserted)
        ++new_class_count;

      new_partition[i] = it->second;
    }

    // ------------------------------------------------------------
    // IMPORTANT:
    //
    // Do not compare class numbers directly to determine whether
    // refinement changed. Class numbering can change even when the
    // actual partition is identical.
    //
    // Compare equivalence relations instead.
    // ------------------------------------------------------------

    bool changed = false;

    if (new_partition.size() != partition_of.size()) {
      changed = true;
    } else {
      for (std::size_t i = 0; i < n && !changed; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {

          const bool old_same = partition_of.at(i) == partition_of.at(j);

          const bool new_same = new_partition.at(i) == new_partition.at(j);

          if (old_same != new_same) {
            changed = true;
            break;
          }
        }
      }
    }

    partition_of = std::move(new_partition);

    if (!changed)
      break;
  }

  // ================================================================
  // 3. Create one intermediate state per final partition.
  // ================================================================

  std::map<std::size_t, std::size_t> class_to_new_index;

  States<StateWithActions> minimized(&nfa);

  // Force class numbering into deterministic state numbering by
  // traversing original states in order.
  for (std::size_t i = 0; i < n; ++i) {
    const std::size_t cls = partition_of.at(i);

    if (!class_to_new_index.contains(cls)) {
      class_to_new_index.emplace(cls, minimized.makeNew());
    }
  }

  // ================================================================
  // 4. Copy minimized state contents.
  //
  // Since all states in one partition have identical refinement
  // signatures, taking the first state is valid.
  // ================================================================

  std::unordered_set<std::size_t> constructed_classes;

  for (std::size_t i = 0; i < n; ++i) {
    const std::size_t cls = partition_of.at(i);

    if (!constructed_classes.insert(cls).second)
      continue;

    const std::size_t new_idx = class_to_new_index.at(cls);

    const auto &source = input[i];

    auto &destination = minimized[new_idx];

    // ------------------------------------------------------------
    // These remain completely unmaterialized.
    // ------------------------------------------------------------

    destination.accept_binding = source.accept_binding;

    destination.accept_action = source.accept_action;

    // ------------------------------------------------------------
    // Transitions
    // ------------------------------------------------------------

    for (const auto &[symbol, target] : source.transitions) {

      std::visit(
          [&](const auto &arg) {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, NFA::DFATarget>) {
              if (arg.id == NFA::NULL_STATE) {
                destination.transitions[symbol] =
                    NFA::DFATarget{.id = NFA::NULL_STATE, .debug = arg.debug, .char_origin = arg.char_origin};
                return;
              }

              const std::size_t target_class = partition_of.at(arg.id);

              destination.transitions[symbol] =
                  NFA::DFATarget{.id = class_to_new_index.at(target_class), .debug = arg.debug, .char_origin = arg.char_origin};
            } else if constexpr (std::is_same_v<T, ActionSequence>) {
              ActionSequence sequence = arg;

              if (sequence.terminal_dfa_target != NFA::NULL_STATE) {

                const std::size_t target_class =
                    partition_of.at(sequence.terminal_dfa_target);

                sequence.terminal_dfa_target =
                    class_to_new_index.at(target_class);
              }

              destination.transitions[symbol] = std::move(sequence);
            }
          },
          target);
    }
  }

  // ================================================================
  // 5. Reachability on the MINIMIZED intermediate DFA.
  //
  // There is no ActionTable/SemanticTable to walk anymore.
  //
  // ActionSequence already directly exposes its terminal DFA state.
  // ================================================================

  std::vector<bool> reachable(minimized.size(), false);

  std::queue<std::size_t> q;

  if (!minimized.empty()) {
    reachable[0] = true;
    q.push(0);
  }

  while (!q.empty()) {
    const std::size_t current = q.front();

    q.pop();

    const auto &state = minimized[current];

    for (const auto &[symbol, target] : state.transitions) {

      std::size_t next_state = NFA::NULL_STATE;

      std::visit(
          [&](const auto &arg) {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, NFA::DFATarget>) {
              next_state = arg.id;
            } else if constexpr (std::is_same_v<T, ActionSequence>) {
              next_state = arg.terminal_dfa_target;
            }
          },
          target);

      if (next_state == NFA::NULL_STATE)
        continue;

      Assert(next_state < minimized.size(),
             "Minimized DFA state {} has invalid "
             "transition target {}",
             current, next_state);

      if (!reachable[next_state]) {
        reachable[next_state] = true;
        q.push(next_state);
      }
    }
  }

  // ================================================================
  // 6. Compact reachable intermediate DFA.
  // ================================================================

  States<StateWithActions> compact(&nfa);

  std::vector<std::size_t> state_remap(minimized.size(), NFA::NULL_STATE);

  for (std::size_t i = 0; i < minimized.size(); ++i) {

    if (reachable[i]) {
      state_remap[i] = compact.makeNew();
    }
  }

  for (std::size_t i = 0; i < minimized.size(); ++i) {

    if (!reachable[i])
      continue;

    const std::size_t new_idx = state_remap[i];

    const auto &source = minimized[i];

    auto &destination = compact[new_idx];

    destination.accept_binding = source.accept_binding;

    destination.accept_action = source.accept_action;

    for (const auto &[symbol, target] : source.transitions) {

      std::visit(
          [&](const auto &arg) {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, NFA::DFATarget>) {
              if (arg.id == NFA::NULL_STATE)
                return;

              Assert(arg.id < state_remap.size(), "DFA target {} outside remap",
                     arg.id);

              const auto mapped = state_remap[arg.id];

              if (mapped != NFA::NULL_STATE) {
                destination.transitions[symbol] = NFA::DFATarget{.id = mapped, .debug = arg.debug, .char_origin = arg.char_origin};
              }
            } else if constexpr (std::is_same_v<T, ActionSequence>) {
              ActionSequence sequence = arg;

              if (sequence.terminal_dfa_target != NFA::NULL_STATE) {

                Assert(sequence.terminal_dfa_target < state_remap.size(),
                       "ActionSequence terminal "
                       "target {} outside remap",
                       sequence.terminal_dfa_target);

                const auto mapped = state_remap[sequence.terminal_dfa_target];

                if (mapped != NFA::NULL_STATE) {
                  sequence.terminal_dfa_target = mapped;

                  destination.transitions[symbol] = std::move(sequence);
                }
              } else {
                // Accept/terminal sequences are allowed
                // to terminate in NULL_STATE.
                destination.transitions[symbol] = std::move(sequence);
              }
            }
          },
          target);
    }
  }

  // ================================================================
  // At this point:
  //
  //     compact
  //
  // is the FINAL minimized DFA structure, but it still contains
  // ActionSequence objects.
  //
  // Only NOW do we construct ActionTable/SemanticTable.
  // ================================================================

  states.clear();
  action_table.clear();
  semantic_table.clear();
  // ================================================================
  // Materialization helpers
  // ================================================================

  auto materialize_sequence =
      [&](const ActionSequence &sequence) -> TransitionTarget {
    Assert(!sequence.actions.empty(),
           "Attempting to materialize an empty action sequence");

    struct Slot {
      NFA::TableType table_type;
      std::size_t index;
    };

    std::vector<Slot> slots;
    slots.reserve(sequence.actions.size());

    // ------------------------------------------------------------
    // First allocate every table entry.
    //
    // This is done before wiring next_state so chains can freely
    // cross Action <-> Semantic boundaries.
    // ------------------------------------------------------------

    for (const auto &raw : sequence.actions) {

      std::visit(
          [&](const auto &entry) {
            using T = std::decay_t<decltype(entry)>;

            if constexpr (std::is_same_v<T, NFA::ActionState>) {
              // One table row per action-sequence entry.
              action_table.push_back(entry);

              slots.push_back(
                  Slot{NFA::TableType::Action, action_table.size() - 1});
            } else {
              semantic_table.push_back(entry);

              slots.push_back(
                  Slot{NFA::TableType::Semantic, semantic_table.size() - 1});
            }
          },
          raw);
    }

    // ------------------------------------------------------------
    // Then wire the chain.
    //
    // The order is exactly sequence.actions order.
    // ------------------------------------------------------------

    for (std::size_t i = 0; i < slots.size(); ++i) {

      TransitionTarget next;

      if (i + 1 < slots.size()) {
        const auto &next_slot = slots[i + 1];

        if (next_slot.table_type == NFA::TableType::Action) {

          next = NFA::ActionTarget{next_slot.index};
        } else {
          next = NFA::SemanticTarget{next_slot.index};
        }
      } else {
        next = NFA::DFATarget{
            .id = sequence.terminal_dfa_target,
            .debug = extract_debug_origins(sequence.debug),
            .char_origin = sequence.char_origin.empty()
                ? std::nullopt
                : sequence.char_origin.back()
        };
      }

      const auto &slot = slots[i];

      if (slot.table_type == NFA::TableType::Action) {

        action_table[slot.index].next_state = std::move(next);
      } else {
        semantic_table[slot.index].next_state = std::move(next);
      }
    }

    // ------------------------------------------------------------
    // Return the first table target.
    // ------------------------------------------------------------

    // The head target IS this DFA transition, as far as a debugger is
    // concerned - attach the transition's merged grammar provenance here
    // (the individual table rows themselves record no SourceLink of
    // their own; ActionState/SemanticState only carry a free-text
    // debug_note).
    if (slots[0].table_type == NFA::TableType::Action) {

      return NFA::ActionTarget{.id = slots[0].index, .debug = extract_debug_origins(sequence.debug)};
    }

    return NFA::SemanticTarget{.id = slots[0].index, .debug = extract_debug_origins(sequence.debug)};
  };

  // ================================================================
  // Materialize one NextTarget.
  // ================================================================

  auto materialize_target =
      [&](const NextTarget &target) -> std::optional<TransitionTarget> {
    return std::visit(
        [&](const auto &arg) -> std::optional<TransitionTarget> {
          using T = std::decay_t<decltype(arg)>;

          if constexpr (std::is_same_v<T, NFA::DFATarget>) {
            return TransitionTarget{arg};
          } else {
            return materialize_sequence(arg);
          }
        },
        target);
  };

  // ================================================================
  // Materialize `compact` (still StateWithActions, i.e. still
  // carrying ActionSequence objects) into the final `states`
  // (State<>, carrying only DFATarget/ActionTarget/SemanticTarget).
  //
  // One destination state is created per source state, in the same
  // order, so compact's indices and states' indices coincide 1:1 —
  // that identity is what lets transitions below just copy/remap
  // DFATarget ids unchanged.
  //
  // Everything intrinsic to the state (nfa_states, accept_binding,
  // entry action, accept action, transitions) is materialized in a
  // single pass per state, so no state is ever synthesized twice
  // and nothing is silently dropped.
  // ================================================================

  auto materialize_accept_action =
      [&](std::optional<NFA::TokenBinding> &accept_binding,
          const ActionSequence &sequence) {
        auto materialized = materialize_sequence(sequence);

        if (std::holds_alternative<NFA::ActionTarget>(materialized)) {
          const auto target = std::get<NFA::ActionTarget>(materialized);
          accept_binding->reduce_rule_id = target.id;
          accept_binding->target_semantic_state = std::nullopt;
        } else if (std::holds_alternative<NFA::SemanticTarget>(materialized)) {
          const auto target = std::get<NFA::SemanticTarget>(materialized);
          accept_binding->target_semantic_state = target.id;
          accept_binding->reduce_rule_id = std::nullopt;
        } else {
          // An accepting action sequence must contain at least one
          // action (see the Assert inside materialize_sequence), so
          // it can never resolve to a bare DFATarget.
          Assert(false, "Accept action sequence materialized to a direct "
                        "DFA target instead of an Action/Semantic target");
        }
      };

  for (std::size_t i = 0; i < compact.size(); ++i)
    states.makeNew();

  for (std::size_t i = 0; i < compact.size(); ++i) {
    const auto &source = compact[i];
    auto &destination = states[i];

    destination.nfa_states = source.nfa_states;
    destination.accept_binding = source.accept_binding;

    // ------------------------------------------------------------
    // Accept action
    //
    // Materialized into accept_binding's reduce_rule_id /
    // target_semantic_state; State<>::accept_action is left empty
    // since the runtime resolves accepting behavior through
    // accept_binding alone from this point on.
    // ------------------------------------------------------------
    if (source.accept_action.has_value()) {
      Assert(destination.accept_binding.has_value(),
             "DFA state {} has an accept action sequence but no "
             "accept binding to attach it to",
             i);

      materialize_accept_action(destination.accept_binding,
                                *source.accept_action);
    }

    // ------------------------------------------------------------
    // Transitions
    // ------------------------------------------------------------
    for (const auto &[symbol, target] : source.transitions) {
      if (std::holds_alternative<NFA::DFATarget>(target)) {
        destination.transitions[symbol] = std::get<NFA::DFATarget>(target);
        continue;
      }

      destination.transitions[symbol] =
          materialize_sequence(std::get<ActionSequence>(target));
    }
  }

  // ================================================================
  // Table compaction.
  //
  // materialize_sequence() above allocates a fresh action_table /
  // semantic_table entry per action, even when the same chain of
  // actions is reachable from several states. optimizeRegistersAndLRTable()
  // and optimizeSemanticTable() dedupe those tables (and, for the
  // action table, iterate to fold identical tails together) and
  // remap every reference (states, action_table, semantic_table) to
  // point at the surviving entries. Order within a single chain is
  // untouched — only structurally identical chains collapse.
  // ================================================================

  optimizeRegistersAndLRTable();
  optimizeSemanticTable();

  // ================================================================
  // Final validation.
  //
  // Now table indices DO exist, so these checks are meaningful.
  // ================================================================

  for (std::size_t i = 0; i < states.size(); ++i) {

    const auto &state = states[i];

    for (const auto &[symbol, target] : state.transitions) {

      std::visit(
          [&](const auto &arg) {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, NFA::DFATarget>) {
              Assert(arg.id == NFA::NULL_STATE || arg.id < states.size(),
                     "DFA state {} transition "
                     "has invalid DFA target {}",
                     i, arg.id);
            } else if constexpr (std::is_same_v<T, NFA::ActionTarget>) {
              Assert(arg.id < action_table.size(),
                     "DFA state {} transition "
                     "has invalid ActionTable index {}",
                     i, arg.id);
            } else if constexpr (std::is_same_v<T, NFA::SemanticTarget>) {
              Assert(arg.id < semantic_table.size(),
                     "DFA state {} transition "
                     "has invalid SemanticTable index {}",
                     i, arg.id);
            }
          },
          target);
    }
  }

  return states;
}

auto DFA::classify() -> ClassifiedDFA {
    constexpr std::size_t ALPHABET_SIZE = 256;
    const std::size_t n = states.size();

    // Store the full TransitionTarget in the signature so that transitions
    // with different debug info or char_origin get unique character classes.
    using Signature = std::vector<TransitionTarget>;

    std::unordered_map<Signature, std::size_t, uhash> class_of_signature;
    CharClassTable table;

    // Helper to extract default/accepting transitions along with debug metadata
    auto get_default_trans = [](const auto &state) -> TransitionTarget {
        NFA::IR::TokenID debug_id{};
        if (state.accept_action.has_value() && !state.accept_action->debug.empty()) {
            debug_id = state.accept_action->debug.front();
        } else if (!state.debug.empty()) {
            debug_id = state.debug.front();
        }

        if (state.accept_binding.has_value()) {
            const auto &binding = *state.accept_binding;
            if (binding.reduce_rule_id.has_value()) {
                return NFA::ActionTarget{
                    .id = *binding.reduce_rule_id,
                    .debug = debug_id,
                    .char_origin = std::nullopt
                };
            } else if (binding.target_semantic_state.has_value()) {
                return NFA::SemanticTarget{
                    .id = *binding.target_semantic_state,
                    .debug = debug_id,
                    .char_origin = std::nullopt
                };
            } else {
                return NFA::DFATarget{
                    .id = binding.token_id,
                    .debug = debug_id,
                    .char_origin = std::nullopt
                };
            }
        }
        return NFA::DFATarget{
            .id = NULL_STATE,
            .debug = debug_id,
            .char_origin = std::nullopt
        };
    };

    // Precompute default transitions for missing keys across all states
    std::vector<TransitionTarget> default_transitions;
    default_transitions.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        default_transitions.push_back(get_default_trans(states[i]));
    }

    // 1. Build character classes including debug/char_origin signatures
    for (std::size_t c = 0; c < ALPHABET_SIZE; ++c) {
        Signature sig;
        sig.reserve(n);
        NFA::TransitionKey key{static_cast<char>(c)};

        for (std::size_t i = 0; i < n; ++i) {
            auto it = states[i].transitions.find(key);
            if (it == states[i].transitions.end()) {
                sig.push_back(default_transitions[i]);
            } else {
                sig.push_back(it->second);
            }
        }

        auto [it, inserted] = class_of_signature.try_emplace(sig, class_of_signature.size());
        table.char_to_class[c] = it->second;
    }
    table.num_classes = class_of_signature.size();

    // 2. Populate classified DFA output
    States<State<ClassTransitions>> output(&nfa);
    for (std::size_t i = 0; i < n; ++i) {
        auto new_idx = output.makeNew();
        output[new_idx].accept_binding = states[i].accept_binding;

        output[new_idx].transitions.assign(table.num_classes, default_transitions[i]);

        for (const auto &[symbol, value] : states[i].transitions) {
            unsigned char c = symbol;
            std::size_t cls = table.char_to_class[c];

            output[new_idx].transitions[cls] = value;
        }
    }

    return ClassifiedDFA{std::move(table), std::move(output)};
}

auto DFA::clear() -> void {
  states.clear();
  action_table.clear();
  semantic_table.clear();
}

auto DFA::check_dfa() -> void {
  std::size_t index = 0;
  try {
    for (const auto &state : states) {
      for (const auto &[sym, transitions] : state.transitions) {
        if (std::holds_alternative<NFA::DFATarget>(transitions)) {
          const auto next = std::get<NFA::DFATarget>(transitions).id;
          Assert(next == NFA::NULL_STATE || states.size() > next,
                 "Out of bound transition {} in state {}", next, index);
        } else if (std::holds_alternative<NFA::ActionTarget>(transitions)) {
          const auto act_idx = std::get<NFA::ActionTarget>(transitions).id;
          Assert(action_table.size() > act_idx,
                 "Out of bound action index {} in state {}", act_idx, index);
        } else if (std::holds_alternative<NFA::SemanticTarget>(transitions)) {
          const auto sem_idx = std::get<NFA::SemanticTarget>(transitions).id;
          Assert(semantic_table.size() > sem_idx,
                 "Out of bound semantic index {} in state {}", sem_idx, index);
        }
      }
      ++index;
    }
  } catch (Error &e) {
    std::cout << "[MDFA] Check Failed > " << e.what() << '\n';
    std::abort();
  }
}

auto operator<<(std::ostream &os, const DFA &dfa) -> std::ostream & {
  (void)dfa;
  os << "<DFA stream dump unavailable for current transition model>\n";
  return os;
}

auto operator<<(std::ostream &os, const ClassifiedDFA &dfa) -> std::ostream & {
  os << dfa.table.num_classes << " equivalence classes\n";
  for (std::size_t i = 0; i < dfa.table.num_classes; ++i) {
    os << "Class " << i << ": ";
    for (std::size_t j = 0; j < dfa.table.char_to_class.size(); ++j) {
      if (dfa.table.char_to_class.at(j) != i)
        continue;
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
      if (std::holds_alternative<NFA::DFATarget>(t)) {
        os << "Class " << cls << " -> " << "DFA "
           << std::get<NFA::DFATarget>(t).id << '\n';
      } else if (std::holds_alternative<NFA::ActionTarget>(t)) {
        os << "Class " << cls << " -> " << "Action "
           << std::get<NFA::ActionTarget>(t).id << '\n';
      } else if (std::holds_alternative<NFA::SemanticTarget>(t)) {
        os << "Class " << cls << " -> " << "Semantic "
           << std::get<NFA::SemanticTarget>(t).id << '\n';
      }
      os << '\n';
    }
    ++index;
  }
  return os;
}
} // namespace DFA