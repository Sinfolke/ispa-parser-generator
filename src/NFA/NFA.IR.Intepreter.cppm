export module NFA.IR.Interpreter;

import NFA.IR.API;
import AST.API;
import corelib;
import hash;
import dstd;
import std;

export namespace NFA::Interpreter {

struct ParsedASTNode {
    stdu::vector<std::string> rule_name;
    std::string lexeme;
    std::size_t start_pos = 0;
    std::size_t end_pos = 0;
    stdu::vector<ParsedASTNode> children;

    auto operator==(const ParsedASTNode &) const -> bool = default;

    friend auto operator<<(std::ostream &os, const ParsedASTNode &node) -> std::ostream & {
        return printIndented(os, node, 0);
    }

private:
    static auto printIndented(std::ostream &os, const ParsedASTNode &node, std::size_t depth) -> std::ostream & {
        std::string indent(depth * 2, ' ');
        os << indent << corelib::text::join(node.rule_name, "::")
           << " [\"" << node.lexeme << "\"] (" << node.start_pos << ".." << node.end_pos << ")";
        if (!node.children.empty()) {
            os << " {\n";
            for (const auto &child : node.children) {
                printIndented(os, child, depth + 1);
            }
            os << indent << "}\n";
        } else {
            os << "\n";
        }
        return os;
    }
};

struct ParseResult {
    bool success = false;
    std::string input;
    std::size_t consumed_bytes = 0;
    stdu::vector<ParsedASTNode> ast_nodes;

    friend auto operator<<(std::ostream &os, const ParseResult &res) -> std::ostream & {
        os << "ParseResult { success: " << (res.success ? "true" : "false")
           << ", input: \"" << res.input << "\", consumed: " << res.consumed_bytes << " }\n";
        for (const auto &node : res.ast_nodes) {
            os << node;
        }
        return os;
    }
};

class IRInterpreter {
    InitialAPI::Token main_token;

    struct TraceStep {
        InitialAPI::TokenID state;
        std::size_t pos = 0;
        std::string matched_text;
    };

    static auto matchMember(const AST::RuleMember *member, std::string_view input, std::size_t pos) -> std::size_t;

    static auto buildASTFromTrace(const std::vector<TraceStep> &trace) -> stdu::vector<ParsedASTNode>;

    auto dfs(const InitialAPI::TokenID &curr_state,
             std::string_view input,
             std::size_t pos,
             std::vector<TraceStep> &current_trace,
             utype::unordered_set<std::pair<NFA::InitialAPI::TokenID, std::size_t>> &visited) const -> std::optional<std::vector<TraceStep>>;

public:
    explicit IRInterpreter(NFA::InitialAPI::Token token) : main_token(std::move(token)) {}

    auto parse(std::string_view input) const -> ParseResult;

    auto parseAll(const stdu::vector<std::string> &inputs) const -> stdu::vector<ParseResult>;
};

} // namespace NFA::Interpreter