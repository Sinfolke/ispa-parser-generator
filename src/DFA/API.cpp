module DFA.API;
import AST.API;
import AST.Pass;
import NFA.IR.API;
import NFA.TNFA.API;
import logging;
import cpuf.op;
import std;


// Priority ordering by real grammar position, read off whichever
// SourceLink carries a live AST::RuleMember. This replaces the old
// compareNameWithCharacter/compareNameWithName pair, which had to
// *synthesize* a fake AST::RuleMember from the (char | name) key because
// that was the only grammar information a NFA_OLD transition key carried.
// Now the real AST::RuleMember travels with the transition itself (as
// debug provenance), so there is nothing left to reconstruct.
auto DFA::Comparator::compareBySourceLink(const NFA::IR::TokenID &a, const NFA::IR::TokenID &b) const -> bool {
    if (a.member.empty() || b.member.empty()) {
        // No provenance on one (or both) sides - nothing grammar-shaped to
        // compare. Fall back to token_name so the ordering is at least
        // stable/deterministic.
        return a.token_name < b.token_name;
    }
    return AST::TreePass::prioritySort(tree, a.member, b.member);
}
auto DFA::Comparator::operator()(const NFA::TransitionKey &a, const NFA::TransitionKey &b) const -> bool {
    // NFA::TransitionKey is now a plain std::string label (see
    // NFA.TNFA.API); it no longer distinguishes "char" from "nested
    // name" the way NFA_OLD did, so there is no key-shape dispatch left
    // to do here. Kept only for callers that still need *some* ordering
    // over bare keys with no transition context available.
    return a < b;
}
auto DFA::Comparator::operator()(const std::pair<NFA::TransitionKey, TransitionValue> &a, const std::pair<NFA::TransitionKey, TransitionValue> &b) const -> bool {
    // Prefer real grammar-site ordering when both sides carry debug
    // provenance; otherwise fall back to plain key ordering.
    if (!a.second.debug.empty() && !b.second.debug.empty())
        return compareBySourceLink(a.second.debug.front(), b.second.debug.front());
    return operator()(a.first, b.first);
}

auto DFA::operator<<(std::ostream &os, const TransitionValue &value) -> std::ostream & {
    // os << "{next: " << value.next << ", accept_index: " << value.accept_index << ", actions: " << value.actions << "}";
    return os;
}

auto DFA::operator<<(std::ostream &os, const ActionSequence &sequence) -> std::ostream & {
    // os << "{actions: " << sequence.actions << ", terminal_dfa_target: " << sequence.terminal_dfa_target << "}";
    return os;
}

auto DFA::operator<<(std::ostream &os, const TransitionKeyExt &key) -> std::ostream & {
    // os << "{symbol: " << key.symbol
    //    << ", table_type: " << key.table_type
    //    << ", action: " << key.action
    //    << ", action_id: " << key.action_id
    //    << ", target_partition: " << key.target_partition
    //    << "}";
    return os;
}

auto DFA::operator<<(std::ostream &os, const StateWithActions &state) -> std::ostream & {
    // os << "{nfa_states: " << state.nfa_states
    //    << ", transitions: " << state.transitions
    //    << ", accept_action: " << state.accept_action
    //    << ", accept_binding: " << state.accept_binding
    //    << "}";
    return os;
}

auto DFA::operator<<(std::ostream &os, const CharClassTable &table) -> std::ostream & {
    // os << "{num_classes: " << table.num_classes << ", char_to_class: " << table.char_to_class << "}";
    return os;
}

template<typename TransitionType>
auto DFA::operator<<(std::ostream &os, const State<TransitionType> &state) -> std::ostream & {
    // os << "{nfa_states: " << state.nfa_states
    //    << ", transitions: " << state.transitions
    //    << ", accept_action: " << state.accept_action
    //    << ", accept_binding: " << state.accept_binding
    //    << "}";
    return os;
}

template auto DFA::operator<<(std::ostream &os, const State<Transitions> &state) -> std::ostream &;
template auto DFA::operator<<(std::ostream &os, const State<ClassTransitions> &state) -> std::ostream &;