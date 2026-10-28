export module Python.CoreFunctions;

import LangAPI;
import std;
import dstd;

export namespace Python {
    // All conversion state is per Construct, unlike Cpp.CoreFunctions globals.
    class Core {
        std::vector<std::string> counters_{"pos"};
        std::string module_name_;
        std::size_t lambda_number_ = 0;
        std::unordered_set<std::string> method_members_;
        std::unordered_set<std::string> generic_functions_;
    public:
        struct LambdaDefinition { std::string name, params, body; };
    private:
        std::vector<LambdaDefinition> pending_lambdas_;
        // Renders a lambda body; receives the (already escaped) lambda parameter names.
        using StatementWriter = std::function<std::string(const LangAPI::Statements&, const std::vector<std::string>&)>;
        StatementWriter statement_writer_;
        // Lexical bookkeeping for `nonlocal`: one entry per enclosing def (function, constructor, lambda).
        std::vector<std::unordered_set<std::string>> scopes_;    // names bound in that def
        std::vector<std::unordered_set<std::string>> assigned_;  // plain names assigned in that def
        std::string symbol(const LangAPI::Symbol& symbol);
        std::string storage(const LangAPI::StorageSymbol& symbol);
        std::string lib(const LangAPI::IspaLibSymbol& symbol);
        std::string rvalue(const LangAPI::RValue& value);
        std::string call(const LangAPI::FunctionCall& value);
        std::string lib_call(const LangAPI::IspaLibFunctionCall& value);
        std::string lambda(const LangAPI::Lambda& value);
        std::string element(LangAPI::ExpressionElement value);
        std::string templates(const decltype(LangAPI::Type::template_parameters)& values);
        std::string templates(const stdu::vector<std::variant<std::shared_ptr<LangAPI::Type>, std::shared_ptr<LangAPI::RValue>>>& values);
        std::string counter() const;
        std::string recovery_set(const LangAPI::Type& set);
        std::string recovery_policy(const LangAPI::Type& policy);
    public:
        explicit Core(std::string module_name);
        void set_method_members(std::unordered_set<std::string> members) { method_members_ = std::move(members); }
        void set_generic_functions(std::unordered_set<std::string> names) { generic_functions_ = std::move(names); }
        void set_statement_writer(StatementWriter callback);
        // Open/close the Python `def` that owns the statements being rendered.
        void enter_scope(const std::vector<std::string>& params);
        void declare(const std::string& name);          // a C++ local: binds the name in the current def
        void note_assigned(const std::string& name);    // `name = ...` seen in the current def
        // Pops the scope; returns assigned names bound only in an enclosing def (need `nonlocal`).
        std::vector<std::string> leave_scope();
        std::vector<LambdaDefinition> take_pending_lambdas();
        // Used when a nested statement list is rendered while outer lambdas are still queued.
        void restore_pending_lambdas(std::vector<LambdaDefinition> saved);
        bool has_pending_lambdas() const { return !pending_lambdas_.empty(); }
        std::string type(const LangAPI::Type& value);
        std::string value(const LangAPI::RValue& v) { return rvalue(v); }
        std::string expression(const LangAPI::Expression& value);
        std::string statement_expression(const LangAPI::Expression& value);
        std::string literal(const std::string& source) const;
        static std::string identifier(const std::string& s);
        static std::string symbol_path(const std::string& text);
    };
}
