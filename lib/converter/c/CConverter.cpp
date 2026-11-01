module C.Init;

import Converter.Writer;
import logging;
import std;
import LangAPI.Traverse;

namespace C {
    struct Unsupported { std::string why; };
    namespace {
        auto joined(const std::vector<std::string> &items, const std::string &sep) -> std::string {
            std::string out;
            for (std::size_t i = 0; i < items.size(); ++i) { if (i) out += sep; out += items[i]; }
            return out;
        }
        auto cIdent(std::string s) -> std::string {
            for (auto &c : s) if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') c = '_';
            static const std::unordered_set<std::string> reserved = {
                "auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else", "enum", "extern",
                "float", "for", "goto", "if", "inline", "int", "long", "register", "restrict", "return", "short", "signed",
                "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while", "bool",
                "true", "false", "NULL"};
            if (reserved.contains(s)) s += "_";
            return s.empty() ? "_" : s;
        }
        auto symbolParts(const LangAPI::Symbol &symbol) -> std::vector<std::string> {
            std::vector<std::string> parts;
            for (const auto &p : symbol.path)
                if (const auto *s = std::get_if<std::string>(&p)) {
                    if (parts.empty() && (*s == "_" || s->empty()) && symbol.path.size() > 1) continue;     // `_::a::b`: rooted path
                    parts.push_back(cIdent(*s));
                } else parts.push_back("_dyn");
            return parts;
        }
        auto symbolName(const LangAPI::Symbol &symbol) -> std::string { return joined(symbolParts(symbol), "_"); }
    }

    auto Construct::prefix() const -> std::string {
        if (scope.empty()) return cIdent(std::string(namespace_name));
        std::vector<std::string> parts;
        for (const auto &s : scope) parts.push_back(cIdent(s));
        return joined(parts, "_");
    }

    namespace {
        auto definesMethod(const LangAPI::Class &c, const std::string &m) -> const LangAPI::Function * {
            for (const auto &[decl, vis] : c.data)
                if (decl->isFunction() && decl->getFunction().name == m) return &decl->getFunction();
            return nullptr;
        }
    }

    // A method is virtual iff a class redefines a method that an ancestor defines. Slots live in the topmost definer.
    auto Construct::analyzeVirtuals() -> void {
        slot_root.clear(); root_slots.clear();
        const auto parentWith = [&](auto &&self, const LangAPI::Class &c, const std::string &m) -> const LangAPI::Class * {
            for (const auto &[vis, base] : c.inherit_members)
                if (const auto *bs = std::get_if<LangAPI::Symbol>(&base))
                    if (const auto *b = typer->findClass(*bs); b && b != &c) {
                        if (definesMethod(*b, m)) return b;
                        if (const auto *r = self(self, *b, m)) return r;
                    }
            return nullptr;
        };
        for (const auto &[cls, cname] : class_cname) {
            if (!cls->template_parameters.empty()) continue;
            for (const auto &[decl, vis] : cls->data) {
                if (!decl->isFunction() || decl->getFunction().is_static || decl->getFunction().name == "__init__") continue;
                const auto &m = decl->getFunction().name;
                std::vector<const LangAPI::Class *> chain = {cls};
                while (const auto *up = parentWith(parentWith, *chain.back(), m)) chain.push_back(up);
                if (chain.size() < 2) continue;
                for (const auto *k : chain) slot_root[{k, m}] = chain.back();
            }
        }
        // Extension points of the library: generated lexers/parsers override these, so the library build (which does not see
        // the generated classes) must give them slots. Found by running every core-parity grammar in application mode.
        static const std::map<std::string, std::vector<std::string>> open_methods = {
            {"python_ispa_std_lexer_Lexer_base", {"clone", "init", "makeToken", "on_eof", "on_token"}},
            {"python_ispa_std_parser_base_Parser_base", {"lazyParse", "parseFromTokens"}},
            {"python_ispa_std_raw_token_RawToken", {"to_node"}},
        };
        for (const auto &[cls, cname] : class_cname) {
            const auto it = open_methods.find(cname);
            if (it == open_methods.end()) continue;
            for (const auto &m : it->second)
                if (definesMethod(*cls, m) && !slot_root.contains({cls, m})) slot_root[{cls, m}] = cls;
        }
        for (const auto &[key, root] : slot_root)
            if (key.first == root) root_slots[root].push_back(key.second);
        for (auto &[root, names] : root_slots) {       // declaration order
            std::vector<std::string> ordered;
            for (const auto &[decl, vis] : root->data)
                if (decl->isFunction() && std::ranges::find(names, decl->getFunction().name) != names.end()) ordered.push_back(decl->getFunction().name);
            names = std::move(ordered);
        }
    }

    auto Construct::pathToBase(const LangAPI::Class &from, const LangAPI::Class *target) const -> std::optional<std::vector<std::string>> {
        if (&from == target) return std::vector<std::string>{};
        for (const auto &[vis, base] : from.inherit_members)
            if (const auto *bs = std::get_if<LangAPI::Symbol>(&base))
                if (const auto *b = typer->findClass(*bs); b && b != &from)
                    if (auto r = pathToBase(*b, target)) { r->insert(r->begin(), "base_" + symbolName(*bs)); return r; }
        return std::nullopt;
    }

    // <ns>_get_<Class>_handler for a class (its C name is <ns>_<Class>, the handler name follows emitClass)
    auto Construct::handlerName(const LangAPI::Class &cls) const -> std::string {
        const auto it = handler_names.find(&cls);
        if (it != handler_names.end()) return it->second;
        throw Unsupported{"no handler for a class"};
    }

    auto Construct::registerClass(const LangAPI::Class &c) -> void {
        const auto n = prefix() + "_" + symbolName(c.name);
        class_cname[&c] = n;
        handler_names[&c] = prefix() + "_get_" + symbolName(c.name) + "_handler";
        defined.insert(n);
        if (!c.template_parameters.empty()) { generic_classes.insert(n); generic_decl[n] = &c; generic_scope[n] = scope; }
        ClassInfo info; info.c_name = n;
        for (const auto &[vis, base] : c.inherit_members)
            if (const auto *bs = std::get_if<LangAPI::Symbol>(&base)) { info.bases.emplace_back(symbolName(*bs), symbolName(*bs)); }
        for (const auto &[decl, vis] : c.data) {
            if (decl->isVariable() && decl->getVariable().is_static) functions.insert(n + "_" + cIdent(decl->getVariable().name));
            else if (decl->isVariable()) info.fields.insert(cIdent(decl->getVariable().name));
            else if (decl->isFunction()) { info.methods[decl->getFunction().name] = &decl->getFunction(); functions.insert(n + "_" + cIdent(decl->getFunction().name)); }
        }
        classes[n] = std::move(info);
    }

    namespace {
        auto mangle(const std::string &s) -> std::string {
            std::string m;
            for (const char c : s) m += std::isalnum(static_cast<unsigned char>(c)) ? c : (c == '*' ? 'P' : '_');
            return m;
        }
        template<class Root>
        auto substitute(Root &root, const std::map<std::string, LangAPI::Type> &sub) -> void {
            LangAPI::Traverse::unshare_shared = true;
            LangAPI::Traverse::walk(root, [](auto &) {});
            LangAPI::Traverse::unshare_shared = false;
            LangAPI::Traverse::forEach<LangAPI::Type>(root, [&](LangAPI::Type &t) {
                if (!t.isSymbol() || !t.template_parameters.empty() || t.getSymbol().path.size() != 1) return;
                if (const auto *n = std::get_if<std::string>(&t.getSymbol().path[0]))
                    if (const auto it = sub.find(*n); it != sub.end()) { auto r = it->second; t = std::move(r); }
            });
        }
    }

    auto Construct::instantiateClass(const LangAPI::Class &g, const std::vector<LangAPI::Type> &args) -> const LangAPI::Class * {
        std::vector<std::string> d, cargs;
        for (const auto &a : args) cargs.push_back(typeName(a, d));
        const auto gname = class_cname.at(&g);
        const auto key = gname + "<" + joined(cargs, ",") + ">";
        if (const auto it = class_insts.find(key); it != class_insts.end()) return it->second;
        class_clones.push_back(g);
        auto &c = class_clones.back();
        std::map<std::string, LangAPI::Type> sub;
        for (std::size_t i = 0; i < args.size() && i < g.template_parameters.size(); ++i) sub.emplace(g.template_parameters[i], args[i]);
        substitute(c, sub);
        if (auto *last = std::get_if<std::string>(&c.name.path.back())) *last += "__" + mangle(joined(cargs, "_"));
        c.template_parameters.clear();
        class_insts[key] = &c;
        auto saved = scope;
        scope = generic_scope.at(gname);
        registerClass(c);
        typer->addClass(c, scope);
        const auto n = class_cname.at(&c);
        fwd_w.writeln("typedef struct {} {};", n, n);
        pending.push_back({&c, nullptr, scope});
        scope = std::move(saved);
        return &c;
    }

    auto Construct::instantiateFunction(const LangAPI::Function &g, const std::vector<LangAPI::Type> &args) -> const LangAPI::Function * {
        std::vector<std::string> d, cargs;
        for (const auto &a : args) cargs.push_back(typeName(a, d));
        const auto gscope = generic_fn_scope.at(&g);
        const auto key = joined(gscope, "::") + "::" + g.name + "<" + joined(cargs, ",") + ">";
        if (const auto it = fn_insts.find(key); it != fn_insts.end()) return it->second;
        fn_clones.push_back(g);
        auto &f = fn_clones.back();
        std::map<std::string, LangAPI::Type> sub;
        for (std::size_t i = 0; i < args.size() && i < g.template_parameters.size(); ++i) sub.emplace(g.template_parameters[i].first, args[i]);
        substitute(f, sub);
        f.name += "__" + mangle(joined(cargs, "_"));
        f.template_parameters.clear();
        fn_insts[key] = &f;
        typer->addFunction(f, gscope);
        auto saved = scope;
        scope = gscope;
        functions.insert(prefix() + "_" + cIdent(f.name));
        pending.push_back({nullptr, &f, scope});
        scope = std::move(saved);
        return &f;
    }

    auto Construct::collect(const LangAPI::Declarations &decls) -> void {
        for (const auto &d : decls) {
            if (d.isNamespace()) {
                scope.push_back(d.getNamespace().name);
                collect(d.getNamespace().declarations);
                scope.pop_back();
            } else if (d.isClass()) registerClass(d.getClass());
            else if (d.isEnum()) {
                defined.insert(prefix() + "_" + cIdent(d.getEnum().name));
                for (const auto &v : d.getEnum().value) functions.insert(prefix() + "_" + cIdent(d.getEnum().name) + "_" + cIdent(v));
            } else if (d.isFunction()) {
                if (!d.getFunction().template_parameters.empty()) generic_fn_scope[&d.getFunction()] = scope;
                else functions.insert(prefix() + "_" + cIdent(d.getFunction().name));
            } else if (d.isVariable()) functions.insert(prefix() + "_" + cIdent(d.getVariable().name));
            else if (d.isTypeAlias()) defined.insert(prefix() + "_" + cIdent(d.getTypeAlias().name));
        }
    }

    // Symbols arrive qualified (python::ispa_std::lexer::TokenCursor) or relative to the current scope: try the
    // enclosing scopes innermost first, then the path as written.
    auto Construct::resolve(const LangAPI::Symbol &symbol, std::vector<std::string> &diag) const -> std::string {
        const auto parts = symbolParts(symbol);
        for (std::size_t k = scope.size() + 1; k-- > 0;) {
            std::vector<std::string> cand;
            if (k == 0 && scope.empty()) cand.push_back(cIdent(std::string(namespace_name)));
            for (std::size_t i = 0; i < k; ++i) cand.push_back(cIdent(scope[i]));
            cand.insert(cand.end(), parts.begin(), parts.end());
            if (const auto n = joined(cand, "_"); defined.contains(n)) return n;
        }
        if (const auto n = joined(parts, "_"); defined.contains(n)) return n;
        diag.push_back("unresolved symbol " + joined(parts, "::"));
        return joined(parts, "_");
    }

    // pointer-to-class conversions between different classes (derived -> base: the base struct is the first member)
    auto Construct::coerce(const std::string &code, const LangAPI::Typing::MaybeType &from, const LangAPI::Type &to) const -> std::string {
        if (LangAPI::Typing::Typer::isValue(to, LangAPI::ValueType::Any)) {
            if (code == "NULL") return "ISPA_ANY_NONE";
            if (!from) return code;
            if (LangAPI::Typing::Typer::isValue(*from, LangAPI::ValueType::Any)) return code;
            const auto sf = LangAPI::Typing::Typer::strip(*from);
            if (sf.isValueType()) switch (sf.getValueType()) {
                case LangAPI::ValueType::Int: case LangAPI::ValueType::LargeUInt: case LangAPI::ValueType::Bool: case LangAPI::ValueType::Char:
                    return "ISPA_ANY_INT(" + code + ")";
                case LangAPI::ValueType::String: case LangAPI::ValueType::NonOwnedString: return "ispa_any_str(" + code + ")";
                default: return "ISPA_ANY_PTR(" + code + ")";
            }
            return "ISPA_ANY_PTR(" + code + ")";
        }
        if (!from) return code;
        const auto *cf = typer->classOf(*from), *ct = typer->classOf(to);
        if (!cf || !ct || cf == ct) return code;
        const auto it = class_cname.find(ct);
        return it == class_cname.end() ? code : "(" + it->second + "*)" + code;
    }

    auto Construct::vecName(const std::string &elem) const -> std::string {
        std::string m;
        for (const char c : elem) m += std::isalnum(static_cast<unsigned char>(c)) ? c : (c == '*' ? 'P' : '_');
        const auto name = "ispa_vec_" + m;
        if (vec_seen.insert(name).second) vec_defs.emplace_back(name, elem);
        return name;
    }

    // C type of a lowered type. Anything without a direct C spelling is reported and degraded to a void pointer so the
    // header still compiles and the survey counts it.
    auto Construct::typeName(const LangAPI::Type &type, std::vector<std::string> &diag) -> std::string {
        using VT = LangAPI::ValueType;
        const auto inner = [&](std::size_t i) -> std::string {
            if (i < type.template_parameters.size())
                if (const auto *t = std::get_if<LangAPI::Type>(&type.template_parameters[i])) return typeName(*t, diag);
            diag.push_back("missing type argument");
            return "void*";
        };
        if (type.isValueType()) {
            switch (type.getValueType()) {
                case VT::Void: return "void";
                case VT::Char: return "char";
                case VT::Int: return "int64_t";
                case VT::LargeUInt: return "uint64_t";
                case VT::Bool: return "bool";
                case VT::Float: return "double";
                case VT::String: case VT::NonOwnedString: return "ispa_str";
                case VT::Array: return vecName(inner(0)) + "*";
                case VT::Map: return "ispa_map";
                case VT::Callable: return "ispa_fn";
                case VT::Any: return "ispa_any";
                // Box<T>: an arena-owned object. Classes are already handles (T*), plain values get a pointer.
                case VT::Box: { auto t = inner(0); return t.ends_with('*') ? t : t + "*"; }
                // Immutable / Move: by value (classes stay handles); Borrow: by reference (a pointer to a value)
                case VT::ImmutableSemantic: case VT::MoveSemantic: return inner(0);
                case VT::BorrowSemantic: { auto t = inner(0); return t.ends_with('*') || t == "ispa_str" ? t : t + "*"; }
                default: diag.push_back("type " + std::to_string(static_cast<int>(type.getValueType())) + " has no C spelling"); return "void*";
            }
        }
        if (type.isSymbol()) {
            auto n = resolve(type.getSymbol(), diag);
            if (!defined.contains(n)) return "void*";   // unresolved (a generic parameter): already reported by resolve
            if (generic_classes.contains(n)) {
                const auto *g = generic_decl.at(n);
                std::vector<LangAPI::Type> targs;
                for (const auto &tp : type.template_parameters)
                    if (const auto *x = std::get_if<LangAPI::Type>(&tp)) targs.push_back(*x);
                if (targs.size() != g->template_parameters.size()) { diag.push_back("generic class " + n + " used without its type arguments"); return "void*"; }
                return class_cname.at(instantiateClass(*g, targs)) + "*";
            }
            return n + "*";
        }
        diag.push_back("library type has no C spelling yet");
        return "void*";
    }


