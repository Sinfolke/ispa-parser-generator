module DFA.closure;
import logging;
import std;

namespace {

using ActionPath = std::vector<DFA::FiredAction>;

bool sameAction(
    const DFA::FiredAction &a,
    const DFA::FiredAction &b
) {
    // Physical NFA owners are not semantic action identity. Nested-token
    // cloning creates different NFA states for the same capture operation.
    // For ordinary actions use the operation + variable as the identity so
    // those clones collapse during epsilon-path exploration. Snapshot
    // markers remain identified by their structural owner/index.
    if (a.table_type != b.table_type)
        return false;

    if (a.table_type == NFA::TableType::Action) {
        const auto &aa =
            std::get<NFA::ActionState>(a.raw);
        const auto &bb =
            std::get<NFA::ActionState>(b.raw);

        return
            aa.action == bb.action &&
            aa.variable == bb.variable;
    }

    return
        a.owner == b.owner &&
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
    closure.clear();
    paths.clear();

    struct WorkItem {
        std::size_t state;
        ActionPath path;
        std::vector<std::size_t> epsilon_stack;
    };

    std::queue<WorkItem> work;

    // Each state's own action list never changes across visits — only the
    // incoming `path` differs. Rebuilding it (and re-copying every
    // ActionState/SemanticState inside it) on every visit was pure waste
    // for states reached via several distinct paths. Memoize it once per
    // closure computation instead.
    std::unordered_map<std::size_t, ActionPath> own_actions_cache;

    auto makeOwnActions =
        [&](std::size_t state_id) -> const ActionPath & {

        if (auto it = own_actions_cache.find(state_id);
            it != own_actions_cache.end()) {
            return it->second;
        }

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

        return own_actions_cache
            .emplace(state_id, std::move(result))
            .first->second;
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

    for (const auto &[state, seed_path] : seeded_source) {
        if (state == NFA::NULL_STATE)
            continue;

        WorkItem item{
            .state = state,
            .path = seed_path,
            .epsilon_stack = {state}
        };

        work.push(std::move(item));
    }

    std::unordered_set<std::size_t> closure_seen;

    while (!work.empty()) {
        WorkItem item = std::move(work.front());
        work.pop();

        const std::size_t state_id = item.state;

        auto path = std::move(item.path);

        const auto &own_actions =
            makeOwnActions(state_id);

        path.reserve(path.size() + own_actions.size());

        for (const auto &action : own_actions) {

            if (containsAction(path, action))
                continue;

            path.push_back(action);
        }

        auto &state_paths = paths[state_id];

        if (!containsPath(state_paths, path)) {
            state_paths.push_back(path);
        }

        if (closure_seen.insert(state_id).second) {
            closure.push_back(state_id);
        }

        const auto &state =
            nfa->getStates().at(state_id);

        std::vector<std::size_t> targets;
        targets.reserve(state.epsilon_transitions.size());

        for (const auto &edge : state.epsilon_transitions) {
            if (edge.next != NFA::NULL_STATE)
                targets.push_back(edge.next);
        }

        std::sort(targets.begin(), targets.end());

        // Split into "actually needs to be enqueued" vs. "already
        // covered / cycle-closing", so the queue-push loop below knows
        // in advance which target is last and can move `path` into it
        // instead of copying.
        std::vector<std::size_t> pending_targets;
        pending_targets.reserve(targets.size());

        for (const std::size_t target : targets) {

            const auto &target_paths = paths[target];

            if (containsPath(target_paths, path))
                continue;

            if (std::ranges::find(
                    item.epsilon_stack,
                    target
                ) != item.epsilon_stack.end()) {

                auto &cycle_paths = paths[target];

                if (!containsPath(cycle_paths, path))
                    cycle_paths.push_back(path);

                continue;
            }

            pending_targets.push_back(target);
        }

        for (std::size_t k = 0; k < pending_targets.size(); ++k) {
            const std::size_t target = pending_targets[k];

            auto next_stack = item.epsilon_stack;
            next_stack.push_back(target);

            const bool is_last = (k + 1 == pending_targets.size());

            work.push(
                WorkItem{
                    .state = target,
                    .path = is_last ? std::move(path) : path,
                    .epsilon_stack = std::move(next_stack)
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