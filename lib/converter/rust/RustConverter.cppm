export module Rust.Init;

import Converter.Writer;
import Converter.Construct;
import LangAPI;
import std;
import Rust.CoreFunctions;
import Rust.Rewriter;

namespace Rust {
    enum class FnKind { Free, Method, Constructor };

    struct Context {
        std::vector<std::string> module_path;                           // enclosing `mod`s (namespaces)
        std::unordered_map<std::string, std::size_t> fn_counts;         // overload suffixes per scope
        std::unordered_map<std::string, std::size_t> file_counts;       // unique file stems (incremental mode)
        std::size_t marker_counter = 0;                                 // unique names for the allowed-type marker traits
        std::vector<std::pair<std::string, std::string>> files;         // incremental mode: [path, text]
    };

    class Construct : protected ::Converter::ConstructBase {
        Context context;
        bool debug_instrumentation_ = true;
        std::vector<std::pair<std::string, std::string>> writable_files;
        LangAPI::Declarations program;                                  // the rewritten IR (Rust.Rewriter)

        // declarations
        auto emitDeclarations(const LangAPI::Declarations &decls, ::Converter::Writer &w) -> void;
        auto emitDeclaration(const LangAPI::Declaration &decl, ::Converter::Writer &w) -> void;
        auto emitItem(const LangAPI::Declaration &decl, ::Converter::Writer &w) -> void;
        auto emitNamespace(const LangAPI::Namespace &ns, ::Converter::Writer &w) -> void;
        auto emitClass(const LangAPI::Class &cls, ::Converter::Writer &w) -> void;
        auto emitFunction(const LangAPI::Function &func, ::Converter::Writer &w, ::Converter::Writer &aux, FnKind kind,
                          const std::string &scope, const std::vector<std::string> &bases) -> void;
        auto emitFunctionSurveyed(const LangAPI::Function &func, ::Converter::Writer &w, ::Converter::Writer &aux, FnKind kind,
                                  const std::string &scope, const std::vector<std::string> &bases) -> void;
        auto emitFunctionChecked(const LangAPI::Function &func, ::Converter::Writer &w, ::Converter::Writer &aux, FnKind kind,
                                 const std::string &scope, const std::vector<std::string> &bases) -> void;
        auto emitOutputFunction(const LangAPI::Function &func, ::Converter::Writer &w) -> void;
        auto emitEnum(const LangAPI::Enum &enum_, ::Converter::Writer &w) -> void;
        auto emitTypeAlias(const LangAPI::TypeAlias &alias, ::Converter::Writer &w) -> void;
        auto emitVariable(const LangAPI::Variable &v, ::Converter::Writer &w) -> void;
        auto emitVariants(::Converter::Writer &w) -> void;
        auto initializer(const LangAPI::Type &type, const LangAPI::Expression &value) -> std::string;

        // statements
        auto produceIf(const LangAPI::If &if_stmt, ::Converter::Writer &o) -> void;
        auto produceWhile(const LangAPI::While &while_stmt, ::Converter::Writer &o) -> void;
        auto produceDoWhile(const LangAPI::DoWhile &do_while_stmt, ::Converter::Writer &o) -> void;
        auto produceSwitch(const LangAPI::Switch &switch_stmt, ::Converter::Writer &o) -> void;
        auto produceVariableStatement(const LangAPI::Variable &variable, ::Converter::Writer &o) -> void;
        auto produceThrow(const LangAPI::Throw &throw_stmt, ::Converter::Writer &o) -> void;
        auto produceComment(const LangAPI::Comment &comment, ::Converter::Writer &o) -> void;
        auto convertStatement(const LangAPI::Statement &statement, ::Converter::Writer &o) -> void;
        auto convertStatements(const LangAPI::Statements &statements, ::Converter::Writer &o) -> void;
    public:
        Construct(const LangAPI::Declarations *source, const char *namespace_name, bool incremental,
                  bool debug_instrumentation = true)
            : ConstructBase(source, namespace_name, incremental), debug_instrumentation_(debug_instrumentation) {
            RustCore::lambda_body = [this](const LangAPI::Statements &body) {
                ::Converter::Writer writer;
                convertStatements(body, writer);
                return std::string(writer.get());
            };
        }
        auto get_files() -> const std::vector<std::pair<std::string, std::string>>& override {
            return writable_files;
        }
        auto construct() -> void override;
    };
}
extern "C" auto rust_construct(const LangAPI::Declarations *tree, const char *namespace_name,
                               bool incremental, bool debug_instrumentation, bool text_view)
    -> Rust::Construct*;
extern "C" auto rust_construct_finish(Rust::Construct *c) -> void;