    // ---------------------------------------------------------------- bodies
    auto Construct::isLocal(const std::string &n) const -> bool {
        return std::ranges::any_of(locals, [&](const auto &f) { return f.contains(n); });
    }
    // `self->base_X.field` for a field of the class or of one of its (embedded) bases
    auto Construct::memberPath(const std::string &cls, const std::string &name) const -> std::optional<std::string> {
        const auto it = classes.find(cls);
        if (it == classes.end()) return std::nullopt;
        if (it->second.fields.contains(cIdent(name))) return cIdent(name);
        for (const auto &[base_c, member] : it->second.bases) {
            // bases are recorded by bare name: find the resolved class in the same scope
            std::vector<std::string> d;
            const auto b = resolve(LangAPI::Symbol(base_c), d);
            if (const auto r = memberPath(b, name)) return "base_" + member + "." + *r;
        }
        return std::nullopt;
    }

    static auto cString(const std::string &raw) -> std::string {
        std::string out = "\"";
        for (const unsigned char c : raw) {
            switch (c) {
                case '\\': out += "\\\\"; break;
                case '"': out += "\\\""; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if (c < 32 || c >= 127) { char b[8]; std::snprintf(b, sizeof b, "\\x%02x\"\"", c); out += b; }
                    else out += static_cast<char>(c);
            }
        }
        return out + "\"";
    }

    // IspaLib symbols (types and base classes of generated code) become the lowered library classes they stand for, so the
    // generated classes see ordinary declarations. Node<T>/MatchResult<T> payloads are type-erased (ispa_any): the
    // template arguments are dropped.
    namespace {
        auto libPath(LangAPI::StdlibExports e) -> std::optional<std::vector<std::string>> {
            using S = LangAPI::StdlibExports;
            const auto P = [](std::initializer_list<const char *> l) { std::vector<std::string> v = {"python", "ispa_std"}; for (auto x : l) v.emplace_back(x); return v; };
            switch (e) {
                case S::Node: return P({"node", "Node"});
                case S::MatchResult: return P({"match_result", "MatchResult"});
                case S::Lexer: return P({"lexer", "Lexer_base"});
                case S::Parser: return P({"parser_ll", "LLParser"});
                case S::LRParser: return P({"lr", "LRParser"});
                case S::LRGeneralizedParser: return P({"lr_conflicts", "LRGeneralizedParser"});
                case S::LexerMakeTokenParameter: return P({"refs", "Cursor"});
                case S::ParserFunctionParameter: case S::LexerTokenIterator: case S::LexerLazyIterator: return P({"lexer", "TokenCursor"});
                case S::DfaCaptures: return P({"dfa_api", "Captures"});
                case S::TdfaLayout: return P({"dfa_api", "TdfaLayout"});
                case S::DFADebug: return P({"dfa_api", "DFADebug"});
                case S::ASTPrinter: return P({"ast_printer", "ASTPrinter"});
                case S::RecoveryDecision: return P({"recovery", "Decision"});
                case S::RecoveryState: return P({"recovery", "State"});
                case S::RecoveryHandler: return P({"recovery", "Handler"});
                default: return std::nullopt;
            }
        }
        auto libType(const LangAPI::IspaLibSymbol &s) -> std::optional<LangAPI::Type> {
            using S = LangAPI::StdlibExports;
            const auto I = LangAPI::Type(LangAPI::ValueType::Int);
            switch (s.exports) {
                case S::DfaState: case S::DfaClassTable: return LangAPI::Type(LangAPI::ValueType::Array, I);
                case S::DfaTable: case S::DfaAcceptTable: case S::DfaLRTable: return LangAPI::Type(LangAPI::ValueType::Array, LangAPI::Type(LangAPI::ValueType::Array, I));
                case S::LRValue: return LangAPI::Type(LangAPI::ValueType::Any);
                default: break;
            }
            if (const auto p = libPath(s.exports)) return LangAPI::Type(LangAPI::Symbol(*p));
            return std::nullopt;
        }
    }

    auto Construct::staticOwner(const LangAPI::Class &cls, const std::string &name) const -> const LangAPI::Class * {
        for (const auto &[decl, vis] : cls.data)
            if (decl->isVariable() && decl->getVariable().is_static && decl->getVariable().name == name) return &cls;
        for (const auto &[vis, base] : cls.inherit_members)
            if (const auto *bs = std::get_if<LangAPI::Symbol>(&base))
                if (const auto *b = typer->findClass(*bs); b && b != &cls)
                    if (const auto *o = staticOwner(*b, name)) return o;
        return nullptr;
    }

    auto Construct::convertSymbolValue(const LangAPI::Symbol &sym) -> std::string {
        std::vector<std::string> parts;
        for (const auto &p : sym.path) {
            const auto *str = std::get_if<std::string>(&p);
            if (!str) throw Unsupported{"symbol with a call/library path part"};
            parts.push_back(*str);
        }
        if (parts.size() == 1) {
            const auto &n = parts[0];
            if (isLocal(n)) return cIdent(n);
            if (n == "self") return "self";
            if (!cur_class.empty())
                if (const auto m = memberPath(cur_class, n)) return "self->" + *m;
            if (cur_class_decl)
                if (const auto so = staticOwner(*cur_class_decl, n)) return class_cname.at(so) + "_" + cIdent(n);
            if (n == "None") return "NULL";
            if (n == "True") return "true";
            if (n == "False") return "false";
        }
        std::vector<std::string> diag;
        if (const auto q = resolve(LangAPI::Symbol(sym.path), diag); defined.contains(q) || functions.contains(q)) return q;
        // qualified function / global: try the scope chain
        const auto joinedName = joined([&] { std::vector<std::string> v; for (auto &x : parts) v.push_back(cIdent(x)); return v; }(), "_");
        for (std::size_t k = scope.size() + 1; k-- > 0;) {
            std::vector<std::string> cand;
            for (std::size_t i = 0; i < k; ++i) cand.push_back(cIdent(scope[i]));
            cand.push_back(joinedName);
            if (const auto n = joined(cand, "_"); functions.contains(n)) return n;
        }
        throw Unsupported{"unresolved name " + joined(parts, "::")};
    }

    // ------------------------------------------------------------------ parser rule layer
    // Values built from C++ template arguments (token sets, policies, claims) never change: they are made once per call site.
    auto Construct::siteStatic(const std::string &type, const std::string &init) -> std::string {
        const auto name = prefix() + "_site" + std::to_string(lambda_counter++);
        lambda_defs += "static " + type + " " + name + "(void) { static " + type + " v; if (!v) v = " + init + "; return v; }\n\n";
        return name + "()";
    }

    auto Construct::tokenIdOf(const std::variant<std::shared_ptr<LangAPI::Type>, std::shared_ptr<LangAPI::RValue>> &item) -> std::string {
        if (const auto *t = std::get_if<std::shared_ptr<LangAPI::Type>>(&item); t && *t) {
            if ((*t)->isIspaLibSymbol() && (*t)->getIspaLibSymbol().exports == LangAPI::StdlibExports::RecoveryEndOfInput) return "python_ispa_std_recovery_EOF_TOKEN";
            if ((*t)->isSymbol()) return "(int64_t)" + convertSymbolValue((*t)->getSymbol());
            throw Unsupported{"recovery token set item is not a token id"};
        }
        const auto *r = std::get_if<std::shared_ptr<LangAPI::RValue>>(&item);
        if (!r || !*r) throw Unsupported{"empty recovery token set item"};
        return "(int64_t)" + convertRValue(**r);
    }

    auto Construct::tokenSetExpr(const LangAPI::Type &set) -> std::string {
        if (!set.isIspaLibSymbol() || set.getIspaLibSymbol().exports != LangAPI::StdlibExports::RecoveryTokenSet) throw Unsupported{std::string("recovery token set expected, got ") + (set.isValueType() ? "valuetype " + std::to_string(static_cast<int>(set.getValueType())) : set.isSymbol() ? "symbol " + symbolName(set.getSymbol()) : "ispalib " + std::to_string(static_cast<int>(set.getIspaLibSymbol().exports)))};
        std::vector<std::string> ids;
        for (const auto &item : set.getIspaLibSymbol().template_parameters) ids.push_back(tokenIdOf(item));
        const auto vec = ids.empty() ? std::string("ispa_vec_int64_t_new()")
                                     : "ispa_vec_int64_t_from((int64_t[]){" + joined(ids, ", ") + "}, " + std::to_string(ids.size()) + ")";
        return "python_ispa_std_recovery_TokenSet_new(" + vec + ")";
    }

    auto Construct::policyExpr(const LangAPI::Type &policy) -> std::string {
        if (!policy.isIspaLibSymbol() || policy.getIspaLibSymbol().exports != LangAPI::StdlibExports::RecoveryPolicy) throw Unsupported{"recovery policy expected"};
        std::vector<std::string> v;
        for (const auto &item : policy.getIspaLibSymbol().template_parameters) {
            if (const auto *r = std::get_if<std::shared_ptr<LangAPI::RValue>>(&item); r && *r) v.push_back(convertRValue(**r));
            else if (const auto *t = std::get_if<std::shared_ptr<LangAPI::Type>>(&item); t && *t && (*t)->isSymbol()) v.push_back(convertSymbolValue((*t)->getSymbol()));
            else throw Unsupported{"recovery policy item"};
        }
        static const char *const defaults[] = {"true", "true", "true", "INT64_C(64)"};
        for (std::size_t i = v.size(); i < 4; ++i) v.emplace_back(defaults[i]);
        return "python_ispa_std_recovery_Policy_new(" + joined(v, ", ") + ")";
    }

    // The class a rule/token type names, and whether it is a token (has a Tokens id) or a rule.
    auto Construct::ruleOrToken(const LangAPI::Type &t) -> RuleOrToken {
        const auto *cls = typer->classOf(t);
        if (!cls) throw Unsupported{std::string("parser combinator over an unknown type: ") + (t.isSymbol() ? symbolName(t.getSymbol()) : t.isValueType() ? "valuetype " + std::to_string(static_cast<int>(t.getValueType())) : "ispalib")};
        const auto cn = class_cname.at(cls);
        const auto at = cn.find("_FlatTypes_");
        if (at == std::string::npos) throw Unsupported{"parser combinator over a type outside FlatTypes: " + cn};
        const auto ns = cn.substr(0, at), suffix = cn.substr(at + std::string("_FlatTypes_").size());
        RuleOrToken out;
        const auto tok = ns + "_Tokens_" + suffix + "_id";
        if (functions.contains(tok)) { out.token = true; out.id = "(int64_t)" + tok; }
        else out.name = ns + "_RulesStr_" + suffix + "_id";
        return out;
    }

    auto Construct::convertLibCall(const LangAPI::FunctionCall &call, const LangAPI::IspaLibSymbol &lib) -> std::string {
        using S = LangAPI::StdlibExports;
        if (!cur_class_decl) throw Unsupported{"parser combinator outside a class"};
        const auto typeArg = [&](std::size_t i) -> const LangAPI::Type & {
            if (i >= call.template_parameters.size()) throw Unsupported{"missing template argument"};
            const auto *t = std::get_if<std::shared_ptr<LangAPI::Type>>(&call.template_parameters[i]);
            if (!t || !*t) throw Unsupported{"template argument is not a type"};
            return **t;
        };
        const auto param = [&](const char *method, std::size_t idx) -> LangAPI::Type {
            const auto *m = typer->findMethod(*cur_class_decl, method);
            if (!m || idx >= m->parameters.size()) throw Unsupported{std::string("library method ") + method + " not found"};
            return m->parameters[idx].first;
        };
        const auto arg = [&](std::size_t i) { return convertExpr(call.args.at(i)); };
        switch (lib.exports) {
            case S::RecoveryHandler: {
                if (call.template_parameters.size() != 5 || call.args.size() != 2) throw Unsupported{"recovery handler needs 5 template and 2 call arguments"};
                const auto r = arg(0), e = arg(1);
                const auto init = "python_ispa_std_recovery_Handler_new(" + r + ", " + e + ", " + tokenSetExpr(typeArg(0)) + ", " + tokenSetExpr(typeArg(1)) + ", " +
                                  tokenSetExpr(typeArg(2)) + ", " + tokenSetExpr(typeArg(3)) + ", " + policyExpr(typeArg(4)) + ")";
                const bool constant = r.starts_with("ISPA_STR(") && e.starts_with("ISPA_STR(");
                return constant ? siteStatic("python_ispa_std_recovery_Handler*", init) : init;
            }
            case S::ParserRequired: case S::ParserRange: case S::ParserOptional: case S::ParserRangeOptional: case S::ParserCounted: {
                const auto rt = ruleOrToken(typeArg(0));
                const bool has_fail = lib.exports == S::ParserRequired || lib.exports == S::ParserRange;
                const char *base = lib.exports == S::ParserRequired ? "required" : lib.exports == S::ParserRange ? "range" :
                                   lib.exports == S::ParserOptional ? "optional" : lib.exports == S::ParserRangeOptional ? "range_optional" : "counted";
                const std::string method = std::string(base) + (rt.token ? "_token" : "_rule");
                std::vector<std::string> a = {arg(0)};
                const auto cb = [&](std::size_t call_index, std::size_t param_index) {
                    return convertExprAs(call.args.at(call_index), param(method.c_str(), param_index));
                };
                if (lib.exports == S::ParserCounted) {
                    const auto spec = arg(1);
                    if (rt.token) a.insert(a.end(), {rt.id, spec});
                    else a.insert(a.end(), {cb(2, 1), spec});
                } else if (rt.token) {
                    a.push_back(rt.id);
                    if (has_fail) a.push_back(arg(1));
                } else {
                    if (has_fail) a.push_back(rt.name);       // rule name
                    a.push_back(cb(1, has_fail ? 2 : 1));
                    if (has_fail) a.push_back(arg(2));
                }
                return methodCall(*cur_class_decl, method, "self", a);
            }
            default: break;
        }
        throw Unsupported{"library call " + std::to_string(static_cast<int>(lib.exports))};
    }

