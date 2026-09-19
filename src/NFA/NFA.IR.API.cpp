module NFA.IR.API;

import AST.API;
import corelib;
import std;

auto print_transition_values(std::ostream &os, const stdu::vector<NFA::IR::TransitionValue> &values) -> std::ostream & {
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0) {
            os << " | ";
        }
        os << values[i];
    }
    return os;
}

auto NFA::IR::operator<<(std::ostream &os, const TokenID &id) -> std::ostream & {
    if (id.member.empty()) {
        return os << "ACCEPT";
    }
    os << id.member << "#" << corelib::text::join(id.token_name, "::") << "{" << id.position_in_token << "}";
    if (id.group != NULL_STATE) {
        os << "$group[" << id.group << "]";
    }
    if (id.call != NULL_STATE) {
        os << "$call[" << id.call << "]";
    }
    if (!id.capture.empty()) {
        for (const auto &cap : id.capture) {
            os << "@cap[" << cap->member << "#" << corelib::text::join(cap->token_name, "::") << "{" << cap->position_in_token << "}]";
        }
    }
    return os;
}

auto NFA::IR::operator<<(std::ostream &os, const Transition &transition) -> std::ostream & {
    os << transition.first << " -> ";
    print_transition_values(os, transition.second);
    return os;
}

auto NFA::IR::operator<<(std::ostream &os, const Transitions &transitions) -> std::ostream & {
    for (const auto &t : transitions) {
        os << '\t' << t << "\n";
    }
    return os;
}

auto NFA::IR::operator<<(std::ostream &os, const Token &token) -> std::ostream & {
    os << corelib::text::join(token.name, "::") << "{\n";
    os << "\t[top level]: " << std::boolalpha << token.top_level << "\n";
    os << token.transitions << "\n";
    os << "}";
    return os;
}
