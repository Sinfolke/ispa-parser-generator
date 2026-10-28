module Rust.Rewriter;

import Rust.CoreFunctions;
import LangAPI;
import dstd;
import std;

namespace {
    using namespace LangAPI;

    auto stringParts(const Symbol &s) -> std::vector<std::string> {
        std::vector<std::string> out;
        for (const auto &part : s.path)
            if (std::holds_alternative<std::string>(part)) out.push_back(std::get<std::string>(part));
        return out;
    }
    auto allStrings(const Symbol &s) -> bool {
        for (const auto &part : s.path)
            if (!std::holds_alternative<std::string>(part)) return false;
        return true;
    }
    auto joinStrings(const std::vector<std::string> &items, std::string_view sep) -> std::string {
        std::string out;
        for (std::size_t i = 0; i < items.size(); ++i) {
            if (i) out += sep;
            out += items[i];
        }
        return out;
    }
    auto makeSymbol(const std::string &name) -> Symbol { return Symbol{name}; }
    auto isImmutableWrapped(const Type &t) -> bool {
        return t.template_parameters.size() == 1 && std::holds_alternative<Type>(t.template_parameters.front()) &&
               std::get<Type>(t.template_parameters.front()).isValueType() &&
               std::get<Type>(t.template_parameters.front()).getValueType() == ValueType::ImmutableSemantic;
    }

    // ========================================================================
    // Generic, mutating IR walker. Hooks decide what happens; the walker owns the traversal order and scoping.
    // Everything reached through a shared_ptr is copied before it is mutated (the source tree is shared).
    // ========================================================================
    struct IRWalker {
        enum class FnKind { Free, Method, Constructor, Output };

        std::function<void(Symbol &, bool /*in_type*/)> on_symbol;
        std::function<void(FunctionCall &)> on_call;                // after the arguments were walked
        std::function<void(const std::string &)> on_local;
        std::function<void()> scope_push, scope_pop;
        std::function<void(Namespace &, bool)> on_namespace;
        std::function<void(Class &, bool)> on_class;
        std::function<void(Function &, FnKind, bool)> on_function;

        void push() { if (scope_push) scope_push(); }
        void pop() { if (scope_pop) scope_pop(); }

        void symbol(Symbol &s, bool in_type) {
            if (on_symbol) on_symbol(s, in_type);
            for (auto &part : s.path) {
                if (auto *il = std::get_if<IspaLibSymbol>(&part)) ilsym(*il);
                else if (auto *fc = std::get_if<FunctionCall>(&part)) call(*fc, false);
            }
        }
        template<class P> void templateParams(P &params) {
            for (auto &p : params) std::visit([&](auto &x) { tp(x); }, p);
        }
        void tp(Type &t) { type(t); }
        void tp(RValue &v) { rvalue(v); }
        void tp(std::shared_ptr<Type> &t) { if (t) { t = std::make_shared<Type>(*t); type(*t); } }
        void tp(std::shared_ptr<RValue> &v) { if (v) { v = std::make_shared<RValue>(*v); rvalue(*v); } }

        void ilsym(IspaLibSymbol &s) { templateParams(s.template_parameters); }

        void type(Type &t) {
            if (t.isSymbol()) {
                Symbol s = t.getSymbol();
                symbol(s, true);
                t.type = std::move(s);
            } else if (t.isIspaLibSymbol()) {
                auto s = t.getIspaLibSymbol();
                ilsym(s);
                t.type = std::move(s);
            }
            templateParams(t.template_parameters);
        }

        void storage(StorageSymbol &s) {
            expr(s.what);
            for (auto &part : s.path) {
                if (auto *off = std::get_if<StorageOffset>(&part)) expr(off->offset);
                else if (auto *m = std::get_if<ArrayMethodCall>(&part)) { for (auto &a : m->args) expr(a); }
                else if (auto *il = std::get_if<IspaLibSymbol>(&part)) ilsym(*il);
                else if (auto *fc = std::get_if<FunctionCall>(&part)) call(*fc, true);   // `obj.f()`: f is not a name in scope
            }
        }

