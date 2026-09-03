export module LangRepr.ConstructLexer;
import NFA;
import DFA.API;
import LLIR.IR;
import AST.API;
import LangRepr.Holder;
import ConstructBase;
import LexerBuilder;
import LangAPI;
import DFA.States;
import DFA.API;
import AST.Tree;
import dstd;
import std;
export namespace LangRepr {
    class ConstructLexer : ConstructBase {
    public:
        struct DfaSpan {
            std::size_t start;
            std::size_t length;
        };
        auto makeCharClassTableDecl(const DFA::CharClassTable &table)
            -> std::pair<std::shared_ptr<LangAPI::Declaration>, LangAPI::Visibility>;
        auto makeDfaTableDecl(
            const stdu::vector<DFA::State<DFA::ClassTransitions>> &states,
            std::size_t state_count,
            std::size_t class_count
        ) -> std::pair<std::shared_ptr<LangAPI::Declaration>, LangAPI::Visibility>;
        auto makeAcceptTableDecl(const auto &states, std::size_t state_count)
            -> std::pair<std::shared_ptr<LangAPI::Declaration>, LangAPI::Visibility>;
        auto makeConstructTableDecl(const auto &states)
            -> std::pair<std::shared_ptr<LangAPI::Declaration>, LangAPI::Visibility>;
        auto makeLRTableDecl(
            const stdu::vector<NFA::ActionState>& states,
            std::size_t state_count
        ) -> std::pair<std::shared_ptr<LangAPI::Declaration>, LangAPI::Visibility>;
        auto makeSemanticSwitchFunction(const stdu::vector<NFA::SemanticState> semantic_table) -> LangAPI::Function;
        auto constructLexer() -> void;
        ConstructLexer(Holder &holder, LexerBuilder &lexer_builder, LLIR::IR &ir, AST::Tree &tree) : ConstructBase(holder, lexer_builder, ir, tree) {}
    };
}