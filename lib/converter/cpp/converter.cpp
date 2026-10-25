module;
#include "tools/LibraryDeps.hpp"   // constexpr view of ispastdlib_deps.json (tools/ispa_deps.py)
module Cpp.Init;
import Cpp.CoreFunctions;
import corelib;
import Converter.Writer;
import dstd;
import std;
namespace Cpp {
    // converter.cpp — add near the top, file-local
    auto symbolToPath(const LangAPI::Symbol &sym) -> std::string {
        std::string path;
        for (const auto &part : sym.path) {
            if (!std::holds_alternative<std::string>(part))
                continue; // function-call / lib-symbol path parts can't name a file
            if (!path.empty()) path += '/';
            path += std::get<std::string>(part);
        }
        return path;
    }
    auto symbolToVec(const LangAPI::Symbol &sym) -> std::vector<std::string> {
        std::vector<std::string> path;
        for (const auto &part : sym.path) {
            if (!std::holds_alternative<std::string>(part))
                continue; // function-call / lib-symbol path parts can't name a file
            path.push_back(std::get<std::string>(part));
        }
        return path;
    }
    auto generateMembers(
        Converter::Writer& o,
        const std::vector<std::string>& fields,
        const char* interface_name,
        std::string prefix_for_functions = ""
    ) -> void {
        for (const auto is_const : {false, true}) {
            o.writeln("constexpr auto {}(){} {", interface_name, is_const ? " const" : "");
            o.increaseIndentation();

            o.write("return std::{}(", prefix_for_functions.empty() ? "tie" : "make_tuple");
            for (std::size_t i = 0; i < fields.size(); ++i) {
                if (i) o.dwrite(", ");
                if (!prefix_for_functions.empty())
                    o.dwrite("&{}::", prefix_for_functions);
                o.dwrite(fields[i]);
            }
            o.dwriteln(");");

            o.decreaseIndentation();
            o.writeln("}");
        }
    }
    auto generateMembersStr(
        Converter::Writer& o,
        const std::vector<std::string>& fields,
        const char* interface_name
    ) -> void {
        o.writeln("static constexpr auto {}_str() {", interface_name);
        o.increaseIndentation();

        o.write("return std::array<std::string_view, {}>{{", fields.size());

        for (std::size_t i = 0; i < fields.size(); ++i) {
            if (i) o.dwrite(", ");
            o.dwrite("\"{}\"", fields[i]);
        }

        o.dwriteln("}};");

        o.decreaseIndentation();
        o.writeln("}");
    }
    // ------------------------------------------------------------------------
    // File-local helpers.
    //
    // Writer conventions relied on here:
    //   write/writeln   -> start-of-line text, indentation applied
    //   dwrite/dwriteln -> raw text, NO indentation, braces are NOT unescaped
    // Every line is therefore emitted as writeln("{}", <fully built string>):
    // the text goes through the formatter as an *argument*, so braces in it can
    // never be mis-escaped ("}}" leaking into output) and indentation is always applied.
    // ------------------------------------------------------------------------
    namespace {
        auto emitLine(Converter::Writer &w, const std::string &s) -> void {
            w.writeln("{}", s);
        }

        auto joinStrings(const std::vector<std::string> &items, const std::string &sep) -> std::string {
            std::string out;
            for (std::size_t i = 0; i < items.size(); ++i) {
                if (i) out += sep;
                out += items[i];
            }
            return out;
        }

        // A class whose Symbol has more than one component (Types::rule::value::group::AUTO_1) is a nested
        // structure: {prefix = rule, value, group ; leaf = AUTO_1} relative to the scope it is declared in.
        // Everything that registers / looks up / places such a class (known-table, file path, symbol_path)
        // must use prefix + leaf, never the joined string as a single name.
        struct SplitName {
            std::vector<std::string> prefix;
            std::string leaf;
        };
        // Split the text Core::convertSymbol produced (`text`), because that is exactly what every use of the
        // class (alias / base / member types, all printed by Core::convertType) is built from. symbolToVec drops
        // non-string path parts, which made the registered path differ from the path the lookups use.
        auto splitClassName(const LangAPI::Class &, const std::string &text) -> SplitName {
            std::vector<std::string> parts;
            std::string cur;
            int angle = 0;
            for (std::size_t i = 0; i < text.size(); ++i) {
                const char c = text[i];
                if (c == '<') ++angle;
                else if (c == '>' && angle > 0) --angle;
                if (angle == 0 && c == ':' && i + 1 < text.size() && text[i + 1] == ':') {
                    if (!cur.empty()) parts.push_back(std::move(cur));   // a leading "::" yields no empty part
                    cur.clear();
                    ++i;
                } else {
                    cur += c;
                }
            }
            if (!cur.empty()) parts.push_back(std::move(cur));
            if (parts.empty()) return {{}, text};
            SplitName out;
            out.leaf = std::move(parts.back());
            parts.pop_back();
            out.prefix = std::move(parts);
            return out;
        }

        auto isIdentChar(char c) -> bool {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
        }

        // Replaces whole identifiers only (IT -> X, but not ITER, not Foo::IT).
        auto replaceIdentifier(const std::string &text, const std::string &ident, const std::string &repl) -> std::string {
            if (ident.empty()) return text;
            std::string out;
            std::size_t i = 0;
            while (i < text.size()) {
                const bool match = text.compare(i, ident.size(), ident) == 0;
                const bool left_ok = i == 0 || (!isIdentChar(text[i - 1]) && !(i >= 2 && text[i - 1] == ':' && text[i - 2] == ':'));
                const bool right_ok = i + ident.size() >= text.size() || !isIdentChar(text[i + ident.size()]);
                if (match && left_ok && right_ok) {
                    out += repl;
                    i += ident.size();
                } else {
                    out += text[i++];
                }
            }
            return out;
        }

        // "public" / "private" (no colon) — valid in a base-clause, unlike getDefaultVisibility ("struct"/"class").
        auto accessName(LangAPI::Visibility visibility) -> std::string {
            switch (visibility) {
                case LangAPI::Visibility::Public:
                    return "public";
                case LangAPI::Visibility::Private:
                    return "private";
                case LangAPI::Visibility::Protected:
                    return "protected";
                default:
                    throw std::runtime_error("Cpp Converter only supports Public, Protected and Private visibility yet");
            }
        }

        enum class SigKind {
            Declaration,   // template<typename T, class = enable_if...> [static] auto f(T x) [const] -> R [override]
            Definition,    // template<typename T, class>               auto Scope::f(T x) [const] -> R
            Instantiation  //                                           auto Scope::f<A>(A x) [const] -> R'   (caller adds "template"/"extern template")
        };

