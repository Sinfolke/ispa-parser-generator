export module AST.Semantic;
import std;
import AST.API;
import UAST.Python;
import ISPA.Diagnostics;

export namespace AST {
    // Metadata not currently preserved in AST::Rule (formal arguments/counter ops).
    // The grammar frontend should populate these when registering a rule.
    struct SemanticParameter {
        std::string name;
        UAST::SourcePosition position;
        std::optional<DataType> type;
    };
    struct SemanticCounter {
        std::string name;
        UAST::SourcePosition position;
        std::size_t assignments = 0;
        std::size_t reads = 0;
    };
    struct SemanticFailure {
        std::string name;
        UAST::SourcePosition position;
        std::size_t references = 0;
    };
    struct SemanticRuleInfo {
        std::string name;                         // fully qualified rule name
        const Rule* rule = nullptr;                // borrowed until check()
        std::vector<SemanticParameter> parameters;
        std::vector<SemanticParameter> templateParameters;
        std::vector<std::string> modifiers;
        std::vector<SemanticCounter> counters;
        std::vector<SemanticFailure> failures;
        std::set<std::string> externalASTReads;  // actual reads from downstream code
        std::set<std::string> externalCaptureReads; // e.g. captures used by opaque failure actions
        std::vector<std::string> pythonImports;    // e.g. "py.std" exposes name "std"
        std::vector<UAST::py::Expr> embeddedExpressions;
        std::vector<UAST::py::Module> embeddedStatements;
    };
    struct SemanticOptions {
        bool strictPythonNames = false;          // unknown Python names: error vs warning
        bool checkUnusedASTFields = false;       // requires a complete consumer read index
        bool warnSuspiciousAtCaptures = true;    // heuristic, never an error
        bool checkUnusedLocals = true;
    };

    class Semantic {
    public:
        explicit Semantic(SemanticOptions options = {});
        ~Semantic();
        Semantic(Semantic&&) noexcept;
        Semantic& operator=(Semantic&&) noexcept;
        Semantic(const Semantic&) = delete;
        Semantic& operator=(const Semantic&) = delete;

