module NFA.TNFA.API;

namespace NFA::TNFA {
    std::ostream &
operator<<(
    std::ostream &os,
    const NFA::TNFA::CharOrigin &o
) {
        const auto uc = static_cast<unsigned char>(o.ch);
        os << "(" << o.offset << "/" << o.length << " - ";

        if (std::isprint(uc)) {
            os << '\'' << o.ch << '\'';
        } else {
            os << "\\x"
               << std::hex
               << std::setw(2)
               << std::setfill('0')
               << static_cast<unsigned>(uc)
               << std::dec
               << std::setfill(' ');
        }
        os << " of " << o.source.member << ')';
        return os;
    }
    std::ostream &
operator<<(
    std::ostream &os,
    const NFA::TNFA::ActionTarget &t
) {
        os << "Action " << t.id << "(";
        os << t.debug;
        os << ")";
        return os;
    }
    std::ostream &
operator<<(
    std::ostream &os,
    const NFA::TNFA::SemanticTarget &t
) {
        os << "Semantic " << t.id << "(";
        os << t.debug;
        os << ")";
        return os;
    }
    std::ostream &
operator<<(
    std::ostream &os,
    const NFA::TNFA::DFATarget &t
) {
        os << "DFA " << t.id << "(";
        os << t.debug;
        os << ")";
        return os;
    }
    std::ostream &
operator<<(
    std::ostream &os,
    const NFA::TNFA::TokenBinding &b
) {
        os << "{token_id=" << b.token_id;
        if (b.reduce_rule_id) {
            os << ", reduce_rule_id="
               << *b.reduce_rule_id;
        }

        os << ", unique="
           << std::boolalpha
           << b.is_unique_representation
           << "}";

        return os;
    }
    std::ostream &
operator<<(
    std::ostream &os,
    const NFA::TNFA::ActionState &s
) {
        switch (s.action) {
            case NFA::TNFA::Action::UNDEF:
                os << "UNDEF";
                break;
            case NFA::TNFA::Action::BEGIN:
                os << "BEGIN";
                break;
            case NFA::TNFA::Action::END:
                os << "END";
                break;
        }
        os << ": " << s.variable;
        if (!s.debug_note.empty()) {
            os << " from {";
            for (std::size_t i = 0; i < s.debug_note.size(); ++i) {
                if (i != 0)
                    os << ", ";
                os << s.debug_note[i];
            }
            os << '}';
        }
        return os;
    }
    std::ostream &
operator<<(
    std::ostream &os,
    const NFA::TNFA::SemanticState &s
) {
        os << "{semantic instance of " << s.instance_value << " from " << s.debug_note << '}';
        return os;
    }
    std::ostream &
operator<<(
    std::ostream &os,
    const NFA::TNFA::TransitionValue &t
) {
        os << "{"
           << "next=" << t.next
           << ", priority=" << t.priority;

        if (!t.next_name.empty()) {
            os << ", next_name=" << t.next_name;
        }

        os << ", source=" << t.source;

        if (t.char_origin) {
            os << ", char_origin=" << *t.char_origin;
        }

        if (!t.fragment.empty()) {
            os << ", fragment=" << t.fragment;
        }

        if (!t.actions.empty()) {
            os << ", actions=[";

            for (std::size_t i = 0; i < t.actions.size(); ++i) {
                if (i != 0)
                    os << ", ";

                std::visit(
                    [&os](const auto &action) {
                        os << action;
                    },
                    t.actions[i]
                );
            }

            os << "]";
        }

        return os;
    }
    std::ostream &
operator<<(
    std::ostream &os,
    const NFA::TNFA::State &s
) {
        os << "State{"
           << "id=" << s.id
           << ", name=" << s.name;

        if (!s.origin.member.empty()) {
            os << ", origin=" << s.origin;
        }

        if (s.char_origin) {
            os << ", char_origin=" << *s.char_origin;
        }

        if (!s.debug_note.empty()) {
            os << ", note=" << s.debug_note;
        }

        if (s.accept_binding) {
            os << ", accept=" << *s.accept_binding;
        }

        os << ", transitions=" << s.transitions.size()
           << ", epsilon_transitions=" << s.epsilon_transitions.size()
           << "}";

        return os;
    }
}