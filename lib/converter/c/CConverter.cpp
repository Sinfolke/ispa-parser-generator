module C.Init;

import Converter.Writer;
import logging;
import std;

namespace C {
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
        impls.writeln("{} {", sig);
        impls.increaseIndentation();
        impls.writeln("/* ISPA-C-TODO: body not lowered yet ({} statements) */", fn.statements.size());
        impls.writeln("ISPA_UNIMPLEMENTED(\"{}\");", pfx + "." + fn.name);
        impls.decreaseIndentation();
        impls.writeln("}");
        ++skipped_functions;
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
        std::cerr << "C backend: " << skipped_functions << " function bodies pending, " << diagnostics.size() << " warnings\n";
    }
}

extern "C" auto c_construct(const LangAPI::Declarations *tree, const char *namespace_name, bool incremental,
                            bool, bool) -> C::Construct* {
    return new C::Construct(tree, namespace_name, incremental);
}
extern "C" auto c_construct_finish(C::Construct *c) -> void { delete c; }