        void addRule(SemanticRuleInfo rule);
        void addSource(std::string file, std::string source);
        // Sidecar Python: name is the importable module name (e.g. "py.std").
        void addPythonModule(std::string name, UAST::py::Module module);
        void addPythonSource(std::string name, std::string file, std::string source);
        // Fields actually consumed outside their defining rule; used with checkUnusedASTFields.
        void markASTRead(std::string qualifiedRuleName, std::string fieldName);
        // Run both grammar checks and type/scope checks over embedded and sidecar Python.
        void check();
        ISPA::Diagnostics::Reporter& diagnostics();
        const ISPA::Diagnostics::Reporter& diagnostics() const;
    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}

namespace {
    using namespace AST;
    namespace py = UAST::py;
    namespace diag = ISPA::Diagnostics;
    using Loc = diag::Location;
    template<class P> Loc at(const P& p) {
        return {p.file, p.offset, p.length, p.line, p.column};
    }
    std::string join(const auto& items, std::string_view sep = "::") {
        std::string out;
        for (const auto& s : items) { if (!out.empty()) out += sep; out += s; }
        return out;
    }
    std::string terminalName(const RuleMemberName& n) { return join(n.name); }
    std::string leaf(const std::string& name) {
        const auto pos = name.rfind("::");
        return pos == std::string::npos ? name : name.substr(pos + 2);
    }
    bool isSpace(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
    bool startsWhite(const RuleMember& n) {
        if (auto* s = std::get_if<AST::String>(&n.value))
            return !s->value.empty() && isSpace(s->value.front());
        if (auto* x = std::get_if<RuleMemberName>(&n.value))
            return !x->name.empty() && (x->name.back() == "SPACE" || x->name.back() == "WS");
        return false;
    }
    void walk(const RuleMember& m, const auto& fn, std::set<const RuleMember*>& active) {
        if (!active.insert(&m).second) return;
        fn(m);
        if (auto* g = std::get_if<RuleMemberGroup>(&m.value)) {
            for (const auto& child : g->values) if (child) walk(*child, fn, active);
        } else if (auto* op = std::get_if<RuleMemberOp>(&m.value)) {
            for (const auto& child : op->options) if (child) walk(*child, fn, active);
        }
        active.erase(&m);
    }
    void walk(const RuleMember& m, const auto& fn) {
        std::set<const RuleMember*> active;
        walk(m, fn, active);
    }
    bool repeating(char q) { return q == '*' || q == '+'; }
    bool optional(char q) { return q == '*' || q == '?'; }
    enum class Nullable { No, Yes, Unknown };
    Nullable baseNullable(const RuleMember& m, const std::map<std::string, Nullable>& known,
                          std::set<const RuleMember*>& active);
    Nullable nullable(const RuleMember& m, const std::map<std::string, Nullable>& known,
                      std::set<const RuleMember*>& active) {
        if (optional(m.quantifier)) return Nullable::Yes;
        if (!active.insert(&m).second) return Nullable::Unknown;
        auto result = baseNullable(m, known, active);
        active.erase(&m);
        return result;
    }
    Nullable baseNullable(const RuleMember& m, const std::map<std::string, Nullable>& known,
                          std::set<const RuleMember*>& active) {
        // Lexer negation consumes a character, and a dynamic pattern only
        // succeeds for a nonempty runtime string.
        if (m.negative_match || std::ranges::any_of(m.features, [](const auto& feature) {
            return std::holds_alternative<AST::DynamicPattern>(feature);
        })) return Nullable::No;
        if (auto* s = std::get_if<AST::String>(&m.value)) return s->value.empty() ? Nullable::Yes : Nullable::No;
        if (std::holds_alternative<RuleMemberNospace>(m.value)) return Nullable::Yes;
        if (auto* n = std::get_if<RuleMemberName>(&m.value)) {
            if (!n->templateParameter.empty()) return Nullable::Unknown;
            auto it = known.find(terminalName(*n));
            return it == known.end() ? Nullable::Unknown : it->second;
        }
        if (auto* g = std::get_if<RuleMemberGroup>(&m.value)) {
            bool unsure = false;
            for (const auto& c : g->values) if (c) {
                auto v = nullable(*c, known, active);
                if (v == Nullable::No) return Nullable::No;
                unsure |= v == Nullable::Unknown;
            }
            return unsure ? Nullable::Unknown : Nullable::Yes;
        }
        if (auto* op = std::get_if<RuleMemberOp>(&m.value)) {
            bool unsure = false;
            for (const auto& c : op->options) if (c) {
                auto v = nullable(*c, known, active);
                if (v == Nullable::Yes) return Nullable::Yes;
                unsure |= v == Nullable::Unknown;
            }
            return unsure ? Nullable::Unknown : Nullable::No;
        }
        if (std::holds_alternative<std::monostate>(m.value)) return Nullable::Yes;
        return Nullable::No;
    }
    Nullable nullable(const RuleMember& m, const std::map<std::string, Nullable>& known) {
        std::set<const RuleMember*> active;
        return nullable(m, known, active);
    }
    Nullable sequenceNullable(const Rule& rule, const std::map<std::string, Nullable>& known) {
        bool unsure = false;
        for (const auto& m : rule.rule_members) if (m) {
            const auto n = nullable(*m, known);
            if (n == Nullable::No) return Nullable::No;
            unsure |= n == Nullable::Unknown;
        }
        return unsure ? Nullable::Unknown : Nullable::Yes;
    }
    // Only recognizes complete *obvious* overlapping single-token/literal alternatives.
    std::optional<std::string> firstLiteral(const RuleMember& m) {
        if (auto* s = std::get_if<AST::String>(&m.value))
            return s->value.empty() ? std::nullopt : std::optional<std::string>("str:" + s->value);
        if (auto* n = std::get_if<RuleMemberName>(&m.value))
            return !n->name.empty() && n->isTerminal() ?
            std::optional<std::string>("token:" + terminalName(*n)) : std::nullopt;
        if (auto* g = std::get_if<RuleMemberGroup>(&m.value)) {
            if (g->values.size() == 1 && g->values.front()) return firstLiteral(*g->values.front());
        }
        return std::nullopt;
    }

    enum class TK { Unknown, None, Bool, Int, Float, Str, List, Tuple, Dict, Function, Module, Rule, Token, Context, Union };
    struct Ty {
        TK kind = TK::Unknown;
        std::string name;
        std::vector<Ty> args;
        bool operator==(const Ty&) const = default;
    };
    Ty type(TK k) { return {k, {}, {}}; }
    bool numeric(const Ty& t) { return t.kind == TK::Int || t.kind == TK::Float || t.kind == TK::Bool; }
    bool known(const Ty& t) { return t.kind != TK::Unknown && t.kind != TK::Union; }
    std::string repr(const Ty& t) {
        switch (t.kind) {
            case TK::Unknown: return "unknown"; case TK::None: return "None";
            case TK::Bool: return "bool"; case TK::Int: return "int";
            case TK::Float: return "float"; case TK::Str: return "str";
            case TK::List: return "list"; case TK::Tuple: return "tuple";
            case TK::Dict: return "dict"; case TK::Function: return "callable";
            case TK::Module: return "module " + t.name;
            case TK::Rule: return "rule"; case TK::Token: return "token";
            case TK::Context: return "ctx";
            case TK::Union: return "union";
        }
        return "unknown";
    }
    Ty unify(Ty a, Ty b) {
        if (a.kind == TK::Unknown || b.kind == TK::Unknown) return type(TK::Unknown);
        if (a == b) return a;
        if (numeric(a) && numeric(b)) return type((a.kind == TK::Float || b.kind == TK::Float) ? TK::Float : TK::Int);
        return {TK::Union, {}, {std::move(a), std::move(b)}};
    }
    Ty ofCapture(const DataType& d) {
        switch (d.kind) {
            case DataType::Kind::Character: return type(TK::Str);
            case DataType::Kind::String: return type(TK::Str);
            case DataType::Kind::Token: return type(TK::Token);
            case DataType::Kind::Rule: return type(TK::Rule);
            case DataType::Kind::Array: return type(TK::List);
            case DataType::Kind::Tuple: return type(TK::Tuple);
            default: return type(TK::Unknown);
        }
    }
    Ty annotation(const py::ExprPtr& e) {
        if (!e) return type(TK::Unknown);
        if (auto* n = e->get<py::Name>()) {
            if (n->id == "int") return type(TK::Int);
            if (n->id == "float") return type(TK::Float);
            if (n->id == "str") return type(TK::Str);
            if (n->id == "bool") return type(TK::Bool);
            if (n->id == "list") return type(TK::List);
            if (n->id == "tuple") return type(TK::Tuple);
            if (n->id == "dict") return type(TK::Dict);
            if (n->id == "None") return type(TK::None);
        }
        if (auto* sub = e->get<py::Subscript>()) return annotation(sub->value);
        return type(TK::Unknown);
    }
    bool assignable(const Ty& dest, const Ty& src) {
        if (!known(dest) || !known(src)) return true;
        if (dest == src) return true;
        if (dest.kind == TK::Float && numeric(src)) return true;
        if (dest.kind == TK::Int && src.kind == TK::Bool) return true;
        return false;
    }
    struct Signature {
        std::vector<std::pair<std::string, Ty>> parameters;
        std::size_t minArgs = 0;
        bool variadic = false;
        Ty result;
    };
    struct Symbol {
        Ty ty;
        Loc declaration;
        bool used = false;
        bool isParam = false;
        bool isImport = false;
        bool isCapture = false;
        bool isBuiltin = false;
        bool modulePublic = false;
        bool annotated = false;
        std::optional<Signature> signature;
    };
    using Symbols = std::map<std::string, Symbol>;
    using Modules = std::map<std::string, Symbols>;

    Symbols builtins() {
        Symbols s;
        auto add = [&](std::string name, Ty result, std::size_t min, bool variadic = false) {
            Symbol sym;
            sym.ty = type(TK::Function); sym.isBuiltin = true;
            sym.signature = Signature{{}, min, variadic, result};
            if (!variadic) for (std::size_t i = 0; i < min; ++i)
                sym.signature->parameters.emplace_back("arg"+std::to_string(i+1), type(TK::Unknown));
            s.emplace(std::move(name), std::move(sym));
        };
        for (auto& [name, kind] : std::vector<std::pair<std::string,TK>>{
                 {"int",TK::Int},{"float",TK::Float},{"str",TK::Str},
                 {"bool",TK::Bool},{"list",TK::List},{"tuple",TK::Tuple},{"dict",TK::Dict}})
            add(name, type(kind), 0, true);
        add("len", type(TK::Int), 1); add("range", type(TK::List), 1, true);
        add("enumerate", type(TK::List), 1, true); add("zip", type(TK::List), 0, true);
        add("print", type(TK::None), 0, true); add("sum", type(TK::Unknown), 1, true);
        add("min", type(TK::Unknown), 1, true); add("max", type(TK::Unknown), 1, true);
        add("abs", type(TK::Unknown), 1); add("isinstance", type(TK::Bool), 2);
        add("all", type(TK::Bool), 1); add("any", type(TK::Bool), 1);
        add("sorted", type(TK::List), 1, true); add("reversed", type(TK::List), 1);
        for (auto& n : {"True", "False", "None"}) {
            Symbol sym; sym.ty = type(std::string_view(n) == "None" ? TK::None : TK::Bool);
            sym.isBuiltin = true; s.emplace(n, sym);
        }
        return s;
    }
    struct Scope {
        Scope* parent = nullptr;
        Symbols symbols;
        bool module = false;
        int loopDepth = 0, functionDepth = 0;
        bool inClass = false;
        std::set<std::string> globals, nonlocals;
        Symbol* find(const std::string& name) {
            for (Scope* s = this; s; s = s->parent) {
                auto it = s->symbols.find(name);
                if (it != s->symbols.end()) return &it->second;
            }
            return nullptr;
        }
    };
    std::optional<Signature> signature(const py::FunctionDef& f) {
        Signature sig;
        sig.result = annotation(f.returns);
        bool defaultSeen = false;
        for (const auto& p : f.params) {
            sig.parameters.emplace_back(p.name, annotation(p.annotation));
            if (!p.default_value) {
                if (!defaultSeen) ++sig.minArgs;
            } else defaultSeen = true;
        }
        return sig;
    }
    // A collection of referenced Python identifiers inside an expression.
    void names(const py::Expr* e, std::set<std::string>& out) {
        if (!e) return;
        std::visit([&](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            auto one = [&](const py::ExprPtr& x) { names(x.get(), out); };
            auto many = [&](const py::ExprList& xs) { for (const auto& x : xs) one(x); };
            if constexpr (std::is_same_v<T,py::Name>) {
                if (v.ctx == py::ExprContext::Load) out.insert(v.id);
            } else if constexpr (std::is_same_v<T,py::Attribute>) one(v.value);
            else if constexpr (std::is_same_v<T,py::Subscript>) { one(v.value); one(v.slice); }
            else if constexpr (std::is_same_v<T,py::Call>) {
                one(v.func); many(v.args); for (const auto& k : v.keywords) one(k.value);
            } else if constexpr (std::is_same_v<T,py::UnaryExpr>) one(v.operand);
            else if constexpr (std::is_same_v<T,py::BinaryExpr>) { one(v.left); one(v.right); }
            else if constexpr (std::is_same_v<T,py::BoolExpr>) many(v.values);
            else if constexpr (std::is_same_v<T,py::Compare>) { one(v.left); many(v.comparators); }
            else if constexpr (std::is_same_v<T,py::IfExpr>) { one(v.test); one(v.body); one(v.orelse); }
            else if constexpr (std::is_same_v<T,py::Tuple> || std::is_same_v<T,py::List>) many(v.elts);
            else if constexpr (std::is_same_v<T,py::Dict>) { many(v.keys); many(v.values); }
        }, e->kind);
    }
    void names(const py::Stmt* stmt, std::set<std::string>& out);
    void names(const py::StmtList& stmts, std::set<std::string>& out) {
        for (const auto& s : stmts) names(s.get(),out);
    }
    void names(const py::Stmt* stmt, std::set<std::string>& out) {
        if (!stmt) return;
        const auto one = [&](const py::ExprPtr& e) { names(e.get(),out); };
        const auto many = [&](const py::ExprList& xs) {
            for (const auto& e : xs) one(e);
        };
        std::visit([&](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T,py::FunctionDef>) {
                one(v.returns); many(v.decorators);
                for (const auto& p : v.params) { one(p.annotation); one(p.default_value); }
                names(v.body,out);
            } else if constexpr (std::is_same_v<T,py::ClassDef>) {
                many(v.bases); many(v.decorators);
                for (const auto& k : v.keywords) one(k.value);
                names(v.body,out);
            } else if constexpr (std::is_same_v<T,py::Return>) one(v.value);
            else if constexpr (std::is_same_v<T,py::Delete>) many(v.targets);
            else if constexpr (std::is_same_v<T,py::Assign>) one(v.value);
            else if constexpr (std::is_same_v<T,py::AugAssign>) { one(v.target); one(v.value); }
            else if constexpr (std::is_same_v<T,py::AnnAssign>) { one(v.annotation); one(v.value); }
            else if constexpr (std::is_same_v<T,py::For>) {
                one(v.iter); names(v.body,out); names(v.orelse,out);
            } else if constexpr (std::is_same_v<T,py::While> || std::is_same_v<T,py::If>) {
                one(v.test); names(v.body,out); names(v.orelse,out);
            } else if constexpr (std::is_same_v<T,py::With>) {
                for (const auto& i : v.items) one(i.context);
                names(v.body,out);
            } else if constexpr (std::is_same_v<T,py::Raise>) { one(v.exc); one(v.cause); }
            else if constexpr (std::is_same_v<T,py::Try>) {
                names(v.body,out); names(v.orelse,out); names(v.finalbody,out);
                for (const auto& h : v.handlers) { one(h.type); names(h.body,out); }
            } else if constexpr (std::is_same_v<T,py::Assert>) { one(v.test); one(v.msg); }
            else if constexpr (std::is_same_v<T,py::ExprStmt>) one(v.value);
        },stmt->kind);
    }
}

namespace {
    // Conservative, AST-based subset analyzer: unknown types propagate without type errors.
    // All names are looked up in lexical scopes, sidecar module exports and builtins.
    class PythonChecker {
    public:
        PythonChecker(diag::Reporter& out, const Modules& imports, const SemanticOptions& opt)
            : out_(out), modules_(imports), options_(opt) {}

