module DFA.closure;
import std;

void DFA::Closure::epsilonClosure(const stdu::vector<std::size_t> &source) {
    std::queue<std::size_t> work;
    std::unordered_set<std::size_t> new_closure;
    new_closure.insert(source.begin(), source.end());
    for (std::size_t s : source) work.push(s);

    while (!work.empty()) {
        std::size_t current_id = work.front();
        work.pop();

        const auto &epsilons = nfa.getStates().at(current_id).epsilon_transitions;

        for (const auto &edge : epsilons) {
            std::size_t target = edge.next;

            if (edge.table_type == NFA::TableType::Action) {
                fired.push_back({edge.table_type, edge.next});
                target = nfa.getActionTable().at(edge.next).next_nfa_state;
            } else if (edge.table_type == NFA::TableType::Semantic) {
                fired.push_back({edge.table_type, edge.next});
                continue; // Semantic epsilon edges only occur via markAccept on accept
                // states; there's no NFA-space continuation field for them.
            }

            if (target == NFA::NULL_STATE)
                continue;

            if (!new_closure.contains(target)) {
                new_closure.insert(target);
                work.push(target);
            }
        }
    }
    closure.assign(new_closure.begin(), new_closure.end());
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