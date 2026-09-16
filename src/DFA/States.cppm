export module DFA.States;

import DFA.API;
import NFA_OLD;
import dstd;
import std;

export namespace DFA {
    template<typename State>
    class States {
        const NFA::NFA *nfa = nullptr;
        stdu::vector<State> states;
        auto leadToEmptyState(std::size_t current, std::unordered_set<std::size_t> &visited) const -> std::size_t;
    public:
        States(const NFA::NFA *nfa) : nfa(nfa) {}
        States(const NFA::NFA *nfa, stdu::vector<State> states) : nfa(nfa), states(states) {}

        auto &get() const { return states; }
        auto &get()       { return states; }

        auto makeNew() -> std::size_t;
        auto constructNewFrom(const State &state) -> std::size_t;
        auto empty() const -> bool;
        auto size() const -> std::size_t;
        auto clear() -> void;

        auto begin() { return states.begin(); }
        auto end()   { return states.end();   }
        auto begin() const { return states.begin(); }
        auto end()   const { return states.end();   }

        auto &operator[](const std::size_t index) { return states.at(index); }
        auto &operator[](const std::size_t index) const { return states.at(index); }
    };

    template<typename StateType>
    auto operator<<(std::ostream &os, const States<StateType> &states) -> std::ostream &;
}
