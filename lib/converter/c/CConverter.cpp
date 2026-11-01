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
                if (const auto *s = std::get_if<std::string>(&p)) parts.push_back(cIdent(*s));
                else parts.push_back("_dyn");
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

    auto Construct::collect(const LangAPI::Declarations &decls) -> void {
        for (const auto &d : decls) {
            if (d.isNamespace()) {
                scope.push_back(d.getNamespace().name);
                collect(d.getNamespace().declarations);
                scope.pop_back();
            } else if (d.isClass()) {
                const auto n = prefix() + "_" + symbolName(d.getClass().name);
                class_cname[&d.getClass()] = n;
                defined.insert(n);
                if (!d.getClass().template_parameters.empty()) generic_classes.insert(n);
                ClassInfo info; info.c_name = n;
                for (const auto &[vis, base] : d.getClass().inherit_members)
                    if (const auto *bs = std::get_if<LangAPI::Symbol>(&base)) { info.bases.emplace_back(symbolName(*bs), symbolName(*bs)); }
                for (const auto &[decl, vis] : d.getClass().data) {
                    if (decl->isVariable() && decl->getVariable().is_static) functions.insert(n + "_" + cIdent(decl->getVariable().name));
                    else if (decl->isVariable()) info.fields.insert(cIdent(decl->getVariable().name));
                    else if (decl->isFunction()) { info.methods[decl->getFunction().name] = &decl->getFunction(); functions.insert(n + "_" + cIdent(decl->getFunction().name)); }
                }
                classes[n] = std::move(info);
            } else if (d.isEnum()) {
                defined.insert(prefix() + "_" + cIdent(d.getEnum().name));
                for (const auto &v : d.getEnum().value) functions.insert(prefix() + "_" + cIdent(d.getEnum().name) + "_" + cIdent(v));
            } else if (d.isFunction()) functions.insert(prefix() + "_" + cIdent(d.getFunction().name));
            else if (d.isVariable()) functions.insert(prefix() + "_" + cIdent(d.getVariable().name));
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
    auto Construct::typeName(const LangAPI::Type &type, std::vector<std::string> &diag) const -> std::string {
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
            if (!type.template_parameters.empty() || generic_classes.contains(n)) {
                diag.push_back("generic instantiation of " + n + " (monomorphization pending)");
                return "void*";
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

    auto Construct::convertCall(const LangAPI::FunctionCall &call) -> std::string {
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
        if (!call.template_parameters.empty()) {
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
        for (std::size_t k = 0; k < call.args.size(); ++k) {
            if (decl && k < decl->parameters.size()) {
                const auto &pt = decl->parameters[k].first;
                args.push_back(coerce(convertExprAs(call.args[k], pt), typer->typeOf(call.args[k]), pt));
            } else args.push_back(convertExpr(call.args[k]));
        }
        const auto &path = (*sym)->path;
        if (path.size() == 1 && std::holds_alternative<std::string>(path[0]) && !cur_class.empty()) {
            // implicit-self method call
            const auto &m = std::get<std::string>(path[0]);
            for (auto cls = cur_class; ;) {
                const auto it = classes.find(cls);
                if (it == classes.end() || !it->second.methods.contains(m)) break;
                std::string recv = "self";
                if (cls != cur_class) throw Unsupported{"call of inherited method " + m};
                args.insert(args.begin(), recv);
                return cls + "_" + cIdent(m) + "(" + joined(args, ", ") + ")";
            }
        }
        const auto fn = convertSymbolValue(**sym);
        return fn + "(" + joined(args, ", ") + ")";
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
        cur_return = &ret;
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
        def += "static " + typeName(ret, d) + " " + id + "(" + joined(ps, ", ") + ") {\n    " + id + "_env *env = (" + id + "_env *)env_; (void)env;\n" + std::string(body.get()) + "}\n";
        def += "static ispa_fn " + id + "_make(" + (make_params.empty() ? std::string("void") : make_params) + ") {\n    " + id + "_env *e = ISPA_NEW(" + id + "_env);\n" + make_assign + "    return (ispa_fn){ (void *)" + id + ", e };\n}\n\n";
        lambda_defs += def;
        return id + "_make(" + make_args + ")";
    }

    auto Construct::arrayLiteral(const LangAPI::Array &arr, const LangAPI::Type &type) -> std::string {
        const auto elem = LangAPI::Typing::Typer::elementType(type);
        if (!elem) throw Unsupported{"array literal into a non-array"};
        std::vector<std::string> d;
        const auto et = typeName(*elem, d);
        const auto vn = vecName(et);
        if (arr.values.empty()) return vn + "_new()";
        std::vector<std::string> items;
        for (const auto &v : arr.values) items.push_back(coerce(convertExpr(v), typer->typeOf(v), *elem));
        return vn + "_from((" + et + "[]){" + joined(items, ", ") + "}, " + std::to_string(items.size()) + ")";
    }

    // an expression whose expected type is known (initialiser, assignment, return): lets `[]` pick its element type
    auto Construct::convertExprAs(const LangAPI::Expression &e, const LangAPI::Type &expected) -> std::string {
        const auto saved = array_hint;
        array_hint = expected;
        struct R { LangAPI::Typing::MaybeType &r; LangAPI::Typing::MaybeType v; ~R() { r = v; } } restore{array_hint, saved};
        return convertExpr(e);
    }

    auto Construct::methodCall(const LangAPI::Class &recv_cls, const std::string &name, const std::string &recv,
                               std::vector<std::string> args) -> std::string {
        const auto *owner = typer->methodOwner(recv_cls, name);
        if (!owner) throw Unsupported{"method " + name + " not found"};
        const auto it = class_cname.find(owner);
        if (it == class_cname.end()) throw Unsupported{"class of method " + name + " unknown"};
        // TODO(vtable): Python methods are virtual; dispatch through the vtable once registration exists
        const auto cast = owner == &recv_cls ? recv : "(" + it->second + "*)" + recv;
        args.insert(args.begin(), cast);
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
                        if (a.size() != 1) throw Unsupported{"append arity"};
                        out = vn + "_push(" + out + ", " + a[0] + ")"; cur = LangAPI::Type(LangAPI::ValueType::Void); break;
                    case AM::Pop: case AM::SequencePopBack: out = vn + "_pop(" + out + ")"; cur = elemT; break;
                    case AM::PythonPop:
                        if (!a.empty()) throw Unsupported{"pop(index)"};
                        out = vn + "_pop(" + out + ")"; cur = elemT; break;
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
        if (r.isSymbol()) return convertSymbolValue(r.getSymbol());
        if (r.isStorageSymbol()) return convertStorage(r.getStorageSymbol());
        if (r.isArray()) {
            const auto &arr = r.getArray();
            if (!arr.template_parameters.empty())
                if (const auto *t = std::get_if<std::shared_ptr<LangAPI::Type>>(&arr.template_parameters[0]); t && *t)
                    return arrayLiteral(arr, LangAPI::Type(LangAPI::ValueType::Array, **t));
            if (arr.values.empty() && array_hint) return arrayLiteral(arr, *array_hint);
            throw Unsupported{"array literal of unknown element type"};
        }
        if (r.type() == LangAPI::RValueType::CharToStringConstructor) return "ispa_str_from_char(" + convertExpr(r.getCharToStringConstructor().what) + ")";
        if (r.type() == LangAPI::RValueType::Instance) {
            if (cur_class.empty()) throw Unsupported{"instance outside a method"};
            return "self";
        }
        // Box<class> is already the object handle; a Box of a plain value will need a dereference once locals are typed
        if (r.type() == LangAPI::RValueType::GetFromBox) return convertExpr(r.getGetFromBox().what);
        throw Unsupported{"rvalue kind " + std::to_string(static_cast<int>(r.type()))};
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
                        case LangAPI::StdlibExports::Attempt: {
                            if (c.args.size() != 1 || c.args[0].size() != 1 || c.args[0][0].type() != LangAPI::ExpressionValueType::Lambda)
                                throw Unsupported{"attempt of a non-lambda"};
                            out += "ispa_attempt_fn(" + convertLambda(c.args[0][0].getLambda(), std::nullopt, true) + ")"; break;
                        }
                        default: throw Unsupported{"ispa-lib call " + std::to_string(static_cast<int>(c.symbol.exports))};
                    }
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
                last_type = last_type;  // group keeps the type of its content
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
        for (const auto &s : st) convertStatement(s, o);
        typer->popScope();
        locals.pop_back();
    }

    auto Construct::convertStatement(const LangAPI::Statement &st, Converter::Writer &o) -> void {
        if (st.isVariable()) {
            const auto &v = st.getVariable();
            std::vector<std::string> d;
            auto declared = v.type;
            if (declared.isValueType() && declared.getValueType() == LangAPI::ValueType::Undef) {
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
        } else {
            throw Unsupported{st.isSwitch() ? "switch statement" : st.isThrow() ? "throw (error model pending)" : "statement kind"};
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
        std::vector<const LangAPI::Function *> virtuals;
        for (const auto &[decl, vis] : cls.data)
            if (decl->isFunction() && decl->getFunction().is_virtual) virtuals.push_back(&decl->getFunction());

        if (!virtuals.empty()) {
            types.writeln("typedef struct {}_vt {", c_name);
            types.increaseIndentation();
            for (const auto *f : virtuals) {
                std::vector<std::string> ps = {c_name + "* self"};
                for (const auto &[t, n] : f->parameters) ps.push_back(typeName(t, diag));
                types.writeln("{} (*{})({});", typeName(f->type, diag), cIdent(f->name), joined(ps, ", "));
            }
            types.decreaseIndentation();
            types.writeln("} {}_vt;", c_name);
        }
        types.writeln("struct {} {", c_name);
        types.increaseIndentation();
        for (const auto &[vis, base] : cls.inherit_members)
            if (const auto *s = std::get_if<LangAPI::Symbol>(&base)) types.writeln("{} base_{};", resolve(*s, diag), symbolName(*s));
            else diagnostics.push_back(c_name + ": library base class not mapped");
        if (!virtuals.empty()) types.writeln("const {}_vt* vt;", c_name);
        for (const auto &[decl, vis] : cls.data)
            if (decl->isVariable() && !decl->getVariable().is_static) types.writeln("{} {};", typeName(decl->getVariable().type, diag), cIdent(decl->getVariable().name));
        types.decreaseIndentation();
        types.writeln("};");

        protos.writeln("{}* {}_get_{}_handler(ispa_arena* arena);", c_name, pfx, name);
        protos.writeln("void {}_free_{}_handler({}* handler);", pfx, name, c_name);
        impls.writeln("{}* {}_get_{}_handler(ispa_arena* arena) {", c_name, pfx, name);
        impls.increaseIndentation();
        impls.writeln("{}* h = ({}*)ispa_arena_calloc(arena, sizeof({}));", c_name, c_name, c_name);
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

    auto Construct::construct() -> void {
        scope.clear();
        defined.clear();
        classes.clear();
        functions.clear();
        reasons.clear();
        generic_classes.clear();
        diagnostics.clear();
        init_globals.clear();
        vec_defs.clear(); vec_seen.clear(); class_cname.clear(); fwd_w = {};
        typer = std::make_unique<LangAPI::Typing::Typer>(*source);
        collect(*source);
        emitted_functions = skipped_functions = 0;
        Converter::Writer types, protos, impls;
        emitDeclarations(*source, types, protos, impls);

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
