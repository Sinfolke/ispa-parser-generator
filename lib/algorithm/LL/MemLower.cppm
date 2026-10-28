export module MemLower;

import AST.API;
import AST.Tree;
import Parser.Base;
import UAST.PyAST;
import hash;
import dstd;
import LangAPI;
import RuleLower;
import std;

// Lowers ONE rule member (and everything nested in it) of an LL grammar to
// LangAPI statements. LL specifics:
//   * no speculative parsing / rollback: every branch is chosen by lookahead;
//   * `A | B | C` becomes a decision tree over the minimal k that separates the
//     alternatives (FIRST_k from AST::Tree::getAlternativeLookahead);
//   * `( ... )?  *  +` are guarded by the FIRST_1 set of the group body;
//   * names/tokens use the stdlib combinators (required/optional/range/...).
export class MemLower : Parser::ParserBase {
    FunctionCtx *fun_ctx;
    AST::Tree *tree;
    std::size_t mem_idx;          // top-level member index inside the rule
    std::size_t depth = 0;        // nesting depth, for LoweredExport::depth

    static constexpr std::size_t max_lookahead = 32;

    struct Resolution {
        std::size_t k = 0;
        std::vector<std::set<AST::LLkSeq>> perAlt;
    };

    // ---- dispatch ----
    auto lower(const AST::RuleMember &mem) -> Parser::LowerMeta;
    auto lowerName(const AST::RuleMember &mem) -> Parser::LowerMeta;
    auto lowerGroup(const AST::RuleMember &mem) -> Parser::LowerMeta;
    auto lowerOp(const AST::RuleMember &mem) -> Parser::LowerMeta;
    void lowerStreamMatch(const AST::StreamMatch &match);
    auto lowerEmbeddedMatch(const UAST::py::IspaMatch &match, std::set<std::string>& locals) -> LangAPI::Statements;
    auto lowerSequence(const AST::RuleMember &mem) -> Parser::LowerMeta;
    auto lowerDispatch(const AST::RuleMember &mem) -> Parser::LowerMeta;
    auto lowerQuantified(const AST::RuleMember &mem) -> Parser::LowerMeta;
    auto lowerBody(const AST::RuleMember &mem) -> Parser::LowerMeta;

    // ---- LL(k) ----
    auto resolveLookahead(const stdu::vector<std::shared_ptr<AST::RuleMember>> &alts) -> Resolution;
    auto decisionTree(
        const Resolution &res,
        const std::vector<LangAPI::Statements> &bodies,
        const std::vector<std::size_t> &candidates,
        std::size_t level,
        std::size_t op_id,
        const std::vector<AST::LLkToken> &path,
        const LangAPI::Variable &matched
    ) -> LangAPI::Statements;
    auto firstTokens(const AST::RuleMember &mem) -> std::vector<AST::LLkToken>;
    auto firstGuard(const AST::RuleMember &mem) -> LangAPI::Expression;

    // ---- name calls ----
    auto combinatorArgs(const AST::RuleMember &mem) -> stdu::vector<LangAPI::Expression>;
    auto ruleCallable(const AST::RuleMember &mem) -> LangAPI::Expression;
    auto failureHandler(const AST::RuleMember &mem) -> LangAPI::Expression;
    auto getMatchSymbol(char quantifier) -> LangAPI::StdlibExports;
    auto ismemberOptional(char quantifier) -> bool;
    static auto nodeType(const AST::RuleMemberName &name) -> LangAPI::Type;

    // ---- emission helpers ----
    auto push(const LangAPI::Statement &stmt) -> void;
    auto comment(const AST::RuleMember &mem, const std::string &meaning) -> void;
    auto declare(const std::string &name, const LangAPI::Type &type) -> bool;
    auto bind(const std::string &name, const LangAPI::Type &type, const LangAPI::Expression &value) -> LangAPI::Variable;
    auto createValueAssignment(const std::string &name, const LangAPI::Expression &expr) -> LangAPI::VariableAssignment;
    auto composeValue(const AST::RuleMember &mem, const std::vector<Parser::LowerMeta> &parts) -> Parser::LowerMeta;
    auto tokenKindIs(const AST::LLkToken &token) -> LangAPI::Expression;
    static auto kindOfCurrent() -> LangAPI::Expression;
    static auto tokenIdSymbol(const AST::LLkToken &token) -> LangAPI::RValue;
    static auto hasValue(const Parser::LowerMeta &meta) -> bool;
    static auto isWhitespaceRef(const AST::RuleMember &mem) -> bool;
public:
    MemLower(const Parser::Ctx &ctx, FunctionCtx &fun_ctx, AST::Tree &tree, std::size_t mem_idx)
        : ParserBase(ctx), fun_ctx(&fun_ctx), tree(&tree), mem_idx(mem_idx) {}

    // Appends the lowered member to fun_ctx.statements and records the values
    // it produces in fun_ctx.exports.
    auto build(const AST::RuleMember &mem) -> void;
};