        auto buildSignature(const LangAPI::Function &func, SigKind kind, const std::string &qualifier,
                            bool keep_static, const std::vector<std::string> &targs, bool constructor = false) -> std::string {
            const auto &tps = func.template_parameters;
            std::string out;

            if (!tps.empty() && kind != SigKind::Instantiation) {
                std::vector<std::string> parts;
                for (const auto &p : tps)
                    parts.push_back("typename " + p.first);
                for (const auto &p : tps) {
                    if (p.second.empty()) continue;
                    std::string conds;
                    for (std::size_t i = 0; i < p.second.size(); ++i) {
                        if (i) conds += " || ";
                        conds += "std::is_same_v<std::remove_const_t<std::remove_reference_t<" + p.first + ">>, "
                                 + Core::convertType(p.second[i]) + ">";
                    }
                    // Default template arguments may only appear on the declaration, never on the out-of-class definition.
                    parts.push_back(kind == SigKind::Declaration ? "class = std::enable_if_t<" + conds + ">" : "class");
                }
                out += "template<" + joinStrings(parts, ", ") + "> ";   // closed exactly once
            }

            // `static` is only valid on the in-class / in-header declaration, `override` likewise.
            if (kind == SigKind::Declaration && keep_static && func.is_static)
                out += "static ";

            out += (constructor ? "" : "auto ") + qualifier + func.name;
            if (kind == SigKind::Instantiation)
                out += std::string(func.name.back() == '<' ? " <" : "<") + joinStrings(targs, ", ") + ">";   // `operator< <T>`

            std::vector<std::string> params;
            for (const auto &p : func.parameters)
                params.push_back(Core::convertType(p.first) + " " + p.second);
            std::string plist = joinStrings(params, ", ");
            std::string ret = Core::convertType(func.type);

            if (kind == SigKind::Instantiation) {
                // Substitute each template parameter name with its concrete type.
                for (std::size_t i = 0; i < tps.size() && i < targs.size(); ++i) {
                    plist = replaceIdentifier(plist, tps[i].first, targs[i]);
                    ret = replaceIdentifier(ret, tps[i].first, targs[i]);
                }
            }

            out += "(" + plist + ")";
            if (func.is_const) out += " const";
            if (!constructor) out += " -> " + ret;
            if (kind == SigKind::Declaration && func.override) out += " override";
            return out;
        }

        // Cartesian product of the allowed types of every template parameter.
        auto instantiationSets(const LangAPI::Function &func) -> std::vector<std::vector<std::string>> {
            std::vector<std::vector<std::string>> sets{{}};
            for (const auto &p : func.template_parameters) {
                std::vector<std::vector<std::string>> next;
                for (const auto &base : sets) {
                    for (const auto &t : p.second) {
                        auto s = base;
                        s.push_back(Core::convertType(t));
                        next.push_back(std::move(s));
                    }
                }
                sets = std::move(next);
            }
            return sets;
        }

        // --- dependency handling -------------------------------------------------
        // How an occurrence of a name in a piece of generated text counts as a dependency:
        //   Strong      complete type / value needed (bodies, initialisers, aliases)  -> #include
        //   Weak        a declaration is enough (extern lines)                        -> forward declaration if possible
        //   Declaration class / function declarations: names inside ( ... ), after "->", and in template<...>
        //               are Weak, everything else Strong; declarator names (members, parameters, functions)
        //               are not dependencies at all
        enum class DepMode { Strong, Declaration, Weak };
        struct DepPart {
            const std::string &text;
            DepMode mode;
        };
        // One (possibly qualified) name: `A::B::c` -> chain {A, B, c}.
        struct Use {
            std::vector<std::string> chain;
            bool global = false;   // starts with "::" (or is the declarator of a definition, `auto A::f`)
            bool strong = false;   // needs the complete definition
        };

        auto isKeywordBeforeName(const std::string &id) -> bool {
            static const std::unordered_set<std::string> kw{
                "public", "private", "protected", "struct", "class", "enum", "typename", "using", "namespace",
                "return", "new", "delete", "throw", "case", "else", "do", "goto", "sizeof", "typedef", "template", "union"};
            return kw.contains(id);
        }

        // Names used in `t`, one entry per distinct qualified name, in order of first appearance. Skipped: string/char
        // literals, // comments, numbers, member accesses (x.name / x->name) and the names in `namespace A::B {`.
        auto scanDependencies(const std::string &t, DepMode mode) -> std::vector<Use> {
            std::vector<Use> out;
            std::unordered_map<std::string, std::size_t> index;
            const std::size_t n = t.size();
            const auto isSpace = [](char c) { return c == ' ' || c == '\n' || c == '\t' || c == '\r'; };
            int depth = 0;          // ( ) nesting
            int angle = 0;          // < > nesting inside "template<" ... ">"
            bool template_head = false;
            bool trailing = false;  // between "-> " and the end of the declaration
            bool ns_decl = false;   // inside `namespace ... {` / `using namespace ...;`
            std::string last_id;    // previous token if it was an identifier
            std::size_t i = 0;
            while (i < n) {
                const char c = t[i];
                if (c == '"' || c == '\'') {
                    ++i;
                    while (i < n && t[i] != c) { if (t[i] == '\\') ++i; ++i; }
                    ++i;
                    last_id.clear();
                } else if (c == '/' && i + 1 < n && t[i + 1] == '/') {
                    while (i < n && t[i] != '\n') ++i;
                } else if (isIdentChar(c)) {
                    std::size_t j = i;
                    while (j < n && isIdentChar(t[j])) ++j;
                    if (c >= '0' && c <= '9') {            // numbers are not names
                        last_id.clear();
                        i = j;
                        continue;
                    }
                    const std::string id = t.substr(i, j - i);
                    if (id == "namespace") ns_decl = true;
                    if (id == "template" && j < n && t[j] == '<') template_head = true;

                    const bool member = (i >= 1 && t[i - 1] == '.') || (i >= 2 && t[i - 1] == '>' && t[i - 2] == '-');
                    const bool after_scope = i >= 2 && t[i - 1] == ':' && t[i - 2] == ':';
                    bool global = false;
                    bool chain_start = true;
                    if (after_scope) {
                        const char b = i >= 3 ? t[i - 3] : '\0';
                        if (!isIdentChar(b) && b != '>') global = true;      // leading "::"
                        else chain_start = false;                            // continuation, handled at the chain start
                    }
                    if (member || ns_decl || !chain_start) {
                        last_id = id;
                        i = j;
                        continue;
                    }

                    std::vector<std::string> comps{id};
                    std::size_t k = j;
                    while (k + 2 < n && t[k] == ':' && t[k + 1] == ':' && isIdentChar(t[k + 2]) && !(t[k + 2] >= '0' && t[k + 2] <= '9')) {
                        std::size_t e = k + 2;
                        while (e < n && isIdentChar(t[e])) ++e;
                        comps.push_back(t.substr(k + 2, e - k - 2));
                        k = e;
                    }
                    if (comps.size() > 1 && !after_scope && last_id == "auto") global = true;   // `auto A::f(...)`

                    bool declared = false;
                    if (mode == DepMode::Declaration && comps.size() == 1) {
                        std::size_t q = j;
                        while (q < n && isSpace(t[q])) ++q;
                        const char nx = q < n ? t[q] : '\0';
                        std::size_t p = i;
                        while (p > 0 && isSpace(t[p - 1])) --p;
                        const char pv = p > 0 ? t[p - 1] : '\0';
                        const bool arrow = pv == '>' && p >= 2 && t[p - 2] == '-';
                        const bool prev_decl = p > 0 && !arrow && (isIdentChar(pv) || pv == '>' || pv == '&' || pv == '*');
                        const bool prev_kw = isIdentChar(pv) && isKeywordBeforeName(last_id);
                        if (prev_decl && !prev_kw) {
                            if (nx == ',' || nx == ')' || nx == ';' || nx == '=' || nx == '{' || nx == '[') declared = true;
                            else if (nx == '(' && last_id == "auto") declared = true;
                        }
                    }
                    if (!declared) {
                        const bool strong = mode == DepMode::Strong
                            || (mode == DepMode::Declaration && depth == 0 && !trailing && angle == 0);
                        std::string key = global ? "::" : "";
                        for (const auto &comp : comps) key += comp + "::";
                        const auto it = index.find(key);
                        if (it == index.end()) {
                            index.emplace(key, out.size());
                            out.push_back(Use{comps, global, strong});
                        } else {
                            out[it->second].strong = out[it->second].strong || strong;
                        }
                    }
                    last_id = id;
                    i = j;
                } else {
                    if (c == '(') ++depth;
                    else if (c == ')') { if (depth > 0) --depth; }
                    else if (c == ';' || c == '{') { trailing = false; ns_decl = false; }
                    else if (c == '<' && template_head) ++angle;
                    else if (c == '>' && angle > 0 && !(i >= 1 && t[i - 1] == '-')) {
                        if (--angle == 0) template_head = false;
                    }
                    if (c == '-' && i + 2 < n && t[i + 1] == '>' && t[i + 2] == ' ') {
                        trailing = true;                    // " -> Type": trailing return type
                        i += 3;
                        last_id.clear();
                        continue;
                    }
                    if (!isSpace(c)) last_id.clear();
                    ++i;
                }
            }
            return out;
        }

