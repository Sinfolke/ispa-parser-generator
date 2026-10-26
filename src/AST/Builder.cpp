module;
#include <ispa/parser.h>
module AST.Builder;
import AST.types;
import AST.Tree;
import cpuf.printf;
import cpuf.op;
import corelib;
import logging;
import dstd;
import std;
import UAST.Python;
import Python.Imports;

namespace {
    template<typename T>
    const T& payload(const T& value) {
        return value;
    }

    template<typename T>
    const T& payload(const std::shared_ptr<T>& value) {
        if (!value) {
            throw std::runtime_error("Null parser AST node");
        }
        return *value;
    }

    template<typename T>
    decltype(auto) payload(const UAST::Node<T>& node) {
        return payload(node.data());
    }
    // LR literal captures are characters; the LL bootstrap keeps token nodes.
    char captureMarker(char value) { return value; }
    template<class... T>
    char captureMarker(const std::variant<T...>& value) {
        return std::visit([](const auto& token) -> char {
            using Token = std::decay_t<decltype(token)>;
            if constexpr (std::is_same_v<Token, UAST::Node<UAST::COMMERCIAL_AT>>) return '@';
            else if constexpr (std::is_same_v<Token, UAST::Node<UAST::PERCENT_SIGN>>) return '%';
            else if constexpr (std::is_same_v<Token, char>) return token;
            else throw std::runtime_error("Missing capture marker");
        }, value);
    }
    std::string id(const UAST::Node<UAST::ID>& node) {
        return payload(node).value;
    }
    stdu::vector<std::string> qualifiedName(const UAST::rule_name& n) {
        stdu::vector<std::string> result;
        result.push_back(id(n.primary));
        for (const auto& segment : n.nested) result.push_back(id(segment));
        return result;
    }
    stdu::vector<std::string> qualifiedName(const UAST::call_name& n) {
        stdu::vector<std::string> result;
        result.push_back(id(n.primary));
        for (const auto& segment : n.nested) result.push_back(id(segment));
        return result;
    }
    // The generated nodes know offset/length/line/column but not the file name;
    // Builder::build() publishes it here for the duration of the conversion.
    thread_local const std::string* currentFile = nullptr;
    thread_local const Python::Imports* currentPythonImports = nullptr;
    struct PythonScope {
        const Python::Imports* previous;
        explicit PythonScope(const Python::Imports& imports) : previous(currentPythonImports) { currentPythonImports = &imports; }
        ~PythonScope() { currentPythonImports = previous; }
    };
    void bindPython(AST::PythonAction& action) {
        if (currentPythonImports) std::visit([&](auto& syntax) {
            if constexpr (!std::is_same_v<std::decay_t<decltype(syntax)>, std::monostate>)
                currentPythonImports->bind(syntax, action.position.file);
        }, action.code);
    }
    struct FileScope {
        const std::string* previous;
        explicit FileScope(const std::string& file) : previous(currentFile) { currentFile = &file; }
        ~FileScope() { currentFile = previous; }
    };
    template<class T> UAST::SourcePosition where(const UAST::Node<T>& node) {
        static const std::string none;
        return UAST::positionOf(node, currentFile ? *currentFile : none);
    }
    AST::SourcePosition position(const UAST::SourcePosition& p) {
        return {p.file, p.offset, p.length, p.line, p.column};
    }
    template<class T> AST::SourcePosition position(const UAST::Node<T>& node) {
        return position(where(node));
    }
    AST::PythonNode python(const UAST::PythonNode& n) {
        AST::PythonNode out{n.kind, n.field, n.scalar, position(n.position), n.absent, {}};
        for (const auto& child : n.children) out.children.push_back(python(child));
        return out;
    }

    // Typed Python AST (UAST::py) -> AST::PythonNode. Each typed node becomes one
    // PythonNode: kind = node type ("BinaryExpr", "If", ...), children carry the
    // struct's field names, scalar holds the identifier / operator / literal text.
    namespace pylower {
        namespace py = UAST::py;
        using PN = AST::PythonNode;
        using Pos = UAST::SourcePosition;
        constexpr std::string_view kCtx[] = {"Load", "Store", "Del"};
        constexpr std::string_view kBool[] = {"And", "Or"};
        constexpr std::string_view kBin[] = {"Add", "Sub", "Mult", "MatMult", "Div", "Mod", "Pow", "LShift", "RShift", "BitOr", "BitXor", "BitAnd", "FloorDiv"};
        constexpr std::string_view kUnary[] = {"Invert", "Not", "UAdd", "USub"};
        constexpr std::string_view kCmp[] = {"Eq", "NotEq", "Lt", "LtE", "Gt", "GtE", "Is", "IsNot", "In", "NotIn"};
        constexpr std::string_view kConst[] = {"None", "Bool", "Int", "Float", "Str"};
        template<class E> std::string name(const std::string_view* table, E e) { return std::string(table[static_cast<std::size_t>(e)]); }

        inline PN make(std::string kind, std::string field, const Pos& p, std::string scalar = {}, std::vector<PN> children = {}) {
            PN out{std::move(kind), std::move(field), std::move(scalar), position(p), false, {}};
            for (auto& c : children) out.children.push_back(std::move(c));
            return out;
        }
        inline PN absentNode(std::string field, const Pos& p) {
            PN out{"absent", std::move(field), {}, position(p), true, {}};
            return out;
        }

        struct Lower {
            PN expr(const py::ExprPtr& e, std::string f, const Pos& fb) const {
                if (!e) return absentNode(std::move(f), fb);
                return std::visit([&](const auto& k) { return node(k, std::move(f), e->pos); }, e->kind);
            }
            PN stmt(const py::StmtPtr& s, std::string f, const Pos& fb) const {
                if (!s) return absentNode(std::move(f), fb);
                return std::visit([&](const auto& k) { return node(k, std::move(f), s->pos); }, s->kind);
            }
            PN exprs(const std::vector<py::ExprPtr>& v, std::string f, const Pos& p) const {
                std::vector<PN> c; for (const auto& e : v) c.push_back(expr(e, {}, p));
                return make("sequence", std::move(f), p, {}, std::move(c));
            }
            PN stmts(const std::vector<py::StmtPtr>& v, std::string f, const Pos& p) const {
                std::vector<PN> c; for (const auto& s : v) c.push_back(stmt(s, {}, p));
                return make("sequence", std::move(f), p, {}, std::move(c));
            }
            PN keywords(const std::vector<py::Keyword>& v, std::string f, const Pos& p) const {
                std::vector<PN> c;
                for (const auto& k : v) c.push_back(make("keyword", {}, k.pos, k.arg, {expr(k.value, "value", k.pos)}));
                return make("sequence", std::move(f), p, {}, std::move(c));
            }
            PN aliases(const std::vector<py::Alias>& v, std::string f, const Pos& p) const {
                std::vector<PN> c;
                for (const auto& a : v) c.push_back(make("alias", {}, a.pos, a.name, {make("asname", "asname", a.pos, a.asname)}));
                return make("sequence", std::move(f), p, {}, std::move(c));
            }
            PN strings(const std::vector<std::string>& v, std::string f, const Pos& p) const {
                std::vector<PN> c; for (const auto& s : v) c.push_back(make("string", {}, p, s));
                return make("sequence", std::move(f), p, {}, std::move(c));
            }
            PN module(const py::Module& m) const { return make("Module", {}, m.pos, {}, {stmts(m.body, "body", m.pos)}); }

