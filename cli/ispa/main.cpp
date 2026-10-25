#include <ispa/parser.h>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif
import cpuf.color;
import cpuf.printf;
import cpuf.op;
import cpuf.dlib;
import CLI;
import init;
import args;
import corelib;
import logging;
import Dump;
import AST.API;
import AST.types;
import AST.Tree;
import AST.BackendGuard;
import AST.Builder;
import UAST;
import Modules;
import External.Exports;
import AST.Pass;
import AST.Semantic;
import ISPA.Diagnostics;
import LangRepr.Converter;
import Algorithm.API;
import Algorithm.Cache;
import hash;
import AST.Builder;
import LexerBuilder;
import DFA;
import dstd;
import InstalledLibraries;

namespace {
    namespace diagnostics = ISPA::Diagnostics;

    std::filesystem::path executablePath(const char* argv0) {
        namespace fs = std::filesystem;
#ifdef _WIN32
        std::wstring buffer(32768, L'\0');
        const auto length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length && length < buffer.size()) return fs::weakly_canonical(fs::path(buffer.substr(0, length)));
#elif defined(__APPLE__)
        std::uint32_t length = 0;
        _NSGetExecutablePath(nullptr, &length);
        std::string buffer(length, '\0');
        if (_NSGetExecutablePath(buffer.data(), &length) == 0)
            return fs::weakly_canonical(fs::path(buffer.c_str()));
#else
        std::error_code error;
        auto path = fs::canonical("/proc/self/exe", error);
        if (!error) return path;
#endif
        return fs::weakly_canonical(fs::path(argv0));
    }

    void printDiagnostics(const diagnostics::Reporter& reporter) {
        reporter.print(std::cerr);
        reporter.printSummary(std::cerr);
    }

    void printExceptionDiagnostic(std::string code, const std::exception& error) {
        const std::string message = error.what();
        diagnostics::Reporter reporter;
        diagnostics::Location location;
        std::string display_message = message;
        for (std::size_t colon = message.find(':'); colon != std::string::npos;
             colon = message.find(':', colon + 1)) {
            const std::string file = message.substr(0, colon);
            std::size_t cursor = colon + 1;
            auto number = [&](std::size_t& value) {
                const auto begin = cursor;
                while (cursor < message.size() && message[cursor] >= '0' && message[cursor] <= '9') ++cursor;
                if (begin == cursor || cursor >= message.size() || message[cursor] != ':') return false;
                value = std::stoull(message.substr(begin, cursor - begin));
                ++cursor;
                return true;
            };
            std::size_t line = 0, column = 0;
            if (!number(line) || !number(column) || cursor >= message.size() || message[cursor] != ' ' ||
                !std::filesystem::is_regular_file(file))
                continue;
            location = {file, std::string::npos, 1, line, column};
            std::ifstream input(file, std::ios::binary);
            if (input) reporter.source(file, {std::istreambuf_iterator<char>{input}, {}});
            display_message = message.substr(cursor + 1);
            break;
        }
        reporter.error(std::move(code), std::move(display_message), std::move(location));
        printDiagnostics(reporter);
    }

    std::string sourceSnapshot(const Modules::Source& source) {
        std::ifstream input(source.file, std::ios::binary);
        if (!input) return source.grammar;
        return {std::istreambuf_iterator<char>{input}, {}};
    }

    UAST::SourcePosition semanticPosition(const AST::SourcePosition& position) {
        return {position.file, position.offset, position.length, position.line, position.column};
    }

    void registerAction(AST::SemanticRuleInfo& info, const AST::PythonAction& action) {
        if (const auto* expression = std::get_if<UAST::py::Expr>(&action.code))
            info.embeddedExpressions.push_back(*expression);
        if (const auto* statements = std::get_if<UAST::py::Module>(&action.code))
            info.embeddedStatements.push_back(*statements);
        for (const auto& binding : action.bindings)
            info.externalCaptureReads.insert(binding.name);
    }

    void registerFeatures(AST::SemanticRuleInfo& info, const auto& features) {
        for (const auto& feature : features) {
            if (const auto* declaration = std::get_if<AST::DeclarationSyntax>(&feature)) {
                info.modifiers = declaration->modifiers;
                for (const auto& name : declaration->parameters)
                    info.parameters.push_back({name, semanticPosition(declaration->position), std::nullopt});
                for (const auto& name : declaration->templateParameters)
                    info.templateParameters.push_back({name, semanticPosition(declaration->position), std::nullopt});
                for (const auto& action : declaration->parameterAnnotations) registerAction(info, action);
            } else if (const auto* action = std::get_if<AST::PythonAction>(&feature)) {
                registerAction(info, *action);
            } else if (const auto* dynamic = std::get_if<AST::DynamicPattern>(&feature)) {
                registerAction(info, dynamic->expression);
            } else if (const auto* counter = std::get_if<AST::Counter>(&feature)) {
                registerAction(info, counter->expression);
            }
        }
    }

    void registerMemberActions(AST::SemanticRuleInfo& info, const AST::RuleMember& member) {
        registerFeatures(info, member.features);
        registerAction(info, member.failure_handler.action);
        if (const auto* group = std::get_if<AST::RuleMemberGroup>(&member.value))
            for (const auto& child : group->values)
                if (child) registerMemberActions(info, *child);
        if (const auto* alternatives = std::get_if<AST::RuleMemberOp>(&member.value))
            for (const auto& child : alternatives->options)
                if (child) registerMemberActions(info, *child);
    }

    // --- generic "key \t hash" manifest, shared by the rule-cache and
    // the output-content-cache below (same file format, different keys) ---
    auto loadKeyHashMap(const std::filesystem::path &path) -> std::unordered_map<std::string, std::size_t> {
        std::unordered_map<std::string, std::size_t> map;
        std::ifstream in(path);
        std::string line;
        while (std::getline(in, line)) {
            const auto tab = line.find('\t');
            if (tab == std::string::npos) continue;
            map[line.substr(0, tab)] = std::stoull(line.substr(tab + 1));
        }
        return map;
    }
    auto saveKeyHashMap(const std::filesystem::path &path, const std::unordered_map<std::string, std::size_t> &map) -> void {
        std::ofstream out(path, std::ios::trunc);
        for (const auto &[key, hash] : map)
            out << key << '\t' << hash << '\n';
    }

    // --- write generated files, skipping unchanged content (so mtimes and
    // downstream build graphs stay untouched), then prune files this run
    // no longer produces ---
    struct WriteResult {
        std::unordered_map<std::string, std::size_t> hashes;
        std::size_t updated = 0;
    };
    auto writeGeneratedFiles(
        const std::filesystem::path &dir_path,
        const std::vector<std::pair<std::string, std::string>> &files,
        const std::unordered_map<std::string, std::size_t> &previous_hashes
    ) -> WriteResult {
        WriteResult result;
        std::unordered_set<std::string> expected;

        for (const auto &[rel_path, content] : files) {
            const auto path = dir_path / rel_path;
            if (rel_path.starts_with("incremental/"))
                std::filesystem::create_directories(path.parent_path());

            expected.insert(rel_path);
            const std::size_t content_hash = std::hash<std::string>{}(content);
            result.hashes[rel_path] = content_hash;

            const auto previous = previous_hashes.find(rel_path);
            if (previous != previous_hashes.end() &&
                previous->second == content_hash &&
                std::filesystem::exists(path)) {
                continue; // unchanged — leave the file and its mtime alone
            }

            std::ofstream out(path, std::ios::binary | std::ios::trunc);
            if (!out)
                throw std::runtime_error("Cannot write: " + path.string());
            out.exceptions(std::ios::badbit | std::ios::failbit);
            out << content;
            ++result.updated;
        }

        for (const auto &[key, hash] : previous_hashes) {
            if (expected.contains(key)) continue;
            if (std::filesystem::remove(dir_path / key))
                cpuf::printf("removed stale -> {}\n", (dir_path / key).string());
        }
        return result;
    }
}