        // Scopes a name is looked up in, innermost first (a global chain only from the root).
        auto lookupLengths(bool global, const std::vector<std::string> &scope) -> std::vector<std::size_t> {
            std::vector<std::size_t> lens;
            if (global) lens.push_back(0);
            else for (std::size_t l = scope.size() + 1; l-- > 0;) lens.push_back(l);
            return lens;
        }

        // Which declaration(s) does the chain name, looked up like C++ does: the first scope (innermost outwards) in
        // which the full path exists decides; if that path is a namespace the chain names nothing we generate.
        auto resolveEntity(const Context &ctx, const std::vector<std::string> &chain, bool global,
                           const std::vector<std::string> &scope) -> std::vector<const SymbolInfo *> {
            std::vector<const SymbolInfo *> found;
            const auto it = ctx.known.find(chain.back());
            if (it == ctx.known.end()) return found;
            for (const std::size_t len : lookupLengths(global, scope)) {
                std::vector<std::string> full(scope.begin(), scope.begin() + static_cast<std::ptrdiff_t>(len));
                full.insert(full.end(), chain.begin(), chain.end());
                for (const auto &info : it->second) {
                    std::vector<std::string> path = info.ns;
                    path.push_back(info.name);
                    if (path == full) found.push_back(&info);
                }
                if (!found.empty() || ctx.known_namespaces.contains(joinStrings(full, "::"))) return found;
            }
            // not visible by scope (e.g. a using-directive): fall back to the name if it is unambiguous
            if (chain.size() == 1 && !global && it->second.size() == 1) found.push_back(&it->second.front());
            // qualified name that matched no scope exactly: accept declarations whose full path ends with the chain
            if (found.empty() && chain.size() > 1 && !global) {
                for (const auto &info : it->second) {
                    std::vector<std::string> path = info.ns;
                    path.push_back(info.name);
                    if (path.size() >= chain.size() && std::equal(chain.rbegin(), chain.rend(), path.rbegin()))
                        found.push_back(&info);
                }
            }
            return found;
        }

        // ---- standard library chunks ---------------------------------------------
        // ispastdlib.hpp is split into ispastdlib/<chunk>.hpp. Which chunk defines which name, and which chunk includes
        // which, is NOT hand-written here: tools/ispa_deps.py scans the library and writes ispastdlib_deps.json, which
        // LibraryDeps.hpp parses at compile time (constexpr). A generated file gets ispastdlib/core.hpp plus the chunks
        // whose names its text uses, reduced to the minimal include set (a chunk another chunk already includes is dropped).
        auto libraryIncludes(const std::vector<std::string_view> &texts) -> std::vector<std::string> {
            std::vector<std::string_view> wanted{"ispastdlib/core.hpp"};
            for (const auto text : texts) {
                const std::string t(text);
                for (const auto &u : scanDependencies(t, DepMode::Strong)) {
                    if (const auto h = LibraryDeps::header_of(u.chain)) wanted.push_back(*h);
                    // a library name the scan does not know (library newer than the JSON): take everything
                    else if (LibraryDeps::in_library_namespace(u.chain)) wanted.push_back(LibraryDeps::umbrella);
                }
                // generic `os << variant` lives in its own opt-in chunk (it instantiates every alternative)
                if (LibraryDeps::has_file("ispastdlib/variant_io.hpp") && t.find("std::variant") != std::string::npos
                    && t.find("<<") != std::string::npos)
                    wanted.push_back("ispastdlib/variant_io.hpp");
            }
            std::vector<std::string> out;
            for (const auto h : LibraryDeps::minimal_includes(wanted)) out.emplace_back(h);
            return out;
        }
        auto libraryIncludeBlock(const std::vector<std::string_view> &texts) -> std::string {
            std::string out;
            for (const auto &inc : libraryIncludes(texts)) out += "#include <" + inc + ">\n";
            return out + "\n";
        }

        // `#pragma once` (headers), the library chunks, forward declarations and #include <...> lines for what the file needs.
        //  * Weak use of a class                     -> forward declaration only
        //  * Strong use of a class                   -> #include (complete type) plus a forward declaration placed
        //                                               before the includes, so that classes which refer to each other
        //                                               through pointers / shared_ptr still compile
        //  * class used as qualifier (Lexer::x)      -> #include
        //  * enums / aliases / nested types          -> #include (can't be forward declared)
        // Names are resolved against `scope` (the scope the text lives in), so a name only matches the declaration
        // C++ would pick: `LBRACE` inside Parser::FlatTypes is FlatTypes::LBRACE, not Types::LBRACE, and the word
        // Parser in `namespace Parser {` or `Parser::FlatTypes::X` is the namespace, not class Parser::Parser.
        auto buildPrefix(const Context &ctx, const std::string &file, const std::vector<DepPart> &parts,
                         bool is_header, const std::string &extra, const std::vector<std::string> &scope) -> std::string {
            std::vector<Use> uses;
            std::unordered_map<std::string, std::size_t> index;
            for (const auto &part : parts) {
                for (auto &u : scanDependencies(part.text, part.mode)) {
                    std::string key = u.global ? "::" : "";
                    for (const auto &comp : u.chain) key += comp + "::";
                    const auto it = index.find(key);
                    if (it == index.end()) {
                        index.emplace(key, uses.size());
                        uses.push_back(std::move(u));
                    } else {
                        uses[it->second].strong = uses[it->second].strong || u.strong;
                    }
                }
            }

            // classes used as a nested-name-specifier (A::x) must be complete
            std::unordered_set<std::string> forced;
            for (const auto &u : uses) {
                for (std::size_t m = 0; m + 1 < u.chain.size(); ++m) {
                    const std::vector<std::string> prefix(u.chain.begin(), u.chain.begin() + static_cast<std::ptrdiff_t>(m) + 1);
                    for (const auto *info : resolveEntity(ctx, prefix, u.global, scope))
                        forced.insert(info->header);
                }
            }

            std::vector<std::string> includes;
            std::vector<std::string> forwards;
            const auto add = [&](const std::string &h) {
                if (h == file) return;
                if (std::find(includes.begin(), includes.end(), h) == includes.end()) includes.push_back(h);
            };
            if (!extra.empty()) add(extra);

            for (const auto &u : uses) {
                for (const auto *info : resolveEntity(ctx, u.chain, u.global, scope)) {
                    if (info->header == file) continue;
                    const bool is_forced = forced.contains(info->header);
                    if (info->is_value && !u.strong) continue;
                    if (is_header && !info->keyword.empty() && !is_forced) {
                        const std::string ns = joinStrings(info->ns, "::");
                        const std::string decl = info->keyword + " " + info->name + ";";
                        const std::string fwd = ns.empty() ? decl : "namespace " + ns + " { " + decl + " }";
                        if (std::find(forwards.begin(), forwards.end(), fwd) == forwards.end()) forwards.push_back(fwd);
                    }
                    if (u.strong || is_forced || info->keyword.empty() || !is_header)
                        add(info->header);
                }
            }
            for (const auto &h : forced) add(h);   // qualifiers whose own name was not a separate use

            std::string out;
            if (is_header) out += "#pragma once\n\n";
            const bool lib = !parts.empty();                    // enums depend on nothing, not even the lib
            if (lib) {
                std::vector<std::string_view> texts;
                for (const auto &part : parts) texts.push_back(part.text);
                for (const auto &inc : libraryIncludes(texts)) out += "#include <" + inc + ">\n";
            }
            for (const auto &fwd : forwards) out += fwd + "\n";
            for (const auto &inc : includes) out += "#include <" + inc + ">\n";
            if (lib || !includes.empty() || !forwards.empty()) out += "\n";
            return out;
        }

