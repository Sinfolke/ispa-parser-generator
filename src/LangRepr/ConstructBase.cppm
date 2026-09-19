export module ConstructBase;
import LangRepr.Holder;
import LexerBuilder;
import LLIR.IR;
import LangAPI;
import AST.Tree;
import NFA_OLD;
import DFA.API;

import dstd;
export namespace LangRepr {
    class ConstructBase {
    protected:
        Holder &holder;
        LexerBuilder &lexer_builder;
        LLIR::IR &ir;
        AST::Tree &tree;
        LangAPI::Symbol Token;
        std::size_t insideTypeCount = 0; // needed by ensureTypesNS to distinguish which symbol require namespace
        auto makeIntRValue(int v) -> std::shared_ptr<LangAPI::RValue>;
        auto ensureTypesNs(LangAPI::Type s) -> LangAPI::Type;
        auto ensureTypesNs(LangAPI::IspaLibSymbol s) -> LangAPI::IspaLibSymbol;
        auto ensureTypesNs(LangAPI::IspaLibFunctionCall s) -> LangAPI::IspaLibFunctionCall;
        auto ensureTypesNs(LangAPI::Symbol s) -> LangAPI::Symbol;
        auto ensureTypesNs(LangAPI::MakeTuple t) -> LangAPI::MakeTuple;
        auto ensureTypesNs(LangAPI::GetVariant t) -> LangAPI::GetVariant;
        auto ensureTypesNs(LangAPI::CheckVariant t) -> LangAPI::CheckVariant;
        auto ensureTypesNs(LangAPI::StorageSymbol s) -> LangAPI::StorageSymbol;
        auto ensureTypesNs(LangAPI::Inheritance s) -> LangAPI::Inheritance;
        auto ensureTypesNs(LangAPI::RValue r) -> LangAPI::RValue;
        auto ensureTypesNs(LangAPI::FunctionCall s) -> LangAPI::FunctionCall;
        auto ensureTypesNs(LangAPI::Lambda l) -> LangAPI::Lambda;
        auto ensureTypesNs(LangAPI::ExpressionValue ev) -> LangAPI::ExpressionValue;
        auto ensureTypesNs(LangAPI::Expression expr) -> LangAPI::Expression;
        auto ensureTypesNs(LangAPI::Variable v) -> LangAPI::Variable;
        auto ensureTypesNs(LangAPI::If s) -> LangAPI::If;
        auto ensureTypesNs(LangAPI::While s) -> LangAPI::While;
        auto ensureTypesNs(LangAPI::DoWhile s) -> LangAPI::DoWhile;
        auto ensureTypesNs(LangAPI::Switch s) -> LangAPI::Switch;
        auto ensureTypesNs(LangAPI::Throw s) -> LangAPI::Throw;
        auto ensureTypesNs(const LangAPI::Statement &s) -> LangAPI::Statement;
        auto ensureTypesNs(LangAPI::Statements stmts) -> LangAPI::Statements;
        auto extractRawSymbol(const LangAPI::Type &t) -> stdu::vector<LangAPI::Type>;
        auto createTypeToken() -> LangAPI::TypeAlias;
        auto createLexerClass() -> LangAPI::Class;
        ConstructBase(Holder &holder, LexerBuilder &lexer_builder, LLIR::IR &ir, AST::Tree &tree) : holder(holder), lexer_builder(lexer_builder), ir(ir), tree(tree) {}

        ~ConstructBase() {}
    private:
    };
}