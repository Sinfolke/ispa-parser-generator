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

  // ================================================================
  // Intermediate DFA representation
  //
  // IMPORTANT:
  //   No ActionTable/SemanticTable indices are created here.
  //
  // A transition is either:
  //
  //     DFA state
  //
  // or:
  //
  //     ordered action sequence -> DFA state
  //
  // The sequence is kept intact until after minimization/classification.
  // ================================================================

  using RawAction = std::variant<NFA::ActionState, NFA::SemanticState>;

  using ActionSequence = ActionSequence;

  using NextTarget = std::variant<NFA::DFATarget, ActionSequence>;

  // Use the vector of NFA states as the unique identity of a DFA
  // state.
  //
  // std::map is deliberately used here:
  //   - deterministic
  //   - independent of Closure hashing/equality
  //   - guarantees that equivalent subsets are represented by one
  //     DFA state
  //
  // Earlier revisions also needed a second, richer key for DFA
  // states reached through a "divergent" transition -- one whose
  // action sequence had to be deferred and replayed at runtime
  // via a SNAPSHOT_APPLY / SNAPSHOT_APPLY_END pair, because two
  // NFA lineages disagreed on what actions applied. That case no
  // longer exists: DFA.closure::Closure resolves every epsilon
  // ambiguity by NFA transition priority AT CLOSURE TIME (see
  // Closure::epsilonClosure), so every NFA state in every subset
  // carries exactly one, already-decided tag path. A plain NFA
  // subset is sufficient identity again.
  std::map<std::vector<std::size_t>, std::size_t> dfa_state_map;

  std::queue<std::size_t> work_queue;

  // DFA state ID -> corresponding epsilon closure.
  std::vector<Closure> dfa_closures;

  states_with_actions.clear();

  // These tables MUST NOT be populated during subset construction.
  //
  // They will be materialized after minimization/classification.
  action_table.clear();
  semantic_table.clear();

  // ================================================================
  // Helper: construct an intermediate action sequence.
  //
  // This deliberately does NOT:
  //   - allocate ActionTable entries
  //   - allocate SemanticTable entries
  //   - create ActionTarget/SemanticTarget links
  //   - sort actions
  //   - deduplicate actions
  //
  // The exact order supplied by the NFA is preserved.
  // ================================================================

  auto make_action_sequence =
      [](const stdu::vector<RawAction> &actions,
         std::size_t terminal_dfa_target) -> NextTarget {
    if (actions.empty())
      return NFA::DFATarget{terminal_dfa_target};

    return ActionSequence{.actions = actions,
                          .terminal_dfa_target = terminal_dfa_target};
  };

  // ================================================================
  // Helper: two FiredActions are the SAME CAPTURE, regardless of
  // which physical NFA state/action-table entry owns them.
  //
  // Nested-token cloning (and, below, two NFA lineages that
  // reconverge onto one merged DFA transition) can each carry
  // their own physical copy of "the" BEGIN(x) / END(x) action.
  // Those are not two different captures -- they are the same
  // logical operation reached two different ways. This mirrors
  // `sameAction` in Closure.cpp exactly; kept as a separate
  // local copy here because it's used for a different purpose
  // (see append_actions below), not because the rule is
  // different.
  // ================================================================

  auto same_capture = [](const NFA::ActionState &a,
                         const NFA::ActionState &b) -> bool {
    return a.action == b.action && a.variable == b.variable;
  };

  // ================================================================
  // Helper: flatten a set of ALREADY priority-ordered tag paths
  // (highest priority first) into one RawAction sequence, firing
  // each distinct capture exactly once.
  //
  // Two kinds of duplication are collapsed here, both
  // deliberately, both by keeping the FIRST (i.e. highest
  // priority) occurrence and discarding the rest:
  //
  //   - physical duplicates: the identical (owner, table_type,
  //     table_index) FiredAction reachable via more than one
  //     NFA lineage that both happen to be live right now (e.g.
  //     a loop's "continue" and "exit" epsilon edges both having
  //     already passed through the same earlier action).
  //
  //   - semantic duplicates: two DIFFERENT physical actions that
  //     are the same logical capture (see same_capture above) --
  //     this is what happens when two NFA lineages that diverged
  //     earlier (different priorities, different physical clone)
  //     reconverge on one merged DFA transition. Exactly one of
  //     them should fire; priority order decides which, the same
  //     way epsilonClosure() already decides between competing
  //     epsilon routes within a single closure step. This
  //     extends that same rule across the character-consuming
  //     step where such lineages recombine.
  // ================================================================

  auto append_actions =
      [](const std::vector<ActionPath> &priority_ordered_paths) {
        stdu::vector<RawAction> out;

        for (const auto &path : priority_ordered_paths) {
          for (const auto &fa : path) {
            const auto &raw = fa.raw;

            if (std::holds_alternative<NFA::SemanticState>(raw)) {
              const auto &semantic =
                  std::get<NFA::SemanticState>(raw);

              const bool already_present =
                  std::ranges::any_of(
                      out,
                      [&](const RawAction &existing) {
                          if (!std::holds_alternative<
                                  NFA::SemanticState
                              >(existing)) {
                              return false;
                          }

                          return std::get<NFA::SemanticState>(
                              existing
                          ) == semantic;
                      }
                  );

              if (already_present)
                continue;
            }

            out.push_back(raw);
          }
        }

        return out;
  };

  // ================================================================
  // 1. Start state
  // ================================================================

  Closure start_closure(&nfa, std::vector<std::size_t>{0});

  std::vector<std::size_t> start_subset = start_closure.get();

  const std::size_t start_idx = states_with_actions.makeNew();

  dfa_state_map.emplace(start_subset, start_idx);
  dfa_closures.push_back(std::move(start_closure));

  work_queue.push(start_idx);

  // NOTE: state 0's own actions (e.g. a leading BEGIN) are NOT
  // seeded here as a special-cased entry_action. Closure::epsilonClosure
  // already folds a state's own actions into its committed tag
  // path, and the subset-construction loop below reads that same
  // path via getActionsForState() for every outgoing transition
  // and accept_action of the start state. Duplicating them here
  // would fire state 0's actions (e.g. BEGIN) twice: once from
  // this block, once from the closure-derived sequence below.

  // ================================================================
  // 2. Subset construction
  // ================================================================

  while (!work_queue.empty()) {
    const std::size_t current_dfa_index = work_queue.front();
    work_queue.pop();

    // Copy, not reference: dfa_closures.push_back() below (new
    // DFA states discovered from this one) can reallocate the
    // vector this element lives in.
    const Closure current_closure = dfa_closures.at(current_dfa_index);
    logger.log("State {};", current_dfa_index);
    for (const auto &nfa_index : current_closure.get()) {
      const auto &state = nfa.getStates().at(nfa_index);
    }
    const std::vector<std::size_t> &current_subset = current_closure.get();

    // ============================================================
    // A. Resolve accepting binding
    // ============================================================

    std::optional<NFA::TokenBinding> best_binding;
    std::size_t best_binding_nfa_index = NFA::NULL_STATE;
    for (const std::size_t nfa_index : current_subset) {
      const auto &nfa_state = nfa.getStates().at(nfa_index);

      auto accept_it = nfa.getAcceptMap().find(nfa_index);

      std::optional<NFA::TokenBinding> binding =
          accept_it != nfa.getAcceptMap().end()
              ? std::make_optional(accept_it->second)
              : nfa_state.accept_binding;

      if (!binding.has_value() || binding->token_id >= NFA::NESTED_REDUCE_ID_BASE)
        continue;
      const bool current_has_semantic =
          binding->target_semantic_state.has_value();

      const bool best_has_semantic =
          best_binding.has_value() &&
          best_binding->target_semantic_state.has_value();

      if (!best_binding.has_value() || (current_has_semantic && !best_has_semantic) || (current_has_semantic == best_has_semantic && binding->token_id < best_binding->token_id)) {
        std::cout << "Writing binding: " << binding->token_id << " over " << best_binding->token_id << std::endl;
        best_binding = binding;
        best_binding_nfa_index = nfa_index;
      }
    }

    // ============================================================
    // B. Preserve accepting action sequence
    //
    // Exactly one tag path reaches `best_binding_nfa_index` --
    // it's read straight off the closure, no lookup helper,
    // no ambiguity check needed.
    // ============================================================

    if (best_binding.has_value()) {

      const auto &accept_path =
          current_closure.getActionsForState(best_binding_nfa_index);

      stdu::vector<RawAction> final_actions = append_actions({accept_path});

      states_with_actions[current_dfa_index].accept_binding = best_binding;

      if (!final_actions.empty()) {
        states_with_actions[current_dfa_index].accept_action =
            ActionSequence{.actions = std::move(final_actions),
                           .terminal_dfa_target = NFA::NULL_STATE};
      }
    }

    // ============================================================
    // C. Compute transition candidates.
    //
    // Exactly one candidate per (source NFA state, symbol, raw
    // NFA target) triple: current_closure.getActionsForState()
    // already returns the single, priority-resolved tag path
    // for that source state, so there is nothing left to
    // enumerate per source.
    // ============================================================

    struct TransitionCandidate {
      std::size_t priority;
      ActionPath actions;
      std::size_t target;
      std::size_t source;
    };

    std::map<NFA::TransitionKey, std::vector<TransitionCandidate>>
        transition_candidates;

    for (const std::size_t nfa_index : current_subset) {

      const auto &nfa_state = nfa.getStates().at(nfa_index);

      const auto &actions = current_closure.getActionsForState(nfa_index);

      for (const auto &[symbol, targets] : nfa_state.transitions) {
        for (const auto &target : targets) {

          transition_candidates[symbol].push_back(
              TransitionCandidate{.priority = target.priority,
                                  .actions = actions,
                                  .target = target.next,
                                  .source = nfa_index
              });
        }
      }
    }

    // ============================================================
    // D. Build each outgoing DFA transition.
    //
    // Several NFA source states inside the same DFA state can
    // legitimately reach the SAME raw NFA target on the same
    // symbol -- that's ordinary NFA nondeterminism collapsing
    // under subset construction, not a tag conflict. Keep only
    // the highest-priority candidate per raw target (mirrors
    // epsilonClosure()'s "first/highest priority route wins"
    // rule, applied here to non-epsilon edges).
    //
    // Different raw targets on the same symbol all survive
    // into the same `kernel` -- a DFA can only have one
    // transition per symbol, so every live NFA lineage that
    // takes this symbol necessarily ends up in the same
    // resulting DFA state, however many distinct NFA states
    // that lineage set spans.
    // ============================================================

    for (auto &[symbol, candidates] : transition_candidates) {

      if (candidates.empty())
        continue;

      std::map<std::size_t, const TransitionCandidate *> by_target;

      for (const auto &candidate : candidates) {

        auto [existing, inserted] =
            by_target.emplace(candidate.target, &candidate);

        if (!inserted && candidate.priority < existing->second->priority) {
          existing->second = &candidate;
        }
      }

      std::vector<std::size_t> kernel;
      kernel.reserve(by_target.size());

      for (const auto &[target, candidate] : by_target)
        kernel.push_back(target);

      Closure next_closure(&nfa, kernel);

      std::vector<std::size_t> next_subset = next_closure.get();

      if (next_subset.empty())
        continue;
      stdu::vector<std::size_t> semantic_states;

      for (const std::size_t nfa_index : next_subset) {
        const auto &nfa_state = nfa.getStates().at(nfa_index);

        auto accept_it = nfa.getAcceptMap().find(nfa_index);

        std::optional<NFA::TokenBinding> binding =
        accept_it != nfa.getAcceptMap().end()
        ? std::make_optional(accept_it->second)
        : nfa_state.accept_binding;

        if (binding.has_value() &&
        binding->token_id >= NFA::NESTED_REDUCE_ID_BASE) {
          semantic_states.push_back(nfa_index);
        }
      }
      // --------------------------------------------------------
      // Source-side actions: every surviving candidate's tag
      // path, folded together in priority order.
      //
      // These are the actions belonging to the states being
      // LEFT (the source NFA states that own this outgoing
      // transition), fired the moment `symbol` is consumed.
      //
      // Destination-side actions are intentionally NOT
      // folded in here: they belong to `next_closure`'s own
      // states and are picked up on THAT DFA state's own
      // turn through this loop (via getActionsForState() in
      // step C/B above) -- exactly the same reasoning as the
      // "NOTE" on state 0 at the top of this function. Doing
      // it here as well would fire a destination action one
      // transition too early, or twice.
      // --------------------------------------------------------

      std::vector<const TransitionCandidate *> ordered;
      ordered.reserve(by_target.size());

      for (const auto &[target, candidate] : by_target)
        ordered.push_back(candidate);

      std::ranges::sort(ordered, [](const auto *a, const auto *b) {
        return a->priority < b->priority;
      });

      std::vector<ActionPath> priority_ordered_paths;
      priority_ordered_paths.reserve(ordered.size());

      for (const auto *candidate : ordered) {
        priority_ordered_paths.push_back(candidate->actions);
      }

      stdu::vector<RawAction> actions =
      append_actions(priority_ordered_paths);

      for (const auto nfa_index : semantic_states) {
        const auto &path = next_closure.getActionsForState(nfa_index);

        for (const auto &action : path) {
          if (action.table_type == NFA::TableType::Semantic) {
            actions.push_back(action.raw);
          }
        }
      }
      // --------------------------------------------------------
      // Get/create DFA state.
      // --------------------------------------------------------

      std::size_t target_dfa_index;
      auto it = dfa_state_map.find(next_subset);

      if (it == dfa_state_map.end()) {

        target_dfa_index = states_with_actions.makeNew();

        dfa_state_map.emplace(next_subset, target_dfa_index);

        Assert(dfa_closures.size() == target_dfa_index,
               "DFA closure/state index mismatch: "
               "closure count {}, new DFA index {}",
               dfa_closures.size(), target_dfa_index);

        dfa_closures.push_back(std::move(next_closure));

        work_queue.push(target_dfa_index);

      } else {
        target_dfa_index = it->second;
      }
      states_with_actions[current_dfa_index].transitions[symbol] =
          make_action_sequence(actions, target_dfa_index);
    }
  }

  // ================================================================
  // 3. Basic validation
  //
  // There are deliberately NO ActionTable/SemanticTable index
  // validations here. Those tables don't exist semantically yet.
  // ================================================================

  if (states_with_actions.empty())
    throw Error("DFA cannot be empty");

  // Validate DFA target indices in intermediate action sequences.
  //
  // NOTE: at this point in build(), the intermediate DFA lives in
  // states_with_actions. The final `states` member is populated only
  // later, by minimize(). Every bound check below therefore has to be
  // against states_with_actions.size(), not states.size() (which may
  // still hold a stale/empty result from a previous build).
  for (std::size_t i = 0; i < states_with_actions.size(); ++i) {
    const auto &state = states_with_actions[i];

    // ------------------------------------------------------------
    // Accept action sequence
    // ------------------------------------------------------------

    if (state.accept_action.has_value()) {
      Assert(state.accept_action->terminal_dfa_target == NFA::NULL_STATE,
             "DFA state {} accept action sequence "
             "must terminate in NULL_STATE",
             i);
    }

    // ------------------------------------------------------------
    // Transitions
    // ------------------------------------------------------------

    for (const auto &[symbol, target] : state.transitions) {

      std::visit(
          [&](const auto &next) {
            using T = std::decay_t<decltype(next)>;

            if constexpr (std::is_same_v<T, NFA::DFATarget>) {
              Assert(next.id < states_with_actions.size(),
                     "DFA state {} transition has "
                     "invalid DFA target {}",
                     i, next.id);
            } else if constexpr (std::is_same_v<T, ActionSequence>) {
              Assert(next.terminal_dfa_target < states_with_actions.size(),
                     "DFA state {} transition has "
                     "invalid terminal DFA target {}",
                     i, next.terminal_dfa_target);

              // An action sequence must contain at least
              // one action. Otherwise make_action_sequence()
              // would have returned DFATarget directly.
              Assert(!next.actions.empty(),
                     "DFA state {} transition contains "
                     "empty ActionSequence",
                     i);
            }
          },
          target);
    }
  }
  std::cout << "DFA build complete: " << states_with_actions.size()
            << " states; " << std::endl;
  std::size_t actions = 0;
  for (const auto &state : states_with_actions) {
    for (const auto &trans : state.transitions) {
      if (std::holds_alternative<ActionSequence>(trans.second)) {
        actions += std::get<ActionSequence>(trans.second).actions.size();
      }
    }
  }
  std::cout << "Total actions: " << actions << std::endl;
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

    bool operator<(const RefinedTransition &other) const {
      return std::tie(symbol, kind, action_hash, action_identity,
                      target_partition) <
             std::tie(other.symbol, other.kind, other.action_hash,
                      other.action_identity, other.target_partition);
    }

    bool operator==(const RefinedTransition &other) const {
      return symbol == other.symbol && kind == other.kind &&
             action_hash == other.action_hash &&
             action_identity == other.action_identity &&
             target_partition == other.target_partition;
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

              key.push_back(
                  RefinedTransition{.symbol = symbol,
                                    .kind = 0,
                                    .action_hash = 0,
                                    .action_identity = {},
                                    .target_partition = target_partition});
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

              key.push_back(RefinedTransition{
                  .symbol = symbol,
                  .kind = 1,
                  .action_hash = action_hash,
                  .action_identity = std::move(action_identity),
                  .target_partition = target_partition});
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
                    NFA::DFATarget{NFA::NULL_STATE};
                return;
              }

              const std::size_t target_class = partition_of.at(arg.id);

              destination.transitions[symbol] =
                  NFA::DFATarget{class_to_new_index.at(target_class)};
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
                destination.transitions[symbol] = NFA::DFATarget{mapped};
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
        next = NFA::DFATarget{sequence.terminal_dfa_target};
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

    if (slots[0].table_type == NFA::TableType::Action) {

      return NFA::ActionTarget{slots[0].index};
    }

    return NFA::SemanticTarget{slots[0].index};
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
  if (!nfa.isCharNfa()) {
    throw Error(
        "classify() only applies to character-keyed (CharMachineDFA) automata");
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
        std::visit(
            [&](auto &&target) {
              using T = std::decay_t<decltype(target)>;
              if constexpr (std::is_same_v<T, NFA::DFATarget>) {
                sig.emplace_back(NFA::TableType::DFA, target.id);
              } else if constexpr (std::is_same_v<T, NFA::ActionTarget>) {
                sig.emplace_back(NFA::TableType::Action, target.id);
              } else if constexpr (std::is_same_v<T, NFA::SemanticTarget>) {
                sig.emplace_back(NFA::TableType::Semantic, target.id);
              }
            },
            it->second);
      }
    }
    auto [it, inserted] =
        class_of_signature.try_emplace(sig, class_of_signature.size());
    table.char_to_class[c] = it->second;
  }
  table.num_classes = class_of_signature.size();

  States<State<ClassTransitions>> output(&nfa);
  for (std::size_t i = 0; i < n; ++i) {
    auto new_idx = output.makeNew();
    output[new_idx].accept_binding = states[i].accept_binding;

    // Default terminal transition when no character shift exists in state i
    TransitionTarget default_trans{NFA::DFATarget{NULL_STATE}};
    if (states[i].accept_binding.has_value()) {
      const auto &binding = *states[i].accept_binding;
      if (binding.reduce_rule_id.has_value()) {
        default_trans = NFA::ActionTarget{*binding.reduce_rule_id};
      } else if (binding.target_semantic_state.has_value()) {
        default_trans = NFA::SemanticTarget{*binding.target_semantic_state};
      } else {
        default_trans = NFA::DFATarget{binding.token_id};
      }
    }

    output[new_idx].transitions.assign(table.num_classes, default_trans);

    for (const auto &[symbol, value] : states[i].transitions) {
      if (!std::holds_alternative<char>(symbol))
        continue;
      unsigned char c = static_cast<unsigned char>(std::get<char>(symbol));
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
      if (dfa.table.char_to_class[j] != i)
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