        void check(const py::Module& module, Symbols provided = {}, bool exported = false) {
            Scope root;
            root.module = exported;
            root.symbols = builtins();
            for (auto& [name, sym] : provided) root.symbols.insert_or_assign(name, std::move(sym));
            predeclare(module.body, root);
            block(module.body, root, type(TK::Unknown));
            // Top-level module names may be imported from other files. A grammar
            // action is not a public Python module, so only explicit captures are
            // diagnosed by the grammar analyzer rather than unused locals here.
        }
        Ty expression(const py::Expr& expr, Symbols provided = {}) {
            Scope root; root.symbols = builtins();
            for (auto& [n,s] : provided) root.symbols.insert_or_assign(n, std::move(s));
            return eval(&expr, root);
        }
    private:
        diag::Reporter& out_;
        const Modules& modules_;
        const SemanticOptions& options_;

        void report(diag::Severity severity, std::string code, std::string message,
                    Loc pos, std::string help = {}) {
            out_.emit({severity, std::move(code), std::move(message), std::move(pos), {}, std::move(help)});
        }
        void error(std::string c, std::string m, Loc p) { report(diag::Severity::Error, std::move(c), std::move(m), std::move(p)); }
        void warning(std::string c, std::string m, Loc p) { report(diag::Severity::Warning, std::move(c), std::move(m), std::move(p)); }
        Symbol* look(Scope& scope, const std::string& n) { return scope.find(n); }
        void bind(Scope& scope, std::string n, Ty ty, Loc pos, bool parameter = false) {
            Scope* target = &scope;
            if (scope.globals.contains(n)) {
                while (target->parent && !target->module) target = target->parent;
            } else if (scope.nonlocals.contains(n)) {
                for (auto* s = scope.parent; s; s = s->parent) {
                    if (!s->module && s->symbols.contains(n)) { target = s; break; }
                }
            }
            auto it = target->symbols.find(n);
            if (it != target->symbols.end() && !it->second.isBuiltin) {
                if (!assignable(it->second.ty, ty))
                    warning("PY210", "cannot assign " + repr(ty) + " to '" + n + "' of type " + repr(it->second.ty), pos);
                // A re-assigned inferred variable acquires the new type; an annotated
                // variable retains its declared type (tracked by isParam here).
                if (!it->second.isParam && !it->second.annotated) it->second.ty = ty;
                return;
            }
            Symbol sym; sym.ty = std::move(ty); sym.declaration = pos; sym.isParam = parameter;
            target->symbols.insert_or_assign(std::move(n),std::move(sym));
        }
        static void define(Scope& scope, std::string name, Ty ty, Loc pos,
                           std::optional<Signature> sig = std::nullopt) {
            Symbol sym; sym.ty = std::move(ty); sym.declaration = pos;
            sym.signature = std::move(sig); sym.modulePublic = scope.module;
            scope.symbols.insert_or_assign(std::move(name), std::move(sym));
        }
        void predeclare(const py::StmtList& body, Scope& scope) {
            for (const auto& p : body) if (p) {
                if (auto* fn = p->get<py::FunctionDef>())
                    define(scope, fn->name, type(TK::Function), at(p->pos), signature(*fn));
                else if (auto* cl = p->get<py::ClassDef>())
                    define(scope, cl->name, {TK::Function, cl->name, {}}, at(p->pos));
            }
        }
        Ty eval(const py::ExprPtr& e, Scope& s) { return eval(e.get(), s); }
        Ty eval(const py::Expr* e, Scope& s) {
            if (!e) return type(TK::Unknown);
            const Loc p = at(e->pos);
            return std::visit([&](const auto& v) -> Ty {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T,py::Name>) {
                    if (v.ctx != py::ExprContext::Load) return type(TK::Unknown);
                    if (auto* sym = look(s, v.id)) { sym->used = true; return sym->ty; }
                    report(options_.strictPythonNames ? diag::Severity::Error : diag::Severity::Warning,
                        "PY001", "unresolved Python name '" + v.id + "'", p,
                        "Declare it in the action, a capture, or an imported Python sidecar module.");
                    return type(TK::Unknown);
                } else if constexpr (std::is_same_v<T,py::Constant>) {
                    switch (v.kind) {
                        case py::ConstantKind::None: return type(TK::None);
                        case py::ConstantKind::Bool: return type(TK::Bool);
                        case py::ConstantKind::Int: return type(TK::Int);
                        case py::ConstantKind::Float: return type(TK::Float);
                        case py::ConstantKind::Str: return type(TK::Str);
                    }
                    return type(TK::Unknown);
                } else if constexpr (std::is_same_v<T,py::Attribute>) {
                    const auto base = eval(v.value, s);
                    if (base.kind == TK::Context) {
                        if (v.attr == "pos" || v.attr == "start_pos") return type(TK::Int);
                        if (v.attr == "rule" || v.attr == "failed_rule" || v.attr == "text" ||
                            v.attr == "line_prefix" || v.attr == "next_char") return type(TK::Str);
                        if (v.attr == "ast" || v.attr == "shared") return type(TK::Unknown);
                        static const std::set<std::string> methods{
                            "at", "mark", "restore", "commit", "collect_braced", "error", "note",
                            "sync", "sync_tokens", "skip", "insert", "replace", "retry", "abort", "emit"};
                        if (methods.contains(v.attr)) return type(TK::Function);
                        error("PY261", "unknown ctx member '" + v.attr + "'", p);
                        return type(TK::Unknown);
                    }
                    if (base.kind == TK::Module) {
                        auto mod = modules_.find(base.name);
                        if (mod != modules_.end()) {
                            auto field = mod->second.find(v.attr);
                            if (field != mod->second.end()) return field->second.ty;
                            warning("PY202", "module '" + base.name + "' has no exported name '" + v.attr + "'", p);
                        }
                    }
                    return type(TK::Unknown); // dynamic object attributes are intentionally unknown
                } else if constexpr (std::is_same_v<T,py::Subscript>) {
                    Ty base = eval(v.value, s); Ty key = eval(v.slice, s);
                    if (base.kind == TK::Str || base.kind == TK::List || base.kind == TK::Tuple) {
                        if (known(key) && key.kind != TK::Int && key.kind != TK::Bool)
                            warning("PY203", "sequence index should be integer, got " + repr(key), p);
                        if (base.kind == TK::Str) return type(TK::Str);
                        if (!base.args.empty()) return base.args.front();
                    } else if (base.kind == TK::Dict && base.args.size() > 1) return base.args[1];
                    return type(TK::Unknown);
                } else if constexpr (std::is_same_v<T,py::Call>) {
                    auto fn = eval(v.func, s);
                    std::vector<Ty> actual;
                    for (const auto& a : v.args) actual.push_back(eval(a, s));
                    std::set<std::string> usedKeys;
                    for (const auto& k : v.keywords) {
                        eval(k.value, s);
                        if (!k.arg.empty() && !usedKeys.insert(k.arg).second)
                            error("PY204", "duplicate keyword argument '" + k.arg + "'", at(k.pos));
                    }
                    const Symbol* callee = nullptr;
                    if (v.func) {
                        if (auto* name = v.func->template get<py::Name>()) {
                            callee = look(s, name->id);
                        } else if (auto* attr = v.func->template get<py::Attribute>()) {
                            const auto base = eval(attr->value, s);
                            if (base.kind == TK::Module) {
                                auto module = modules_.find(base.name);
                                if (module != modules_.end()) {
                                    auto exported = module->second.find(attr->attr);
                                    if (exported != module->second.end()) callee = &exported->second;
                                }
                            }
                        }
                    }
                    if (callee && callee->signature) {
                        const auto& sig = *callee->signature;
                        const std::string callableName = v.func && v.func->template get<py::Name>() ?
                            v.func->template get<py::Name>()->id : "module function";
                            if (actual.size() + v.keywords.size() < sig.minArgs)
                                error("PY205", "too few arguments to '" + callableName + "'", p);
                            if (!sig.variadic && actual.size() + v.keywords.size() > sig.parameters.size()
                                && !sig.parameters.empty())
                                error("PY206", "too many arguments to '" + callableName + "'", p);
                            for (std::size_t i = 0; i < actual.size() && i < sig.parameters.size(); ++i)
                                if (!assignable(sig.parameters[i].second, actual[i]))
                                    error("PY207", "argument '" + sig.parameters[i].first + "' expects " +
                                          repr(sig.parameters[i].second) + ", got " + repr(actual[i]), p);
                            for (const auto& k : v.keywords) if (!k.arg.empty()) {
                                auto it = std::find_if(sig.parameters.begin(), sig.parameters.end(),
                                    [&](const auto& a) { return a.first == k.arg; });
                                if (it != sig.parameters.end()) {
                                    const std::size_t index = static_cast<std::size_t>(it - sig.parameters.begin());
                                    if (index < actual.size()) error("PY208", "multiple values for '" + k.arg + "'", at(k.pos));
                                } else if (!sig.variadic && !sig.parameters.empty())
                                    warning("PY209", "unknown keyword parameter '" + k.arg + "'", at(k.pos));
                            }
                        return sig.result;
                    }
                    (void)fn;
                    return type(TK::Unknown);
                } else if constexpr (std::is_same_v<T,py::UnaryExpr>) {
                    auto operand = eval(v.operand, s);
                    if (v.op == py::UnaryOp::Not) return type(TK::Bool);
                    if (v.op == py::UnaryOp::Invert && known(operand) &&
                        operand.kind != TK::Int && operand.kind != TK::Bool)
                        error("PY222", "bitwise invert requires an integer operand",p);
                    else if (known(operand) && !numeric(operand))
                        error("PY220", "unary numeric operator cannot be applied to " + repr(operand), p);
                    return operand;
                } else if constexpr (std::is_same_v<T,py::BinaryExpr>) {
                    const auto a = eval(v.left, s), b = eval(v.right, s);
                    if (v.op == py::BinOp::Add && a.kind == TK::Str && b.kind == TK::Str) return type(TK::Str);
                    if (v.op == py::BinOp::Add && a.kind == TK::List && b.kind == TK::List)
                        return {TK::List, {}, {a.args.empty() || b.args.empty() ? type(TK::Unknown) :
                                                   unify(a.args.front(),b.args.front())}};
                    if (v.op == py::BinOp::Add && a.kind == TK::Tuple && b.kind == TK::Tuple) {
                        Ty result = a; result.args.insert(result.args.end(),b.args.begin(),b.args.end());
                        return result;
                    }
                    if (v.op == py::BinOp::Mult &&
                        ((a.kind == TK::Str && (b.kind == TK::Int || b.kind == TK::Bool)) ||
                         (b.kind == TK::Str && (a.kind == TK::Int || a.kind == TK::Bool))))
                        return type(TK::Str);
                    if (v.op == py::BinOp::Mult &&
                        ((a.kind == TK::List && b.kind == TK::Int) ||
                         (b.kind == TK::List && a.kind == TK::Int)))
                        return a.kind == TK::List ? a : b;
                    if (numeric(a) && numeric(b))
                        return v.op == py::BinOp::Div ? type(TK::Float) : unify(a,b);
                    if (known(a) && known(b))
                        error("PY221", "incompatible operands: " + repr(a) + " and " + repr(b), p);
                    return type(TK::Unknown);
                } else if constexpr (std::is_same_v<T,py::BoolExpr>) {
                    Ty result = type(TK::Unknown);
                    bool first = true;
                    for (const auto& e : v.values) { auto current = eval(e, s);
                        result = first ? current : unify(result,current); first = false; }
                    return result;
                } else if constexpr (std::is_same_v<T,py::Compare>) {
                    eval(v.left,s); for (const auto& c : v.comparators) eval(c,s);
                    return type(TK::Bool);
                } else if constexpr (std::is_same_v<T,py::IfExpr>) {
                    eval(v.test,s); return unify(eval(v.body,s), eval(v.orelse,s));
                } else if constexpr (std::is_same_v<T,py::Tuple>) {
                    Ty t = type(TK::Tuple);
                    for (const auto& x : v.elts) t.args.push_back(eval(x,s));
                    return t;
                } else if constexpr (std::is_same_v<T,py::List>) {
                    Ty t = type(TK::List);
                    bool first = true; Ty element;
                    for (const auto& x : v.elts) { auto current = eval(x,s);
                        element = first ? current : unify(element,current); first = false; }
                    if (!first) t.args.push_back(element);
                    return t;
                } else if constexpr (std::is_same_v<T,py::Dict>) {
                    Ty k, val; bool first = true;
                    for (const auto& x : v.keys) { auto curr = eval(x,s); k = first ? curr : unify(k,curr); first = false; }
                    first = true;
                    for (const auto& x : v.values) { auto curr = eval(x,s); val = first ? curr : unify(val,curr); first = false; }
                    return {TK::Dict, {}, {k,val}};
                }
                return type(TK::Unknown);
            },e->kind);
        }
        void store(const py::ExprPtr& target, Ty value, Scope& scope) {
            if (!target) return;
            if (auto* n = target->template get<py::Name>()) {
                bind(scope,n->id,value,at(target->pos));
            } else if (auto* tuple = target->get<py::Tuple>()) {
                for (std::size_t i = 0; i < tuple->elts.size(); ++i)
                    store(tuple->elts[i], i < value.args.size() ? value.args[i] : type(TK::Unknown), scope);
            } else if (auto* list = target->get<py::List>()) {
                for (const auto& item : list->elts)
                    store(item, !value.args.empty() ? value.args.front() : type(TK::Unknown), scope);
            } else if (auto* a = target->get<py::Attribute>()) {
                eval(a->value,scope);
            } else if (auto* sub = target->get<py::Subscript>()) {
                eval(sub->value,scope); eval(sub->slice,scope);
            } else error("PY230","invalid assignment target",at(target->pos));
        }
        void checkLocalUnused(const Scope& scope) {
            if (!options_.checkUnusedLocals) return;
            for (const auto& [name,sym] : scope.symbols) {
                if (name.starts_with('_') || sym.isBuiltin || sym.isCapture || sym.modulePublic) continue;
                if (!sym.used && !sym.declaration.file.empty())
                    warning(sym.isParam ? "PY102" : "PY103",
                            std::string(sym.isParam ? "unused parameter '" : "unused local '") + name + "'", sym.declaration);
            }
        }
        void block(const py::StmtList& statements, Scope& scope, const Ty& returnType) {
            bool terminated = false;
            for (const auto& stmt : statements) if (stmt) {
                if (terminated) warning("PY104", "unreachable statement", at(stmt->pos));
                bool kills = statement(*stmt, scope, returnType);
                terminated |= kills;
            }
        }
        bool statement(const py::Stmt& stmt, Scope& s, const Ty& returns) {
            const Loc p = at(stmt.pos);
            return std::visit([&](const auto& v) -> bool {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T,py::FunctionDef>) {
                    for (const auto& d : v.decorators) eval(d,s);
                    for (const auto& par : v.params) { eval(par.default_value,s); /* annotations are type syntax */ }
                    Scope child; child.parent = &s; child.functionDepth = s.functionDepth + 1;
                    predeclare(v.body,child);
                    bool seenDefault = false;
                    for (const auto& par : v.params) {
                        if (par.default_value) seenDefault = true;
                        else if (seenDefault) error("PY240", "non-default parameter follows default parameter", at(par.pos));
                        bind(child,par.name,annotation(par.annotation),at(par.pos),true);
                    }
                    block(v.body,child,annotation(v.returns));
                    checkLocalUnused(child);
                } else if constexpr (std::is_same_v<T,py::ClassDef>) {
                    for (const auto& b : v.bases) eval(b,s);
                    for (const auto& k : v.keywords) eval(k.value,s);
                    for (const auto& d : v.decorators) eval(d,s);
                    Scope child; child.parent = &s; child.inClass = true;
                    predeclare(v.body,child); block(v.body,child,returns);
                } else if constexpr (std::is_same_v<T,py::Return>) {
                    if (s.functionDepth == 0) error("PY241", "return outside a function", p);
                    const Ty result = v.value ? eval(v.value,s) : type(TK::None);
                    if (!assignable(returns,result)) error("PY242", "return type " + repr(result) +
                        " is incompatible with declared " + repr(returns),
                        v.value ? at(v.value->pos) : p);
                    return true;
                } else if constexpr (std::is_same_v<T,py::Delete>) {
                    for (const auto& t : v.targets) {
                        if (t && t->template get<py::Name>()) {
                            const auto& n = t->template get<py::Name>()->id;
                            if (!look(s,n)) warning("PY243", "deleting unknown name '" + n + "'", at(t->pos));
                        } else eval(t,s);
                    }
                } else if constexpr (std::is_same_v<T,py::Assign>) {
                    const auto result = eval(v.value,s);
                    for (const auto& t : v.targets) store(t,result,s);
                } else if constexpr (std::is_same_v<T,py::AugAssign>) {
                    Ty lhs = type(TK::Unknown);
                    if (v.target) {
                        if (auto* n = v.target->template get<py::Name>()) {
                            if (auto* sym = look(s,n->id)) { sym->used = true; lhs = sym->ty; }
                            else warning("PY001", "unresolved Python name '" + n->id + "'", at(v.target->pos));
                        } else lhs = eval(v.target,s);
                    }
                    auto rhs = eval(v.value,s);
                    if (known(lhs) && known(rhs) && !numeric(lhs) &&
                        !(lhs.kind == TK::Str && rhs.kind == TK::Str))
                        warning("PY244", "possibly incompatible augmented assignment",p);
                    store(v.target,unify(lhs,rhs),s);
                } else if constexpr (std::is_same_v<T,py::AnnAssign>) {
                    auto declared = annotation(v.annotation);
                    if (v.value) { auto actual = eval(v.value,s);
                        if (!assignable(declared,actual)) error("PY245", "annotated assignment expects " +
                            repr(declared) + ", got " + repr(actual),p); }
                    store(v.target,declared,s);
                    if (v.target && v.target->template get<py::Name>())
                        if (auto* sym = look(s,v.target->template get<py::Name>()->id)) sym->annotated = true;
                } else if constexpr (std::is_same_v<T,py::For>) {
                    Ty iterable = eval(v.iter,s); Ty item;
                    if (!iterable.args.empty()) item = iterable.args.front();
                    else if (iterable.kind == TK::Str) item = type(TK::Str);
                    store(v.target,item,s);
                    ++s.loopDepth; block(v.body,s,returns); --s.loopDepth;
                    block(v.orelse,s,returns);
                } else if constexpr (std::is_same_v<T,py::While>) {
                    eval(v.test,s); ++s.loopDepth; block(v.body,s,returns); --s.loopDepth;
                    block(v.orelse,s,returns);
                } else if constexpr (std::is_same_v<T,py::If>) {
                    eval(v.test,s); block(v.body,s,returns); block(v.orelse,s,returns);
                } else if constexpr (std::is_same_v<T,py::With>) {
                    for (const auto& item : v.items) {
                        eval(item.context,s); if (item.vars) store(item.vars,type(TK::Unknown),s);
                    }
                    block(v.body,s,returns);
                } else if constexpr (std::is_same_v<T,py::Raise>) {
                    eval(v.exc,s); eval(v.cause,s); return true;
                } else if constexpr (std::is_same_v<T,py::Try>) {
                    block(v.body,s,returns);
                    for (const auto& h : v.handlers) {
                        eval(h.type,s);
                        if (!h.name.empty()) bind(s,h.name,type(TK::Unknown),at(h.pos));
                        block(h.body,s,returns);
                    }
                    block(v.orelse,s,returns); block(v.finalbody,s,returns);
                } else if constexpr (std::is_same_v<T,py::Assert>) {
                    eval(v.test,s); eval(v.msg,s);
                } else if constexpr (std::is_same_v<T,py::Import>) {
                    for (const auto& a : v.names) {
                        auto n = a.asname.empty() ? a.name.substr(0,a.name.find('.')) : a.asname;
                        define(s,n,{TK::Module,a.asname.empty() ? n : a.name,{}},at(a.pos));
                        s.symbols[n].isImport = true;
                    }
                } else if constexpr (std::is_same_v<T,py::ImportFrom>) {
                    auto mod = modules_.find(v.module_name);
                    for (const auto& a : v.names) {
                        if (a.name == "*") {
                            if (mod != modules_.end()) {
                                for (const auto& [name,sym] : mod->second)
                                    if (!name.starts_with('_')) s.symbols.insert_or_assign(name,sym);
                            } else warning("PY250", "cannot statically resolve wildcard import from '" + v.module_name + "'",p);
                            continue;
                        }
                        auto n = a.asname.empty() ? a.name : a.asname;
                        Symbol sym; sym.declaration = at(a.pos); sym.isImport = true;
                        if (mod != modules_.end()) {
                            auto it = mod->second.find(a.name);
                            if (it == mod->second.end()) warning("PY251", "unknown imported name '" + a.name + "'",at(a.pos));
                            else { sym.ty = it->second.ty; sym.signature = it->second.signature; }
                        }
                        s.symbols.insert_or_assign(n,std::move(sym));
                    }
                } else if constexpr (std::is_same_v<T,py::Global>) {
                    for (const auto& name : v.names) s.globals.insert(name);
                } else if constexpr (std::is_same_v<T,py::Nonlocal>) {
                    for (const auto& name : v.names) {
                        bool found = false;
                        for (Scope* x=s.parent; x && !x->module; x=x->parent)
                            if (x->symbols.contains(name)) { found=true; break; }
                        if (!found) error("PY252", "no enclosing function binding for nonlocal '" + name + "'",p);
                        s.nonlocals.insert(name);
                    }
                } else if constexpr (std::is_same_v<T,py::ExprStmt>) {
                    eval(v.value,s);
                } else if constexpr (std::is_same_v<T,py::Break>) {
                    if (!s.loopDepth) error("PY253","break outside a loop",p);
                    return true;
                } else if constexpr (std::is_same_v<T,py::Continue>) {
                    if (!s.loopDepth) error("PY254","continue outside a loop",p);
                    return true;
                }
                return false;
            },stmt.kind);
        }
    };
}

