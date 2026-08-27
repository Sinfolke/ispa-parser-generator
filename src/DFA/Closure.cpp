module DFA.closure;
import std;

void DFA::Closure::epsilonClosure(const std::vector<std::size_t> &source) {
    std::queue<std::size_t> work;
    std::unordered_set<std::size_t> visited;
    std::vector<std::size_t> new_closure;

    for (std::size_t s : source) {
        if (visited.insert(s).second) {
            work.push(s);
            new_closure.push_back(s);
        }
    }

    while (!work.empty()) {
        std::size_t current_id = work.front();
        work.pop();

        const auto &epsilons = nfa.getStates().at(current_id).epsilon_transitions;

        for (const auto &edge : epsilons) {
            std::size_t target = edge.next;

            if (edge.table_type == NFA::TableType::Action) {
                if (edge.next >= nfa.getActionTable().size()) continue;
                target = nfa.getActionTable().at(edge.next).next_nfa_state;

                // Record action ONLY when discovering a new, unvisited target state
                if (target != NFA::NULL_STATE && !visited.contains(target)) {
                    fired.push_back({edge.table_type, edge.next});
                }
            } else if (edge.table_type == NFA::TableType::Semantic) {
                if (!visited.contains(edge.next)) {
                    fired.push_back({edge.table_type, edge.next});
                }
                continue;
            }

            if (target == NFA::NULL_STATE)
                continue;

            if (visited.insert(target).second) {
                new_closure.push_back(target);
                work.push(target);
            }
        }
    }
    closure = std::move(new_closure);
}

void DFA::Closure::move(const stdu::vector<std::size_t> &src, const NFA::TransitionKey &sym) {
    std::unordered_set<std::size_t> result;

    for (auto state_id : src) {
        const auto &state = nfa.getStates().at(state_id);
        auto it = state.transitions.find(sym);
        if (it != state.transitions.end()) {
            for (const auto &next_id : it->second) {
                result.insert(next_id.next);
            }
        }
    }

    closure.assign(result.begin(), result.end());
}

DFA::Closure::Closure(const NFA &nfa, const stdu::vector<std::size_t> *current) : nfa(nfa) {
    if (current) {
        epsilonClosure(*current);
        std::sort(closure.begin(), closure.end());
        closure.erase(std::unique(closure.begin(), closure.end()), closure.end());
    }
}

DFA::Closure::Closure(const NFA &nfa, const stdu::vector<std::size_t> &current) : nfa(nfa) {
    epsilonClosure(current);
    std::sort(closure.begin(), closure.end());
    closure.erase(std::unique(closure.begin(), closure.end()), closure.end());
}

DFA::Closure::Closure(const NFA &nfa, const stdu::vector<std::size_t> &current, const NFA::TransitionKey &symbol) : nfa(nfa) {
    move(current, symbol);
    epsilonClosure(closure);
    std::sort(closure.begin(), closure.end());
    closure.erase(std::unique(closure.begin(), closure.end()), closure.end());
}

auto DFA::Closure::contains(std::size_t state) const -> bool {
    return std::binary_search(closure.begin(), closure.end(), state);
}