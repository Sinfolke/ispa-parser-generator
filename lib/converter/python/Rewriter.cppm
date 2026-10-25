export module Python.Rewriter;

import LangAPI;
import std;
import dstd;

export namespace Python {
    // A semantic LangAPI->LangAPI lowering pass, independent of text emission.
    // run() clones the tree, including Class::data pointer-owned declarations.
    // Standalone ++/-- become +=/-=; value-producing ++/-- are left for Python.Core to lower.
    class Rewriter {
    public:
        struct Statistics {
            std::size_t increments = 0;
            std::size_t decrements = 0;
            std::size_t erased_any_types = 0;
        };
        LangAPI::Declarations run(const LangAPI::Declarations& input);
        const Statistics& statistics() const noexcept { return stats_; }

    private:
        Statistics stats_{};
        void declarations(LangAPI::Declarations& decls, const std::string& context);
        void declaration(LangAPI::Declaration& decl, const std::string& context);
        void statements(LangAPI::Statements& stmts, const std::string& context);
        void statement(LangAPI::Statement& stmt, const std::string& context);
        void expression(LangAPI::Expression& expr, const std::string& context);
        void expr_value(LangAPI::ExpressionValue& value, const std::string& context);
        void rvalue(LangAPI::RValue& value, const std::string& context);
        void symbol(LangAPI::Symbol& value, const std::string& context);
        void storage(LangAPI::StorageSymbol& value, const std::string& context);
        void function_call(LangAPI::FunctionCall& value, const std::string& context);
        void lib_call(LangAPI::IspaLibFunctionCall& value, const std::string& context);
        void type(LangAPI::Type& value, const std::string& context);
        void variable(LangAPI::Variable& value, const std::string& context);
        void lambda(LangAPI::Lambda& value, const std::string& context);
        void template_args(stdu::vector<std::variant<std::shared_ptr<LangAPI::Type>, std::shared_ptr<LangAPI::RValue>>>& args,
                           const std::string& context);
        bool increment_as_assignment(const LangAPI::Expression& expr, LangAPI::Expression& result,
                                     const std::string& context);
    };
}