int generate(Args args);

// Plan external packages from the same discovered grammar sources, then run
// each package through the ordinary generator with its own namespace/cache.
// No generated parser implementation is folded into its consumer.
void generateDependencies(Args& args) {
    namespace fs = std::filesystem;
    const auto discovered = Modules::discover(args.dir, args.files, args.library_dirs);
    if (!discovered.modular) return;
    std::map<Modules::Path, const Modules::Source*> sources;
    for (const auto& source : discovered.sources) {
        auto [it, inserted] = sources.emplace(source.name, &source);
        if (!inserted && discovered.library_files.contains(source.file)) continue;
        if (!inserted && discovered.library_files.contains(it->second->file)) {
            it->second = &source;
            continue;
        }
        if (!inserted) throw std::runtime_error("Duplicate module: " + Modules::spelling(source.name));
    }
    std::map<std::string, std::string> dependencies;
    for (const auto& [name, prefix] : args.dependencies)
        if (!dependencies.emplace(name, prefix).second)
            throw std::runtime_error("Duplicate --dep binding: " + name);
    const Modules::Source* root = nullptr;
    if (!args.files.empty()) root = &discovered.source(fs::canonical(args.files.front()).string());
    else {
        for (const auto& source : discovered.sources)
            if (!discovered.library_files.contains(source.file))
            if (source.name == Modules::Path{"main"}) { root = &source; break; }
        if (!root)
        for (const auto& source : discovered.sources) {
            if (discovered.library_files.contains(source.file)) continue;
            if (source.entry || source.declarations.contains({"main"})) {
                if (root) throw std::runtime_error("Multiple module entries; specify the root source file explicitly");
                root = &source;
            }
        }
    }
    if (!root) {
        for (const auto& source : discovered.sources) {
            if (discovered.library_files.contains(source.file)) continue;
            if (root) throw std::runtime_error("Multiple source modules; specify the root source file explicitly");
            root = &source;
        }
    }
    if (!root) throw std::runtime_error("--no-dep requires a root grammar source");

    auto closure = [&](const std::set<Modules::Path>& roots) {
        std::set<Modules::Path> visited;
        auto visit = [&](auto& self, const Modules::Path& name) -> void {
            if (!visited.insert(name).second) return;
            const auto found = sources.find(name);
            if (found == sources.end()) throw std::runtime_error("Dependency grammar module not found: " + Modules::spelling(name));
            for (const auto& imp : found->second->imports)
                if (imp.external.empty()) self(self, imp.source);
        };
        for (const auto& name : roots) visit(visit, name);
        return visited;
    };
    const auto host = closure({root->name});
    std::map<std::string, std::set<Modules::Path>> packages;
    auto collect = [&](const std::set<Modules::Path>& modules) {
        bool changed = false;
        for (const auto& module : modules)
            for (const auto& imp : sources.at(module)->imports)
                if (!imp.external.empty() && !dependencies.contains(imp.external))
                    changed |= packages[imp.external].insert(imp.source).second;
        return changed;
    };
    collect(host);
    for (bool changed = true; changed;) {
        changed = false;
        const auto snapshot = packages;
        for (const auto& [name, roots] : snapshot) changed |= collect(closure(roots));
    }
    const auto output_dir = fs::path(args.output).has_parent_path() ? fs::path(args.output).parent_path() : fs::path(".");
    const auto host_name = fs::path(args.output).filename().string();
    std::set<std::string> visiting, built;
    auto build = [&](auto& self, const std::string& name) -> void {
        if (built.contains(name)) return;
        if (!visiting.insert(name).second) throw std::runtime_error("Cyclic external parser dependency: " + name);
        if (name == host_name) throw std::runtime_error("Dependency output collides with host parser: " + name);
        const auto modules = closure(packages.at(name));
        for (const auto& module : modules)
            for (const auto& imp : sources.at(module)->imports)
                if (!imp.external.empty() && packages.contains(imp.external)) self(self, imp.external);
        Args child = args;
        child.no_dep = false;
        child.dependency_build = true;
        child.dir.clear();
        child.files.clear();
        for (const auto& module : modules) child.files.push_back(sources.at(module)->file);
        child.output = (output_dir / name).string();
        child.dependencies.assign(dependencies.begin(), dependencies.end());
        if (generate(std::move(child)) != 0) throw std::runtime_error("Failed to generate external parser: " + name);
        dependencies.emplace(name, (output_dir / name).string());
        visiting.erase(name);
        built.insert(name);
    };
    for (const auto& [name, roots] : packages) build(build, name);
    args.dependencies.assign(dependencies.begin(), dependencies.end());
    args.dir.clear();
    args.files = {root->file};
    for (const auto& module : host) if (module != root->name) args.files.push_back(sources.at(module)->file);
}

