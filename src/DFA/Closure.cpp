module DFA.closure;
import logging;
import std;

namespace {

using ActionPath = std::vector<DFA::FiredAction>;

bool sameAction(
    const DFA::FiredAction &a,
    const DFA::FiredAction &b
) {
    return
        a.owner == b.owner &&
        a.table_type == b.table_type &&
        a.table_index == b.table_index;
}

bool samePath(
    const ActionPath &a,
    const ActionPath &b
) {
    if (a.size() != b.size())
        return false;

    for (std::size_t i = 0; i < a.size(); ++i) {
        if (!sameAction(a[i], b[i]))
            return false;
    }

    return true;
}

bool containsPath(
    const std::vector<ActionPath> &paths,
    const ActionPath &path
) {
    return std::ranges::any_of(
        paths,
        [&](const ActionPath &existing) {
            return samePath(existing, path);
        }
    );
}

} // namespace


void DFA::Closure::epsilonClosure(
    const std::vector<std::size_t> &source
) {
    // Plain entry point: every source state starts with an empty pending
    // action path. This is the historical behaviour, kept as a thin
    // wrapper around the seeded overload below so existing call sites
    // don't need to change.
    std::vector<std::pair<std::size_t, ActionPath>> seeded_source;
    seeded_source.reserve(source.size());

    for (const std::size_t state : source) {
        seeded_source.emplace_back(state, ActionPath{});
    }

    epsilonClosure(seeded_source);
}

void DFA::Closure::epsilonClosure(
    const std::vector<std::pair<std::size_t, std::vector<FiredAction>>> &seeded_source
) {
    // ----------------------------------------------------------------
    // Seeded epsilon closure.
    //
    // Each entry in `seeded_source` is (NFA state, initial action path).
    // A non-empty initial path is how DFA::build() threads a deferred
    // SNAPSHOT_APPLY obligation (see DFA.cpp step D) forward into the
    // closure of the state that transition targets: the seed path is
    // simply prepended to whatever epsilon-derived actions this state
    // would already accumulate on its own.
    //
    // The plain epsilonClosure(source) overload above is exactly this
    // with an empty seed path for every state, so all pre-existing
    // behaviour is unchanged when no seed is supplied.
    // ----------------------------------------------------------------
    closure.clear();
    paths.clear();

    /*
     * Queue contains:
     *
     *     (NFA state, action path)
     *
     * rather than merely an NFA state.
     *
     * This is the important difference from the old implementation.
     */
    struct WorkItem {
        std::size_t state;
        ActionPath path;
    };

    std::queue<WorkItem> work;

    /*
     * We need to avoid infinite traversal through epsilon cycles.
     *
     * But the visited identity includes the ACTION PATH.
     *
     * Therefore:
     *
     *     X -> X
     *
     * doesn't loop forever, while
     *
     *     X -> BEGIN -> X
     *
     * is still treated as a different semantic path.
     *
     * Since actions are only attached to actual NFA states, and the
     * NFA is finite, paths containing the same action-state identity
     * repeatedly are not useful for deterministic transition
     * construction.
     *
     * This termination is enforced below via `paths`: a (state, exact
     * path) pair is only ever enqueued once — see the `containsPath`
     * check against `paths[target]` right before `work.push()`. Once a
     * state has recorded a given path, revisiting it with that same
     * path is a no-op fixed point, not a new traversal.
     */

    auto makeOwnActions =
        [&](std::size_t state_id) -> ActionPath {

        const auto &state =
            nfa->getStates().at(state_id);

        ActionPath result;
        result.reserve(state.actions.size());

        for (std::size_t i = 0;
             i < state.actions.size();
             ++i) {

            const auto &raw = state.actions[i];

            const NFA::TableType type =
                std::holds_alternative<NFA::ActionState>(raw)
                    ? NFA::TableType::Action
                    : NFA::TableType::Semantic;

            result.push_back(
                FiredAction{
                    .owner = state_id,
                    .table_type = type,
                    .table_index = i,
                    .raw = raw
                }
            );
        }

        return result;
    };
    auto containsAction =
        [](const ActionPath &path,
           const FiredAction &action) {

            return std::any_of(
                path.begin(),
                path.end(),
                [&](const FiredAction &existing) {
                    return sameAction(existing, action);
                }
            );
    };

    /*
     * Each source state begins with its seeded path (empty, unless this
     * closure was constructed to carry forward a deferred SNAPSHOT_APPLY
     * obligation for that specific state — see DFA.cpp step D).
     */
    for (const auto &[state, seed_path] : seeded_source) {
        if (state == NFA::NULL_STATE)
            continue;

        WorkItem item{
            .state = state,
            .path = seed_path
        };

        work.push(std::move(item));
    }


    /*
     * We keep the state set separately from the path set.
     *
     * A state can therefore occur in the closure multiple times
     * conceptually, each with a different path.
     */
    std::unordered_set<std::size_t> closure_seen;


    while (!work.empty()) {
        WorkItem item = std::move(work.front());
        work.pop();

        const std::size_t state_id = item.state;

        auto path = std::move(item.path);

        /*
         * Actions hosted by the state are executed when the epsilon
         * traversal ENTERS that state.
         *
         * Therefore they are appended before traversing its outgoing
         * epsilon edges.
         */
        const auto own_actions =
            makeOwnActions(state_id);

        for (const auto &action : own_actions) {

            if (containsAction(path, action))
                continue;

            path.push_back(action);
        }


        /*
         * Record this particular path.
         *
         * Do not union it with another path.
         */
        auto &state_paths = paths[state_id];

        if (!containsPath(state_paths, path)) {
            state_paths.push_back(path);
        }


        /*
         * The actual DFA closure contains the state only once.
         */
        if (closure_seen.insert(state_id).second) {
            closure.push_back(state_id);
        }


        const auto &state =
            nfa->getStates().at(state_id);


        /*
         * Deterministic epsilon traversal order.
         */
        std::vector<std::size_t> targets;

        targets.reserve(
            state.epsilon_transitions.size()
        );

        for (const auto &edge :
             state.epsilon_transitions) {

            if (edge.next != NFA::NULL_STATE)
                targets.push_back(edge.next);
        }

        std::sort(
            targets.begin(),
            targets.end()
        );


        for (const std::size_t target : targets) {

            /*
             * If this exact action path reaches target already,
             * there is nothing new to propagate.
             */
            const auto &target_paths =
                paths[target];

            if (containsPath(target_paths, path))
                continue;

            work.push(
                WorkItem{
                    .state = target,
                    .path = path
                }
            );
        }
    }
}