            // expressions
            PN node(const py::Name& x, std::string f, const Pos& p) const { return make("Name", std::move(f), p, x.id, {make("ctx", "ctx", p, name(kCtx, x.ctx))}); }
            PN node(const py::Constant& x, std::string f, const Pos& p) const { return make("Constant", std::move(f), p, x.value, {make("kind", "kind", p, name(kConst, x.kind))}); }
            PN node(const py::Attribute& x, std::string f, const Pos& p) const { return make("Attribute", std::move(f), p, x.attr, {expr(x.value, "value", p), make("ctx", "ctx", p, name(kCtx, x.ctx))}); }
            PN node(const py::Subscript& x, std::string f, const Pos& p) const { return make("Subscript", std::move(f), p, {}, {expr(x.value, "value", p), expr(x.slice, "slice", p), make("ctx", "ctx", p, name(kCtx, x.ctx))}); }
            PN node(const py::Call& x, std::string f, const Pos& p) const { return make("Call", std::move(f), p, {}, {expr(x.func, "func", p), exprs(x.args, "args", p), keywords(x.keywords, "keywords", p)}); }
            PN node(const py::UnaryExpr& x, std::string f, const Pos& p) const { return make("UnaryExpr", std::move(f), p, name(kUnary, x.op), {expr(x.operand, "operand", p)}); }
            PN node(const py::BinaryExpr& x, std::string f, const Pos& p) const { return make("BinaryExpr", std::move(f), p, name(kBin, x.op), {expr(x.left, "left", p), expr(x.right, "right", p)}); }
            PN node(const py::BoolExpr& x, std::string f, const Pos& p) const { return make("BoolExpr", std::move(f), p, name(kBool, x.op), {exprs(x.values, "values", p)}); }
            PN node(const py::Compare& x, std::string f, const Pos& p) const {
                std::vector<PN> ops; for (auto o : x.ops) ops.push_back(make("op", {}, p, name(kCmp, o)));
                return make("Compare", std::move(f), p, {}, {expr(x.left, "left", p), make("sequence", "ops", p, {}, std::move(ops)), exprs(x.comparators, "comparators", p)});
            }
            PN node(const py::IfExpr& x, std::string f, const Pos& p) const { return make("IfExpr", std::move(f), p, {}, {expr(x.test, "test", p), expr(x.body, "body", p), expr(x.orelse, "orelse", p)}); }
            PN node(const py::Tuple& x, std::string f, const Pos& p) const { return make("Tuple", std::move(f), p, {}, {exprs(x.elts, "elts", p), make("ctx", "ctx", p, name(kCtx, x.ctx))}); }
            PN node(const py::List& x, std::string f, const Pos& p) const { return make("List", std::move(f), p, {}, {exprs(x.elts, "elts", p), make("ctx", "ctx", p, name(kCtx, x.ctx))}); }
            PN node(const py::Dict& x, std::string f, const Pos& p) const { return make("Dict", std::move(f), p, {}, {exprs(x.keys, "keys", p), exprs(x.values, "values", p)}); }