        void rvalue(RValue &v) {
            switch (v.type()) {
                case RValueType::Symbol: symbol(v.getSymbol(), false); break;
                case RValueType::StorageSymbol: storage(v.getStorageSymbol()); break;
                case RValueType::IspaLibSymbol: ilsym(v.getIspaSymbol()); break;
                case RValueType::Array: {
                    auto &a = v.getArray();
                    templateParams(a.template_parameters);
                    for (auto &e : a.values) expr(e);
                    break;
                }
                case RValueType::FixedSizeArray: {
                    auto &a = v.getFixedSizeArray();
                    templateParams(a.template_parameters);
                    for (auto &e : a.values) expr(e);
                    break;
                }
                case RValueType::Map: {
                    auto &m = v.getMap();
                    templateParams(m.template_parameters);
                    for (auto &e : m.values) expr(e);
                    break;
                }
                case RValueType::Inheritance: {
                    auto &in = v.getInheritance();
                    std::visit([&](auto &n) {
                        using T = std::decay_t<decltype(n)>;
                        if constexpr (std::is_same_v<T, Symbol>) symbol(n, true);
                        else if constexpr (std::is_same_v<T, IspaLibSymbol>) ilsym(n);
                        else tp(n);
                    }, in.name);
                    for (auto &a : in.args) expr(a);
                    break;
                }
                case RValueType::Reference: {
                    auto &r = v.getReference();
                    if (r.value) { r.value = std::make_shared<RValue>(*r.value); rvalue(*r.value); }
                    break;
                }
                case RValueType::Span: {
                    auto &s = v.getSpan();
                    symbol(s.sym, false);
                    tp(s.type);
                    break;
                }
                case RValueType::MakeTuple: for (auto &a : v.getMakeTuple().args) expr(a); break;
                case RValueType::GetVariant: { auto &g = v.getVariantCast(); expr(g.sym); tp(g.type); break; }
                case RValueType::CheckVariant: { auto &g = v.CheckVariantCast(); expr(g.sym); tp(g.type); break; }
                case RValueType::ToString: expr(v.getToString().what); break;
                case RValueType::CharToStringConstructor: expr(v.getCharToStringConstructor().what); break;
                case RValueType::Cast: { auto &c = v.getCast(); expr(c.what); tp(c.type); break; }
                case RValueType::GetFromBox: expr(v.getGetFromBox().what); break;
                default: break;
            }
        }

        void call(FunctionCall &c, bool as_member) {
            std::visit([&](auto &ptr) {
                using T = std::decay_t<decltype(*ptr)>;
                if (!ptr) return;
                ptr = std::make_shared<T>(*ptr);
                if constexpr (std::is_same_v<T, Symbol>) { if (!as_member) symbol(*ptr, false); }
                else ilsym(*ptr);
            }, c.name);
            templateParams(c.template_parameters);
            for (auto &a : c.args) expr(a);
            if (on_call && !as_member) on_call(c);
        }

        void lambda(Lambda &l) {
            push();
            for (auto &p : l.parameters) { type(p.first); if (on_local) on_local(p.second); }
            stmts(l.statements);
            pop();
        }

        void value(ExpressionValue &v) {
            switch (v.type()) {
                case ExpressionValueType::RValue: rvalue(v.getRValue()); break;
                case ExpressionValueType::FunctionCall: call(v.getFunctionCall(), false); break;
                case ExpressionValueType::Return: expr(v.getReturn().value); break;
                case ExpressionValueType::VariableAssignment: {
                    auto &a = v.getVariableAssignment();
                    if (std::holds_alternative<Symbol>(a.name)) symbol(std::get<Symbol>(a.name), false);
                    else storage(std::get<StorageSymbol>(a.name));
                    expr(a.value);
                    break;
                }
                case ExpressionValueType::CounterAssignment: {
                    auto &c = v.getCounterAssignment();
                    if (c.v) {
                        expr(*c.v);
                    }
                    break;
                }
                case ExpressionValueType::Lambda: lambda(v.getLambda()); break;
                case ExpressionValueType::IspaLibFunctionCall: {
                    auto &c = v.getIspaLibFunctionCall();
                    ilsym(c.symbol);
                    templateParams(c.template_parameters);
                    for (auto &a : c.args) expr(a);
                    break;
                }
                default: break;
            }
        }
        void expr(Expression &e) { for (auto &v : e) value(v); }