        auto openNamespaces(Converter::Writer &w, const std::vector<std::string> &ns) -> void {
            for (const auto &n : ns) {
                emitLine(w, "namespace " + n + " {");
                w.increaseIndentation();
            }
        }
        auto closeNamespaces(Converter::Writer &w, const std::vector<std::string> &ns) -> void {
            for (auto it = ns.rbegin(); it != ns.rend(); ++it) {
                w.decreaseIndentation();
                emitLine(w, "} // namespace " + *it);
            }
        }

        // `operator<<` etc. can't be file names
        auto fileSafe(const std::string &name) -> std::string {
            std::string out;
            for (const char c : name) {
                switch (c) {
                    case '<': out += "lt"; break;
                    case '>': out += "gt"; break;
                    case '=': out += "eq"; break;
                    case '+': out += "plus"; break;
                    case '-': out += "minus"; break;
                    case '*': out += "mul"; break;
                    case '/': out += "div"; break;
                    case '%': out += "mod"; break;
                    case '!': out += "not"; break;
                    case '&': out += "and"; break;
                    case '|': out += "or"; break;
                    case '^': out += "xor"; break;
                    case '~': out += "tilde"; break;
                    case '(': out += "call"; break;
                    case '[': out += "index"; break;
                    case ',': out += "comma"; break;
                    case ' ': break;
                    default: out += c;
                }
            }
            return out;
        }

        auto fileBase(const std::vector<std::string> &scope, const std::string &name) -> std::string {
            const std::string dir = joinStrings(scope, "/");
            return "incremental/" + (dir.empty() ? std::string() : dir + "/") + name;
        }
    }

    auto Construct::setVisibility(LangAPI::Visibility visibility) -> std::string {
        context.visibility = visibility;
        return getVisibility(visibility);
    }
    auto Construct::getVisibility(LangAPI::Visibility visibility) -> std::string {
        switch (visibility) {
            case LangAPI::Visibility::Public:
                return "public:";
            case LangAPI::Visibility::Private:
                return "private:";
                break;
            case LangAPI::Visibility::Protected:
                return "protected:";
                break;
            default:
                throw std::runtime_error("Cpp Converter only supports Public, Protected and Private visibility yet");
        }
    }
    auto Construct::setDefaultClsVisibility(LangAPI::Visibility visibility) -> std::string {
        context.visibility = visibility;
        return getDefaultVisibility(visibility);
    }
    auto Construct::getDefaultVisibility(LangAPI::Visibility visibility) -> std::string {
        switch (visibility) {
            case LangAPI::Visibility::Public:
                return "struct";
            case LangAPI::Visibility::Private:
                return "class";
                break;
            default:
                throw std::runtime_error("Cpp Converter only supports Public and Private visibility yet");
        }
    }
    auto Construct::determineOffloadCompatible(const LangAPI::Function &func) -> bool {
        const bool is_templated_forward =
        !func.template_parameters.empty() &&
        std::all_of(
            func.template_parameters.begin(),
            func.template_parameters.end(),
            [](const auto &p) {
                return !p.second.empty();
            }
        );

        const bool is_plain_cpp_forward =
            func.template_parameters.empty() &&
            !func.statements.empty();

        const bool offload =
            is_templated_forward || is_plain_cpp_forward;
        return offload;
    }

    // Namespaces produce no text of their own: every structure / function file re-opens the
    // namespaces it lives in (context.namespace_path), and .cpp definitions use qualified names.
    auto Construct::produceNamespace(const LangAPI::Namespace &ns, OutputSrc &o) -> void {
        context.symbol_path.push_back(ns.name);
        context.namespace_path.push_back(ns.name);
        convertDeclarations(ns.declarations, o);
        context.namespace_path.pop_back();
        context.symbol_path.pop_back();
    }

    // Every function gets its own files, whether it is a free function or a class member:
    //   incremental/<scope path>/<name>.h    : free function: its declaration (inside its namespaces), plus
    //                                          `extern template` lines for constrained templates;
    //                                          any function: the out-of-class definition when the template can't be
    //                                          instantiated up front (the body has to be visible)
    //   incremental/<scope path>/<name>.cpp  : definition (+ explicit instantiations for every allowed type)
    // A class member is declared inside its class header. Member `extern template` lines are not written: the body
    // only exists in the .cpp, so nothing could be instantiated implicitly, and the lines would force every user
    // to include the complete class. Includes: see buildPrefix. Overloads get a _2, _3 ... suffix.
    // The standalone main header gets the declarations of free functions and (at its very end) the bodies of
    // the inline templates; it never includes anything from incremental/.
    auto Construct::produceFunction(const LangAPI::Function &func, OutputSrc &o, bool constructor) -> void {
        const bool debug_rule = !func.statements.empty() && func.statements.front().isComment() &&
            func.statements.front().getComment().value.starts_with("ISPA_DEBUG_SCOPE rule ");
        if (o.in_templated_class) {
            const auto signature = buildSignature(func, SigKind::Declaration, "", true, {}, constructor);
            emitLine(*o.h, signature + (func.statements.empty() ? ";" : " {"));
            if (!func.statements.empty()) {
                o.h->increaseIndentation();
                if (debug_rule) { emitLine(*o.h, "try {"); o.h->increaseIndentation(); }
                convertStatements(func.statements, *o.h);
                if (debug_rule) {
                    o.h->decreaseIndentation();
                    emitLine(*o.h, "} catch (const NoMatch&) { throw;");
                    emitLine(*o.h, "} catch (const ::ISPA_STD::ExternalParseError&) { throw;");
                    emitLine(*o.h, "} catch (...) { ::ispa::debug::breakpoint(\"uncaught parser failure\"); throw; }");
                }
                o.h->decreaseIndentation();
                emitLine(*o.h, "}");
            }
            ++context.next_function;
            return;
        }
        const bool is_template = !func.template_parameters.empty();
        const bool has_body = !func.statements.empty();
        const bool offload = determineOffloadCompatible(func);
        const bool inline_def = has_body && !offload;                       // some template parameter has no fixed type list
        const bool constrained_def = has_body && offload && is_template;    // every template parameter has a fixed type list

        // file name chosen by the pre-pass in construct() (same traversal order => same function)
        const std::string base = context.next_function < context.function_bases.size()
            ? context.function_bases[context.next_function]
            : fileBase(context.symbol_path, fileSafe(func.name));
        ++context.next_function;

        const std::string decl = buildSignature(func, SigKind::Declaration, "", o.in_class, {}, constructor);
        Converter::Writer fdecl;  // free function declaration
        Converter::Writer fext;   // extern template lines (free functions)
        Converter::Writer fdef;   // inline template definition
        Converter::Writer fcpp;   // <base>.cpp

        if (o.in_class) {
            emitLine(*o.h, decl + ";");                      // member: declared in the class
        } else {
            openNamespaces(fdecl, context.namespace_path);   // free function: declared in its own header
            emitLine(fdecl, decl + ";");
            closeNamespaces(fdecl, context.namespace_path);
        }

        if (has_body) {
            const std::string scope = joinStrings(context.symbol_path, "::");   // enclosing scope, before the name is pushed
            const std::string qualifier = scope.empty() ? std::string() : scope + "::";

            context.symbol_path.push_back(func.name);
            struct PathGuard {
                std::vector<std::string> &path;
                ~PathGuard() { path.pop_back(); }
            } guard{context.symbol_path};

            auto writeDefinition = [&](Converter::Writer &w) {
                emitLine(w, buildSignature(func, SigKind::Definition, qualifier, false, {}, constructor) + " {");
                w.increaseIndentation();
                if (debug_rule) { emitLine(w, "try {"); w.increaseIndentation(); }
                convertStatements(func.statements, w);
                if (debug_rule) {
                    w.decreaseIndentation();
                    emitLine(w, "} catch (const NoMatch&) { throw;");
                    emitLine(w, "} catch (const ::ISPA_STD::ExternalParseError&) { throw;");
                    emitLine(w, "} catch (...) { ::ispa::debug::breakpoint(\"uncaught parser failure\"); throw; }");
                }
                w.decreaseIndentation();
                emitLine(w, "}");
            };

            if (inline_def) {
                writeDefinition(fdef);
            } else {
                writeDefinition(fcpp);
                if (constrained_def) {
                    for (const auto &targs : instantiationSets(func)) {
                        const std::string inst = buildSignature(func, SigKind::Instantiation, qualifier, false, targs, constructor);
                        emitLine(fcpp, "template " + inst + ";");
                        if (!o.in_class) emitLine(fext, "extern template " + inst + ";");
                    }
                }
            }
        }

        const std::string decl_text = fdecl.get();
        const std::string ext_text = fext.get();
        const std::string def_text = fdef.get();
        const std::string cpp_text = fcpp.get();

        // standalone main header
        if (!decl_text.empty()) context.main_h.dwrite("{}", decl_text);
        if (!def_text.empty()) context.main_tail.dwrite("{}", def_text);

        if (!incremental) {
            // single .h/.cpp mode: declarations (+ extern template lines) stay in the main header, inline template
            // bodies go to its tail (above), every out-of-line definition / explicit instantiation goes to the main .cpp
            if (!ext_text.empty()) context.main_h.dwrite("{}", ext_text);
            if (!cpp_text.empty()) context.main_cpp.dwrite("{}", cpp_text);
            return;
        }

        const std::string h_text = decl_text + ext_text + def_text;
        if (!h_text.empty()) {
            const std::string file = base + ".h";
            const std::vector<DepPart> parts{{decl_text, DepMode::Declaration}, {ext_text, DepMode::Weak}, {def_text, DepMode::Strong}};
            context.function_files.emplace_back(file, buildPrefix(context, file, parts, true, "", context.symbol_path) + h_text);
        }
        if (!cpp_text.empty()) {
            const std::string file = base + ".cpp";
            const std::vector<DepPart> parts{{cpp_text, DepMode::Strong}};
            context.function_files.emplace_back(file,
                buildPrefix(context, file, parts, false, h_text.empty() ? std::string() : base + ".h", context.symbol_path) + cpp_text);
        }
    }