            // statements and declarations
            PN node(const py::FunctionDef& x, std::string f, const Pos& p) const {
                std::vector<PN> params;
                for (const auto& a : x.params)
                    params.push_back(make("Param", {}, a.pos, a.name, {expr(a.annotation, "annotation", a.pos), expr(a.default_value, "default_value", a.pos)}));
                return make("FunctionDef", std::move(f), p, x.name, {make("sequence", "params", p, {}, std::move(params)), expr(x.returns, "returns", p),
                    stmts(x.body, "body", p), exprs(x.decorators, "decorators", p), make("bool", "is_async", p, x.is_async ? "true" : "false")});
            }
            PN node(const py::ClassDef& x, std::string f, const Pos& p) const {
                return make("ClassDef", std::move(f), p, x.name, {exprs(x.bases, "bases", p), keywords(x.keywords, "keywords", p), stmts(x.body, "body", p), exprs(x.decorators, "decorators", p)});
            }
            PN node(const py::Return& x, std::string f, const Pos& p) const { return make("Return", std::move(f), p, {}, {expr(x.value, "value", p)}); }
            PN node(const py::Delete& x, std::string f, const Pos& p) const { return make("Delete", std::move(f), p, {}, {exprs(x.targets, "targets", p)}); }
            PN node(const py::Assign& x, std::string f, const Pos& p) const { return make("Assign", std::move(f), p, {}, {exprs(x.targets, "targets", p), expr(x.value, "value", p)}); }
            PN node(const py::AugAssign& x, std::string f, const Pos& p) const { return make("AugAssign", std::move(f), p, name(kBin, x.op), {expr(x.target, "target", p), expr(x.value, "value", p)}); }
            PN node(const py::AnnAssign& x, std::string f, const Pos& p) const { return make("AnnAssign", std::move(f), p, {}, {expr(x.target, "target", p), expr(x.annotation, "annotation", p), expr(x.value, "value", p)}); }
            PN node(const py::For& x, std::string f, const Pos& p) const {
                return make("For", std::move(f), p, {}, {expr(x.target, "target", p), expr(x.iter, "iter", p), stmts(x.body, "body", p), stmts(x.orelse, "orelse", p), make("bool", "is_async", p, x.is_async ? "true" : "false")});
            }
            PN node(const py::While& x, std::string f, const Pos& p) const { return make("While", std::move(f), p, {}, {expr(x.test, "test", p), stmts(x.body, "body", p), stmts(x.orelse, "orelse", p)}); }
            PN node(const py::If& x, std::string f, const Pos& p) const { return make("If", std::move(f), p, {}, {expr(x.test, "test", p), stmts(x.body, "body", p), stmts(x.orelse, "orelse", p)}); }
            PN node(const py::With& x, std::string f, const Pos& p) const {
                std::vector<PN> items;
                for (const auto& i : x.items) items.push_back(make("WithItem", {}, i.pos, {}, {expr(i.context, "context", i.pos), expr(i.vars, "vars", i.pos)}));
                return make("With", std::move(f), p, {}, {make("sequence", "items", p, {}, std::move(items)), stmts(x.body, "body", p), make("bool", "is_async", p, x.is_async ? "true" : "false")});
            }
            PN node(const py::Raise& x, std::string f, const Pos& p) const { return make("Raise", std::move(f), p, {}, {expr(x.exc, "exc", p), expr(x.cause, "cause", p)}); }
            PN node(const py::Try& x, std::string f, const Pos& p) const {
                std::vector<PN> handlers;
                for (const auto& h : x.handlers) handlers.push_back(make("ExceptHandler", {}, h.pos, h.name, {expr(h.type, "type", h.pos), stmts(h.body, "body", h.pos)}));
                return make("Try", std::move(f), p, {}, {stmts(x.body, "body", p), make("sequence", "handlers", p, {}, std::move(handlers)), stmts(x.orelse, "orelse", p), stmts(x.finalbody, "finalbody", p)});
            }
            PN node(const py::Assert& x, std::string f, const Pos& p) const { return make("Assert", std::move(f), p, {}, {expr(x.test, "test", p), expr(x.msg, "msg", p)}); }
            PN node(const py::Import& x, std::string f, const Pos& p) const { return make("Import", std::move(f), p, {}, {aliases(x.names, "names", p)}); }
            PN node(const py::ImportFrom& x, std::string f, const Pos& p) const { return make("ImportFrom", std::move(f), p, x.module_name, {aliases(x.names, "names", p), make("level", "level", p, std::to_string(x.level))}); }
            PN node(const py::Global& x, std::string f, const Pos& p) const { return make("Global", std::move(f), p, {}, {strings(x.names, "names", p)}); }
            PN node(const py::Nonlocal& x, std::string f, const Pos& p) const { return make("Nonlocal", std::move(f), p, {}, {strings(x.names, "names", p)}); }
            PN node(const py::ExprStmt& x, std::string f, const Pos& p) const { return make("ExprStmt", std::move(f), p, {}, {expr(x.value, "value", p)}); }
            PN node(const py::Pass&, std::string f, const Pos& p) const { return make("Pass", std::move(f), p); }
            PN node(const py::Break&, std::string f, const Pos& p) const { return make("Break", std::move(f), p); }
            PN node(const py::Continue&, std::string f, const Pos& p) const { return make("Continue", std::move(f), p); }
            PN node(const py::IspaMatch& x, std::string f, const Pos& p) const { return make("IspaMatch", std::move(f), p, x.source); }
        };
    }
    AST::PythonNode python(const UAST::py::ExprPtr& e, const UAST::SourcePosition& fallback) { return pylower::Lower{}.expr(e, {}, fallback); }
    AST::PythonNode python(const UAST::py::Expr& e, const UAST::SourcePosition& fallback) { return pylower::Lower{}.expr(std::make_shared<UAST::py::Expr>(e), {}, fallback); }
    AST::PythonNode python(const UAST::py::Module& m, const UAST::SourcePosition&) { return pylower::Lower{}.module(m); }
    template<class Params> stdu::vector<std::string> parameters(const Params& p) {
        stdu::vector<std::string> out;
        if (p.empty() || payload(p).value.empty()) return out;
        const auto& args = payload(payload(p).value);
        const auto parameterName = [](const auto& arg) {
            if constexpr (requires { payload(arg).name; }) return id(payload(arg).name);
            else return id(arg);
        };
        if (!args.first_p.empty()) out.push_back(parameterName(args.first_p));
        for (const auto& arg : args.following_p) out.push_back(parameterName(arg));
        return out;
    }
    // External Python actions carry the original PyRawParser nodes.
    // `at` is the node whose position labels the resulting action.
    template<class T, class At> AST::PythonAction action(const UAST::Node<T>& code, const At& at) {
        if (code.empty()) throw std::runtime_error("Cannot build AST from a missing Python expression or action");
        const auto& n = payload(code);
        if (n.value.empty()) throw std::runtime_error("Cannot build AST from an invalid Python expression or action");
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(payload(n.value))>, UAST::ExternalPythonExpression> ||
                      std::is_same_v<std::remove_cvref_t<decltype(payload(n.value))>, UAST::ExternalPythonStatements>) {
            const auto body = where(n.value);
            AST::PythonAction result;
            result.position = position(at);
            if constexpr (std::is_same_v<T, UAST::expr>) result.code = *UAST::convertPythonExpr(n.value, body);
            else result.code = UAST::convertPythonModule(n.value, body);
            bindPython(result);
            std::visit([&](const auto& syntax) {
                if constexpr (!std::is_same_v<std::decay_t<decltype(syntax)>, std::monostate>) result.python = python(syntax, body);
            }, result.code);
            return result;
        } else {
        auto body = where(n.value);      // text starts after the two-character opening delimiter
        body.offset += 2;
        body.column += 2;
        const auto& source = payload(n.value).value;
        AST::PythonAction result;
        result.position = position(at);
        if constexpr (std::is_same_v<T, UAST::expr>) result.code = *UAST::parsePythonExpr(source, body);
        else result.code = UAST::parsePythonModule(source, body);
        bindPython(result);
        std::visit([&](const auto& syntax) {
            if constexpr (!std::is_same_v<std::decay_t<decltype(syntax)>, std::monostate>) result.python = python(syntax, body);
        }, result.code);
        return result;
        }
    }
    template<class T> AST::PythonAction action(const UAST::Node<T>& code) { return action(code, code); }
    template<class Params> stdu::vector<AST::PythonAction> parameterAnnotations(const Params& p) {
        stdu::vector<AST::PythonAction> out;
        if (p.empty() || payload(p).value.empty()) return out;
        const auto& args = payload(payload(p).value);
        const auto append = [&](const auto& arg) {
            if constexpr (requires { payload(arg).annotation; }) {
                const auto& annotation = payload(arg).annotation;
                if (annotation.empty()) out.push_back(AST::PythonAction{});
                else if constexpr (requires { payload(annotation).base; }) {
                    std::string spelling = id(payload(annotation).base);
                    for (const auto& part : payload(annotation).nested) spelling += "." + id(part);
                    AST::PythonAction converted;
                    converted.position = position(annotation);
                    converted.code = *UAST::parsePythonExpr(spelling, where(annotation));
                    converted.python = python(std::get<UAST::py::Expr>(converted.code), where(annotation));
                    bindPython(converted);
                    out.push_back(std::move(converted));
                } else out.push_back(action(annotation));
            }
        };
        if (!args.first_p.empty()) append(args.first_p);
        for (const auto& arg : args.following_p) append(arg);
        return out;
    }
    [[noreturn]] void unsupported(const std::string& message) {
        throw std::runtime_error("ISPA bootstrap converter: unsupported " + message);
    }
    std::string unescapeString(std::string value) {
        // Lexer returns literal body, as in the original Builder. Preserve escape behavior.
        for (std::size_t i = 0; i < value.size(); ++i) {
            if (value[i] != '\\' || i + 1 == value.size()) continue;
            value.erase(i, 1);
            if (value[i] != '\'' && value[i] != '"')
                value[i] = corelib::text::getEscapedFromChar(value[i]);
        }
        return value;
    }
    AST::RuleMemberCsequence characterGroup(bool negative, const std::string& text) {
        AST::RuleMemberCsequence out;
        std::size_t i = 0;
        out.negative = negative;
        while (i < text.size()) {
            if (text[i] == '\\' && i + 1 < text.size()) {
                out.escaped.push_back(text[i + 1]); i += 2; continue;
            }
            if (i + 2 < text.size() && text[i + 1] == '-') {
                out.diapasons.push_back({text[i], text[i + 2]}); i += 3; continue;
            }
            out.characters.push_back(text[i++]);
        }
        return out;
    }

    // The generated parser uses both Node<Rules, Value> and
    // Node<Rules, shared_ptr<Value>> for recursive rule values.
    // Keep conversion inside Builder through the supplied callback.
    template<typename T, typename Converter>
    auto convertValues(const std::vector<T>& values, Converter&& convert)
        -> stdu::vector<std::shared_ptr<AST::RuleMember>>
    {
        stdu::vector<std::shared_ptr<AST::RuleMember>> result;
        for (const auto& node : values) {
            result.push_back(std::make_shared<AST::RuleMember>(
                convert(payload(node))
            ));
        }
        return result;
    }
    static AST::RegularDataBlock makeAtDataBlock() {
        return AST::At{};
    }
}