struct AST::Semantic::Impl {
    SemanticOptions options;
    diag::Reporter out;
    std::vector<SemanticRuleInfo> rules;
    std::map<std::string,py::Module> pythonModules;
    struct PendingPython { std::string module, file; };
    std::vector<PendingPython> pythonText;
    std::map<std::string,std::set<std::string>> astReads;
    explicit Impl(SemanticOptions opt) : options(opt) {}

    void warn(std::string code,std::string msg,Loc p,std::string hint = {}) {
        out.emit({diag::Severity::Warning,std::move(code),std::move(msg),std::move(p),{},std::move(hint)});
    }
    void info(std::string code,std::string msg,Loc p,std::string hint = {}) {
        out.emit({diag::Severity::Info,std::move(code),std::move(msg),std::move(p),{},std::move(hint)});
    }

    Modules indexModules() const {
        Modules index;
        // Export declared functions, classes, and simple top-level bindings.
        for (const auto& [name,module] : pythonModules) {
            auto& exports = index[name];
            for (const auto& node : module.body) if (node) {
                const Loc p = at(node->pos);
                if (const auto* f = node->get<py::FunctionDef>()) {
                    Symbol s; s.declaration = p; s.ty = type(TK::Function);
                    s.signature = signature(*f); s.modulePublic = true;
                    exports.insert_or_assign(f->name,std::move(s));
                } else if (const auto* cls = node->get<py::ClassDef>()) {
                    Symbol s; s.declaration = p; s.ty = {TK::Function,cls->name,{}}; s.modulePublic = true;
                    exports.insert_or_assign(cls->name,std::move(s));
                } else if (const auto* assn = node->get<py::Assign>()) {
                    for (const auto& target : assn->targets) if (target)
                        if (auto* n = target->template get<py::Name>()) {
                            Symbol s; s.declaration = at(target->pos); s.modulePublic = true;
                            if (assn->value) if (auto* c = assn->value->get<py::Constant>()) {
                                switch (c->kind) {
                                    case py::ConstantKind::Int:s.ty = type(TK::Int);break;
                                    case py::ConstantKind::Float:s.ty = type(TK::Float);break;
                                    case py::ConstantKind::Str:s.ty = type(TK::Str);break;
                                    case py::ConstantKind::Bool:s.ty = type(TK::Bool);break;
                                    default:s.ty = type(TK::None);
                                }
                            }
                            exports.insert_or_assign(n->id,std::move(s));
                        }
                } else if (const auto* annotated = node->get<py::AnnAssign>()) {
                    if (annotated->target)
                        if (auto* n = annotated->target->template get<py::Name>()) {
                            Symbol s; s.ty = annotation(annotated->annotation);
                            s.declaration = at(annotated->target->pos); s.modulePublic = true;
                            exports.insert_or_assign(n->id,std::move(s));
                        }
                }
            }
        }
        // Dotted module hierarchy, e.g. `import py.std` then `py.std.f()`.
        for (const auto& [name, exports] : pythonModules) {
            std::size_t dot = name.find('.');
            while (dot != std::string::npos) {
                const std::string parent = name.substr(0,dot);
                const auto next = name.find('.',dot+1);
                const std::string part = name.substr(dot+1, next == std::string::npos ?
                                                         std::string::npos : next-dot-1);
                const std::string child = name.substr(0, next);
                Symbol s; s.ty = {TK::Module,child,{}}; s.modulePublic = true;
                index[parent].insert_or_assign(part,std::move(s));
                dot = next;
            }
        }
        return index;
    }
    Symbols ruleSymbols(const SemanticRuleInfo& info) const {
        Symbols symbols;
        if (!info.rule) return symbols;
        Symbol context; context.ty = type(TK::Context); context.isParam = true;
        symbols.emplace("ctx", std::move(context));
        for (const auto& imported : info.pythonImports) {
            std::string alias = imported.substr(imported.find_last_of('.') == std::string::npos ?
                                                0 : imported.find_last_of('.') + 1);
            if (!alias.empty()) {
                Symbol s; s.ty = {TK::Module, imported, {}}; s.isImport = true;
                symbols.insert_or_assign(std::move(alias),std::move(s));
            }
        }
        for (const auto& [name,dt] : info.rule->data_block.fields) {
            Symbol s; s.ty = ofCapture(dt); s.isCapture = true; s.modulePublic = true;
            symbols.insert_or_assign(name,std::move(s));
        }
        for (const auto& p : info.parameters) {
            Symbol s; s.ty = p.type ? ofCapture(*p.type) : type(TK::Unknown);
            s.declaration = at(p.position); s.isParam = true;
            symbols.insert_or_assign(p.name,std::move(s));
        }
        for (const auto& p : info.templateParameters) {
            Symbol s; s.ty = p.type ? ofCapture(*p.type) : type(TK::Unknown);
            s.declaration = at(p.position); s.isParam = true;
            symbols.insert_or_assign(p.name,std::move(s));
        }
        for (const auto& m : info.rule->rule_members) if (m) walk(*m,[&](const RuleMember& n) {
            if (n.prefix.present && !n.prefix.name.empty()) {
                Symbol s; s.ty = ofCapture(captureType(n)); s.declaration = at(n.position);
                s.isCapture = true;
                symbols.insert_or_assign(n.prefix.name,std::move(s));
            }
        });
        return symbols;
    }
    void grammarChecks(const Modules& imported) {
        std::map<std::string,Nullable> nullables;
        std::set<std::string> terminalNames;
        for (const auto& r : rules) if (r.rule) {
            nullables.emplace(r.name,Nullable::Unknown);
            if (r.name.size() && std::isupper(static_cast<unsigned char>(leaf(r.name)[0])))
                terminalNames.insert(leaf(r.name));
        }
        // Monotone-enough convergence with a bound to avoid cycles on invalid grammars.
        for (std::size_t step = 0; step <= rules.size(); ++step) {
            bool changed = false;
            for (const auto& r : rules) if (r.rule) {
                auto next = sequenceNullable(*r.rule,nullables);
                if (nullables[r.name] != next) { nullables[r.name] = next; changed = true; }
            }
            if (!changed) break;
        }
        PythonChecker python(out, imported,options);
        for (const auto& r : rules) {
            if (!r.rule) continue;
            const Rule& rule = *r.rule;
            if (std::ranges::find(r.modifiers, "inline") != r.modifiers.end()) {
                std::size_t captures = 0;
                bool valid = true;
                const auto countCaptures = [&](const auto& self, const RuleMember& member) -> void {
                    if (member.prefix.present) {
                        ++captures;
                        if (member.quantifier) valid = false;
                    }
                    if (member.isGroup()) for (const auto& child : member.getGroup().values)
                        if (child) self(self, *child); else valid = false;
                    if (member.isOp()) for (const auto& child : member.getOp().options)
                        if (child) self(self, *child); else valid = false;
                };
                for (const auto& member : rule.rule_members)
                    if (member) countCaptures(countCaptures, *member); else valid = false;
                valid = valid && captures == 1;
                if (!valid) out.error("GRM101", "[inline] requires exactly one captured rule reference", at(rule.position));
            }
            if (std::ranges::find(r.modifiers, "skip") != r.modifiers.end() &&
                (r.name.empty() || !std::isupper(static_cast<unsigned char>(leaf(r.name)[0]))))
                out.error("GRM102", "[skip] can only be applied to token rules", at(rule.position));
            std::set<std::string> references = r.externalCaptureReads;
            std::set<std::string> captureNames;
            std::set<std::string> parametersUsed;
            bool positional = false;
            if (auto* reg = std::get_if<RegularDataBlock>(&rule.data_block.value))
                if (auto* expression = std::get_if<py::Expr>(reg)) names(expression,references);
            if (auto* keyed = std::get_if<RegularDataBlockWKeys>(&rule.data_block.value))
                for (const auto& [key, expression] : keyed->value) names(&expression,references);
            if (auto* templated = std::get_if<TemplatedDataBlock>(&rule.data_block.value))
                for (const auto& name : templated->names) references.insert(name);
            if (auto* reg = std::get_if<RegularDataBlock>(&rule.data_block.value))
                positional = std::holds_alternative<At>(*reg);
            for (const auto& expr : r.embeddedExpressions) names(&expr,references);
            for (const auto& statements : r.embeddedStatements) names(statements.body,references);
            // Information from UAST typed expressions is exact; opaque failure
            // handlers must pass externalCaptureReads via SemanticRuleInfo.
            std::vector<const RuleMember*> members;
            for (const auto& m : rule.rule_members) if (m) walk(*m,[&](const RuleMember& n) {
                members.push_back(&n);
                if (n.prefix.present && !n.prefix.name.empty()) captureNames.insert(n.prefix.name);
                if (auto* ruleName = std::get_if<RuleMemberName>(&n.value)) {
                    if (!ruleName->templateParameter.empty()) parametersUsed.insert(ruleName->templateParameter);
                    for (std::size_t j = 0; j < ruleName->templateArguments.size(); ++j)
                        if (j < ruleName->templateArgumentIsParameter.size() &&
                            ruleName->templateArgumentIsParameter[j])
                            for (const auto& name : ruleName->templateArguments[j]) parametersUsed.insert(name);
                }
                if (repeating(n.quantifier)) {
                    std::set<const RuleMember*> active;
                    if (baseNullable(n,nullables,active) == Nullable::Yes)
                        warn("G301","repetition of a nullable expression may cause ambiguity or non-termination",
                             at(n.position),"Make the repeated expression consume input on every iteration.");
                    if (auto* g = std::get_if<RuleMemberGroup>(&n.value))
                        if (g->values.size() == 1 && g->values.front() &&
                            (optional(g->values.front()->quantifier) || repeating(g->values.front()->quantifier)))
                            warn("G302","nested quantifiers likely have redundant or ambiguous repetition",
                                 at(n.position),"Consider removing one quantifier, e.g. (a*)* -> a*.");
                }
                if (n.quantifier != '\0' && std::holds_alternative<RuleMemberNospace>(n.value))
                    info("G303","quantifying NOSPACE is unlikely to have an effect",at(n.position));
                if (auto* choice = std::get_if<RuleMemberOp>(&n.value)) {
                    std::set<std::string> seen;
                    for (const auto& option : choice->options) if (option) {
                        auto first = firstLiteral(*option);
                        if (first && !seen.insert(*first).second)
                            info("G304","alternatives start with the same literal or token; lookahead may be needed",
                                 at(option->position));
                    }
                }
                // Textual adjacency is essential for @ID vs @ ID; the AST alone
                // has no way to recover this ambiguity after normalization.
                if (options.warnSuspiciousAtCaptures && n.prefix.present && !n.prefix.name.empty() &&
                    terminalNames.contains(n.prefix.name)) {
                    if (const auto* text = out.sourceText(n.position.file)) {
                        const auto pos = n.position.offset;
                        if (pos != std::string::npos && pos < text->size()) {
                            const std::string needle = "@" + n.prefix.name;
                            const auto end = std::min(text->size(),pos + n.position.length + needle.size() + 2);
                            if (text->substr(pos,end-pos).find(needle) != std::string::npos)
                                info("G305","'@" + n.prefix.name + "' names a capture rather than matching token '" + n.prefix.name + "'",
                                     at(n.position),"If you intended a positional token capture, write '@ " + n.prefix.name + "'.");
                        }
                    }
                }
            });
            // Adjacent input-level matcher nodes (not the nested group layout).
            auto seq = [&](const auto& self,const auto& list) -> void {
                for (std::size_t i=0; i<list.size(); ++i) if (list[i]) {
                    const auto& m = *list[i];
                    if (std::holds_alternative<RuleMemberNospace>(m.value) && i+1<list.size() &&
                        list[i+1] && startsWhite(*list[i+1]))
                        warn("G306","NOSPACE immediately precedes an explicit whitespace matcher",
                             at(m.position),"This constraint may conflict with the following space token.");
                    if (auto* g = std::get_if<RuleMemberGroup>(&m.value)) self(self,g->values);
                }
            };
            seq(seq,rule.rule_members);
            // Formals and capture references include datablock expressions as well as
            // every explicitly supplied embedded expression.
            for (const auto& name : references) parametersUsed.insert(name);
            for (const auto& p : r.parameters)
                if (!parametersUsed.contains(p.name))
                    warn("G101","unused rule parameter '" + p.name + "'",at(p.position));
            for (const auto& p : r.templateParameters)
                if (!parametersUsed.contains(p.name))
                    warn("G102","unused rule template parameter '" + p.name + "'",at(p.position));
            for (const auto* m : members)
                if (m->prefix.present && !m->prefix.name.empty() && !positional &&
                    !references.contains(m->prefix.name)) {
                    auto location = at(m->position);
                    if (location.offset == std::string::npos ||
                        (out.sourceText(location.file) &&
                         location.offset > out.sourceText(location.file)->size()))
                        location = at(rule.position);
                    if (const auto* source = out.sourceText(location.file);
                        source && location.offset > source->size()) {
                        const auto found = source->find("@" + m->prefix.name);
                        if (found != std::string::npos) {
                            location.offset = found;
                            location.length = m->prefix.name.size() + 1;
                            location.line = location.column = 0;
                        }
                    }
                    warn("G103","unused named capture '" + m->prefix.name + "'",location,
                         "Remove the capture or reference it in a data block or action.");
                }
            for (const auto& c : r.counters) {
                if (!c.reads && !c.assignments)
                    warn("G104","unused shared counter registration '" + c.name + "'",at(c.position));
                else if (!c.reads)
                    warn("G105","shared counter '" + c.name + "' is assigned but never read",at(c.position));
                else if (!c.assignments)
                    warn("G106","shared counter '" + c.name + "' is read but never assigned",at(c.position));
            }
            for (const auto& f : r.failures) if (!f.references)
                warn("G107","unused failure binding '" + f.name + "'",at(f.position));
            if (options.checkUnusedASTFields) {
                auto consumed = astReads[r.name];
                consumed.insert(r.externalASTReads.begin(),r.externalASTReads.end());
                for (const auto& [field,dtype] : rule.data_block.fields)
                    if (!consumed.contains(field))
                        warn("G108","AST field '" + field + "' is never read by a registered consumer",
                             at(rule.position),"Only enable this check with a complete downstream usage index.");
            }
            for (const auto& dependency : r.pythonImports)
                if (!imported.contains(dependency))
                    warn("PY260","unknown Python sidecar module '"+dependency+"'",at(rule.position));
            Symbols provided = ruleSymbols(r);
            if (auto* reg = std::get_if<RegularDataBlock>(&rule.data_block.value))
                if (auto* expr = std::get_if<py::Expr>(reg)) python.expression(*expr,provided);
            if (auto* keyed = std::get_if<RegularDataBlockWKeys>(&rule.data_block.value))
                for (const auto& [name,expr] : keyed->value) python.expression(expr,provided);
            for (const auto& expr : r.embeddedExpressions) python.expression(expr,provided);
            for (const auto& module : r.embeddedStatements) python.check(module,provided);
        }
    }
};

