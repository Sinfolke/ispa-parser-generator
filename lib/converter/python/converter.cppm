export module Python.Init;

import Converter.Construct;
import LangAPI;
import std;
import Python.CoreFunctions;
import Python.Rewriter;

export namespace Python {
    class Construct : protected ::Converter::ConstructBase {
        Core core_;
        Rewriter rewriter_;
        LangAPI::Declarations rewritten_source_;
        std::vector<std::pair<std::string, std::string>> files_;
        std::string output_;
        std::string current_scope_;
        bool debug_instrumentation_ = true;
        unsigned indentation_ = 0;
        bool class_context_ = false;
        bool namespace_context_ = false;
        bool inside_function_ = false;
        std::size_t hidden_class_counter_ = 0;
        std::vector<std::pair<std::string, std::string>> deferred_aliases_;
        std::size_t switch_counter_ = 0;
        // Per scope: function names declared more than once (only those need ispa_std.overload).
        std::vector<std::unordered_set<std::string>> overloaded_scopes_;
        std::unordered_set<std::string> class_members_;

        void line(const std::string& text = "");
        void emit_pending_lambdas();
        void declarations(const LangAPI::Declarations& decls);
        void declaration(const LangAPI::Declaration& decl);
        void ns(const LangAPI::Namespace& ns);
        void function(const LangAPI::Function& fn, bool constructor = false);
        void cls(const LangAPI::Class& cls);
        void variable(const LangAPI::Variable& var);
        void alias(const LangAPI::TypeAlias& alias);
        void enumeration(const LangAPI::Enum& en);
        void statements(const LangAPI::Statements& statements);
        void statement(const LangAPI::Statement& statement);
        void if_stmt(const LangAPI::If& value);
        void if_chain(const LangAPI::If& value, bool elif);
        void switch_body(const LangAPI::Statements& stmts);
        void while_stmt(const LangAPI::While& value);
        void do_while_stmt(const LangAPI::DoWhile& value);
        void switch_stmt(const LangAPI::Switch& value);
        void body_or_pass(const LangAPI::Statements& stmts);
        std::string statements_inline(const LangAPI::Statements& stmts, const std::vector<std::string>& params);
        void comment(const std::string& text);
    public:
        Construct(const LangAPI::Declarations* src, const char* module_name, bool incremental,
                  bool debug_instrumentation = true);
        auto get_files() -> const std::vector<std::pair<std::string, std::string>>& override {
            return files_;
        }
        void construct() override;
    };
}

extern "C" auto python_construct(const LangAPI::Declarations*, const char*, bool, bool, bool text_view) -> Python::Construct*;
extern "C" auto python_construct_finish(Python::Construct*) -> void;