    // recovery_at<Sets...>(expecting, labels...) and recover_choice<Sets...>(pos, rule, expected)
    auto Construct::convertRecoveryCall(const LangAPI::FunctionCall &call, const std::string &name) -> std::string {
        if (!cur_class_decl) throw Unsupported{"recovery call outside a class"};
        std::vector<LangAPI::Type> sets;
        for (const auto &tp : call.template_parameters) {
            const auto *t = std::get_if<std::shared_ptr<LangAPI::Type>>(&tp);
            if (!t || !*t) throw Unsupported{"recovery set argument"};
            sets.push_back(**t);
        }
        if (name == "recovery_at") {
            if (call.args.size() != sets.size() + 1) throw Unsupported{"recovery_at needs an expectation and one label per set"};
            const auto expecting = convertExpr(call.args[0]);
            std::vector<std::string> claims;
            bool constant = expecting.starts_with("ISPA_STR(");
            for (std::size_t i = 0; i < sets.size(); ++i) {
                const auto label = convertExpr(call.args[i + 1]);
                constant = constant && label.starts_with("ISPA_STR(");
                claims.push_back("python_ispa_std_recovery_ContextClaim_new(" + label + ", " + tokenSetExpr(sets[i]) + ")");
            }
            const auto vn = vecName("python_ispa_std_recovery_ContextClaim*");
            const auto vec = claims.empty() ? vn + "_new()" : vn + "_from((python_ispa_std_recovery_ContextClaim*[]){" + joined(claims, ", ") + "}, " + std::to_string(claims.size()) + ")";
            const auto init = "python_ispa_std_recovery_ClaimSet_new(" + vec + ")";
            const auto set = constant ? siteStatic("python_ispa_std_recovery_ClaimSet*", init) : init;
            return methodCall(*cur_class_decl, "recovery_at_set", "self", {expecting, set});
        }
        // recover_choice: even template arguments are the alternatives' first sets, odd ones their inner sets
        if (call.args.size() != 3 || sets.size() % 2 != 0) throw Unsupported{"invalid recover_choice"};
        std::vector<std::string> first, inner;
        for (std::size_t i = 0; i < sets.size(); ++i) (i % 2 == 0 ? first : inner).push_back(tokenSetExpr(sets[i]));
        const auto vn = vecName("python_ispa_std_recovery_TokenSet*");
        const auto mk = [&](const std::vector<std::string> &v) {
            const auto init = v.empty() ? vn + "_new()" : vn + "_from((python_ispa_std_recovery_TokenSet*[]){" + joined(v, ", ") + "}, " + std::to_string(v.size()) + ")";
            return siteStatic(vn + "*", init);
        };
        return methodCall(*cur_class_decl, "recover_choice", "self", {convertExpr(call.args[0]), convertExpr(call.args[1]), convertExpr(call.args[2]), mk(first), mk(inner)});
    }

    auto Construct::convertCall(const LangAPI::FunctionCall &call) -> std::string {
        if (const auto *lib = std::get_if<std::shared_ptr<LangAPI::IspaLibSymbol>>(&call.name); lib && *lib) return convertLibCall(call, **lib);
        if (const auto *rs = std::get_if<std::shared_ptr<LangAPI::Symbol>>(&call.name); rs && *rs && (*rs)->path.size() == 1 &&
            std::holds_alternative<std::string>((*rs)->path[0]) && !call.template_parameters.empty() &&
            (std::get<std::string>((*rs)->path[0]) == "recovery_at" || std::get<std::string>((*rs)->path[0]) == "recover_choice"))
            return convertRecoveryCall(call, std::get<std::string>((*rs)->path[0]));
        if (const auto *sy = std::get_if<std::shared_ptr<LangAPI::Symbol>>(&call.name); sy && *sy && (*sy)->path.size() == 2 &&
            std::holds_alternative<std::string>((*sy)->path[0]) && std::get<std::string>((*sy)->path[0]) == "ISPA_STD" &&
            std::holds_alternative<std::string>((*sy)->path[1]) &&
            (std::get<std::string>((*sy)->path[1]) == "construct" || std::get<std::string>((*sy)->path[1]) == "make_value") &&
            call.template_parameters.size() == 1) {
            const auto *tp = std::get_if<std::shared_ptr<LangAPI::Type>>(&call.template_parameters[0]);
            if (!tp || !*tp) throw Unsupported{"construct with a non-type argument"};
            const auto &T = **tp;
            const auto *cls = typer->classOf(T);
            std::vector<std::string> d;
            if (cls) {
                const LangAPI::Function *init = typer->findMethod(*cls, "__init__");
                if (!init && cls->constructor) init = &*cls->constructor;
                std::vector<std::string> args;
                for (std::size_t k = 0; k < call.args.size(); ++k) {
                    if (init && k < init->parameters.size()) {
                        const auto &pt = init->parameters[k].first;
                        args.push_back(coerce(convertExprAs(call.args[k], pt), typer->typeOf(call.args[k]), pt));
                    } else args.push_back(convertExpr(call.args[k]));
                }
                return class_cname.at(cls) + "_new(" + joined(args, ", ") + ")";
            }
            if (LangAPI::Typing::Typer::isValue(T, LangAPI::ValueType::Array) && call.args.empty()) {
                const auto elem = LangAPI::Typing::Typer::elementType(T);
                if (elem) return vecName(typeName(*elem, d)) + "_new()";
            }
            throw Unsupported{"construct of a non-class type"};
        }
        if (const auto *g = typer->resolveFunction(call); g && !g->template_parameters.empty()) {
            const auto targs = typer->bindTemplates(*g, call);
            if (!targs) {
                std::string tp;
                for (const auto &[n, bounds] : g->template_parameters) { tp += n + "("; for (const auto &b : bounds) tp += b.isIspaLibSymbol() ? "lib" + std::to_string(static_cast<int>(b.getIspaLibSymbol().exports)) : b.isSymbol() ? symbolName(b.getSymbol()) : "v"; tp += ") "; }
                throw Unsupported{"cannot infer the type arguments of " + g->name + " : " + tp};
            }
            const auto *inst = instantiateFunction(*g, *targs);
            std::vector<std::string> cargs;
            for (std::size_t k = 0; k < call.args.size(); ++k) {
                if (k < inst->parameters.size()) {
                    const auto &pt = inst->parameters[k].first;
                    cargs.push_back(coerce(convertExprAs(call.args[k], pt), typer->typeOf(call.args[k]), pt));
                } else cargs.push_back(convertExpr(call.args[k]));
            }
            const auto &gs = generic_fn_scope.at(g);
            std::vector<std::string> parts;
            if (gs.empty()) parts.push_back(cIdent(std::string(namespace_name)));
            for (const auto &x : gs) parts.push_back(cIdent(x));
            return joined(parts, "_") + "_" + cIdent(inst->name) + "(" + joined(cargs, ", ") + ")";
        }
        const auto *isym = std::get_if<std::shared_ptr<LangAPI::Symbol>>(&call.name);
        if (isym && *isym && (*isym)->path.size() == 2 && std::holds_alternative<std::string>((*isym)->path[0]) && std::get<std::string>((*isym)->path[0]) == "ISPA_STD" &&
            std::holds_alternative<std::string>((*isym)->path[1])) {
            const auto &f = std::get<std::string>((*isym)->path[1]);
            if (call.args.size() == 1) {
                const auto a0 = convertExpr(call.args[0]);
                const auto t0 = typer->typeOf(call.args[0]);
                if (f == "borrow") return a0;     // a handle already is a reference
                if (f == "python_char_code")
                    return t0 && LangAPI::Typing::Typer::isValue(*t0, LangAPI::ValueType::Char) ? "ispa_char_code(" + a0 + ")" : "ispa_str_code(" + a0 + ")";
                if (f == "python_char_from_code") return "ispa_char_from_code(" + a0 + ")";
            }
        }
        // template arguments of a call to a lowered (dynamically typed) function carry no information in C
        if (!call.template_parameters.empty() && !typer->resolveFunction(call)) {
            std::string nm = "?";
            if (const auto *sy = std::get_if<std::shared_ptr<LangAPI::Symbol>>(&call.name); sy && *sy) nm = symbolName(**sy);
            else nm = "ispalib";
            throw Unsupported{"call with template arguments: " + nm};
        }
        const auto *sym = std::get_if<std::shared_ptr<LangAPI::Symbol>>(&call.name);
        if (!sym || !*sym) throw Unsupported{"library function call"};
        if ((*sym)->path.size() == 1 && std::holds_alternative<std::string>((*sym)->path[0])) {
            const auto &nm = std::get<std::string>((*sym)->path[0]);
            const bool is_local = isLocal(nm);
            const auto vt = is_local ? typer->localType(nm) : (cur_class_decl ? typer->fieldType(*cur_class_decl, nm) : std::nullopt);
            if (vt) if (const auto sig = LangAPI::Typing::Typer::callableSignature(*vt)) {
                std::vector<std::string> cargs;
                for (std::size_t k = 0; k < call.args.size(); ++k)
                    cargs.push_back(k < sig->first.size() ? coerce(convertExprAs(call.args[k], sig->first[k]), typer->typeOf(call.args[k]), sig->first[k]) : convertExpr(call.args[k]));
                return invokeCallable(convertSymbolValue(**sym), *vt, cargs);
            }
        }
        std::vector<std::string> args;
        const auto *decl = typer->resolveFunction(call);
        if (const auto *qcls = typer->qualifiedMethodClass(**sym); qcls && decl && !decl->is_static && !call.args.empty()) {
            // Class.method(self, args...): explicit receiver
            const auto &mname = std::get<std::string>((*sym)->path.back());
            const auto *owner = typer->methodOwner(*qcls, mname);
            std::vector<std::string> margs;
            for (std::size_t k = 1; k < call.args.size(); ++k) {
                if (k - 1 < decl->parameters.size()) {
                    const auto &pt = decl->parameters[k - 1].first;
                    margs.push_back(coerce(convertExprAs(call.args[k], pt), typer->typeOf(call.args[k]), pt));
                } else margs.push_back(convertExpr(call.args[k]));
            }
            const auto recv = "(" + class_cname.at(owner) + "*)" + convertExpr(call.args[0]);
            margs.insert(margs.begin(), recv);
            return class_cname.at(owner) + "_" + cIdent(mname) + "(" + joined(margs, ", ") + ")";
        }
        for (std::size_t k = 0; k < call.args.size(); ++k) {
            if (decl && k < decl->parameters.size()) {
                const auto &pt = decl->parameters[k].first;
                args.push_back(coerce(convertExprAs(call.args[k], pt), typer->typeOf(call.args[k]), pt));
            } else args.push_back(convertExpr(call.args[k]));
        }
        const auto &path = (*sym)->path;
        if (path.size() == 1 && std::holds_alternative<std::string>(path[0]) && !cur_class.empty()) {
            // implicit-self method call (own or inherited)
            const auto &m = std::get<std::string>(path[0]);
            if (cur_class_decl && typer->findMethod(*cur_class_decl, m)) return methodCall(*cur_class_decl, m, "self", args);
        }
        const auto fn = convertSymbolValue(**sym);
        return fn + "(" + joined(args, ", ") + ")";
    }

    // a free function used as a value: static thunk (void *env, params...) forwarding to it
    auto Construct::functionValue(const LangAPI::Symbol &sy, const LangAPI::Function &fn) -> std::string {
        const auto target = convertSymbolValue(sy);
        const auto thunk = target + "__thunk";
        if (thunks_done.insert(thunk).second) {
            std::vector<std::string> d, ps = {"void *env_"}, as;
            for (const auto &[t, n] : fn.parameters) { ps.push_back(typeName(t, d) + " " + cIdent(n)); as.push_back(cIdent(n)); }
            const auto ret = typeName(fn.type, d);
            lambda_defs += "static " + ret + " " + thunk + "(" + joined(ps, ", ") + ") { (void)env_; " + (ret == "void" ? "" : "return ") + target + "(" + joined(as, ", ") + "); }\n\n";
        }
        return "((ispa_fn){ (void *)" + thunk + ", NULL })";
    }

    // `self.method` used as a value: a thunk that receives self through the callable's env
    auto Construct::boundMethodValue(const std::string &name, const LangAPI::Function &fn) -> std::string {
        const auto cn = class_cname.at(cur_class_decl);
        const auto thunk = cn + "_" + cIdent(name) + "__bound";
        if (thunks_done.insert(thunk).second) {
            std::vector<std::string> d, ps = {"void *env_"}, as;
            for (const auto &[t, n] : fn.parameters) { ps.push_back(typeName(t, d) + " " + cIdent(n)); as.push_back(cIdent(n)); }
            const auto ret = typeName(fn.type, d);
            const auto call = methodCall(*cur_class_decl, name, "(" + cn + "*)env_", as);
            lambda_defs += "static " + ret + " " + thunk + "(" + joined(ps, ", ") + ") { " + (ret == "void" ? "" : "return ") + call + "; }\n\n";
        }
        return "((ispa_fn){ (void *)" + thunk + ", self })";
    }

    auto Construct::invokeCallable(const std::string &code, const LangAPI::Type &callable, const std::vector<std::string> &args) -> std::string {
        const auto sig = LangAPI::Typing::Typer::callableSignature(callable);
        if (!sig) throw Unsupported{"call of a non-callable"};
        std::vector<std::string> d, ps = {"void*"};
        for (const auto &p : sig->first) ps.push_back(typeName(p, d));
        auto as = args;
        as.insert(as.begin(), "(" + code + ").env");
        return "((" + typeName(sig->second, d) + " (*)(" + joined(ps, ", ") + "))(" + code + ").fn)(" + joined(as, ", ") + ")";
    }