        void stmts(Statements &s) { for (auto &st : s) stmt(st); }
        void stmt(Statement &s) {
            if (s.isExpression()) expr(s.getExpression());
            else if (s.isIf()) {
                auto &i = s.getIf();
                expr(i.expr);
                push(); stmts(i.stmt); pop();
                push(); stmts(i.else_stmt); pop();
            } else if (s.isWhile() || s.isDoWhile()) {
                auto &w = s.getWhileOrDoWhile();
                expr(w.expr);
                push(); stmts(w.stmt); pop();
            } else if (s.isSwitch()) {
                auto &sw = s.getSwitch();
                expr(sw.expression);
                for (auto &[r, body] : sw.cases) { rvalue(r); push(); stmts(body); pop(); }
            } else if (s.isThrow()) {
                expr(s.getThrow().throw_value);
            } else if (s.isVariable()) {
                auto &v = s.getVariable();
                type(v.type);
                expr(v.value);
                if (on_local) on_local(v.name);          // visible only after its own initialiser
            }
        }

        void function(Function &f, FnKind kind) {
            if (on_function) on_function(f, kind, true);
            push();
            for (auto &tpar : f.template_parameters) for (auto &t : tpar.second) type(t);
            for (auto &p : f.parameters) { type(p.first); if (on_local) on_local(p.second); }
            type(f.type);
            stmts(f.statements);
            pop();
            if (on_function) on_function(f, kind, false);
        }

        void klass(Class &c) {
            if (on_class) on_class(c, true);
            for (auto &in : c.inherit_members)
                std::visit([&](auto &n) {
                    if constexpr (std::is_same_v<std::decay_t<decltype(n)>, Symbol>) symbol(n, true);
                    else ilsym(n);
                }, in.second);
            if (c.constructor) function(*c.constructor, FnKind::Constructor);
            for (auto &entry : c.data) if (entry.first) decl(*entry.first, true);
            if (c.to_str_fun) function(*c.to_str_fun, FnKind::Method);
            if (c.output_fun) function(*c.output_fun, FnKind::Output);
            if (on_class) on_class(c, false);
        }

        void decl(Declaration &d, bool in_class) {
            if (d.isNamespace()) {
                auto &ns = d.getNamespace();
                if (on_namespace) on_namespace(ns, true);
                decls(ns.declarations);
                if (on_namespace) on_namespace(ns, false);
            } else if (d.isFunction()) function(d.getFunction(), in_class ? FnKind::Method : FnKind::Free);
            else if (d.isClass()) klass(d.getClass());
            else if (d.isTypeAlias()) type(d.getTypeAlias().type);
            else if (d.isVariable()) { auto &v = d.getVariable(); type(v.type); expr(v.value); }
        }
        void decls(Declarations &ds) { for (auto &d : ds) decl(d, false); }
    };

    // ========================================================================
    // 1. merge same-named namespaces
    // ========================================================================
    auto mergeNamespaces(Declarations &decls) -> void {
        Declarations out;
        std::unordered_map<std::string, std::size_t> index;
        for (auto &d : decls) {
            if (d.isNamespace()) {
                auto &ns = d.getNamespace();
                if (const auto it = index.find(ns.name); it != index.end()) {
                    auto &target = out[it->second].getNamespace();
                    for (auto &inner : ns.declarations) target.declarations.push_back(inner);
                    continue;
                }
                index[ns.name] = out.size();
            }
            out.push_back(d);
        }
        for (auto &d : out) if (d.isNamespace()) mergeNamespaces(d.getNamespace().declarations);
        decls = std::move(out);
    }

    // ========================================================================
    // 2. flatten nested declarations
    // ========================================================================
    struct Entry {
        std::vector<std::string> ns;      // namespace the declaration lives in
        std::vector<std::string> chain;   // original path below the namespace: {rule, value, STRING}
        std::string flat;                 // rule_value_STRING
    };
    struct Registry {
        std::vector<Entry> entries;
        std::unordered_map<std::string, std::vector<std::size_t>> by_chain;       // renamed declarations only
        std::unordered_map<std::string, std::vector<std::size_t>> by_flat;        // every class, renamed or not
        auto add(Entry e, bool renamed) -> void {
            const auto idx = entries.size();
            const auto key = joinStrings(e.chain, "::");
            by_flat[e.flat].push_back(idx);
            if (renamed) by_chain[key].push_back(idx);
            entries.push_back(std::move(e));
        }
        auto chainOfFlat(const std::string &flat, const std::vector<std::string> &ns) const -> std::vector<std::string> {
            if (const auto it = by_flat.find(flat); it != by_flat.end())
                for (const auto idx : it->second)
                    if (entries[idx].ns == ns) return entries[idx].chain;
            return {};
        }
    };

    auto suffixOf(const std::vector<std::string> &small, const std::vector<std::string> &big) -> bool {
        return small.size() <= big.size() && std::equal(small.rbegin(), small.rend(), big.rbegin());
    }

