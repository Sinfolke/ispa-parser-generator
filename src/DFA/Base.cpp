module DFA.Base;
import LexerBuilder;
import logging;
import hash;
import std;
auto DFA::Base::getTransitionKeyType(const NFA::TransitionKey &transition_key) -> DfaType {
    if (std::holds_alternative<char>(transition_key)) {
        return DfaType::Char;
    } else {
        return DfaType::Token;
    }
}
auto DFA::Base::getEmptyState() -> std::size_t& {
    if (empty_state == NULL_STATE)
        throw Error("Dfa has no empty state registered");
    return empty_state;
}

auto DFA::Base::hasOneEmptyState() -> bool {
    return empty_state != NULL_STATE;
}
auto DFA::Base::isMerged() -> bool {
    return merged;
}