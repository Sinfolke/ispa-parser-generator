module Parser.Base;
import UAST.PyAST;
import std;

namespace Parser {
namespace {
namespace py = UAST::py;
namespace la = LangAPI;
// Dots occur only in synthetic names bound by Python.Imports. Keep namespace
// paths in the language-neutral symbol rather than embedding C++ spelling.
la::Symbol pythonSymbol(const std::string& name) {
    la::Symbol result;
    std::size_t begin = 0;
    do {
        const auto end = name.find('.', begin);
        result.path.emplace_back(name.substr(begin, end - begin));
        if (end == std::string::npos) break;
        begin = end + 1;
    } while (true);
    return result;
}
[[noreturn]] void unsupported(const UAST::SourcePosition& p, std::string_view what) {
    throw std::invalid_argument(p.file + ":" + std::to_string(p.line) + ":" +
        std::to_string(p.column) + ": Python conversion: " + std::string(what));
}
// Decode source syntax at the source-language boundary. LangAPI stores values.
std::string decodedString(std::string_view source, const UAST::SourcePosition& position) {
    std::string result;
    const auto unicode = [&](unsigned value) {
        if (value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff))
            unsupported(position, "string contains a non-scalar Unicode escape");
        if (value < 0x80) result += static_cast<char>(value);
        else if (value < 0x800) {
            result += static_cast<char>(0xc0 | (value >> 6));
            result += static_cast<char>(0x80 | (value & 0x3f));
        } else if (value < 0x10000) {
            result += static_cast<char>(0xe0 | (value >> 12));
            result += static_cast<char>(0x80 | ((value >> 6) & 0x3f));
            result += static_cast<char>(0x80 | (value & 0x3f));
        } else {
            result += static_cast<char>(0xf0 | (value >> 18));
            result += static_cast<char>(0x80 | ((value >> 12) & 0x3f));
            result += static_cast<char>(0x80 | ((value >> 6) & 0x3f));
            result += static_cast<char>(0x80 | (value & 0x3f));
        }
    };
    for (std::size_t i = 0; i < source.size(); ++i) {
        if (source[i] != '\\') { result += source[i]; continue; }
        if (++i == source.size()) unsupported(position, "dangling string escape");
        const char escaped = source[i];
        switch (escaped) {
            case '\\': case '\'': case '"': result += escaped; break;
            case 'n': result += '\n'; break;
            case 'r': result += '\r'; break;
            case 't': result += '\t'; break;
            case 'a': result += '\a'; break;
            case 'b': result += '\b'; break;
            case 'f': result += '\f'; break;
            case 'v': result += '\v'; break;
            case '\n': break;
            case 'x': case 'u': case 'U': {
                const std::size_t width = escaped == 'x' ? 2 : escaped == 'u' ? 4 : 8;
                unsigned value = 0;
                for (std::size_t j = 0; j < width; ++j) {
                    if (++i == source.size()) unsupported(position, "truncated string escape");
                    const unsigned char c = source[i];
                    if (!std::isxdigit(c)) unsupported(position, "invalid hexadecimal string escape");
                    value = (value << 4) | (std::isdigit(c) ? c - '0' : std::tolower(c) - 'a' + 10);
                }
                unicode(value);
                break;
            }
            case 'N': unsupported(position, "named Unicode escapes need source-language decoding");
            default:
                if (escaped >= '0' && escaped <= '7') {
                    unsigned value = escaped - '0';
                    for (int j = 0; j < 2 && i + 1 < source.size() && source[i + 1] >= '0' && source[i + 1] <= '7'; ++j)
                        value = value * 8 + source[++i] - '0';
                    unicode(value);
                } else { result += '\\'; result += escaped; }
        }
    }
    return result;
}
struct PythonConverter {
    std::set<std::string> locals;
    bool action_context = false, failure_context = false;
    std::string self_name;
    std::string token_rule;
    std::set<std::string> class_names, objects;
    // TypeVar names in scope (module-level `T = TypeVar("T")`) and the subset owned by the enclosing generic class.
    std::set<std::string> typevars, class_typevars;
    std::map<std::string, la::Type> local_types;
    // Declared signatures of the loaded modules, and the qualified class the current method belongs to.
    const py::Signatures* signatures = nullptr;
    std::string module_prefix, self_class;
    std::set<std::string> debug_ready;
    bool debug_ready_initialized = false;
    std::size_t debug_line_cursor = 0;
    std::function<la::Statements(const py::IspaMatch&, std::set<std::string>&)> embedded_match;
    std::string localName(const std::string& name) const {
        return action_context && locals.contains(name) ? "ispa_action_" + name : name;
    }
    std::string tokenName(const py::ExprPtr& value) const {
        if (!value) return {};
        if (auto name = value->template get<py::Name>())
            return locals.contains(name->id) || objects.contains(name->id) ||
                class_names.contains(name->id) || name->id == "ctx" ? "" : name->id;
        if (auto attribute = value->template get<py::Attribute>()) {
            auto prefix = tokenName(attribute->value);
            return prefix.empty() ? "" : prefix + "." + attribute->attr;
        }
        return {};
    }
    std::size_t block_depth = 0, loop_depth = 0;
    // Python locals are function-scoped; the targets' are block-scoped. A first assignment inside a block
    // is declared there when the block alone uses the name, otherwise it is hoisted to the function top.
    const py::StmtList* function_body = nullptr;
    std::vector<const py::StmtList*> block_lists;
    std::vector<std::vector<std::string>> block_declared;
    // Names first assigned in a block (declared there or hoisted): not definitely assigned after it.
    std::vector<std::vector<std::string>> block_assigned;
    std::vector<std::pair<std::string, la::Type>> hoisted;
    template<class T> static void count_name(const T& value, const std::string& name, std::size_t& total) {
        if constexpr (std::is_same_v<T, py::Expr>) {
            if (auto n = value.template get<py::Name>(); n && n->id == name) ++total;
            count_name(value.kind, name, total);
        } else if constexpr (requires { std::visit([](const auto&) {}, value); }) {
            std::visit([&](const auto& item) { count_name(item, name, total); }, value);
        } else if constexpr (requires { value.get(); *value; }) {
            if (value) count_name(*value, name, total);
        } else if constexpr (requires { value.members(); }) {
            std::apply([&](const auto&... items) { (count_name(items, name, total), ...); }, value.members());
        } else if constexpr (std::ranges::range<T> && !std::is_same_v<T, std::string>) {
            for (const auto& item : value) count_name(item, name, total);
        }
    }
    template<class T, class F> static void walk(const T& value, F& visit) {
        if constexpr (std::is_same_v<T, py::Expr> || std::is_same_v<T, py::Stmt>) {
            visit(value);
            walk(value.kind, visit);
        } else if constexpr (requires { std::visit([](const auto&) {}, value); }) {
            std::visit([&](const auto& item) { walk(item, visit); }, value);
        } else if constexpr (requires { value.get(); *value; }) {
            if (value) walk(*value, visit);
        } else if constexpr (requires { value.members(); }) {
            std::apply([&](const auto&... items) { (walk(items, visit), ...); }, value.members());
        } else if constexpr (std::ranges::range<T> && !std::is_same_v<T, std::string>) {
            for (const auto& item : value) walk(item, visit);
        }
    }
    // Whether `name` is assigned (or augmented) anywhere in `body`.
    static bool rebinds(const py::StmtList& body, const std::string& name) {
        bool found = false;
        auto visit = [&](const auto& node) {
            if constexpr (std::is_same_v<std::decay_t<decltype(node)>, py::Stmt>) {
                if (auto a = node.template get<py::AugAssign>(); a && a->target)
                    if (auto n = a->target->template get<py::Name>(); n && n->id == name) found = true;
            } else if (auto n = node.template get<py::Name>(); n && n->id == name && n->ctx != py::ExprContext::Load) found = true;
        };
        walk(body, visit);
        return found;
    }
    // How a list/dict parameter is passed. Python passes the object itself, so a callee that mutates it (or
    // keeps it) must see the caller's container: by reference. A parameter that is only read is const&.
    enum class ContainerPass { Value, ConstRef, Ref };
    static ContainerPass container_pass(const py::StmtList& body, const std::string& name) {
        if (rebinds(body, name)) return ContainerPass::Value;
        bool mutated = false;
        const auto is_name = [&](const py::ExprPtr& e) {
            auto n = e ? e->template get<py::Name>() : nullptr;
            return n && n->id == name;
        };
        auto visit = [&](const auto& node) {
            if constexpr (std::is_same_v<std::decay_t<decltype(node)>, py::Expr>) {
                // A direct mutation: a mutating method, or a store through a subscript. Passing the list on is
                // not one; a callee that mutates what it is handed makes the C++ compiler refuse the const&.
                if (auto c = node.template get<py::Call>(); c && c->func)
                    if (auto m = c->func->template get<py::Attribute>(); m && is_name(m->value)) {
                        static const std::set<std::string> writes{"append", "extend", "insert", "remove", "pop", "clear",
                            "sort", "reverse", "update", "setdefault", "popitem"};
                        if (writes.contains(m->attr)) mutated = true;
                    }
                if (auto s = node.template get<py::Subscript>(); s && is_name(s->value) && s->ctx != py::ExprContext::Load)
                    mutated = true;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(node)>, py::Stmt>) {
                if (auto a = node.template get<py::AugAssign>(); a && a->target)
                    if (auto sub = a->target->template get<py::Subscript>(); sub && is_name(sub->value)) mutated = true;
            }
        };
        walk(body, visit);
        return mutated ? ContainerPass::Ref : ContainerPass::ConstRef;
    }
    static std::size_t store_count(const py::StmtList& body, const std::string& name) {
        std::size_t count = 0;
        auto visit = [&](const auto& node) {
            if constexpr (std::is_same_v<std::decay_t<decltype(node)>, py::Stmt>) {
                if (auto a = node.template get<py::AugAssign>(); a && a->target)
                    if (auto n = a->target->template get<py::Name>(); n && n->id == name) ++count;
            } else if (auto n = node.template get<py::Name>(); n && n->id == name && n->ctx != py::ExprContext::Load) ++count;
        };
        walk(body, visit);
        return count;
    }

    // ---------------------------------------------------------------------------------------------------------
    // Escape analysis. A value "escapes" when it can outlive the call it was made in (returned, stored in a field
    // or container, captured by a lambda, passed to a parameter the callee retains, ...). Objects that never
    // escape live on the stack. Everything unknown is treated as escaping.
    py::Signatures* mutable_signatures = nullptr;
    std::string function_key;
    std::set<std::string> retained;
    std::map<std::string, std::set<std::string>> flows_into;  // alias target -> source names
    std::set<std::string> previous_retained;                  // from the analysis pass (for decisions)
    bool analysing = false;
    bool analysis_missing = true;     // no escape analysis for this function: nothing is proven
    bool construct_as_value = false;  // the next constructor call builds a stack object

