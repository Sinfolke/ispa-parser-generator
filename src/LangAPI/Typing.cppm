export module LangAPI.Typing;

import std;
import LangAPI;

// Shared static typing for the lowered IR. The IR itself is untyped at expression level (an Expression is a flat list of
// operands and operators), so every backend used to re-derive types ad hoc (or not at all and emit text as written).
// A Typer indexes the declarations once and answers "what is the type of this expression" in a scope that the backend
// drives while it walks a function body (beginFunction / pushScope / declare / popScope). Answers are best effort:
// std::nullopt means "unknown" and backends must degrade (cast, report) instead of guessing.
export namespace LangAPI::Typing {
    using MaybeType = std::optional<Type>;

    class Typer {
    public:
        explicit Typer(const Declarations &decls) { index(decls, {}); }

        // ---- scope driving
        auto beginFunction(const Class *owner, const Function &fn) -> void {
            locals.clear();
            cur_class = owner;
            pushScope();
            for (const auto &[t, n] : fn.parameters) declare(n, t);
        }
        auto pushScope() -> void { locals.emplace_back(); }
        auto popScope() -> void { if (!locals.empty()) locals.pop_back(); }
        auto declare(const std::string &name, const Type &type) -> void {
            if (locals.empty()) pushScope();
            locals.back()[name] = type;
        }
        auto currentClass() const -> const Class * { return cur_class; }
        auto setLocalType(const std::string &name, const Type &type) -> void {
            for (auto it = locals.rbegin(); it != locals.rend(); ++it)
                if (const auto f = it->find(name); f != it->end()) { f->second = type; return; }
            declare(name, type);
        }

        // ---- queries
        static auto strip(const Type &t) -> Type {
            auto cur = t;
            while (cur.isValueType() && !cur.template_parameters.empty()) {
                const auto vt = cur.getValueType();
                if (vt != ValueType::Box && vt != ValueType::ImmutableSemantic && vt != ValueType::MoveSemantic &&
                    vt != ValueType::BorrowSemantic) break;
                const auto *inner = std::get_if<Type>(&cur.template_parameters[0]);
                if (!inner) break;
                auto next = *inner;
                cur = std::move(next);
            }
            return cur;
        }
        static auto isValue(const Type &t, ValueType v) -> bool { const auto s = strip(t); return s.isValueType() && s.getValueType() == v; }
        // element type of Array<T> / value type of Map<K,V> / Char of String
        static auto elementType(const Type &container) -> MaybeType {
            const auto s = strip(container);
            if (!s.isValueType()) return std::nullopt;
            const auto n = s.template_parameters.size();
            const auto pick = [&](std::size_t i) -> MaybeType {
                if (i < n) if (const auto *t = std::get_if<Type>(&s.template_parameters[i])) return *t;
                return std::nullopt;
            };
            switch (s.getValueType()) {
                case ValueType::Array: case ValueType::FixedSizeArray: case ValueType::Span: return pick(0);
                case ValueType::Map: return pick(1);
                case ValueType::String: case ValueType::NonOwnedString: return Type(ValueType::Char);
                default: return std::nullopt;
            }
        }

        auto classOf(const Type &t) const -> const Class * {
            const auto s = strip(t);
            return s.isSymbol() ? findClass(s.getSymbol()) : nullptr;
        }
        auto findClass(const Symbol &sym) const -> const Class * {
            const auto parts = literalParts(sym);
            if (parts.empty()) return nullptr;
            const auto it = classes_by_bare.find(parts.back());
            if (it == classes_by_bare.end()) return nullptr;
            const Class *best = nullptr;
            for (const auto &e : it->second) {
                if (suffixMatches(e.full, parts)) { if (!best || e.full.size() < best_len(best)) best = e.cls; }
            }
            if (!best && !it->second.empty() && parts.size() == 1) best = it->second.front().cls;
            return best;
        }
        // method `name` of the class or of one of its bases
        auto findMethod(const Class &cls, const std::string &name) const -> const Function * {
            for (const auto &[decl, vis] : cls.data)
                if (decl->isFunction() && decl->getFunction().name == name) return &decl->getFunction();
            for (const auto &[vis, base] : cls.inherit_members)
                if (const auto *s = std::get_if<Symbol>(&base))
                    if (const auto *b = findClass(*s); b && b != &cls)
                        if (const auto *m = findMethod(*b, name)) return m;
            return nullptr;
        }
        // class that declares `name` (cls itself or a base)
        auto methodOwner(const Class &cls, const std::string &name) const -> const Class * {
            for (const auto &[decl, vis] : cls.data)
                if (decl->isFunction() && decl->getFunction().name == name) return &cls;
            for (const auto &[vis, base] : cls.inherit_members)
                if (const auto *s = std::get_if<Symbol>(&base))
                    if (const auto *b = findClass(*s); b && b != &cls)
                        if (const auto *o = methodOwner(*b, name)) return o;
            return nullptr;
        }
        auto fieldType(const Class &cls, const std::string &name) const -> MaybeType {
            for (const auto &[decl, vis] : cls.data)
                if (decl->isVariable() && decl->getVariable().name == name) return decl->getVariable().type;
            for (const auto &[vis, base] : cls.inherit_members)
                if (const auto *s = std::get_if<Symbol>(&base))
                    if (const auto *b = findClass(*s); b && b != &cls)
                        if (const auto t = fieldType(*b, name)) return t;
            return std::nullopt;
        }
        auto classType(const Class &cls) const -> Type { return Type(cls.name); }

