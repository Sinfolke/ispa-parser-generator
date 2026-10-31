export module Modules;
import std;

export namespace Modules {
using Path = std::vector<std::string>;
struct Position {
    std::string file;
    std::size_t offset = 0, line = 1, column = 1;
};
struct Import {
    enum class Kind { Qualified, All, Selected };
    Kind kind = Kind::Qualified;
    Path source;
    std::string alias;
    std::string external;
    std::vector<Path> symbols;
    Position position;
    bool python_import = false;
};
struct ExternalSymbol {
    std::string cpp_type, cpp_binding, header;
};
struct ExternalModule {
    std::optional<Path> entry;
    std::map<Path, ExternalSymbol> symbols;
};
using ExternalPackage = std::map<Path, ExternalModule>;
ExternalPackage readExternal(const std::string& prefix);
struct Detach {
    Path name;
    bool feature = false;
    Position position;
};
struct Source {
    std::string file;
    Path name;
    std::optional<Path> entry;
    // Directives/comments are blanked, preserving byte offsets and newlines.
    std::string grammar;
    std::map<Path, Position> declarations;
    std::vector<Import> imports;
    std::vector<Detach> detach;
    std::map<Path, std::vector<Path>> features;
    // Local spelling -> canonical declaration. No transitive re-exports.
    std::map<Path, Path> allowed_symbols;
    // Includes explicitly detached bindings so Builder can propagate removal
    // through required references rather than treating them as misspellings.
    std::map<Path, Path> resolved_symbols;
    std::map<Path, std::vector<Path>> allowed_features;
    std::set<Path> detached_symbols;
    bool has_directives = false;
};
// A Python helper module carried by a precompiled library.
struct PythonSource {
    std::filesystem::path file;
    std::string text;
};
struct Project {
    // Keyed by path relative to the library's python root, e.g. "stdmath.py".
    std::map<std::string, PythonSource> python_sources;
    std::vector<Source> sources;
    bool modular = false;
    std::optional<Path> entry;
    std::optional<Path> root;
    std::map<Path, ExternalSymbol> external_symbols;
    std::vector<std::string> search_directories;
    std::vector<std::string> library_search_directories;
    std::vector<std::string> input_files;
    std::set<std::string> library_files;
    const Source& source(const std::string& file) const;
};
Project discover(const std::vector<std::string>& directories,
                 const std::vector<std::string>& files = {},
                 const std::vector<std::string>& library_directories = {});
// Recursively discover .isc files in the supplied directories, plus explicit
// inputs. Discovery is deterministic and overlapping paths are deduplicated.
// With no module directives, CLI inputs retain their historical shared scope.
Project preprocess(const std::vector<std::string>& directories,
                   const std::vector<std::string>& files = {},
                   const std::map<std::string, std::string>& resolve = {},
                   const std::vector<std::string>& library_directories = {});
std::string spelling(const Path& path);
// Bundle the .isc modules (`directory/grammar`) and .py helpers (`directory/python`) of a
// library into `directory/library.ispac`; a flat directory holds both. discover() loads a
// .ispac entry of the library list instead of re-scanning and re-parsing module directives, as long as the
// listed sources are unchanged (or no longer present, as in a shipped library). Returns the
// number of modules written.
std::size_t precompile(const std::string& directory);
}