    // A lambda becomes a static function taking its captured variables through an env struct (captured by value; the
    // lowered library only reads captured names inside lambdas, or mutates the objects they point to).
    auto Construct::convertLambda(const LangAPI::Lambda &lam, const LangAPI::Typing::MaybeType &expected, bool void_ret) -> std::string {
        const auto id = prefix() + "_lam" + std::to_string(lambda_counter++);
        std::vector<std::string> d;
        std::vector<LangAPI::Type> eparams; std::optional<LangAPI::Type> eret;
        if (expected) if (auto sig = LangAPI::Typing::Typer::callableSignature(*expected)) { eparams = sig->first; eret = sig->second; }
        std::vector<std::pair<LangAPI::Type, std::string>> params;
        for (std::size_t i = 0; i < lam.parameters.size(); ++i) {
            auto t = lam.parameters[i].first;
            if (t.isValueType() && t.getValueType() == LangAPI::ValueType::Undef) {
                if (i >= eparams.size()) throw Unsupported{"lambda parameter without a type"};
                t = eparams[i];
            }
            params.emplace_back(t, lam.parameters[i].second);
        }
        const LangAPI::Type ret = void_ret || !eret ? LangAPI::Type(LangAPI::ValueType::Void) : *eret;
        std::set<std::string> pnames;
        for (const auto &[t, n] : params) pnames.insert(n);
        std::vector<std::pair<std::string, LangAPI::Type>> caps;
        std::set<std::string> seen;
        bool bad = false;
        LangAPI::Traverse::forEach<LangAPI::Symbol>(lam.statements, [&](const LangAPI::Symbol &sy) {
            if (sy.path.size() != 1 || !std::holds_alternative<std::string>(sy.path[0])) return;
            const auto &n = std::get<std::string>(sy.path[0]);
            if (n == "self" || pnames.contains(n) || !isLocal(n) || !seen.insert(n).second) return;
            if (const auto t = typer->localType(n)) caps.emplace_back(n, *t); else bad = true;
        });
        if (bad) throw Unsupported{"lambda captures a local of unknown type"};
        if (cur_class_decl) caps.emplace_back("self", typer->classType(*cur_class_decl));
        // body
        Converter::Writer body;
        body.increaseIndentation();
        auto saved_locals = locals; const auto *saved_ret = cur_return;
        locals.emplace_back();
        for (const auto &[t, n] : params) locals.back().insert(n);
        typer->pushScope();
        for (const auto &[t, n] : params) typer->declare(n, t);
        LangAPI::Type ret_inferred = ret;
        if (!void_ret && !eret) {      // no expected signature: the type of the first `return`
            std::optional<LangAPI::Type> found;
            LangAPI::Traverse::forEach<LangAPI::Return>(lam.statements, [&](const LangAPI::Return &rt) { if (!found && !rt.value.empty()) found = typer->typeOf(rt.value); });
            if (found) ret_inferred = *found;
        }
        const LangAPI::Type &ret_final = ret_inferred;
        cur_return = &ret_final;
        std::string env_fields, make_params, make_assign, make_args;
        for (const auto &[n, t] : caps) {
            const auto ct = n == "self" && cur_class_decl ? class_cname.at(cur_class_decl) + "*" : typeName(t, d);
            body.writeln("{} {} = env->{};", ct, cIdent(n), cIdent(n));
            env_fields += "    " + ct + " " + cIdent(n) + ";\n";
            make_params += (make_params.empty() ? "" : ", ") + ct + " " + cIdent(n);
            make_assign += "    e->" + cIdent(n) + " = " + cIdent(n) + ";\n";
            make_args += (make_args.empty() ? "" : ", ") + cIdent(n);
        }
        try { convertStatements(lam.statements, body); }
        catch (...) { typer->popScope(); locals = saved_locals; cur_return = saved_ret; throw; }
        typer->popScope(); locals = saved_locals; cur_return = saved_ret;
        std::vector<std::string> ps = {"void *env_"};
        for (const auto &[t, n] : params) ps.push_back(typeName(t, d) + " " + cIdent(n));
        std::string def = "typedef struct {\n" + (env_fields.empty() ? std::string("    char unused_;\n") : env_fields) + "} " + id + "_env;\n";
        def += "static " + typeName(ret_final, d) + " " + id + "(" + joined(ps, ", ") + ") {\n    " + id + "_env *env = (" + id + "_env *)env_; (void)env;\n" + std::string(body.get()) + "}\n";
        def += "static ispa_fn " + id + "_make(" + (make_params.empty() ? std::string("void") : make_params) + ") {\n    " + id + "_env *e = ISPA_NEW(" + id + "_env);\n" + make_assign + "    return (ispa_fn){ (void *)" + id + ", e };\n}\n\n";
        lambda_defs += def;
        return id + "_make(" + make_args + ")";
    }

    auto Construct::arrayLiteralValues(const decltype(LangAPI::Array::values) &values, const LangAPI::Typing::MaybeType &type) -> std::string {
        LangAPI::Typing::MaybeType elem = type ? LangAPI::Typing::Typer::elementType(*type) : std::nullopt;
        if (!elem && !values.empty()) elem = typer->typeOf(values[0]);
        if (!elem) throw Unsupported{"array literal of unknown element type"};
        std::vector<std::string> d;
        const auto et = typeName(*elem, d);
        const auto vn = vecName(et);
        if (values.empty()) return vn + "_new()";
        std::vector<std::string> items;
        for (const auto &v : values) items.push_back(coerce(convertExprAs(v, *elem), typer->typeOf(v), *elem));
        return vn + "_from((" + et + "[]){" + joined(items, ", ") + "}, " + std::to_string(items.size()) + ")";
    }
    auto Construct::arrayLiteral(const LangAPI::Array &arr, const LangAPI::Type &type) -> std::string {
        return arrayLiteralValues(arr.values, type);
    }

    // an expression whose expected type is known (initialiser, assignment, return): lets `[]` pick its element type
    auto Construct::convertExprAs(const LangAPI::Expression &e, const LangAPI::Type &expected) -> std::string {
        const auto saved = array_hint;
        array_hint = expected;
        struct R { LangAPI::Typing::MaybeType &r; LangAPI::Typing::MaybeType v; ~R() { r = v; } } restore{array_hint, saved};
        return convertExpr(e);
    }

    // call of a TokenCursor method (dispatched when a subclass overrides it)
    auto Construct::cursorCall(const std::string &method, const std::string &recv, std::vector<std::string> args) -> std::string {
        const auto *tc = typer->findClass(LangAPI::Symbol(std::vector<std::string>{"python", "ispa_std", "lexer", "TokenCursor"}));
        if (!tc) throw Unsupported{"TokenCursor is not in the tree"};
        return methodCall(*tc, method, recv, std::move(args));
    }

    auto Construct::methodCall(const LangAPI::Class &recv_cls, const std::string &name, const std::string &recv,
                               std::vector<std::string> args) -> std::string {
        const auto *owner = typer->methodOwner(recv_cls, name);
        if (!owner) throw Unsupported{"method " + name + " not found"};
        const auto it = class_cname.find(owner);
        if (it == class_cname.end()) throw Unsupported{"class of method " + name + " unknown"};
        const auto *fn = typer->findMethod(recv_cls, name);
        if (fn && fn->is_static) return it->second + "_" + cIdent(name) + "(" + joined(args, ", ") + ")";
        if (const auto v = slot_root.find({owner, name}); v != slot_root.end()) {   // overridden somewhere: dispatch through the vtable
            const auto *root = v->second;
            const auto rc = class_cname.at(root);
            args.insert(args.begin(), root == &recv_cls ? recv : "(" + rc + "*)" + recv);
            return rc + "_" + cIdent(name) + "_v(" + joined(args, ", ") + ")";
        }
        args.insert(args.begin(), owner == &recv_cls ? recv : "(" + it->second + "*)" + recv);
        return it->second + "_" + cIdent(name) + "(" + joined(args, ", ") + ")";
    }

    // x.a.b[i].m(args) ... walked with the static type of the value built so far
    auto Construct::convertStorage(const LangAPI::StorageSymbol &ss) -> std::string {
        std::string out;
        LangAPI::Typing::MaybeType cur;
        if (ss.what.empty()) {
            if (!cur_class_decl) throw Unsupported{"storage path without a receiver"};
            out = "self"; cur = typer->classType(*cur_class_decl);
        } else { out = convertExpr(ss.what); cur = typer->typeOf(ss.what); }
        const auto args_of = [&](const auto &as) {
            std::vector<std::string> v; for (const auto &a : as) v.push_back(convertExpr(a)); return v;
        };
        for (const auto &part : ss.path) {
            if (!cur) throw Unsupported{"storage path of unknown type"};
            const auto *name = std::get_if<std::string>(&part);
            if (!name) if (const auto *p = std::get_if<LangAPI::Property>(&part)) name = &p->name;
            const auto stripped = LangAPI::Typing::Typer::strip(*cur);
            if (name) {
                const auto *cls = typer->classOf(*cur);
                if (!cls) throw Unsupported{"member " + *name + " of a non-class"};
                const auto m = memberPath(class_cname.at(cls), *name);
                if (!m) throw Unsupported{"unknown member " + *name};
                out += "->" + *m; cur = typer->fieldType(*cls, *name);
            } else if (const auto *off = std::get_if<LangAPI::StorageOffset>(&part)) {
                const auto idx = convertExpr(off->offset);
                if (stripped.isValueType() && stripped.getValueType() == LangAPI::ValueType::Array) {
                    std::vector<std::string> dd;
                    out = vecName(typeName(*LangAPI::Typing::Typer::elementType(stripped), dd)) + "_get(" + out + ", " + idx + ")";
                } else if (stripped.isValueType() && (stripped.getValueType() == LangAPI::ValueType::String ||
                                                       stripped.getValueType() == LangAPI::ValueType::NonOwnedString)) {
                    out = "ispa_str_at(" + out + ", " + idx + ")";
                } else throw Unsupported{"index into a map/other container"};
                cur = LangAPI::Typing::Typer::elementType(stripped);
            } else if (const auto *call = std::get_if<LangAPI::FunctionCall>(&part)) {
                const auto *sym = std::get_if<std::shared_ptr<LangAPI::Symbol>>(&call->name);
                const auto *cls = typer->classOf(*cur);
                if (!sym || !*sym || !cls || (*sym)->path.size() != 1 || !std::holds_alternative<std::string>((*sym)->path[0]))
                    throw Unsupported{"storage call on a non-class or library member"};
                const auto &mname = std::get<std::string>((*sym)->path[0]);
                const auto *fn = typer->findMethod(*cls, mname);
                if (!fn) if (const auto ft = typer->fieldType(*cls, mname)) if (const auto sig = LangAPI::Typing::Typer::callableSignature(*ft)) {
                    const auto m = memberPath(class_cname.at(cls), mname);
                    std::vector<std::string> cargs;
                    for (std::size_t k = 0; k < call->args.size(); ++k)
                        cargs.push_back(k < sig->first.size() ? coerce(convertExprAs(call->args[k], sig->first[k]), typer->typeOf(call->args[k]), sig->first[k]) : convertExpr(call->args[k]));
                    out = invokeCallable(out + "->" + *m, *ft, cargs);
                    cur = sig->second;
                    continue;
                }
                std::vector<std::string> margs;
                for (std::size_t k = 0; k < call->args.size(); ++k) {
                    if (fn && k < fn->parameters.size()) margs.push_back(coerce(convertExprAs(call->args[k], fn->parameters[k].first), typer->typeOf(call->args[k]), fn->parameters[k].first));
                    else margs.push_back(convertExpr(call->args[k]));
                }
                out = methodCall(*cls, mname, out, margs);
                cur = fn ? LangAPI::Typing::MaybeType(fn->type) : std::nullopt;
            } else if (const auto *am = std::get_if<LangAPI::ArrayMethodCall>(&part)) {
                if (stripped.isValueType() && (stripped.getValueType() == LangAPI::ValueType::String || stripped.getValueType() == LangAPI::ValueType::NonOwnedString) &&
                    am->method == LangAPI::ArrayMethods::Size) {
                    out = "(int64_t)(" + out + ").len"; cur = LangAPI::Type(LangAPI::ValueType::Int); continue;
                }
                if (!stripped.isValueType() || stripped.getValueType() != LangAPI::ValueType::Array)
                    throw Unsupported{"array method " + std::to_string(static_cast<int>(am->method)) + " on valuetype " + (stripped.isValueType() ? std::to_string(static_cast<int>(stripped.getValueType())) : std::string("symbol"))};
                std::vector<std::string> d;
                const auto elemT = *LangAPI::Typing::Typer::elementType(stripped);
                const auto vn = vecName(typeName(elemT, d));
                auto a = args_of(am->args);
                if (am->method == LangAPI::ArrayMethods::Push || am->method == LangAPI::ArrayMethods::PythonAppend)
                    if (am->args.size() == 1) a[0] = coerce(convertExprAs(am->args[0], elemT), typer->typeOf(am->args[0]), elemT);
                using AM = LangAPI::ArrayMethods;
                switch (am->method) {
                    case AM::Size: out = "(int64_t)(" + out + ")->len"; cur = LangAPI::Type(LangAPI::ValueType::Int); break;
                    case AM::Push: case AM::PythonAppend:
                        if (a.empty()) { out = vn + "_push(" + out + ", (" + typeName(elemT, d) + "){0})"; cur = LangAPI::Type(LangAPI::ValueType::Void); break; }
                        if (a.size() != 1) throw Unsupported{"append arity"};
                        out = vn + "_push(" + out + ", " + a[0] + ")"; cur = LangAPI::Type(LangAPI::ValueType::Void); break;
                    case AM::Pop: case AM::SequencePopBack: out = vn + "_pop(" + out + ")"; cur = elemT; break;
                    case AM::PythonPop:
                        out = a.empty() ? vn + "_pop(" + out + ")" : vn + "_pop_at(" + out + ", " + a[0] + ")"; cur = elemT; break;
                    case AM::PythonReverse: out = vn + "_reverse(" + out + ")"; cur = LangAPI::Type(LangAPI::ValueType::Void); break;
                    case AM::PythonInsert:
                        if (a.size() != 2) throw Unsupported{"insert arity"};
                        out = vn + "_insert(" + out + ", " + a[0] + ", " + a[1] + ")"; cur = LangAPI::Type(LangAPI::ValueType::Void); break;
                    case AM::Clear: case AM::PythonClear: out = vn + "_clear(" + out + ")"; cur = LangAPI::Type(LangAPI::ValueType::Void); break;
                    case AM::Extend: case AM::PythonExtend:
                        out = vn + "_extend(" + out + ", " + a.at(0) + ")"; cur = LangAPI::Type(LangAPI::ValueType::Void); break;
                    default: throw Unsupported{"array method " + std::to_string(static_cast<int>(am->method))};
                }
            } else throw Unsupported{"storage path with library member"};
        }
        return out;
    }

