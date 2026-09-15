module DFA.API;
import AST.API;
import AST.Pass;
import logging;
import cpuf.op;
import std;


auto DFA::Comparator::compareNameWithCharacter(const stdu::vector<std::string> &name, const char c) const -> bool {
    AST::RuleMember ast_name_as_member {.value = AST::RuleMemberName {name} };
    AST::RuleMember ast_char_as_member {.value = AST::String {std::string(1, c)} };
    return AST::TreePass::prioritySort(tree, ast_name_as_member, ast_char_as_member);
}
auto DFA::Comparator::compareNameWithName(const stdu::vector<std::string> &first_name, const stdu::vector<std::string> &second_name) const -> bool {
    AST::RuleMemberName ast_first_name_as_member(AST::RuleMemberName {first_name});
    AST::RuleMemberName ast_second_name_as_member(AST::RuleMemberName {second_name});
    return AST::TreePass::prioritySort(tree, ast_first_name_as_member, ast_second_name_as_member);
}
auto DFA::Comparator::operator()(const NFA::TransitionKey &a, const NFA::TransitionKey &b) const -> bool {
    if (std::holds_alternative<char>(a) && std::holds_alternative<char>(b))
        return 0;
    if (std::holds_alternative<stdu::vector<std::string>>(a) && std::holds_alternative<char>(b)) {
        return compareNameWithCharacter(std::get<stdu::vector<std::string>>(a), std::get<char>(b));
    }
    if (std::holds_alternative<char>(a) && std::holds_alternative<stdu::vector<std::string>>(b)) {
        return compareNameWithCharacter(std::get<stdu::vector<std::string>>(b), std::get<char>(a));
    }
    if (std::holds_alternative<stdu::vector<std::string>>(a) && std::holds_alternative<stdu::vector<std::string>>(b)) {
        return compareNameWithName(std::get<stdu::vector<std::string>>(a), std::get<stdu::vector<std::string>>(b));
    }
    throw Error("Undefined transition Key sort condition");
}
auto DFA::Comparator::operator()(const std::pair<NFA::TransitionKey, TransitionValue> &a, const std::pair<NFA::TransitionKey, TransitionValue> &b) const -> bool {
    return operator()(a.first, b.first);
}

auto DFA::operator<<(std::ostream &os, const TransitionValue &value) -> std::ostream & {
    os << "{next: " << value.next << ", accept_index: " << value.accept_index << ", actions: " << value.actions << "}";
    return os;
}

auto DFA::operator<<(std::ostream &os, const ActionSequence &sequence) -> std::ostream & {
    os << "{actions: " << sequence.actions << ", terminal_dfa_target: " << sequence.terminal_dfa_target << "}";
    return os;
}

auto DFA::operator<<(std::ostream &os, const TransitionKeyExt &key) -> std::ostream & {
    os << "{symbol: " << key.symbol
       << ", table_type: " << key.table_type
       << ", action: " << key.action
       << ", action_id: " << key.action_id
       << ", target_partition: " << key.target_partition
       << "}";
    return os;
}

auto DFA::operator<<(std::ostream &os, const StateWithActions &state) -> std::ostream & {
    os << "{nfa_states: " << state.nfa_states
       << ", transitions: " << state.transitions
       << ", accept_action: " << state.accept_action
       << ", accept_binding: " << state.accept_binding
       << "}";
    return os;
}

auto DFA::operator<<(std::ostream &os, const CharClassTable &table) -> std::ostream & {
    os << "{num_classes: " << table.num_classes << ", char_to_class: " << table.char_to_class << "}";
    return os;
}

template<typename TransitionType>
auto DFA::operator<<(std::ostream &os, const State<TransitionType> &state) -> std::ostream & {
    os << "{nfa_states: " << state.nfa_states
       << ", transitions: " << state.transitions
       << ", accept_action: " << state.accept_action
       << ", accept_binding: " << state.accept_binding
       << "}";
    return os;
}

template auto DFA::operator<<(std::ostream &os, const State<Transitions> &state) -> std::ostream &;
template auto DFA::operator<<(std::ostream &os, const State<ClassTransitions> &state) -> std::ostream &;