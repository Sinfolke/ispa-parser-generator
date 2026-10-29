module Python.Imports;
import UAST.Python;
import Parser.Base;
import std;

namespace Python {
namespace {
namespace py = UAST::py;
namespace fs = std::filesystem;

[[noreturn]] void fail(const UAST::SourcePosition& p, const std::string& message) {
    throw std::invalid_argument(p.file + ":" + std::to_string(p.line) + ":" +
        std::to_string(p.column) + ": Python import: " + message);
}
// Walk the typed syntax, including nested suites, without reparsing Python text.
template<class T, class Visitor> void walk(T& value, Visitor& visitor) {
    if constexpr (std::is_same_v<T, py::Expr>) visitor.expression(value);
    else if constexpr (std::is_same_v<T, py::Stmt>) visitor.statement(value);
    else if constexpr (requires { std::visit([](auto&) {}, value); }) {
        std::visit([&](auto& item) { walk(item, visitor); }, value);
    } else if constexpr (requires { value.get(); *value; }) {
        if (value) walk(*value, visitor);
    } else if constexpr (requires { value.members(); }) {
        std::apply([&](auto&... items) { (walk(items, visitor), ...); }, value.members());
    } else if constexpr (std::ranges::range<T> && !std::is_same_v<T, std::string>) {
        for (auto& item : value) walk(item, visitor);
    }
}
struct Validate {
    void expression(py::Expr& e) { walk(e.kind, *this); }
    void statement(py::Stmt& s) {
        if (s.get<py::Import>())
            fail(s.pos, "imports are supported only at module level in imported Python files");
        walk(s.kind, *this);
    }
};
std::optional<std::pair<std::string, std::optional<std::size_t>>> native(const py::Stmt& statement) {
    auto assignment = statement.get<py::Assign>();
    if (!assignment || !assignment->value) return std::nullopt;
    auto call = assignment->value->get<py::Call>();
    if (!call || !call->func || call->func->get<py::Name>() == nullptr ||
        call->func->get<py::Name>()->id != "__ispa_internal__") return std::nullopt;
    if (call->args.empty() || call->args.size() > 2 || !call->keywords.empty())
        fail(statement.pos, "__ispa_internal__ expects a native name and optional arity");
    auto name = call->args[0] ? call->args[0]->get<py::Constant>() : nullptr;
    if (!name || name->kind != py::ConstantKind::Str ||
        !(name->value.starts_with("__ispa_parser__.") || name->value.starts_with("ISPA_STD.") || name->value.starts_with("__ispa_ir__.")) ||
        name->value.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_.") != std::string::npos)
        fail(statement.pos, "invalid native name in __ispa_internal__");
    std::optional<std::size_t> arity;
    if (call->args.size() == 2) {
        auto number = call->args[1] ? call->args[1]->get<py::Constant>() : nullptr;
        if (!number || number->kind != py::ConstantKind::Int) fail(statement.pos, "native arity must be an integer");
        try { arity = std::stoull(number->value); }
        catch (const std::exception&) { fail(statement.pos, "invalid native arity"); }
    }
    return std::make_pair(name->value, arity);
}
std::string path(const py::Expr& e) {
    if (auto n = e.get<py::Name>()) return n->id;
    if (auto a = e.get<py::Attribute>(); a && a->value) {
        auto prefix = path(*a->value);
        if (!prefix.empty()) return prefix + "." + a->attr;
    }
    return {};
}
bool nativeName(const std::string& name) {
    return name.starts_with("__ispa_parser__.") || name.starts_with("ISPA_STD.") || name.starts_with("__ispa_ir__.");
}
struct Locals {
    std::set<std::string> names;
    void expression(py::Expr& e) {
        if (auto n = e.get<py::Name>(); n && n->ctx == py::ExprContext::Store) names.insert(n->id);
        walk(e.kind, *this);
    }
    void statement(py::Stmt& s) { walk(s.kind, *this); }
};
template<class Bindings> struct Bind {
    const Bindings& bindings;
    std::set<std::string> locals;
    bool reserve_imports = false;
    void expression(py::Expr& e) {
        const auto name = path(e);
        const auto first = name.substr(0, name.find('.'));
        if (!locals.contains(first)) {
            if (auto found = bindings.find(name); found != bindings.end()) {
                if (reserve_imports) if (auto n = e.get<py::Name>(); n && n->ctx != py::ExprContext::Load)
                    fail(e.pos, "cannot rebind imported name " + name);
                e.kind = py::Name{found->second.qualified};
                return;
            }
            for (const auto& [binding, target] : bindings) {
                if (nativeName(target.qualified) && name.starts_with(binding + ".")) {
                    e.kind = py::Name{target.qualified + name.substr(binding.size())};
                    return;
                }
            }
            for (const auto& [binding, target] : bindings)
                if (name == binding.substr(0, binding.find('.')) && !bindings.contains(name))
                    fail(e.pos, "unknown imported declaration " + name);
        }
        walk(e.kind, *this);
        if (auto call = e.get<py::Call>(); call && call->func) {
            const auto target = path(*call->func);
            for (const auto& [binding, item] : bindings) if (item.qualified == target) {
                if (!item.arity) {
                    if (nativeName(item.qualified)) continue;
                    fail(e.pos, "imported variable is not callable: " + binding);
                }
                if (!call->keywords.empty() || call->args.size() != *item.arity)
                    fail(e.pos, binding + " expects " + std::to_string(*item.arity) + " positional arguments");
            }
        }
    }
    void statement(py::Stmt& s) {
        walk(s.kind, *this);
    }
};
void annotation(const py::ExprPtr& e, const UAST::SourcePosition& at, bool returns = false) {
    if (!e) fail(at, "function parameters and returns require type annotations");
    if (e->get<py::Name>() || e->get<py::Attribute>() || e->get<py::Subscript>()) return;
    if (returns) if (auto c = e->get<py::Constant>(); c && c->kind == py::ConstantKind::None) return;
    fail(e->pos, "unsupported type annotation");
}
}

Imports::Imports(const Modules::Project* project) {
    if (project) for (const auto& source : project->sources)
        for (const auto& imp : source.imports)
            if (imp.python_import)
                load(source, imp, *project);
}
std::string Imports::module(const Modules::Path& source_path, const std::string& from_file,
                            const Modules::Project& project, const UAST::SourcePosition& at,
                            const fs::path& package_root) {
    // Resolve only Python modules here; grammar modules are handled by Modules::preprocess.
    fs::path relative;
    for (const auto& part : source_path) {
        if (part.empty() || part == "." || part == ".." ||
            part.find('/') != std::string::npos ||
            part.find('\\') != std::string::npos)
            fail(at, "invalid Python module component: " + part);
        relative /= part;
    }
    if (relative.empty()) fail(at, "empty Python module path");

    std::vector<fs::path> roots;
    roots.reserve(project.search_directories.size() + project.library_search_directories.size() + 1);
    for (const auto& directory : project.search_directories)
        roots.emplace_back(directory);
    roots.push_back(fs::path(from_file).parent_path());
    if (!package_root.empty()) roots.push_back(package_root);
    const auto local_root_count = roots.size();
    for (const auto& directory : project.library_search_directories)
        roots.emplace_back(directory);

    auto tryFile = [](const fs::path& candidate) -> fs::path {
        std::error_code error;
        if (!fs::is_regular_file(candidate, error) || error) return {};
        auto canonical = fs::canonical(candidate, error);
        return error ? fs::path{} : canonical;
    };

    fs::path canonical;
    // Explicit .py files can be outside the directory search roots.
    for (const auto& input : project.input_files) {
        const fs::path file(input);
        if (file.extension() != ".py") continue;
        const auto actual = tryFile(file);
        if (actual.empty()) continue;
        bool matches = source_path.size() == 1 && actual.stem() == source_path.front();
        for (const auto& root : roots) {
            std::error_code error;
            const auto relative_file = fs::relative(actual, root, error);
            if (!error && (relative_file == fs::path(relative.string() + ".py") ||
                           relative_file == relative / "__init__.py"))
                matches = true;
        }
        if (matches) {
            if (!canonical.empty() && canonical != actual)
                fail(at, "ambiguous Python module: " + Modules::spelling(source_path));
            canonical = actual;
        }
    }
    if (canonical.empty()) {
        for (std::size_t index = 0; index < roots.size(); ++index) {
            if (index == local_root_count && !canonical.empty()) break;
            const auto& root = roots[index];
            auto file = relative;
            file += ".py";
            auto candidate = tryFile(root / file);
            if (candidate.empty()) candidate = tryFile(root / relative / "__init__.py");
            if (candidate.empty()) continue;
            if (!canonical.empty() && candidate != canonical)
                fail(at, "ambiguous Python module: " + Modules::spelling(source_path));
            canonical = std::move(candidate);
        }
    }
    if (canonical.empty())
        fail(at, "Python module not found: " + Modules::spelling(source_path));
    const auto name = Modules::spelling(source_path);
    if (paths.contains(name) && paths.at(name) != canonical)
        fail(at, "module " + name + " resolves to different Python files");
    if (!paths.contains(name)) {
        std::ifstream input(canonical, std::ios::binary);
        if (!input) fail(at, "cannot read " + canonical.string());
        const std::string text(std::istreambuf_iterator<char>{input}, {});
        auto syntax = UAST::parsePythonModule(text, {canonical.string(), 0, text.size(), 1, 1});
        if (!loading.insert(name).second) fail(at, "circular Python import: " + name);
        Validate validate;
        for (auto& statement : syntax.body)
            if (!statement->get<py::Import>() && !statement->get<py::ImportFrom>()) walk(statement->kind, validate);
        Bindings dependencies;
        std::set<std::string> imported_classes;
        // Package root: strip the module's own path (and __init__) from its file location.
        auto package_base = canonical.parent_path();
        for (std::size_t i = canonical.filename() == "__init__.py" ? 0 : 1; i < source_path.size(); ++i)
            package_base = package_base.parent_path();
        const auto inherit = [&](const std::string& module_name, const std::string& prefix,
                                 const std::string& only, const UAST::SourcePosition& where) {
            for (const auto& [key, value] : exports.at(module_name)) {
                if (!only.empty() && key != only && !key.starts_with(only + ".")) continue;
                if (!dependencies.emplace(prefix + key, value).second &&
                    dependencies.at(prefix + key).qualified != value.qualified)
                    fail(where, "duplicate imported name " + prefix + key);
            }
        };
        const auto split = [](const std::string& dotted) {
            Modules::Path parts;
            std::size_t begin = 0;
            while (begin <= dotted.size()) {
                const auto end = dotted.find('.', begin);
                parts.push_back(dotted.substr(begin, end == std::string::npos ? end : end - begin));
                if (end == std::string::npos) break;
                begin = end + 1;
            }
            return parts;
        };
        const auto ignorable = [](const std::string& module_name) {
            return module_name == "typing" || module_name == "__future__" || module_name == "__ispa_internal__";
        };
        for (const auto& statement : syntax.body) if (auto plain = statement->get<py::Import>()) {
            for (const auto& item : plain->names) {
                if (ignorable(item.name)) continue;
                const auto target = module(split(item.name), canonical.string(), project, item.pos, package_base);
                inherit(target, (item.asname.empty() ? target : item.asname) + ".", "", item.pos);
            }
        }
        for (const auto& statement : syntax.body) if (auto from = statement->get<py::ImportFrom>()) {
            const bool internal = (from->module_name == "internals" || from->module_name == "langapi") && from->level <= 1;
            if (!internal) {
                if (from->level == 0 && ignorable(from->module_name)) continue;
                Modules::Path base;
                if (from->level > 0) {
                    base = source_path;
                    if (canonical.filename() != "__init__.py") base.pop_back();
                    for (std::size_t i = 1; i < from->level; ++i) {
                        if (base.empty()) fail(statement->pos, "relative import goes beyond the top-level package");
                        base.pop_back();
                    }
                }
                if (!from->module_name.empty()) for (auto& part : split(from->module_name)) base.push_back(std::move(part));
                if (base.empty()) fail(statement->pos, "empty Python import");
                for (const auto& item : from->names) {
                    const auto key = item.asname.empty() ? item.name : item.asname;
                    if (item.name == "*") {
                        inherit(module(base, canonical.string(), project, item.pos, package_base), "", "", item.pos);
                        continue;
                    }
                    auto submodule = base;
                    submodule.push_back(item.name);
                    const auto sub_name = Modules::spelling(submodule);
                    if (!paths.contains(sub_name)) {
                        // `from pkg import name` names a submodule when pkg/name(.py|/__init__.py) exists.
                        std::error_code probe;
                        bool is_module = false;
                        fs::path relative_sub;
                        for (const auto& part : submodule) relative_sub /= part;
                        std::vector<fs::path> probe_roots{package_base};
                        for (const auto& directory : project.search_directories) probe_roots.emplace_back(directory);
                        for (const auto& directory : project.library_search_directories) probe_roots.emplace_back(directory);
                        probe_roots.push_back(canonical.parent_path());
                        for (const auto& root : probe_roots)
                            if (fs::is_regular_file(root / (relative_sub.string() + ".py"), probe) ||
                                fs::is_regular_file(root / relative_sub / "__init__.py", probe)) { is_module = true; break; }
                        if (!is_module) {
                            const auto target = module(base, canonical.string(), project, item.pos, package_base);
                            if (!exports.at(target).contains(item.name))
                                fail(item.pos, "unknown imported name " + item.name + " in " + target);
                            if (!dependencies.emplace(key, exports.at(target).at(item.name)).second &&
                                dependencies.at(key).qualified != exports.at(target).at(item.name).qualified)
                                fail(item.pos, "duplicate imported name " + key);
                            for (const auto& [member, value] : exports.at(target))
                                if (member.starts_with(item.name + "."))
                                    dependencies.emplace(key + member.substr(item.name.size()), value);
                            if (class_names.contains(exports.at(target).at(item.name).qualified))
                                imported_classes.insert(exports.at(target).at(item.name).qualified);
                            continue;
                        }
                    }
                    const auto target = module(submodule, canonical.string(), project, item.pos, package_base);
                    inherit(target, key + ".", "", item.pos);
                }
                continue;
            }
            const auto internal_file = canonical.parent_path() / (from->module_name + ".py");
            std::ifstream internal_input(internal_file, std::ios::binary);
            if (!internal_input) fail(statement->pos, "cannot read " + internal_file.string());
            const std::string internal_text(std::istreambuf_iterator<char>{internal_input}, {});
            auto internal_syntax = UAST::parsePythonModule(internal_text,
                {internal_file.string(), 0, internal_text.size(), 1, 1});
            Bindings available;
            for (const auto& line : internal_syntax.body) {
                if (auto spec = native(*line)) {
                    auto assignment = line->get<py::Assign>();
                    if (assignment->targets.size() != 1 || !assignment->targets[0] ||
                        !assignment->targets[0]->get<py::Name>())
                        fail(line->pos, "native declaration needs one name");
                    available.emplace(assignment->targets[0]->get<py::Name>()->id,
                        Export{spec->first, spec->second});
                } else if (auto expr = line->get<py::ExprStmt>(); expr && expr->value &&
                    expr->value->get<py::Constant>() && expr->value->get<py::Constant>()->kind == py::ConstantKind::Str) {
                    continue;
                } else fail(line->pos, "internals.py may only contain native declarations");
            }
            for (const auto& item : from->names) {
                if (item.name == "*") {
                    for (const auto& [key, value] : available)
                        if (!dependencies.emplace(key, value).second) fail(item.pos, "duplicate imported name " + key);
                } else {
                    auto found = available.find(item.name);
                    if (found == available.end()) fail(item.pos, "unknown internal name " + item.name);
                    const auto key = item.asname.empty() ? item.name : item.asname;
                    if (!dependencies.emplace(key, found->second).second)
                        fail(item.pos, "duplicate imported name " + key);
                }
            }
        }
        Bindings symbols;
        for (const auto& statement : syntax.body) {
            if (statement->get<py::ImportFrom>() || statement->get<py::Import>()) continue;
            if (auto spec = native(*statement)) {
                if (canonical.filename() != "internals.py" && canonical.filename() != "langapi.py" && canonical.filename() != "prim.py")
                    fail(statement->pos, "native declarations belong in internals.py, langapi.py or prim.py");
                auto assignment = statement->get<py::Assign>();
                if (assignment->targets.size() != 1 || !assignment->targets[0] ||
                    !assignment->targets[0]->get<py::Name>())
                    fail(statement->pos, "native declaration needs one name");
                const auto key = assignment->targets[0]->get<py::Name>()->id;
                if (!symbols.emplace(key, Export{spec->first, spec->second}).second)
                    fail(statement->pos, "duplicate declaration " + key);
                continue;
            }
            std::string symbol;
            std::optional<std::size_t> arity;
            if (auto f = statement->get<py::FunctionDef>()) {
                symbol = f->name;
                arity = f->params.size();
                annotation(f->returns, statement->pos, true);
                for (const auto& p : f->params) annotation(p.annotation, p.pos);
            } else if (auto c = statement->get<py::ClassDef>()) {
                symbol = c->name;
                arity = 0;
                for (const auto& member : c->body) if (auto f = member->get<py::FunctionDef>()) {
                    if (f->name == "__init__") {
                        if (f->params.empty()) fail(member->pos, "__init__ requires self");
                        arity = f->params.size() - 1;
                    }
                    const bool is_static = f->decorators.size() == 1 && f->decorators[0]->get<py::Name>() &&
                        f->decorators[0]->get<py::Name>()->id == "staticmethod";
                    for (std::size_t i = is_static ? 0 : 1; i < f->params.size(); ++i)
                        annotation(f->params[i].annotation, f->params[i].pos);
                    if (f->name != "__init__") annotation(f->returns, member->pos, true);
                }
            } else if (auto a = statement->get<py::AnnAssign>(); a && a->target) {
                if (auto n = a->target->get<py::Name>()) symbol = n->id;
                annotation(a->annotation, statement->pos);
                if (!a->value) fail(statement->pos, "module variables require an initializer");
            } else if (auto a = statement->get<py::Assign>(); a && a->targets.size() == 1 && a->targets[0]) {
                if (auto n = a->targets[0]->get<py::Name>()) symbol = n->id;
            } else if (statement->get<py::Pass>()) continue;
            else if (auto e = statement->get<py::ExprStmt>(); e && e->value) {
                // Plain Python module docstrings do not generate declarations.
                if (auto c = e->value->get<py::Constant>(); c && c->kind == py::ConstantKind::Str) continue;
            }
            if (symbol.empty()) fail(statement->pos, "expected a function, class or initialized module variable");
            if (!symbols.emplace(symbol, Export{"python." + name + "." + symbol, arity}).second)
                fail(statement->pos, "duplicate declaration " + symbol);
        }
        std::set<std::string> classes = imported_classes;
        for (const auto& statement : syntax.body) if (auto c = statement->get<py::ClassDef>()) {
            classes.insert(c->name);
            classes.insert(symbols.at(c->name).qualified);
            class_names.insert(symbols.at(c->name).qualified);
            for (const auto& member : c->body) if (auto f = member->get<py::FunctionDef>(); f &&
                f->decorators.size() == 1 && f->decorators[0]->get<py::Name>() && f->decorators[0]->get<py::Name>()->id == "staticmethod") {
                const auto key = c->name + "." + f->name;
                symbols.emplace(key, Export{"python." + name + "." + key, f->params.size()});
            }
            // `NAME: ClassVar[T] = value` constants are addressed as Class.NAME.
            for (const auto& member : c->body) if (auto a = member->get<py::AnnAssign>(); a && a->target && a->annotation) {
                auto marker = a->annotation->get<py::Subscript>();
                auto marker_name = marker && marker->value ? marker->value->get<py::Name>() : nullptr;
                auto field = a->target->get<py::Name>();
                if (marker_name && marker_name->id == "ClassVar" && field) {
                    const auto key = c->name + "." + field->id;
                    symbols.emplace(key, Export{"python." + name + "." + key, std::nullopt});
                }
            }
        }
        LangAPI::Declarations lowered;
        for (auto& statement : syntax.body) {
            if (statement->get<py::ImportFrom>() || statement->get<py::Import>() || native(*statement)) continue;
            if (auto f = statement->get<py::FunctionDef>()) {
                Locals local;
                for (const auto& p : f->params) local.names.insert(p.name);
                walk(f->body, local);
                auto bindings = symbols;
                bindings.insert(dependencies.begin(), dependencies.end());
                Bind bind{bindings, std::move(local.names)};
                walk(f->body, bind);
                Bind type_bind{bindings};
                walk(f->returns, type_bind);
                for (auto& p : f->params) walk(p.annotation, type_bind);
            } else if (auto c = statement->get<py::ClassDef>()) {
                auto bindings = symbols;
                bindings.insert(dependencies.begin(), dependencies.end());
                Bind type_bind{bindings};
                walk(c->bases, type_bind);
                for (auto& member : c->body) {
                    if (auto f = member->get<py::FunctionDef>()) {
                        Locals local;
                        for (const auto& p : f->params) local.names.insert(p.name);
                        walk(f->body, local);
                        Bind bind{bindings, std::move(local.names)};
                        walk(f->body, bind);
                        walk(f->returns, type_bind);
                        for (auto& p : f->params) walk(p.annotation, type_bind);
                    } else walk(*member, type_bind);
                }
            } else if (auto a = statement->get<py::AnnAssign>()) {
                auto bindings = symbols;
                bindings.insert(dependencies.begin(), dependencies.end());
                Bind bind{bindings};
                walk(a->value, bind);
            } else if (auto a = statement->get<py::Assign>()) {
                auto bindings = symbols;
                bindings.insert(dependencies.begin(), dependencies.end());
                Bind bind{bindings};
                walk(a->value, bind);
            } else continue;
            auto declaration = Parser::ParserBase::convertPythonDeclaration(*statement, classes);
            if (declaration.isFunction() && declaration.getFunction().statements.empty()) {
                auto& function = declaration.getFunction();
                if (function.type != LangAPI::ValueType::Void)
                    fail(statement->pos, "a non-None function requires an executable body");
                function.statements.push_back(LangAPI::Return::createStatement(LangAPI::Return{}));
            }
            if (declaration.isVariable() && declaration.getVariable().type == LangAPI::ValueType::Undef) {
                const auto* assignment = statement->get<py::Assign>();
                const auto* literal = assignment && assignment->value ? assignment->value->get<py::Constant>() : nullptr;
                // A negated numeric literal (`NPOS = -1`) is still a literal.
                if (!literal && assignment && assignment->value)
                    if (auto negated = assignment->value->get<py::UnaryExpr>(); negated && negated->operand &&
                        (negated->op == py::UnaryOp::USub || negated->op == py::UnaryOp::UAdd))
                        literal = negated->operand->get<py::Constant>();
                if (!literal) fail(statement->pos, "nonliteral module variables require a type annotation");
                auto& type = declaration.getVariable().type;
                switch (literal->kind) {
                    case py::ConstantKind::Int: type = LangAPI::ValueType::Int; break;
                    case py::ConstantKind::Float: type = LangAPI::ValueType::Float; break;
                    case py::ConstantKind::Bool: type = LangAPI::ValueType::Bool; break;
                    case py::ConstantKind::Str: type = LangAPI::ValueType::String; break;
                    default: fail(statement->pos, "module variables require a supported type annotation");
                }
            }
            lowered.push_back(std::move(declaration));
        }
        for (auto it = source_path.rbegin(); it != source_path.rend(); ++it) {
            LangAPI::Namespace ns;
            ns.name = *it;
            ns.declarations = std::move(lowered);
            lowered.clear();
            lowered.emplace_back(std::move(ns));
        }
        declarations.insert(declarations.end(), lowered.begin(), lowered.end());
        paths.emplace(name, canonical);
        exports.emplace(name, std::move(symbols));
        loading.erase(name);
    }
    return name;
}
void Imports::load(const Modules::Source& source, const Modules::Import& imp,
                   const Modules::Project& project) {
    const auto at = UAST::SourcePosition{imp.position.file, imp.position.offset, 0, imp.position.line, imp.position.column};
    const auto name = module(imp.source, source.file, project, at, {});
    const auto local = imp.alias.empty() ? name : imp.alias;
    if (local == "ctx") fail(at, "ctx is reserved for the action context");
    const auto [alias, inserted] = aliases.emplace(std::make_pair(source.file, local), name);
    if (!inserted && alias->second != name) fail(at, "ambiguous import " + local);
    auto& bindings = files[source.file];
    for (const auto& [symbol, item] : exports.at(name)) {
        const auto key = local + "." + symbol;
        const auto [found, inserted] = bindings.emplace(key, item);
        if (!inserted && found->second.qualified != item.qualified) fail(at, "ambiguous import " + local);
    }
}
void Imports::bind(py::Expr& expression, const std::string& file) const {
    if (auto found = files.find(file); found != files.end()) {
        Bind bind{found->second, {}, true};
        walk(expression, bind);
    }
}
void Imports::bind(py::Module& module, const std::string& file) const {
    if (auto found = files.find(file); found != files.end()) {
        Bind bind{found->second, {}, true};
        walk(module, bind);
    }
}
}
