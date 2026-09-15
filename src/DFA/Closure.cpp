module DFA.closure;
import logging;
import std;

namespace {

// Transition actions are now the source of truth.  An action belongs to
// the edge that was selected, not to the state that happens to follow it.
//
// Ordinary actions are identified by operation + variable so cloned NFA
// fragments do not make the same capture operation fire twice along one
// path.  Semantic actions keep their structural transition identity.
bool sameAction(
    const DFA::FiredAction &a,
    const DFA::FiredAction &b
) {
    if (a.table_type != b.table_type)
        return false;

    if (a.table_type == NFA::TableType::Action) {
        const auto &aa = std::get<NFA::ActionState>(a.raw);
        const auto &bb = std::get<NFA::ActionState>(b.raw);

        return aa.action == bb.action &&
               aa.variable == bb.variable;
    }

    return a.owner == b.owner &&
           a.table_index == b.table_index;
}

DFA::ActionPath transitionActions(
    std::size_t owner,
    const NFA::TransitionValue &edge
) {
    DFA::ActionPath result;
    result.reserve(edge.actions.size());

    for (std::size_t i = 0; i < edge.actions.size(); ++i) {
        const auto &raw = edge.actions[i];

        const NFA::TableType type =
            std::holds_alternative<NFA::ActionState>(raw)
                ? NFA::TableType::Action
                : NFA::TableType::Semantic;

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

void appendActions(
    DFA::ActionPath &path,
    const DFA::ActionPath &actions
) {
    // TDFA actions are an ordered program.  Never deduplicate them:
    // repeated BEGIN/END/PUSH/REDUCE operations may be semantically
    // meaningful, especially across repetitions.
    path.insert(path.end(), actions.begin(), actions.end());
}

} // namespace


void DFA::Closure::epsilonClosure(
    const std::vector<std::size_t> &source
) {
    std::vector<std::pair<std::size_t, ActionPath>> seeded_source;
    seeded_source.reserve(source.size());

    for (const std::size_t state : source)
        seeded_source.emplace_back(state, ActionPath{});

    epsilonClosure(seeded_source);
}


void DFA::Closure::epsilonClosure(
    const std::vector<std::pair<std::size_t, ActionPath>> &seeded_source
) {
    closure.clear();
    sorted_unique_closure.clear();
    actions_for.clear();

    struct Frame {
        std::size_t state = NFA::NULL_STATE;
        ActionPath path;
        std::vector<NFA::TransitionValue> edges;
        std::size_t next = 0;
    };

    std::vector<Frame> stack;
    std::unordered_set<std::size_t> visited;

    auto commit = [&](std::size_t state_id, ActionPath incoming) {
        if (state_id == NFA::NULL_STATE)
            return;

        // First route wins.  This is the TDFA priority decision.
        if (!visited.insert(state_id).second)
            return;

        actions_for.emplace(state_id, incoming);
        sorted_unique_closure.insert(state_id);

        const auto &state = nfa->getStates().at(state_id);

        std::vector<NFA::TransitionValue> edges(
            state.epsilon_transitions.begin(),
            state.epsilon_transitions.end()
        );

        std::ranges::stable_sort(
            edges,
            [](const auto &a, const auto &b) {
                return a.priority < b.priority;
            }
        );

        stack.push_back(
            Frame{
                .state = state_id,
                .path = std::move(incoming),
                .edges = std::move(edges)
            }
        );
    };

    // Seed order is already priority order.  This matters when the same
    // NFA state occurs in more than one initial subset member.
    for (const auto &[state, path] : seeded_source)
        commit(state, path);

    while (!stack.empty()) {
        const std::size_t frame_index = stack.size() - 1;
        auto &frame = stack[frame_index];

        if (frame.next >= frame.edges.size()) {
            stack.pop_back();
            continue;
        }

        // Copy before commit(): pushing a frame may reallocate `stack`.
        const NFA::TransitionValue edge = frame.edges[frame.next++];
        ActionPath next_path = frame.path;

        // IMPORTANT: actions live on the selected epsilon edge now.
        // They must be appended before entering the target state.
        const ActionPath edge_actions =
            transitionActions(frame.state, edge);

        appendActions(next_path, edge_actions);
        commit(edge.next, std::move(next_path));
    }

    closure.assign(
        sorted_unique_closure.begin(),
        sorted_unique_closure.end()
    );
}


void DFA::Closure::move(
    const stdu::vector<std::size_t> &src,
    const NFA::TransitionKey &sym
) {
    sorted_unique_closure.clear();
    closure.clear();

    for (const auto state_id : src) {
        const auto &state = nfa->getStates().at(state_id);

        const auto it = state.transitions.find(sym);
        if (it == state.transitions.end())
            continue;

        for (const auto &next : it->second) {
            if (next.next != NFA::NULL_STATE)
                sorted_unique_closure.insert(next.next);
        }
    }

    closure.assign(
        sorted_unique_closure.begin(),
        sorted_unique_closure.end()
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
}


DFA::Closure::Closure(
    const NFA *nfa,
    const stdu::vector<std::size_t> &current
)
    : nfa(nfa)
{
    epsilonClosure(current);
}


DFA::Closure::Closure(
    const NFA *nfa,
    const std::vector<std::pair<std::size_t, ActionPath>> &seeded_current
)
    : nfa(nfa)
{
    epsilonClosure(seeded_current);
}


DFA::Closure::Closure(
    const NFA *nfa,
    const stdu::vector<std::size_t> &current,
    const NFA::TransitionKey &symbol
)
    : nfa(nfa)
{
    // A symbol transition may itself carry TDFA actions.  Therefore the
    // result of `move` cannot be represented only as a vector of states:
    // every destination has a potentially different incoming tag path.
    // Seed epsilon-closure directly with the actions of the selected
    // symbol edge.
    std::vector<std::pair<std::size_t, ActionPath>> seeded;

    for (const std::size_t state_id : current) {
        const auto &state = nfa->getStates().at(state_id);
        const auto it = state.transitions.find(symbol);

        if (it == state.transitions.end())
            continue;

        std::vector<NFA::TransitionValue> edges(
            it->second.begin(),
            it->second.end()
        );

        // If several transitions consume the same symbol, preserve their
        // explicit transition priority before entering the epsilon graph.
        std::ranges::stable_sort(
            edges,
            [](const auto &a, const auto &b) {
                return a.priority < b.priority;
            }
        );

        for (const auto &edge : edges) {
            if (edge.next == NFA::NULL_STATE)
                continue;

            seeded.emplace_back(
                edge.next,
                transitionActions(state_id, edge)
            );
        }
    }

    epsilonClosure(seeded);
}


auto DFA::Closure::contains(
    std::size_t state
) const -> bool {
    return sorted_unique_closure.contains(state);
}


auto DFA::Closure::getActionsForState(
    std::size_t state
) const -> const ActionPath & {
    static const ActionPath empty;

    const auto it = actions_for.find(state);
    if (it == actions_for.end())
        return empty;

    return it->second;
}