    struct Hoister {
        Registry &reg;

        // `chain` is empty for a namespace-level class (its own Symbol gives the chain), else the parent's chain
        auto hoistClass(Declaration &d, const std::vector<std::string> &ns, const std::vector<std::string> &parent_chain,
                        Declarations &out) -> void {
            auto &cls = d.getClass();
            const auto own = stringParts(cls.name);
            if (own.empty()) { out.push_back(d); return; }
            std::vector<std::string> chain = parent_chain;
            if (parent_chain.empty()) chain = own;
            else chain.push_back(own.back());                    // a class nested in a class is named by its leaf
            const auto flat = joinStrings(chain, "_");
            const bool renamed = chain.size() > 1;
            reg.add(Entry{ns, chain, flat}, renamed);
            cls.name = makeSymbol(flat);

            Declarations hoisted;
            std::vector<std::pair<std::shared_ptr<Declaration>, Visibility>> kept;
            for (auto &entry : cls.data) {
                Declaration member = *entry.first;               // copy: the source tree stays untouched
                const auto member_chain = [&](const std::string &n) { auto c = chain; c.push_back(n); return c; };
                const auto hoist_named = [&](const std::string &n) {
                    auto c = member_chain(n);
                    const auto f = joinStrings(c, "_");
                    reg.add(Entry{ns, c, f}, true);
                    return f;
                };
                if (member.isClass()) {
                    hoistClass(member, ns, chain, hoisted);
                } else if (member.isEnum()) {
                    member.getEnum().name = hoist_named(member.getEnum().name);
                    hoisted.push_back(member);
                } else if (member.isTypeAlias()) {
                    member.getTypeAlias().name = hoist_named(member.getTypeAlias().name);
                    hoisted.push_back(member);
                } else if (member.isVariable() && member.getVariable().is_static) {
                    member.getVariable().name = hoist_named(member.getVariable().name);
                    hoisted.push_back(member);
                } else {
                    kept.emplace_back(std::make_shared<Declaration>(std::move(member)), entry.second);
                }
            }
            cls.data = std::move(kept);
            out.push_back(d);
            for (auto &h : hoisted) out.push_back(std::move(h));
        }

        auto run(Declarations &decls, std::vector<std::string> &ns) -> void {
            Declarations out;
            for (auto &d : decls) {
                if (d.isNamespace()) {
                    ns.push_back(d.getNamespace().name);
                    run(d.getNamespace().declarations, ns);
                    ns.pop_back();
                    out.push_back(d);
                } else if (d.isClass()) {
                    hoistClass(d, ns, {}, out);
                } else {
                    out.push_back(d);
                }
            }
            decls = std::move(out);
        }
    };

    struct Resolver {
        const Registry &reg;
        std::vector<std::string> ns;                 // current namespace
        std::vector<std::string> class_chain;        // original chain of the class being walked

        auto resolve(Symbol &s) const -> void {
            if (s.path.empty() || !allStrings(s)) return;
            const auto path = stringParts(s);
            for (std::size_t i = 0; i < path.size(); ++i) {
                for (std::size_t len = path.size() - i; len >= 1; --len) {
                    const std::vector<std::string> window(path.begin() + i, path.begin() + i + len);
                    const std::vector<std::string> prefix(path.begin(), path.begin() + i);
                    std::vector<std::size_t> hits;
                    if (const auto it = reg.by_chain.find(joinStrings(window, "::")); it != reg.by_chain.end())
                        for (const auto idx : it->second)
                            if (suffixOf(prefix, reg.entries[idx].ns)) hits.push_back(idx);
                    // relative to the enclosing classes: inside rule::value a bare STRING means rule::value::STRING
                    if (hits.empty() && i == 0)
                        for (std::size_t k = class_chain.size(); k >= 1 && hits.empty(); --k) {
                            std::vector<std::string> full(class_chain.begin(), class_chain.begin() + k);
                            full.insert(full.end(), window.begin(), window.end());
                            if (const auto it = reg.by_chain.find(joinStrings(full, "::")); it != reg.by_chain.end())
                                for (const auto idx : it->second)
                                    if (reg.entries[idx].ns == ns) hits.push_back(idx);
                        }
                    if (hits.size() != 1) {
                        if (len == 1) break;
                        continue;
                    }
                    std::vector<std::string> rewritten(path.begin(), path.begin() + i);
                    rewritten.push_back(reg.entries[hits.front()].flat);
                    rewritten.insert(rewritten.end(), path.begin() + i + len, path.end());
                    Symbol out = makeSymbol(rewritten.front());
                    out.path.clear();
                    for (auto &piece : rewritten) out.path.push_back(piece);
                    s = std::move(out);
                    return;
                }
            }
        }
    };

