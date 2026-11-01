export module C.Init;

import Converter.Writer;
import Converter.Construct;
import LangAPI;
import LangAPI.Typing;
import std;

// C backend. Lowered classes become structs whose first member is the base struct (so a derived pointer is a valid
// base pointer), virtual methods become function-pointer slots in the introducing class, and every class X gets
//     <ns>_get_X_handler(ispa_arena*, ...)   allocate + initialise (constructor)
//     <ns>_free_X_handler(X*)                 release the handle (arena-owned memory is reclaimed with the arena)
// Namespaces are flattened into name prefixes. Generics are monomorphized (see the design notes in
// parser/stdlibs/target/c/README.md).
namespace C {
    struct Instance { std::string c_name; };

    class Construct : protected ::Converter::ConstructBase {
        std::vector<std::pair<std::string, std::string>> writable_files;
        std::vector<std::string> scope;                          // enclosing namespaces
        std::vector<std::string> diagnostics;                    // survey: constructs that could not be emitted
        std::size_t emitted_functions = 0, skipped_functions = 0;
        std::unordered_set<std::string> defined;                 // C names of every class/enum/alias, filled before emission
        std::unique_ptr<LangAPI::Typing::Typer> typer;           // shared static typing (LangAPI.Typing)
        const LangAPI::Class *cur_class_decl = nullptr;
        std::unordered_map<const LangAPI::Class *, std::string> class_cname;
        mutable std::vector<std::pair<std::string, std::string>> vec_defs;   // (vector type name, element C type) in dependency order
        mutable std::unordered_set<std::string> vec_seen;
        LangAPI::Typing::MaybeType array_hint;                   // expected type of the sole operand being converted
        std::string lambda_defs;                                 // hoisted lambda definitions, flushed before the enclosing function
        std::size_t lambda_counter = 0;
        auto convertLambda(const LangAPI::Lambda &lam, const LangAPI::Typing::MaybeType &expected, bool void_ret) -> std::string;
        auto invokeCallable(const std::string &code, const LangAPI::Type &callable, const std::vector<std::string> &args) -> std::string;
        auto arrayLiteral(const LangAPI::Array &arr, const LangAPI::Type &type) -> std::string;
        auto convertExprAs(const LangAPI::Expression &e, const LangAPI::Type &expected) -> std::string;
        auto coerce(const std::string &code, const LangAPI::Typing::MaybeType &from, const LangAPI::Type &to) const -> std::string;
        auto vecName(const std::string &elem) const -> std::string;
        auto convertStorage(const LangAPI::StorageSymbol &ss) -> std::string;
        auto methodCall(const LangAPI::Class &recv_cls, const std::string &name, const std::string &recv,
                        std::vector<std::string> args) -> std::string;
        ::Converter::Writer fwd_w;                               // forward typedefs + enums (before vector definitions)
        std::vector<std::string> init_globals;                   // non-literal global initialisers, run by <stem>_init_globals()
        std::unordered_set<std::string> generic_classes;         // classes with type parameters (instantiated on demand)

        struct ClassInfo {
            std::string c_name;
            std::vector<std::pair<std::string, std::string>> bases;          // (C name of base, member name)
            std::unordered_map<std::string, const LangAPI::Function *> methods;
            std::unordered_set<std::string> fields;
        };
        std::unordered_map<std::string, ClassInfo> classes;                  // by C name
        std::unordered_set<std::string> functions;                           // C names of free functions
        // state of the body being emitted
        std::string cur_class;                                               // C name of the class owning `self`, or empty
        std::vector<std::unordered_set<std::string>> locals;
        const LangAPI::Type *cur_return = nullptr;
        std::map<std::string, std::size_t> reasons;                          // survey: why a body stayed a placeholder

        auto convertStatements(const LangAPI::Statements &st, ::Converter::Writer &o) -> void;
        auto convertStatement(const LangAPI::Statement &st, ::Converter::Writer &o) -> void;
        LangAPI::Typing::MaybeType last_type;                    // type of the operand parsed last (see parseBinary)
        auto convertValue(const LangAPI::ExpressionValue &v) -> std::string;
        auto parsePrimary(const LangAPI::Expression &e, std::size_t &i) -> std::string;
        auto parseBinary(const LangAPI::Expression &e, std::size_t &i, int min_prec) -> std::string;
        auto convertExpr(const LangAPI::Expression &e) -> std::string;
        auto convertRValue(const LangAPI::RValue &r) -> std::string;
        auto convertSymbolValue(const LangAPI::Symbol &s) -> std::string;
        auto convertCall(const LangAPI::FunctionCall &c) -> std::string;
        auto memberPath(const std::string &cls, const std::string &name) const -> std::optional<std::string>;
        auto isLocal(const std::string &n) const -> bool;
        auto collect(const LangAPI::Declarations &decls) -> void;
        auto resolve(const LangAPI::Symbol &symbol, std::vector<std::string> &diag) const -> std::string;
        auto typeName(const LangAPI::Type &type, std::vector<std::string> &diag) const -> std::string;

        auto emitDeclarations(const LangAPI::Declarations &decls, ::Converter::Writer &types, ::Converter::Writer &protos,
                              ::Converter::Writer &impls) -> void;
        auto emitGlobal(const LangAPI::Variable &v, ::Converter::Writer &types, ::Converter::Writer &protos,
                        ::Converter::Writer &impls, const std::string &owner_c_name = {}) -> void;
        auto emitClass(const LangAPI::Class &cls, ::Converter::Writer &types, ::Converter::Writer &protos,
                       ::Converter::Writer &impls) -> void;
        auto emitFunction(const LangAPI::Function &fn, const std::string &owner, ::Converter::Writer &protos,
                          ::Converter::Writer &impls) -> void;
        auto prefix() const -> std::string;
    public:
        Construct(const LangAPI::Declarations *source, const char *namespace_name, bool incremental)
            : ConstructBase(source, namespace_name, incremental) {}
        auto get_files() -> const std::vector<std::pair<std::string, std::string>>& override { return writable_files; }
        auto construct() -> void override;
    };
}
extern "C" auto c_construct(const LangAPI::Declarations *tree, const char *namespace_name, bool incremental,
                            bool debug_instrumentation, bool text_view) -> C::Construct*;
extern "C" auto c_construct_finish(C::Construct *c) -> void;