AST::RuleMember AST::Builder::convertPattern(const Pattern& pattern) {
    AST::RuleMember output;
    std::visit([&](const auto& item) {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            unsupported("empty pattern");
        } else if constexpr (std::is_same_v<T, UAST::Node<UAST::rule_value_STRING>>) {
            output.value = AST::String{unescapeString(payload(item).value)};
        } else if constexpr (std::is_same_v<T, UAST::Node<UAST::rule_value_CGROUP>>) {
            output.value = characterGroup(payload(item).negative == '^', payload(item).v);
        } else if constexpr (std::is_same_v<T, UAST::Node<UAST::rule_value_NOSPACE>>) {
            output.value = AST::RuleMemberNospace{};
        } else if constexpr (std::is_same_v<T, UAST::Node<UAST::rule_value_SPACE>>) {
            output.features.push_back(AST::SpacePattern{position(item)});
        } else if constexpr (std::is_same_v<T, UAST::Node<UAST::rule_name>> || std::is_same_v<T, UAST::Node<UAST::call_name>>) {
            const auto& n = payload(item);
            if (!n.temp_params.empty() || !n.param.empty())
                output.features.push_back(AST::DeclarationSyntax{{}, parameters(n.param), parameters(n.temp_params), position(item)});
            // Resolve references after all declarations have been collected.
            auto path = qualifiedName(n);
            AST::RuleMemberName reference{path};
            for (const auto& argument : parameters(n.temp_params))
                reference.templateArguments.push_back({argument});
            output.value = std::move(reference);
        } else if constexpr (std::is_same_v<T, UAST::Node<std::shared_ptr<UAST::rule_value_group>>>) {
            auto children = convertValues(payload(item).value, [this](const Value& v) { return convertValue(v); });
            output.value = AST::RuleMemberGroup{std::move(children)};
        }
        else if constexpr (std::is_same_v<T, UAST::Node<UAST::rule_value_ANY>>) {
            output.value = AST::RuleMemberAny{};
        }
        else if constexpr (std::is_same_v<T, UAST::Node<std::shared_ptr<UAST::rule_value_not_a>>>) {
            UAST::rule_value_pattern operand;
            std::visit([&](const auto& value) { operand.value = value; }, payload(item).value);
            output = convertPattern(operand);
            output.negative_match = !output.negative_match;
        }
        else {
            AST::DynamicPattern dynamic;
            dynamic.position = position(item);
            std::visit([&](const auto& n) {
                using D = std::decay_t<decltype(n)>;
                if constexpr (std::is_same_v<D, UAST::Node<UAST::ID>>) dynamic.name = id(n);
                else if constexpr (!std::is_same_v<D, std::monostate>) dynamic.expression = action(payload(n).value);
            }, payload(item).value);
            output.features.push_back(std::move(dynamic));
        }
    }, pattern.value);
    return output;
}

AST::RuleMember AST::Builder::convertValue(const Value& value) {
    return std::visit([this](const auto& n) -> AST::RuleMember {
        using T = std::decay_t<decltype(n)>;
        if constexpr (std::is_same_v<T, UAST::Node<std::shared_ptr<UAST::rule_value_one_capture>>>)
            return convertCapture(payload(n));
        else if constexpr (std::is_same_v<T, UAST::Node<std::shared_ptr<UAST::rule_value_mul_capture>>>) {
            AST::RuleMember out;
            out.position = position(n);
            AST::RuleMemberGroup group;
            for (const auto& c : payload(n).cap)
                group.values.push_back(std::make_shared<AST::RuleMember>(convertCapture(payload(c))));
            out.value = std::move(group);
            const char kind = captureMarker(payload(n).type);
            out.features.push_back(AST::RegionCapture{kind, out.position});
            return out;
        } else unsupported("absent rule value");
    }, value.value);
}

AST::RuleMember AST::Builder::convertCapture(const UAST::rule_value_one_capture& value) {
    AST::RuleMember output = convertPattern(payload(value.val));
    output.position = position(value.val);
    auto patternFeatures = std::move(output.features);
    output.features.clear();
    if (!value.registration.empty())
        output.features.push_back(AST::SharedCounterRegistration{id(payload(value.registration).value), position(value.registration)});
    if (!value.v_cap.empty())
        output.features.push_back(AST::VariableCapture{id(payload(value.v_cap).value), position(value.v_cap)});
    if (!value.fail_cap.empty()) {
        AST::FailMarker marker;
        marker.position = position(value.fail_cap);
        const auto& n = payload(value.fail_cap).value;
        if (!n.empty()) { marker.name = qualifiedName(payload(n)); marker.parameters = parameters(payload(n).param); }
        output.features.push_back(std::move(marker));
    }

    if (!value.ast_cap.empty()) {
        output.prefix.present = true;

        const auto& capture = payload(value.ast_cap);

        if (!capture.value.empty())
            output.prefix.name = id(capture.value);
    }
    if (!value.alts.empty()) {
        AST::RuleMemberOp alternatives;
        auto original = output;
        original.prefix.clear();
        original.features = std::move(patternFeatures);
        alternatives.options.push_back(std::make_shared<AST::RuleMember>(std::move(original)));
        for (const auto& alt : value.alts) {
            auto next = convertPattern(payload(payload(alt).value));
            alternatives.options.push_back(std::make_shared<AST::RuleMember>(std::move(next)));
        }
        output.value = std::move(alternatives);
    } else {
        output.features.insert(output.features.begin(), patternFeatures.begin(), patternFeatures.end());
    }

    if (!value.quantifier.empty()) {
        const auto& q = payload(value.quantifier);
        auto bounded = [&](const auto& repetitions) {
            AST::Counter counter;
            counter.position = position(repetitions);
            auto convertBound = [&](const auto& n) {
                using T = std::decay_t<decltype(n)>;
                if constexpr (std::is_same_v<T, UAST::Node<UAST::ID>>) { counter.kind = "name"; counter.bound = id(n); }
                else if constexpr (requires { payload(n).full; payload(n).floating; }) {
                    // Accept the previous bootstrap's numeric counter nodes
                    // while bootstrapping the direct-expression grammar.
                    counter.kind = "number";
                    counter.bound = payload(n).full;
                    if (!payload(n).floating.empty()) counter.bound += "." + payload(n).floating;
                }
                else if constexpr (std::is_same_v<T, UAST::Node<UAST::expr>>) {
                    counter.kind = "expression";
                    counter.expression = action(n);
                    if (const auto* e = std::get_if<UAST::py::Expr>(&counter.expression.code)) {
                        if (auto name = e->template get<UAST::py::Name>()) {
                            counter.kind = "name";
                            counter.bound = name->id;
                            counter.expression = {};
                        } else if (auto number = e->template get<UAST::py::Constant>(); number &&
                            (number->kind == UAST::py::ConstantKind::Int || number->kind == UAST::py::ConstantKind::Float)) {
                            counter.kind = "number";
                            counter.bound = number->value;
                            counter.expression = {};
                        }
                    }
                }
            };
            const auto& bound = payload(repetitions).value;
            if constexpr (std::is_same_v<std::remove_cvref_t<decltype(bound)>, UAST::Node<UAST::expr>>)
                convertBound(bound);
            else std::visit(convertBound, bound);
            output.features.push_back(std::move(counter));
        };
        std::visit([&](const auto& node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (!std::is_same_v<T, std::monostate>) {
                if constexpr (std::is_same_v<T, UAST::Node<UAST::rule_value_counter_bounded>>) {
                    output.quantifier = '*';
                    bounded(node);
                } else {
                    const auto& quantified = payload(node);
                    if (!quantified.quantifier.empty()) output.quantifier = payload(quantified.quantifier).value;
                    if (!quantified.repetitions.empty()) bounded(quantified.repetitions);
                }
            }
        }, q.value);
    }
    return output;
}