    auto flattenNested(Declarations &decls) -> void {
        Registry reg;
        std::vector<std::string> ns;
        Hoister{reg}.run(decls, ns);

        Resolver resolver{reg, {}, {}};
        IRWalker w;
        w.on_namespace = [&](Namespace &n, bool enter) {
            if (enter) resolver.ns.push_back(n.name); else resolver.ns.pop_back();
        };
        w.on_class = [&](Class &c, bool enter) {
            if (!enter) { resolver.class_chain.clear(); return; }
            const auto parts = stringParts(c.name);
            resolver.class_chain = parts.empty() ? std::vector<std::string>{} : reg.chainOfFlat(parts.back(), resolver.ns);
        };
        w.on_symbol = [&](Symbol &s, bool) { resolver.resolve(s); };
        w.decls(decls);
    }

    // ========================================================================
    // 3. qualify members
    // ========================================================================
    struct ClassInfo {
        std::unordered_set<std::string> fields, methods, static_methods;
        std::vector<std::string> bases;
    };
    using ClassTable = std::unordered_map<std::string, ClassInfo>;

    auto collectClasses(const Declarations &decls, ClassTable &table) -> void {
        for (const auto &d : decls) {
            if (d.isNamespace()) { collectClasses(d.getNamespace().declarations, table); continue; }
            if (!d.isClass()) continue;
            const auto &cls = d.getClass();
            const auto parts = stringParts(cls.name);
            if (parts.empty()) continue;
            ClassInfo info;
            for (const auto &entry : cls.data) {
                const auto &m = *entry.first;
                if (m.isVariable()) info.fields.insert(m.getVariable().name);
                else if (m.isFunction()) (m.getFunction().is_static ? info.static_methods : info.methods).insert(m.getFunction().name);
            }
            if (cls.to_str_fun) info.methods.insert(cls.to_str_fun->name);
            for (const auto &in : cls.inherit_members)
                if (std::holds_alternative<Symbol>(in.second)) {
                    const auto base = stringParts(std::get<Symbol>(in.second));
                    if (!base.empty()) info.bases.push_back(base.back());
                }
            table[parts.back()] = std::move(info);
        }
    }
    // own members plus everything inherited from bases that are defined in the IR (library bases are unknown)
    auto aggregate(const ClassTable &table, const std::string &name, std::unordered_set<std::string> &seen) -> ClassInfo {
        ClassInfo out;
        if (!seen.insert(name).second) return out;
        const auto it = table.find(name);
        if (it == table.end()) return out;
        out = it->second;
        for (const auto &b : it->second.bases) {
            const auto base = aggregate(table, b, seen);
            out.fields.insert(base.fields.begin(), base.fields.end());
            out.methods.insert(base.methods.begin(), base.methods.end());
            out.static_methods.insert(base.static_methods.begin(), base.static_methods.end());
        }
        return out;
    }

    auto qualifyMembers(Declarations &decls) -> void {
        ClassTable table;
        collectClasses(decls, table);

        std::optional<ClassInfo> current;
        IRWalker::FnKind kind = IRWalker::FnKind::Free;
        bool static_fn = false;
        std::vector<std::unordered_set<std::string>> scopes;
        const auto is_local = [&](const std::string &n) {
            for (const auto &s : scopes) if (s.contains(n)) return true;
            return false;
        };

        IRWalker w;
        w.scope_push = [&] { scopes.emplace_back(); };
        w.scope_pop = [&] { scopes.pop_back(); };
        w.on_local = [&](const std::string &n) { if (!scopes.empty()) scopes.back().insert(n); };
        w.on_class = [&](Class &c, bool enter) {
            if (!enter) { current.reset(); return; }
            const auto parts = stringParts(c.name);
            std::unordered_set<std::string> seen;
            current = parts.empty() ? ClassInfo{} : aggregate(table, parts.back(), seen);
        };
        w.on_function = [&](Function &f, IRWalker::FnKind k, bool enter) {
            kind = enter ? k : IRWalker::FnKind::Free;
            static_fn = enter && f.is_static;
        };
        w.on_symbol = [&](Symbol &s, bool in_type) {
            if (in_type || !current || s.path.empty()) return;
            if (kind != IRWalker::FnKind::Method && kind != IRWalker::FnKind::Constructor) return;
            if (!std::holds_alternative<std::string>(s.path.front())) return;
            const auto &n = std::get<std::string>(s.path.front());
            if (n == "self" || n == "this" || n == "Self" || n == "__ispa_parser__" || n == "__ispa_python_self" || is_local(n)) return;
            if (!static_fn && (current->fields.contains(n) || current->methods.contains(n)))
                s.path.insert(s.path.begin(), std::string(kind == IRWalker::FnKind::Constructor ? "this" : "self"));
            else if (current->static_methods.contains(n))
                s.path.insert(s.path.begin(), std::string("Self"));
        };
        w.decls(decls);
    }