    // The class body in the main header holds only declarations (members, variables, nested types).
    auto Construct::produceClass(const LangAPI::Class &cls, OutputSrc &o) -> void {
        const auto saved_visibility = context.visibility;
        const std::string name = Core::convertSymbol(cls.name);
        const auto split = splitClassName(cls, name);
        // A class lexically nested in another class must be declared using its
        // unqualified leaf name. `struct Parent::Child {}` is not valid inside Parent.
        const std::string declaration_name = o.in_class ? split.leaf : name;
        std::vector<std::string> fields;
        std::vector<std::string> methods;
        // struct Name : public A, private B {
        std::string head = setDefaultClsVisibility(cls.default_visibility) + " " + declaration_name;
        if (!cls.inherit_members.empty()) {
            std::vector<std::string> bases;
            for (const auto &inherit : cls.inherit_members) {
                std::string base = accessName(inherit.first) + " ";
                if (std::holds_alternative<LangAPI::Symbol>(inherit.second))
                    base += Core::convertSymbol(std::get<LangAPI::Symbol>(inherit.second));
                else
                    base += Core::convertIspaLibSymbol(std::get<LangAPI::IspaLibSymbol>(inherit.second));
                bases.push_back(std::move(base));
            }
            head += " : " + joinStrings(bases, ", ");
        }
        if (!cls.template_parameters.empty()) {
            std::vector<std::string> parameters;
            for (const auto& parameter : cls.template_parameters)
                parameters.push_back("typename " + parameter);
            emitLine(*o.h, "template<" + joinStrings(parameters, ", ") + ">");
        }
        emitLine(*o.h, head + " {");
        o.h->increaseIndentation();

        // Preserve the lexical path for nested classes. Qualified symbol prefixes
        // only contribute to the path for top-level LangAPI declarations.
        const auto parent_path = context.symbol_path;
        if (!o.in_class)
            context.symbol_path.insert(context.symbol_path.end(), split.prefix.begin(), split.prefix.end());
        context.symbol_path.push_back(split.leaf);
        OutputSrc member(o.h, o.cpp);
        member.in_class = true;
        member.in_templated_class = o.in_templated_class || !cls.template_parameters.empty();

        if (cls.constructor) {
            emitLine(*o.h, "public:");
            auto constructor = *cls.constructor;
            constructor.name = split.leaf;
            produceFunction(constructor, member, true);
            if (cls.default_visibility != LangAPI::Visibility::Public) emitLine(*o.h, getVisibility(cls.default_visibility));
        }
        auto current = cls.default_visibility;
        for (const auto &[declaration, visibility] : cls.data) {
            if (visibility != current) {                    // label only when the access actually changes
                o.h->decreaseIndentation();
                emitLine(*o.h, getVisibility(visibility));
                o.h->increaseIndentation();
                current = visibility;
            }
            convertDeclaration(*declaration, member);
            if (visibility == LangAPI::Visibility::Public) {
                if (declaration->isVariable()) {
                    fields.push_back(declaration->getVariable().name);
                } else if (declaration->isFunction()) {
                    methods.push_back(declaration->getFunction().name);
                }
            }
        }
        if (cls.to_str_fun.has_value()) {
            if (context.visibility != LangAPI::Visibility::Public) {
                o.h->decreaseIndentation();
                emitLine(*o.h, "public:");
                o.h->increaseIndentation();
            }
            produceFunction(cls.to_str_fun.value(), member);
        }
        generateMembers(*o.h, fields, "members");
        generateMembersStr(*o.h, fields, "members");
        generateMembers(*o.h, methods, "methods", Core::convertSymbol(cls.name));
        generateMembersStr(*o.h, methods, "methods");
        o.h->decreaseIndentation();
        emitLine(*o.h, "};");
        if (debug_instrumentation_ && !o.in_class && cls.template_parameters.empty() &&
            (!fields.empty() ||
             std::find(context.symbol_path.begin(), context.symbol_path.end(), "FlatTypes") != context.symbol_path.end() ||
             std::find(context.symbol_path.begin(), context.symbol_path.end(), "Types") != context.symbol_path.end())) {
            std::ostringstream debug_type;
            debug_type << std::quoted(joinStrings(context.symbol_path, "."));
            emitLine(*o.h, "inline std::string_view ispa_debug_type(const " + split.leaf + "&) { return " + debug_type.str() + "; }");
            emitLine(*o.h, "template<class F> void ispa_debug_fields(const " + split.leaf + "& value, F&& field) {");
            o.h->increaseIndentation();
            for (const auto& field : fields) {
                std::ostringstream label;
                label << std::quoted(field);
                emitLine(*o.h, "field(" + label.str() + ", value." + field + ");");
            }
            o.h->decreaseIndentation();
            emitLine(*o.h, "}");
        }
        // A nested class is defined in its enclosing class's def.h. Emit a
        // forwarding header for the nested type so incremental consumers have
        // a stable include path without defining the C++ class a second time.
        if (incremental && o.in_class) {
            const std::string nested_header = fileBase(parent_path, split.leaf) + "/def.h";
            const std::string parent_header = fileBase(
                std::vector<std::string>(parent_path.begin(), parent_path.end() - 1),
                parent_path.back()) + "/def.h";
            context.function_files.emplace_back(nested_header,
                "#pragma once\n#include <" + parent_header + ">\n");
        }
        context.symbol_path = parent_path;
        if (!member.src_empty) o.src_empty = false;
        context.visibility = saved_visibility;
        if (cls.output_fun.has_value() && cls.template_parameters.empty()) {
            auto output_operator = cls.output_fun.value();
            output_operator.name = "operator<<";
            output_operator.parameters.insert(output_operator.parameters.begin(), std::make_pair(LangAPI::Type {LangAPI::Symbol {"std::ostream &"}}, "os"));
            output_operator.type = LangAPI::Type {LangAPI::Symbol {"std::ostream &"}};
            OutputSrc free_function(o.h, o.cpp);
            free_function.in_class = false;

            produceFunction(output_operator, free_function);
        }
    }