    auto Construct::convertRValue(const LangAPI::RValue &r) -> std::string {
        if (r.isInt()) return "INT64_C(" + std::to_string(r.getInt().value) + ")";
        if (r.isBool()) return r.getBool().value ? "true" : "false";
        if (r.isFloat()) return std::to_string(r.getFloat().value);
        if (r.isChar()) {
            const char c = r.getChar().value;
            if (c == '\'') return "'\\''";
            if (c == '\\') return "'\\\\'";
            if (c == '\n') return "'\\n'";
            if (c == '\t') return "'\\t'";
            if (c == '\r') return "'\\r'";
            if (c == 0) return "'\\0'";
            return std::string("'") + c + "'";
        }
        if (r.isString()) return "ISPA_STR(" + cString(r.getString().value) + ")";
        if (r.isSymbol()) {
            const auto &sy = r.getSymbol();
            const bool single_local = sy.path.size() == 1 && std::holds_alternative<std::string>(sy.path[0]) &&
                                      (isLocal(std::get<std::string>(sy.path[0])) || std::get<std::string>(sy.path[0]) == "self" ||
                                       (!cur_class.empty() && memberPath(cur_class, std::get<std::string>(sy.path[0]))) ||
                                       (cur_class_decl && typer->findMethod(*cur_class_decl, std::get<std::string>(sy.path[0]))));
            if (sy.path.size() == 1 && std::holds_alternative<std::string>(sy.path[0]) && cur_class_decl && !isLocal(std::get<std::string>(sy.path[0])) &&
                !memberPath(cur_class, std::get<std::string>(sy.path[0])))
                if (const auto *m = typer->findMethod(*cur_class_decl, std::get<std::string>(sy.path[0])); m && !m->is_static)
                    return boundMethodValue(std::get<std::string>(sy.path[0]), *m);
            if (!single_local)
                if (const auto *fn = typer->resolveFunctionSymbol(sy); fn && fn->template_parameters.empty() && !typer->qualifiedMethodClass(sy))
                    return functionValue(sy, *fn);
            return convertSymbolValue(sy);
        }
        if (r.isStorageSymbol()) return convertStorage(r.getStorageSymbol());
        if (r.isArray() || r.isFixedSizeArray()) {
            const auto &vals = r.isArray() ? r.getArray().values : r.getFixedSizeArray().values;
            const auto &tps = r.isArray() ? r.getArray().template_parameters : r.getFixedSizeArray().template_parameters;
            const auto hint = array_hint;
            array_hint.reset();
            if (!tps.empty())
                if (const auto *t = std::get_if<std::shared_ptr<LangAPI::Type>>(&tps[0]); t && *t)
                    return arrayLiteralValues(vals, LangAPI::Type(LangAPI::ValueType::Array, **t));
            return arrayLiteralValues(vals, hint);
        }
        if (r.type() == LangAPI::RValueType::MakeTuple) {      // (a, b) -> utility::Pair<A, B>
            const auto &mt = r.getMakeTuple();
            if (mt.args.size() != 2) throw Unsupported{"tuple of " + std::to_string(mt.args.size()) + " elements"};
            auto hint = array_hint;
            array_hint.reset();
            LangAPI::Type pair_t;
            if (hint && LangAPI::Typing::Typer::strip(*hint).isSymbol()) pair_t = LangAPI::Typing::Typer::strip(*hint);
            else {
                const auto a = typer->typeOf(mt.args[0]), b = typer->typeOf(mt.args[1]);
                if (!a || !b) throw Unsupported{"tuple of unknown element types"};
                pair_t = LangAPI::Type(LangAPI::Symbol(std::vector<std::string>{"python", "ispa_std", "utility", "Pair"}), *a, *b);
            }
            const auto *cls = typer->classOf(pair_t);
            if (!cls) throw Unsupported{"tuple type has no class"};
            const auto *init = typer->findMethod(*cls, "__init__");
            std::vector<std::string> targs;
            for (std::size_t k = 0; k < mt.args.size(); ++k) {
                if (init && k < init->parameters.size()) {
                    const auto &pt = init->parameters[k].first;
                    targs.push_back(coerce(convertExprAs(mt.args[k], pt), typer->typeOf(mt.args[k]), pt));
                } else targs.push_back(convertExpr(mt.args[k]));
            }
            return class_cname.at(cls) + "_new(" + joined(targs, ", ") + ")";
        }
        if (r.type() == LangAPI::RValueType::Inheritance) {      // base-class construction inside a constructor
            const auto &in = r.getInheritance();
            const LangAPI::Class *base = nullptr;
            if (const auto *sy = std::get_if<LangAPI::Symbol>(&in.name)) base = typer->findClass(*sy);
            else if (const auto *ty = std::get_if<std::shared_ptr<LangAPI::Type>>(&in.name); ty && *ty) base = typer->classOf(**ty);
            else if (const auto *il = std::get_if<LangAPI::IspaLibSymbol>(&in.name))
                if (const auto lp = libPath(il->exports)) base = typer->findClass(LangAPI::Symbol(*lp));
            // `Cls(args)` that is not one of the enclosing class's bases constructs an object
            if (base && !(cur_class_decl && base != cur_class_decl && typer->classOf(typer->classType(*base)) == base &&
                          pathToBase(*cur_class_decl, base))) {
                const bool printer = std::get_if<LangAPI::IspaLibSymbol>(&in.name) &&
                                     std::get<LangAPI::IspaLibSymbol>(in.name).exports == LangAPI::StdlibExports::ASTPrinter;
                const auto *init = typer->findMethod(*base, "__init__");
                std::vector<std::string> cargs;
                for (std::size_t k = 0; init && !printer && k < in.args.size(); ++k) {
                    if (k < init->parameters.size()) {
                        const auto &pt = init->parameters[k].first;
                        cargs.push_back(coerce(convertExprAs(in.args[k], pt), typer->typeOf(in.args[k]), pt));
                    } else cargs.push_back(convertExpr(in.args[k]));
                }
                return class_cname.at(base) + "_new(" + joined(cargs, ", ") + ")";
            }
            if (!base) {
                std::string what = "variant " + std::to_string(in.name.index());
                if (const auto *sy = std::get_if<LangAPI::Symbol>(&in.name)) { what += " symbol"; for (const auto &pp : sy->path) what += std::holds_alternative<std::string>(pp) ? " [" + std::get<std::string>(pp) + "]" : " <call>"; for (const auto &f : typer->debugClasses("NUMBER")) what += " cand:" + f; }
                else if (const auto *il = std::get_if<LangAPI::IspaLibSymbol>(&in.name)) what += " ispalib " + std::to_string(static_cast<int>(il->exports));
                else if (const auto *ty = std::get_if<std::shared_ptr<LangAPI::Type>>(&in.name); ty && *ty) what += (*ty)->isValueType() ? " valuetype " + std::to_string(static_cast<int>((*ty)->getValueType())) : std::string(" type");
                throw Unsupported{"base class of an inheritance call is unknown: " + what};
            }
            const auto *init = typer->findMethod(*base, "__init__");
            if (!init) return "((void)0)";
            const auto *owner = typer->methodOwner(*base, "__init__");
            std::vector<std::string> margs = {"(" + class_cname.at(owner) + "*)self"};
            for (std::size_t k = 0; k < in.args.size(); ++k) {
                if (k < init->parameters.size()) {
                    const auto &pt = init->parameters[k].first;
                    margs.push_back(coerce(convertExprAs(in.args[k], pt), typer->typeOf(in.args[k]), pt));
                } else margs.push_back(convertExpr(in.args[k]));
            }
            return class_cname.at(owner) + "___init__(" + joined(margs, ", ") + ")";
        }
        if (r.type() == LangAPI::RValueType::Pos) {
            const auto &p = r.getPos();
            const auto cur = "(python_ispa_std_lexer_TokenCursor*)" + counters.back();
            const auto at = p.offset ? cursorCall("plus", cur, {"INT64_C(" + std::to_string(p.offset) + ")"}) : cur;
            return p.dereference ? cursorCall("current", at, {}) : at;
        }
        if (r.type() == LangAPI::RValueType::CharToStringConstructor) return "ispa_str_from_char(" + convertExpr(r.getCharToStringConstructor().what) + ")";
        if (r.type() == LangAPI::RValueType::Instance) {
            if (cur_class.empty()) throw Unsupported{"instance outside a method"};
            return "self";
        }
        // Box<class> is already the object handle; a Box of a plain value will need a dereference once locals are typed
        if (r.type() == LangAPI::RValueType::GetFromBox) return convertExpr(r.getGetFromBox().what);
        if (r.type() == LangAPI::RValueType::IspaLibSymbol) {
            auto &mr = const_cast<LangAPI::RValue &>(r);
            switch (mr.getIspaSymbol().exports) {
                case LangAPI::StdlibExports::DfaNullState: return "python_ispa_std_dfa_api_NULL_STATE";
                case LangAPI::StdlibExports::DfaDynamicFailState: return "python_ispa_std_dfa_api_DYNAMIC_FAIL_STATE";
                default: break;
            }
        }
        throw Unsupported{"rvalue kind " + std::to_string(static_cast<int>(r.type())) +
                          (r.type() == LangAPI::RValueType::IspaLibSymbol ? " export " + std::to_string(static_cast<int>(const_cast<LangAPI::RValue &>(r).getIspaSymbol().exports)) : std::string())};
    }

    auto Construct::convertValue(const LangAPI::ExpressionValue &v) -> std::string {
        using EE = LangAPI::ExpressionElement;
        std::string out;
        {
            switch (v.type()) {
                case LangAPI::ExpressionValueType::Empty: break;
                case LangAPI::ExpressionValueType::RValue: out += convertRValue(v.getRValue()); break;
                case LangAPI::ExpressionValueType::FunctionCall: out += convertCall(v.getFunctionCall()); break;
                case LangAPI::ExpressionValueType::IspaLibFunctionCall: {
                    const auto &c = v.getIspaLibFunctionCall();
                    std::vector<std::string> a;
                    for (const auto &x : c.args) a.push_back(convertExpr(x));
                    switch (c.symbol.exports) {
                        case LangAPI::StdlibExports::Fail:
                            if (a.size() != 2) throw Unsupported{"fail arity"};
                            out += "ispa_fail(" + a[0] + ", " + a[1] + ")"; break;
                        case LangAPI::StdlibExports::FailureMessage: out += "ispa_failure_message()"; break;
                        case LangAPI::StdlibExports::RawTokenNodeConstruct: case LangAPI::StdlibExports::ParserNodeConstructor: {
                            if (token_cname.empty() || a.size() != 6) throw Unsupported{"token construction needs the application token class and 6 arguments"};
                            const auto vt = typer->typeOf(c.args[5]);
                            out += token_cname + "_create(" + a[0] + ", " + a[1] + ", " + a[2] + ", " + a[3] + ", " + a[4] + ", " +
                                   coerce(a[5], vt, LangAPI::Type(LangAPI::ValueType::Any)) + ")";
                            break;
                        }
                        case LangAPI::StdlibExports::LookaheadKind:
                            if (a.size() != 1) throw Unsupported{"lookahead_kind arity"};
                            out += "python_ispa_std_parser_ll_lookahead_kind((python_ispa_std_lexer_TokenCursor*)" + a[0] + ")"; break;
                        case LangAPI::StdlibExports::AssignCompatible: {       // the value, converted to the target's type
                            if (c.args.size() != 2) throw Unsupported{"assign_compatible arity"};
                            const auto tt = typer->typeOf(c.args[0]), st = typer->typeOf(c.args[1]);
                            if (!tt || !st) throw Unsupported{"assign_compatible between unknown types"};
                            std::vector<std::string> dd;
                            const auto tn = typeName(*tt, dd), sn = typeName(*st, dd);
                            const auto v = coerce(a[1], st, *tt);
                            if (tn != sn && !v.starts_with("(")) throw Unsupported{"assign_compatible converts " + sn + " to " + tn};
                            out += v; break;
                        }
                        case LangAPI::StdlibExports::TokenCursor:
                            if (a.size() != 1) throw Unsupported{"token cursor arity"};
                            out += "(python_ispa_std_lexer_TokenCursor*)python_ispa_std_lexer_TokenIterator_begin((python_ispa_std_lexer_Lexer_base*)" + a[0] + ")"; break;
                        case LangAPI::StdlibExports::LazyTokenCursor:
                            if (a.size() != 2) throw Unsupported{"lazy token cursor arity"};
                            out += "(python_ispa_std_lexer_Lexer_base_setInput((python_ispa_std_lexer_Lexer_base*)" + a[0] + ", " + a[1] +
                                   "), (python_ispa_std_lexer_TokenCursor*)python_ispa_std_lexer_LazyIterator_begin((python_ispa_std_lexer_Lexer_base*)" + a[0] + "))"; break;
                        case LangAPI::StdlibExports::CloneSelfBox:
                            if (!cur_class_decl) throw Unsupported{"clone outside a method"};
                            out += "(" + class_cname.at(cur_class_decl) + "*)ispa_clone_obj(self, sizeof(" + class_cname.at(cur_class_decl) + "))"; break;
                        case LangAPI::StdlibExports::ResetTokenStream: {
                            if (a.size() != 1) throw Unsupported{"reset token stream arity"};
                            const auto lt = typer->typeOf(c.args[0]);
                            const auto *lc = lt ? typer->classOf(*lt) : nullptr;
                            const auto m = lc ? memberPath(class_cname.at(lc), "tokens") : std::nullopt;
                            if (!m) throw Unsupported{"lexer without a tokens list"};
                            out += "(" + a[0] + ")->" + *m + "->len = 0"; break;
                        }
                        case LangAPI::StdlibExports::Attempt: {
                            if (c.args.size() != 1 || c.args[0].size() != 1 || c.args[0][0].type() != LangAPI::ExpressionValueType::Lambda)
                                throw Unsupported{"attempt of a non-lambda"};
                            out += "ispa_attempt_fn(" + convertLambda(c.args[0][0].getLambda(), std::nullopt, true) + ")"; break;
                        }
                        case LangAPI::StdlibExports::ParserRequired: case LangAPI::StdlibExports::ParserOptional: case LangAPI::StdlibExports::ParserRange:
                        case LangAPI::StdlibExports::ParserRangeOptional: case LangAPI::StdlibExports::ParserCounted: case LangAPI::StdlibExports::RecoveryHandler: {
                            LangAPI::FunctionCall fc;
                            fc.name = std::make_shared<LangAPI::IspaLibSymbol>(c.symbol);
                            fc.template_parameters = c.template_parameters;
                            fc.args = c.args;
                            out += convertLibCall(fc, c.symbol);
                            break;
                        }
                        default: throw Unsupported{"ispa-lib call " + std::to_string(static_cast<int>(c.symbol.exports))};
                    }
                    break;
                }
                case LangAPI::ExpressionValueType::SkipSpaces:
                    if (!cur_class_decl) throw Unsupported{"skip_spaces outside a class"};
                    out += methodCall(*cur_class_decl, "skip_spaces", "self", {"(python_ispa_std_lexer_TokenCursor*)" + counters.back()}); break;
                case LangAPI::ExpressionValueType::CounterIncreament:
                    out += cursorCall("next", "(python_ispa_std_lexer_TokenCursor*)" + counters.back(), {}); break;
                case LangAPI::ExpressionValueType::CounterAssignment: {
                    const auto &ca = v.getCounterAssignment();
                    out += cursorCall("assign", "(python_ispa_std_lexer_TokenCursor*)" + counters.back(), {"(python_ispa_std_lexer_TokenCursor*)(" + convertExpr(*ca.v) + ")"}); break;
                }
                case LangAPI::ExpressionValueType::PushPosCounter:
                    out += v.getPushPosCounter().name + " = " + cursorCall("copy", "(python_ispa_std_lexer_TokenCursor*)" + counters.back(), {});
                    counters.push_back(v.getPushPosCounter().name); break;
                case LangAPI::ExpressionValueType::PopPosCounter: {
                    if (counters.size() < 2) throw Unsupported{"unbalanced counter pop"};
                    const auto prev = counters[counters.size() - 2], child = counters.back();
                    counters.pop_back();
                    out += cursorCall("assign", "(python_ispa_std_lexer_TokenCursor*)" + prev, {"(python_ispa_std_lexer_TokenCursor*)" + child}); break;
                }
                case LangAPI::ExpressionValueType::ResetPosCounter:
                    if (counters.size() < 2) throw Unsupported{"unbalanced counter reset"};
                    counters.pop_back(); out += "(void)0"; break;
                case LangAPI::ExpressionValueType::EmptyInitializer: {     // `T{}`: a default-constructed value of the expected type
                    const auto expected = array_hint;
                    array_hint.reset();
                    if (!expected) throw Unsupported{"default initializer without a known type"};
                    std::vector<std::string> d;
                    const auto st = LangAPI::Typing::Typer::strip(*expected);
                    if (const auto *cls = typer->classOf(st)) {
                        out += handlerName(*cls) + "(ispa_cur)";
                    } else if (LangAPI::Typing::Typer::isValue(st, LangAPI::ValueType::Array)) out += vecName(typeName(*LangAPI::Typing::Typer::elementType(st), d)) + "_new()";
                    else if (LangAPI::Typing::Typer::isValue(st, LangAPI::ValueType::String)) out += "ISPA_STR(\"\")";
                    else if (LangAPI::Typing::Typer::isValue(st, LangAPI::ValueType::Any)) out += "ISPA_ANY_NONE";
                    else out += "(" + typeName(st, d) + "){0}";
                    break;
                }
                case LangAPI::ExpressionValueType::Lambda: {
                    const auto expected = array_hint;
                    array_hint.reset();
                    out += convertLambda(v.getLambda(), expected, false);
                    break;
                }
                case LangAPI::ExpressionValueType::Break: out += "break"; break;
                case LangAPI::ExpressionValueType::Continue: out += "continue"; break;
                case LangAPI::ExpressionValueType::Return: {
                    auto value = cur_return ? convertExprAs(v.getReturn().value, *cur_return) : convertExpr(v.getReturn().value);
                    if (cur_return && cur_return->isValueType() && cur_return->getValueType() == LangAPI::ValueType::Void && !value.empty()) {
                        out += "{ (void)(" + value + "); return; }";
                        break;
                    }
                    if (cur_return && !value.empty()) value = coerce(value, typer->typeOf(v.getReturn().value), *cur_return);
                    out += "return" + (value.empty() ? std::string() : " " + value);
                    break;
                }
                case LangAPI::ExpressionValueType::VariableAssignment: {
                    const auto &a = v.getVariableAssignment();
                    const char *op = "=";
                    switch (a.type) {
                        case LangAPI::OperatorType::Assign: op = "="; break;
                        case LangAPI::OperatorType::Add: op = "+="; break;
                        case LangAPI::OperatorType::Minus: op = "-="; break;
                        case LangAPI::OperatorType::Multiply: op = "*="; break;
                        case LangAPI::OperatorType::Divide: op = "/="; break;
                        case LangAPI::OperatorType::Modulo: op = "%="; break;
                    }
                    const bool sym_lhs = std::holds_alternative<LangAPI::Symbol>(a.name);
                    LangAPI::Typing::MaybeType lt;
                    if (sym_lhs) lt = typer->typeOf(LangAPI::RValue(std::get<LangAPI::Symbol>(a.name)));
                    else lt = typer->typeOfStorage(std::get<LangAPI::StorageSymbol>(a.name));
                    auto rhs = lt ? convertExprAs(a.value, *lt) : convertExpr(a.value);
                    if (lt && a.type == LangAPI::OperatorType::Assign) rhs = coerce(rhs, typer->typeOf(a.value), *lt);
                    // element assignment: v[i] = x  ->  NAME_set(v, i, x)
                    if (!sym_lhs) {
                        const auto &st = std::get<LangAPI::StorageSymbol>(a.name);
                        if (!st.path.empty() && std::holds_alternative<LangAPI::StorageOffset>(st.path.back())) {
                            std::vector<LangAPI::StorageSymbol::PathPart> kept(st.path.begin(), st.path.end() - 1);
                            LangAPI::StorageSymbol prefix(st.what, kept);
                            const auto idx = convertExpr(std::get<LangAPI::StorageOffset>(st.path.back()).offset);
                            const auto ct = typer->typeOfStorage(prefix);
                            if (!ct || !LangAPI::Typing::Typer::isValue(*ct, LangAPI::ValueType::Array)) throw Unsupported{"element assignment into a map/other container"};
                            std::vector<std::string> dd;
                            const auto vn = vecName(typeName(*LangAPI::Typing::Typer::elementType(*ct), dd));
                            const auto c = convertStorage(prefix);
                            const char *bin = a.type == LangAPI::OperatorType::Add ? "+" : a.type == LangAPI::OperatorType::Minus ? "-" :
                                              a.type == LangAPI::OperatorType::Multiply ? "*" : a.type == LangAPI::OperatorType::Divide ? "/" : "%";
                            if (a.type == LangAPI::OperatorType::Assign) out += vn + "_set(" + c + ", " + idx + ", " + rhs + ")";
                            else out += vn + "_set(" + c + ", " + idx + ", " + vn + "_get(" + c + ", " + idx + ") " + bin + " " + rhs + ")";
                            break;
                        }
                    }
                    const auto lhs = sym_lhs ? convertSymbolValue(std::get<LangAPI::Symbol>(a.name))
                                             : convertStorage(std::get<LangAPI::StorageSymbol>(a.name));
                    if (lt && a.type == LangAPI::OperatorType::Add && LangAPI::Typing::Typer::isValue(*lt, LangAPI::ValueType::String))
                        out += lhs + " = ISPA_CONCAT(" + lhs + ", " + rhs + ")";
                    else out += lhs + " " + op + " " + rhs;
                    break;
                }
                case LangAPI::ExpressionValueType::ExpressionElement: {
                    switch (v.getExpressionElement()) {
                        case EE::GroupOpen: out += "("; break;
                        case EE::GroupClose: out += ")"; break;
                        case EE::And: out += " && "; break;
                        case EE::Or: out += " || "; break;
                        case EE::Not: out += "!"; break;
                        case EE::Equal: out += " == "; break;
                        case EE::NotEqual: out += " != "; break;
                        case EE::Higher: out += " > "; break;
                        case EE::Lower: out += " < "; break;
                        case EE::HigherOrEqual: out += " >= "; break;
                        case EE::LowerOrEqual: out += " <= "; break;
                        case EE::Add: out += " + "; break;
                        case EE::Minus: out += " - "; break;
                        case EE::Multiply: out += " * "; break;
                        case EE::Divide: out += " / "; break;
                        case EE::Modulo: out += " % "; break;
                        case EE::PlusPlus: out += "++"; break;
                        case EE::MinusMinus: out += "--"; break;
                        default: throw Unsupported{"expression element"};
                    }
                    break;
                }
                default: throw Unsupported{"expression value kind " + std::to_string(static_cast<int>(v.type()))};
            }
        }
        return out;
    }