void DFA::Closure::move(
    const stdu::vector<std::size_t> &src,
    const NFA::TransitionKey &sym
) {
    std::unordered_set<std::size_t> result;

    for (const auto state_id : src) {

        const auto &state =
            nfa->getStates().at(state_id);

        const auto it =
            state.transitions.find(sym);

        if (it == state.transitions.end())
            continue;

        for (const auto &next : it->second) {

            if (next.next != NFA::NULL_STATE)
                result.insert(next.next);
        }
    }

    closure.assign(
        result.begin(),
        result.end()
    );
}


DFA::Closure::Closure(
    const NFA *nfa,
    const stdu::vector<std::size_t> *current
)
    : nfa(nfa)
{
    if (!current)
        return;

    epsilonClosure(*current);

    std::sort(
        closure.begin(),
        closure.end()
    );

    closure.erase(
        std::unique(
            closure.begin(),
            closure.end()
        ),
        closure.end()
    );
}


DFA::Closure::Closure(
    const NFA *nfa,
    const stdu::vector<std::size_t> &current
)
    : nfa(nfa)
{
    epsilonClosure(current);

    std::sort(
        closure.begin(),
        closure.end()
    );

    closure.erase(
        std::unique(
            closure.begin(),
            closure.end()
        ),
        closure.end()
    );
}


DFA::Closure::Closure(
    const NFA *nfa,
    const std::vector<std::pair<std::size_t, std::vector<FiredAction>>> &seeded_current
)
    : nfa(nfa)
{
    // See DFA.cpp step D: used when a DFA transition is reached through
    // NFA paths with disagreeing action sequences. Each kernel state is
    // seeded with its own (SNAPSHOT_APPLY-wrapped) deferred tail instead
    // of starting from an empty path, so the correct actions surface
    // later — attached to whichever future transition/accept actually
    // consumes that specific NFA state — without ever being dropped or
    // fired against the wrong position.
    epsilonClosure(seeded_current);

    std::sort(
        closure.begin(),
        closure.end()
    );

    closure.erase(
        std::unique(
            closure.begin(),
            closure.end()
        ),
        closure.end()
    );
}

DFA::Closure::Closure(
    const NFA *nfa,
    const stdu::vector<std::size_t> &current,
    const NFA::TransitionKey &symbol
)
    : nfa(nfa)
{
    move(current, symbol);

    /*
     * The transition itself has already consumed `symbol`.
     *
     * Therefore the actions discovered here belong to the state
     * AFTER the transition, not to the transition itself.
     */
    epsilonClosure(closure);

    std::sort(
        closure.begin(),
        closure.end()
    );

    closure.erase(
        std::unique(
            closure.begin(),
            closure.end()
        ),
        closure.end()
    );
}


auto DFA::Closure::contains(
    std::size_t state
) const -> bool {
    return std::binary_search(
        closure.begin(),
        closure.end(),
        state
    );
}


auto DFA::Closure::getPathsForState(
    std::size_t state
) const
    -> const std::vector<std::vector<FiredAction>> &
{
    static const std::vector<std::vector<FiredAction>> empty;

    const auto it = paths.find(state);

    if (it == paths.end())
        return empty;

    return it->second;
}


auto DFA::Closure::getUniquePathForState(
    std::size_t state
) const
    -> const std::vector<FiredAction> &
{
    const auto &state_paths =
        getPathsForState(state);

    if (state_paths.empty()) {

        static const std::vector<FiredAction> empty;

        return empty;
    }

    if (state_paths.size() != 1) {

        throw Error(
            "NFA state {} has {} distinct action paths",
            state,
            state_paths.size()
        );
    }

    return state_paths.front();
}