int generate(Args args) {
    UAST::main modules;
    std::vector<std::string> source_files;
    if (args.no_dep) generateDependencies(args);
    dumper.setArgsPointer(&args);
    dumper.initDumpDirectory();
    if (logger.enabled()) logger.clearLogDirectory();

    if (args.files.empty() && args.dir.empty())
        throw UError("No input files");

    Modules::Project module_project;
    try {
        std::map<std::string, std::string> resolve;
        for (const auto& [name, path] : args.dependencies)
            if (!resolve.emplace(name, path).second) throw std::runtime_error("Duplicate --dep binding: " + name);
        module_project = Modules::preprocess(args.dir, args.files, resolve, args.library_dirs);
        if (args.dependency_build) {
            module_project.root.reset();
            // A library module can export rules without declaring a default
            // entry. The generated parser class still needs an internal main.
            if (!module_project.entry) {
                for (const auto& source : module_project.sources) {
                    if (source.declarations.empty()) continue;
                    auto relative = source.entry.value_or(source.declarations.begin()->first);
                    auto entry = source.name;
                    entry.insert(entry.end(), relative.begin(), relative.end());
                    module_project.entry = std::move(entry);
                    if (source.entry) break;
                }
            }
        }
    } catch (const std::exception& error) {
        printExceptionDiagnostic("P000", error);
        return 1;
    }
    diagnostics::Reporter parse_diagnostics;
    for (const auto& source : module_project.sources) {
        parse_diagnostics.source(source.file, sourceSnapshot(source));
        IspaParser parser;
        const auto collect_errors = [&] {
            for (const auto& e : parser.getErrors()) {
                const auto offset = e.offset <= source.grammar.size()
                    ? e.offset : std::string::npos;
                parse_diagnostics.error("P001", e.message,
                    {source.file, offset, e.length});
            }
            for (const auto& e : parser.getContextErrors()) {
                const auto offset = e.offset <= source.grammar.size()
                    ? e.offset : std::string::npos;
                parse_diagnostics.error("P002", e.message,
                    {source.file, offset, e.length});
            }
        };
        decltype(&parser.parse(source.grammar.c_str()).value.data()) syntax = nullptr;
        try {
            syntax = &parser.parse(source.grammar.c_str()).value.data();
        } catch (const std::exception& error) {
            collect_errors();
            if (parser.getErrors().empty() && parser.getContextErrors().empty())
                parse_diagnostics.error("P000", error.what(), {source.file});
            printDiagnostics(parse_diagnostics);
            return 1;
        }
        collect_errors();
        if (!parser.getErrors().empty() || !parser.getContextErrors().empty()) {
            printDiagnostics(parse_diagnostics);
            return 1;
        }
        modules.value.insert(modules.value.end(), syntax->value.begin(), syntax->value.end());
        source_files.insert(source_files.end(), syntax->value.size(), source.file);
    }

    if (dumper.shouldDump("NFA"))
        std::ofstream(dumper.makeDumpPath("NFA"), std::ios::trunc);
    if (dumper.shouldDump("DFA"))
        std::ofstream(dumper.makeDumpPath("DFA"), std::ios::trunc);

    AST::Builder ast_builder(modules);
    ast_builder.setSourceFiles(std::move(source_files));
    ast_builder.setModules(module_project);
    ast_builder.build();
    auto ast = ast_builder.get();
    AST::requireCppSupport(ast, args.algorithm == Args::Algorithm::LL);

    AST::Semantic semantic_checker;
    for (const auto& source : module_project.sources)
        semantic_checker.addSource(source.file, sourceSnapshot(source));
    std::set<std::string> registered_python;
    for (const auto& source : module_project.sources)
        for (const auto& imported : source.imports) {
            if (!imported.python_import) continue;
            const auto module_name = Modules::spelling(imported.source);
            if (!registered_python.insert(module_name).second) continue;
            std::filesystem::path relative;
            for (const auto& part : imported.source) relative /= part;
            std::vector<std::filesystem::path> roots;
            for (const auto& dir : module_project.search_directories) roots.emplace_back(dir);
            roots.emplace_back(std::filesystem::path(source.file).parent_path());
            const auto local_root_count = roots.size();
            for (const auto& dir : module_project.library_search_directories) roots.emplace_back(dir);
            const auto canonicalFile = [](const std::filesystem::path& path) {
                std::error_code error;
                if (!std::filesystem::is_regular_file(path, error) || error) return std::filesystem::path{};
                auto result = std::filesystem::canonical(path, error);
                return error ? std::filesystem::path{} : result;
            };
            std::filesystem::path resolved;
            for (const auto& input_path : module_project.input_files) {
                const std::filesystem::path input_file(input_path);
                if (input_file.extension() != ".py") continue;
                auto actual = canonicalFile(input_file);
                if (actual.empty()) continue;
                bool matches = imported.source.size() == 1 && actual.stem() == imported.source.front();
                for (const auto& root : roots) {
                    std::error_code error;
                    auto from_root = std::filesystem::relative(actual, root, error);
                    if (!error && (from_root == std::filesystem::path(relative.string() + ".py") ||
                                   from_root == relative / "__init__.py")) matches = true;
                }
                if (!matches) continue;
                if (!resolved.empty() && resolved != actual)
                    throw std::runtime_error("ambiguous Python module: " + module_name);
                resolved = std::move(actual);
            }
            if (resolved.empty()) for (std::size_t index = 0; index < roots.size(); ++index) {
                if (index == local_root_count && !resolved.empty()) break;
                const auto& root = roots[index];
                auto candidate_path = relative;
                candidate_path += ".py";
                auto candidate = canonicalFile(root / candidate_path);
                if (candidate.empty()) candidate = canonicalFile(root / relative / "__init__.py");
                if (candidate.empty()) continue;
                if (!resolved.empty() && resolved != candidate)
                    throw std::runtime_error("ambiguous Python module: " + module_name);
                resolved = std::move(candidate);
            }
            if (!resolved.empty()) {
                std::ifstream input(resolved, std::ios::binary);
                if (input) semantic_checker.addPythonSource(module_name, resolved.string(),
                    std::string(std::istreambuf_iterator<char>{input}, {}));
            }
        }
    for (const auto& [path, rule] : ast.getTreeMap()) {
        AST::SemanticRuleInfo info;
        info.name = corelib::text::join(path, "::");
        info.rule = &rule;
        registerFeatures(info, rule.features);
        for (const auto& member : rule.rule_members)
            if (member) registerMemberActions(info, *member);
        semantic_checker.addRule(std::move(info));
    }
    semantic_checker.check();
    if (!semantic_checker.diagnostics().diagnostics().empty())
        printDiagnostics(semantic_checker.diagnostics());
    if (semantic_checker.diagnostics().hasErrors()) return 1;
    AST::TreePass pass(ast);
    pass.work();
    {
        std::ofstream treeAPIO("treeAPI.txt");
        for (const auto &[name, value] : ast.getTreeMap())
            treeAPIO << "name<" << corelib::text::join(name, "_") << "> : " << value;
    }

    std::filesystem::path output_path = args.output;
    LexerBuilder lexer_data(ast);
    lexer_data.build();

    std::string name;
    if (!ast.getName().empty() && !args.dependency_build) {
        name = ast.getName();
        output_path.append(ast.getName());
    } else {
        name = std::filesystem::path(args.output).filename().string();
        cpuf::printf("Output directory: {}; using filename: {}\n", std::filesystem::path(args.output).parent_path().string(), name.c_str());
    }

    const auto dir_path = output_path.has_parent_path() ? output_path.parent_path() : std::filesystem::path{"."};
    std::filesystem::create_directories(dir_path);
    std::filesystem::create_directories(dir_path / "incremental");

    // Every algorithm uses the same entry point; output conversion belongs to it.
    const auto algorithm_name = to_string(args.algorithm);
    dlib algorithm_lib(InstalledLibraries::pluginPrefix(args.executable_dir, "algorithm", algorithm_name));
    const auto* algorithm = algorithm_lib.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    if (!algorithm || algorithm->version != Algorithm::interface_version ||
        !algorithm->id || algorithm_name != algorithm->id || !algorithm->build)
        throw std::runtime_error("Incompatible algorithm module: " + algorithm_name);
    const auto cache_dir = dir_path / ".ispa-cache" / name /
        (algorithm_name + "-" + std::to_string(algorithm->cache_version)) /
        (args.language_str + "-" + std::to_string(args.auto_conflicts) + "-" +
         std::to_string(args.conflict_lookahead));
    std::filesystem::create_directories(cache_dir);
    auto metadata = Algorithm::loadMetadata(cache_dir / "data");
    const auto rule_manifest_path = cache_dir / "rule-hashes";
    const auto previous_rule_hashes = loadKeyHashMap(rule_manifest_path);

    const auto &tree_map = ast.getTreeMap();
    AST::TreeMap changed_rules;
    std::unordered_map<std::string, std::size_t> new_rule_hashes;
    for (const auto &[path, rule] : tree_map) {
        std::ostringstream oss;
        oss << rule; // reuses AST::Rule's existing operator<<
        const std::size_t rule_hash = std::hash<std::string>{}(oss.str());
        const auto key = corelib::text::join(path, ".");
        new_rule_hashes[key] = rule_hash;

        const auto prev = previous_rule_hashes.find(key);
        if (prev == previous_rule_hashes.end() || prev->second != rule_hash)
            changed_rules.emplace(path, rule);
    }
    cpuf::printf("{} of {} rules changed since last run\n", changed_rules.size(), tree_map.size());
    const Algorithm::BuildRequest build_request{ast, changed_rules,
        args.auto_conflicts, args.conflict_lookahead};
    auto built = algorithm->build(build_request, metadata);
    if (!built.output || !built.to_lang_repr)
        throw std::runtime_error("Algorithm returned no LangRepr converter: " + algorithm_name);
    Algorithm::ConversionRequest conversion_request{lexer_data, ast, args.language, name};
    auto holder = built.to_lang_repr(built.output.get(), conversion_request);

    // dlib-loads libispa-converter-<lang> and resolves "<lang>_construct"
    // (e.g. the extern "C" cpp_construct in converter.cpp) internally.
    LangRepr::Converter converter(holder, to_string(args.language), name, args.incremental,
        InstalledLibraries::pluginPrefix(args.executable_dir, "converter", to_string(args.language)));
    converter.init();
    converter.construct();

    const auto manifest_path = dir_path / (".codegen-manifest." + name);
    const auto previous_hashes = loadKeyHashMap(manifest_path);
    auto generated = converter.get();
    if (args.algorithm == Args::Algorithm::LL) {
        auto exports = External::exports(ast, module_project, name, args.language == Args::Language::Python);
        for (auto& [file, code] : exports) {
            auto target = std::ranges::find_if(generated, [&](const auto& item) { return item.first == file; });
            if (target == generated.end()) generated.emplace_back(std::move(file), std::move(code));
            else target->second += code;
        }
    }
    const auto result = writeGeneratedFiles(dir_path, generated, previous_hashes);

    saveKeyHashMap(manifest_path, result.hashes);
    Algorithm::saveMetadata(cache_dir / "data", metadata);
    saveKeyHashMap(rule_manifest_path, new_rule_hashes);

    std::cout << "Updated " << result.updated << " file(s)\n";
    return 0;
}