    // Precedence-climbing over the flat operand/operator list so operators can be chosen by operand type
    // (ispa_str has no C operators). Anything unexpected falls back to element-wise text.
    namespace {
        auto precOf(LangAPI::ExpressionElement e) -> int {
            using EE = LangAPI::ExpressionElement;
            switch (e) {
                case EE::Or: return 1; case EE::And: return 2;
                case EE::Equal: case EE::NotEqual: return 3;
                case EE::Higher: case EE::Lower: case EE::HigherOrEqual: case EE::LowerOrEqual: return 4;
                case EE::Add: case EE::Minus: return 5;
                case EE::Multiply: case EE::Divide: case EE::Modulo: return 6;
                default: return 0;
            }
        }
        auto opText(LangAPI::ExpressionElement e) -> const char * {
            using EE = LangAPI::ExpressionElement;
            switch (e) {
                case EE::Or: return "||"; case EE::And: return "&&"; case EE::Equal: return "=="; case EE::NotEqual: return "!=";
                case EE::Higher: return ">"; case EE::Lower: return "<"; case EE::HigherOrEqual: return ">="; case EE::LowerOrEqual: return "<=";
                case EE::Add: return "+"; case EE::Minus: return "-"; case EE::Multiply: return "*"; case EE::Divide: return "/";
                default: return "%";
            }
        }
    }

    auto Construct::parsePrimary(const LangAPI::Expression &e, std::size_t &i) -> std::string {
        using EE = LangAPI::ExpressionElement;
        if (i >= e.size()) throw Unsupported{"truncated expression"};
        const auto &v = e[i];
        std::string out;
        if (v.isExpressionElement()) {
            const auto el = v.getExpressionElement();
            if (el == EE::GroupOpen) {
                ++i;
                out = "(" + parseBinary(e, i, 1) + ")";
                if (i >= e.size() || !e[i].isExpressionElement() || e[i].getExpressionElement() != EE::GroupClose) throw Unsupported{"unbalanced group"};
                ++i;
            } else if (el == EE::Not) { ++i; out = "!" + parsePrimary(e, i); last_type = LangAPI::Type(LangAPI::ValueType::Bool); return out; }
            else if (el == EE::Minus) { ++i; out = "-" + parsePrimary(e, i); return out; }
            else if (el == EE::PlusPlus || el == EE::MinusMinus) { ++i; out = std::string(el == EE::PlusPlus ? "++" : "--") + parsePrimary(e, i); return out; }
            else throw Unsupported{"expression element"};
        } else {
            out = convertValue(v);
            if (v.isRvalue()) last_type = typer->typeOf(v.getRValue());
            else if (v.isFunctionCall()) last_type = typer->typeOf(v.getFunctionCall());
            else if (v.type() == LangAPI::ExpressionValueType::IspaLibFunctionCall &&
                     v.getIspaLibFunctionCall().symbol.exports == LangAPI::StdlibExports::FailureMessage) last_type = LangAPI::Type(LangAPI::ValueType::String);
            else last_type = std::nullopt;
            ++i;
        }
        while (i < e.size() && e[i].isExpressionElement() &&
               (e[i].getExpressionElement() == EE::PlusPlus || e[i].getExpressionElement() == EE::MinusMinus)) {
            out += e[i].getExpressionElement() == EE::PlusPlus ? "++" : "--"; ++i;
        }
        return out;
    }

    auto Construct::parseBinary(const LangAPI::Expression &e, std::size_t &i, int min_prec) -> std::string {
        auto lhs = parsePrimary(e, i);
        auto lt = last_type;
        while (i < e.size() && e[i].isExpressionElement()) {
            const auto el = e[i].getExpressionElement();
            const int p = precOf(el);
            if (p == 0 || p < min_prec) break;
            ++i;
            auto rhs = parseBinary(e, i, p + 1);
            const auto rt = last_type;
            const bool strs = lt && rt && LangAPI::Typing::Typer::isValue(*lt, LangAPI::ValueType::String) &&
                              LangAPI::Typing::Typer::isValue(*rt, LangAPI::ValueType::String);
            using EE = LangAPI::ExpressionElement;
            if (strs && el == EE::Add) { lhs = "ISPA_CONCAT(" + lhs + ", " + rhs + ")"; }
            else if (strs && el == EE::Equal) { lhs = "ispa_str_eq(" + lhs + ", " + rhs + ")"; lt = LangAPI::Type(LangAPI::ValueType::Bool); }
            else if (strs && p == 4) { lhs = "(ispa_str_cmp(" + lhs + ", " + rhs + ") " + opText(el) + " 0)"; lt = LangAPI::Type(LangAPI::ValueType::Bool); }
            else if (strs && el == EE::NotEqual) { lhs = "!ispa_str_eq(" + lhs + ", " + rhs + ")"; lt = LangAPI::Type(LangAPI::ValueType::Bool); }
            else {
                lhs = lhs + " " + opText(el) + " " + rhs;
                if (p <= 4) lt = LangAPI::Type(LangAPI::ValueType::Bool);
                if (p <= 4) lhs = "(" + lhs + ")";
            }
        }
        last_type = lt;
        return lhs;
    }

    auto Construct::convertExpr(const LangAPI::Expression &e) -> std::string {
        std::size_t i = 0;
        std::string out;
        const auto hint = array_hint;      // the expected type only applies to a sole operand
        array_hint.reset();
        if (e.size() == 1) array_hint = hint;
        struct Clear { LangAPI::Typing::MaybeType &h; ~Clear() { h.reset(); } } clear{array_hint};
        while (i < e.size()) {
            if (e[i].type() == LangAPI::ExpressionValueType::Empty) { ++i; continue; }
            out += parseBinary(e, i, 1);
            if (i < e.size()) out += " ";
        }
        return out;
    }

    auto Construct::convertStatements(const LangAPI::Statements &st, Converter::Writer &o) -> void {
        locals.emplace_back();
        typer->pushScope();
        for (std::size_t i = 0; i < st.size(); ++i) {
            const auto &s = st[i];
            pending_local_type.reset();
            if (s.isVariable()) {
                const auto &v = s.getVariable();
                const bool untyped = v.type.isValueType() && v.type.getValueType() == LangAPI::ValueType::Undef;
                if (untyped && v.value.size() == 1 && v.value[0].isRvalue() && v.value[0].getRValue().isArray() &&
                    v.value[0].getRValue().getArray().values.empty() && v.value[0].getRValue().getArray().template_parameters.empty()) {
                    // `x = []` : the element type is whatever the first x.append(...) adds
                    LangAPI::Statements tail(st.begin() + static_cast<std::ptrdiff_t>(i) + 1, st.end());
                    std::optional<LangAPI::Type> elem;
                    LangAPI::Traverse::forEach<LangAPI::StorageSymbol>(tail, [&](const LangAPI::StorageSymbol &ss) {
                        if (elem) return;
                        bool mine = false;
                        if (ss.what.size() == 1 && ss.what[0].isRvalue() && ss.what[0].getRValue().isSymbol()) {
                            const auto &p = ss.what[0].getRValue().getSymbol().path;
                            mine = p.size() == 1 && std::holds_alternative<std::string>(p[0]) && std::get<std::string>(p[0]) == v.name;
                        } else if (ss.what.empty() && !ss.path.empty()) {
                            const auto *n = std::get_if<std::string>(&ss.path[0]);
                            mine = n && *n == v.name;
                        }
                        if (!mine) return;
                        for (const auto &part : ss.path)
                            if (const auto *am = std::get_if<LangAPI::ArrayMethodCall>(&part))
                                if ((am->method == LangAPI::ArrayMethods::Push || am->method == LangAPI::ArrayMethods::PythonAppend) && am->args.size() == 1)
                                    if (const auto t = typer->typeOf(am->args[0])) { elem = LangAPI::Typing::Typer::strip(*t); return; }
                    });
                    if (elem) pending_local_type = LangAPI::Type(LangAPI::ValueType::Array, *elem);
                }
            }
            convertStatement(s, o);
        }
        pending_local_type.reset();
        typer->popScope();
        locals.pop_back();
    }