    auto Construct::produceTypeAlias(const LangAPI::TypeAlias &alias, OutputSrc &o) -> void {
        if (!alias.template_parameters.empty()) {
            std::vector<std::string> parameters;
            for (const auto& parameter : alias.template_parameters)
                parameters.push_back("typename " + parameter);
            emitLine(*o.h, "template<" + joinStrings(parameters, ", ") + ">");
        }
        emitLine(*o.h, "using " + alias.name + " = " + Core::convertType(alias.type) + ";");   // used to miss the newline
    }

    auto Construct::produceEnum(const LangAPI::Enum &enum_, OutputSrc &o) -> void {
        emitLine(*o.h, "enum class " + enum_.name + " {");
        o.h->increaseIndentation();
        std::size_t i = 0;
        for (const auto &n : enum_.value) {
            emitLine(*o.h, n + ",");
            if (++i % 6 == 0 && i < enum_.value.size())        // blank line between groups of six
                emitLine(*o.h, "");
        }
        o.h->decreaseIndentation();
        emitLine(*o.h, "};");
    }

    auto Construct::produceVariable(const LangAPI::Variable &v, OutputSrc &o) -> void {
        const bool in_class = o.in_class;
        const std::string type = Core::convertType(v.type);
        if (v.is_static) {
            // class: `static T x;` + `T Cls::x = v;`   namespace: `extern T x;` + `T ns::x = v;`
            emitLine(*o.h, std::string(in_class ? "static " : "extern ") + type + " " + v.name + ";");
            const std::string scope = joinStrings(context.symbol_path, "::");
            std::string def = type + " " + (scope.empty() ? std::string() : scope + "::") + v.name;
            if (!v.value.empty())
                def += " = " + Core::convertExpression(v.value);
            emitLine(*o.cpp, def + ";");
            o.src_empty = false;
        } else if (!v.value.empty()) {
            emitLine(*o.h, std::string(in_class ? "" : "inline ") + type + " " + v.name + " = " + Core::convertExpression(v.value) + ";");
        } else {
            emitLine(*o.h, type + " " + v.name + ";");
        }
    }

    auto Construct::convertDeclaration(const LangAPI::Declaration &decl, OutputSrc &o)-> void {
        if (decl.isNamespace()) {
            produceNamespace(decl.getNamespace(), o);
            return;
        }
        if (decl.isFunction()) {
            produceFunction(decl.getFunction(), o);
            return;
        }
        if (decl.isForwardDeclaredClass()) {
            const std::string n = Core::convertSymbol(decl.getForwardDeclaredClass().name);
            std::string def_visibility = decl.getForwardDeclaredClass().isStruct ? "struct " : "class ";
            std::string template_prefix;
            if (!decl.getForwardDeclaredClass().template_parameters.empty()) {
                std::vector<std::string> parameters;
                for (const auto& parameter : decl.getForwardDeclaredClass().template_parameters)
                    parameters.push_back("typename " + parameter);
                template_prefix = "template<" + joinStrings(parameters, ", ") + "> ";
            }
            if (o.in_class) {
                emitLine(*o.h, template_prefix + def_visibility + n + ";");
            } else {
                const std::string ns = joinStrings(context.namespace_path, "::");
                emitLine(context.main_h, ns.empty() ? template_prefix + def_visibility + n + ";" : "namespace " + ns + " { " + template_prefix + def_visibility + n + "; }");
            }
            return;
        }

        if (o.in_class) {                                    // nested member: part of the enclosing class header
            if (decl.isClass()) produceClass(decl.getClass(), o);
            else if (decl.isEnum()) produceEnum(decl.getEnum(), o);
            else if (decl.isTypeAlias()) produceTypeAlias(decl.getTypeAlias(), o);
            else if (decl.isVariable()) produceVariable(decl.getVariable(), o);
            return;
        }

        // namespace-level structure / enum / alias / variable: its own .h (and .cpp for static definitions)
        std::string name;
        std::vector<std::string> nested_prefix;   // Symbol components before the leaf (nested structures)
        if (decl.isClass()) {
            const auto split = splitClassName(decl.getClass(), Core::convertSymbol(decl.getClass().name));
            name = split.leaf;
            nested_prefix = split.prefix;
        }
        else if (decl.isEnum()) name = decl.getEnum().name;
        else if (decl.isTypeAlias()) name = decl.getTypeAlias().name;
        else if (decl.isVariable()) name = decl.getVariable().name;
        else return;

        Converter::Writer body;
        Converter::Writer cpp;
        openNamespaces(body, context.namespace_path);
        OutputSrc so(&body, &cpp);
        if (decl.isClass()) produceClass(decl.getClass(), so);
        else if (decl.isEnum()) produceEnum(decl.getEnum(), so);
        else if (decl.isTypeAlias()) produceTypeAlias(decl.getTypeAlias(), so);
        else produceVariable(decl.getVariable(), so);
        closeNamespaces(body, context.namespace_path);

        auto file_scope = context.symbol_path;
        file_scope.insert(file_scope.end(), nested_prefix.begin(), nested_prefix.end());
        const std::string base = fileBase(file_scope, name) + "/def";   // incremental/<scope>/<nested...>/<Name>/def.h
        const std::string h_text = body.get();
        const std::string cpp_text = cpp.get();

        context.main_h.dwrite("{}", h_text);                 // the standalone main header holds every definition itself
        if (!incremental) {
            // single .h/.cpp mode: static member / namespace variable definitions go to the main .cpp, no per-symbol files
            if (!cpp_text.empty()) context.main_cpp.dwrite("{}", cpp_text);
            return;
        }

        // enumerators are not type uses (they often share names with structs/functions): enums depend on nothing
        std::vector<DepPart> parts;
        if (decl.isClass() || decl.isVariable()) parts.push_back({h_text, DepMode::Declaration});
        // an alias to a template-id (using T = ISPA_STD::X<A, B>) makes every user of T instantiate X<A, B>, which needs
        // A and B complete, so aliases depend strongly on everything they name
        else if (decl.isTypeAlias()) parts.push_back({h_text, DepMode::Strong});
        auto scope = file_scope;                               // scope the struct body lives in
        if (decl.isClass()) scope.push_back(name);
        std::string adl_tail;
        if (const auto it = context.adl.find(base + ".h"); it != context.adl.end()) {
            adl_tail = "\n";
            for (const auto &fn : it->second) adl_tail += "#include <" + fn + ">\n";
        }
        context.function_files.emplace_back(base + ".h", buildPrefix(context, base + ".h", parts, true, "", scope) + h_text + adl_tail);
        if (!cpp_text.empty()) {
            const std::vector<DepPart> cpp_parts{{cpp_text, DepMode::Strong}};
            context.function_files.emplace_back(base + ".cpp",
                buildPrefix(context, base + ".cpp", cpp_parts, false, base + ".h", scope) + cpp_text);
        }
    }
    auto Construct::convertDeclarations(const LangAPI::Declarations &decls, OutputSrc &o) -> void {
        for (const auto &decl : decls) {
            convertDeclaration(decl, o);
        }
    }