AST::DataBlock AST::Builder::convertData(
    const UAST::Node<UAST::rule_ast>& data) {
    AST::DataBlock result;
    if (data.empty()) return result;
    const auto& d = payload(data);
    std::visit([&](const auto& item) {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, UAST::Node<UAST::rule_ast_abbr>>) {
            AST::TemplatedDataBlock out;
            out.names.push_back(id(payload(item).f));
            for (const auto& v : payload(item).seq) out.names.push_back(id(v));
            result.value = std::move(out);
        } else if constexpr (std::is_same_v<T, UAST::Node<UAST::rule_ast_single_value>>) {
            const auto& single = payload(item);

            std::visit([&](const auto& value) {
                using Value = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<Value, UAST::Node<UAST::COMMERCIAL_AT>>) {
                    result.value = makeAtDataBlock();
                } else if constexpr (std::is_same_v<Value, char>) {
                    if (value != '@') unsupported("invalid data block marker");
                    result.value = makeAtDataBlock();
                } else if constexpr (std::is_same_v<Value, UAST::Node<UAST::expr>>) {
                    auto initializer = action(value);
                    initializer.target = "value";
                    rule_features.push_back(std::move(initializer));
                } else unsupported("absent data block value");
            }, single.value);
        } else if constexpr (std::is_same_v<
            T,
            UAST::Node<
                UAST::rule_ast_full
            >
        >) {
            for (const auto& part : payload(item).value) {
                auto a = action(payload(part).e, part);
                a.target = id(payload(part).key);
                rule_features.push_back(std::move(a));
            }
        } else if constexpr (std::is_same_v<T, std::monostate>) {
            // absent data block
        }
    }, d.value);
    return result;
}

void AST::Builder::convertRule(const Rule& rule) {
    AST::Rule output;
    output.rule_members = convertValues(rule.v, [this](const Value& v) { return convertValue(v); });
    output.data_block = convertData(rule.ast_pointer);
    output.features = std::move(rule_features);
    rule_features.clear();
    if (!rule.fail_pointer.empty()) {
        AST::FailBinding binding;
        binding.position = position(rule.fail_pointer);
        binding.names.push_back(id(payload(rule.fail_pointer).f));
        for (const auto& n : payload(rule.fail_pointer).seq) binding.names.push_back(id(n));
        output.features.push_back(std::move(binding));
    }
    output.isTopLevel = fullname.size() == module_scope_size + 1;
    tree_map[fullname] = std::move(output);

    // convertGrammar() owns exactly one scope push/pop per declaration.
    for (const auto& nested : rule.nested) {
        const auto& grammar = payload(payload(nested).value);
        convertGrammar(grammar);
    }

}
void AST::Builder::convertGrammar(const Grammar& grammar) {
    const auto path = qualifiedName(payload(grammar.name));
    const auto scopeSize = fullname.size();
    fullname.insert(fullname.end(), path.begin(), path.end());

    AST::DeclarationSyntax declaration;
    declaration.position = position(grammar.name);
    declaration.parameters = parameters(payload(grammar.name).param);
    declaration.parameterAnnotations = parameterAnnotations(payload(grammar.name).param);
    declaration.templateParameters = parameters(payload(grammar.name).temp_params);
    std::set<std::string> parameter_names;
    for (const auto& parameter : declaration.templateParameters) {
        if (parameter == "IT") unsupported("reserved template parameter name IT");
        if (!parameter_names.insert(parameter).second) unsupported("duplicate template parameter " + parameter);
    }
    for (const auto& parameter : declaration.parameters) {
        if (parameter == "ctx") unsupported("reserved rule parameter name ctx");
        if (!parameter_names.insert(parameter).second) unsupported("duplicate rule parameter " + parameter);
    }
    for (const auto& mod : grammar.mod) declaration.modifiers.push_back(payload(payload(mod).value).value);
    if (!declaration.modifiers.empty() || !declaration.parameters.empty() || !declaration.templateParameters.empty())
        rule_features.push_back(std::move(declaration));
    using OrdinaryNode = UAST::Node<std::shared_ptr<Rule>>;
    if (const auto* ordinary = std::get_if<OrdinaryNode>(&grammar.entry)) {
        convertRule(payload(*ordinary));
    } else {
        AST::Rule out;
        out.features = std::move(rule_features);
        rule_features.clear();
        const auto& advanced = std::get<UAST::Node<UAST::advanced_rule>>(grammar.entry);
        out.features.push_back(AST::AdvancedRule{python(UAST::describe(payload(advanced), where(advanced))), position(advanced)});
        for (const auto& entry : payload(advanced).body) {
            if (const auto* statement = std::get_if<UAST::Node<UAST::stmt>>(&entry)) {
                auto code = action(*statement);
                out.features.push_back(code);
                if (!corelib::text::isUpper(fullname.back())) {
                    auto member = std::make_shared<AST::RuleMember>();
                    member->value = AST::RuleMemberGroup{}; // zero-width parser action
                    member->position = position(*statement);
                    member->features.push_back(std::move(code));
                    out.rule_members.push_back(std::move(member));
                }
            } else if (const auto* match = std::get_if<UAST::Node<UAST::advanced_rule_regular_match>>(&entry)) {
                const auto& syntax = payload(*match);
                auto members = convertValues(syntax.patterns, [this](const Value& v) { return convertValue(v); });
                if (!syntax.stream.empty()) {
                    if (members.size() != 1 || !members.front()->isName() || members.front()->quantifier != '\0')
                        unsupported("match(stream) requires one unquantified rule name");
                    if (!members.front()->getName().templateArguments.empty() ||
                        std::ranges::any_of(members.front()->features, [](const auto& feature) {
                            return std::holds_alternative<AST::DeclarationSyntax>(feature);
                        })) unsupported("match(stream) does not support rule arguments yet");
                    std::string capture;
                    if (!syntax.assign_names.empty()) {
                        const auto& names = payload(syntax.assign_names).value;
                        if (names.size() != 1) unsupported("match(stream) requires one capture name");
                        capture = id(names.front());
                    }
                    auto action_member = std::make_shared<AST::RuleMember>();
                    action_member->value = AST::RuleMemberGroup{};
                    action_member->position = position(*match);
                    action_member->features.push_back(AST::StreamMatch{
                        id(payload(syntax.stream).value), members.front()->getName().name,
                        std::move(capture), position(*match)});
                    out.rule_members.push_back(std::move(action_member));
                    continue;
                }
                if (!syntax.assign_names.empty()) {
                    const auto& names = payload(syntax.assign_names).value;
                    if (names.size() != members.size()) unsupported("match unpack requires one name per pattern");
                    for (std::size_t i = 0; i < names.size(); ++i)
                        members[i]->features.push_back(AST::VariableCapture{id(names[i]), position(names[i])});
                }
                out.rule_members.insert(out.rule_members.end(), members.begin(), members.end());
            }
        }
        if (out.rule_members.empty()) {
            auto epsilon = std::make_shared<AST::RuleMember>();
            epsilon->value = AST::RuleMemberGroup{};
            epsilon->features.push_back(AST::AdvancedRule{});
            out.rule_members.push_back(std::move(epsilon));
        }
        tree_map[fullname] = std::move(out);
    }
    tree_map.at(fullname).position = position(grammar.name);
    if (module_project && module_project->modular) {
        const auto& source = module_project->source(*currentFile);
        Modules::Path relative(fullname.begin() + module_scope_size, fullname.end());
        if (const auto declared = source.declarations.find(relative); declared != source.declarations.end()) {
            const auto& p = declared->second;
            tree_map.at(fullname).position = {p.file, p.offset, 0, p.line, p.column};
        }
    }
    fullname.resize(scopeSize);
}