    // ========================================================================
    // 4. call-site adaptation
    // ========================================================================
    enum class ParamKind { Value, Ref, Mut, Move };
    auto paramKind(const Type &t) -> ParamKind {
        if (!t.isValueType()) return ParamKind::Value;
        switch (t.getValueType()) {
            case ValueType::BorrowSemantic: return isImmutableWrapped(t) ? ParamKind::Ref : ParamKind::Mut;
            case ValueType::MoveSemantic: return ParamKind::Move;
            default: return ParamKind::Value;
        }
    }
    auto wrapCall(const char *name, Expression inner) -> Expression {
        FunctionCall fc;
        fc.name = std::make_shared<Symbol>(makeSymbol(name));
        fc.args.push_back(std::move(inner));
        Expression out;
        out.push_back(ExpressionValue{std::move(fc)});
        return out;
    }
    auto isPlace(const Expression &e) -> bool {
        return e.size() == 1 && e[0].type() == ExpressionValueType::RValue &&
               (e[0].getRValue().isSymbol() || e[0].getRValue().isStorageSymbol());
    }
    auto collectFunctions(const Declarations &decls, std::unordered_map<std::string, std::vector<const Function *>> &table) -> void {
        for (const auto &d : decls) {
            if (d.isNamespace()) collectFunctions(d.getNamespace().declarations, table);
            else if (d.isFunction()) table[d.getFunction().name].push_back(&d.getFunction());
            else if (d.isClass()) {
                for (const auto &entry : d.getClass().data)
                    if (entry.first->isFunction()) table[entry.first->getFunction().name].push_back(&entry.first->getFunction());
                if (d.getClass().to_str_fun) table[d.getClass().to_str_fun->name].push_back(&*d.getClass().to_str_fun);
            }
        }
    }

    auto adaptCallSites(Declarations &decls) -> void {
        // Overloaded names are skipped: without expression types the right overload cannot be picked.
        std::unordered_map<std::string, std::vector<const Function *>> table;
        const Declarations snapshot = decls;                    // function signatures are read from a stable copy
        collectFunctions(snapshot, table);

        IRWalker w;
        w.on_call = [&](FunctionCall &c) {
            if (!std::holds_alternative<std::shared_ptr<Symbol>>(c.name)) return;
            const auto &sym = *std::get<std::shared_ptr<Symbol>>(c.name);
            const auto parts = stringParts(sym);
            if (parts.empty() || parts.back().starts_with("__ispa_rust_")) return;
            const auto it = table.find(parts.back());
            if (it == table.end() || it->second.size() != 1) return;
            const auto &f = *it->second.front();
            if (f.parameters.size() != c.args.size()) return;
            for (std::size_t i = 0; i < c.args.size(); ++i) {
                switch (paramKind(f.parameters[i].first)) {
                    case ParamKind::Ref: c.args[i] = wrapCall(RustCore::kRefWrapper, c.args[i]); break;
                    case ParamKind::Mut: c.args[i] = wrapCall(RustCore::kMutWrapper, c.args[i]); break;
                    case ParamKind::Value: if (isPlace(c.args[i])) c.args[i] = wrapCall(RustCore::kCloneWrapper, c.args[i]); break;
                    case ParamKind::Move: break;
                }
            }
        };
        w.decls(decls);
    }
}

auto Rust::Rewriter::rewrite(const LangAPI::Declarations &source) -> LangAPI::Declarations {
    LangAPI::Declarations program = source;      // top level is a value copy; shared parts are copied by the passes
    mergeNamespaces(program);
    flattenNested(program);                      // also replaces every class's shared member list with fresh copies
    qualifyMembers(program);
    adaptCallSites(program);
    return program;
}