    // statements
    auto Construct::produceIf(const LangAPI::If &if_stmt, Converter::Writer &o) -> void {
        emitLine(o, "if (" + Core::convertExpression(if_stmt.expr) + ") {");
        o.increaseIndentation();
        convertStatements(if_stmt.stmt, o);
        o.decreaseIndentation();
        if (!if_stmt.else_stmt.empty()) {
            emitLine(o, "} else {");
            o.increaseIndentation();
            convertStatements(if_stmt.else_stmt, o);
            o.decreaseIndentation();
        }
        emitLine(o, "}");
    }

    auto Construct::produceWhile(const LangAPI::While &while_stmt, Converter::Writer &o) -> void {
        emitLine(o, "while (" + Core::convertExpression(while_stmt.expr) + ") {");
        o.increaseIndentation();
        convertStatements(while_stmt.stmt, o);
        o.decreaseIndentation();
        emitLine(o, "}");
    }

    auto Construct::produceDoWhile(const LangAPI::DoWhile &do_while_stmt, Converter::Writer &o) -> void {
        emitLine(o, "do {");
        o.increaseIndentation();
        convertStatements(do_while_stmt.stmt, o);
        o.decreaseIndentation();
        emitLine(o, "} while (" + Core::convertExpression(do_while_stmt.expr) + ");");
    }

    auto Construct::produceSwitch(const LangAPI::Switch &switch_stmt, Converter::Writer &o) -> void {
        emitLine(o, "switch (" + Core::convertExpression(switch_stmt.expression) + ") {");
        o.increaseIndentation();
        for (const auto &case_stmt : switch_stmt.cases) {
            emitLine(o, "case " + Core::convertRValue(case_stmt.first) + ": {");
            o.increaseIndentation();
            convertStatements(case_stmt.second, o);
            emitLine(o, "break;");
            o.decreaseIndentation();
            emitLine(o, "}");
        }
        o.decreaseIndentation();
        emitLine(o, "}");
    }
    auto Construct::produceVariableStatement(const LangAPI::Variable &variable, Converter::Writer &o) -> void {
        std::string line = Core::convertType(variable.type) + " " + variable.name;
        const std::string initializer = Core::convertExpression(variable.value);
        if (!variable.value.empty())
            line += " = " + initializer;
        emitLine(o, line + ";");
        if (debug_instrumentation_ && !context.symbol_path.empty() && context.symbol_path.back() == "semantic_action_exec" &&
            initializer.find("captures.") != std::string::npos)
            emitLine(o, "if (::ispa::debug::enabled()) ::ispa::debug::lexer_capture(\"" +
                variable.name + "\", " + variable.name + ");");
    }
    auto Construct::produceComment(const LangAPI::Comment &comment, Converter::Writer &o) -> void {
        const std::string &text = comment.value;
        if (text.starts_with("ISPA_DEBUG_LINE ")) {
            std::istringstream input(text.substr(16));
            std::string file, local;
            int row = 0, column = 0;
            input >> std::quoted(file) >> row >> column;
            if (!input || row <= 0) throw std::runtime_error("Malformed ISPA debug line marker");
            std::ostringstream location;
            location << std::quoted(file) << ", " << row << ", " << column;
            std::string snapshot = "::ispa::debug::Snapshot{}";
            std::string phase = "before";
            while (input >> local) {
                if (local == "@after") phase = "after";
                else if (local == "ctx") {
                    snapshot += ".add(\"ctx.rule\", ctx.rule).add(\"ctx.pos\", ctx.pos).add(\"ctx.ast\", ctx.ast)";
                } else {
                    const auto equal = local.find('=');
                    const auto label = equal == std::string::npos ? local : local.substr(0, equal);
                    const auto symbol = equal == std::string::npos ? local : local.substr(equal + 1);
                    snapshot += ".add(\"" + label + "\", " + symbol + ")";
                }
            }
            emitLine(o, "if (::ispa::debug::enabled()) ::ispa::debug::line({" + location.str() + "}, " + snapshot + ", \"" + phase + "\");");
            return;
        }
        if (text.starts_with("ISPA_DEBUG_SCOPE ")) {
            std::istringstream input(text.substr(17));
            std::string kind, name, file;
            int row = 0, column = 0;
            input >> kind >> std::quoted(name) >> std::quoted(file) >> row >> column;
            if (!input || (kind != "rule" && kind != "production"))
                throw std::runtime_error("Malformed ISPA debug scope marker");
            std::ostringstream value;
            value << "::ispa::debug::" << (kind == "rule" ? "RuleScope" : "ProductionScope")
                  << " ispa_debug_" << kind << "{" << std::quoted(name) << ", {"
                  << std::quoted(file) << ", " << row << ", " << column << "}};";
            emitLine(o, value.str());
            return;
        }
        std::size_t start = 0;
        while (true) {
            const auto nl = text.find('\n', start);
            emitLine(o, "// " + text.substr(start, nl == std::string::npos ? std::string::npos : nl - start));
            if (nl == std::string::npos) break;
            start = nl + 1;
        }
    }

    auto Construct::produceThrow(const LangAPI::Throw &throw_stmt, Converter::Writer &o) -> void {
        emitLine(o, "throw " + Core::convertExpression(throw_stmt.throw_value) + ";");
    }

    auto Construct::convertStatements(const LangAPI::Statements &statements, Converter::Writer &o)-> void  {
        for (const auto &stmt : statements) {
            convertStatement(stmt, o);
        }
    }

