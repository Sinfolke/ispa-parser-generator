module DFA.closure;
import NFA.IR.API;
import logging;
import std;

namespace {

namespace TNFA = NFA::TNFA;
namespace InitialAPI = NFA::IR;

/*
 * ----------------------------------------------------------------------
 * Convert actions physically attached to one TNFA edge into an
 * ActionPath.
 *
 * table_index is only the position inside this particular ActionChain.
 * There is no global ActionTable/SemanticTable in TNFA anymore.
 * ----------------------------------------------------------------------
 */

auto transitionActions(
    const std::size_t owner,
    const TNFA::TransitionValue &edge
) -> DFA::ActionPath {

    DFA::ActionPath result;

    result.reserve(
        edge.actions.size()
    );

    for (std::size_t i = 0;
         i < edge.actions.size();
         ++i) {

        const auto &raw =
            edge.actions[i];

        const TNFA::TableType type =
            std::holds_alternative<TNFA::ActionState>(raw)
                ? TNFA::TableType::Action
                : TNFA::TableType::Semantic;

        result.push_back(
            DFA::FiredAction{
                .owner = owner,
                .table_type = type,
                .table_index = i,
                .raw = raw
            }
        );
    }

    return result;
}


/*
 * ----------------------------------------------------------------------
 * Append an action path.
 *
 * DO NOT deduplicate.
 *
 * If the same capture operation occurs twice because of repetition, those
 * are two executions and must remain two executions.
 * ----------------------------------------------------------------------
 */

void appendActions(
    DFA::ActionPath &dst,
    const DFA::ActionPath &src
) {
    dst.insert(
        dst.end(),
        src.begin(),
        src.end()
    );
}


/*
 * ----------------------------------------------------------------------
 * Symbol seeds
 * ----------------------------------------------------------------------
 *
 * For:
 *
 *     current DFA subset
 *             |
 *             | symbol
 *             v
 *       consuming TNFA edge
 *
 * produce:
 *
 *     { destination NFA state, action path }
 *
 * The action path contains ONLY actions physically attached to the
 * consuming edge.
 *
 * Epsilon actions are added later by epsilonClosure().
 * ----------------------------------------------------------------------
 */

} // namespace

auto DFA::Closure::collectSeeds(
    const TNFA::TNFABuilder *nfa,
    const std::vector<std::size_t> &current,
    const TNFA::TransitionKey &symbol
) -> std::vector<DFA::SymbolSeed> {
    std::vector<DFA::SymbolSeed> candidates;

    for (const std::size_t source : current) {
        const auto &state = nfa->getStates().at(source);
        const auto it = state.transitions.find(symbol);
        if (it == state.transitions.end())
            continue;

        for (const auto &edge : it->second) {
            if (edge.next == TNFA::NULL_STATE)
                continue;
            candidates.push_back(DFA::SymbolSeed{
                .priority = edge.priority,
                .source = source,
                .target = edge.next,
                .actions = transitionActions(source, edge),
                .source_link = &edge.source,
                .char_origin = edge.char_origin,
            });
        }
    }

    // Lower priority number wins; source is only a deterministic tie-breaker.
    std::ranges::stable_sort(candidates, [](const DFA::SymbolSeed &a, const DFA::SymbolSeed &b) {
        if (a.priority != b.priority)
            return a.priority < b.priority;
        return a.source < b.source;
    });
    return candidates;
}


/*
 * ==========================================================================
 * Closure
 * ==========================================================================
 */


/*
 * --------------------------------------------------------------------------
 * Plain epsilon closure.
 *
 * Every source starts with an empty action path.
 * --------------------------------------------------------------------------
 */

void DFA::Closure::epsilonClosure(
    const std::vector<std::size_t> &source
) {
    std::vector<std::pair<std::size_t, ActionPath>> seeded;

    seeded.reserve(
        source.size()
    );

    for (const std::size_t state : source) {
        seeded.emplace_back(
            state,
            ActionPath{}
        );
    }

    epsilonClosure(seeded);
}


/*
 * --------------------------------------------------------------------------
 * Epsilon closure with explicit action paths.
 *
 * The invariant is:
 *
 *     actions_for[state]
 *
 * contains the complete ordered action program required to reach `state`
 * from one of the supplied seeds.
 *
 * For symbol movement, the seed already contains actions from the
 * consuming edge.
 *
 * For ordinary epsilon closure, the seed path is empty.
 * --------------------------------------------------------------------------
 */