int main(int argc, char** argv) {
    try {
        auto args = init(argc, argv);
        if (args.logs || args.debug) logger.includeAll();
        else logger.excludeAll();
        if (args.version) {
            cpuf::printf(
                "{};\n"
                        "ISPA is an open source parser generator (https://github.com/Sinfolke/ispa-parser-generator)\n"
                        "ISPA responsibility is to generate everything, so your work belongs only writing grammar and sometimes python scripts alongside it\n"
                        "All your error messages are also part of .isc files, unless you want to put them outside\n"
                        "You can output different target languages. See --help for more\n"
                        "And different parsing algorithms can be used, like LL (recursive descent top-level) and LR (bottom-up)\n"
                        "GOOD LUCK!\n",
                        PROGRAM_VERSION
            );
            return 0;
        }
        const auto executable = executablePath(argv[0]);
        args.executable_dir = executable.parent_path().string();
        if (!args.version) {
            const auto libraries = InstalledLibraries::discover(
                InstalledLibraries::root(executable));
            args.library_dirs = libraries.source_directories;
            std::set<std::string> explicit_names;
            for (const auto& [name, prefix] : args.dependencies) explicit_names.insert(name);
            for (const auto& [name, prefix] : libraries.dependencies)
                if (!explicit_names.contains(name)) args.dependencies.emplace_back(name, prefix);
        }
        return generate(std::move(args));
    }
    catch (const std::exception& error) {
        printExceptionDiagnostic("E000", error);
        return 1;
    }
}