    const py::Signatures::Summary* summary_of(const std::string& key) const {
        if (!signatures) return nullptr;
        const auto it = signatures->summaries.find(key);
        return it == signatures->summaries.end() ? nullptr : &it->second;
    }
    std::string method_key(const std::string& cls, const std::string& method, std::size_t depth = 0) const {
        if (!signatures || depth > 16) return {};
        if (signatures->summaries.contains(cls + "." + method)) return cls + "." + method;
        const auto found = signatures->classes.find(cls);
        if (found == signatures->classes.end()) return {};
        for (const auto& base : found->second.bases)
            if (auto key = method_key(base, method, depth + 1); !key.empty()) return key;
        return {};
    }
    void retain_name(const std::string& name) { retained.insert(name); }
    // The names whose objects the value of `e` can be (a name, either branch of `a if c else b`, `a or b`, cast).
    void flow_names(const py::ExprPtr& e, std::vector<std::string>& out) const {
        if (!e) return;
        if (auto n = e->template get<py::Name>()) out.push_back(n->id);
        else if (auto c = e->template get<py::IfExpr>()) { flow_names(c->body, out); flow_names(c->orelse, out); }
        else if (auto b = e->template get<py::BoolExpr>()) { for (const auto& v : b->values) flow_names(v, out); }
        else if (auto call = e->template get<py::Call>(); call && call->func)
            if (auto f = call->func->template get<py::Name>(); f && f->id == "cast" && call->args.size() == 2) flow_names(call->args[1], out);
    }
    void retain_flow(const py::ExprPtr& e) {
        std::vector<std::string> names;
        flow_names(e, names);
        for (const auto& n : names) retain_name(n);
    }
    void scan_args(const py::ExprList& args, const py::Signatures::Summary* summary, std::size_t first) {
        for (std::size_t i = 0; i < args.size(); ++i) {
            const bool keeps = !summary || first + i >= summary->retains.size() || summary->retains[first + i];
            if (keeps) retain_flow(args[i]);
            scan(args[i], true);  // the argument is itself an expression to look into
        }
    }
    void scan(const py::ExprPtr& e, bool safe) {
        if (!e) return;
        if (auto n = e->template get<py::Name>()) { if (!safe) retain_name(n->id); return; }
        if (e->template get<py::Constant>()) return;
        if (auto a = e->template get<py::Attribute>()) { scan(a->value, true); return; }
        if (auto s = e->template get<py::Subscript>()) { scan(s->value, true); scan(s->slice, true); return; }
        if (auto u = e->template get<py::UnaryExpr>()) { scan(u->operand, true); return; }
        if (auto b = e->template get<py::BinaryExpr>()) { scan(b->left, true); scan(b->right, true); return; }
        if (auto c = e->template get<py::Compare>()) {
            scan(c->left, true);
            for (const auto& item : c->comparators) scan(item, true);
            return;
        }
        if (auto b = e->template get<py::BoolExpr>()) { for (const auto& v : b->values) scan(v, safe); return; }
        if (auto c = e->template get<py::IfExpr>()) { scan(c->test, true); scan(c->body, safe); scan(c->orelse, safe); return; }
        if (auto l = e->template get<py::List>()) { for (const auto& item : l->elts) scan(item, false); return; }
        if (auto t = e->template get<py::Tuple>()) { for (const auto& item : t->elts) scan(item, false); return; }
        if (auto d = e->template get<py::Dict>()) {
            for (const auto& k : d->keys) scan(k, false);
            for (const auto& v : d->values) scan(v, false);
            return;
        }
        if (auto lambda = e->template get<py::Lambda>()) {
            // Whatever the closure mentions may be used after this call returns.
            auto visit = [&](const auto& node) {
                if constexpr (std::is_same_v<std::decay_t<decltype(node)>, py::Expr>)
                    if (auto n = node.template get<py::Name>()) retain_name(n->id);
            };
            walk(lambda->body, visit);
            return;
        }
        if (auto call = e->template get<py::Call>()) { scan_call(*call); return; }
        // Anything else: every name inside is treated as escaping.
        auto visit = [&](const auto& node) {
            if constexpr (std::is_same_v<std::decay_t<decltype(node)>, py::Expr>)
                if (auto n = node.template get<py::Name>()) retain_name(n->id);
        };
        walk(*e, visit);
    }
    void scan_call(const py::Call& call) {
        for (const auto& keyword : call.keywords) { retain_flow(keyword.value); scan(keyword.value, true); }
        if (!call.func) return;
        if (auto f = call.func->template get<py::Name>()) {
            const auto& id = f->id;
            if (!locals.contains(id)) {
                if (id == "len" || id == "int" || id == "str" || id == "float" || id == "bool") { scan_args(call.args, nullptr, 0); for (const auto& a : call.args) (void)a; return; }
                if (id == "cast") { for (const auto& a : call.args) { retain_flow(a); scan(a, true); } return; }
                if (id.starts_with("__ispa_ir__.")) {
                    for (const auto& a : call.args) { retain_flow(a); scan(a, true); }
                    return;
                }
                if (is_class(id)) { scan_args(call.args, summary_of(method_key(id, "__init__")), 1); return; }
                if (const auto* s = summary_of(id)) { scan_args(call.args, s, 0); return; }
            }
            scan_args(call.args, nullptr, 0);  // a callable value: unknown callee
            return;
        }
        if (auto m = call.func->template get<py::Attribute>(); m && m->value) {
            // Class.static(...)
            if (auto owner = m->value->template get<py::Name>(); owner && !locals.contains(owner->id) && is_class(owner->id)) {
                const auto key = method_key(owner->id, m->attr);
                const auto* s = key.empty() ? nullptr : summary_of(key);
                scan_args(call.args, s, 0);
                return;
            }
            static const std::set<std::string> collection_methods{"append", "extend", "insert", "remove", "pop", "clear",
                "copy", "count", "index", "reverse", "sort", "get", "setdefault", "update", "keys", "values", "items", "popitem"};
            const auto receiver_class = symbol_id(inferred(m->value));
            std::string key = receiver_class.empty() ? std::string() : method_key(receiver_class, m->attr);
            if (key.empty() && collection_methods.contains(m->attr) && receiver_class.empty()) {
                // list/dict/str method: only the inserted elements can be kept
                const bool stores = m->attr == "append" || m->attr == "insert" || m->attr == "setdefault" || m->attr == "get";
                for (const auto& a : call.args) { if (stores) retain_flow(a); scan(a, true); }
                scan(m->value, true);
                return;
            }
            const auto* s = key.empty() ? nullptr : summary_of(key);
            // The receiver is retained when the method can keep `self` (or when it is not known).
            if (!s || s->retains.empty() || s->retains[0]) retain_flow(m->value);
            scan(m->value, true);
            scan_args(call.args, s, 1);
            return;
        }
        // calling the result of an expression
        scan(call.func, false);
        scan_args(call.args, nullptr, 0);
    }
    void scan_stmts(const py::StmtList& list) {
        for (const auto& statement : list) if (statement) scan_stmt(*statement);
    }
    void scan_stmt(const py::Stmt& s) {
        std::visit([&](const auto& x) {
            using T = std::decay_t<decltype(x)>;
            if constexpr (std::is_same_v<T, py::ExprStmt>) scan(x.value, true);
            else if constexpr (std::is_same_v<T, py::Return>) { retain_flow(x.value); scan(x.value, true); }
            else if constexpr (std::is_same_v<T, py::Assign>) {
                for (const auto& target : x.targets) {
                    if (auto n = target ? target->template get<py::Name>() : nullptr) {
                        std::vector<std::string> sources;
                        flow_names(x.value, sources);
                        for (const auto& source : sources) flows_into[n->id].insert(source);
                        scan(x.value, true);
                        if (!local_types.contains(n->id)) { locals.insert(n->id); local_types.insert_or_assign(n->id, inferred(x.value)); }
                    } else { retain_flow(x.value); scan(x.value, true); scan(target, true); }
                }
            } else if constexpr (std::is_same_v<T, py::AnnAssign>) {
                if (auto n = x.target ? x.target->template get<py::Name>() : nullptr) {
                    std::vector<std::string> sources;
                    flow_names(x.value, sources);
                    for (const auto& source : sources) flows_into[n->id].insert(source);
                    scan(x.value, true);
                    locals.insert(n->id);
                    local_types.insert_or_assign(n->id, declared(x.annotation));
                } else { retain_flow(x.value); scan(x.value, true); scan(x.target, true); }
            } else if constexpr (std::is_same_v<T, py::AugAssign>) { scan(x.value, true); scan(x.target, true); }
            else if constexpr (std::is_same_v<T, py::If>) { scan(x.test, true); scan_stmts(x.body); scan_stmts(x.orelse); }
            else if constexpr (std::is_same_v<T, py::While>) { scan(x.test, true); scan_stmts(x.body); scan_stmts(x.orelse); }
            else if constexpr (std::is_same_v<T, py::Pass> || std::is_same_v<T, py::Break> || std::is_same_v<T, py::Continue>) {}
            else {
                // statements outside the lowered subset: everything they mention escapes
                auto visit = [&](const auto& node) {
                    if constexpr (std::is_same_v<std::decay_t<decltype(node)>, py::Expr>)
                        if (auto n = node.template get<py::Name>()) retain_name(n->id);
                };
                walk(x, visit);
            }
        }, s.kind);
    }
    // Analyse one function body: record which parameters and locals can outlive the call.
    void analyse_function(const py::FunctionDef& f, const std::string& key, bool is_method, bool is_static) {
        retained.clear();
        flows_into.clear();
        analysing = true;
        function_key = key;
        std::vector<std::string> parameters;
        for (const auto& p : f.params) {
            parameters.push_back(p.name);
            locals.insert(p.name);
            local_types.insert_or_assign(p.name, p.annotation ? declared(p.annotation) : la::Type{la::ValueType::Undef});
        }
        if (is_method && !is_static && !parameters.empty()) {
            self_name = parameters.front();
            local_types.insert_or_assign(self_name, boxed(la::Type{pythonSymbol(self_class)}));
        }
        scan_stmts(f.body);
        // aliases: x = y means y is retained when x is
        for (bool changed = true; changed;) {
            changed = false;
            for (const auto& [target, sources] : flows_into)
                if (retained.contains(target))
                    for (const auto& source : sources) changed |= retained.insert(source).second;
        }
        if (mutable_signatures) {
            py::Signatures::Summary summary;
            for (const auto& name : parameters) summary.retains.push_back(retained.contains(name));
            mutable_signatures->summaries[key] = std::move(summary);
            // Not eligible for the stack: escaping values, and anything another name is bound to (an alias would
            // have to be a different kind of variable).
            auto blocked = retained;
            for (const auto& [target, sources] : flows_into) blocked.insert(sources.begin(), sources.end());
            mutable_signatures->retained_locals[key] = std::move(blocked);
        }
    }
    // Entry point used by the module loader: analyse every function and method of a statement.
    void analyse_statement(const py::Stmt& s) {
        if (auto f = s.template get<py::FunctionDef>()) {
            PythonConverter scope;
            scope.class_names = class_names; scope.typevars = typevars; scope.signatures = signatures;
            scope.mutable_signatures = mutable_signatures; scope.module_prefix = module_prefix;
            scope.analyse_function(*f, module_prefix + f->name, false, false);
        } else if (auto c = s.template get<py::ClassDef>()) {
            for (const auto& member : c->body) if (member)
                if (auto m = member->template get<py::FunctionDef>()) {
                    PythonConverter scope;
                    scope.class_names = class_names; scope.typevars = typevars; scope.signatures = signatures;
                    scope.mutable_signatures = mutable_signatures; scope.module_prefix = module_prefix;
                    scope.self_class = module_prefix + c->name;
                    bool is_static = m->decorators.size() == 1 && m->decorators[0] && m->decorators[0]->template get<py::Name>() &&
                                     m->decorators[0]->template get<py::Name>()->id == "staticmethod";
                    scope.analyse_function(*m, scope.self_class + "." + m->name, true, is_static);
                }
        }
    }
    la::Type return_type{la::ValueType::Undef};
    // `x = []` carries no element type. Python code states it later: `return x` (the declared return type)
    // or `x.append(value)` (the value's type).
    la::Type usage_type(const std::string& name) {
        if (!function_body) return la::Type{la::ValueType::Undef};
        la::Type found{la::ValueType::Undef};
        const auto undefined = [](const la::Type& t) { return t.isValueType() && t.getValueType() == la::ValueType::Undef; };
        auto visit = [&](const auto& node) {
            if (!undefined(found)) return;
            if constexpr (std::is_same_v<std::decay_t<decltype(node)>, py::Stmt>) {
                if (auto r = node.template get<py::Return>(); r && r->value)
                    if (auto n = r->value->template get<py::Name>(); n && n->id == name &&
                        return_type.isValueType() && return_type.getValueType() == la::ValueType::Array) found = return_type;
            } else if (auto c = node.template get<py::Call>(); c && c->func && c->args.size() == 1)
                if (auto m = c->func->template get<py::Attribute>(); m && m->attr == "append" && m->value)
                    if (auto n = m->value->template get<py::Name>(); n && n->id == name) {
                        const auto element = inferred(c->args.front());
                        if (!undefined(element)) found = la::Type{la::ValueType::Array, element};
                    }
        };
        walk(*function_body, visit);
        return found;
    }
    // Occurrences of `name` in the statements of `list` itself, not in the blocks they contain.
    static void count_shallow(const py::StmtList& list, const std::string& name, std::size_t& total) {
        for (const auto& statement : list) {
            if (!statement) continue;
            if (auto s = statement->template get<py::If>()) count_name(s->test, name, total);
            else if (auto s = statement->template get<py::While>()) count_name(s->test, name, total);
            else if (auto s = statement->template get<py::For>()) { count_name(s->target, name, total); count_name(s->iter, name, total); }
            else if (auto s = statement->template get<py::With>()) count_name(s->items, name, total);
            else if (statement->template get<py::Try>()) continue;
            else count_name(*statement, name, total);
        }
    }
    // A name escapes its block when a statement of an enclosing block (outside any sibling block) uses it.
    // Sibling blocks that assign the same name are independent variables, as they are in the targets.
    bool escapes_block(const std::string& name) const {
        if (!function_body || block_lists.empty()) return false;
        std::size_t outside = 0;
        count_shallow(*function_body, name, outside);
        for (std::size_t i = 0; i + 1 < block_lists.size(); ++i) count_shallow(*block_lists[i], name, outside);
        return outside > 0;
    }
    la::Statements nested(const py::StmtList& list, bool loop = false) {
        ++block_depth;
        loop_depth += loop;
        block_lists.push_back(&list);
        block_declared.emplace_back();
        block_assigned.emplace_back();
        auto out = body(list);
        for (const auto& name : block_assigned.back()) debug_ready.erase(name);
        block_assigned.pop_back();
        for (const auto& name : block_declared.back()) {
            locals.erase(name);
            local_types.erase(name);
            objects.erase(name);
        }
        block_declared.pop_back();
        block_lists.pop_back();
        loop_depth -= loop;
        --block_depth;
        return out;
    }
    la::Expression expr(const py::ExprPtr& p) {
        if (!p) throw std::invalid_argument("Python conversion: missing expression");
        return expr(*p);
    }
    // A stack object is handed to a callee as a non-owning pointer: the analysis proved the callee does not keep it.
    la::Expression argument(const py::ExprPtr& a) {
        auto value = expr(a);
        if (auto n = a ? a->template get<py::Name>() : nullptr)
            if (const auto found = local_types.find(n->id); found != local_types.end() && found->second.isSymbol() &&
                !found->second.getSymbol().path.empty() && is_class(symbol_id(found->second))) {
                la::FunctionCall borrow;
                borrow.name = std::make_shared<la::Symbol>(pythonSymbol("ISPA_STD.borrow"));
                borrow.args.push_back(std::move(value));
                return la::FunctionCall::createExpression(borrow);
            }
        return value;
    }
    la::Expression group(la::Expression e) {
        e.insert(e.begin(), la::ExpressionElement::GroupOpen);
        e.emplace_back(la::ExpressionElement::GroupClose);
        return e;
    }
    la::Expression binary(la::Expression left, la::ExpressionElement op, la::Expression right) {
        auto out = group(std::move(left));
        out.emplace_back(op);
        auto rhs = group(std::move(right));
        out.insert(out.end(), rhs.begin(), rhs.end());
        return group(std::move(out));
    }
    la::ExpressionElement binop(py::BinOp op, const UAST::SourcePosition& p) {
        switch (op) {
            case py::BinOp::Add: return la::ExpressionElement::Add;
            case py::BinOp::Sub: return la::ExpressionElement::Minus;
            case py::BinOp::Mult: return la::ExpressionElement::Multiply;
            // `//` lowers to the typed Divide: exact for the non-negative ints the runtime uses.
            case py::BinOp::Div: case py::BinOp::FloorDiv: return la::ExpressionElement::Divide;
            case py::BinOp::Mod: return la::ExpressionElement::Modulo;
            default: unsupported(p, "operator has no LangAPI representation");
        }
    }
    // Class instances have reference semantics: values of a class type are boxes (shared pointers).
    static la::Type boxed(la::Type inner) { return la::Type{la::ValueType::Box, std::move(inner)}; }
    bool is_class(const std::string& id) const { return class_names.contains(id) || (signatures && signatures->classes.contains(id)); }
    static bool is_box(const la::Type& t) { return t.isValueType() && t.getValueType() == la::ValueType::Box; }
    la::Expression receiver(const py::ExprPtr& base) {
        if (auto n = base ? base->template get<py::Name>() : nullptr; n && !self_name.empty() && n->id == self_name)
            return la::Symbol::createExpression(pythonSymbol("__ispa_python_self"));
        auto value = expr(base);
        if (is_box(inferred(base))) return la::GetFromBox::createExpression(la::GetFromBox{.what = std::move(value)});
        return value;
    }
    la::Type type(const py::ExprPtr& p) {
        if (!p) return la::Type{la::ValueType::Undef};
        if (auto n = p->template get<py::Name>()) {
            if (n->id == "int") return la::Type{la::ValueType::Int};
            if (n->id == "float") return la::Type{la::ValueType::Float};
            if (n->id == "bool") return la::Type{la::ValueType::Bool};
            if (n->id == "str") return la::Type{la::ValueType::String};
            if (n->id == "object") return la::Type{la::ValueType::Any};
            if (typevars.contains(n->id)) return la::Type{la::Symbol{n->id}};
            if (is_class(n->id)) return boxed(la::Type{pythonSymbol(n->id)});
            return la::Type{pythonSymbol(n->id)};
        }
        if (auto sub = p->template get<py::Subscript>(); sub && sub->value) {
            auto name = sub->value->template get<py::Name>();
            if (name && name->id == "list") return la::Type{la::ValueType::Array, type(sub->slice)};
            if (name && name->id == "dict") {
                auto args = sub->slice ? sub->slice->template get<py::Tuple>() : nullptr;
                if (!args || args->elts.size() != 2) unsupported(p->pos, "dict requires key and value types");
                return la::Type{la::ValueType::Map, type(args->elts[0]), type(args->elts[1])};
            }
            if (name && (name->id == "Callable" || name->id == "typing.Callable")) {
                auto args = sub->slice ? sub->slice->template get<py::Tuple>() : nullptr;
                auto params = args && args->elts.size() == 2 && args->elts[0] ? args->elts[0]->template get<py::List>() : nullptr;
                if (!params) unsupported(p->pos, "Callable requires [[parameter types], return type]");
                la::Type result{la::ValueType::Callable};
                for (const auto& parameter : params->elts) result.template_parameters.emplace_back(type(parameter));
                result.template_parameters.emplace_back(type(args->elts[1]));
                return result;
            }
            if (name) {
                // A generic class used with type arguments: Pair[int, RawToken].
                la::Type result{pythonSymbol(name->id)};
                if (auto args = sub->slice ? sub->slice->template get<py::Tuple>() : nullptr) {
                    for (const auto& arg : args->elts) result.template_parameters.emplace_back(type(arg));
                } else result.template_parameters.emplace_back(type(sub->slice));
                return is_class(name->id) ? boxed(result) : result;
            }
        }
        if (auto c = p->template get<py::Constant>(); c && c->kind == py::ConstantKind::None)
            return la::Type{la::ValueType::Void};
        unsupported(p->pos, "unsupported type annotation");
    }
    bool object(const py::ExprPtr& value) const {
        if (!value) return false;
        if (auto n = value->template get<py::Name>()) return objects.contains(n->id) || class_names.contains(n->id) || (action_context && n->id == "ctx");
        if (auto c = value->template get<py::Call>(); c && c->func)
            if (auto n = c->func->template get<py::Name>()) return class_names.contains(n->id);
        return false;
    }
    static std::string symbol_id(const la::Type& t) {
        if (is_box(t) && !t.template_parameters.empty())
            if (const auto* inner = std::get_if<la::Type>(&t.template_parameters.front())) return symbol_id(*inner);
        if (!t.isSymbol()) return {};
        std::string id;
        for (const auto& part : t.getSymbol().path) {
            const auto* text = std::get_if<std::string>(&part);
            if (!text) return {};
            id += (id.empty() ? "" : ".") + *text;
        }
        return id;
    }
    // A field's (method == false) or method's return annotation, searching base classes.
    py::ExprPtr member_annotation(const std::string& cls, const std::string& member, bool method, std::size_t depth = 0) const {
        if (!signatures || depth > 16) return nullptr;
        const auto found = signatures->classes.find(cls);
        if (found == signatures->classes.end()) return nullptr;
        const auto& table = method ? found->second.methods : found->second.fields;
        if (const auto it = table.find(member); it != table.end()) return it->second;
        for (const auto& base : found->second.bases)
            if (auto inherited = member_annotation(base, member, method, depth + 1)) return inherited;
        return nullptr;
    }
    la::Type declared(const py::ExprPtr& annotation) {
        if (!annotation) return la::Type{la::ValueType::Undef};
        try { return type(annotation); } catch (const std::invalid_argument&) { return la::Type{la::ValueType::Undef}; }
    }
    la::Type inferred_call(const py::Call& call) {
        const la::Type unknown{la::ValueType::Undef};
        if (!call.func) return unknown;
        if (auto n = call.func->template get<py::Name>()) {
            if (auto local = local_types.find(n->id); local != local_types.end()) {
                // A Callable local returns the last template parameter.
                if (local->second.isValueType() && local->second.getValueType() == la::ValueType::Callable &&
                    !local->second.template_parameters.empty())
                    if (const auto* result = std::get_if<la::Type>(&local->second.template_parameters.back())) return *result;
                return unknown;
            }
            if (locals.contains(n->id)) return unknown;
            if (n->id == "len" || n->id == "int") return la::Type{la::ValueType::Int};
            if (n->id == "str") return la::Type{la::ValueType::String};
            if (n->id == "float") return la::Type{la::ValueType::Float};
            if (n->id == "bool") return la::Type{la::ValueType::Bool};
            if (signatures) {
                if (const auto it = signatures->functions.find(n->id); it != signatures->functions.end()) return declared(it->second);
                if (signatures->classes.contains(n->id)) return boxed(la::Type{pythonSymbol(n->id)});
            }
            return unknown;
        }
        if (auto attribute = call.func->template get<py::Attribute>(); attribute && attribute->value) {
            const auto owner = symbol_id(inferred(attribute->value));
            if (!owner.empty()) return declared(member_annotation(owner, attribute->attr, true));
        }
        return unknown;
    }
    la::Type inferred(const py::ExprPtr& value) {
        if (!value) return la::Type{la::ValueType::Undef};
        if (auto n = value->template get<py::Name>()) {
            if (auto found = local_types.find(n->id); found != local_types.end()) return found->second;
            if (!self_name.empty() && n->id == self_name && !self_class.empty()) return boxed(la::Type{pythonSymbol(self_class)});
        }
        if (auto n = value->template get<py::Name>(); n && signatures && !locals.contains(n->id)) {
            if (const auto it = signatures->variables.find(n->id); it != signatures->variables.end()) return declared(it->second);
        }
        if (auto a = value->template get<py::Attribute>(); a && a->value) {
            const auto owner = symbol_id(inferred(a->value));
            if (!owner.empty()) return declared(member_annotation(owner, a->attr, false));
        }
        if (auto c = value->template get<py::Call>()) return inferred_call(*c);
        if (auto c = value->template get<py::Constant>()) {
            switch (c->kind) {
                case py::ConstantKind::Int: return la::Type{la::ValueType::Int};
                case py::ConstantKind::Float: return la::Type{la::ValueType::Float};
                case py::ConstantKind::Bool: return la::Type{la::ValueType::Bool};
                case py::ConstantKind::Str: return la::Type{la::ValueType::String};
                default: break;
            }
        }
        const auto is = [](const la::Type& t, la::ValueType v) { return t.isValueType() && t.getValueType() == v; };
        if (auto b = value->template get<py::BinaryExpr>()) {
            const auto l = inferred(b->left), r = inferred(b->right);
            if (is(l, la::ValueType::Int) && is(r, la::ValueType::Int)) return l;
            if ((is(l, la::ValueType::Float) || is(l, la::ValueType::Int)) &&
                (is(r, la::ValueType::Float) || is(r, la::ValueType::Int))) return la::Type{la::ValueType::Float};
            if (b->op == py::BinOp::Add && l == r && (is(l, la::ValueType::String) || is(l, la::ValueType::Array))) return l;
            return la::Type{la::ValueType::Undef};
        }
        if (auto u = value->template get<py::UnaryExpr>())
            return u->op == py::UnaryOp::Not ? la::Type{la::ValueType::Bool} : inferred(u->operand);
        if (value->template get<py::Compare>()) return la::Type{la::ValueType::Bool};
        if (auto b = value->template get<py::BoolExpr>()) {
            for (const auto& item : b->values) if (!is(inferred(item), la::ValueType::Bool)) return la::Type{la::ValueType::Undef};
            return la::Type{la::ValueType::Bool};
        }
        if (auto c = value->template get<py::IfExpr>()) {
            const auto l = inferred(c->body);
            return l == inferred(c->orelse) ? l : la::Type{la::ValueType::Undef};
        }
        if (auto sub = value->template get<py::Subscript>()) {
            const auto base = inferred(sub->value);
            if (is(base, la::ValueType::String)) return base;
            if (is(base, la::ValueType::Array) && !base.template_parameters.empty())
                if (const auto* element = std::get_if<la::Type>(&base.template_parameters.front())) return *element;
        }
        if (auto list = value->template get<py::List>(); list && !list->elts.empty())
            return la::Type{la::ValueType::Array, inferred(list->elts.front())};
        if (auto dict = value->template get<py::Dict>(); dict && !dict->keys.empty())
            return la::Type{la::ValueType::Map, inferred(dict->keys.front()), inferred(dict->values.front())};
        return la::Type{la::ValueType::Undef};
    }
    la::Expression initializer(const py::ExprPtr& value, const la::Type& expected) {
        auto result = expr(value);
        if (result.size() == 1 && result.front().isRvalue()) {
            auto& literal = result.front().getRValue();
            if (literal.isArray() || literal.isMap()) {
                decltype(la::Array::template_parameters) params;
                for (const auto& item : expected.template_parameters) if (auto t = std::get_if<la::Type>(&item))
                    params.push_back(std::make_shared<la::Type>(*t));
                if (literal.isArray()) literal.getArray().template_parameters = params;
                else literal.getMap().template_parameters = params;
            }
        }
        return result;
    }
    std::variant<la::Symbol, la::StorageSymbol> target(const py::ExprPtr& p) {
        if (action_context) {
            const py::Expr* root = p.get();
            const py::Attribute* property = nullptr;
            while (root) {
                if (auto a = root->template get<py::Attribute>()) { property = a; root = a->value.get(); }
                else if (auto s = root->template get<py::Subscript>()) root = s->value.get();
                else break;
            }
            if (root) if (auto n = root->template get<py::Name>(); n && n->id == "ctx") {
                if (!property || (token_rule.empty() ? property->attr != "pos" && property->attr != "ast" :
                    property->attr != "ast" && property->attr != "shared"))
                    unsupported(p->pos, token_rule.empty() ?
                        "ctx and its metadata are read-only; only ctx.pos and ctx.ast are writable" :
                        "token ctx metadata are read-only; only ctx.ast and ctx.shared fields are writable");
            }
        }
        auto e = expr(p);
        if (e.size() == 1 && e.front().isRvalue()) {
            auto& r = e.front().getRValue();
            if (r.isSymbol()) return r.getSymbol();
            if (r.isStorageSymbol()) return r.getStorageSymbol();
        }
        unsupported(p->pos, "assignment target must be a name, attribute or subscript");
    }
    la::Expression expr(const py::Expr& e) {
        return std::visit([&](const auto& x) -> la::Expression {
            using T = std::decay_t<decltype(x)>;
            if constexpr (std::is_same_v<T, py::Name>) {
                if (!token_rule.empty() && !failure_context && x.id == "ctx") unsupported(e.pos, "token context needs a property");
                return la::Symbol::createExpression(pythonSymbol(!self_name.empty() && x.id == self_name ? "__ispa_python_self_box" : localName(x.id)));
            }
            else if constexpr (std::is_same_v<T, py::Constant>) {
                switch (x.kind) {
                    case py::ConstantKind::Bool: return la::Bool::createExpression(la::Bool{.value = x.value == "True"});
                    case py::ConstantKind::Int: {
                        auto text = x.value;
                        std::erase(text, '_');
                        std::size_t end;
                        int base = 10;
                        if (text.size() > 2 && text[0] == '0') {
                            switch (text[1]) {
                                case 'x': case 'X': base = 16; break;
                                case 'o': case 'O': base = 8; break;
                                case 'b': case 'B': base = 2; break;
                            }
                            if (base != 10) text.erase(0, 2);
                        }
                        long long value;
                        try { value = std::stoll(text, &end, base); }
                        catch (const std::exception&) { unsupported(e.pos, "integer literal is invalid or exceeds LangAPI range"); }
                        if (end != text.size()) unsupported(e.pos, "invalid integer literal");
                        return la::Int::createExpression(la::Int{.value = value});
                    }
                    case py::ConstantKind::Float: {
                        auto text = x.value;
                        std::erase(text, '_');
                        std::size_t end;
                        double value;
                        try { value = std::stod(text, &end); }
                        catch (const std::exception&) { unsupported(e.pos, "float literal is invalid or exceeds LangAPI range"); }
                        if (end != text.size()) unsupported(e.pos, "invalid float literal");
                        return la::Float::createExpression(la::Float{.value = value});
                    }
                    case py::ConstantKind::Str: {
                        return la::String::createExpression(la::String{.value = decodedString(x.value, e.pos)});
                    }
                    default: unsupported(e.pos, "None has no LangAPI value representation");
                }
            } else if constexpr (std::is_same_v<T, py::Attribute> || std::is_same_v<T, py::Subscript>) {
                if constexpr (std::is_same_v<T, py::Attribute>) {
                    if (!token_rule.empty() && !failure_context && x.value)
                        if (auto shared = x.value->template get<py::Attribute>(); shared && shared->attr == "shared" && shared->value)
                            if (auto ctx = shared->value->template get<py::Name>(); ctx && ctx->id == "ctx")
                                return la::Symbol::createExpression(la::Symbol{"ispa_shared_" + x.attr});
                    if (!token_rule.empty() && !failure_context && x.value) if (auto n = x.value->template get<py::Name>(); n && n->id == "ctx") {
                        if (x.attr == "ast") return la::Symbol::createExpression(la::Symbol{"ispa_token_result"});
                        if (x.attr == "start_pos") return la::Symbol::createExpression(la::Symbol{"start_pos"});
                        if (x.attr == "rule") return la::String::createExpression(la::String{.value = token_rule});
                        if (x.attr == "failed_rule") return la::String::createExpression(la::String{.value = token_rule});
                        if (x.attr == "text") return la::Symbol::createExpression(la::Symbol{"ispa_token_text"});
                        if (x.attr == "line_prefix") return la::FunctionCall::createExpression(la::FunctionCall{
                            .name = std::make_shared<la::Symbol>(la::Symbol{"__line_prefix"}),
                            .args = {la::Symbol::createExpression(la::Symbol{"start"})}});
                        if (x.attr == "next_char") return la::FunctionCall::createExpression(la::FunctionCall{
                            .name = std::make_shared<la::Symbol>(la::Symbol{"__next_char"}),
                            .args = {binary(la::Symbol::createExpression(la::Symbol{"start"}),
                                la::ExpressionElement::Add, la::Symbol::createExpression(la::Symbol{"length"}))}});
                        unsupported(e.pos, "unknown token ctx property: " + x.attr);
                    }
                    if (action_context && x.value) if (auto n = x.value->template get<py::Name>(); n && n->id == "ctx") {
                        if (x.attr != "pos" && x.attr != "ast" && x.attr != "start_pos" && x.attr != "rule" &&
                            !(failure_context && x.attr == "failed_rule"))
                            unsupported(e.pos, "unknown ctx property: " + x.attr);
                    }
                }
                la::StorageSymbol s;
                s.what = group(receiver(x.value));
                if constexpr (std::is_same_v<T, py::Attribute>) s.path.emplace_back(x.attr);
                else {
                    s.path.emplace_back(la::StorageOffset{.offset = expr(x.slice)});
                    // Indexing a str gives a one-character str, not the target's character type.
                    if (const auto base = inferred(x.value); base.isValueType() && base.getValueType() == la::ValueType::String)
                        return la::CharToStringConstructor::createExpression(la::CharToStringConstructor{.what = la::StorageSymbol::createExpression(s)});
                }
                return la::StorageSymbol::createExpression(s);
            } else if constexpr (std::is_same_v<T, py::Call>) {
                const bool as_value = std::exchange(construct_as_value, false);
                if (const auto* builtin = x.func ? x.func->template get<py::Name>() : nullptr;
                    builtin && builtin->id == "len" && !locals.contains("len") && x.args.size() == 1 && x.keywords.empty()) {
                    la::StorageSymbol access;
                    access.what = expr(x.args[0]);
                    access.path.emplace_back(la::ArrayMethodCall{.method = la::ArrayMethods::Size});
                    return la::StorageSymbol::createExpression(access);
                }
                if (const auto* builtin = x.func ? x.func->template get<py::Name>() : nullptr;
                    builtin && builtin->id == "cast" && !locals.contains("cast") && x.args.size() == 2 && x.keywords.empty()) {
                    // typing.cast(T, value): an object recovered as its dynamic type.
                    return la::Cast::createExpression(la::Cast{.type = std::make_shared<la::Type>(type(x.args[0])), .what = expr(x.args[1])});
                }
                if (const auto* intrinsic = x.func ? x.func->template get<py::Name>() : nullptr;
                    intrinsic && intrinsic->id.starts_with("__ispa_ir__.")) {
                    const auto name = intrinsic->id.substr(12);
                    if (!x.keywords.empty()) unsupported(e.pos, "IR operations require positional arguments");
                    const std::map<std::string, std::pair<la::ArrayMethods, std::size_t>> collections{
                        {"size", {la::ArrayMethods::Size, 1}}, {"push", {la::ArrayMethods::Push, 2}},
                        {"extend", {la::ArrayMethods::Extend, 2}}, {"clear", {la::ArrayMethods::Clear, 1}}};
                    if (const auto found = collections.find(name); found != collections.end()) {
                        if (x.args.size() != found->second.second) unsupported(e.pos, "wrong IR operation arity");
                        la::StorageSymbol access;
                        access.what = expr(x.args[0]);
                        la::ArrayMethodCall call{.method = found->second.first};
                        for (std::size_t i = 1; i < x.args.size(); ++i) call.args.push_back(expr(x.args[i]));
                        access.path.emplace_back(std::move(call));
                        return la::StorageSymbol::createExpression(access);
                    }
                    const std::map<std::string, std::pair<la::StdlibExports, std::size_t>> streams{
                        {"token_cursor", {la::StdlibExports::TokenCursor, 1}},
                        {"lazy_token_cursor", {la::StdlibExports::LazyTokenCursor, 2}},
                        {"reset_token_stream", {la::StdlibExports::ResetTokenStream, 1}},
                        {"fail", {la::StdlibExports::Fail, 2}}, {"attempt", {la::StdlibExports::Attempt, 1}},
                        {"failure_message", {la::StdlibExports::FailureMessage, 0}}};
                    if (const auto found = streams.find(name); found != streams.end()) {
                        if (x.args.size() != found->second.second) unsupported(e.pos, "wrong IR operation arity");
                        la::IspaLibFunctionCall call{.symbol = la::IspaLibSymbol{.exports = found->second.first}};
                        for (const auto& argument : x.args) call.args.push_back(expr(argument));
                        return la::IspaLibFunctionCall::createExpression(call);
                    }
                    // Character primitives: native per target (ord/chr over 8-bit units).
                    if ((name == "char_code" || name == "char_from_code") && x.args.size() == 1) {
                        la::FunctionCall call;
                        call.name = std::make_shared<la::Symbol>(pythonSymbol("ISPA_STD.python_" + name));
                        call.args.push_back(expr(x.args[0]));
                        return la::FunctionCall::createExpression(call);
                    }
                    unsupported(e.pos, "unknown IR operation " + name);
                }
                if (!token_rule.empty() && !failure_context) if (auto method = x.func ? x.func->template get<py::Attribute>() : nullptr)
                    if (auto owner = method->value ? method->value->template get<py::Name>() : nullptr;
                        owner && owner->id == "ctx") {
                        if (method->attr == "skip" && x.args.empty() && x.keywords.empty()) {
                            return la::FunctionCall::createExpression(la::FunctionCall{
                                .name = std::make_shared<la::Symbol>(la::Symbol{"ispa_skip_current_token"})});
                        }
                        if (method->attr == "error" && x.args.size() == 1 && x.keywords.empty()) {
                            return la::FunctionCall::createExpression(la::FunctionCall{
                                .name = std::make_shared<la::Symbol>(la::Symbol{"ispa_token_error"}),
                                .args = {expr(x.args.front())}});
                        }
                        if (method->attr == "emit" && x.args.size() == 1 && x.keywords.empty()) {
                            auto literal = x.args.front() ? x.args.front()->template get<py::Constant>() : nullptr;
                            if (!literal || literal->kind != py::ConstantKind::Str || literal->value.empty() ||
                                !std::ranges::all_of(literal->value, [](unsigned char c) {
                                    return std::isalnum(c) || c == '_' || c == '.';
                                })) unsupported(e.pos, "ctx.emit requires a token-name string literal");
                            const auto scope_end = token_rule.rfind('.');
                            const auto qualified = literal->value.find('.') != std::string::npos || scope_end == std::string::npos ?
                                literal->value : token_rule.substr(0, scope_end + 1) + literal->value;
                            auto end = binary(la::Symbol::createExpression(la::Symbol{"start"}),
                                la::ExpressionElement::Add, la::Symbol::createExpression(la::Symbol{"length"}));
                            la::FunctionCall create;
                            create.name = std::make_shared<la::Symbol>(la::Symbol{"Token", "create"});
                            create.args = {binary(la::Symbol::createExpression(la::Symbol{"start_pos"}),
                                    la::ExpressionElement::Add, la::Symbol::createExpression(la::Symbol{"length"})),
                                end,
                                la::Int::createExpression(la::Int{.value = 0}),
                                la::FunctionCall::createExpression(la::FunctionCall{
                                    .name = std::make_shared<la::Symbol>(la::Symbol{"__line"}), .args = {end}}),
                                la::Symbol::createExpression(pythonSymbol("Tokens." + qualified + ".id")),
                                la::Inheritance::createExpression(la::Inheritance{.name = pythonSymbol("Types." + qualified)})};
                            la::StorageSymbol queue;
                            queue.what = la::Symbol::createExpression(la::Symbol{"ispa_pending_tokens"});
                            queue.path.emplace_back(la::FunctionCall{
                                .name = std::make_shared<la::Symbol>(la::Symbol{"push_back"}),
                                .args = {la::FunctionCall::createExpression(create)}});
                            return la::StorageSymbol::createExpression(queue);
                        }
                        unsupported(e.pos, "unknown token context method: " + method->attr);
                    }
                if (auto method = x.func ? x.func->template get<py::Attribute>() : nullptr; method && !object(method->value)) {
                    if (method->attr == "fromkeys") {
                        auto owner = method->value ? method->value->template get<py::Name>() : nullptr;
                        if (owner && owner->id == "dict") {
                            if (x.args.size() != 2 || !x.keywords.empty()) unsupported(e.pos, "dict.fromkeys requires keys and an explicit default");
                            la::FunctionCall call;
                            call.name = std::make_shared<la::Symbol>(pythonSymbol("ISPA_STD.python_fromkeys"));
                            for (const auto& arg : x.args) call.args.push_back(expr(arg));
                            return la::FunctionCall::createExpression(call);
                        }
                    }
                    static const std::map<std::string, std::tuple<la::ArrayMethods, std::size_t, std::size_t>> methods{
                        {"append", {la::ArrayMethods::PythonAppend, 1, 1}},
                        {"extend", {la::ArrayMethods::PythonExtend, 1, 1}},
                        {"insert", {la::ArrayMethods::PythonInsert, 2, 2}},
                        {"remove", {la::ArrayMethods::PythonRemove, 1, 1}},
                        {"pop", {la::ArrayMethods::PythonPop, 0, 2}},
                        {"clear", {la::ArrayMethods::PythonClear, 0, 0}},
                        {"copy", {la::ArrayMethods::PythonCopy, 0, 0}},
                        {"count", {la::ArrayMethods::PythonCount, 1, 1}},
                        {"index", {la::ArrayMethods::PythonIndex, 1, 3}},
                        {"reverse", {la::ArrayMethods::PythonReverse, 0, 0}},
                        {"sort", {la::ArrayMethods::PythonSort, 0, 0}},
                        {"get", {la::ArrayMethods::PythonGet, 2, 2}},
                        {"setdefault", {la::ArrayMethods::PythonSetDefault, 2, 2}},
                        {"update", {la::ArrayMethods::PythonUpdate, 0, 1}},
                        {"keys", {la::ArrayMethods::PythonKeys, 0, 0}},
                        {"values", {la::ArrayMethods::PythonValues, 0, 0}},
                        {"items", {la::ArrayMethods::PythonItems, 0, 0}},
                        {"fromkeys", {la::ArrayMethods::PythonFromKeys, 2, 2}},
                    };
                    // A name that also belongs to a user class (`get(key)`) is a plain method call when the
                    // arguments do not fit the collection method.
                    // A receiver known to be a user class keeps its own methods (`TokenSet.values()`).
                    const bool user_class = signatures && signatures->classes.contains(symbol_id(inferred(method->value)));
                    if (auto found = methods.find(method->attr); found != methods.end() && !user_class &&
                        x.args.size() >= std::get<1>(found->second) && x.args.size() <= std::get<2>(found->second)) {
                        const auto& [kind, minimum, maximum] = found->second;
                        la::ArrayMethodCall call;
                        call.method = kind;
                        for (const auto& arg : x.args) call.args.push_back(expr(arg));
                        if (!x.keywords.empty()) {
                            if (method->attr == "update") {
                                std::set<std::string> names;
                                for (const auto& keyword : x.keywords) {
                                    if (!names.insert(keyword.arg).second) unsupported(e.pos, "duplicate update keyword");
                                    la::MakeTuple item;
                                    item.args = {la::String::createExpression(la::String{.value = keyword.arg}), expr(keyword.value)};
                                    call.args.push_back(la::MakeTuple::createExpression(item));
                                }
                            } else {
                                if (method->attr != "sort") unsupported(e.pos, "unsupported collection keyword arguments");
                                py::ExprPtr reverse, key;
                                for (const auto& keyword : x.keywords) {
                                    if (keyword.arg == "reverse" && !reverse) reverse = keyword.value;
                                    else if (keyword.arg == "key" && !key) key = keyword.value;
                                    else unsupported(e.pos, "sort accepts key and reverse once each");
                                }
                                call.args.push_back(reverse ? expr(reverse) : la::Bool::createExpression(la::Bool{.value = false}));
                                if (key) call.args.push_back(expr(key));
                            }
                        }
                        la::StorageSymbol storage;
                        storage.what = group(receiver(method->value));
                        storage.path.emplace_back(call);
                        return la::StorageSymbol::createExpression(storage);
                    }
                    if (method->attr == "popitem")
                        unsupported(e.pos, "collection method requires insertion ordering or a concrete dictionary type");
                }
                if (!x.keywords.empty()) unsupported(e.pos, "keyword arguments are unsupported");
                if (action_context) if (auto method = x.func ? x.func->template get<py::Attribute>() : nullptr) {
                    if (auto owner = method->value ? method->value->template get<py::Name>() : nullptr; owner && owner->id == "ctx") {
                        const auto& name = method->attr;
                        const bool failure_method = name == "error" || name == "note" || name == "sync" || name == "sync_tokens" ||
                            name == "skip" || name == "delete" || name == "insert" || name == "replace" || name == "retry" || name == "abort";
                        const bool action_method = name == "at" || name == "peek" || name == "take" || name == "prev" ||
                            name == "mark" || name == "restore" || name == "commit" || name == "collect_braced";
                        if ((failure_method && !failure_context) || (!failure_method && !action_method))
                            unsupported(e.pos, "unknown context method: " + name);
                        if (failure_context && !token_rule.empty() &&
                            name != "at" && name != "error" && name != "delete" && name != "skip" &&
                            name != "insert" && name != "abort")
                            unsupported(e.pos, "unsupported lexer failure method: " + name);
                        if (name == "sync" || name == "sync_tokens") {
                            if (x.args.empty()) unsupported(e.pos, "ctx." + name + " requires at least one boundary");
                            for (const auto& arg : x.args) {
                                auto literal = arg ? arg->template get<py::Constant>() : nullptr;
                                if (literal && literal->kind == py::ConstantKind::Str && !literal->value.empty()) continue;
                                if (name == "sync_tokens" && !tokenName(arg).empty()) continue;
                                if (name == "sync_tokens") if (auto variable = arg ? arg->template get<py::Name>() : nullptr;
                                    variable && locals.contains(variable->id)) continue;
                                unsupported(e.pos, "ctx." + name + " boundaries must be nonempty strings or token identifiers");
                            }
                        } else if (name == "error") {
                            if (x.args.empty() || x.args.size() == 3 || x.args.size() > 4)
                                unsupported(e.pos, "ctx.error requires a message, optional code, and optional offset/length pair");
                            if (x.args.size() == 2 || x.args.size() == 4) {
                                auto code = x.args[1] ? x.args[1]->template get<py::Constant>() : nullptr;
                                if (!code || code->kind != py::ConstantKind::Str || code->value.empty())
                                    unsupported(e.pos, "ctx.error diagnostic code must be a nonempty string literal");
                            }
                        } else if (name == "note") {
                            if (x.args.size() != 1) unsupported(e.pos, "ctx.note requires one message");
                        } else if (name == "at") {
                            if (x.args.empty() || x.args.size() > 2) unsupported(e.pos, "ctx.at requires a token and optional lookahead offset");
                        } else if (name == "peek") {
                            if (x.args.size() > 1) unsupported(e.pos, "ctx.peek takes at most one lookahead offset");
                        } else if (name == "mark" || name == "restore" || name == "commit") {
                            if (x.args.size() != 1) unsupported(e.pos, "ctx." + name + " requires one checkpoint name");
                            auto label = x.args.front() ? x.args.front()->template get<py::Constant>() : nullptr;
                            if (!label || label->kind != py::ConstantKind::Str || label->value.empty())
                                unsupported(e.pos, "ctx." + name + " checkpoint name must be a nonempty string literal");
                        } else if (x.args.size() != 0) unsupported(e.pos, "ctx." + name + " takes no arguments");
                        if (name == "at") {
                            const auto token = tokenName(x.args.front());
                            if (!token.empty()) {
                                la::FunctionCall call;
                                call.name = std::make_shared<la::Symbol>("at_token");
                                call.args = {la::String::createExpression(la::String{.value = token})};
                                if (x.args.size() == 2) call.args.push_back(expr(x.args[1]));
                                la::StorageSymbol storage;
                                storage.what = group(receiver(method->value));
                                storage.path.emplace_back(call);
                                return la::StorageSymbol::createExpression(storage);
                            }
                        }
                        if (name == "delete" && failure_context && !token_rule.empty()) {
                            la::StorageSymbol storage;
                            storage.what = group(receiver(method->value));
                            storage.path.emplace_back(la::FunctionCall{
                                .name = std::make_shared<la::Symbol>("erase")});
                            return la::StorageSymbol::createExpression(storage);
                        }
                        if (name == "sync_tokens") {
                            la::FunctionCall call;
                            call.name = std::make_shared<la::Symbol>("sync_tokens");
                            for (const auto& arg : x.args) {
                                const auto token = tokenName(arg);
                                if (token.empty()) { call.args.push_back(expr(arg)); continue; }
                                la::FunctionCall boundary;
                                boundary.name = std::make_shared<la::Symbol>("token_boundary");
                                boundary.args.push_back(la::String::createExpression(la::String{.value = token}));
                                la::StorageSymbol tagged;
                                tagged.what = group(receiver(method->value));
                                tagged.path.emplace_back(boundary);
                                call.args.push_back(la::StorageSymbol::createExpression(tagged));
                            }
                            la::StorageSymbol storage;
                            storage.what = group(receiver(method->value));
                            storage.path.emplace_back(call);
                            return la::StorageSymbol::createExpression(storage);
                        }
                    }
                }
                la::FunctionCall call;
                for (const auto& a : x.args) call.args.push_back(argument(a));
                if (auto n = x.func ? x.func->template get<py::Name>() : nullptr) {
                    if (!locals.contains(n->id) && is_class(n->id)) {
                        call.name = std::make_shared<la::Symbol>(pythonSymbol(as_value ? "ISPA_STD.make_value" : "ISPA_STD.construct"));
                        call.template_parameters.emplace_back(std::make_shared<la::Type>(la::Type{pythonSymbol(n->id)}));
                        return la::FunctionCall::createExpression(call);
                    }
                    call.name = std::make_shared<la::Symbol>(pythonSymbol(localName(n->id)));
                    return la::FunctionCall::createExpression(call);
                }
                if (auto sub = x.func ? x.func->template get<py::Subscript>() : nullptr;
                    sub && sub->value && sub->value->template get<py::Name>()) {
                    // Explicit type arguments: Optional[T]() constructs Optional<T>.
                    const auto class_id = sub->value->template get<py::Name>()->id;
                    la::Type built{pythonSymbol(class_id)};
                    if (auto args = sub->slice ? sub->slice->template get<py::Tuple>() : nullptr) {
                        for (const auto& arg : args->elts) built.template_parameters.emplace_back(type(arg));
                    } else built.template_parameters.emplace_back(type(sub->slice));
                    if (is_class(class_id)) {
                        call.name = std::make_shared<la::Symbol>(pythonSymbol("ISPA_STD.construct"));
                        call.template_parameters.emplace_back(std::make_shared<la::Type>(std::move(built)));
                        return la::FunctionCall::createExpression(call);
                    }
                    call.name = std::make_shared<la::Symbol>(pythonSymbol(class_id));
                    for (const auto& item : built.template_parameters)
                        if (const auto* t = std::get_if<la::Type>(&item)) call.template_parameters.emplace_back(std::make_shared<la::Type>(*t));
                    return la::FunctionCall::createExpression(call);
                }
                if (auto a = x.func ? x.func->template get<py::Attribute>() : nullptr) {
                    if (auto owner = a->value ? a->value->template get<py::Name>() : nullptr; owner && class_names.contains(owner->id)) {
                        call.name = std::make_shared<la::Symbol>(pythonSymbol(owner->id + "." + a->attr));
                        return la::FunctionCall::createExpression(call);
                    }
                    call.name = std::make_shared<la::Symbol>(a->attr);
                    la::StorageSymbol s;
                    s.what = group(receiver(a->value));
                    s.path.emplace_back(call);
                    return la::StorageSymbol::createExpression(s);
                }
                unsupported(e.pos, "call target must be a name or method");
            } else if constexpr (std::is_same_v<T, py::BinaryExpr>) return binary(expr(x.left), binop(x.op, e.pos), expr(x.right));
            else if constexpr (std::is_same_v<T, py::UnaryExpr>) {
                la::Expression out;
                switch (x.op) {
                    case py::UnaryOp::Not: out.emplace_back(la::ExpressionElement::Not); break;
                    case py::UnaryOp::UAdd: out.emplace_back(la::ExpressionElement::Add); break;
                    case py::UnaryOp::USub: out.emplace_back(la::ExpressionElement::Minus); break;
                    default: unsupported(e.pos, "bitwise invert is unsupported");
                }
                auto operand = group(expr(x.operand));
                out.insert(out.end(), operand.begin(), operand.end());
                return group(out);
            } else if constexpr (std::is_same_v<T, py::BoolExpr>) {
                if (x.values.empty()) unsupported(e.pos, "empty boolean expression");
                auto out = expr(x.values.front());
                for (std::size_t i = 1; i < x.values.size(); ++i)
                    out = binary(out, x.op == py::BoolOp::And ? la::ExpressionElement::And : la::ExpressionElement::Or, expr(x.values[i]));
                return out;
            } else if constexpr (std::is_same_v<T, py::Compare>) {
                if (x.ops.size() != 1 || x.comparators.size() != 1)
                    unsupported(e.pos, "chained comparison requires single-evaluation lowering");
                la::ExpressionElement op;
                switch (x.ops[0]) {
                    case py::CmpOp::Eq: op = la::ExpressionElement::Equal; break;
                    case py::CmpOp::NotEq: op = la::ExpressionElement::NotEqual; break;
                    case py::CmpOp::Lt: op = la::ExpressionElement::Lower; break;
                    case py::CmpOp::LtE: op = la::ExpressionElement::LowerOrEqual; break;
                    case py::CmpOp::Gt: op = la::ExpressionElement::Higher; break;
                    case py::CmpOp::GtE: op = la::ExpressionElement::HigherOrEqual; break;
                    case py::CmpOp::In: case py::CmpOp::NotIn: {
                        // `needle in container`: a collection method call on the container.
                        la::ArrayMethodCall call;
                        call.method = la::ArrayMethods::PythonContains;
                        call.args.push_back(expr(x.left));
                        la::StorageSymbol storage;
                        storage.what = group(expr(x.comparators[0]));
                        storage.path.emplace_back(call);
                        auto member = la::StorageSymbol::createExpression(storage);
                        if (x.ops[0] == py::CmpOp::In) return member;
                        la::Expression negated{la::ExpressionElement::Not};
                        auto operand = group(member);
                        negated.insert(negated.end(), operand.begin(), operand.end());
                        return group(negated);
                    }
                    default: unsupported(e.pos, "identity comparisons are unsupported");
                }
                if (action_context && (x.ops[0] == py::CmpOp::Eq || x.ops[0] == py::CmpOp::NotEq)) {
                    auto tokenCompare = [&](const py::ExprPtr& property, const py::ExprPtr& other) -> std::optional<la::Expression> {
                        auto attribute = property ? property->template get<py::Attribute>() : nullptr;
                        if (!attribute || attribute->attr != "name") return {};
                        if (const auto token = tokenName(other); !token.empty()) {
                            la::FunctionCall call;
                            call.name = std::make_shared<la::Symbol>("matches_name");
                            call.args.push_back(la::String::createExpression(la::String{.value = token}));
                            la::StorageSymbol storage;
                            storage.what = group(expr(attribute->value));
                            storage.path.emplace_back(call);
                            auto result = la::StorageSymbol::createExpression(storage);
                            if (x.ops[0] == py::CmpOp::NotEq) {
                                result = group(std::move(result));
                                result.insert(result.begin(), la::ExpressionElement::Not);
                            }
                            return result;
                        }
                        if (auto literal = other ? other->template get<py::Constant>() : nullptr;
                            literal && literal->kind == py::ConstantKind::Str) {
                            la::StorageSymbol text;
                            text.what = group(expr(attribute->value));
                            text.path.emplace_back("text");
                            return binary(la::StorageSymbol::createExpression(text), op, expr(other));
                        }
                        return {};
                    };
                    if (auto converted = tokenCompare(x.left, x.comparators[0])) return *converted;
                    if (auto converted = tokenCompare(x.comparators[0], x.left)) return *converted;
                }
                return binary(expr(x.left), op, expr(x.comparators[0]));
            } else if constexpr (std::is_same_v<T, py::Tuple>) {
                la::MakeTuple out;
                for (auto& item : x.elts) out.args.push_back(expr(item));
                return la::MakeTuple::createExpression(out);
            } else if constexpr (std::is_same_v<T, py::List>) {
                la::Array out;
                for (auto& item : x.elts) out.values.push_back(expr(item));
                for (const auto& item : inferred(std::make_shared<py::Expr>(e)).template_parameters)
                    if (auto t = std::get_if<la::Type>(&item)) out.template_parameters.push_back(std::make_shared<la::Type>(*t));
                return la::Array::createExpression(out);
            } else if constexpr (std::is_same_v<T, py::Dict>) {
                if (x.keys.size() != x.values.size()) unsupported(e.pos, "invalid dictionary");
                la::Map out;
                for (std::size_t i = 0; i < x.keys.size(); ++i) {
                    auto key = expr(x.keys[i]);
                    if (key.size() != 1 || !key.front().isRvalue()) unsupported(e.pos, "dictionary keys must be integer or string literals");
                    const auto& r = key.front().getRValue();
                    if (r.isInt()) out.keys.emplace_back(r.getInt());
                    else if (r.isString()) out.keys.emplace_back(r.getString());
                    else unsupported(e.pos, "dictionary keys must be integer or string literals");
                    out.values.push_back(expr(x.values[i]));
                }
                for (const auto& item : inferred(std::make_shared<py::Expr>(e)).template_parameters)
                    if (auto t = std::get_if<la::Type>(&item)) out.template_parameters.push_back(std::make_shared<la::Type>(*t));
                return la::Map::createExpression(out);
            } else if constexpr (std::is_same_v<T, py::Lambda>) {
                // Parameter types stay Undef: C++ (auto), Rust (closure inference) and Python infer them
                // from the expected Callable. The body sees the enclosing locals; its parameters shadow them.
                PythonConverter scope = *this;
                scope.block_depth = scope.loop_depth = 0;
                la::Lambda out;
                for (const auto& name : x.params) {
                    out.parameters.emplace_back(la::Type{la::ValueType::Undef}, name);
                    scope.locals.insert(name);
                    scope.local_types.insert_or_assign(name, la::Type{la::ValueType::Undef});
                    scope.objects.erase(name);
                }
                out.statements = la::Return::createStatements(la::Return{.value = scope.expr(x.body)});
                return la::Lambda::createExpression(std::move(out));
            } else unsupported(e.pos, "conditional expression is unsupported");
        }, e.kind);
    }
    la::Statements body(const py::StmtList& list) {
        if (!debug_ready_initialized) { debug_ready = locals; debug_ready_initialized = true; }
        la::Statements out;
        const auto checkpoint = [&](const py::Stmt& statement, const UAST::SourcePosition& position,
                                    bool after) -> std::optional<la::Statement> {
            if (position.file.empty() || position.line == 0 || statement.template get<py::Pass>())
                return std::nullopt;
            std::ostringstream marker;
            marker << "ISPA_DEBUG_LINE " << std::quoted(position.file) << ' '
                   << position.line << ' ' << position.column;
            if (after) marker << " @after";
            if (action_context && token_rule.empty()) marker << " ctx";
            for (const auto& name : debug_ready)
                if (name != "ctx") marker << ' ' << name << '=' << localName(name);
            return la::Comment::createStatement(la::Comment{.value = marker.str()});
        };
        for (const auto& p : list) {
            if (!p) throw std::invalid_argument("Python conversion: missing statement");
            auto position = p->pos;
            if (position.file.ends_with(".py") && position.line <= 1) {
                std::ifstream source(position.file);
                std::string line;
                std::size_t row = 0;
                while (std::getline(source, line)) {
                    ++row;
                    if (row < debug_line_cursor) continue;
                    const auto column = line.find_first_not_of(" \t\r");
                    if (column == std::string::npos || line[column] == '#' ||
                        line.compare(column, 5, "else:") == 0 ||
                        line.compare(column, 7, "except ") == 0 ||
                        line.compare(column, 8, "finally:") == 0) continue;
                    position.line = row;
                    position.column = column + 1;
                    debug_line_cursor = row + 1;
                    break;
                }
            }
            if (auto before = checkpoint(*p, position, false)) out.push_back(std::move(*before));
            auto part = stmt(*p);
            out.insert(out.end(), part.begin(), part.end());
            if (auto assignment = p->template get<py::AnnAssign>(); assignment && !assignment->value) {
                if (auto name = assignment->target ? assignment->target->template get<py::Name>() : nullptr)
                    debug_ready.erase(name->id);
            } else if (auto assignment = p->template get<py::Assign>()) {
                if (assignment->targets.size() == 1)
                    if (auto name = assignment->targets[0] ? assignment->targets[0]->template get<py::Name>() : nullptr)
                        debug_ready.insert(name->id);
            } else if (auto assignment = p->template get<py::AnnAssign>()) {
                if (auto name = assignment->target ? assignment->target->template get<py::Name>() : nullptr)
                    debug_ready.insert(name->id);
            }
            if (auto after = checkpoint(*p, position, true)) out.push_back(std::move(*after));
        }
        return out;
    }
    la::Statements stmt(const py::Stmt& s) {
        return std::visit([&](const auto& x) -> la::Statements {
            using T = std::decay_t<decltype(x)>;
            if constexpr (std::is_same_v<T, py::Pass>) return {};
            else if constexpr (std::is_same_v<T, py::IspaMatch>) {
                if (!embedded_match) unsupported(s.pos, "embedded ISPA match requires an advanced rule");
                return embedded_match(x, locals);
            }
            else if constexpr (std::is_same_v<T, py::ExprStmt>) return la::Statements{la::Statement{expr(x.value)}};
            else if constexpr (std::is_same_v<T, py::Return>) {
                if (!token_rule.empty()) unsupported(s.pos, "token actions cannot return; update ctx.ast instead");
                if (!action_context) return la::Return::createStatements(la::Return{.value = x.value ? expr(x.value) : la::Expression{}});
                if (failure_context && x.value) unsupported(s.pos, "fail blocks use a bare return");
                la::FunctionCall call;
                call.name = std::make_shared<la::Symbol>(failure_context ? "custom_failure_decision" : "action_result");
                if (failure_context) call.args = {la::Symbol::createExpression(la::Symbol{"before_failure"}), la::Symbol::createExpression(la::Symbol{"failed_pos"}), la::Symbol::createExpression(la::Symbol{"ctx"})};
                else {
                    la::StorageSymbol ast{"ast"}, pos{"pos"};
                    ast.what = pos.what = la::Symbol::createExpression(la::Symbol{"ctx"});
                    call.args = {x.value ? expr(x.value) : la::StorageSymbol::createExpression(ast), la::StorageSymbol::createExpression(pos)};
                }
                return la::Return::createStatements(la::Return{.value = la::FunctionCall::createExpression(call)});
            }
            else if constexpr (std::is_same_v<T, py::Break>) {
                if (!loop_depth) unsupported(s.pos, "break outside loop");
                return la::Break::createStatements(la::Break{});
            }
            else if constexpr (std::is_same_v<T, py::Continue>) {
                if (!loop_depth) unsupported(s.pos, "continue outside loop");
                return la::Continue::createStatements(la::Continue{});
            }
            else if constexpr (std::is_same_v<T, py::If>) return la::Statements{la::If{expr(x.test), nested(x.body), nested(x.orelse)}};
            else if constexpr (std::is_same_v<T, py::While>) {
                if (!x.orelse.empty()) unsupported(s.pos, "loop else is unsupported");
                return la::Statements{la::While{expr(x.test), nested(x.body, true)}};
            } else if constexpr (std::is_same_v<T, py::Assign>) {
                if (x.targets.size() != 1) unsupported(s.pos, "multiple assignment targets are unsupported");
                // `x = Class(...)` where x never outlives the call and is bound once: a stack object.
                std::string stack_class;
                if (!action_context && !analysis_missing && function_body && x.value)
                    if (auto call = x.value->template get<py::Call>(); call && call->func)
                        if (auto cn = call->func->template get<py::Name>(); cn && !locals.contains(cn->id) && is_class(cn->id) && signatures)
                            if (auto target_name = x.targets[0] ? x.targets[0]->template get<py::Name>() : nullptr;
                                target_name && !locals.contains(target_name->id) && !previous_retained.contains(target_name->id) &&
                                store_count(*function_body, target_name->id) == 1 && !(block_depth && escapes_block(target_name->id))) {
                                const auto cls = signatures->classes.find(cn->id);
                                if (cls != signatures->classes.end() && !cls->second.generic) stack_class = cn->id;
                            }
                construct_as_value = !stack_class.empty();
                auto value = expr(x.value);
                construct_as_value = false;
                if (!stack_class.empty()) {
                    const auto name = x.targets[0]->template get<py::Name>()->id;
                    locals.insert(name);
                    local_types.insert_or_assign(name, la::Type{pythonSymbol(stack_class)});
                    if (block_depth) { block_assigned.back().push_back(name); block_declared.back().push_back(name); }
                    return la::Statements{la::Variable{.name = localName(name), .type = la::Type{pythonSymbol(stack_class)}, .value = value}};
                }
                if (auto n = x.targets[0] ? x.targets[0]->template get<py::Name>() : nullptr; n && !locals.contains(n->id)) {
                    if (action_context && n->id == "ctx") unsupported(s.pos, "ctx cannot be rebound");
                    const bool hoist = block_depth && escapes_block(n->id);
                    auto value_type = inferred(x.value);
                    bool collection = false;
                    if (value_type.isValueType() && value_type.getValueType() == la::ValueType::Undef)
                        if (auto list = x.value ? x.value->template get<py::List>() : nullptr; list && list->elts.empty()) {
                            if (const auto used = usage_type(n->id); !(used.isValueType() && used.getValueType() == la::ValueType::Undef)) {
                                value_type = used;
                                collection = true;
                                value = initializer(x.value, used);
                            }
                        }
                    if (hoist && value_type.isValueType() && value_type.getValueType() == la::ValueType::Undef)
                        unsupported(s.pos, "cannot infer the type of '" + n->id + "', which is used outside this block; declare it with an annotation first");
                    locals.insert(n->id);
                    if (object(x.value)) objects.insert(n->id);
                    local_types.insert_or_assign(n->id, value_type);
                    if (block_depth) block_assigned.back().push_back(n->id);
                    if (hoist) {
                        hoisted.emplace_back(localName(n->id), value_type);
                        return la::VariableAssignment::createStatements(la::VariableAssignment{.name = target(x.targets[0]), .value = value});
                    }
                    if (block_depth) block_declared.back().push_back(n->id);
                    // A literal's own C++ type (`const char*` for "") is not the Python type, so scalars are declared.
                    const bool scalar = value_type.isValueType() && (value_type.getValueType() == la::ValueType::String ||
                        value_type.getValueType() == la::ValueType::Int || value_type.getValueType() == la::ValueType::Bool ||
                        value_type.getValueType() == la::ValueType::Float);
                    // `x = self.items` binds the same list: a reference, unless x is rebound later.
                    auto declared_type = scalar || collection ? value_type : la::Type{la::ValueType::Undef};
                    if (!hoist && !collection && function_body && value_type.isValueType() &&
                        (value_type.getValueType() == la::ValueType::Array || value_type.getValueType() == la::ValueType::Map) &&
                        x.value && (x.value->template get<py::Attribute>() || x.value->template get<py::Subscript>() ||
                                    x.value->template get<py::Name>()) && store_count(*function_body, n->id) == 1)
                        declared_type = la::Type{la::ValueType::BorrowSemantic, value_type};
                    return la::Statements{la::Variable{.name = localName(n->id), .type = declared_type, .value = value}};
                }
                return la::VariableAssignment::createStatements(la::VariableAssignment{.name = target(x.targets[0]), .value = value});
            } else if constexpr (std::is_same_v<T, py::AnnAssign>) {
                auto n = x.target ? x.target->template get<py::Name>() : nullptr;
                if (!n && (action_context || !self_name.empty())) {
                    const auto destination = target(x.target);
                    if (!x.value) return {};
                    return la::VariableAssignment::createStatements(la::VariableAssignment{
                        .name = destination, .value = initializer(x.value, type(x.annotation))});
                }
                if (!n) unsupported(s.pos, "annotated target must be a name");
                if (action_context && n->id == "ctx") unsupported(s.pos, "ctx cannot be rebound");
                if (locals.contains(n->id)) unsupported(s.pos, "annotated declaration must introduce a new local");
                const bool hoist = block_depth && escapes_block(n->id);
                locals.insert(n->id);
                local_types.insert_or_assign(n->id, type(x.annotation));
                if (block_depth) block_assigned.back().push_back(n->id);
                if (hoist) {
                    hoisted.emplace_back(localName(n->id), type(x.annotation));
                    if (!x.value) return {};
                    return la::VariableAssignment::createStatements(la::VariableAssignment{
                        .name = target(x.target), .value = initializer(x.value, type(x.annotation))});
                }
                if (block_depth) block_declared.back().push_back(n->id);
                if (auto annotation = x.annotation ? x.annotation->template get<py::Name>() : nullptr;
                    annotation && class_names.contains(annotation->id)) objects.insert(n->id);
                return la::Statements{la::Variable{.name = localName(n->id), .type = type(x.annotation), .value = x.value ? initializer(x.value, type(x.annotation)) : la::Expression{}}};
            } else if constexpr (std::is_same_v<T, py::AugAssign>) {
                la::OperatorType op;
                switch (x.op) {
                    case py::BinOp::Add: op = la::OperatorType::Add; break;
                    case py::BinOp::Sub: op = la::OperatorType::Minus; break;
                    case py::BinOp::Mult: op = la::OperatorType::Multiply; break;
                    case py::BinOp::Div: case py::BinOp::FloorDiv: op = la::OperatorType::Divide; break;
                    case py::BinOp::Mod: op = la::OperatorType::Modulo; break;
                    default: unsupported(s.pos, "augmented operator is unsupported");
                }
                return la::VariableAssignment::createStatements(la::VariableAssignment{.name = target(x.target), .type = op, .value = expr(x.value)});
            } else if constexpr (std::is_same_v<T, py::Raise>) {
                if (x.cause) unsupported(s.pos, "raise from is unsupported");
                return la::Statements{la::Throw{.throw_value = x.exc ? expr(x.exc) : la::Expression{}}};
            } else unsupported(s.pos, "statement has no LangAPI representation (use declaration conversion for definitions)");
        }, s.kind);
    }
    la::Declaration declaration(const py::Stmt& s) {
        if (auto f = s.get<py::FunctionDef>()) {
            if (f->is_async || !f->decorators.empty()) unsupported(s.pos, "async functions and decorators are unsupported");
            PythonConverter scope;
            la::Function out;
            out.name = f->name;
            out.type = type(f->returns);
            scope.self_name = self_name;
            scope.class_names = class_names;
            scope.typevars = typevars;
            scope.class_typevars = class_typevars;
            scope.signatures = signatures;
            scope.module_prefix = module_prefix;
            scope.self_class = self_class;
            scope.function_key = self_class.empty() ? module_prefix + f->name : self_class + "." + f->name;
            if (signatures) {
                if (const auto found = signatures->retained_locals.find(scope.function_key); found != signatures->retained_locals.end()) {
                    scope.previous_retained = found->second;
                    scope.analysis_missing = false;
                }
            } else scope.analysis_missing = true;
            {
                // Type variables the class does not already own become template parameters of the function.
                std::vector<std::string> used;
                const auto collect = [&](const auto& self, const py::ExprPtr& annotation) -> void {
                    if (!annotation) return;
                    if (auto n = annotation->template get<py::Name>()) {
                        if (typevars.contains(n->id) && !class_typevars.contains(n->id) &&
                            std::ranges::find(used, n->id) == used.end()) used.push_back(n->id);
                    } else if (auto sub = annotation->template get<py::Subscript>()) {
                        self(self, sub->slice);
                    } else if (auto tuple = annotation->template get<py::Tuple>()) {
                        for (const auto& item : tuple->elts) self(self, item);
                    } else if (auto list = annotation->template get<py::List>()) {
                        for (const auto& item : list->elts) self(self, item);
                    }
                };
                for (const auto& p : f->params) collect(collect, p.annotation);
                collect(collect, f->returns);
                for (const auto& name : used) out.template_parameters.emplace_back(name, decltype(out.template_parameters)::value_type::second_type{});
            }
            if (s.pos.file.ends_with(".py")) {
                std::ifstream source(s.pos.file);
                std::string line;
                std::size_t row = 0;
                while (std::getline(source, line)) {
                    ++row;
                    if (line.find("def " + f->name + "(") != std::string::npos) {
                        scope.debug_line_cursor = row + 1;
                        break;
                    }
                }
            }
            if (!self_name.empty()) scope.objects.insert(self_name);
            for (auto& p : f->params) {
                if (!self_name.empty() && p.name == self_name && &p == &f->params.front()) continue;
                if (p.default_value) unsupported(p.pos, "default parameters are unsupported");
                auto parameter_type = type(p.annotation);
                // A str is immutable: a parameter the body never rebinds is passed by const reference.
                if (parameter_type.isValueType() && parameter_type.getValueType() == la::ValueType::String && !rebinds(f->body, p.name))
                    parameter_type = la::Type{la::ValueType::BorrowSemantic, la::Type{la::ValueType::ImmutableSemantic, parameter_type}};
                if (parameter_type.isValueType() && (parameter_type.getValueType() == la::ValueType::Array ||
                                                     parameter_type.getValueType() == la::ValueType::Map)) {
                    const auto pass = container_pass(f->body, p.name);
                    if (pass == ContainerPass::ConstRef)
                        parameter_type = la::Type{la::ValueType::BorrowSemantic, la::Type{la::ValueType::ImmutableSemantic, parameter_type}};
                    else if (pass == ContainerPass::Ref)
                        parameter_type = la::Type{la::ValueType::BorrowSemantic, parameter_type};
                }
                out.parameters.emplace_back(std::move(parameter_type), p.name);
                scope.locals.insert(p.name);
                scope.local_types.insert_or_assign(p.name, type(p.annotation));
                if (auto annotation = p.annotation ? p.annotation->template get<py::Name>() : nullptr;
                    annotation && class_names.contains(annotation->id)) scope.objects.insert(p.name);
            }
            scope.function_body = &f->body;
            scope.return_type = out.type;
            out.statements = scope.body(f->body);
            for (auto it = scope.hoisted.rbegin(); it != scope.hoisted.rend(); ++it)
                out.statements.insert(out.statements.begin(), la::Statement{la::Variable{.name = it->first, .type = it->second}});
            if (out.statements.empty() && (out.type == la::ValueType::Void || f->name == "__init__")) out.statements = la::Return::createStatements(la::Return{});
            return la::Declaration{out};
        }
        if (auto c = s.get<py::ClassDef>()) {
            if (!c->decorators.empty() || !c->keywords.empty()) unsupported(s.pos, "class decorators and keywords are unsupported");
            la::Class out;
            out.shared_object = true;
            out.name = la::Symbol{c->name};
            std::set<std::string> own_typevars;
            for (auto& b : c->bases) {
                if (auto generic = b ? b->template get<py::Subscript>() : nullptr;
                    generic && generic->value && generic->value->template get<py::Name>() &&
                    (generic->value->template get<py::Name>()->id == "Generic" || generic->value->template get<py::Name>()->id == "typing.Generic")) {
                    std::vector<py::ExprPtr> names;
                    if (auto tuple = generic->slice ? generic->slice->template get<py::Tuple>() : nullptr) names = tuple->elts;
                    else names.push_back(generic->slice);
                    for (const auto& item : names) {
                        auto var = item ? item->template get<py::Name>() : nullptr;
                        if (!var || !typevars.contains(var->id)) unsupported(s.pos, "Generic[...] arguments must be TypeVars");
                        out.template_parameters.push_back(var->id);
                        own_typevars.insert(var->id);
                    }
                    continue;
                }
                auto n = b ? b->template get<py::Name>() : nullptr;
                if (!n) unsupported(s.pos, "base class must be a name");
                out.inherit_members.emplace_back(la::Visibility::Public, pythonSymbol(n->id));
            }
            PythonConverter scope;
            scope.class_names = class_names;
            scope.typevars = typevars;
            scope.class_typevars = own_typevars;
            scope.signatures = signatures;
            scope.module_prefix = module_prefix;
            scope.self_class = module_prefix + c->name;
            std::set<std::string> fields;
            for (const auto& member : c->body) if (auto a = member->template get<py::AnnAssign>(); a && a->target)
                if (auto n = a->target->template get<py::Name>()) fields.insert(n->id);
            // Discover fixed instance fields from __init__. Explicit annotations
            // win; otherwise a typed parameter or scalar literal supplies the type.
            for (const auto& member : c->body) if (auto init = member->template get<py::FunctionDef>();
                init && init->name == "__init__" && !init->params.empty()) {
                for (const auto& line : init->body) {
                    py::ExprPtr destination, value, annotation;
                    if (auto a = line->template get<py::Assign>(); a && a->targets.size() == 1) {
                        destination = a->targets[0]; value = a->value;
                    } else if (auto a = line->template get<py::AnnAssign>()) {
                        destination = a->target; value = a->value; annotation = a->annotation;
                    } else continue;
                    auto attr = destination ? destination->template get<py::Attribute>() : nullptr;
                    auto owner = attr && attr->value ? attr->value->template get<py::Name>() : nullptr;
                    if (!owner || owner->id != init->params.front().name || fields.contains(attr->attr)) continue;
                    la::Type field_type{la::ValueType::Undef};
                    if (annotation) field_type = type(annotation);
                    else if (auto name = value ? value->template get<py::Name>() : nullptr) {
                        for (const auto& p : init->params) if (p.name == name->id) field_type = type(p.annotation);
                    } else if (auto literal = value ? value->template get<py::Constant>() : nullptr) {
                        switch (literal->kind) {
                            case py::ConstantKind::Int: field_type = la::Type{la::ValueType::Int}; break;
                            case py::ConstantKind::Float: field_type = la::Type{la::ValueType::Float}; break;
                            case py::ConstantKind::Bool: field_type = la::Type{la::ValueType::Bool}; break;
                            case py::ConstantKind::Str: field_type = la::Type{la::ValueType::String}; break;
                            default: break;
                        }
                    }
                    // A subclass may assign fields its base class declares.
                    if (field_type == la::ValueType::Undef && !c->bases.empty()) continue;
                    if (field_type == la::ValueType::Undef) unsupported(line->pos, "instance field requires an annotation or a typed parameter initializer");
                    fields.insert(attr->attr);
                    out.data.emplace_back(std::make_shared<la::Declaration>(la::Variable{.name = attr->attr, .type = field_type}), la::Visibility::Public);
                }
            }
            for (auto& member : c->body) {
                if (!member) unsupported(s.pos, "missing class member");
                if (member->template get<py::Pass>()) continue;
                if (auto text = member->template get<py::ExprStmt>(); text && text->value && text->value->template get<py::Constant>() &&
                    text->value->template get<py::Constant>()->kind == py::ConstantKind::Str) continue;
                if (auto f = member->template get<py::FunctionDef>()) {
                    auto method = *f;
                    bool is_static = false;
                    if (method.decorators.size() == 1) {
                        auto name = method.decorators[0]->template get<py::Name>();
                        if (name && name->id == "staticmethod") { is_static = true; method.decorators.clear(); }
                    }
                    if (!is_static && method.params.empty()) unsupported(member->pos, "instance methods require a self parameter");
                    scope.self_name = is_static ? "" : method.params.front().name;
                    py::Stmt syntax{member->pos, method};
                    auto lowered = scope.declaration(syntax);
                    auto& function = lowered.getFunction();
                    function.is_static = is_static;
                    function.is_virtual = !is_static && f->name != "__init__";
                    if (f->name == "__init__") {
                        if (is_static || (f->returns && function.type != la::ValueType::Void))
                            unsupported(member->pos, "__init__ must be an instance method returning None");
                        // A method run by construct(): `self` may escape while it executes.
                        function.name = "__init__";
                        function.type = la::Type{la::ValueType::Void};
                        out.data.emplace_back(std::make_shared<la::Declaration>(std::move(lowered)), la::Visibility::Public);
                        scope.self_name.clear();
                        continue;
                    }
                    out.data.emplace_back(std::make_shared<la::Declaration>(std::move(lowered)), la::Visibility::Public);
                    scope.self_name.clear();
                } else {
                    // `NAME: ClassVar[T] = value` is a class-level constant, not an instance field.
                    if (auto a = member->template get<py::AnnAssign>(); a && a->target && a->annotation) {
                        auto marker = a->annotation->template get<py::Subscript>();
                        auto marker_name = marker && marker->value ? marker->value->template get<py::Name>() : nullptr;
                        auto field = a->target->template get<py::Name>();
                        if (marker_name && (marker_name->id == "ClassVar" || marker_name->id == "typing.ClassVar") && field) {
                            if (!a->value) unsupported(member->pos, "ClassVar requires an initializer");
                            const auto field_type = scope.type(marker->slice);
                            la::Variable constant{.name = field->id, .type = field_type,
                                                  .value = scope.initializer(a->value, field_type), .is_static = true};
                            out.data.emplace_back(std::make_shared<la::Declaration>(la::Declaration{constant}), la::Visibility::Public);
                            continue;
                        }
                    }
                    auto lowered = scope.declaration(*member);
                    if (lowered.isVariable() && lowered.getVariable().type == la::ValueType::Undef)
                        unsupported(member->pos, "class fields require type annotations");
                    out.data.emplace_back(std::make_shared<la::Declaration>(std::move(lowered)), la::Visibility::Public);
                }
            }
            return la::Declaration{out};
        }
        auto statements = stmt(s);
        if (statements.size() == 1 && statements.front().isVariable()) return la::Declaration{statements.front().getVariable()};
        unsupported(s.pos, "expected a function, class or variable declaration");
    }
};
}
auto ParserBase::convertPython(const UAST::py::Expr& e) -> LangAPI::Expression { return PythonConverter{}.expr(e); }
auto ParserBase::convertPython(const UAST::py::Stmt& s, const std::set<std::string>& variables) -> LangAPI::Statements { return PythonConverter{variables}.stmt(s); }
auto ParserBase::convertPython(const UAST::py::Module& m, const std::set<std::string>& variables) -> LangAPI::Statements { return PythonConverter{variables}.body(m.body); }
auto ParserBase::convertPythonDeclaration(const UAST::py::Stmt& s, const std::set<std::string>& classes,
                                          const std::set<std::string>& typevars, const UAST::py::Signatures* signatures,
                                          const std::string& module_prefix) -> LangAPI::Declaration {
    PythonConverter converter;
    converter.class_names = classes;
    converter.typevars = typevars;
    converter.signatures = signatures;
    converter.module_prefix = module_prefix;
    return converter.declaration(s);
}
auto ParserBase::analyzePythonDeclaration(const UAST::py::Stmt& s, const std::set<std::string>& classes,
                                          const std::set<std::string>& typevars, UAST::py::Signatures& signatures,
                                          const std::string& module_prefix) -> void {
    PythonConverter converter;
    converter.class_names = classes;
    converter.typevars = typevars;
    converter.signatures = &signatures;
    converter.mutable_signatures = &signatures;
    converter.module_prefix = module_prefix;
    converter.analyse_statement(s);
}
auto ParserBase::convertPythonDeclarations(const UAST::py::Module& m) -> LangAPI::Declarations {
    PythonConverter converter;
    for (const auto& s : m.body) if (s) if (auto cls = s->template get<py::ClassDef>()) converter.class_names.insert(cls->name);
    LangAPI::Declarations out;
    for (auto& s : m.body) {
        if (!s) unsupported(m.pos, "missing declaration");
        if (s->template get<UAST::py::Pass>()) continue;
        out.push_back(converter.declaration(*s));
    }
    return out;
}
auto ParserBase::convertPythonType(const UAST::py::ExprPtr& annotation) -> LangAPI::Type {
    return PythonConverter{}.type(annotation);
}
auto ParserBase::convertPythonAction(const AST::PythonAction& action, std::set<std::string>& variables, bool failure,
    const std::set<std::string>& classes, std::set<std::string>* objects, const std::string& token_rule,
    const std::function<LangAPI::Statements(const UAST::py::IspaMatch&, std::set<std::string>&)>& match) -> LangAPI::Statements {
    PythonConverter converter{variables, true, failure};
    converter.class_names = classes;
    converter.token_rule = token_rule;
    converter.embedded_match = match;
    if (objects) converter.objects = *objects;
    auto result = std::visit([&](const auto& code) -> LangAPI::Statements {
        using T = std::decay_t<decltype(code)>;
        if constexpr (std::is_same_v<T, UAST::py::Module>) return converter.body(code.body);
        else if constexpr (std::is_same_v<T, UAST::py::Expr>) {
            auto value = converter.expr(code);
            if (action.target.empty()) return {LangAPI::Statement{std::move(value)}};
            LangAPI::StorageSymbol destination{action.target};
            destination.what = LangAPI::Symbol::createExpression(LangAPI::Symbol{
                token_rule.empty() ? "result" : "ispa_token_result"});
            return LangAPI::IspaLibFunctionCall::createStatements(LangAPI::IspaLibFunctionCall{
                .symbol = LangAPI::IspaLibSymbol{.exports = LangAPI::StdlibExports::AssignCompatible},
                .args = {LangAPI::StorageSymbol::createExpression(std::move(destination)), std::move(value)}});
        }
        else throw std::invalid_argument("Python action has no executable syntax");
    }, action.code);
    variables = std::move(converter.locals);
    if (objects) *objects = std::move(converter.objects);
    return result;
}
}
