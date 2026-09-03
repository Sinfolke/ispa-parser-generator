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

    auto DFA::build() -> const States<StateWithActions>& {
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

        using RawAction =
            std::variant<
                NFA::ActionState,
                NFA::SemanticState
            >;

        using ActionSequence = ActionSequence;

        using NextTarget =
            std::variant<
                NFA::DFATarget,
                ActionSequence
            >;
        struct ClosureKey {
            std::vector<std::size_t> subset;
            NFA::ActionChain paths;

            bool operator==(const ClosureKey &) const;
        };
        // Use the vector of NFA states as the unique identity of a DFA state.
        //
        // std::map is deliberately used here:
        //   - deterministic
        //   - independent of Closure hashing/equality
        //   - guarantees that equivalent subsets are represented by one DFA state
        std::map<std::vector<std::size_t>, std::size_t> dfa_state_map;

        // Secondary map used only for DFA states reached through a
        // divergent transition (see step D below). Such a state carries
        // deferred SNAPSHOT_APPLY obligations that are NOT reflected in
        // its plain NFA-subset identity, so it must never be folded
        // together with (or mistaken for) a plain dfa_state_map entry
        // that happens to have the same subset. Keyed by subset plus a
        // flattened signature of the pending obligations.
        using SeedSignature = std::vector<std::size_t>;
        // value = (dfa state index, the snapshot_id its SNAPSHOT_APPLY
        // entries were built against) — see step D. Both must be reused
        // together; reusing the state with a *different* snapshot_id
        // would pair a SNAPSHOT_APPLY with a SNAPSHOT that never runs.
        std::map<std::pair<std::vector<std::size_t>, SeedSignature>, std::pair<std::size_t, std::size_t>> seeded_state_map;

        // DFA state index -> snapshot_id of the deferred window it is
        // already living inside, if any. Populated whenever a divergent
        // state is created (see step F). Consulted at the *start* of step F
        // so that a state reached while a snapshot window is still open
        // (e.g. every character of a keyword/identifier prefix ambiguity,
        // or every pass around a `*`/`+` loop) extends that SAME window
        // instead of nesting a brand new SNAPSHOT_APPLY/SNAPSHOT_APPLY_END
        // pair around it with a fresh id. Without this, each such state
        // mints its own id, the deferred action run grows by one wrap
        // layer every time, the SeedSignature below is therefore never the
        // same twice, and seeded_state_map never converges.
        std::unordered_map<std::size_t, std::size_t> active_snapshot_of_state;

        // Work queue now carries the DFA state index directly rather
        // than re-deriving it from the NFA subset via dfa_state_map:
        // once seeded_state_map exists, a subset alone is no longer a
        // unique key, so re-deriving the index from it is unsafe.
        std::queue<std::size_t> work_queue;

        // DFA state ID -> corresponding epsilon closure.
        std::vector<Closure> dfa_closures;

        // Fresh id generator pairing each SNAPSHOT with its SNAPSHOT_APPLY
        // entries. See step D.
        std::size_t next_snapshot_id = 0;

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

            return ActionSequence{
                .actions = actions,
                .terminal_dfa_target = terminal_dfa_target
            };
        };
        // ================================================================
        // Helpers for semantic paths
        // ================================================================

        auto same_fired_sequence =
            [](const stdu::vector<FiredAction> &a,
               const stdu::vector<FiredAction> &b) -> bool {

                if (a.size() != b.size())
                    return false;

                for (std::size_t i = 0; i < a.size(); ++i) {
                    if (a[i].owner != b[i].owner ||
                        a[i].table_type != b[i].table_type ||
                        a[i].table_index != b[i].table_index) {
                        return false;
                        }
                }

                return true;
        };

        // ================================================================
        // Helpers for divergent transition candidates (step D).
        //
        // A divergent transition no longer wraps each deferred action
        // individually. Instead, the *entire* deferred source-side
        // action run for a given by_target entry is bounded by a single
        // SNAPSHOT_APPLY / SNAPSHOT_APPLY_END pair:
        //
        //     SNAPSHOT_APPLY(id)  <- rewinds `pos` once
        //     action_1
        //     action_2
        //     ...
        //     action_n
        //     SNAPSHOT_APPLY_END(id)  <- restores `pos` once
        //
        // The actions in between are threaded through completely
        // unmodified — not reconstructed, not re-tagged. This means:
        //
        //   - An arbitrary number of deferred actions is supported; the
        //     old design could only carry exactly one, because it tried
        //     to fold "which action this replays" into the SNAPSHOT_APPLY
        //     entry itself, and the runtime has no slot for that (see
        //     DFA.API::scan()'s SNAPSHOT_ACCEPT case: it only rewinds
        //     `pos` and falls through to next_state — the deferred
        //     action has to be its own real, separate LR state, or it
        //     silently never runs. That gap is exactly what produced
        //     "END without matching BEGIN" before this rewrite).
        //
        //   - Nesting is free: if `candidate->actions` already contains
        //     an earlier SNAPSHOT_APPLY(...)...SNAPSHOT_APPLY_END(...)
        //     block from a prior divergence, it is simply carried along
        //     as part of the run being bounded — untouched, at whatever
        //     position it already occupies. There is no more "already
        //     wrapped, don't wrap again" special case to maintain,
        //     because nothing here ever mutates an existing FiredAction.
        //
        //   - Convergence for repeating divergences (e.g. the "ID" token:
        //     LETTER (LETTER|DIGIT)*) still holds, and now for a
        //     stronger reason than before: the SeedSignature below is
        //     built from `candidate->actions` exactly as it stood prior
        //     to this step, and this step never writes back into it. A
        //     structurally-identical recurrence therefore always
        //     produces byte-for-byte the same signature, regardless of
        //     which fresh snapshot_id this particular occurrence mints,
        //     so seeded_state_map still converges on a fixed point.
        //
        // SemanticState raw actions (accept-time REDUCE bindings) are
        // intentionally rejected here — deferring a semantic reduce
        // through a snapshot needs its own design and is not part of
        // this change.
        // ================================================================

        auto make_snapshot_boundary =
            [](NFA::Action marker_action,
               NFA::TableType marker_type,
               std::size_t snapshot_id) -> FiredAction {

            // Value-initialize: ActionState::wrapped_action has no
            // default member initializer, and is meaningless for a
            // boundary marker, but leaving it truly uninitialized would
            // feed indeterminate bits into hash_raw_action() below.
            NFA::ActionState marker{};
            marker.action = marker_action;
            marker.snapshot_id = snapshot_id;

            return FiredAction{
                .owner = NFA::NULL_STATE,
                .table_type = marker_type,
                .table_index = snapshot_id,
                .raw = marker
            };
        };

        auto make_deferred_tail =
            [&](const std::vector<FiredAction> &actions,
                std::size_t snapshot_id) -> std::vector<FiredAction> {

            std::vector<FiredAction> tail;
            tail.reserve(actions.size() + 2);

            tail.push_back(
                make_snapshot_boundary(
                    NFA::Action::SNAPSHOT_APPLY,
                    NFA::TableType::Snapshot,
                    snapshot_id
                )
            );

            for (const auto &fa : actions) {

                Assert(
                    std::holds_alternative<NFA::ActionState>(fa.raw),
                    "SNAPSHOT_APPLY deferral is only implemented for "
                    "ActionState actions (BEGIN/END/PUSH), not semantic "
                    "(REDUCE) actions"
                );

                tail.push_back(fa);
            }

            tail.push_back(
                make_snapshot_boundary(
                    NFA::Action::SNAPSHOT_APPLY_END,
                    NFA::TableType::SnapshotEnd,
                    snapshot_id
                )
            );

            return tail;
        };

        auto find_path_to_state =
        [&](const Closure &closure,
            std::size_t state)
            -> const std::vector<FiredAction> * {

            const auto &paths =
                closure.getPathsForState(state);

            if (paths.empty())
                return nullptr;

            const auto &result = paths.front();

            for (std::size_t i = 1; i < paths.size(); ++i) {
                Assert(
                    same_fired_sequence(
                        result,
                        paths[i]
                    ),
                    "NFA state {} is reachable through multiple "
                    "epsilon paths with different action sequences",
                    state
                );
            }

            return &result;
        };
        // ================================================================
        // 1. Start state
        // ================================================================

        Closure start_closure(
            &nfa,
            std::vector<std::size_t>{0}
        );

        std::vector<std::size_t> start_subset = start_closure.get();

        const std::size_t start_idx = states_with_actions.makeNew();

        dfa_state_map.emplace(start_subset, start_idx);
        dfa_closures.push_back(std::move(start_closure));

        work_queue.push(start_idx);

        // NOTE: state 0's own actions (e.g. a leading BEGIN) are NOT
        // seeded here as a special-cased entry_action. Closure::epsilonClosure
        // already folds a state's own actions into path_actions[state] (see
        // Closure.cpp step 1), and the subset-construction loop below reads
        // that same path via getActionsForState() for every outgoing
        // transition and accept_action of the start state. Duplicating them
        // here would fire state 0's actions (e.g. BEGIN) twice: once from
        // this block, once from the closure-derived sequence below.

        // ================================================================
        // 2. Subset construction
        // ================================================================

        while (!work_queue.empty()) {
            const std::size_t current_dfa_index = work_queue.front();
            work_queue.pop();

            const Closure &current_closure =
                dfa_closures.at(current_dfa_index);

            const std::vector<std::size_t> &current_subset =
                current_closure.get();

            // ============================================================
            // A. Resolve accepting binding
            // ============================================================

            std::optional<NFA::TokenBinding> best_binding;
            std::size_t best_binding_nfa_index = NFA::NULL_STATE;

            for (const std::size_t nfa_index : current_subset) {
                const auto &nfa_state =
                    nfa.getStates().at(nfa_index);

                auto accept_it =
                    nfa.getAcceptMap().find(nfa_index);

                std::optional<NFA::TokenBinding> binding =
                    accept_it != nfa.getAcceptMap().end()
                        ? std::make_optional(accept_it->second)
                        : nfa_state.accept_binding;

                if (!binding.has_value())
                    continue;

                const bool current_has_semantic =
                    binding->target_semantic_state.has_value();

                const bool best_has_semantic =
                    best_binding.has_value() &&
                    best_binding->target_semantic_state.has_value();

                if (!best_binding.has_value() ||
                    (current_has_semantic && !best_has_semantic) ||
                    (current_has_semantic == best_has_semantic &&
                     binding->token_id < best_binding->token_id)) {

                    best_binding = binding;
                    best_binding_nfa_index = nfa_index;
                     }
            }

            // ============================================================
            // B. Preserve accepting action sequence
            // ============================================================

            if (best_binding.has_value()) {

                const auto *accept_path =
                    find_path_to_state(
                        current_closure,
                        best_binding_nfa_index
                    );

                Assert(
                    accept_path != nullptr,
                    "Accepting NFA state {} is in DFA closure but has "
                    "no corresponding semantic path",
                    best_binding_nfa_index
                );

                stdu::vector<RawAction> final_actions;
                final_actions.reserve(
                    accept_path->size()
                );

                for (const auto &fa :
                     *accept_path) {

                    final_actions.push_back(fa.raw);
                     }

                states_with_actions[current_dfa_index]
                    .accept_binding =
                        best_binding;

                if (!final_actions.empty()) {
                    states_with_actions[current_dfa_index]
                        .accept_action =
                            ActionSequence{
                        .actions =
                            std::move(final_actions),

                        .terminal_dfa_target =
                            NFA::NULL_STATE
                    };
                }
            }

            // ============================================================
            // C. Compute transition paths
            //
            // A transition path consists of:
            //
            //     epsilon actions leading to an NFA state
            //          +
            //     the character transition leaving that state
            //
            // The actions belong to THIS transition.
            //
            // They must never be merged with actions from another
            // independent epsilon path.
            // ============================================================

            struct TransitionCandidate {
                std::size_t source;
                std::size_t target;
                std::vector<FiredAction> actions;
            };

            std::map<
                NFA::TransitionKey,
                std::vector<TransitionCandidate>
            > transition_candidates;

            for (const std::size_t nfa_index : current_subset) {

                const auto &nfa_state =
                    nfa.getStates().at(nfa_index);

                for (const auto &[symbol, targets] :
                     nfa_state.transitions) {

                    /*
                     * Every path reaching this source NFA state is a separate
                     * candidate transition.
                     */
                    const auto &paths =
                        current_closure.getPathsForState(nfa_index);

                    for (const auto &path : paths) {

                        TransitionCandidate candidate;

                        candidate.source = nfa_index;

                        for (const auto &fa : path)
                            candidate.actions.push_back(fa);

                        for (const auto &target : targets) {

                            candidate.target = target.next;

                            transition_candidates[symbol].push_back(
                                candidate
                            );
                        }
                    }
                     }
            }


            // ============================================================
            // D. Create DFA states and transitions
            // ============================================================

            for (auto &[symbol, candidates] :
                 transition_candidates) {

                if (candidates.empty())
                    continue;

                // ============================================================
                // A. Group candidates by destination NFA state.
                // ============================================================

                std::map<
                    std::size_t,
                    const TransitionCandidate *
                > by_target;

                for (const auto &candidate : candidates) {

                    auto [existing, inserted] =
                        by_target.emplace(
                            candidate.target,
                            &candidate
                        );

                    if (!inserted) {

                        // The same NFA state cannot have two different
                        // semantic histories in one deterministic closure.
                        Assert(
                            same_fired_sequence(
                                existing->second->actions,
                                candidate.actions
                            ),
                            "DFA transition reaches NFA state {} through "
                            "multiple epsilon paths with different action "
                            "sequences",
                            candidate.target
                        );
                    }
                }

                // ============================================================
                // B. Determine source-side divergence.
                //
                // `by_target` now contains exactly one action path per
                // destination NFA state.
                //
                // Different destination states may legitimately have different
                // source paths. That is exactly the ambiguity for which
                // SNAPSHOT exists.
                // ============================================================

                bool source_diverges = false;

                const auto &source_baseline =
                    by_target.begin()->second->actions;

                for (auto it = std::next(by_target.begin());
                     it != by_target.end();
                     ++it) {

                    if (!same_fired_sequence(
                            source_baseline,
                            it->second->actions
                        )) {

                        source_diverges = true;
                        break;
                    }
                }

                // ============================================================
                // C. Build destination epsilon closure ONCE.
                //
                // This is the real NFA subset after consuming `symbol`.
                //
                // Do not throw this closure away in the divergent case. Its
                // subset is still the identity of the resulting DFA state.
                // ============================================================

                std::vector<std::size_t> kernel;
                kernel.reserve(by_target.size());

                for (const auto &[target, candidate] :
                     by_target) {

                    kernel.push_back(target);
                }

                Closure next_closure(
                    &nfa,
                    kernel
                );

                const std::vector<std::size_t> next_subset =
                    next_closure.get();

                if (next_subset.empty())
                    continue;

                // ============================================================
                // D. Analyse destination epsilon paths.
                //
                // `paths` has the form:
                //
                //     NFA state -> [path1, path2, ...]
                //
                // There are TWO different kinds of divergence here.
                //
                // 1. Same destination NFA state:
                //
                //        X -> [A]
                //        X -> [B]
                //
                //    This is a genuine semantic ambiguity and is rejected.
                //
                // 2. Different destination NFA states:
                //
                //        X -> [A]
                //        Y -> [B]
                //
                //    This is NOT an error. It means the DFA subset contains
                //    multiple semantic lineages and therefore requires the
                //    snapshot mechanism.
                // ============================================================

                bool destination_state_ambiguity = false;
                bool destination_diverges = false;

                std::optional<
                    std::vector<FiredAction>
                > destination_baseline;

                for (const auto &[destination_state, state_paths] :
                     next_closure.getPaths()) {

                    if (state_paths.empty())
                        continue;

                    // --------------------------------------------------------
                    // First: validate all paths reaching THIS state.
                    // --------------------------------------------------------

                    const auto &state_baseline =
                        state_paths.front();

                    for (std::size_t i = 1;
                         i < state_paths.size();
                         ++i) {

                        if (!same_fired_sequence(
                                state_baseline,
                                state_paths[i]
                            )) {

                            destination_state_ambiguity = true;
                            break;
                        }
                    }

                    if (destination_state_ambiguity)
                        break;

                    // --------------------------------------------------------
                    // Second: compare this state's path with paths belonging
                    // to OTHER destination states.
                    // --------------------------------------------------------

                    if (!destination_baseline.has_value()) {

                        destination_baseline =
                            state_baseline;

                    } else if (
                        !same_fired_sequence(
                            *destination_baseline,
                            state_baseline
                        )
                    ) {

                        destination_diverges = true;
                    }
                }

                // A single NFA state cannot have two incompatible semantic
                // histories.
                Assert(
                    !destination_state_ambiguity,
                    "NFA destination state has multiple epsilon paths "
                    "with different action sequences"
                );

                // ============================================================
                // E. Ordinary transition.
                //
                // This is possible ONLY when:
                //
                //     source does not diverge
                //     AND
                //     destination does not diverge
                //
                // In that case there is one complete action sequence:
                //
                //     source actions
                //         |
                //         | consume character
                //         v
                //     destination actions
                // ============================================================

                if (!source_diverges) {

                    std::vector<FiredAction> selected_actions =
                        source_baseline;

                    // --------------------------------------------------------
                    // NOTE: destination-side actions are intentionally NOT
                    // folded into this transition.
                    //
                    // `destination_baseline` (see step D above) is derived
                    // from `next_closure.getPathsForState(Y)` for a
                    // destination NFA state Y — but `next_closure` does not
                    // get discarded: it is pushed into `dfa_closures` and
                    // becomes `current_closure` the moment the resulting
                    // DFA state is popped from `work_queue` (see the
                    // `dfa_closures.push_back(std::move(next_closure))`
                    // below). At that point Y's own action, already folded
                    // into `path_actions[Y]` by Closure::epsilonClosure
                    // ("Actions hosted by the state are executed when the
                    // epsilon traversal ENTERS that state"), gets read
                    // AGAIN — this time via `current_closure
                    // .getPathsForState(Y)` in step C above, feeding Y's
                    // own outgoing transitions as source-side actions.
                    //
                    // Folding `destination_baseline` in here as well would
                    // attach the SAME FiredAction to two different
                    // transitions: this one (into the new state, firing
                    // immediately) and Y's own future outgoing transition
                    // or accept action (firing one character later) — a
                    // silent double-fire that surfaces as actions
                    // appearing to run one transition later than expected,
                    // or running twice.
                    //
                    // This is exactly the duplication the start-state
                    // comment near the top of DFA::build() already avoids
                    // for state 0: a state's own actions fire exactly
                    // once, read via getPathsForState() on THAT state's
                    // own turn through the work_queue loop (either as
                    // source-side here in step C/E, or via the accept
                    // path in step B) — never pre-attached to the edge
                    // that merely reaches it.
                    // --------------------------------------------------------

                    stdu::vector<RawAction> actions;

                    std::vector<
                        std::tuple<
                            std::size_t,
                            NFA::TableType,
                            std::size_t
                        >
                    > seen;

                    actions.reserve(
                        selected_actions.size()
                    );

                    seen.reserve(
                        selected_actions.size()
                    );

                    auto append_unique =
                        [&](const FiredAction &fa) {

                            const auto identity =
                                std::make_tuple(
                                    fa.owner,
                                    fa.table_type,
                                    fa.table_index
                                );

                            if (std::find(
                                    seen.begin(),
                                    seen.end(),
                                    identity
                                ) != seen.end()) {

                                return;
                            }

                            seen.push_back(identity);
                            actions.push_back(fa.raw);
                        };

                    // --------------------------------------------------------
                    // Source-side actions.
                    //
                    // This is the state we are LEAVING (the source NFA
                    // state that owns this outgoing transition), plus
                    // everything on the epsilon path that reached it.
                    // --------------------------------------------------------

                    for (const auto &fa :
                         selected_actions) {

                        append_unique(fa);
                    }

                    // --------------------------------------------------------
                    // Get/create DFA state.
                    // --------------------------------------------------------

                    std::size_t target_dfa_index;
                    auto it =
                        dfa_state_map.find(next_subset);

                    if (it == dfa_state_map.end()) {

                        target_dfa_index =
                            states_with_actions.makeNew();

                        dfa_state_map.emplace(
                            next_subset,
                            target_dfa_index
                        );

                        Assert(
                            dfa_closures.size() ==
                                target_dfa_index,
                            "DFA closure/state index mismatch: "
                            "closure count {}, new DFA index {}",
                            dfa_closures.size(),
                            target_dfa_index
                        );

                        dfa_closures.push_back(
                            std::move(next_closure)
                        );

                        work_queue.push(
                            target_dfa_index
                        );

                    } else {

                        target_dfa_index =
                            it->second;
                    }

                    states_with_actions[current_dfa_index]
                        .transitions[symbol] =
                            make_action_sequence(
                                actions,
                                target_dfa_index
                            );

                    continue;
                }

                // ============================================================
                // F. Divergent transition.
                //
                // Either:
                //
                //     source_diverges
                //
                // or:
                //
                //     destination_diverges
                //
                // means that one ActionSequence cannot represent the complete
                // semantic history.
                //
                // We therefore:
                //
                //     1. SNAPSHOT at the character boundary.
                //     2. Enter a DFA state carrying the appropriate deferred
                //        SNAPSHOT_APPLY actions.
                //
                // The character transition itself is NOT rolled back.
                // SNAPSHOT_APPLY only restores the saved `pos` while executing
                // the deferred action.
                // ============================================================

                // ------------------------------------------------------------
                // F.0 Self-loop short-circuit.
                //
                // If the destination NFA subset is IDENTICAL to the subset of
                // the DFA state we are currently expanding, this transition
                // loops back onto the very same DFA state. This is exactly
                // what happens on every additional character consumed by a
                // repetition such as `[a-zA-Z0-9_]*` once the loop has
                // already diverged once (e.g. loop-vs-accept).
                //
                // Without this check, every pass around the loop re-enters
                // the "genuinely new divergent state" branch below: it wraps
                // the ALREADY-deferred action run (which itself contains the
                // previous pass's SNAPSHOT_APPLY/SNAPSHOT_APPLY_END pair) in
                // one more such pair with a freshly minted snapshot_id. That
                // grows `candidate->actions` by two entries every iteration,
                // so the SeedSignature built below is never the same twice —
                // seeded_state_map.find() always misses, a brand new DFA
                // state is minted and pushed onto work_queue every time, and
                // DFA::build() never terminates (this is the infinite loop
                // reported for grammars like `ID: @([a-zA-Z][a-zA-Z0-9_]*)
                // {@};`).
                //
                // The deferred obligations already active for
                // `current_dfa_index` (baked into its own seed, if it is
                // itself a seeded/divergent state) remain correct for every
                // subsequent pass through the loop — nothing new needs to be
                // deferred just to consume one more character of the same
                // repetition — so we simply route the transition back onto
                // the state we are already in instead of minting anything
                // new.
                // ------------------------------------------------------------

                if (next_subset == current_subset) {

                    states_with_actions[current_dfa_index]
                        .transitions[symbol] =
                            NFA::DFATarget{current_dfa_index};

                    continue;
                }

                // ------------------------------------------------------------
                // F.1 Are we already inside an open deferred window?
                //
                // If `current_dfa_index` was itself produced by a prior
                // divergence whose snapshot has not yet been resolved
                // (SNAPSHOT_APPLY/SNAPSHOT_APPLY_END has not fired), then
                // `candidate->actions` for every by_target entry here is
                // ALREADY the correctly-bounded deferred run from that
                // earlier boundary — it must not be wrapped in a second,
                // nested SNAPSHOT_APPLY/SNAPSHOT_APPLY_END pair with a new
                // id. Doing so is exactly what produced 3 separate
                // snapshots for the 3 characters of "int": each character
                // re-diverges, and the old code minted id=1, then id=2,
                // nesting deeper each time.
                //
                // Extending the SAME window means: reuse the existing
                // snapshot_id, thread `candidate->actions` through
                // unmodified (no extra wrap), and do NOT emit a fresh
                // SNAPSHOT marker on this character transition — the
                // position was already recorded when the window opened.
                // ------------------------------------------------------------

                const auto active_it =
                    active_snapshot_of_state.find(current_dfa_index);

                const bool extending_open_window =
                    active_it != active_snapshot_of_state.end();

                // ------------------------------------------------------------
                // Build a structural signature for the semantic obligations.
                //
                // The snapshot ID is deliberately excluded.
                // ------------------------------------------------------------

                SeedSignature signature;

                signature.reserve(
                    by_target.size() * 4
                );

                for (const auto &[target, candidate] :
                     by_target) {

                    signature.push_back(target);

                    signature.push_back(
                        candidate->actions.size()
                    );

                    for (const auto &fa :
                         candidate->actions) {

                        signature.push_back(
                            fa.owner
                        );

                        signature.push_back(
                            static_cast<std::size_t>(
                                fa.table_type
                            )
                        );

                        signature.push_back(
                            fa.table_index
                        );
                    }
                }

                // ------------------------------------------------------------
                // The destination NFA subset is already known from
                // `next_closure`.
                //
                // Use both subset and semantic seed signature as the identity
                // of this divergent DFA state. Since an already-open window
                // is threaded through unmodified rather than re-wrapped, its
                // signature is stable across recurrences, so this key still
                // converges instead of growing without bound.
                // ------------------------------------------------------------

                auto seed_key =
                    std::make_pair(
                        next_subset,
                        signature
                    );

                std::size_t target_dfa_index;
                std::size_t snapshot_id;

                auto sit =
                    seeded_state_map.find(seed_key);

                if (sit == seeded_state_map.end()) {

                    // --------------------------------------------------------
                    // This is a genuinely new divergent state.
                    // --------------------------------------------------------

                    snapshot_id =
                        extending_open_window
                            ? active_it->second
                            : next_snapshot_id++;

                    std::vector<
                        std::pair<
                            std::size_t,
                            std::vector<FiredAction>
                        >
                    > seeded_kernel;

                    seeded_kernel.reserve(
                        by_target.size()
                    );

                    for (const auto &[target, candidate] :
                         by_target) {

                        // ----------------------------------------------------
                        // Source-side actions are deferred, as a single
                        // SNAPSHOT_APPLY ... SNAPSHOT_APPLY_END-bounded
                        // run (see make_deferred_tail above) -- but only when
                        // this is the boundary where the window is opening.
                        // If we're extending an already-open window, the
                        // bounding markers are already present in
                        // `candidate->actions` from the earlier boundary, so
                        // they are threaded through as-is.
                        //
                        // Destination-side actions are NOT manually added
                        // here. Closure will append the actions belonging to
                        // the destination NFA states itself, after this
                        // seed.
                        // ----------------------------------------------------

                        seeded_kernel.emplace_back(
                            target,
                            extending_open_window
                                ? candidate->actions
                                : make_deferred_tail(
                                      candidate->actions,
                                      snapshot_id
                                  )
                        );
                    }

                    // --------------------------------------------------------
                    // IMPORTANT:
                    //
                    // This closure is intentionally seeded.
                    //
                    // The seed contains the deferred source-side actions.
                    // epsilonClosure() then appends destination-side actions
                    // when entering destination NFA states.
                    // --------------------------------------------------------

                    Closure seeded_closure(
                        &nfa,
                        seeded_kernel
                    );

                    target_dfa_index =
                        states_with_actions.makeNew();

                    seeded_state_map.emplace(
                        seed_key,
                        std::make_pair(
                            target_dfa_index,
                            snapshot_id
                        )
                    );

                    // The new state remains inside the same open window
                    // (or opens a fresh one) -- either way, downstream
                    // divergences reached through it must keep extending
                    // this snapshot_id rather than minting their own.
                    active_snapshot_of_state.emplace(
                        target_dfa_index,
                        snapshot_id
                    );

                    Assert(
                        dfa_closures.size() ==
                            target_dfa_index,
                        "DFA closure/state index mismatch: "
                        "closure count {}, new DFA index {}",
                        dfa_closures.size(),
                        target_dfa_index
                    );

                    dfa_closures.push_back(
                        std::move(seeded_closure)
                    );

                    work_queue.push(
                        target_dfa_index
                    );

                } else {

                    // --------------------------------------------------------
                    // Reuse the existing semantic state AND its snapshot ID.
                    // --------------------------------------------------------

                    target_dfa_index =
                        sit->second.first;

                    snapshot_id =
                        sit->second.second;
                }

                // ============================================================
                // G. Put SNAPSHOT on the character transition -- but only
                // when this transition is the one OPENING the deferred
                // window. When extending an already-open window (see F.1),
                // the position was already recorded at the earlier
                // boundary; emitting another SNAPSHOT here would silently
                // overwrite that saved position with the wrong one and is
                // exactly what turned one intended snapshot into three for
                // a keyword/identifier prefix like "int".
                //
                // The transition already consumed the character. We do NOT
                // restore the transition itself.
                //
                // SNAPSHOT merely records the position at this boundary.
                //
                // SNAPSHOT_APPLY later restores that position temporarily
                // while executing the deferred BEGIN/END/PUSH action.
                // ============================================================

                if (extending_open_window) {

                    states_with_actions[current_dfa_index]
                        .transitions[symbol] =
                            NFA::DFATarget{target_dfa_index};

                } else {

                    NFA::ActionState snapshot_marker;

                    snapshot_marker.action =
                        NFA::Action::SNAPSHOT;

                    snapshot_marker.snapshot_id =
                        snapshot_id;

                    stdu::vector<RawAction> actions;

                    actions.push_back(
                        snapshot_marker
                    );

                    states_with_actions[current_dfa_index]
                        .transitions[symbol] =
                            make_action_sequence(
                                actions,
                                target_dfa_index
                            );
                }
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
                Assert(
                    state.accept_action->terminal_dfa_target ==
                        NFA::NULL_STATE,
                    "DFA state {} accept action sequence "
                    "must terminate in NULL_STATE",
                    i
                );
            }

            // ------------------------------------------------------------
            // Transitions
            // ------------------------------------------------------------

            for (const auto &[symbol, target] :
                 state.transitions) {

                std::visit(
                    [&](const auto &next) {
                        using T =
                            std::decay_t<decltype(next)>;

                        if constexpr (
                            std::is_same_v<T, NFA::DFATarget>
                        ) {
                            Assert(
                                next.id < states_with_actions.size(),
                                "DFA state {} transition has "
                                "invalid DFA target {}",
                                i,
                                next.id
                            );
                        }
                        else if constexpr (
                            std::is_same_v<T, ActionSequence>
                        ) {
                            Assert(
                                next.terminal_dfa_target <
                                    states_with_actions.size(),
                                "DFA state {} transition has "
                                "invalid terminal DFA target {}",
                                i,
                                next.terminal_dfa_target
                            );

                            // An action sequence must contain at least
                            // one action. Otherwise make_action_sequence()
                            // would have returned DFATarget directly.
                            Assert(
                                !next.actions.empty(),
                                "DFA state {} transition contains "
                                "empty ActionSequence",
                                i
                            );
                        }
                    },
                    target
                );
            }
        }

        return states_with_actions;
    }
    void DFA::optimizeRegistersAndLRTable() {
        std::vector<bool> used(action_table.size(), false);

        // Recursively mark reachable action table entries
        auto mark_action = [&](auto self, std::size_t idx) -> void {
            if (idx >= action_table.size() || used[idx]) return;
            used[idx] = true;

            if (std::holds_alternative<NFA::ActionTarget>(action_table[idx].next_state)) {
                self(self, std::get<NFA::ActionTarget>(action_table[idx].next_state).id);
            }
        };

        // 1. Mark phase: Collect all roots reachable from states and semantic table
        for (const auto &state : states) {

            // Accept binding / reduce rule root
            if (state.accept_binding && state.accept_binding->reduce_rule_id.has_value()) {
                mark_action(mark_action, *state.accept_binding->reduce_rule_id);
            }

            // Transition target roots
            for (const auto &[symbol, target] : state.transitions) {
                if (std::holds_alternative<NFA::ActionTarget>(target)) {
                    mark_action(mark_action, std::get<NFA::ActionTarget>(target).id);
                }
            }
        }

        // Semantic table cross-reference roots
        for (const auto &sem : semantic_table) {
            if (std::holds_alternative<NFA::ActionTarget>(sem.next_state)) {
                mark_action(mark_action, std::get<NFA::ActionTarget>(sem.next_state).id);
            }
        }

        // 2. Iterative deduplication & tail-folding pass
        std::vector<NFA::ActionState> deduplicated_lr_table;
        std::unordered_map<std::size_t, std::size_t> lr_index_remap;

        bool merged_any = true;
        while (merged_any) {
            merged_any = false;
            deduplicated_lr_table.clear();
            lr_index_remap.clear();

            for (std::size_t i = 0; i < action_table.size(); ++i) {
                if (!used[i]) continue;
                const auto &entry = action_table[i];
                std::size_t canonical_idx = NFA::NULL_STATE;

                for (std::size_t j = 0; j < deduplicated_lr_table.size(); ++j) {
                    if (deduplicated_lr_table[j].action == entry.action &&
                        deduplicated_lr_table[j].variable == entry.variable &&
                        deduplicated_lr_table[j].next_state == entry.next_state &&
                        // SNAPSHOT / SNAPSHOT_APPLY / SNAPSHOT_APPLY_END
                        // entries carry their real identity in
                        // snapshot_id (which SNAPSHOT/SNAPSHOT_APPLY_END
                        // pairs with which SNAPSHOT_APPLY). Two entries
                        // with the same .action/.variable/.next_state but
                        // different snapshot_id are NOT the same entry —
                        // collapsing them silently pairs a rewind/restore
                        // with the wrong boundary, which is what produced
                        // "END without matching BEGIN" originally. See
                        // hash_raw_action() above, which already accounts
                        // for this; this comparison must match it.
                        //
                        // wrapped_action no longer carries meaning after
                        // the switch to boundary-marker deferral (a
                        // SNAPSHOT_APPLY brackets a run of ordinary,
                        // already-separate actions instead of wrapping
                        // one), but comparing it is still harmless and
                        // keeps this in lockstep with hash_raw_action().
                        deduplicated_lr_table[j].snapshot_id == entry.snapshot_id &&
                        deduplicated_lr_table[j].wrapped_action == entry.wrapped_action) {
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

            // Remap internal pointers within the action table itself
            for (auto &entry : deduplicated_lr_table) {
                if (std::holds_alternative<NFA::ActionTarget>(entry.next_state)) {
                    auto &act = std::get<NFA::ActionTarget>(entry.next_state);
                    if (auto it = lr_index_remap.find(act.id); it != lr_index_remap.end()) {
                        act.id = it->second;
                    }
                }
            }

            action_table = std::move(deduplicated_lr_table);
            used.assign(action_table.size(), true);

            // Update all external references pointing into action_table
            for (auto &state : states) {

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
                if (act_id < action_table.size()) {
                    // RECURSE through action chains to reach downstream semantic states!
                    self(self, action_table[act_id].next_state);
                }
            }
        };

        for (const auto &state : states) {
            if (state.accept_binding && state.accept_binding->target_semantic_state) {
                // Implicit conversion from NFA::SemanticTarget to NextTarget now works
                resolve_target(resolve_target, NFA::SemanticTarget{*state.accept_binding->target_semantic_state});
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

        for (auto &act : action_table) {
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

            for (auto &[symbol, target] : state.transitions) {
                if (std::holds_alternative<NFA::SemanticTarget>(target)) {
                    update_sem_target(std::get<NFA::SemanticTarget>(target));
                }
            }
        }

        semantic_table = std::move(compacted);
    }

    auto DFA::sameAcceptBinding(
        const State<> &a,
        const State<> &b
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

        auto hash_raw_action =
            [&](const NFA::ActionState &action,
                std::size_t &hash) {

                hash_combine(
                    hash,
                    uhash{}(action.action)
                );

                hash_combine(
                    hash,
                    uhash{}(action.variable)
                );

                // SNAPSHOT / SNAPSHOT_APPLY / SNAPSHOT_APPLY_END entries
                // additionally carry meaning in snapshot_id (which
                // SNAPSHOT/SNAPSHOT_APPLY_END pairs with which
                // SNAPSHOT_APPLY). They must not be collapsed together
                // with a different one that happens to share
                // `.action`/`.variable` but not this. wrapped_action is
                // hashed too for backward-compatible identity, though it
                // no longer carries meaning under boundary-marker
                // deferral (see step D / make_deferred_tail).
                hash_combine(
                    hash,
                    uhash{}(action.snapshot_id)
                );

                hash_combine(
                    hash,
                    uhash{}(action.wrapped_action)
                );

                // Do NOT hash action.next_state here.
                //
                // The intermediate action sequence owns the control flow.
                // The next action is simply the next element of the vector.
            };

        auto hash_raw_semantic =
            [&](const NFA::SemanticState &semantic,
                std::size_t &hash) {

                hash_combine(
                    hash,
                    uhash{}(semantic.instance_value)
                );

                hash_combine(
                    hash,
                    uhash{}(semantic.statements)
                );

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
                hash_combine(
                    hash,
                    uhash{}(sequence.actions.size())
                );

                for (const auto &raw : sequence.actions) {
                    std::visit(
                        [&](const auto &action) {
                            using T = std::decay_t<decltype(action)>;

                            if constexpr (
                                std::is_same_v<
                                    T,
                                    NFA::ActionState
                                >
                            ) {
                                // Distinguish ActionState from
                                // SemanticState even if their contents
                                // happen to hash similarly.
                                hash_combine(
                                    hash,
                                    std::size_t{0}
                                );

                                hash_raw_action(action, hash);
                            }
                            else {
                                hash_combine(
                                    hash,
                                    std::size_t{1}
                                );

                                hash_raw_semantic(action, hash);
                            }
                        },
                        raw
                    );
                }

                return hash;
            };

        // ================================================================
        // Initial state class.
        //
        // This must describe properties intrinsic to the state itself.
        // Transition destinations are refined later.
        // ================================================================

        auto state_base_hash =
            [&](const StateWithActions &state) -> std::size_t {

                std::size_t hash = 0;

                // Preserve the old initialClass semantics for accepting
                // information.
                //
                // We deliberately still use initialClass here because it
                // already contains the token/acceptance semantics that
                // define an initial partition.
                hash_combine(
                    hash,
                    initialClass(state)
                );
                hash_combine(hash, std::size_t{0});

                // --------------------------------------------------------
                // Accept action sequence is also intrinsic state behavior.
                // --------------------------------------------------------

                if (state.accept_action.has_value()) {
                    hash_combine(hash, std::size_t{1});
                    hash_combine(
                        hash,
                        hash_action_sequence(
                            *state.accept_action
                        )
                    );
                }
                else {
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
                return std::tie(
                    symbol,
                    kind,
                    action_hash,
                    action_identity,
                    target_partition
                ) < std::tie(
                    other.symbol,
                    other.kind,
                    other.action_hash,
                    other.action_identity,
                    other.target_partition
                );
            }

            bool operator==(const RefinedTransition &other) const {
                return symbol == other.symbol &&
                       kind == other.kind &&
                       action_hash == other.action_hash &&
                       action_identity == other.action_identity &&
                       target_partition == other.target_partition;
            }
        };

        auto make_transition_key =
            [&](const StateWithActions &state,
                const std::unordered_map<
                    std::size_t,
                    std::size_t
                > &partition_of)
            -> std::vector<RefinedTransition> {

                std::vector<RefinedTransition> key;

                key.reserve(
                    state.transitions.size()
                );

                for (const auto &[symbol, target] :
                     state.transitions) {

                    std::visit(
                        [&](const auto &arg) {
                            using T =
                                std::decay_t<decltype(arg)>;

                            // ------------------------------------------------
                            // Direct DFA transition
                            // ------------------------------------------------

                            if constexpr (
                                std::is_same_v<
                                    T,
                                    NFA::DFATarget
                                >
                            ) {
                                const std::size_t target_partition =
                                    arg.id == NFA::NULL_STATE
                                        ? NFA::NULL_STATE
                                        : partition_of.at(arg.id);

                                key.push_back(
                                    RefinedTransition{
                                        .symbol = symbol,
                                        .kind = 0,
                                        .action_hash = 0,
                                        .action_identity = {},
                                        .target_partition =
                                            target_partition
                                    }
                                );
                            }

                            // ------------------------------------------------
                            // Ordered action sequence
                            // ------------------------------------------------

                            else if constexpr (
                                std::is_same_v<
                                    T,
                                    ActionSequence
                                >
                            ) {
                                std::vector<std::size_t>
                                    action_identity;

                                action_identity.reserve(
                                    arg.actions.size() * 3
                                );

                                std::size_t action_hash = 0;

                                hash_combine(
                                    action_hash,
                                    uhash{}(
                                        arg.actions.size()
                                    )
                                );

                                for (const auto &raw :
                                     arg.actions) {

                                    std::visit(
                                        [&](const auto &action) {
                                            using A =
                                                std::decay_t<
                                                    decltype(action)
                                                >;

                                            if constexpr (
                                                std::is_same_v<
                                                    A,
                                                    NFA::ActionState
                                                >
                                            ) {
                                                // Type marker.
                                                action_identity.push_back(0);

                                                hash_combine(
                                                    action_hash,
                                                    std::size_t{0}
                                                );

                                                hash_raw_action(
                                                    action,
                                                    action_hash
                                                );

                                                // Keep an explicit
                                                // structural identity as
                                                // well as the hash.
                                                action_identity.push_back(
                                                    uhash{}(
                                                        action.action
                                                    )
                                                );

                                                action_identity.push_back(
                                                    uhash{}(
                                                        action.variable
                                                    )
                                                );

                                                // See hash_raw_action:
                                                // snapshot_id / wrapped_action
                                                // carry real semantic meaning
                                                // for SNAPSHOT/SNAPSHOT_APPLY
                                                // entries and must factor into
                                                // structural identity too.
                                                action_identity.push_back(
                                                    uhash{}(
                                                        action.snapshot_id
                                                    )
                                                );

                                                action_identity.push_back(
                                                    uhash{}(
                                                        action.wrapped_action
                                                    )
                                                );
                                            }
                                            else {
                                                // Type marker.
                                                action_identity.push_back(1);

                                                hash_combine(
                                                    action_hash,
                                                    std::size_t{1}
                                                );

                                                hash_raw_semantic(
                                                    action,
                                                    action_hash
                                                );

                                                action_identity.push_back(
                                                    uhash{}(
                                                        action.instance_value
                                                    )
                                                );

                                                action_identity.push_back(
                                                    uhash{}(
                                                        action.statements
                                                    )
                                                );
                                            }
                                        },
                                        raw
                                    );
                                }

                                const std::size_t target_partition =
                                    arg.terminal_dfa_target ==
                                            NFA::NULL_STATE
                                        ? NFA::NULL_STATE
                                        : partition_of.at(
                                              arg.terminal_dfa_target
                                          );

                                key.push_back(
                                    RefinedTransition{
                                        .symbol = symbol,
                                        .kind = 1,
                                        .action_hash = action_hash,
                                        .action_identity =
                                            std::move(action_identity),
                                        .target_partition =
                                            target_partition
                                    }
                                );
                            }
                        },
                        target
                    );
                }

                std::sort(
                    key.begin(),
                    key.end()
                );

                return key;
            };

        // ================================================================
        // 1. Initial partition
        // ================================================================

        std::unordered_map<
            std::size_t,
            std::size_t
        > partition_of;

        std::map<
            std::size_t,
            std::size_t
        > initial_hash_to_class;

        std::size_t class_count = 0;

        for (std::size_t i = 0; i < n; ++i) {
            const std::size_t hash =
                state_base_hash(input[i]);

            auto [it, inserted] =
                initial_hash_to_class.emplace(
                    hash,
                    class_count
                );

            if (inserted)
                ++class_count;

            partition_of[i] = it->second;
        }

        // ================================================================
        // 2. Partition refinement
        // ================================================================

        while (true) {
            std::unordered_map<
                std::size_t,
                std::size_t
            > new_partition;

            std::map<
                std::pair<
                    std::size_t,
                    std::vector<RefinedTransition>
                >,
                std::size_t
            > signature_to_class;

            std::size_t new_class_count = 0;

            for (std::size_t i = 0; i < n; ++i) {
                const std::size_t base_hash =
                    state_base_hash(input[i]);

                const auto transition_key =
                    make_transition_key(
                        input[i],
                        partition_of
                    );

                const auto signature =
                    std::make_pair(
                        base_hash,
                        transition_key
                    );

                auto [it, inserted] =
                    signature_to_class.emplace(
                        signature,
                        new_class_count
                    );

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
            }
            else {
                for (std::size_t i = 0; i < n && !changed; ++i) {
                    for (std::size_t j = i + 1;
                         j < n;
                         ++j) {

                        const bool old_same =
                            partition_of.at(i) ==
                            partition_of.at(j);

                        const bool new_same =
                            new_partition.at(i) ==
                            new_partition.at(j);

                        if (old_same != new_same) {
                            changed = true;
                            break;
                        }
                    }
                }
            }

            partition_of =
                std::move(new_partition);

            if (!changed)
                break;
        }

        // ================================================================
        // 3. Create one intermediate state per final partition.
        // ================================================================

        std::map<
            std::size_t,
            std::size_t
        > class_to_new_index;

        States<StateWithActions> minimized(
            &nfa
        );

        // Force class numbering into deterministic state numbering by
        // traversing original states in order.
        for (std::size_t i = 0; i < n; ++i) {
            const std::size_t cls =
                partition_of.at(i);

            if (!class_to_new_index.contains(cls)) {
                class_to_new_index.emplace(
                    cls,
                    minimized.makeNew()
                );
            }
        }

        // ================================================================
        // 4. Copy minimized state contents.
        //
        // Since all states in one partition have identical refinement
        // signatures, taking the first state is valid.
        // ================================================================

        std::unordered_set<std::size_t>
            constructed_classes;

        for (std::size_t i = 0; i < n; ++i) {
            const std::size_t cls =
                partition_of.at(i);

            if (!constructed_classes.insert(cls).second)
                continue;

            const std::size_t new_idx =
                class_to_new_index.at(cls);

            const auto &source =
                input[i];

            auto &destination =
                minimized[new_idx];

            // ------------------------------------------------------------
            // These remain completely unmaterialized.
            // ------------------------------------------------------------

            destination.accept_binding =
                source.accept_binding;

            destination.accept_action =
                source.accept_action;

            // ------------------------------------------------------------
            // Transitions
            // ------------------------------------------------------------

            for (const auto &[symbol, target] :
                 source.transitions) {

                std::visit(
                    [&](const auto &arg) {
                        using T =
                            std::decay_t<decltype(arg)>;

                        if constexpr (
                            std::is_same_v<
                                T,
                                NFA::DFATarget
                            >
                        ) {
                            if (arg.id == NFA::NULL_STATE) {
                                destination.transitions[symbol] =
                                    NFA::DFATarget{
                                        NFA::NULL_STATE
                                    };
                                return;
                            }

                            const std::size_t target_class =
                                partition_of.at(arg.id);

                            destination.transitions[symbol] =
                                NFA::DFATarget{
                                    class_to_new_index.at(
                                        target_class
                                    )
                                };
                        }
                        else if constexpr (
                            std::is_same_v<
                                T,
                                ActionSequence
                            >
                        ) {
                            ActionSequence sequence =
                                arg;

                            if (sequence.terminal_dfa_target !=
                                NFA::NULL_STATE) {

                                const std::size_t target_class =
                                    partition_of.at(
                                        sequence.terminal_dfa_target
                                    );

                                sequence.terminal_dfa_target =
                                    class_to_new_index.at(
                                        target_class
                                    );
                            }

                            destination.transitions[symbol] =
                                std::move(sequence);
                        }
                    },
                    target
                );
            }
        }

        // ================================================================
        // 5. Reachability on the MINIMIZED intermediate DFA.
        //
        // There is no ActionTable/SemanticTable to walk anymore.
        //
        // ActionSequence already directly exposes its terminal DFA state.
        // ================================================================

        std::vector<bool> reachable(
            minimized.size(),
            false
        );

        std::queue<std::size_t> q;

        if (!minimized.empty()) {
            reachable[0] = true;
            q.push(0);
        }

        while (!q.empty()) {
            const std::size_t current =
                q.front();

            q.pop();

            const auto &state =
                minimized[current];

            for (const auto &[symbol, target] :
                 state.transitions) {

                std::size_t next_state =
                    NFA::NULL_STATE;

                std::visit(
                    [&](const auto &arg) {
                        using T =
                            std::decay_t<decltype(arg)>;

                        if constexpr (
                            std::is_same_v<
                                T,
                                NFA::DFATarget
                            >
                        ) {
                            next_state = arg.id;
                        }
                        else if constexpr (
                            std::is_same_v<
                                T,
                                ActionSequence
                            >
                        ) {
                            next_state =
                                arg.terminal_dfa_target;
                        }
                    },
                    target
                );

                if (next_state == NFA::NULL_STATE)
                    continue;

                Assert(
                    next_state < minimized.size(),
                    "Minimized DFA state {} has invalid "
                    "transition target {}",
                    current,
                    next_state
                );

                if (!reachable[next_state]) {
                    reachable[next_state] = true;
                    q.push(next_state);
                }
            }
        }

        // ================================================================
        // 6. Compact reachable intermediate DFA.
        // ================================================================

        States<StateWithActions> compact(
            &nfa
        );

        std::vector<std::size_t> state_remap(
            minimized.size(),
            NFA::NULL_STATE
        );

        for (std::size_t i = 0;
             i < minimized.size();
             ++i) {

            if (reachable[i]) {
                state_remap[i] =
                    compact.makeNew();
            }
        }

        for (std::size_t i = 0;
             i < minimized.size();
             ++i) {

            if (!reachable[i])
                continue;

            const std::size_t new_idx =
                state_remap[i];

            const auto &source =
                minimized[i];

            auto &destination =
                compact[new_idx];

            destination.accept_binding =
                source.accept_binding;

            destination.accept_action =
                source.accept_action;

            for (const auto &[symbol, target] :
                 source.transitions) {

                std::visit(
                    [&](const auto &arg) {
                        using T =
                            std::decay_t<decltype(arg)>;

                        if constexpr (
                            std::is_same_v<
                                T,
                                NFA::DFATarget
                            >
                        ) {
                            if (arg.id == NFA::NULL_STATE)
                                return;

                            Assert(
                                arg.id < state_remap.size(),
                                "DFA target {} outside remap",
                                arg.id
                            );

                            const auto mapped =
                                state_remap[arg.id];

                            if (mapped != NFA::NULL_STATE) {
                                destination.transitions[symbol] =
                                    NFA::DFATarget{
                                        mapped
                                    };
                            }
                        }
                        else if constexpr (
                            std::is_same_v<
                                T,
                                ActionSequence
                            >
                        ) {
                            ActionSequence sequence =
                                arg;

                            if (sequence.terminal_dfa_target !=
                                NFA::NULL_STATE) {

                                Assert(
                                    sequence.terminal_dfa_target <
                                        state_remap.size(),
                                    "ActionSequence terminal "
                                    "target {} outside remap",
                                    sequence.terminal_dfa_target
                                );

                                const auto mapped =
                                    state_remap[
                                        sequence.terminal_dfa_target
                                    ];

                                if (mapped != NFA::NULL_STATE) {
                                    sequence.terminal_dfa_target =
                                        mapped;

                                    destination.transitions[symbol] =
                                        std::move(sequence);
                                }
                            }
                            else {
                                // Accept/terminal sequences are allowed
                                // to terminate in NULL_STATE.
                                destination.transitions[symbol] =
                                    std::move(sequence);
                            }
                        }
                    },
                    target
                );
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
            [&](const ActionSequence &sequence)
            -> TransitionTarget {

            Assert(
                !sequence.actions.empty(),
                "Attempting to materialize an empty action sequence"
            );

            struct Slot {
                NFA::TableType table_type;
                std::size_t index;
            };

            std::vector<Slot> slots;
            slots.reserve(
                sequence.actions.size()
            );

            // ------------------------------------------------------------
            // First allocate every table entry.
            //
            // This is done before wiring next_state so chains can freely
            // cross Action <-> Semantic boundaries.
            // ------------------------------------------------------------

            for (const auto &raw :
                 sequence.actions) {

                std::visit(
                    [&](const auto &entry) {
                        using T =
                            std::decay_t<decltype(entry)>;

                        if constexpr (
                            std::is_same_v<
                                T,
                                NFA::ActionState
                            >
                        ) {
                            // NOTE: SNAPSHOT_APPLY/SNAPSHOT_APPLY_END no
                            // longer "wrap" a single action's identity
                            // (see step F in DFA::build()). They are
                            // plain boundary markers around a run of
                            // ordinary FiredActions, and every action in
                            // that run - including the boundary markers
                            // themselves - already has its own entry in
                            // `sequence.actions`. No synthesis needed
                            // here: one table row per entry, same as
                            // everything else.
                            action_table.push_back(entry);

                            slots.push_back(
                                Slot{
                                    NFA::TableType::Action,
                                    action_table.size() - 1
                                }
                            );
                        }
                        else {
                            semantic_table.push_back(entry);

                            slots.push_back(
                                Slot{
                                    NFA::TableType::Semantic,
                                    semantic_table.size() - 1
                                }
                            );
                        }
                    },
                    raw
                );
            }

            // ------------------------------------------------------------
            // Then wire the chain.
            //
            // The order is exactly sequence.actions order.
            // ------------------------------------------------------------

            for (std::size_t i = 0;
                 i < slots.size();
                 ++i) {

                TransitionTarget next;

                if (i + 1 < slots.size()) {
                    const auto &next_slot =
                        slots[i + 1];

                    if (next_slot.table_type ==
                        NFA::TableType::Action) {

                        next =
                            NFA::ActionTarget{
                                next_slot.index
                            };
                    }
                    else {
                        next =
                            NFA::SemanticTarget{
                                next_slot.index
                            };
                    }
                }
                else {
                    next =
                        NFA::DFATarget{
                            sequence.terminal_dfa_target
                        };
                }

                const auto &slot =
                    slots[i];

                if (slot.table_type ==
                    NFA::TableType::Action) {

                    action_table[slot.index].next_state =
                        std::move(next);
                }
                else {
                    semantic_table[slot.index].next_state =
                        std::move(next);
                }
            }

            // ------------------------------------------------------------
            // Return the first table target.
            // ------------------------------------------------------------

            if (slots[0].table_type ==
                NFA::TableType::Action) {

                return NFA::ActionTarget{
                    slots[0].index
                };
            }

            return NFA::SemanticTarget{
                slots[0].index
            };
        };

        // ================================================================
        // Materialize one NextTarget.
        // ================================================================

        auto materialize_target =
            [&](const NextTarget &target)
            -> std::optional<TransitionTarget> {

            return std::visit(
                [&](const auto &arg)
                -> std::optional<TransitionTarget> {

                    using T =
                        std::decay_t<decltype(arg)>;

                    if constexpr (
                        std::is_same_v<
                            T,
                            NFA::DFATarget
                        >
                    ) {
                        return TransitionTarget{
                            arg
                        };
                    }
                    else {
                        return materialize_sequence(arg);
                    }
                },
                target
            );
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
            }
            else if (std::holds_alternative<NFA::SemanticTarget>(materialized)) {
                const auto target = std::get<NFA::SemanticTarget>(materialized);
                accept_binding->target_semantic_state = target.id;
                accept_binding->reduce_rule_id = std::nullopt;
            }
            else {
                // An accepting action sequence must contain at least one
                // action (see the Assert inside materialize_sequence), so
                // it can never resolve to a bare DFATarget.
                Assert(
                    false,
                    "Accept action sequence materialized to a direct "
                    "DFA target instead of an Action/Semantic target"
                );
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
                Assert(
                    destination.accept_binding.has_value(),
                    "DFA state {} has an accept action sequence but no "
                    "accept binding to attach it to",
                    i
                );

                materialize_accept_action(
                    destination.accept_binding,
                    *source.accept_action
                );
            }

            // ------------------------------------------------------------
            // Transitions
            // ------------------------------------------------------------
            for (const auto &[symbol, target] : source.transitions) {
                if (std::holds_alternative<NFA::DFATarget>(target)) {
                    destination.transitions[symbol] =
                        std::get<NFA::DFATarget>(target);
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

        for (std::size_t i = 0;
             i < states.size();
             ++i) {

            const auto &state =
                states[i];

            for (const auto &[symbol, target] :
                 state.transitions) {

                std::visit(
                    [&](const auto &arg) {
                        using T =
                            std::decay_t<decltype(arg)>;

                        if constexpr (
                            std::is_same_v<
                                T,
                                NFA::DFATarget
                            >
                        ) {
                            Assert(
                                arg.id == NFA::NULL_STATE ||
                                arg.id < states.size(),
                                "DFA state {} transition "
                                "has invalid DFA target {}",
                                i,
                                arg.id
                            );
                        }
                        else if constexpr (
                            std::is_same_v<
                                T,
                                NFA::ActionTarget
                            >
                        ) {
                            Assert(
                                arg.id <
                                    action_table.size(),
                                "DFA state {} transition "
                                "has invalid ActionTable index {}",
                                i,
                                arg.id
                            );
                        }
                        else if constexpr (
                            std::is_same_v<
                                T,
                                NFA::SemanticTarget
                            >
                        ) {
                            Assert(
                                arg.id <
                                    semantic_table.size(),
                                "DFA state {} transition "
                                "has invalid SemanticTable index {}",
                                i,
                                arg.id
                            );
                        }
                    },
                    target
                );
            }
        }

        return states;
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

        States<State<ClassTransitions>> output(&nfa);
        for (std::size_t i = 0; i < n; ++i) {
            auto new_idx = output.makeNew();
            output[new_idx].accept_binding = states[i].accept_binding;

            // Default terminal transition when no character shift exists in state i
            TransitionTarget default_trans{ NFA::DFATarget {NULL_STATE} };
            if (states[i].accept_binding.has_value()) {
                const auto &binding = *states[i].accept_binding;
                if (binding.reduce_rule_id.has_value()) {
                    default_trans = NFA::ActionTarget {
                        *binding.reduce_rule_id
                    };
                } else if (binding.target_semantic_state.has_value()) {
                    default_trans = NFA::SemanticTarget {
                        *binding.target_semantic_state
                    };
                } else {
                    default_trans = NFA::DFATarget {
                        binding.token_id
                    };
                }
            }

            output[new_idx].transitions.assign(table.num_classes, default_trans);

            for (const auto &[symbol, value] : states[i].transitions) {
                if (!std::holds_alternative<char>(symbol)) continue;
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
                        Assert(action_table.size() > act_idx, "Out of bound action index {} in state {}", act_idx, index);
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
                if (std::holds_alternative<NFA::DFATarget>(t)) {
                    os << "Class " << cls << " -> " << "DFA " << std::get<NFA::DFATarget>(t).id << '\n';
                } else if (std::holds_alternative<NFA::ActionTarget>(t)) {
                    os << "Class " << cls << " -> " << "Action " << std::get<NFA::ActionTarget>(t).id << '\n';
                } else if (std::holds_alternative<NFA::SemanticTarget>(t)) {
                    os << "Class " << cls << " -> " << "Semantic " << std::get<NFA::SemanticTarget>(t).id << '\n';
                }
                os << '\n';
            }
            ++index;
        }
        return os;
    }
}