    auto Construct::convertStatement(const LangAPI::Statement &st, Converter::Writer &o) -> void {
        if (st.isVariable()) {
            const auto &v = st.getVariable();
            std::vector<std::string> d;
            auto declared = v.type;
            if (pending_local_type) { declared = *pending_local_type; pending_local_type.reset(); }
            const bool is_auto = declared.isSymbol() && declared.getSymbol().path.size() == 1 && std::holds_alternative<std::string>(declared.getSymbol().path[0]) &&
                                 (std::get<std::string>(declared.getSymbol().path[0]) == "auto" || std::get<std::string>(declared.getSymbol().path[0]) == "auto&");
            if (is_auto || (declared.isValueType() && declared.getValueType() == LangAPI::ValueType::Undef)) {
                const auto inferred = typer->typeOf(v.value);
                if (!inferred) throw Unsupported{"local without a declared type"};
                declared = *inferred;
            }
            typer->declare(v.name, declared);
            const auto t = typeName(declared, d);
            if (!d.empty()) throw Unsupported{"local type: " + d.front()};
            locals.back().insert(v.name);
            if (v.value.empty()) o.writeln("{} {} = {0};", t, cIdent(v.name));
            else o.writeln("{} {} = {};", t, cIdent(v.name), coerce(convertExprAs(v.value, declared), typer->typeOf(v.value), declared));
        } else if (st.isIf()) {
            const auto &i = st.getIf();
            o.writeln("if ({}) {", convertExpr(i.expr));
            o.increaseIndentation(); convertStatements(i.stmt, o); o.decreaseIndentation();
            if (!i.else_stmt.empty()) {
                o.writeln("} else {");
                o.increaseIndentation(); convertStatements(i.else_stmt, o); o.decreaseIndentation();
            }
            o.writeln("}");
        } else if (st.isWhile()) {
            const auto &w = st.getWhile();
            o.writeln("while ({}) {", convertExpr(w.expr));
            o.increaseIndentation(); convertStatements(w.stmt, o); o.decreaseIndentation();
            o.writeln("}");
        } else if (st.isDoWhile()) {
            const auto &w = st.getDoWhile();
            o.writeln("do {");
            o.increaseIndentation(); convertStatements(w.stmt, o); o.decreaseIndentation();
            o.writeln("} while ({});", convertExpr(w.expr));
        } else if (st.isExpression()) {
            o.writeln("{};", convertExpr(st.getExpression()));
        } else if (st.isComment()) {
            o.writeln("/* comment */");
        } else if (st.isThrow()) {
            const auto &tv = st.getThrow().throw_value;
            if (tv.size() == 1 && tv[0].type() == LangAPI::ExpressionValueType::IspaLibFunctionCall &&
                tv[0].getIspaLibFunctionCall().symbol.exports == LangAPI::StdlibExports::Error && tv[0].getIspaLibFunctionCall().args.size() == 1)
                o.writeln("ispa_fail(python_ispa_std_errors_ErrorKind_RuntimeFailure, {});", convertExpr(tv[0].getIspaLibFunctionCall().args[0]));
            else if (const auto tt = typer->typeOf(tv); tt && LangAPI::Typing::Typer::isValue(*tt, LangAPI::ValueType::String))
                o.writeln("ispa_fail(python_ispa_std_errors_ErrorKind_RuntimeFailure, {});", convertExpr(tv));
            else throw Unsupported{"throw of something other than Error(message): n=" + std::to_string(tv.size()) + " t0=" + (tv.empty() ? std::string("-") : std::to_string(static_cast<int>(tv[0].type()))) + (tv.size() && tv[0].type() == LangAPI::ExpressionValueType::IspaLibFunctionCall ? " lib=" + std::to_string(static_cast<int>(tv[0].getIspaLibFunctionCall().symbol.exports)) + " args=" + std::to_string(tv[0].getIspaLibFunctionCall().args.size()) : std::string())};
        } else if (st.isSwitch()) {
            const auto &sw = st.getSwitch();
            const auto st_type = typer->typeOf(sw.expression);
            const bool is_str = st_type && LangAPI::Typing::Typer::isValue(*st_type, LangAPI::ValueType::String);
            const auto tmp = "sw_" + std::to_string(switch_counter++);
            std::vector<std::string> d;
            const auto tn = st_type ? typeName(*st_type, d) : std::string("int64_t");
            o.writeln("{");
            o.increaseIndentation();
            o.writeln("{} {} = {};", tn, tmp, convertExpr(sw.expression));
            bool first = true;
            for (const auto &[val, body] : sw.cases) {
                const auto cv = convertRValue(val);
                o.writeln("{}if ({}) {", first ? "" : "else ", is_str ? "ispa_str_eq(" + tmp + ", " + cv + ")" : tmp + " == " + cv);
                LangAPI::Statements trimmed = body;     // a trailing `break` ends the case; in an if-chain it would leave the enclosing loop
                if (!trimmed.empty() && trimmed.back().isExpression() && trimmed.back().getExpression().size() == 1 &&
                    trimmed.back().getExpression()[0].type() == LangAPI::ExpressionValueType::Break) trimmed.pop_back();
                o.increaseIndentation(); convertStatements(trimmed, o); o.decreaseIndentation();
                o.writeln("}");
                first = false;
            }
            o.decreaseIndentation();
            o.writeln("}");
        } else {
            throw Unsupported{"statement kind"};
        }
    }

    auto Construct::emitFunction(const LangAPI::Function &fn, const std::string &owner, Converter::Writer &protos,
                                 Converter::Writer &impls) -> void {
        const auto pfx = prefix();
        std::vector<std::string> diag;
        if (!fn.template_parameters.empty()) {
            diagnostics.push_back(pfx + "." + fn.name + ": generic function not instantiated yet");
            return;
        }
        std::string sig = typeName(fn.type, diag) + " " + pfx + "_" + (owner.empty() ? "" : owner + "_") + cIdent(fn.name) + "(";
        std::vector<std::string> params;
        if (!owner.empty() && !fn.is_static) params.push_back(pfx + "_" + owner + "* self");
        for (const auto &[t, n] : fn.parameters) params.push_back(typeName(t, diag) + " " + cIdent(n));
        sig += params.empty() ? "void" : joined(params, ", ");
        sig += ")";
        protos.writeln("{};", sig);
        if (fn.is_native) { ++skipped_functions; return; }
        cur_class = owner.empty() ? std::string() : pfx + "_" + owner;
        locals.clear();
        locals.emplace_back();
        for (const auto &[t, n] : fn.parameters) locals.back().insert(n);
        typer->beginFunction(owner.empty() ? nullptr : cur_class_decl, fn);
        cur_return = &fn.type;
        Converter::Writer body;
        std::string why;
        try {
            body.increaseIndentation();
            counters = {"pos"};
            {   // position counters pushed anywhere in the body are declared up front (Python function scope)
                std::set<std::string> pushed;
                LangAPI::Traverse::forEach<LangAPI::PushPosCounter>(fn.statements, [&](const LangAPI::PushPosCounter &pp) { pushed.insert(pp.name); });
                for (const auto &n : pushed) {
                    body.writeln("python_ispa_std_lexer_TokenCursor* {} = NULL;", cIdent(n));
                    typer->declare(n, LangAPI::Type(LangAPI::Symbol(std::vector<std::string>{"python", "ispa_std", "lexer", "TokenCursor"})));
                    locals.back().insert(n);
                }
            }
            convertStatements(fn.statements, body);
        } catch (const Unsupported &u) { why = u.why; }
        if (!lambda_defs.empty()) { if (why.empty()) impls.dwrite("{}", lambda_defs); lambda_defs.clear(); }
        impls.writeln("{} {", sig);
        if (why.empty()) {
            impls.dwrite("{}", body.get());
            ++emitted_functions;
        } else {
            impls.increaseIndentation();
            impls.writeln("/* ISPA-C-TODO: {} */", why);
            impls.writeln("ISPA_UNIMPLEMENTED(\"{}\");", pfx + "." + fn.name);
            impls.decreaseIndentation();
            ++skipped_functions;
            ++reasons[why];
        }
        impls.writeln("}");
        for (auto &d : diag) diagnostics.push_back(pfx + "." + fn.name + ": " + d);
    }

    // Namespace-level variables: `extern T x;` in the header, a definition in the source. Literal initialisers are static
    // initialisers; anything else is assigned by <stem>_init_globals() (called by the facade before the first parse).
    auto Construct::emitGlobal(const LangAPI::Variable &v, Converter::Writer &, Converter::Writer &protos,
                               Converter::Writer &impls, const std::string &owner_c_name) -> void {
        std::vector<std::string> diag;
        const auto name = owner_c_name.empty() ? prefix() + "_" + cIdent(v.name) : owner_c_name + "_" + cIdent(v.name);
        const auto t = typeName(v.type, diag);
        for (auto &x : diag) diagnostics.push_back(name + ": " + x);
        protos.writeln("extern {} {};", t, name);
        const bool literal = v.value.size() == 1 && v.value[0].type() == LangAPI::ExpressionValueType::RValue &&
            (v.value[0].getRValue().isInt() || v.value[0].getRValue().isBool() || v.value[0].getRValue().isFloat());
        if (v.value.empty() || !literal) {
            impls.writeln("{} {};", t, name);
            if (!v.value.empty()) {
                cur_class.clear(); locals.clear(); locals.emplace_back();
                try { init_globals.push_back(name + " = " + convertExpr(v.value) + ";"); }
                catch (const Unsupported &u) { diagnostics.push_back(name + ": initialiser pending (" + u.why + ")"); }
            }
        } else impls.writeln("{} {} = {};", t, name, convertExpr(v.value));
    }

    // The application's token: lowered RawToken + the typed value. `create` is what the lexer's semantic actions call;
    // `to_node` is the hook through which the value reaches the AST node.
    auto Construct::emitTokenClass(const LangAPI::Class &cls, Converter::Writer &protos, Converter::Writer &impls) -> void {
        const auto c = class_cname.at(&cls);
        const auto raw = std::string("python_ispa_std_raw_token_RawToken");
        const auto node = std::string("python_ispa_std_node_Node");
        const auto base = "base_" + raw;
        protos.writeln("{}* {}_create(int64_t start_pos, ispa_str start, int64_t length, int64_t line, int64_t id, ispa_any value);", c, c);
        impls.writeln("{}* {}_create(int64_t start_pos, ispa_str start, int64_t length, int64_t line, int64_t id, ispa_any value) {", c, c);
        impls.increaseIndentation();
        impls.writeln("ispa_str whole = { start.data - start_pos, (size_t)start_pos + start.len };");
        impls.writeln("{}* core = {}_create(start_pos, whole, true, length, line, id, id);", raw, raw);
        impls.writeln("{}* t = {}(ispa_cur);", c, handler_names.at(&cls));
        impls.writeln("const void* vt = (const void*)t->{}.vt;", base);
        impls.writeln("t->{} = *core;", base);
        impls.writeln("t->{}.vt = (const void*)vt;", base);
        impls.writeln("t->value = value;");
        impls.writeln("return t;");
        impls.decreaseIndentation();
        impls.writeln("}");
        impls.writeln("{}* {}_to_node({}* self, int64_t type_id) {", node, c, c);
        impls.increaseIndentation();
        impls.writeln("{}* n = {}_to_node(&self->{}, type_id);", node, raw, base);
        impls.writeln("if (self->{}._type_id == type_id) n->_data = self->value;", base);
        impls.writeln("return n;");
        impls.decreaseIndentation();
        impls.writeln("}");
    }

    auto Construct::emitClass(const LangAPI::Class &cls, Converter::Writer &types, Converter::Writer &protos,
                              Converter::Writer &impls) -> void {
        const auto pfx = prefix();
        const auto name = symbolName(cls.name);
        const auto c_name = pfx + "_" + name;
        std::vector<std::string> diag;
        if (!cls.template_parameters.empty()) {
            // monomorphized on demand; the generic template itself has no C form
            diagnostics.push_back(c_name + ": generic class (" + std::to_string(cls.template_parameters.size()) + " parameters) not instantiated yet");
            return;
        }
        const auto *saved_decl = cur_class_decl;
        cur_class_decl = &cls;
        struct Restore { const LangAPI::Class *&r; const LangAPI::Class *v; ~Restore() { r = v; } } restore{cur_class_decl, saved_decl};
        const auto slots_it = root_slots.find(&cls);
        const bool is_root = slots_it != root_slots.end();
        const auto sigOf = [&](const LangAPI::Function &f, const std::string &self_t) {
            std::vector<std::string> ps = {self_t + "* self"};
            for (const auto &[t, n] : f.parameters) ps.push_back(typeName(t, diag) + " " + cIdent(n));
            return ps;
        };
        if (is_root) {
            types.writeln("typedef struct {}_vt {", c_name);
            types.increaseIndentation();
            for (const auto &m : slots_it->second) {
                const auto *f = definesMethod(cls, m);
                std::vector<std::string> ps = {c_name + "* self"};
                for (const auto &[t, n] : f->parameters) ps.push_back(typeName(t, diag));
                types.writeln("{} (*{})({});", typeName(f->type, diag), cIdent(m), joined(ps, ", "));
            }
            types.decreaseIndentation();
            types.writeln("} {}_vt;", c_name);
        }
        types.writeln("struct {} {", c_name);
        types.increaseIndentation();
        for (const auto &[vis, base] : cls.inherit_members)
            if (const auto *s = std::get_if<LangAPI::Symbol>(&base)) types.writeln("{} base_{};", resolve(*s, diag), symbolName(*s));
            else diagnostics.push_back(c_name + ": library base class not mapped");
        if (is_root) types.writeln("const {}_vt* vt;", c_name);
        for (const auto &[decl, vis] : cls.data)
            if (decl->isVariable() && !decl->getVariable().is_static) types.writeln("{} {};", typeName(decl->getVariable().type, diag), cIdent(decl->getVariable().name));
        types.decreaseIndentation();
        types.writeln("};");

        if (is_root)
            for (const auto &m : slots_it->second) {      // dispatcher: the one call form used for virtual methods
                const auto *f = definesMethod(cls, m);
                std::vector<std::string> ps = sigOf(*f, c_name), as = {"self"};
                for (const auto &[t, n] : f->parameters) as.push_back(cIdent(n));
                protos.writeln("static inline {} {}_{}_v({}) { {} self->vt->{}({}); }", typeName(f->type, diag), c_name, cIdent(m), joined(ps, ", "),
                               typeName(f->type, diag) == "void" ? "" : "return", cIdent(m), joined(as, ", "));
            }
        // vtable instances: one per vtable root this class takes part in
        std::vector<std::pair<const LangAPI::Class *, std::string>> installs;   // (root, member path to its vt)
        {
            std::set<const LangAPI::Class *> roots;
            for (const auto &[decl, vis] : cls.data)
                if (decl->isFunction())
                    if (const auto v = slot_root.find({&cls, decl->getFunction().name}); v != slot_root.end()) roots.insert(v->second);
            for (const auto *root : roots) {
                const auto rc = class_cname.at(root);
                auto path = pathToBase(cls, root);
                if (!path) { diagnostics.push_back(c_name + ": cannot reach vtable root " + rc); continue; }
                const auto vt = c_name + "_as_" + rc + "_vtable";
                std::vector<std::string> entries;
                for (const auto &m : root_slots.at(root)) {
                    const auto *owner = typer->methodOwner(cls, m);
                    const auto *f = typer->findMethod(cls, m);
                    const auto oc = class_cname.at(owner);
                    const auto thunk = c_name + "_" + cIdent(m) + "_as_" + rc;
                    std::vector<std::string> ps = sigOf(*f, rc), as = {"(" + oc + "*)self"};
                    for (const auto &[t, n] : f->parameters) as.push_back(cIdent(n));
                    impls.writeln("static {} {}({}) { {} {}_{}({}); }", typeName(f->type, diag), thunk, joined(ps, ", "),
                                  typeName(f->type, diag) == "void" ? "" : "return", oc, cIdent(m), joined(as, ", "));
                    entries.push_back("." + cIdent(m) + " = " + thunk);
                }
                impls.writeln("static const {}_vt {} = { {} };", rc, vt, joined(entries, ", "));
                path->push_back("vt");
                installs.emplace_back(root, vt + "\x01" + joined(*path, "."));
            }
        }
        protos.writeln("{}* {}_get_{}_handler(ispa_arena* arena);", c_name, pfx, name);
        protos.writeln("void {}_free_{}_handler({}* handler);", pfx, name, c_name);
        impls.writeln("{}* {}_get_{}_handler(ispa_arena* arena) {", c_name, pfx, name);
        impls.increaseIndentation();
        impls.writeln("{}* h = ({}*)ispa_arena_calloc(arena, sizeof({}));", c_name, c_name, c_name);
        if (!c_name.starts_with("python_ispa_std_"))      // application classes: list fields start as fresh empty lists (Python `= []`)
            for (const auto &[decl, vis] : cls.data)
                if (decl->isVariable() && !decl->getVariable().is_static && LangAPI::Typing::Typer::isValue(decl->getVariable().type, LangAPI::ValueType::Array)) {
                    std::vector<std::string> dd;
                    const auto tn = typeName(decl->getVariable().type, dd);
                    impls.writeln("h->{} = {}_new();", cIdent(decl->getVariable().name), tn.substr(0, tn.size() - 1));
                }
        for (const auto &[root, spec] : installs) {
            const auto cut = spec.find('\x01');
            impls.writeln("h->{} = &{};", spec.substr(cut + 1), spec.substr(0, cut));
        }
        impls.writeln("return h;");
        impls.decreaseIndentation();
        impls.writeln("}");
        impls.writeln("void {}_free_{}_handler({}* handler) { (void)handler; }", pfx, name, c_name);
        {   // <Class>_new(args): allocate + __init__ (what ISPA_STD::construct<Class>(args) lowers to)
            const LangAPI::Function *init = typer->findMethod(cls, "__init__");
            const LangAPI::Class *init_owner = init ? typer->methodOwner(cls, "__init__") : &cls;
            if (!init && cls.constructor) init = &*cls.constructor;
            std::vector<std::string> ps, as;
            if (init) for (const auto &[t, n] : init->parameters) { ps.push_back(typeName(t, diag) + " " + cIdent(n)); as.push_back(cIdent(n)); }
            protos.writeln("{}* {}_new({});", c_name, c_name, ps.empty() ? "void" : joined(ps, ", "));
            impls.writeln("{}* {}_new({}) {", c_name, c_name, ps.empty() ? "void" : joined(ps, ", "));
            impls.increaseIndentation();
            impls.writeln("{}* o = {}_get_{}_handler(ispa_cur);", c_name, pfx, name);
            if (init) {
                const auto oc = class_cname.find(init_owner);
                if (oc != class_cname.end()) {
                    as.insert(as.begin(), oc->second == c_name ? "o" : "(" + oc->second + "*)o");
                    impls.writeln("{}_{}({});", oc->second, cIdent(init->name), joined(as, ", "));
                }
            }
            impls.writeln("return o;");
            impls.decreaseIndentation();
            impls.writeln("}");
        }

        for (const auto &[decl, vis] : cls.data)
            if (decl->isVariable() && decl->getVariable().is_static) emitGlobal(decl->getVariable(), types, protos, impls, c_name);
        for (const auto &[decl, vis] : cls.data)
            if (decl->isFunction()) emitFunction(decl->getFunction(), name, protos, impls);
        if (cls.constructor) emitFunction(*cls.constructor, name, protos, impls);
        if (token_classes.contains(name) && !c_name.starts_with("python_ispa_std_")) { token_cname = c_name; emitTokenClass(cls, protos, impls); }
        for (auto &d : diag) diagnostics.push_back(c_name + ": " + d);
    }

