export module ConstructBase;
import LangRepr.Holder;
import LexerBuilder;
import LLIR.IR;
import LangAPI;
import NFA;
import DFA.API;

import dstd;
export namespace LangRepr {
    class ConstructBase {
    protected:
        Holder &holder;
        LexerBuilder &lexer_builder;
        LLIR::IR &ir;

        LangAPI::Symbol Token;
        auto makeIntRValue(int v) -> std::shared_ptr<LangAPI::RValue>;
        auto ensureTypesNs(LangAPI::Type s) -> LangAPI::Type;
        auto ensureTypesNs(LangAPI::Symbol s) -> LangAPI::Symbol;
        auto ensureTypesNs(LangAPI::MakeTuple t) -> LangAPI::MakeTuple;
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
        auto getTransitionCount(const std::variant<DFA::FullCharTable, DFA::SortedTransitions> &transition) -> std::size_t;
        auto getStateType(DFA::DfaType state_type) -> LangAPI::IspaLibSymbol;
        auto buildLambdaContent(
            LangAPI::Symbol builder_sym,
            const NFA::TemplatedDataBlockValue &data_block,
            long long N
        ) -> LangAPI::StorageSymbol;
        auto makeEmptyStateLambda(const stdu::vector<std::string> &name, const stdu::vector<std::string> &clear_name) -> LangAPI::Lambda;
        ConstructBase(Holder &holder, LexerBuilder &lexer_builder, LLIR::IR &ir) : holder(holder), lexer_builder(lexer_builder), ir(ir) {}

        ~ConstructBase() {}
    private:
    };
}