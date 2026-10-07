export module LLIR.Builder.Base;
import AST.API;
import LangAPI;
import LLIR.API;
import LLIR.Builder.DataWrapper;
import LLIR.Builder.Utility;
import LLIR.Builder.Data;

import dstd;
import std;

export namespace LLIR {
    class BuilderBase : public BuilderDataWrapper, public BuilderUtility {
    public:
        // public static functions
        static auto createEmptyVariable(const std::string &name) -> LangAPI::Variable;
        static auto processExitStatements(LangAPI::Statements &values) -> void;
        static auto generateVariableName(std::size_t &variable_count) -> std::string;
        static auto createSuccessVariable(std::size_t &variable_count) -> LangAPI::Variable;
        static auto createAssignUvarBlock(LangAPI::Statements &statements, const LangAPI::Variable &uvar, const LangAPI::Variable &var, const LangAPI::Variable &shadow_var) -> void;
        static auto createDefaultStatements(const LangAPI::Variable &var, const LangAPI::Variable &svar, stdu::vector<std::string> name = {}) -> LangAPI::Statements;
        static auto createDefaultStatements(const LangAPI::Variable &svar) -> LangAPI::Statements;
        static auto createDefaultStatements() -> LangAPI::Statements;
        static auto createDefaultCall(LangAPI::Statements &block, const LangAPI::Variable &var, const std::string &name, LangAPI::Expression &expr) -> LangAPI::Statement;
        static auto increasePos() -> LangAPI::Expression;
        static auto getPrefixAsExpressionValue(const std::optional<char> &prefix) -> LangAPI::ExpressionValue;
        static void pushVariablePrefix(LangAPI::Expression &expr, const std::optional<char> &prefix);
        static void undoRuleResult(LangAPI::ValueType &type);

        // cll helper functions
        static auto CllTypeToIR(const AST::CllType &type) -> LangAPI::Type;
        static auto CllOpToIR(char op) -> LangAPI::OperatorType;
        static auto CllOpToExpr(char op) -> LangAPI::ExpressionElement;
        static auto CllCompareOpToExpr(const AST::CllCompareOp &op) -> LangAPI::ExpressionElement;
        static auto CllLogicalOpToIR(const AST::CllLogicalOp &lop) -> LangAPI::ExpressionElement;
        static auto CllAssignmentOpToIR(char op) -> LangAPI::OperatorType;

        // helper functions to create structures
        auto assignSvar(const LangAPI::Variable &svar, bool value) -> LangAPI::Statement;
        auto getBuilderData() -> BuilderDataWrapper;

        // deduce type functions
        static auto deduceUvarType(const LangAPI::Variable &var, const LangAPI::Variable &shadow_var) -> LangAPI::Type;
        static auto deduceVarTypeByRuleMember(const AST::RuleMember &mem) -> LangAPI::Type;
    protected:
        LangAPI::Statements statements;
        stdu::vector<LLIR::ExportsAfterBuild> exports_list;
        // helper functions
        auto generateVariableName() -> std::string;
        auto createSuccessVariable() -> LangAPI::Variable;
        auto addPostLoopCheck(const AST::RuleMember &rule, const LangAPI::Variable &var, bool addError = true) -> void;
        auto handle_plus_qualifier(const AST::RuleMember &rule, LangAPI::ConditionalElement loop, const LangAPI::Variable &uvar, const LangAPI::Variable &var, LangAPI::Variable &shadow_var, bool addError = true) -> void;
        auto add_shadow_variable(LangAPI::Statements &block, LangAPI::Statements &statements, const LangAPI::Variable &var) -> LangAPI::Variable;
        virtual auto pushBasedOnQualifier(const AST::RuleMember &rule, LangAPI::Expression &expr, LangAPI::Statements &stmt, LangAPI::Variable &uvar, const LangAPI::Variable &var, const LangAPI::Variable &svar, char quantifier, bool add_shadow_var, stdu::vector<std::string> name = {}) -> LangAPI::Variable;
        void pushConvResult(const AST::RuleMember &rule, const LangAPI::Variable &var, const LangAPI::Variable &uvar, const LangAPI::Variable &svar, const LangAPI::Variable &shadow_var, char quantifier);
        // error handling functions
        auto getNextTerminal(stdu::vector<std::shared_ptr<AST::RuleMember>> symbols, std::size_t pos) ->  std::set<stdu::vector<std::string>>;
        auto getErrorName(const AST::RuleMember &rule) -> std::string;
        auto getLookaheadTerminals(const AST::RuleMember& symbols, const stdu::vector<std::string> &lhs_name) -> BuilderData::SymbolFollow;

        // raise vars on top functions
        void getVariablesToTable(LangAPI::Statements &data, LangAPI::Statements& table, std::string &var_name, bool retain_value, bool recursive);
        static void insertVariablesOnTop(LangAPI::Statements &insertPlace, LangAPI::Statements& table);
        void raiseVarsTop(LangAPI::Statements &insertPlace, LangAPI::Statements &readPlace, std::string var_name = "", bool all_rule = false, bool retain_value = true, bool recursive = true);
    public:
        virtual void build() = 0;
        // interaction functions
        // void push(LLIR::member mem);
        void pop() { statements.pop_back(); };
        // void add(LLIR::Nodes block);

        virtual auto getData() const -> const LangAPI::Statements&;
        virtual auto getData() -> LangAPI::Statements&;
        virtual auto getReturnVars() const -> const stdu::vector<LLIR::ExportsAfterBuild>&;
        void optimizeIR();

        BuilderBase(BuilderDataWrapper& data) : BuilderDataWrapper(data), BuilderUtility(statements) {}
        virtual ~BuilderBase() = default;
    };
}