void AST::Builder::convertMain(const UAST::main& main) {
    std::size_t source_index = 0;
    for (const auto& entry : main.value) {
        FileScope source_scope(source_files.empty() ? file : source_files.at(source_index));
        fullname.clear();
        if (module_project && module_project->modular) {
            fullname.push_back("modules");
            const auto& source = module_project->source(*currentFile);
            fullname.insert(fullname.end(), source.name.begin(), source.name.end());
        }
        module_scope_size = fullname.size();
        ++source_index;
        std::visit([&](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, UAST::Node<UAST::grammar>>) {
                convertGrammar(payload(value));
            } else if constexpr (std::is_same_v<T, UAST::Node<UAST::fail_block>>) {
                const auto& block = payload(value);
                const auto& n = payload(block.name);
                auto block_name = fullname;
                const auto local_name = qualifiedName(n);
                block_name.insert(block_name.end(), local_name.begin(), local_name.end());
                auto code = block.val.empty() ? AST::PythonAction{} : action(block.val);
                if (block.val.empty()) code.code = UAST::py::Module{};
                fail_blocks.push_back(AST::FailBlock{std::move(block_name), parameters(n.param), std::move(code), position(value)});
            }
        }, entry);
    }
}


void AST::Builder::build() {
    tree_map.clear();
    fail_blocks.clear();
    rule_features.clear();
    fullname.clear();
    FileScope fileScope(file);
    Python::Imports pythonImports(module_project);
    PythonScope pythonScope(pythonImports);
    python_declarations = pythonImports.takeDeclarations();

    convertMain(input);
    if (file != "<embedded match>") {
        std::set<std::string> existing;
        const auto collect_literal = [&](auto&& self, const AST::RuleMember& member, std::set<std::string>& into) -> void {
            if (member.isString()) into.insert(member.getString().value);
            if (member.isGroup()) for (const auto& child : member.getGroup().values) self(self, *child, into);
            if (member.isOp()) for (const auto& child : member.getOp().options) self(self, *child, into);
        };
        for (const auto& [_, rule] : tree_map)
            for (const auto& member : rule.rule_members) collect_literal(collect_literal, *member, existing);
        std::set<std::string> embedded_literals;
        const auto scan_body = [&](auto&& self, const UAST::py::StmtList& body) -> void {
            for (const auto& statement : body) if (statement) {
                if (const auto* match = statement->get<UAST::py::IspaMatch>()) {
                    const std::string text = "embedded_action { " + match->source + " }";
                    IspaParser parser;
                    const auto& syntax = parser.parse(text.c_str());
                    AST::Builder embedded_builder(syntax, "<embedded match>");
                    embedded_builder.build();
                    auto embedded = embedded_builder.get();
                    for (const auto& member : embedded.getTreeMap().at(stdu::vector<std::string>{"embedded_action"}).rule_members)
                        collect_literal(collect_literal, *member, embedded_literals);
                } else if (const auto* branch = statement->get<UAST::py::If>()) {
                    self(self, branch->body); self(self, branch->orelse);
                } else if (const auto* loop = statement->get<UAST::py::While>()) {
                    self(self, loop->body); self(self, loop->orelse);
                } else if (const auto* loop = statement->get<UAST::py::For>()) {
                    self(self, loop->body); self(self, loop->orelse);
                }
            }
        };
        for (const auto& [_, rule] : tree_map)
            for (const auto& member : rule.rule_members)
                for (const auto& feature : member->features)
                    if (const auto* action = std::get_if<AST::PythonAction>(&feature))
                        if (const auto* module = std::get_if<UAST::py::Module>(&action->code)) scan_body(scan_body, module->body);
        for (const auto& literal : embedded_literals) if (!existing.contains(literal)) {
            std::string name = "ISPA_EMBEDDED_";
            constexpr char digits[] = "0123456789ABCDEF";
            for (unsigned char c : literal) { name += digits[c >> 4]; name += digits[c & 15]; }
            if (literal.empty()) name += "EMPTY";
            AST::Rule token;
            token.rule_members.push_back(std::make_shared<AST::RuleMember>(AST::RuleMember{.value = AST::String{literal}}));
            tree_map.emplace(stdu::vector<std::string>{name}, std::move(token));
        }
    }
    fullname.clear();
    if (module_project) for (const auto& [name, symbol] : module_project->external_symbols) {
        stdu::vector<std::string> full{"modules"};
        full.insert(full.end(), name.begin(), name.end());
        stdu::vector<std::string> marker{"__ISPA_EXTERNAL_" + std::to_string(tree_map.size())};
        AST::Rule token;
        token.external_token = true;
        token.rule_members.push_back(std::make_shared<AST::RuleMember>(AST::RuleMember{.value = AST::RuleMemberAny{}}));
        tree_map.emplace(marker, std::move(token));
        AST::Rule proxy;
        proxy.external = AST::ExternalRule{symbol.cpp_type, symbol.cpp_binding, symbol.header, marker};
        proxy.rule_members.push_back(std::make_shared<AST::RuleMember>(AST::RuleMember{.value = AST::RuleMemberName{.name = marker}}));
        tree_map.emplace(std::move(full), std::move(proxy));
    }

    // Search the containing rule and then its enclosing scopes using the
    // entire reference path. Never fall back to a matching final segment:
    // rule.ast must not accidentally bind to a global ast declaration.
    for (auto& [scope, rule] : tree_map) {
        if (rule.external || rule.external_token) continue;
        const Modules::Source* source = module_project && module_project->modular
            ? &module_project->source(rule.position.file) : nullptr;
        const std::size_t minimum_scope = source ? source->name.size() + 1 : 0;
        stdu::vector<std::string> template_parameters;
        for (const auto& feature : rule.features)
            if (const auto* syntax = std::get_if<AST::DeclarationSyntax>(&feature))
                template_parameters = syntax->templateParameters;
        const auto resolveName = [&](stdu::vector<std::string>& reference) {
            bool resolved = false;
            for (std::size_t size = scope.size();; --size) {
                stdu::vector<std::string> candidate(scope.begin(), scope.begin() + size);
                candidate.insert(candidate.end(), reference.begin(), reference.end());
                if (tree_map.contains(candidate) && (!source || tree_map.at(candidate).position.file == source->file)) {
                    reference = std::move(candidate);
                    resolved = true;
                    break;
                }
                if (size == minimum_scope) break;
            }
            if (!resolved && source) {
                Modules::Path local(reference.begin(), reference.end());
                const auto found = source->resolved_symbols.find(local);
                if (found == source->resolved_symbols.end())
                    throw std::runtime_error(source->file + ":" + std::to_string(rule.position.line) +
                        ": unknown or unimported symbol " + Modules::spelling(local));
                reference = {"modules"};
                reference.insert(reference.end(), found->second.begin(), found->second.end());
                if (!tree_map.contains(reference)) throw std::runtime_error("Missing module declaration: " + Modules::spelling(found->second));
            }
        };
        const auto resolve = [&](const auto& self, AST::RuleMember& member) -> void {
            for (auto& feature : member.features)
                if (auto* stream = std::get_if<AST::StreamMatch>(&feature)) resolveName(stream->rule);
            for (const auto& feature : member.features) if (const auto* dynamic = std::get_if<AST::DynamicPattern>(&feature)) {
                auto parameter = dynamic->name;
                if (const auto* expression = std::get_if<UAST::py::Expr>(&dynamic->expression.code))
                    if (const auto* variable = expression->get<UAST::py::Name>()) parameter = variable->id;
                if (std::ranges::find(template_parameters, parameter) != template_parameters.end()) {
                    member.value = AST::RuleMemberName{.name = {parameter}};
                    break;
                }
            }
            if (member.isName()) std::erase_if(member.features, [](const auto& feature) {
                return std::holds_alternative<AST::DynamicPattern>(feature);
            });
            if (member.isName()) {
                auto& reference = member.getName();
                if (reference.name.size() == 1 &&
                    std::ranges::find(template_parameters, reference.name.front()) != template_parameters.end())
                    reference.templateParameter = reference.name.front();
                else resolveName(reference.name);
                for (auto& argument : reference.templateArguments) {
                    const bool parameter = argument.size() == 1 &&
                        std::ranges::find(template_parameters, argument.front()) != template_parameters.end();
                    reference.templateArgumentIsParameter.push_back(parameter);
                    if (!parameter) resolveName(argument);
                }
            } else if (member.isGroup()) {
                for (auto& child : member.getGroup().values) self(self, *child);
            } else if (member.isOp()) {
                for (auto& child : member.getOp().options) self(self, *child);
            }
        };
        for (auto& member : rule.rule_members) resolve(resolve, *member);
        inferDataTypes(rule);
    }
    if (module_project && module_project->modular) specializeModules();

    // A lexer is one finite automaton. Instantiate its grammar templates
    // before NFA construction, while retaining a shared templated node type.
    using TemplateName = stdu::vector<std::string>;
    for (auto& [name, rule] : tree_map) if (AST::RuleMemberName{.name = name}.isTerminal())
        for (const auto& feature : rule.features)
            if (const auto* syntax = std::get_if<AST::DeclarationSyntax>(&feature))
                rule.isTemplateDefinition = !syntax->templateParameters.empty();
    const auto cloneMember = [&](const auto& self, const AST::RuleMember& source) -> std::shared_ptr<AST::RuleMember> {
        auto result = std::make_shared<AST::RuleMember>(source);
        if (result->isGroup()) for (auto& child : result->getGroup().values) child = self(self, *child);
        if (result->isOp()) for (auto& child : result->getOp().options) child = self(self, *child);
        return result;
    };
    std::map<std::pair<TemplateName, stdu::vector<TemplateName>>, TemplateName> instances;
    std::set<TemplateName> instance_names;
    std::map<TemplateName, AST::Rule> lexer_instances;
    std::function<void(AST::RuleMember&)> instantiateCalls;
    std::function<TemplateName(const AST::RuleMemberName&)> instantiate = [&](const AST::RuleMemberName& call) {
        const auto key = std::make_pair(call.name, call.templateArguments);
        if (auto found = instances.find(key); found != instances.end()) return found->second;
        const auto& definition = tree_map.at(call.name);
        const AST::DeclarationSyntax* syntax = nullptr;
        for (const auto& feature : definition.features)
            if (const auto* declaration = std::get_if<AST::DeclarationSyntax>(&feature)) syntax = declaration;
        if (!syntax || call.templateArguments.size() != syntax->templateParameters.size())
            throw std::runtime_error("wrong number of lexer template arguments for " + corelib::text::join(call.name, "."));
        if (std::ranges::any_of(call.templateArgumentIsParameter, [](bool parameter) { return parameter; }))
            throw std::runtime_error("lexer template arguments must name concrete rules: " + corelib::text::join(call.name, "."));
        TemplateName instance;
        for (std::size_t index = instances.size();; ++index) {
            instance = {"ISPA_TEMPLATE_INSTANCE_" + std::to_string(index)};
            if (!tree_map.contains(instance) && !instance_names.contains(instance)) break;
        }
        instance_names.insert(instance);
        instances.emplace(key, instance);
        auto specialized = definition;
        specialized.isTemplateDefinition = false;
        specialized.isTopLevel = false;
        specialized.templateOrigin = call;
        specialized.rule_members.clear();
        for (const auto& member : definition.rule_members) specialized.rule_members.push_back(cloneMember(cloneMember, *member));
        for (auto& feature : specialized.features)
            if (auto* declaration = std::get_if<AST::DeclarationSyntax>(&feature)) declaration->templateParameters.clear();
        const auto substitute = [&](const auto& self, AST::RuleMember& member) -> void {
            if (member.isName()) {
                auto& reference = member.getName();
                if (!reference.templateParameter.empty()) {
                    const auto parameter = std::ranges::find(syntax->templateParameters, reference.templateParameter);
                    reference.name = call.templateArguments.at(parameter - syntax->templateParameters.begin());
                    reference.templateParameter.clear();
                }
                for (std::size_t i = 0; i < reference.templateArguments.size(); ++i)
                    if (i < reference.templateArgumentIsParameter.size() && reference.templateArgumentIsParameter[i]) {
                        const auto parameter = std::ranges::find(syntax->templateParameters, reference.templateArguments[i].front());
                        reference.templateArguments[i] = call.templateArguments.at(parameter - syntax->templateParameters.begin());
                        reference.templateArgumentIsParameter[i] = false;
                    }
            }
            if (member.isGroup()) for (auto& child : member.getGroup().values) self(self, *child);
            if (member.isOp()) for (auto& child : member.getOp().options) self(self, *child);
            instantiateCalls(member);
        };
        for (auto& member : specialized.rule_members) substitute(substitute, *member);
        inferDataTypes(specialized);
        lexer_instances.emplace(instance, std::move(specialized));
        return instance;
    };
    instantiateCalls = [&](AST::RuleMember& member) {
        if (member.isName() && tree_map.contains(member.getName().name) && tree_map.at(member.getName().name).isTemplateDefinition) {
            member.getName().name = instantiate(member.getName());
            member.getName().templateArguments.clear();
            member.getName().templateArgumentIsParameter.clear();
            std::erase_if(member.features, [](const auto& feature) {
                const auto* syntax = std::get_if<AST::DeclarationSyntax>(&feature);
                return syntax && syntax->parameters.empty();
            });
        }
    };
    const auto visitCalls = [&](const auto& self, AST::RuleMember& member) -> void {
        instantiateCalls(member);
        if (member.isGroup()) for (auto& child : member.getGroup().values) self(self, *child);
        if (member.isOp()) for (auto& child : member.getOp().options) self(self, *child);
    };
    for (auto& [name, rule] : tree_map) if (!rule.isTemplateDefinition) {
        for (auto& member : rule.rule_members) visitCalls(visitCalls, *member);
        inferDataTypes(rule);
    }
    for (auto& [name, rule] : lexer_instances) tree_map.emplace(std::move(name), std::move(rule));

    // A lexer negation consumes exactly one character outside the operand's
    // single-character language. Resolve template arguments before computing
    // that language (for example !(${q} | [\n\r]) in STRING<q>).
    const auto negatedCharacters = [&](const auto& self, const AST::RuleMember& member,
                                       std::set<TemplateName>& visiting) -> std::set<unsigned char> {
        constexpr int alphabet_size = std::numeric_limits<char>::max() + 1;
        if (member.negative_match || member.quantifier != '\0')
            throw std::runtime_error("negated lexer pattern must contain only single-character alternatives");
        if (member.isString()) {
            const auto& text = member.getString().value;
            if (text.size() != 1) throw std::runtime_error("negated lexer string must be one character");
            if (static_cast<unsigned char>(text.front()) >= alphabet_size)
                throw std::runtime_error("negated lexer pattern contains a character outside the lexer alphabet");
            return {static_cast<unsigned char>(text.front())};
        }
        if (member.isCsequence()) {
            const auto& chars = member.getCsequence();
            std::set<unsigned char> result;
            for (char ch : chars.characters) if (static_cast<unsigned char>(ch) < alphabet_size)
                result.insert(static_cast<unsigned char>(ch));
            for (char ch : chars.escaped) {
                const auto value = static_cast<unsigned char>(corelib::text::getEscapedFromChar(ch));
                if (value < alphabet_size) result.insert(value);
            }
            for (const auto& [first, last] : chars.diapasons)
                for (int ch = static_cast<unsigned char>(first); ch <= static_cast<unsigned char>(last); ++ch)
                    if (ch < alphabet_size) result.insert(static_cast<unsigned char>(ch));
            if (chars.negative) {
                std::set<unsigned char> complement;
                for (int ch = 0; ch < alphabet_size; ++ch) if (!result.contains(static_cast<unsigned char>(ch)))
                    complement.insert(static_cast<unsigned char>(ch));
                return complement;
            }
            return result;
        }
        if (member.isAny()) {
            std::set<unsigned char> result;
            for (int ch = 0; ch < alphabet_size; ++ch) result.insert(static_cast<unsigned char>(ch));
            return result;
        }
        if (member.isOp()) {
            std::set<unsigned char> result;
            for (const auto& option : member.getOp().options) {
                auto part = self(self, *option, visiting);
                result.insert(part.begin(), part.end());
            }
            return result;
        }
        if (member.isGroup()) {
            const auto& values = member.getGroup().values;
            if (values.size() != 1) throw std::runtime_error("negated lexer group must match one character");
            return self(self, *values.front(), visiting);
        }
        if (member.isName()) {
            const auto& name = member.getName().name;
            if (!tree_map.contains(name) || !visiting.insert(name).second)
                throw std::runtime_error("negated lexer pattern has an unresolved or recursive rule");
            const auto& rule = tree_map.at(name);
            if (rule.rule_members.size() != 1)
                throw std::runtime_error("negated lexer rule must match one character");
            auto result = self(self, *rule.rule_members.front(), visiting);
            visiting.erase(name);
            return result;
        }
        throw std::runtime_error("negated lexer pattern must match one character");
    };
    const auto lowerNegations = [&](const auto& self, AST::RuleMember& member, bool lexer) -> void {
        if (member.negative_match && lexer) {
            auto operand = member;
            operand.negative_match = false;
            operand.quantifier = '\0';
            std::set<TemplateName> visiting;
            const auto excluded = negatedCharacters(negatedCharacters, operand, visiting);
            AST::RuleMemberCsequence complement;
            complement.negative = true;
            for (unsigned char ch : excluded) complement.characters.push_back(static_cast<char>(ch));
            member.value = std::move(complement);
            member.negative_match = false;
        }
        if (member.isGroup()) for (auto& child : member.getGroup().values) self(self, *child, lexer);
        if (member.isOp()) for (auto& child : member.getOp().options) self(self, *child, lexer);
    };
    for (auto& [name, rule] : tree_map) if (!rule.isTemplateDefinition)
        for (auto& member : rule.rule_members)
            lowerNegations(lowerNegations, *member, AST::RuleMemberName{.name = name}.isTerminal());

    // Grammar template names need FIRST sets during LL lowering. Give each
    // placeholder a lightweight analysis rule whose alternatives are the
    // concrete rules supplied at call sites; it is never emitted as a parser
    // function or AST node.
    using RuleName = stdu::vector<std::string>;
    const auto placeholder = [](RuleName owner, const std::string& parameter) {
        owner.push_back("ispa_template_" + parameter);
        return owner;
    };
    std::map<RuleName, std::set<RuleName>> candidates;
    std::map<RuleName, std::set<RuleName>> forwards;
    for (const auto& [owner, rule] : tree_map) {
        const auto gather = [&](const auto& self, const AST::RuleMember& member) -> void {
            if (member.isName() && tree_map.contains(member.getName().name)) {
                const auto& reference = member.getName();
                for (const auto& feature : tree_map.at(reference.name).features)
                    if (const auto* syntax = std::get_if<AST::DeclarationSyntax>(&feature)) {
                        for (std::size_t i = 0; i < reference.templateArguments.size() &&
                            i < syntax->templateParameters.size(); ++i) {
                            const auto target = placeholder(reference.name, syntax->templateParameters[i]);
                            if (i < reference.templateArgumentIsParameter.size() &&
                                reference.templateArgumentIsParameter[i])
                                forwards[target].insert(placeholder(owner, reference.templateArguments[i].front()));
                            else candidates[target].insert(reference.templateArguments[i]);
                        }
                    }
            }
            if (member.isGroup()) for (const auto& child : member.getGroup().values) self(self, *child);
            if (member.isOp()) for (const auto& child : member.getOp().options) self(self, *child);
        };
        for (const auto& member : rule.rule_members) gather(gather, *member);
    }
    bool changed;
    do {
        changed = false;
        for (const auto& [target, sources] : forwards)
            for (const auto& source : sources)
                for (const auto& candidate : candidates[source])
                    changed |= candidates[target].insert(candidate).second;
    } while (changed);
    for (auto& [owner, rule] : tree_map) {
        if (AST::RuleMemberName{.name = owner}.isTerminal()) continue;
        for (const auto& feature : rule.features)
            if (const auto* syntax = std::get_if<AST::DeclarationSyntax>(&feature);
                syntax && !syntax->templateParameters.empty()) {
                const bool instantiated = std::ranges::any_of(syntax->templateParameters, [&](const auto& parameter) {
                    return !candidates[placeholder(owner, parameter)].empty();
                });
                rule.isTemplateDefinition = !instantiated;
            }
    }
    std::map<RuleName, AST::Rule> placeholders;
    for (auto& [owner, rule] : tree_map) {
        if (rule.isTemplateDefinition) continue;
        const auto rewrite = [&](const auto& self, AST::RuleMember& member) -> void {
            if (member.isName() && !member.getName().templateParameter.empty()) {
                auto key = placeholder(owner, member.getName().templateParameter);
                member.getName().name = key;
                if (!placeholders.contains(key)) {
                    if (candidates[key].empty())
                        throw std::runtime_error("unbound grammar template parameter " +
                            member.getName().templateParameter + " in " + corelib::text::join(owner, "."));
                    AST::Rule analysis;
                    analysis.isTemplatePlaceholder = true;
                    analysis.isTopLevel = false;
                    AST::RuleMemberOp options;
                    for (const auto& candidate : candidates[key])
                        options.options.push_back(std::make_shared<AST::RuleMember>(AST::RuleMember{
                            .value = AST::RuleMemberName{.name = candidate}}));
                    analysis.rule_members.push_back(std::make_shared<AST::RuleMember>(AST::RuleMember{
                        .value = std::move(options)}));
                    placeholders.emplace(std::move(key), std::move(analysis));
                }
            }
            if (member.isGroup()) for (auto& child : member.getGroup().values) self(self, *child);
            if (member.isOp()) for (auto& child : member.getOp().options) self(self, *child);
        };
        for (auto& member : rule.rule_members) rewrite(rewrite, *member);
    }
    for (auto& [name, rule] : placeholders) tree_map.emplace(std::move(name), std::move(rule));
}