    auto Construct::emitDeclarations(const LangAPI::Declarations &decls, Converter::Writer &types, Converter::Writer &protos,
                                     Converter::Writer &impls) -> void {
        // forward typedefs first so classes may reference each other
        for (const auto &d : decls)
            if (d.isClass() && d.getClass().template_parameters.empty()) {
                const auto n = prefix() + "_" + symbolName(d.getClass().name);
                fwd_w.writeln("typedef struct {} {};", n, n);
            }
        for (const auto &d : decls) {
            if (d.isNamespace()) {
                // application build: the lowered library is compiled separately (lowered/ispa_std.c); it stays indexed only
                const bool library = scope.empty() && d.getNamespace().name == "python";
                if (library && app_mode) continue;
                scope.push_back(d.getNamespace().name);
                emitDeclarations(d.getNamespace().declarations, types, protos, impls);
                scope.pop_back();
            } else if (d.isClass()) emitClass(d.getClass(), types, protos, impls);
            else if (d.isFunction()) emitFunction(d.getFunction(), "", protos, impls);
            else if (d.isVariable()) emitGlobal(d.getVariable(), types, protos, impls);
            else if (d.isEnum()) {
                std::vector<std::string> items;
                for (const auto &v : d.getEnum().value) items.push_back(prefix() + "_" + cIdent(d.getEnum().name) + "_" + cIdent(v));
                fwd_w.writeln("typedef enum { {} } {}_{};", joined(items, ", "), prefix(), cIdent(d.getEnum().name));
            }
        }
    }

    auto Construct::mapLibrary() -> void {
        owned_tree = *source;
        token_classes.clear();
        {   // `Token = RawToken<...>`: the application's token type becomes a class deriving the lowered RawToken that also carries the
            // typed value (the lowered core is untyped; RawToken.to_node is the hook through which the value reaches the AST node)
            const auto fix = [&](auto &&self, LangAPI::Declarations &decls) -> void {
                for (std::size_t i = 0; i < decls.size(); ++i) {
                    if (decls[i].isNamespace()) { self(self, decls[i].getNamespace().declarations); continue; }
                    if (!decls[i].isTypeAlias()) continue;
                    const auto &al = decls[i].getTypeAlias();
                    if (!(al.type.isValueType() && al.type.getValueType() == LangAPI::ValueType::RawToken)) continue;
                    LangAPI::Class cls;
                    cls.name = LangAPI::Symbol(al.name);
                    cls.inherit_members.push_back({LangAPI::Visibility::Public, LangAPI::Symbol(std::vector<std::string>{"python", "ispa_std", "raw_token", "RawToken"})});
                    LangAPI::Variable value;
                    value.name = "value";
                    value.type = LangAPI::Type(LangAPI::ValueType::Any);
                    cls.data.push_back({std::make_shared<LangAPI::Declaration>(value), LangAPI::Visibility::Public});
                    LangAPI::Function to_node;
                    to_node.name = "to_node";
                    to_node.type = LangAPI::Type(LangAPI::Symbol(std::vector<std::string>{"python", "ispa_std", "node", "Node"}));
                    to_node.parameters.push_back({LangAPI::Type(LangAPI::ValueType::Int), "type_id"});
                    to_node.is_native = true;       // emitted by hand (emitTokenClass)
                    cls.data.push_back({std::make_shared<LangAPI::Declaration>(to_node), LangAPI::Visibility::Public});
                    decls[i] = LangAPI::Declaration(cls);
                    token_classes.insert(al.name);
                }
            };
            fix(fix, owned_tree);
        }
        const auto mapType = [&](LangAPI::Type &t) {
            if (t.isValueType() && t.getValueType() == LangAPI::ValueType::Tuple && t.template_parameters.size() == 2) {
                LangAPI::Type pair(LangAPI::Symbol(std::vector<std::string>{"python", "ispa_std", "utility", "Pair"}));
                pair.template_parameters = t.template_parameters;
                t = std::move(pair);
                return;
            }
            if (t.isValueType()) {      // Node<T>, RawToken<...>, MatchResult<T> of the lowered library: payloads are erased
                const auto L = [&](const char *mod, const char *cls) { t = LangAPI::Type(LangAPI::Symbol(std::vector<std::string>{"python", "ispa_std", mod, cls})); };
                switch (t.getValueType()) {
                    case LangAPI::ValueType::Token: case LangAPI::ValueType::Rule: L("node", "Node"); return;
                    case LangAPI::ValueType::RawToken: L("raw_token", "RawToken"); return;
                    case LangAPI::ValueType::TokenResult: case LangAPI::ValueType::RuleResult: L("match_result", "MatchResult"); return;
                    case LangAPI::ValueType::Variant: case LangAPI::ValueType::LargeVariant: t = LangAPI::Type(LangAPI::ValueType::Any); return;
                    default: return;
                }
            }
            if (!t.isIspaLibSymbol()) return;
            if (const auto m = libType(t.getIspaLibSymbol())) t = *m;
        };
        LangAPI::Traverse::forEach<LangAPI::Type>(owned_tree, mapType);
        {   // type aliases (Types::X = FlatTypes::X, Token = RawToken<...>) are expanded in place
            std::unordered_map<std::string, std::vector<std::pair<std::vector<std::string>, LangAPI::Type>>> aliases;
            const auto gather = [&](auto &&self, const LangAPI::Declarations &decls, std::vector<std::string> ns) -> void {
                for (const auto &d : decls) {
                    if (d.isNamespace()) { auto in = ns; in.push_back(d.getNamespace().name); self(self, d.getNamespace().declarations, in); }
                    else if (d.isTypeAlias() && d.getTypeAlias().template_parameters.empty()) {
                        auto full = ns; full.push_back(d.getTypeAlias().name);
                        aliases[d.getTypeAlias().name].emplace_back(full, d.getTypeAlias().type);
                    }
                }
            };
            gather(gather, owned_tree, {});
            for (int round = 0; round < 6 && !aliases.empty(); ++round) {
                bool changed = false;
                const auto expandAlias = [&](LangAPI::Type &t) {
                    if (!t.isSymbol()) return;
                    std::vector<std::string> parts;
                    for (const auto &pp : t.getSymbol().path) { const auto *x = std::get_if<std::string>(&pp); if (!x) return; parts.push_back(*x); }
                    if (parts.empty()) return;
                    // the longest prefix of the path that names an alias (the whole path first)
                    for (std::size_t n = parts.size(); n >= 1; --n) {
                        const auto it = aliases.find(parts[n - 1]);
                        if (it == aliases.end()) continue;
                        for (const auto &[full, target] : it->second) {
                            if (n > full.size() || !std::equal(parts.begin(), parts.begin() + static_cast<std::ptrdiff_t>(n), full.end() - static_cast<std::ptrdiff_t>(n))) continue;
                            if (n == parts.size() && t.template_parameters.empty()) {
                                if (target.isSymbol() && target.getSymbol() == t.getSymbol()) return;
                                auto copy = target; t = std::move(copy); changed = true; return;
                            }
                            if (n < parts.size() && target.isSymbol()) {       // alias of a namespace: splice its target in front of the rest
                                std::vector<std::string> np;
                                for (const auto &pp : target.getSymbol().path) if (const auto *x = std::get_if<std::string>(&pp); x && *x != "_") np.push_back(*x);
                                np.insert(np.end(), parts.begin() + static_cast<std::ptrdiff_t>(n), parts.end());
                                auto targs = t.template_parameters;
                                t = LangAPI::Type(LangAPI::Symbol(np));
                                t.template_parameters = targs;
                                changed = true; return;
                            }
                        }
                    }
                };
                LangAPI::Traverse::forEach<LangAPI::Type>(owned_tree, expandAlias);
                // constructor calls `Alias(args)` name their class by a Symbol, not a Type
                LangAPI::Traverse::forEach<LangAPI::Inheritance>(owned_tree, [&](LangAPI::Inheritance &in) {
                    const auto *sy = std::get_if<LangAPI::Symbol>(&in.name);
                    if (!sy) return;
                    LangAPI::Type tmp(*sy);
                    const bool before = changed;
                    changed = false;
                    expandAlias(tmp);
                    if (changed) {
                        mapType(tmp);
                        if (tmp.isSymbol() && tmp.template_parameters.empty()) in.name = tmp.getSymbol();
                        else in.name = std::make_shared<LangAPI::Type>(tmp);
                    }
                    changed = changed || before;
                });
                if (!changed) break;
            }
        }
        // generated rules are templates over the cursor type (`IT: TokenCursor | TokenCursor`): the lowered cursors share one
        // base class, so a bounded parameter is simply that bound
        LangAPI::Traverse::forEach<LangAPI::Function>(owned_tree, [&](LangAPI::Function &f) {
            if (f.template_parameters.empty()) return;
            std::map<std::string, LangAPI::Type> sub;
            for (const auto &[n, bounds] : f.template_parameters) {
                if (bounds.empty()) return;
                sub.emplace(n, bounds[0]);
            }
            substitute(f, sub);
            f.template_parameters.clear();
        });
        LangAPI::Traverse::forEach<LangAPI::Class>(owned_tree, [&](LangAPI::Class &c) {
            for (auto &[vis, base] : c.inherit_members)
                if (const auto *lib = std::get_if<LangAPI::IspaLibSymbol>(&base))
                    if (const auto p = libPath(lib->exports)) base = LangAPI::Symbol(*p);
        });
    }

    auto Construct::construct() -> void {
        scope.clear();
        defined.clear();
        classes.clear();
        functions.clear();
        reasons.clear();
        generic_classes.clear();
        diagnostics.clear();
        init_globals.clear();
        thunks_done.clear();
        vec_defs.clear(); vec_seen.clear(); class_cname.clear(); fwd_w = {};
        mapLibrary();
        typer = std::make_unique<LangAPI::Typing::Typer>(owned_tree);
        typer->instantiate_class = [this](const LangAPI::Class &c, const std::vector<LangAPI::Type> &a) { return instantiateClass(c, a); };
        typer->instantiate_function = [this](const LangAPI::Function &f, const std::vector<LangAPI::Type> &a) { return instantiateFunction(f, a); };
        class_clones.clear(); fn_clones.clear(); class_insts.clear(); fn_insts.clear(); pending.clear();
        generic_decl.clear(); generic_scope.clear(); generic_fn_scope.clear();
        app_mode = false;
        for (const auto &d : owned_tree)
            if (!(d.isNamespace() && d.getNamespace().name == "python")) app_mode = true;
        collect(owned_tree);
        analyzeVirtuals();
        if (app_mode && std::getenv("ISPA_C_SURVEY"))     // library methods the application overrides (they need slots in the library build)
            for (const auto &[key, root] : slot_root) {
                const auto &cn = class_cname.at(key.first);
                if (!cn.starts_with("python_ispa_std_") && class_cname.at(root).starts_with("python_ispa_std_"))
                    std::cerr << "OVERRIDE " << class_cname.at(root) << "::" << key.second << " by " << cn << "\n";
            }
        emitted_functions = skipped_functions = 0;
        Converter::Writer types, protos, impls;
        emitDeclarations(owned_tree, types, protos, impls);
        while (!pending.empty()) {       // monomorphized instances discovered while emitting
            const auto p = pending.back();
            pending.pop_back();
            scope = p.scope;
            if (p.cls) emitClass(*p.cls, types, protos, impls); else emitFunction(*p.fn, "", protos, impls);
        }
        scope.clear();

        const auto stem = std::string(namespace_name);
        std::string header = "/* @generated by the ISPA C backend. Needs ispa_c.h (parser/stdlibs/target/c). */\n"
                             "#pragma once\n#include \"ispa_c.h\"\n\n";
        std::string vecs;
        for (const auto &[n, e] : vec_defs) vecs += "ISPA_DEFINE_VEC(" + n + ", " + e + ")\n";
        header += std::string(fwd_w.get()) + "\n" + vecs + "\n" + std::string(types.get()) + "\n" + std::string(protos.get());
        std::string source_text = "/* @generated by the ISPA C backend. */\n#include \"" + stem + ".h\"\n\n";
        for (const auto &d : diagnostics) source_text += "/* ISPA-C-WARNING: " + d + " */\n";
        source_text += "\n" + std::string(impls.get());
        header += "\nvoid " + stem + "_init_globals(void);\n";
        source_text += "\nvoid " + stem + "_init_globals(void) {\n";
        for (const auto &g : init_globals) source_text += "    " + g + "\n";
        source_text += "}\n";
        writable_files.clear();
        writable_files.emplace_back(stem + ".h", std::move(header));
        writable_files.emplace_back(stem + ".c", std::move(source_text));
        std::cerr << "C backend: " << emitted_functions << " function bodies emitted, " << skipped_functions << " pending, " << diagnostics.size() << " warnings\n";
        if (std::getenv("ISPA_C_SURVEY")) {
            std::vector<std::pair<std::size_t, std::string>> sorted;
            for (auto &[k, v] : reasons) sorted.emplace_back(v, k);
            std::ranges::sort(sorted, std::greater<>());
            for (auto &[n, k] : sorted) std::cerr << "SURVEY " << n << "  " << k << "\n";
        }
    }
}

extern "C" auto c_construct(const LangAPI::Declarations *tree, const char *namespace_name, bool incremental,
                            bool, bool) -> C::Construct* {
    return new C::Construct(tree, namespace_name, incremental);
}
extern "C" auto c_construct_finish(C::Construct *c) -> void { delete c; }