        auto typeOf(const Expression &e) const -> MaybeType {
            using EE = ExpressionElement;
            MaybeType first;
            bool have_operand = false, boolean = false, arithmetic = false;
            for (const auto &v : e) {
                if (v.isExpressionElement()) {
                    switch (v.getExpressionElement()) {
                        case EE::And: case EE::Or: case EE::Not: case EE::Equal: case EE::NotEqual: case EE::Higher:
                        case EE::Lower: case EE::HigherOrEqual: case EE::LowerOrEqual: boolean = true; break;
                        case EE::Add: case EE::Minus: case EE::Multiply: case EE::Divide: case EE::Modulo: arithmetic = true; break;
                        default: break;
                    }
                    continue;
                }
                MaybeType t;
                if (v.isRvalue()) t = typeOf(v.getRValue());
                else if (v.isFunctionCall()) t = typeOf(v.getFunctionCall());
                else if (v.type() == ExpressionValueType::Return || v.type() == ExpressionValueType::Break ||
                         v.type() == ExpressionValueType::Continue) return std::nullopt;
                if (!have_operand) { first = t; have_operand = true; }
            }
            if (boolean) return Type(ValueType::Bool);
            (void)arithmetic;
            return first;
        }

        auto typeOf(const RValue &r) const -> MaybeType {
            if (r.isInt()) return Type(ValueType::Int);
            if (r.isBool()) return Type(ValueType::Bool);
            if (r.isFloat()) return Type(ValueType::Float);
            if (r.isChar()) return Type(ValueType::Char);
            if (r.isString()) return Type(ValueType::String);
            if (r.type() == RValueType::Instance) return cur_class ? MaybeType(classType(*cur_class)) : std::nullopt;
            if (r.type() == RValueType::GetFromBox) {
                const auto t = typeOf(r.getGetFromBox().what);
                return t ? MaybeType(strip(*t)) : std::nullopt;
            }
            if (r.type() == RValueType::Cast) {
                const auto &c = r.getCast();
                return c.type ? MaybeType(*c.type) : std::nullopt;
            }
            if (r.type() == RValueType::ToString || r.type() == RValueType::CharToStringConstructor) return Type(ValueType::String);
            if (r.type() == RValueType::CheckVariant) return Type(ValueType::Bool);
            if (r.isSymbol()) return typeOfSymbol(r.getSymbol());
            if (r.isStorageSymbol()) return typeOfStorageImpl(r.getStorageSymbol());
            return std::nullopt;
        }

        auto typeOfStorage(const StorageSymbol &st) const -> MaybeType { return typeOfStorageImpl(st); }
        auto typeOf(const FunctionCall &c) const -> MaybeType {
            const auto *sym = std::get_if<std::shared_ptr<Symbol>>(&c.name);
            if (!sym || !*sym) return std::nullopt;
            const auto parts = literalParts(**sym);
            if (parts.empty()) return std::nullopt;
            if (parts.size() == 1 && cur_class)
                if (const auto *m = findMethod(*cur_class, parts[0])) return m->type;
            if (const auto *cls = findClass(**sym)) return classType(*cls);   // constructor call
            if (const auto *fn = findFunction(parts)) return fn->type;
            return std::nullopt;
        }

    private:
        struct ClassEntry { std::string full; const Class *cls; };
        std::unordered_map<std::string, std::vector<ClassEntry>> classes_by_bare;
        std::unordered_map<std::string, std::vector<std::pair<std::string, const Function *>>> functions_by_bare;
        std::unordered_map<std::string, std::vector<std::pair<std::string, const Variable *>>> globals_by_bare;
        std::unordered_map<std::string, std::vector<std::pair<std::string, const Enum *>>> enums_by_bare;
        std::vector<std::unordered_map<std::string, Type>> locals;
        const Class *cur_class = nullptr;

        static auto join(const std::vector<std::string> &v) -> std::string {
            std::string out;
            for (std::size_t i = 0; i < v.size(); ++i) { if (i) out += "::"; out += v[i]; }
            return out;
        }
        static auto literalParts(const Symbol &s) -> std::vector<std::string> {
            std::vector<std::string> parts;
            for (const auto &p : s.path) {
                const auto *str = std::get_if<std::string>(&p);
                if (!str) return {};
                parts.push_back(*str);
            }
            return parts;
        }
        static auto split(const std::string &full) -> std::vector<std::string> {
            std::vector<std::string> out; std::size_t i = 0;
            while (true) {
                const auto j = full.find("::", i);
                out.push_back(full.substr(i, j == std::string::npos ? j : j - i));
                if (j == std::string::npos) break;
                i = j + 2;
            }
            return out;
        }
        static auto suffixMatches(const std::string &full, const std::vector<std::string> &parts) -> bool {
            const auto f = split(full);
            if (parts.size() > f.size()) return false;
            return std::equal(parts.rbegin(), parts.rend(), f.rbegin());
        }
        static auto best_len(const Class *) -> std::size_t { return 0; }

