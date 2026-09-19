export module NFA.TNFA.Interpreter;

import NFA.TNFA.API;
import AST.API;
import LangAPI;
import corelib;
import hash;
import dstd;
import std;

export namespace NFA::Interpreter {
    struct TNFAInterpreterResult {
        bool matched = false;
        std::size_t consumed_length = 0;
        std::size_t final_state = TNFA::NULL_STATE;
        std::optional<TNFA::TokenBinding> matched_token;
        std::unordered_map<std::string, std::string> captured_variables;
        std::unordered_map<std::string, stdu::vector<std::string>> array_captures;
        std::optional<LangAPI::Inheritance> constructed_instance;

        friend std::ostream &operator<<(std::ostream &os, TNFAInterpreterResult &r);
    };

    class TNFAInterpreter {
        const stdu::vector<TNFA::State> &states;

    public:
        explicit TNFAInterpreter(const stdu::vector<TNFA::State> &states) : states(states) {}
        TNFAInterpreterResult run(std::size_t start_state_id, std::string_view input) const;
    };

} // namespace NFA::TNFA::Interpreter