    auto Construct::convertStatement(const LangAPI::Statement &stmt, Converter::Writer &o) -> void {
        if (stmt.isExpression()) {
            // plain expression statements (calls, assignments, return/break/continue, ...) were silently dropped before
            const std::string text = Core::convertExpression(stmt.getExpression());
            if (!text.empty()) {
                emitLine(o, text + ";");
                if (debug_instrumentation_ && text.find("assign_compatible(result.") != std::string::npos)
                    emitLine(o, "if (::ispa::debug::enabled()) ::ispa::debug::value(\"result\", result, ::ispa::debug::active_location());");
            }
        } else if (stmt.isIf()) {
            produceIf(stmt.getIf(), o);
        } else if (stmt.isWhile()) {
            produceWhile(stmt.getWhile(), o);
        } else if (stmt.isDoWhile()) {
            produceDoWhile(stmt.getDoWhile(), o);
        } else if (stmt.isSwitch()) {
            produceSwitch(stmt.getSwitch(), o);
        } else if (stmt.isThrow()) {
            produceThrow(stmt.getThrow(), o);
        } else if (stmt.isComment()) {
            produceComment(stmt.getComment(), o);
        } else if (stmt.isVariable()) {
            produceVariableStatement(stmt.getVariable(), o);
        } else {
            // never drop a statement silently again
            throw std::runtime_error("Cpp Converter: unsupported statement kind");
        }
    }
    // converter.cpp
    auto Construct::construct() -> void {
        // A process can generate several dependency parsers. Resolve runtime
        // symbols in this parser's namespace on every invocation.
        Core::name = std::string(namespace_name);

        // ---- pre-pass: where does every named thing live? (needed to resolve dependencies) ----
        std::size_t order = 0;
        std::unordered_map<std::string, std::size_t> counts;
        std::vector<std::string> ns;       // namespaces only
        std::vector<std::string> scope;    // namespaces + classes
        const auto reg = [&](const std::string &name, SymbolInfo info) {
            info.name = name;
            // full lookup path: namespaces AND enclosing classes. Members of a rule struct (rule::value::STRING)
            // used to be registered with the namespaces only, so Types::rule::value::STRING never matched.
            info.ns = scope;
            info.order = order++;
            context.header_order.emplace(info.header, info.order);
            context.known[name].push_back(std::move(info));
        };
        struct Pending {
            std::vector<std::string> ns;
            std::string header;
            std::vector<std::string> names;
        };
        std::vector<Pending> pending;
        std::function<void(const LangAPI::Declaration &, const std::string &)> collect;
        collect = [&](const LangAPI::Declaration &decl, const std::string &outer) {
            if (decl.isNamespace()) {
                const auto &n = decl.getNamespace();
                ns.push_back(n.name);
                scope.push_back(n.name);
                context.known_namespaces.insert(joinStrings(ns, "::"));
                for (const auto &d : n.declarations) collect(d, "");
                scope.pop_back();
                ns.pop_back();
            } else if (decl.isClass()) {
                const auto &cls = decl.getClass();
                const auto split = splitClassName(cls, Core::convertSymbol(cls.name));
                const std::string &name = split.leaf;                  // leaf only: lookups end in the last component
                auto file_scope = scope;                               // nested symbol -> its own directory
                if (outer.empty())
                    file_scope.insert(file_scope.end(), split.prefix.begin(), split.prefix.end());

                const std::string header =
                    outer.empty()
                        ? fileBase(file_scope, name) + "/def.h"
                        : outer;

                SymbolInfo info;
                info.header = header;
                info.adl_type = outer.empty();

                // a nested structure can't be forward declared (`namespace A::B { struct C; }` is wrong when B is
                // a class), so it keeps an empty keyword and is always #included
                if (outer.empty() && split.prefix.empty() && cls.template_parameters.empty()) {
                    info.keyword =
                        getDefaultVisibility(cls.default_visibility);
                }

                scope = std::move(file_scope);                         // path = scope + prefix + leaf
                reg(name, std::move(info));
                scope.push_back(name);

                if (cls.constructor) {
                    auto constructor = *cls.constructor;
                    constructor.name = name;
                    collect(LangAPI::Function::createDeclaration(constructor), header);
                }
                // 1. Ordinary member declarations
                for (const auto &[declaration, visibility] : cls.data) {
                    collect(*declaration, header);
                }

                // 2. Generated to_string() function
                // Must be registered in the same order as produceClass().
                if (cls.to_str_fun.has_value()) {
                    collect(
                        LangAPI::Function::createDeclaration(
                            cls.to_str_fun.value()
                        ),
                        header
                    );
                }

                scope.resize(scope.size() - 1);
                if (outer.empty() && !split.prefix.empty())
                    scope.resize(scope.size() - split.prefix.size());

                // 3. Namespace-level output operator
                // produceClass() emits it AFTER leaving the class scope.
                if (cls.output_fun.has_value() && cls.template_parameters.empty()) {
                    auto output_operator = cls.output_fun.value();

                    output_operator.name = "operator<<";

                    output_operator.parameters.insert(
                        output_operator.parameters.begin(),
                        std::make_pair(
                            LangAPI::Type{
                                LangAPI::Symbol{"std::ostream &"}
                            },
                            "os"
                        )
                    );

                    output_operator.type = LangAPI::Type{
                        LangAPI::Symbol{"std::ostream &"}
                    };

                    collect(
                        LangAPI::Function::createDeclaration(
                            output_operator
                        ),
                        ""
                    );
                }
            } else if (decl.isFunction()) {
                const auto &func = decl.getFunction();
                std::string base = fileBase(scope, fileSafe(func.name));
                if (const auto n = ++counts[base]; n > 1) base += "_" + std::to_string(n);
                context.function_bases.push_back(base);
                if (outer.empty()) {
                    SymbolInfo info;
                    info.header = base + ".h";
                    info.is_value = true;
                    reg(func.name, std::move(info));
                    // remember which types the parameters name (see the ADL pass below)
                    Pending pend{ns, base + ".h", {}};
                    for (const auto &prm : func.parameters)
                        for (const auto &use : scanDependencies(Core::convertType(prm.first), DepMode::Strong))
                            pend.names.push_back(use.chain.back());
                    // constrained templates (template<typename T, class = enable_if<is_same<T, A> || ...>>) name their
                    // types in the allowed-type lists, e.g. a generic operator<< for every token struct
                    for (const auto &tp : func.template_parameters)
                        for (const auto &allowed : tp.second)
                            for (const auto &use : scanDependencies(Core::convertType(allowed), DepMode::Strong))
                                pend.names.push_back(use.chain.back());
                    pending.push_back(std::move(pend));
                }
            } else if (decl.isEnum() || decl.isTypeAlias() || decl.isVariable()) {
                const std::string name = decl.isEnum() ? decl.getEnum().name
                                       : decl.isTypeAlias() ? decl.getTypeAlias().name
                                       : decl.getVariable().name;
                if (decl.isVariable() && !outer.empty()) return;   // class members live in the class header
                SymbolInfo info;
                info.header = outer.empty() ? fileBase(scope, name) + "/def.h" : outer;
                info.is_value = decl.isVariable();
                info.adl_type = decl.isEnum() && outer.empty();
                reg(name, std::move(info));
            }
        };
        for (const auto &decl : *source) collect(decl, "");

        // ADL: a free function in the namespace of a type, taking that type as a parameter (operator<<, swap, ...),
        // is part of the type's interface and found only through ADL, so nothing ever names it. The type's header
        // therefore includes the function's (declaration-only) header at its end.
        for (const auto &pend : pending) {
            for (const auto &name : pend.names) {
                const auto it = context.known.find(name);
                if (it == context.known.end()) continue;
                for (const auto &info : it->second) {
                    if (!info.adl_type || info.ns != pend.ns) continue;
                    auto &list = context.adl[info.header];
                    if (std::find(list.begin(), list.end(), pend.header) == list.end()) list.push_back(pend.header);
                }
            }
        }

        // ---- generation ----
        // the prefix (#pragma once + library chunks) can only be written once the whole header text is known
        OutputSrc o{&context.main_h, &context.main_cpp};
        convertDeclarations(*source, o);

        // bodies of the inline templates go after every declaration; the main header includes nothing from incremental/
        context.main_h.dwrite("{}", std::string(context.main_tail.get()));

        writable_files.reserve(2 + context.function_files.size());
        {
            const std::string h_body = std::string(context.main_h.get());
            writable_files.emplace_back(std::string(namespace_name) + ".h",
                "#pragma once\n\n" + libraryIncludeBlock({h_body}) + h_body);
            if (auto cpp = std::string(context.main_cpp.get()); !cpp.empty()) {
                // single .h/.cpp mode: the .cpp holds all definitions, so it must see the main header
                const std::string own_header = incremental
                    ? std::string()
                    : "#include \"" + std::string(namespace_name) + ".h\"\n\n";
                writable_files.emplace_back(std::string(namespace_name) + ".cpp", own_header + libraryIncludeBlock({cpp}) + cpp);
            }
        }
        if (incremental)   // nothing is written to incremental/ in single-file mode
            for (auto &file : context.function_files)
                writable_files.emplace_back(std::move(file));
    }
}

extern "C" auto cpp_construct(const LangAPI::Declarations *tree, const char *namespace_name,
                              bool incremental, bool debug_instrumentation) -> Cpp::Construct* {
    return new Cpp::Construct(tree, namespace_name, incremental, debug_instrumentation);
}
extern "C" auto cpp_construct_finish(Cpp::Construct* c) -> void {
    delete c;
}
