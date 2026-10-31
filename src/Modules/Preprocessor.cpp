module;
#include <ispa_dep_scanner/parser.h>
module Modules;
import std;

namespace Modules {
std::string spelling(const Path& path) {
    std::string out;
    for (const auto& part : path) {
        if (!out.empty()) out += ".";
        out += part;
    }
    return out;
}
const Source& Project::source(const std::string& file) const {
    for (const auto& s : sources) if (s.file == file) return s;
    throw std::invalid_argument("Source is not in module project: " + file);
}
namespace {
[[noreturn]] void fail(const Position& p, const std::string& message) {
    throw std::runtime_error(p.file + ":" + std::to_string(p.line) + ":" +
        std::to_string(p.column) + ": " + message);
}
bool identifierStart(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
bool identifierPart(char c) { return identifierStart(c) || (c >= '0' && c <= '9'); }
bool startsWith(const Path& path, const Path& prefix) {
    return path.size() >= prefix.size() && std::equal(prefix.begin(), prefix.end(), path.begin());
}
Path joined(Path a, const Path& b) { a.insert(a.end(), b.begin(), b.end()); return a; }
using Syntax = IspaDependency::Syntax;
using Common = DepParser::Types::modules::common;

template<class T> const T& data(const ISPA_STD::Node<T>& node) { return node.data(); }
template<class T> const T& data(const ISPA_STD::Node<std::shared_ptr<T>>& node) { return *node.data(); }

// A consumer of generated syntax, with no tokenization or grammar decisions.
class SourceSyntax {
    Source& out;
    std::vector<std::size_t> line_starts{0};
    std::vector<IspaDependency::Span> erased;
    bool explicit_module = false;

    Position position(std::size_t offset) const {
        offset = std::min(offset, out.grammar.size());
        const auto line = std::upper_bound(line_starts.begin(), line_starts.end(), offset) - line_starts.begin();
        return {out.file, offset, static_cast<std::size_t>(line), offset - line_starts[line - 1] + 1};
    }
    template<class T> Position position(const ISPA_STD::Node<T>& token) const {
        return position(token.startpos());
    }
    template<class Begin, class End> void erase(const Begin& begin, const End& end) {
        const auto finish = end.startpos() + end.length();
        if (begin.startpos() > finish || finish > out.grammar.size())
            throw std::runtime_error("invalid generated directive span");
        erased.push_back({begin.startpos(), finish - begin.startpos()});
        out.has_directives = true;
    }
    Path path(const Syntax::module_path& value) const {
        Path result{data(value.primary).value};
        for (const auto& part : value.nested) result.push_back(data(part).value);
        return result;
    }
    Path path(const Common::name<Common::rule_params>& value) const {
        Path result{data(value.primary).value};
        for (const auto& part : value.nested) result.push_back(data(part).value);
        return result;
    }
    Path path(const Syntax::source_name& value) const {
        return path(data(value.value));
    }
    Position namePosition(const Common::name<Common::rule_params>& value) const { return position(value.primary); }
    Position namePosition(const Syntax::source_name& value) const {
        return namePosition(data(value.value));
    }
    void visit(const Syntax::mod_module& value) {
        const auto at = position(value.begin);
        if (explicit_module) fail(at, "duplicate module declaration");
        explicit_module = true;
        if (!value.parameters.empty()) fail(at, "generic module parameters are not supported by preprocessing yet");
        out.name = path(data(value.name));
        if (!value.entry.empty()) out.entry = path(data(value.entry));
        erase(value.begin, value.end);
    }
    void visit(const Syntax::mod_import& value) {
        Import imp;
        imp.position = position(value.begin);
        if (!value.external.empty()) imp.external = data(data(value.external).value).value;
        imp.python_import = !value.py_import.empty();
        std::visit([&](const auto& node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (!std::is_same_v<T, std::monostate>) {
                const auto& import_d = data(node);
                using D = std::decay_t<decltype(import_d)>;
                imp.source = path(data(import_d.source));
                if constexpr (std::is_same_v<D, Syntax::mod_import::import_value::module_import>) {
                    if (!import_d.alias.empty()) imp.alias = data(import_d.alias).value;
                } else {
                    std::visit([&](const auto& selection) {
                        using S = std::decay_t<decltype(selection)>;
                        if constexpr (!std::is_same_v<S, std::monostate>) {
                            using D = std::decay_t<decltype(data(selection))>;
                            if constexpr (std::is_same_v<D, Syntax::mod_import::import_value::from_import::selection::items>) {
                                imp.kind = Import::Kind::Selected;
                                imp.symbols.push_back(path(data(data(selection).first)));
                                for (const auto& item : data(selection).sequence) imp.symbols.push_back(path(data(item)));
                            } else imp.kind = Import::Kind::All;
                        }
                    }, data(import_d.sel).value);
                }
            }
        }, data(value.value).value);
        out.imports.push_back(std::move(imp));
        erase(value.begin, value.end);
    }
    void visit(const Syntax::mod_detach& value) {
        Detach detached;
        detached.position = position(value.begin);
        std::visit([&](const auto& node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (!std::is_same_v<T, std::monostate>) {
                const auto& name = data(node);
                using D = std::decay_t<decltype(name)>;
                if constexpr (std::is_same_v<D, Syntax::source_name>) detached.name = path(name);
                else { detached.feature = true; detached.name = path(data(name.value)); }
            }
        }, data(value.value).value);
        out.detach.push_back(std::move(detached));
        erase(value.begin, value.end);
    }
    void visit(const Syntax::feature_decl& value) {
        std::vector<Path> symbols;
        if (!value.symbols.empty()) {
            const auto& list = data(value.symbols);
            symbols.push_back(path(data(list.first)));
            for (const auto& item : list.following) symbols.push_back(path(data(item)));
        }
        const auto name = path(data(value.name));
        if (!out.features.emplace(name, std::move(symbols)).second)
            fail(position(value.begin), "duplicate feature " + spelling(name));
        erase(value.begin, value.end);
    }
    void declaration(const Syntax::declaration& value, const Path& parent = {}) {
        const auto& form = data(value.value);
        const auto& name = data(form.name);
        const auto full = joined(parent, path(name));
        const auto at = namePosition(name);
        if (!out.declarations.emplace(full, at).second) fail(at, "duplicate rule " + spelling(full));
        std::visit([&](const auto& node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (!std::is_same_v<T, std::monostate>) {
                using D = std::decay_t<decltype(data(node))>;
                if constexpr (std::is_same_v<D, Syntax::body::regular>)
                    for (const auto& item : data(node).value) member(data(item), full);
            }
        }, data(form.body).value);
    }
    void member(const Syntax::member& value, const Path& parent) {
        std::visit([&](const auto& node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (!std::is_same_v<T, std::monostate>) {
                using D = std::decay_t<decltype(data(node))>;
                if constexpr (std::is_same_v<D, Syntax::nested_rule>) declaration(data(data(node).value), parent);
            }
        }, value.value);
    }
public:
    explicit SourceSyntax(Source& source) : out(source) {
        for (std::size_t j = 0; j < out.grammar.size(); ++j)
            if (out.grammar[j] == '\n') line_starts.push_back(j + 1);
    }
    void run() {
        IspaDepParser parser;
        try {
            const auto& syntax = parser.parse(out.grammar.c_str()).value.data();
            for (const auto& entry : syntax.value) std::visit([&](const auto& node) {
                using T = std::decay_t<decltype(node)>;
                if constexpr (!std::is_same_v<T, std::monostate>) {
                    using D = std::decay_t<decltype(data(node))>;
                    if constexpr (std::is_same_v<D, Syntax::declaration>) declaration(data(node));
                    else if constexpr (requires { data(node).begin; }) visit(data(node));
                }
            }, entry);
        } catch (const std::exception& error) {
            // Semantic diagnostics already carry their source label.
            if (std::string_view(error.what()).starts_with(out.file + ":")) throw;
            fail(position(parser.offset), error.what());
        }
        erased.insert(erased.end(), parser.comments.begin(), parser.comments.end());
        for (const auto& span : erased)
            for (auto i = span.offset; i < span.offset + span.length; ++i)
                if (out.grammar[i] != '\n' && out.grammar[i] != '\r') out.grammar[i] = ' ';
    }
};

// ---- precompiled library bundle (library.ispac) ----
constexpr std::string_view bundle_name = "library.ispac", bundle_magic = "ISPAC001";
struct BundleWriter {
    std::string out;
    void num(std::uint64_t v) { out.append(reinterpret_cast<const char*>(&v), sizeof v); }
    void str(const std::string& v) { num(v.size()); out += v; }
    void path(const Path& v) { num(v.size()); for (const auto& x : v) str(x); }
    void pos(const Position& v) { str(v.file); num(v.offset); num(v.line); num(v.column); }
    void paths(const std::vector<Path>& v) { num(v.size()); for (const auto& x : v) path(x); }
    void import(const Import& v) {
        num(static_cast<std::uint64_t>(v.kind)); path(v.source); str(v.alias); str(v.external);
        paths(v.symbols); pos(v.position); num(v.python_import);
    }
    // File identity: root-relative path, size and modification time.
    void stamp(const std::filesystem::path& file, const std::filesystem::path& root) {
        std::error_code error;
        str(file.lexically_relative(root).generic_string());
        num(std::filesystem::file_size(file, error));
        num(std::filesystem::last_write_time(file, error).time_since_epoch().count());
    }
    template<class M, class F> void map(const M& m, F f) { num(m.size()); for (const auto& [k, v] : m) { path(k); f(v); } }
};
struct BundleReader {
    const std::string& in;
    std::size_t at = 0;
    std::uint64_t num() {
        if (at + 8 > in.size()) throw std::runtime_error("truncated bundle");
        std::uint64_t v; std::memcpy(&v, in.data() + at, 8); at += 8; return v;
    }
    std::string str() {
        const auto n = num();
        if (at + n > in.size()) throw std::runtime_error("truncated bundle");
        std::string v = in.substr(at, n); at += n; return v;
    }
    Path path() { Path v; for (auto n = num(); n--;) v.push_back(str()); return v; }
    Position pos() { Position v; v.file = str(); v.offset = num(); v.line = num(); v.column = num(); return v; }
    std::vector<Path> paths() { std::vector<Path> v; for (auto n = num(); n--;) v.push_back(path()); return v; }
    Import import() {
        Import v; v.kind = static_cast<Import::Kind>(num()); v.source = path(); v.alias = str();
        v.external = str(); v.symbols = paths(); v.position = pos(); v.python_import = num(); return v;
    }
    // False when the file exists on disk but differs from what was bundled.
    bool stamp(std::filesystem::path& file, const std::filesystem::path& root) {
        std::error_code error;
        file = (root / str()).lexically_normal();
        const auto size = num();
        const auto time = static_cast<std::int64_t>(num());
        if (!std::filesystem::is_regular_file(file, error)) return true;
        return std::filesystem::file_size(file, error) == size &&
               std::filesystem::last_write_time(file, error).time_since_epoch().count() == time;
    }
    template<class M, class F> void map(M& m, F f) { for (auto n = num(); n--;) { auto k = path(); m.emplace(std::move(k), f()); } }
};
// Source files are stored relative to the library root so the bundle survives being moved.
void writeSource(BundleWriter& w, const Source& s, const std::filesystem::path& root) {
    namespace fs = std::filesystem;
    std::error_code error;
    w.stamp(s.file, root);
    w.path(s.name); w.num(s.entry.has_value()); if (s.entry) w.path(*s.entry);
    w.str(s.grammar);
    w.map(s.declarations, [&](const Position& p) { w.pos(p); });
    w.num(s.imports.size()); for (const auto& i : s.imports) w.import(i);
    w.num(s.detach.size()); for (const auto& d : s.detach) { w.path(d.name); w.num(d.feature); w.pos(d.position); }
    w.map(s.features, [&](const std::vector<Path>& v) { w.paths(v); });
    w.map(s.allowed_symbols, [&](const Path& v) { w.path(v); });
    w.map(s.resolved_symbols, [&](const Path& v) { w.path(v); });
    w.map(s.allowed_features, [&](const std::vector<Path>& v) { w.paths(v); });
    w.num(s.detached_symbols.size()); for (const auto& p : s.detached_symbols) w.path(p);
    w.num(s.has_directives);
}
// Returns false when a listed source changed on disk.
bool readSource(BundleReader& r, Source& s, const std::filesystem::path& root) {
    namespace fs = std::filesystem;
    std::error_code error;
    fs::path file;
    const bool fresh = r.stamp(file, root);
    s.file = file.string();
    s.name = r.path(); if (r.num()) s.entry = r.path();
    s.grammar = r.str();
    r.map(s.declarations, [&] { return r.pos(); });
    for (auto n = r.num(); n--;) s.imports.push_back(r.import());
    for (auto n = r.num(); n--;) { Detach d; d.name = r.path(); d.feature = r.num(); d.position = r.pos(); s.detach.push_back(std::move(d)); }
    r.map(s.features, [&] { return r.paths(); });
    r.map(s.allowed_symbols, [&] { return r.path(); });
    r.map(s.resolved_symbols, [&] { return r.path(); });
    r.map(s.allowed_features, [&] { return r.paths(); });
    for (auto n = r.num(); n--;) s.detached_symbols.insert(r.path());
    s.has_directives = r.num();
    // Positions were recorded against the file's location at bundling time.
    for (auto& [name, position] : s.declarations) position.file = s.file;
    for (auto& item : s.imports) item.position.file = s.file;
    for (auto& item : s.detach) item.position.file = s.file;
    return fresh;
}
// The directories a library bundle describes: `grammar/` and `python/` next to it, or the
// bundle's own directory for a flat library.
std::filesystem::path grammarRoot(const std::filesystem::path& bundle) {
    const auto sub = bundle.parent_path() / "grammar";
    return std::filesystem::is_directory(sub) ? sub : bundle.parent_path();
}
std::filesystem::path pythonRoot(const std::filesystem::path& bundle) {
    const auto sub = bundle.parent_path() / "python";
    return std::filesystem::is_directory(sub) ? sub : bundle.parent_path();
}
struct Bundle {
    std::vector<Source> sources;
    std::map<std::string, PythonSource> python;
    std::filesystem::path grammar_root;
};
// Loads a .ispac file; nullopt when damaged or stale (a listed source changed, or a source
// was added since). Sources that are no longer on disk are trusted.
std::optional<Bundle> loadBundle(const std::filesystem::path& bundle_file) {
    namespace fs = std::filesystem;
    std::ifstream in(bundle_file, std::ios::binary);
    if (!in) return std::nullopt;
    const std::string data(std::istreambuf_iterator<char>(in), {});
    if (!data.starts_with(bundle_magic)) return std::nullopt;
    try {
        Bundle result;
        result.grammar_root = grammarRoot(bundle_file);
        const auto python_root = pythonRoot(bundle_file);
        BundleReader reader{data, bundle_magic.size()};
        std::set<fs::path> listed;
        bool fresh = true;
        for (auto n = reader.num(); n--;) {
            Source source;
            fresh &= readSource(reader, source, result.grammar_root);
            listed.insert(fs::path(source.file));
            result.sources.push_back(std::move(source));
        }
        for (auto n = reader.num(); n--;) {
            PythonSource module;
            fresh &= reader.stamp(module.file, python_root);
            module.text = reader.str();
            listed.insert(module.file);
            result.python.emplace(module.file.lexically_relative(python_root).generic_string(), std::move(module));
        }
        if (!fresh) return std::nullopt;
        std::error_code error;
        for (const auto& root : {result.grammar_root, python_root})
            for (const auto& item : fs::recursive_directory_iterator(root, error))
                if (item.is_regular_file() &&
                    (item.path().extension() == ".isc" || item.path().extension() == ".py") &&
                    !listed.contains(item.path().lexically_normal())) return std::nullopt;
        return result;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}
}

Project discover(const std::vector<std::string>& directories, const std::vector<std::string>& files,
                 const std::vector<std::string>& library_directories) {
    namespace fs = std::filesystem;
    Project project;
    std::map<fs::path, fs::path> discovered;
    std::vector<Source> bundled;
    std::set<fs::path> bundled_roots;
    // Precompiled libraries first: a fresh bundle replaces scanning its grammar directory.
    for (const auto& entry : library_directories) {
        if (fs::path(entry).extension() != ".ispac") continue;
        std::error_code error;
        const auto file = fs::weakly_canonical(entry, error);
        if (error) continue;
        if (auto bundle = loadBundle(file)) {
            bundled_roots.insert(fs::weakly_canonical(bundle->grammar_root, error));
            for (auto& source : bundle->sources) bundled.push_back(std::move(source));
            for (auto& [key, module] : bundle->python) project.python_sources.emplace(key, std::move(module));
        }
    }
    auto scan = [&](const std::string& directory, bool library) {
        if (fs::path(directory).extension() == ".ispac") return;
        auto root = fs::canonical(directory);
        if (!fs::is_directory(root)) throw std::runtime_error("Not a module directory: " + directory);
        if (library && bundled_roots.contains(root)) return;
        std::vector<fs::path> paths;
        for (const auto& item : fs::recursive_directory_iterator(root))
            if (item.is_regular_file() && item.path().extension() == ".isc") paths.push_back(fs::canonical(item.path()));
        std::sort(paths.begin(), paths.end());
        for (const auto& file : paths)
            if (discovered.try_emplace(file, file.lexically_relative(root)).second && library)
                project.library_files.insert(file.string());
    };
    for (const auto& directory : directories) scan(directory, false);
    for (const auto& directory : library_directories) scan(directory, true);
    for (const auto& input : files) {
        auto file = fs::canonical(input);
        if (!fs::is_regular_file(file)) throw std::runtime_error("Not a module source file: " + input);
        discovered.try_emplace(file, file.filename());
        project.library_files.erase(file.string());
    }
    for (auto& [file, relative] : discovered) {
        Source source;
        source.file = file.string();
        auto module_path = relative;
        module_path.replace_extension();
        for (const auto& component : module_path) source.name.push_back(component.string());
        std::ifstream in(file, std::ios::binary);
        if (!in) throw std::runtime_error("Cannot read module: " + file.string());
        source.grammar.assign(std::istreambuf_iterator<char>(in), {});
        SourceSyntax(source).run();
        project.modular |= source.has_directives && !project.library_files.contains(source.file);
        project.sources.push_back(std::move(source));
    }
    for (auto& source : bundled) {
        // Real files (local directories, explicit inputs) take precedence over a bundle.
        if (discovered.contains(source.file)) continue;
        project.library_files.insert(source.file);
        project.sources.push_back(std::move(source));
    }
    // Same order as a plain scan, so a bundle never changes generated output.
    std::ranges::stable_sort(project.sources, {}, &Source::file);
    project.input_files = files;
    project.search_directories = directories;
    project.library_search_directories = library_directories;
    return project;
}

Project preprocess(const std::vector<std::string>& directories, const std::vector<std::string>& files,
                   const std::map<std::string, std::string>& resolve,
                   const std::vector<std::string>& library_directories) {
    namespace fs = std::filesystem;
    auto project = discover(directories, files, library_directories);
    std::set<Path> local_names;
    for (const auto& source : project.sources)
        if (!project.library_files.contains(source.file)) local_names.insert(source.name);
    std::erase_if(project.sources, [&](const Source& source) {
        return project.library_files.contains(source.file) && local_names.contains(source.name);
    });
    const bool local_modular = std::ranges::any_of(project.sources, [&](const Source& source) {
        return !project.library_files.contains(source.file) && source.has_directives;
    });
    if (!local_modular) {
        std::erase_if(project.sources, [&](const Source& source) {
            return project.library_files.contains(source.file);
        });
        project.modular = false;
    }
    // Preserve the shared scope used by existing split grammars that contain
    // no module directives at all.
    if (!project.modular) return project;
    const Source* root = nullptr;
    if (!files.empty()) root = &project.source(fs::canonical(files.front()).string());
    else {
        for (const auto& source : project.sources)
            if (!project.library_files.contains(source.file))
            if (source.name == Path{"main"}) { root = &source; break; }
        if (!root) for (const auto& source : project.sources) {
            if (project.library_files.contains(source.file)) continue;
            if (source.entry || source.declarations.contains(Path{"main"})) {
                if (root) throw std::runtime_error("Multiple module entries; specify the root source file explicitly");
                root = &source;
            }
        }
    }
    if (!root) {
        const Source* only = nullptr;
        for (const auto& source : project.sources) {
            if (project.library_files.contains(source.file)) continue;
            if (only) { only = nullptr; break; }
            only = &source;
        }
        root = only;
    }
    if (root) {
        project.root = root->name;
        auto entry = root->entry;
        if (!entry && root->declarations.contains(Path{"main"})) entry = Path{"main"};
        if (entry) project.entry = joined(root->name, *entry);
    }
    if (root) {
        // Directory discovery makes modules available; only imported modules
        // (and explicit inputs) need their external dependencies resolved.
        std::set<Path> needed;
        auto include = [&](auto& self, const Source& source) -> void {
            if (!needed.insert(source.name).second) return;
            for (const auto& imp : source.imports) {
                if (!imp.external.empty()) continue;
                for (const auto& candidate : project.sources)
                    if (candidate.name == imp.source) self(self, candidate);
            }
        };
        include(include, *root);
        for (const auto& file : files) include(include, project.source(fs::canonical(file).string()));
        std::erase_if(project.sources, [&](const Source& source) { return !needed.contains(source.name); });
    }
    std::map<Path, std::size_t> modules;
    for (std::size_t index = 0; index < project.sources.size(); ++index) {
        auto& source = project.sources[index];
        for (const auto& part : source.name)
            if (part.empty() || !identifierStart(part[0]) || !std::ranges::all_of(part, identifierPart))
                fail({source.file}, "module path is not an identifier: " + spelling(source.name));
        if (!modules.emplace(source.name, index).second)
            fail({source.file}, "duplicate module " + spelling(source.name));
        if (source.entry && !source.declarations.contains(*source.entry))
            fail({source.file}, "module entry does not exist: " + spelling(*source.entry));
        for (auto& [name, pos] : source.declarations)
            source.resolved_symbols.emplace(name, joined(source.name, name));
    }
    std::map<Path, std::string> declarations;
    for (const auto& source : project.sources) for (const auto& [name, position] : source.declarations) {
        auto full = joined(source.name, name);
        if (!declarations.emplace(full, source.file).second)
            fail(position, "module and nested rule paths collide: " + spelling(full));
    }
    auto bind = [](Source& source, const Path& local, const Path& canonical, const Position& p) {
        const auto [it, inserted] = source.resolved_symbols.emplace(local, canonical);
        if (!inserted && it->second != canonical) fail(p, "ambiguous imported symbol " + spelling(local));
    };
    auto bind_feature = [](Source& source, const Path& local, const std::vector<Path>& symbols, const Position& p) {
        const auto [it, inserted] = source.allowed_features.emplace(local, symbols);
        if (!inserted && it->second != symbols) fail(p, "ambiguous imported feature " + spelling(local));
    };
    auto prefix = [](const Import& imp) -> Path {
        return imp.kind == Import::Kind::Qualified ? (imp.alias.empty() ? imp.source : Path{imp.alias}) : Path{};
    };
    auto selected = [](const Import& imp, const Path& name) {
        return imp.kind != Import::Kind::Selected || std::ranges::any_of(imp.symbols,
            [&](const Path& item) { return startsWith(name, item); });
    };
    std::map<std::string, ExternalPackage> packages;
    std::map<std::pair<std::string, Path>, Path> external_names;
    // Every module's declarations are already registered. Bind imports without
    // traversing dependency edges: mutually recursive rules need no build order.
    for (auto& source : project.sources) {
        for (const auto& imp : source.imports) {
            if (imp.python_import)
                continue;
            if (!imp.external.empty()) {
                if (!resolve.contains(imp.external)) fail(imp.position, "external parser needs --dep " + imp.external + " OUTPUT_PREFIX (or --no-dep with its grammar sources)");
                if (!packages.contains(imp.external)) packages.emplace(imp.external, readExternal(resolve.at(imp.external)));
                const auto& package = packages.at(imp.external);
                const auto found = package.find(imp.source);
                if (found == package.end()) fail(imp.position, "external module not exported: " + spelling(imp.source));
                const auto& target = found->second;
                for (const auto& symbol : imp.symbols)
                    if (!target.symbols.contains(symbol)) fail(imp.position, "external symbol not exported: " + spelling(symbol));
                auto canonical = [&](const Path& symbol) {
                    const auto key = std::make_pair(imp.external, joined(imp.source, symbol));
                    if (!external_names.contains(key)) {
                        Path name{"__external", imp.external, "symbol_" + std::to_string(external_names.size())};
                        project.external_symbols.emplace(name, target.symbols.at(symbol));
                        external_names.emplace(key, std::move(name));
                    }
                    return external_names.at(key);
                };
                for (const auto& [name, symbol] : target.symbols)
                    if (selected(imp, name)) bind(source, joined(prefix(imp), name), canonical(name), imp.position);
                if (imp.kind == Import::Kind::Qualified && target.entry)
                    bind(source, prefix(imp), canonical(*target.entry), imp.position);
                continue;
            }
            auto found = modules.find(imp.source);
            if (found == modules.end()) fail(imp.position, "module not found: " + spelling(imp.source));
            const auto& target = project.sources[found->second];
            if (imp.kind == Import::Kind::Selected) {
                for (const auto& symbol : imp.symbols)
                    if (!target.declarations.contains(symbol) && !target.features.contains(symbol))
                        fail(imp.position, "symbol not declared by " + spelling(target.name) + ": " + spelling(symbol));
            }
            for (const auto& [name, pos] : target.declarations)
                if (selected(imp, name)) bind(source, joined(prefix(imp), name), joined(target.name, name), imp.position);
            if (imp.kind == Import::Kind::Qualified) {
                auto entry = target.entry;
                if (!entry && target.declarations.contains(Path{"main"})) entry = Path{"main"};
                if (entry) bind(source, prefix(imp), joined(target.name, *entry), imp.position);
            }
        }
    }
    // Features contain symbol paths, so their definitions can be resolved once
    // all rule bindings exist, before importing any feature values.
    for (auto& source : project.sources) {
        for (const auto& [name, symbols] : source.features) {
            std::vector<Path> canonical;
            for (const auto& symbol : symbols) {
                auto found = source.resolved_symbols.find(symbol);
                if (found == source.resolved_symbols.end()) fail({source.file}, "unknown feature symbol " + spelling(symbol));
                canonical.push_back(found->second);
            }
            bind_feature(source, name, canonical, Position{source.file});
        }
    }
    for (auto& source : project.sources) {
        for (const auto& imp : source.imports) {
            if (imp.python_import || !imp.external.empty())
                continue;

            auto found = modules.find(imp.source);
            if (found == modules.end()) {
                fail(imp.position,
                     "module not found: " + spelling(imp.source));
            }

            const auto& target = project.sources[found->second];

            for (const auto& [name, symbols] : target.features) {
                if (selected(imp, name)) {
                    bind_feature(
                        source,
                        joined(prefix(imp), name),
                        target.allowed_features.at(name),
                        imp.position
                    );
                }
            }
        }
        for (const auto& detached : source.detach) {
            if (detached.feature) {
                const auto found = source.allowed_features.find(detached.name);
                if (found == source.allowed_features.end()) fail(detached.position, "unknown feature " + spelling(detached.name));
                source.detached_symbols.insert(found->second.begin(), found->second.end());
            } else {
                const auto found = source.resolved_symbols.find(detached.name);
                if (found == source.resolved_symbols.end()) fail(detached.position, "unknown detached symbol " + spelling(detached.name));
                source.detached_symbols.insert(found->second);
            }
        }
        source.allowed_symbols = source.resolved_symbols;
        std::erase_if(source.allowed_symbols, [&](const auto& binding) {
            return std::ranges::any_of(source.detached_symbols, [&](const Path& removed) { return startsWith(binding.second, removed); });
        });
    }
    return project;
}

std::size_t precompile(const std::string& directory) {
    namespace fs = std::filesystem;
    const auto root = fs::canonical(directory);
    const auto bundle = root / bundle_name;
    const auto grammar = grammarRoot(bundle), python = pythonRoot(bundle);
    const auto list = [](const fs::path& base, std::string_view extension) {
        std::vector<fs::path> paths;
        for (const auto& item : fs::recursive_directory_iterator(base))
            if (item.is_regular_file() && item.path().extension() == extension) paths.push_back(fs::canonical(item.path()));
        std::sort(paths.begin(), paths.end());
        return paths;
    };
    const auto sources = list(grammar, ".isc");
    const auto modules = list(python, ".py");
    BundleWriter writer;
    writer.out.assign(bundle_magic);
    writer.num(sources.size());
    for (const auto& file : sources) {
        Source source;
        source.file = file.string();
        auto module_path = file.lexically_relative(grammar);
        module_path.replace_extension();
        for (const auto& component : module_path) source.name.push_back(component.string());
        std::ifstream in(file, std::ios::binary);
        source.grammar.assign(std::istreambuf_iterator<char>(in), {});
        SourceSyntax(source).run();
        writeSource(writer, source, grammar);
    }
    writer.num(modules.size());
    for (const auto& file : modules) {
        writer.stamp(file, python);
        std::ifstream in(file, std::ios::binary);
        writer.str(std::string(std::istreambuf_iterator<char>(in), {}));
    }
    std::ofstream out(bundle, std::ios::binary | std::ios::trunc);
    out << writer.out;
    if (!out) throw std::runtime_error("Cannot write " + bundle.string());
    return sources.size() + modules.size();
}
}