void DFA::Closure::epsilonClosure(
    const std::vector<
        std::pair<std::size_t, ActionPath>
    > &seeded_source
) {
    closure.clear();
    sorted_unique_closure.clear();
    actions_for.clear();
    terminal_actions_for.clear();
    transition_actions.clear();
    discovery.clear();
    seed_of.clear();
    std::size_t current_seed = 0;


    /*
     * A frame represents one currently explored TNFA state.
     *
     * We use explicit DFS instead of recursive calls because large
     * generated lexers can easily contain deep epsilon chains.
     */
    struct Frame {
        std::size_t state =
            TNFA::NULL_STATE;

        ActionPath path;

        std::vector<TNFA::TransitionValue> edges;

        std::size_t next_edge = 0;
    };


    std::vector<Frame> stack;


    /*
     * A state can have several incoming epsilon paths.
     *
     * Once a path has won for a state, later paths do not replace it.
     *
     * This is intentional: the TNFA priority ordering is resolved while
     * traversing the epsilon graph.
     */
    std::unordered_set<std::size_t> visited;


    /*
     * --------------------------------------------------------------
     * Enter one state.
     * --------------------------------------------------------------
     */

    auto enter =
        [&](const std::size_t state_id,
            ActionPath path) -> bool {

        if (state_id == TNFA::NULL_STATE)
            return false;

        if (!visited.insert(state_id).second)
            return false;


        /*
         * The path stored here is the complete path leading INTO this
         * state. It does not yet contain any outgoing epsilon action.
         */
        actions_for.emplace(
            state_id,
            path
        );
        discovery.push_back(state_id);
        seed_of.emplace(state_id, current_seed);

        sorted_unique_closure.insert(
            state_id
        );


        const auto &state =
            nfa->getStates().at(state_id);


        /*
         * Copy epsilon transitions because the unordered_set does not
         * provide traversal order.
         *
         * TNFA priority is explicitly encoded in TransitionValue.
         */
        std::vector<TNFA::TransitionValue> edges(
            state.epsilon_transitions.begin(),
            state.epsilon_transitions.end()
        );


        /*
         * Lower number = higher priority.
         */
        std::ranges::stable_sort(
            edges,
            [](const auto &a,
               const auto &b) {

                if (a.priority != b.priority)
                    return a.priority < b.priority;

                /*
                 * TransitionValue does not have a separate stable
                 * identity, so preserve the container's copied order
                 * for equal priorities.
                 */
                return false;
            }
        );


        stack.push_back(
            Frame{
                .state = state_id,
                .path = std::move(path),
                .edges = std::move(edges),
                .next_edge = 0
            }
        );

        return true;
    };


    /*
     * --------------------------------------------------------------
     * Drain the stack: run the DFS to completion for whatever is
     * currently on it.
     * --------------------------------------------------------------
     */

    auto drain = [&]() {
        while (!stack.empty()) {
            Frame &frame =
                stack.back();

            if (frame.next_edge >=
                frame.edges.size()) {

                stack.pop_back();
                continue;
            }


            /*
             * IMPORTANT:
             *
             * Copy the edge before modifying the stack.
             *
             * enter() pushes a new frame and may invalidate `frame`.
             */
            const TNFA::TransitionValue edge =
                frame.edges[
                    frame.next_edge++
                ];


            const ActionPath edge_actions =
                transitionActions(
                    frame.state,
                    edge
                );


            /*
             * ----------------------------------------------------------
             * Terminal (NULL_STATE) epsilon edges.
             *
             * A NULL_STATE target does not name another TNFA state to
             * continue into. This is how TNFA construction attaches a
             * "semantic state" directly to an already-reached state:
             * an LR reduce, a value-accumulation step, or one of
             * several disambiguation hypotheses produced when
             * multiple grammar members share a tail state (e.g. the
             * "==" / "!=" tail, or the "int" / "str" tail). It is a
             * leaf: nothing follows it in the TNFA.
             *
             * There is no destination state to race for, so this must
             * NOT be routed through enter()/visited: enter() rightly
             * refuses a NULL_STATE id outright (it is not a state),
             * and gating the action on that refusal is exactly what
             * silently discarded every such action - the reported
             * bug ("no str accumulated", "no semantic state
             * executed"). The edge belongs to `frame.state`, which
             * has already won its own entry, and each frame's edges
             * are drained exactly once, so the action fires exactly
             * once, unconditionally.
             *
             * Multiple terminal edges on the same state (e.g. both
             * the "int" and "str" hypotheses at their shared tail
             * state) are NOT deduplicated here for the same reason
             * appendActions() never deduplicates: which one is
             * actually meaningful is resolved from the recorded
             * actions downstream (by DFATarget/nfa_index), not by
             * Closure silently picking a winner.
             * ----------------------------------------------------------
             */
            if (edge.next == TNFA::NULL_STATE) {
                if (terminal_actions_for.contains(frame.state))
                    continue;   // a higher-priority terminal edge for this state already won
                appendActions(
                    terminal_actions_for[frame.state],
                    edge_actions
                );

                continue;
            }


            /*
             * The path entering the destination is:
             *
             *     path entering current state
             *              +
             *     actions attached to this epsilon edge
             */
            ActionPath next_path =
                frame.path;

            appendActions(
                next_path,
                edge_actions
            );


            /*
             * Only `edge_actions` is the delta introduced by THIS
             * edge. `next_path` is the full cumulative path into the
             * destination and must never be appended to
             * transition_actions - that would re-record every
             * ancestor action once per descendant.
             */
            if (enter(edge.next, std::move(next_path))) {
                appendActions(
                    transition_actions,
                    edge_actions
                );
            }
        }
    };


    /*
     * --------------------------------------------------------------
     * Seed the closure.
     *
     * The caller must provide seeds in priority order.
     *
     * Each seed's ENTIRE epsilon subtree must be explored to
     * completion before moving on to the next seed. A single shared
     * stack across all seeds, drained only after every seed has been
     * pushed, would process the LAST seed's subtree first (LIFO) -
     * meaning a lower-priority seed could claim a downstream state
     * before a higher-priority seed reaches it via a longer path,
     * silently discarding the higher-priority (correct) action
     * chain for that state. Draining per seed keeps priority order
     * intact both across seeds and within each seed's own subtree.
     * --------------------------------------------------------------
     */

    std::size_t seed_index = 0;
    for (const auto &[state, path] : seeded_source) {
        current_seed = seed_index++;

        /*
         * `path` here is the seed's own action path (e.g. the
         * consuming edge's actions, or empty for a plain epsilon
         * closure) - i.e. it IS the delta for this state, since
         * nothing precedes it. Record it once, only if this seed
         * actually wins entry (a higher-priority seed for the same
         * state may already have claimed it).
         */
        if (enter(state, path)) {
            appendActions(
                transition_actions,
                path
            );
        }

        drain();
    }



    /*
     * Materialize sorted closure.
     *
     * sorted_unique_closure is an unordered_set used for O(1)
     * membership tests; `closure` is the stable vector representation
     * used as the DFA subset key.
     */
    closure.assign(
        sorted_unique_closure.begin(),
        sorted_unique_closure.end()
    );

    std::ranges::sort(
        closure
    );
}


