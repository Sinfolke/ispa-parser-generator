export module Cpp.Init;
import Converter.Writer;
import Converter.Construct;
import hash;
import LangAPI;
import std;
import Cpp.CoreFunctions;

namespace Cpp {
    struct OutputSrc {
        Converter::Writer *h;
        Converter::Writer *cpp;
        OutputSrc() {
            h = new Converter::Writer();
            cpp = new Converter::Writer();
            self_containing = true;
        }
        OutputSrc(Converter::Writer *h, Converter::Writer *cpp) : h(h), cpp(cpp) {}

        auto operator=(const OutputSrc &other) -> OutputSrc& {
            delete_if();
            h = new Converter::Writer(*other.h);
            cpp = new Converter::Writer(*other.cpp);
            return *this;
        }
        bool src_empty = true;
        bool self_containing = false;
        bool in_class = false;  // true while emitting the members of a class body
        bool in_templated_class = false;
        void delete_if() {
            if (self_containing) {
                delete h;
                delete cpp;
            }
        }
    };
    // where a named thing is declared (used to work out which headers a file must include)
    struct SymbolInfo {
        std::string header;                 // output path relative to the output root
        std::string keyword;                // "struct"/"class" if it can be forward declared, else empty
        std::vector<std::string> ns;        // enclosing namespaces
        std::string name;
        std::size_t order = 0;              // position in the IR; later declarations get forward-declared, not included
        bool is_value = false;              // function / variable: only bodies depend on it
        bool adl_type = false;              // namespace-level class / enum: free functions taking it are found by ADL
    };
    struct Context {
        LangAPI::Visibility visibility;

        Converter::Writer main_h;
        Converter::Writer main_cpp;
        Converter::Writer cur_incremental_h;
        Converter::Writer cur_incremental_cpp;
        std::vector<std::string> symbol_path;
        utype::unordered_map<LangAPI::Symbol, OutputSrc> converted_symbols;
        // one .h/.cpp pair per function: [path, text], include lines for the main header, overload counters
        std::vector<std::pair<std::string, std::string>> function_files;
        std::vector<std::string> trailing_includes;
        std::unordered_map<std::string, std::size_t> function_name_counts;
        std::vector<std::string> namespace_path;                               // namespaces only (symbol_path also holds classes)
        std::unordered_map<std::string, std::vector<SymbolInfo>> known;        // simple name -> declarations
        std::unordered_map<std::string, std::size_t> header_order;             // header path -> position in the IR
        std::vector<std::string> function_bases;                               // file base per function, in traversal order
        std::size_t next_function = 0;
        std::unordered_map<std::string, std::vector<std::string>> adl;         // type header -> headers of its ADL functions
        std::unordered_set<std::string> known_namespaces;                      // "A::B" for every namespace in the IR
        Converter::Writer main_tail;                                           // out-of-class template bodies, appended to the main header
    };

    class Construct : protected ::Converter::ConstructBase {
        Context context;
        bool debug_instrumentation_ = true;
        std::vector<std::pair<std::string, std::string>> writable_files;
        // declarations related
        auto determineOffloadCompatible(const LangAPI::Function &func) -> bool;
        auto produceNamespace(const LangAPI::Namespace &ns, OutputSrc &o) -> void;
        auto produceFunction(const LangAPI::Function &cls, OutputSrc &o, bool constructor = false) -> void;
        std::set<std::string> constructor_members;
        auto produceClass(const LangAPI::Class &cls, OutputSrc &o) -> void;
        auto produceTypeAlias(const LangAPI::TypeAlias &alias, OutputSrc &o) -> void;
        auto produceEnum(const LangAPI::Enum &enum_, OutputSrc &o) -> void;
        auto produceVariable(const LangAPI::Variable &v, OutputSrc &o) -> void;
        auto setVisibility(LangAPI::Visibility visibility) -> std::string;
        auto getVisibility(LangAPI::Visibility visibility) -> std::string;
        auto setDefaultClsVisibility(LangAPI::Visibility visibility) -> std::string;
        auto getDefaultVisibility(LangAPI::Visibility visibility) -> std::string;
        auto convertDeclaration(const LangAPI::Declaration &decl, OutputSrc &o) -> void;
        auto convertDeclarations(const LangAPI::Declarations &decls, OutputSrc &o) -> void;

        // statements related

        auto produceIf(const LangAPI::If &if_stmt, Converter::Writer &o) -> void;
        auto produceWhile(const LangAPI::While &while_stmt, Converter::Writer &o) -> void;
        auto produceDoWhile(const LangAPI::DoWhile &do_while_stmt, Converter::Writer &o) -> void;
        auto produceSwitch(const LangAPI::Switch &switch_stmt, Converter::Writer &o) -> void;
        auto produceVariableStatement(const LangAPI::Variable &variable, Converter::Writer &o) -> void;
        auto produceThrow(const LangAPI::Throw &throw_stmt, Converter::Writer &o) -> void;
        auto produceComment(const LangAPI::Comment &comment, Converter::Writer &o) -> void;
        auto convertStatement(const LangAPI::Statement &statements, Converter::Writer &o) -> void;
        auto convertStatements(const LangAPI::Statements &statements, Converter::Writer &o) -> void;
    public:
        Construct(const LangAPI::Declarations *source, const char *namespace_name, bool incremental,
                  bool debug_instrumentation = true)
            : ConstructBase(source, namespace_name, incremental), debug_instrumentation_(debug_instrumentation) {
            Core::symbol_path = &context.symbol_path;
            Core::lambda_body = [this](const LangAPI::Statements& body) {
                Converter::Writer writer;
                convertStatements(body, writer);
                return writer.get();
            };
        }
        auto get_files() -> const std::vector<std::pair<std::string, std::string>>& override {;
            return writable_files;
        }
        auto construct() -> void override;
    };
}
extern "C" auto cpp_construct(const LangAPI::Declarations *tree, const char *namespace_name,
                              bool incremental, bool debug_instrumentation)
    -> Cpp::Construct*;
