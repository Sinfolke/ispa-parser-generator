export module Parser.Base;
import AST.API;
import UAST.PyAST;
import AST.Tree;
import LangAPI;
import LangAPI.Context;
import hash;
import std;
export namespace Parser {
    struct Settings {
        bool debug = false;
        bool cst = false;
    };
    struct DebugMeta {
        AST::RuleMember member;
    };
    struct LowerMeta {
        LangAPI::Variable value;
        LangAPI::Variable value_list;
        LangAPI::Variable *ast_value = nullptr;
    };
    // Configuration of an automatically generated recovery handler.
    // AST::Tree produces qualified token paths; the C++ converter emits
    // corresponding Tokens::<path> non-type template arguments.
    struct RecoveryOptions {
        std::vector<AST::LLkToken> sync;
        bool allow_insertion = true;
        bool allow_deletion = true;
        bool allow_replacement = true;
        std::size_t max_skip = 64;
    };
    struct LoweredMember {
        LowerMeta meta;
        DebugMeta debug;
        LangAPI::Statements statements;
        RecoveryOptions options;
    };
    struct Ctx {
        const LangAPI::Context *parser_decls;
        Settings settings;
        std::vector<LoweredMember> lowered_members;
        utype::unordered_map<std::vector<std::string>, std::size_t> variables_lowered;
    };

    class ParserBase {
    protected:
        Ctx ctx;
        LangAPI::Declarations decls;
        LangAPI::Context decls_ctx;

        auto account_mem_built(const std::vector<std::string> &name) -> void;

        // Presence detection only. Custom failure actions are lowered
        // separately by the algorithm/frontend, before automatic fallback.
        static auto hasCustomFailure(const AST::RuleMember& member) -> bool;

        // Emit an expression building ISPA_STD::Recovery::handler<...>(...).
        // It is a callable object: handler(pos, recovery_state).
        auto recoveryHandler(
            AST::Tree& tree,
            const AST::LLkToken& rule,
            std::size_t member_index,
            const RecoveryOptions& options = {}
        ) -> LangAPI::Expression;

        // Build the rule's recovery frame and each member's recovery site.
        auto recoveryFrame(const AST::LLkToken& rule) -> LangAPI::Expression;
        auto recoverySite(
            AST::Tree& tree,
            const AST::LLkToken& rule,
            std::size_t member_index
        ) -> LangAPI::Expression;
        auto recoveryChoice(AST::Tree& tree, const AST::LLkToken& rule,
                            const AST::RuleMember& choice) -> LangAPI::Expression;

        auto var_name(const std::vector<std::string> &name, bool is_list) -> std::string;
    public:
        ParserBase(const Ctx &ctx) : ctx(ctx), decls_ctx(decls) {
            this->ctx.parser_decls = &decls_ctx;
        }
        ParserBase(const ParserBase&) = delete;
        ParserBase& operator=(const ParserBase&) = delete;
        ParserBase(ParserBase&&) = delete;
        ParserBase& operator=(ParserBase&&) = delete;


        // Lower the typed embedded Python AST into the language-neutral IR.
        // Unsupported constructs throw with their source position. `variables`
        // names existing bindings (such as captures); assignments update them.
        // New local bindings must precede branches/loops so they remain visible.
        static auto convertPython(const UAST::py::Expr& expression) -> LangAPI::Expression;
        static auto convertPython(const UAST::py::Stmt& statement, const std::set<std::string>& variables = {}) -> LangAPI::Statements;
        static auto convertPython(const UAST::py::Module& module, const std::set<std::string>& variables = {}) -> LangAPI::Statements;
        static auto convertPythonDeclaration(const UAST::py::Stmt& declaration, const std::set<std::string>& classes = {}, const std::set<std::string>& typevars = {},
            const UAST::py::Signatures* signatures = nullptr, const std::string& module_prefix = {}) -> LangAPI::Declaration;
        static auto convertPythonDeclarations(const UAST::py::Module& module) -> LangAPI::Declarations;
        static auto convertPythonType(const UAST::py::ExprPtr& annotation) -> LangAPI::Type;
        // Action scopes reserve ctx; locals persist across sequential blocks.
        static auto convertPythonAction(const AST::PythonAction& action,
            std::set<std::string>& variables, bool failure = false,
            const std::set<std::string>& classes = {}, std::set<std::string>* objects = nullptr,
            const std::string& token_rule = {},
            const std::function<LangAPI::Statements(const UAST::py::IspaMatch&, std::set<std::string>&)>& match = {}) -> LangAPI::Statements;

        static auto dataType(const AST::DataType& type) -> LangAPI::Type;

        static auto fun_name(const std::vector<std::string> &name) -> std::string;
        // deduce type functions
        static auto deduceUvarType(const LangAPI::Variable &var, const LangAPI::Variable &shadow_var) -> LangAPI::Type;
        static auto deduceVarTypeByRuleMemberBase(const AST::RuleMember &mem) -> LangAPI::Type;
        static auto deduceVarTypeByRuleMember(const AST::RuleMember &mem) -> LangAPI::Type;
    };
}
