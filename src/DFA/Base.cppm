export module DFA.Base;

import LangAPI;
import NFA_OLD;
import DFA.API;
import DFA.States;
import hash;
import dstd;
import std;

export namespace DFA {
    class Base {
    protected:
        std::size_t empty_state = NULL_STATE;
        bool merged = false;
    public:
        Base() = default;
        Base(std::size_t empty_state, bool merged = false) : empty_state(empty_state), merged(merged) {}
        virtual ~Base() = default;

        auto getEmptyState(std::size_t stateIndex) const -> std::size_t;
        auto getEmptyStateIF(std::size_t stateIndex) const -> std::size_t;
        auto getEmptyState() -> std::size_t&;
        auto hasOneEmptyState() -> bool;
        auto getEmptyStateByDfaId(std::size_t dfaIndex) -> std::size_t;
        auto isMerged() -> bool;

        template<typename DfaType>
        auto getDfaNames(const DfaType &dfa) -> stdu::vector<stdu::vector<std::string>> {
            utype::unordered_set<stdu::vector<std::string>> names;
            for (const auto &state : dfa.get()) {
                names.insert(state.rule_name);
            };
            return {names.begin(), names.end()};
        }
        virtual auto check_dfa() -> void {};
    };
}
