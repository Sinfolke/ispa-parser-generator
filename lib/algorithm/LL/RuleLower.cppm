export module RuleLower;
import AST.API;
import AST.Tree;
import Parser.Base;
import LangAPI;
import hash;
import std;

// One lowered member that produced a value. Later stages (data block / AST
// construction) look values up here by prefix name or by position instead of
// re-deriving variable names.
export struct LoweredExport {
    AST::RulePrefix prefix;
    Parser::LowerMeta meta;
    char quantifier = '\0';
    std::size_t member_index = 0; // index in the rule's top-level sequence
    std::size_t depth = 0;      // 0 = top-level member of the rule
};

// State shared by every MemLower of ONE rule function. MemLower objects are
// short-lived (one per top-level member) and ParserBase copies Parser::Ctx, so
// anything that must survive between members lives here.
export struct FunctionCtx {
    LangAPI::Statements *statements = nullptr;   // current emission target
    LangAPI::Statements declarations;            // hoisted variable declarations
    std::vector<LoweredExport> exports;
    std::vector<LoweredExport> captures;
    AST::LLkToken rule_name;                     // rule being lowered (for lookahead / recovery)
    std::size_t next_id = 0;                     // unique suffixes for generated names
    utype::unordered_map<std::vector<std::string>, std::size_t> variables_lowered;
    std::set<std::string> action_locals;
    std::set<std::string> action_objects;
    std::map<std::string, LangAPI::Type> action_types;   // `&name` binding -> type of the captured value

    auto uid() -> std::size_t { return next_id++; }
};

// Persistent result, independent of the transient emission targets in FunctionCtx.
export struct LoweredFunction {
    LangAPI::Function function;
    std::vector<LoweredExport> exports;
    std::vector<LoweredExport> captures;
};

export class RuleLower : Parser::ParserBase {
    utype::unordered_map<std::vector<std::string>, LoweredFunction> lowered_functions;
    AST::Tree *tree;
    auto register_function(const LangAPI::Function &fun) -> void;
public:
    RuleLower(Parser::Ctx &ctx, AST::Tree &tree) : ParserBase(ctx), tree(&tree) {}
    const auto& declarations() const { return decls; }
    const auto& lowered_functions_meta() const { return lowered_functions; }
    void build(std::vector<std::string> name, const AST::Rule &rule);
    void build(const AST::Tree &tree);
};
