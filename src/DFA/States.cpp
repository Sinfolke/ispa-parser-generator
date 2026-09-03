module DFA.States;
import DFA.API;
import constants;
import std;

template<typename State>
auto DFA::States<State>::leadToEmptyState(std::size_t current, std::unordered_set<std::size_t> &visited) const -> std::size_t {
    if (!visited.insert(current).second) {
        return true; // already visited => prevent cycles, assume empty
    }
    const auto &state = nfa->getStates().at(current);
    if (!state.transitions.empty())
        return 0;
    if (state.epsilon_transitions.empty()) {
        return current;
    } else {
        for (const auto &t : state.epsilon_transitions) {
            if (!leadToEmptyState(t.next, visited)) {
                return 0;
            }
        }
        return current;
    }
}
template<typename State>
auto DFA::States<State>::makeNew() -> std::size_t {
    states.emplace_back();
    return states.size() - 1;
}
template<typename State>
auto DFA::States<State>::constructNewFrom(const State &state) -> std::size_t {
    states.emplace_back(state);
    return states.size() - 1;
}
template<typename State>
auto DFA::States<State>::empty() const -> bool {
    return states.empty();
}
template<typename State>
auto DFA::States<State>::size() const -> std::size_t {
    return states.size();
}
template<typename State>
auto DFA::States<State>::clear() -> void {
    states.clear();
}

template class DFA::States<DFA::StateWithActions>;
template class DFA::States<DFA::State<>>;
template class DFA::States<DFA::State<DFA::ClassTransitions>>;