/*
 * ==========================================================================
 * Symbol move
 * ==========================================================================
 *
 * This is the important part.
 *
 * A symbol move does NOT first calculate destinations and then separately
 * calculate epsilon closure.
 *
 * Instead:
 *
 *     source
 *       |
 *       | 't'
 *       | actions attached to 't'
 *       v
 *     target
 *       |
 *       | epsilon
 *       | actions attached to epsilon edge
 *       v
 *     ...
 *
 * is passed into epsilonClosure() as one continuous action path.
 * ==========================================================================
 */

void DFA::Closure::move(
    const stdu::vector<std::size_t> &src,
    const TNFA::TransitionKey &symbol
) {
    const std::vector<std::size_t> current(
        src.begin(),
        src.end()
    );


    const auto seeds =
        collectSeeds(
            nfa,
            current,
            symbol
        );


    std::vector<
        std::pair<std::size_t, ActionPath>
    > seeded;

    seeded.reserve(
        seeds.size()
    );


    for (const auto &seed : seeds) {
        seeded.emplace_back(
            seed.target,
            seed.actions
        );
    }


    epsilonClosure(
        seeded
    );
}


/*
 * ==========================================================================
 * Constructors
 * ==========================================================================
 */

DFA::Closure::Closure(
    const TNFA::TNFABuilder *nfa,
    const stdu::vector<std::size_t> *current
)
    : nfa(nfa)
{
    if (!current)
        return;

    epsilonClosure(
        *current
    );
}


DFA::Closure::Closure(
    const TNFA::TNFABuilder *nfa,
    const stdu::vector<std::size_t> &current
)
    : nfa(nfa)
{
    epsilonClosure(
        current
    );
}


DFA::Closure::Closure(
    const TNFA::TNFABuilder *nfa,
    const std::vector<
        std::pair<std::size_t, ActionPath>
    > &seeded_current
)
    : nfa(nfa)
{
    epsilonClosure(
        seeded_current
    );
}


DFA::Closure::Closure(
    const TNFA::TNFABuilder *nfa,
    const stdu::vector<std::size_t> &current,
    const TNFA::TransitionKey &symbol
)
    : nfa(nfa)
{
    move(
        current,
        symbol
    );
}


/*
 * ==========================================================================
 * Queries
 * ==========================================================================
 */

auto DFA::Closure::contains(
    const std::size_t state
) const -> bool {
    return sorted_unique_closure.contains(
        state
    );
}


auto DFA::Closure::getActionsForState(
    const std::size_t state
) const -> const ActionPath & {

    static const ActionPath empty;

    const auto it =
        actions_for.find(state);

    if (it == actions_for.end())
        return empty;

    return it->second;
}


/*
 * --------------------------------------------------------------------------
 * Every action that fired while building this closure.
 *
 * Unlike getActionsForState(), which returns the cumulative path INTO
 * one particular state, this is the flat set of action chains attached
 * to every edge actually traversed during the DFS - each edge is
 * entered at most once (see the `visited` guard in enter()), so this
 * is exactly "everything that fires when this transition is taken",
 * with no missing epsilon-chained actions and no re-counted shared
 * prefixes.
 * --------------------------------------------------------------------------
 */

auto DFA::Closure::getTransitionActions() const -> const ActionPath & {
    return transition_actions;
}
auto DFA::Closure::getTerminalActionsForState(const std::size_t state) const -> const ActionPath & {
    static const ActionPath empty;

    const auto it =
        terminal_actions_for.find(state);

    if (it == terminal_actions_for.end())
        return empty;

    return it->second;
}