namespace AST {
    Semantic::Semantic(SemanticOptions options) : impl_(std::make_unique<Impl>(options)) {}
    Semantic::~Semantic() = default;
    Semantic::Semantic(Semantic&&) noexcept = default;
    Semantic& Semantic::operator=(Semantic&&) noexcept = default;
    void Semantic::addRule(SemanticRuleInfo r) { impl_->rules.push_back(std::move(r)); }
    void Semantic::addSource(std::string file, std::string source) {
        impl_->out.source(std::move(file),std::move(source));
    }
    void Semantic::addPythonModule(std::string name, py::Module m) {
        impl_->pythonModules.insert_or_assign(std::move(name),std::move(m));
    }
    void Semantic::addPythonSource(std::string name,std::string file,std::string source) {
        impl_->pythonText.push_back({std::move(name),file});
        addSource(std::move(file),std::move(source));
    }
    void Semantic::markASTRead(std::string ruleName,std::string field) {
        impl_->astReads[std::move(ruleName)].insert(std::move(field));
    }
    void Semantic::check() {
        auto& i = *impl_;
        i.out.clear();
        for (const auto& p : i.pythonText) {
            const auto* src = i.out.sourceText(p.file);
            if (!src) continue;
            try {
                UAST::SourcePosition pos{p.file,0,src->size(),1,1};
                i.pythonModules.insert_or_assign(p.module,UAST::parsePythonModule(*src,pos));
            } catch (const std::exception& e) {
                i.pythonModules.erase(p.module);
                diag::Location location{p.file, std::string::npos, 0, 1, 1};
                const std::string prefix = p.file + ":";
                const std::string message = e.what();
                if (message.starts_with(prefix)) {
                    std::size_t cursor = prefix.size();
                    auto number = [&](std::size_t& value) {
                        const auto begin = cursor;
                        while (cursor < message.size() && message[cursor] >= '0' && message[cursor] <= '9') ++cursor;
                        if (begin == cursor || cursor >= message.size() || message[cursor] != ':') return false;
                        value = std::stoull(message.substr(begin, cursor - begin));
                        ++cursor;
                        return true;
                    };
                    std::size_t line = 0, column = 0;
                    if (number(line) && number(column)) {
                        location.line = line;
                        location.column = column;
                    }
                }
                i.out.error("PY000","failed to parse sidecar Python module '" + p.module + "': " + e.what(),
                            std::move(location));
            }
        }
        const Modules modules = i.indexModules();
        PythonChecker checker(i.out,modules,i.options);
        for (const auto& [name,module] : i.pythonModules) checker.check(module,{},true);
        i.grammarChecks(modules);
    }
    diag::Reporter& Semantic::diagnostics() { return impl_->out; }
    const diag::Reporter& Semantic::diagnostics() const { return impl_->out; }
}
