export module Python.Imports;
import Modules;
import UAST.PyAST;
import LangAPI;
import std;

export namespace Python {
// Imported Python is compiled, never executed by the generator.
class Imports {
    struct Export {
        std::string qualified;
        std::optional<std::size_t> arity;
    };
    using Bindings = std::map<std::string, Export>;
    std::map<std::string, Bindings> files;
    std::map<std::pair<std::string, std::string>, std::string> aliases;
    std::map<std::string, std::filesystem::path> paths;
    std::map<std::string, Bindings> exports;
    LangAPI::Declarations declarations;
    UAST::py::Signatures signatures;
    std::set<std::string> class_names, loading;
    // Resolves, compiles (once) and returns the dotted name of a Python module.
    std::string module(const Modules::Path& source_path, const std::string& from_file,
                       const Modules::Project& project, const UAST::SourcePosition& at,
                       const std::filesystem::path& package_root);
    void load(const Modules::Source& source, const Modules::Import& imp,
                   const Modules::Project& project);
public:
    explicit Imports(const Modules::Project* project);
    void bind(UAST::py::Expr& expression, const std::string& file) const;
    void bind(UAST::py::Module& module, const std::string& file) const;
    auto takeDeclarations() -> LangAPI::Declarations {
        if (declarations.empty()) return {};
        LangAPI::Namespace ns;
        ns.name = "python";
        ns.declarations = std::move(declarations);
        return LangAPI::Declarations{ {LangAPI::Declaration{std::move(ns)}} };
    }
};
}