        auto index(const Declarations &decls, std::vector<std::string> ns) -> void {
            for (const auto &d : decls) {
                if (d.isNamespace()) {
                    auto inner = ns; inner.push_back(d.getNamespace().name);
                    index(d.getNamespace().declarations, std::move(inner));
                } else if (d.isClass()) {
                    auto parts = ns;
                    for (const auto &p : d.getClass().name.path) if (const auto *s = std::get_if<std::string>(&p)) parts.push_back(*s);
                    classes_by_bare[parts.back()].push_back({join(parts), &d.getClass()});
                } else if (d.isFunction()) {
                    auto parts = ns; parts.push_back(d.getFunction().name);
                    functions_by_bare[d.getFunction().name].emplace_back(join(parts), &d.getFunction());
                } else if (d.isVariable()) {
                    auto parts = ns; parts.push_back(d.getVariable().name);
                    globals_by_bare[d.getVariable().name].emplace_back(join(parts), &d.getVariable());
                } else if (d.isEnum()) {
                    auto parts = ns; parts.push_back(d.getEnum().name);
                    enums_by_bare[d.getEnum().name].emplace_back(join(parts), &d.getEnum());
                }
            }
        }
        auto findFunction(const std::vector<std::string> &parts) const -> const Function * {
            const auto it = functions_by_bare.find(parts.back());
            if (it == functions_by_bare.end()) return nullptr;
            for (const auto &[full, fn] : it->second) if (suffixMatches(full, parts)) return fn;
            return nullptr;
        }
        auto lookupLocal(const std::string &n) const -> const Type * {
            for (auto it = locals.rbegin(); it != locals.rend(); ++it)
                if (const auto f = it->find(n); f != it->end()) return &f->second;
            return nullptr;
        }

        auto typeOfSymbol(const Symbol &s) const -> MaybeType {
            const auto parts = literalParts(s);
            if (parts.empty()) return std::nullopt;
            if (parts.size() == 1) {
                const auto &n = parts[0];
                if (const auto *t = lookupLocal(n)) return *t;
                if (n == "self") return cur_class ? MaybeType(classType(*cur_class)) : std::nullopt;
                if (cur_class) if (const auto t = fieldType(*cur_class, n)) return t;
            }
            if (const auto it = globals_by_bare.find(parts.back()); it != globals_by_bare.end())
                for (const auto &[full, v] : it->second) if (suffixMatches(full, parts)) return v->type;
            if (parts.size() >= 2)   // Enum::Item
                if (const auto it = enums_by_bare.find(parts[parts.size() - 2]); it != enums_by_bare.end()) {
                    auto prefix = std::vector<std::string>(parts.begin(), parts.end() - 1);
                    for (const auto &[full, en] : it->second)
                        if (suffixMatches(full, prefix)) return Type(Symbol(prefix));
                }
            return std::nullopt;
        }

        auto typeOfStorageImpl(const StorageSymbol &st) const -> MaybeType {
            MaybeType cur;
            if (!st.what.empty()) cur = typeOf(st.what);
            else if (cur_class) cur = classType(*cur_class);
            for (const auto &part : st.path) {
                if (!cur) return std::nullopt;
                const auto *name = std::get_if<std::string>(&part);
                if (!name) if (const auto *p = std::get_if<Property>(&part)) name = &p->name;
                if (name) {
                    const auto *cls = classOf(*cur);
                    if (!cls) return std::nullopt;
                    cur = fieldType(*cls, *name);
                } else if (std::holds_alternative<StorageOffset>(part)) {
                    cur = elementType(*cur);
                } else if (const auto *call = std::get_if<FunctionCall>(&part)) {
                    const auto *sym = std::get_if<std::shared_ptr<Symbol>>(&call->name);
                    const auto *cls = classOf(*cur);
                    if (!sym || !*sym || !cls) return std::nullopt;
                    const auto parts = literalParts(**sym);
                    const auto *m = parts.size() == 1 ? findMethod(*cls, parts[0]) : nullptr;
                    cur = m ? MaybeType(m->type) : std::nullopt;
                } else if (const auto *am = std::get_if<ArrayMethodCall>(&part)) {
                    switch (am->method) {
                        case ArrayMethods::Size: case ArrayMethods::PythonCount: case ArrayMethods::PythonIndex: cur = Type(ValueType::Int); break;
                        case ArrayMethods::PythonContains: cur = Type(ValueType::Bool); break;
                        case ArrayMethods::Pop: case ArrayMethods::PythonPop: case ArrayMethods::PythonGet: case ArrayMethods::SequencePopBack:
                            cur = elementType(*cur); break;
                        case ArrayMethods::ShallowCopy: break;      // same type
                        default: cur = std::nullopt;
                    }
                } else return std::nullopt;
            }
            return cur;
        }
    };
}
