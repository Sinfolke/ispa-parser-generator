module C.Init;

import Converter.Writer;
import logging;
import std;

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
                defined.insert(n);
                if (!d.getClass().template_parameters.empty()) generic_classes.insert(n);
                ClassInfo info; info.c_name = n;
                for (const auto &[vis, base] : d.getClass().inherit_members)
                    if (const auto *bs = std::get_if<LangAPI::Symbol>(&base)) { info.bases.emplace_back(symbolName(*bs), symbolName(*bs)); }
                for (const auto &[decl, vis] : d.getClass().data) {
                    if (decl->isVariable()) info.fields.insert(cIdent(decl->getVariable().name));
                    else if (decl->isFunction()) info.methods[decl->getFunction().name] = &decl->getFunction();
                }
                classes[n] = std::move(info);
            } else if (d.isEnum()) defined.insert(prefix() + "_" + cIdent(d.getEnum().name));
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
                case VT::Array: return "ispa_vec";
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
        if (!call.template_parameters.empty()) throw Unsupported{"call with template arguments"};
        const auto *sym = std::get_if<std::shared_ptr<LangAPI::Symbol>>(&call.name);
        if (!sym || !*sym) throw Unsupported{"library function call"};
        std::vector<std::string> args;
        for (const auto &a : call.args) args.push_back(convertExpr(a));
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
        if (r.isStorageSymbol()) {
            const auto &ss = r.getStorageSymbol();
            std::string out = ss.what.empty() ? std::string() : convertExpr(ss.what);
            std::string recvClass = ss.what.size() == 1 && ss.what[0].isRvalue() && ss.what[0].getRValue().isSymbol() &&
                                        ss.what[0].getRValue().getSymbol().path.size() == 1 && out == "self" ? cur_class : std::string();
            for (const auto &part : ss.path) {
                std::string field;
                if (const auto *prop = std::get_if<LangAPI::Property>(&part)) field = prop->name;
                else if (const auto *str = std::get_if<std::string>(&part)) field = *str;
                else throw Unsupported{"storage path with index/call"};
                if (!recvClass.empty()) {
                    const auto m = memberPath(recvClass, field);
                    if (!m) throw Unsupported{"unknown member " + field + " of " + recvClass};
                    out += "->" + *m; recvClass.clear();
                } else out += "->" + cIdent(field);
            }
            return out;
        }
        if (r.type() == LangAPI::RValueType::Instance) {
            if (cur_class.empty()) throw Unsupported{"instance outside a method"};
            return "self";
        }
        // Box<class> is already the object handle; a Box of a plain value will need a dereference once locals are typed
        if (r.type() == LangAPI::RValueType::GetFromBox) return convertExpr(r.getGetFromBox().what);
        throw Unsupported{"rvalue kind " + std::to_string(static_cast<int>(r.type()))};
    }

    auto Construct::convertExpr(const LangAPI::Expression &e) -> std::string {
        using EE = LangAPI::ExpressionElement;
        std::string out;
        for (const auto &v : e) {
            switch (v.type()) {
                case LangAPI::ExpressionValueType::Empty: break;
                case LangAPI::ExpressionValueType::RValue: out += convertRValue(v.getRValue()); break;
                case LangAPI::ExpressionValueType::FunctionCall: out += convertCall(v.getFunctionCall()); break;
                case LangAPI::ExpressionValueType::Break: out += "break"; break;
                case LangAPI::ExpressionValueType::Continue: out += "continue"; break;
                case LangAPI::ExpressionValueType::Return: {
                    const auto value = convertExpr(v.getReturn().value);
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
                    const auto lhs = std::holds_alternative<LangAPI::Symbol>(a.name)
                        ? convertSymbolValue(std::get<LangAPI::Symbol>(a.name))
                        : convertRValue(LangAPI::RValue(std::get<LangAPI::StorageSymbol>(a.name)));
                    out += lhs + " " + op + " " + convertExpr(a.value);
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

    auto Construct::convertStatements(const LangAPI::Statements &st, Converter::Writer &o) -> void {
        locals.emplace_back();
        for (const auto &s : st) convertStatement(s, o);
        locals.pop_back();
    }

    auto Construct::convertStatement(const LangAPI::Statement &st, Converter::Writer &o) -> void {
        if (st.isVariable()) {
            const auto &v = st.getVariable();
            std::vector<std::string> d;
            if (v.type.isValueType() && v.type.getValueType() == LangAPI::ValueType::Undef) throw Unsupported{"local without a declared type"};
            const auto t = typeName(v.type, d);
            if (!d.empty()) throw Unsupported{"local type: " + d.front()};
            locals.back().insert(v.name);
            if (v.value.empty()) o.writeln("{} {} = {0};", t, cIdent(v.name));
            else o.writeln("{} {} = {};", t, cIdent(v.name), convertExpr(v.value));
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
        cur_return = &fn.type;
        Converter::Writer body;
        std::string why;
        try {
            body.increaseIndentation();
            convertStatements(fn.statements, body);
        } catch (const Unsupported &u) { why = u.why; }
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
            if (decl->isVariable()) types.writeln("{} {};", typeName(decl->getVariable().type, diag), cIdent(decl->getVariable().name));
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
                types.writeln("typedef struct {} {};", n, n);
            }
        for (const auto &d : decls) {
            if (d.isNamespace()) {
                scope.push_back(d.getNamespace().name);
                emitDeclarations(d.getNamespace().declarations, types, protos, impls);
                scope.pop_back();
            } else if (d.isClass()) emitClass(d.getClass(), types, protos, impls);
            else if (d.isFunction()) emitFunction(d.getFunction(), "", protos, impls);
            else if (d.isEnum()) {
                std::vector<std::string> items;
                for (const auto &v : d.getEnum().value) items.push_back(prefix() + "_" + cIdent(d.getEnum().name) + "_" + cIdent(v));
                types.writeln("typedef enum { {} } {}_{};", joined(items, ", "), prefix(), cIdent(d.getEnum().name));
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
        collect(*source);
        emitted_functions = skipped_functions = 0;
        Converter::Writer types, protos, impls;
        emitDeclarations(*source, types, protos, impls);

        const auto stem = std::string(namespace_name);
        std::string header = "/* @generated by the ISPA C backend. Needs ispa_c.h (parser/stdlibs/target/c). */\n"
                             "#pragma once\n#include \"ispa_c.h\"\n\n";
        header += std::string(types.get()) + "\n" + std::string(protos.get());
        std::string source_text = "/* @generated by the ISPA C backend. */\n#include \"" + stem + ".h\"\n\n";
        for (const auto &d : diagnostics) source_text += "/* ISPA-C-WARNING: " + d + " */\n";
        source_text += "\n" + std::string(impls.get());
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
