module;
#include <python/parser.hpp>
module UAST.Python;
import AST.Frontend;
import std;
namespace {
struct ActionParser : PyParser {
    using PyParser::main;
};
}
// Generic, metadata-driven snapshot implementation.
namespace {
struct Snapshot {
    std::string_view source;
    using P = UAST::SourcePosition;
    using R = UAST::PythonNode;
    template<class T> static consteval std::string_view type_name() {
#if defined(_MSC_VER)
        constexpr std::string_view signature = __FUNCSIG__;
        auto start = signature.find("type_name<");
        if (start == std::string_view::npos) return "unknown";
        start += 10;
        auto finish = signature.find(">(void)", start);
        if (finish == std::string_view::npos) finish = signature.find(">(void) noexcept", start);
        if (finish == std::string_view::npos) return "unknown";
        auto name = signature.substr(start, finish - start);
#else
        constexpr std::string_view signature = __PRETTY_FUNCTION__;
        auto start = signature.find("T = ");
        if (start == std::string_view::npos) return "unknown";
        start += 4;
        auto finish = signature.find_first_of(";]", start);
        auto name = signature.substr(start, finish - start);
#endif
        auto ns = name.rfind("::");
        return ns == std::string_view::npos ? name : name.substr(ns + 2);
    }
    R operator()(const P& x, const P&) const {
        return {"SourcePosition", {}, {}, x, false, {
            {"string", "file", x.file, x, false, {}},
            {"integer", "offset", std::to_string(x.offset), x, false, {}},
            {"integer", "length", std::to_string(x.length), x, false, {}},
            {"integer", "line", std::to_string(x.line), x, false, {}},
            {"integer", "column", std::to_string(x.column), x, false, {}}
        }};
    }
    R operator()(std::monostate, const P& p) const { return {"absent", {}, {}, p, true, {}}; }
    R operator()(const std::string& x, const P& p) const { return {"string", {}, x, p, false, {}}; }
    R operator()(char x, const P& p) const { return {"char", {}, std::string(1,x), p, false, {}}; }
    R operator()(bool x, const P& p) const { return {"bool", {}, x ? "true" : "false", p, false, {}}; }
    R operator()(const UAST::PythonNode& x, const P&) const { return x; }
    R operator()(const AST::CaptureBinding& x, const P& p) const {
        return {"CaptureBinding", {}, {}, p, false, {
            {"string", "name", x.name, p, false, {}},
            {"bool", "optional", x.optional ? "true" : "false", p, false, {}},
            {"bool", "list", x.list ? "true" : "false", p, false, {}}
        }};
    }
    template<class T> R operator()(const std::shared_ptr<T>& x, const P& p) const {
        return x ? (*this)(*x, p) : R{"pointer", {}, {}, p, true, {}};
    }
    template<class T> R operator()(const ISPA_STD::Node<T>& x, const P& base) const {
        if (x.empty()) return {"node", {}, {}, base, true, {}};
        auto p = base;
        // Read positions against the owned input, never by subtracting an
        // unchecked parser offset from a node pointer (synthetic tokens may
        // carry sentinel metadata).
        auto offset = x.startpos();
        const auto address = reinterpret_cast<std::uintptr_t>(x.start());
        const auto begin = reinterpret_cast<std::uintptr_t>(source.data());
        if (address >= begin && address - begin <= source.size()) offset = address - begin;
        if (offset <= source.size()) {
            if (p.offset != std::string::npos) p.offset += offset;
            for (char c : source.substr(0, offset)) {
                if (c == '\n') { ++p.line; p.column = 1; }
                else ++p.column;
            }
        }
        p.length = x.length();
        auto result = (*this)(x.data(), base);
        result.position = p;
        return result;
    }
    template<class... T> R operator()(const std::variant<T...>& x, const P& p) const {
        return {"variant", {}, std::to_string(x.index()), p, false,
            {std::visit([&](const auto& element) {return (*this)(element,p);}, x)}};
    }
    template<class T, class Alloc> R operator()(const std::vector<T,Alloc>& x, const P& p) const {
        R result{"sequence", {}, {}, p, false, {}};
        result.children.reserve(x.size());
        for (const auto& element : x) result.children.push_back((*this)(element,p));
        return result;
    }
    template<class A, class B> R operator()(const std::pair<A, B>& x, const P& p) const {
        return {"sequence", {}, {}, p, false, {(*this)(x.first, p), (*this)(x.second, p)}};
    }
    template<class... T> R operator()(const std::tuple<T...>& x, const P& p) const {
        R result{"sequence", {}, {}, p, false, {}};
        std::apply([&](const auto&... values) { (result.children.push_back((*this)(values, p)), ...); }, x);
        return result;
    }
    template<class T> requires requires(const T& x) { x.members(); T::members_str(); }
    R operator()(const T& x, const P& p) const {
        R result{std::string(type_name<T>()), {}, {}, p, false, {}};
        constexpr auto names = T::members_str();
        const auto tuple = x.members();
        static_assert(names.size() == std::tuple_size_v<decltype(tuple)>);
        result.children.reserve(names.size());
        std::apply([&](const auto&... values) {
            std::size_t index = 0;
            (([&] {
                auto child = (*this)(values, p);
                child.field = names[index++];
                result.children.push_back(std::move(child));
            }()), ...);
        }, tuple);
        return result;
    }
};
}


// ---------------------------------------------------------------------------
// Typed AST construction.
//
// The raw parser output is first viewed through Snapshot (members()/members_str()
// reflection, so no per-rule glue and no macros). PyBuilder then reads that tree
// *by rule name and field name*, exactly as the .isc grammar spells them:
//   `@{first, rest}`  -> fields "first", "rest"
//   `{@}`             -> field "value"
// Choice rules (`@ a | b | c {@}`) are looked through, and repetitions are
// gathered with find_all(), which is deliberately tolerant about how a group
// capture such as `(COMMA @ expr)*` is shaped by the generator.
// ---------------------------------------------------------------------------
// BEGIN-BUILDER
namespace {
namespace pyast = UAST::py;

class PyBuilder {
    using Node = UAST::PythonNode;
    using Pos = UAST::SourcePosition;
    using Body = std::vector<pyast::StmtPtr>;

public:
    pyast::ExprPtr expression(const Node& root) {
        auto n = unwrap(&root);
        if (!n || n->kind != "expr") fail(&root, "expected a Python expression, got '" + root.kind + "'");
        return expr(n);
    }

    pyast::Module build_module(const Node& root) {
        if (root.kind != "main" && !root.absent) fail(&root, "expected a Python action body, got '" + root.kind + "'");
        pyast::Module result;
        result.pos = root.position;
        for (auto s : find_all(&root, "statement")) statement(s, result.body);
        return result;
    }

private:
    // ---- generic tree access -------------------------------------------------
    [[noreturn]] static void fail(const Node* n, const std::string& message) {
        if (!n) throw std::runtime_error(message);
        throw std::runtime_error(n->position.file + ":" + std::to_string(n->position.line) + ":" +
                                 std::to_string(n->position.column) + ": " + message);
    }

    static const Node* field(const Node* n, std::string_view name) {
        if (!n) return nullptr;
        for (const auto& c : n->children)
            if (c.field == name) return &c;
        return nullptr;
    }

    // True for a missing node, an empty Node<T>, or a variant that holds monostate.
    static bool absent(const Node* n) {
        if (!n || n->absent) return true;
        if (n->kind == "variant") return n->children.empty() || absent(&n->children.front());
        return false;
    }

    // The payload of a `{@}` rule.
    static const Node* value_of(const Node* n) {
        if (auto v = field(n, "value")) return v;
        if (n && n->children.size() == 1) return &n->children.front();
        return nullptr;
    }

    // Rules of the form `@ a | b | c {@}`: nothing but a variant in field "value".
    static bool is_choice(std::string_view k) {
        static constexpr std::string_view names[] = {
            "statement", "compound_stmt", "simple_stmt", "import_stmt", "def_header", "block",
            "assignment_value", "assignment_suffix", "assignment_op", "try_tail", "import_targets", "inversion",
            "factor", "postfix", "atom", "compare_op", "call_arg"};
        return std::ranges::find(names, k) != std::end(names);
    }

    // Looks through variants and choice rules down to the concrete alternative.
    static const Node* unwrap(const Node* n) {
        while (!absent(n)) {
            if (n->kind == "variant") n = n->children.front().absent ? nullptr : &n->children.front();
            else if (is_choice(n->kind)) n = value_of(n);
            else return n;
        }
        return nullptr;
    }

    static void collect(const Node* n, std::string_view kind, std::vector<const Node*>& out) {
        if (absent(n)) return;
        if (n->kind == kind) { out.push_back(n); return; }
        for (const auto& c : n->children) collect(&c, kind, out);
    }
    // All outermost nodes of `kind` below (or at) n, in source order.
    static std::vector<const Node*> find_all(const Node* n, std::string_view kind) {
        std::vector<const Node*> out;
        collect(n, kind, out);
        return out;
    }
    static const Node* find_first(const Node* n, std::string_view kind) {
        auto all = find_all(n, kind);
        return all.empty() ? nullptr : all.front();
    }
    static bool contains_kind(const Node* n, std::string_view kind) { return find_first(n, kind) != nullptr; }

    static std::vector<const Node*> elements(const Node* n) {
        std::vector<const Node*> out;
        if (absent(n)) return out;
        if (n->kind == "sequence") {
            for (const auto& c : n->children)
                if (!absent(&c)) out.push_back(&c);
        } else out.push_back(n);
        return out;
    }

    static std::string scalar_field(const Node* n, std::string_view name) {
        auto f = field(n, name);
        return f && !f->absent ? f->scalar : std::string{};
    }
    // Text of an ID token node (or of a Node<ID> field).
    static std::string text_of(const Node* n) {
        auto id = find_first(n, "ID");
        if (!id) fail(n, "expected an identifier");
        auto v = value_of(id);
        return v ? v->scalar : std::string{};
    }
    static std::string dotted(const Node* n) { // dotted_name {first, rest}
        std::string out = text_of(field(n, "first"));
        for (auto id : find_all(field(n, "rest"), "ID")) out += "." + value_of(id)->scalar;
        return out;
    }

    static Pos join(const Pos& a, const Pos& b) {
        Pos r = a;
        if (a.offset != std::string::npos && b.offset != std::string::npos && b.offset + b.length >= a.offset)
            r.length = b.offset + b.length - a.offset;
        return r;
    }

    template<class K> static pyast::ExprPtr mk(const Pos& pos, K&& k) {
        return std::make_shared<pyast::Expr>(pyast::Expr{pos, pyast::ExprKind{std::forward<K>(k)}});
    }
    template<class K> static pyast::StmtPtr mks(const Pos& pos, K&& k) {
        return std::make_shared<pyast::Stmt>(pyast::Stmt{pos, pyast::StmtKind{std::forward<K>(k)}});
    }

    // ---- operators -----------------------------------------------------------
    static pyast::BinOp binop(const Node* raw) {
        using enum pyast::BinOp;
        static const std::unordered_map<std::string_view, pyast::BinOp> table{
            {"PLUS", Add}, {"MINUS", Sub}, {"STAR", Mult}, {"MATMUL", MatMult}, {"SLASH", Div},
            {"PERCENT", Mod}, {"FLOOR_DIV", FloorDiv}, {"LSHIFT", LShift}, {"RSHIFT", RShift}};
        auto t = unwrap(raw);
        if (!t) fail(raw, "missing operator");
        auto it = table.find(t->kind);
        if (it == table.end()) fail(t, "unsupported binary operator '" + t->kind + "'");
        return it->second;
    }

    // `x op= y` -> op; nullopt for plain `=`.
    static std::optional<pyast::BinOp> augop(const Node* t) {
        using enum pyast::BinOp;
        static const std::unordered_map<std::string_view, pyast::BinOp> table{
            {"PLUS_ASSIGN", Add}, {"MINUS_ASSIGN", Sub}, {"STAR_ASSIGN", Mult}, {"MATMUL_ASSIGN", MatMult},
            {"SLASH_ASSIGN", Div}, {"FLOOR_DIV_ASSIGN", FloorDiv}, {"PERCENT_ASSIGN", Mod},
            {"POWER_ASSIGN", Pow}, {"AMP_ASSIGN", BitAnd}, {"PIPE_ASSIGN", BitOr},
            {"CARET_ASSIGN", BitXor}, {"LSHIFT_ASSIGN", LShift}, {"RSHIFT_ASSIGN", RShift}};
        if (t->kind == "ASSIGN") return std::nullopt;
        auto it = table.find(t->kind);
        if (it == table.end()) fail(t, "unsupported assignment operator '" + t->kind + "'");
        return it->second;
    }

    static pyast::CmpOp cmpop(const Node* raw) {
        using enum pyast::CmpOp;
        auto t = unwrap(raw);
        if (!t) fail(raw, "missing comparison operator");
        const auto& k = t->kind;
        if (k == "EQ") return Eq;
        if (k == "NE") return NotEq;
        if (k == "LT") return Lt;
        if (k == "LE") return LtE;
        if (k == "GT") return Gt;
        if (k == "GE") return GtE;
        if (k == "IN") return In;
        if (k == "not_in") return NotIn;
        // `is_op: IS NOT?` — "is not" is recognised by a captured NOT token.
        if (k == "is_op") return contains_kind(t, "NOT") ? IsNot : Is;
        fail(t, "unsupported comparison operator '" + k + "'");
    }

    static void set_ctx(pyast::Expr& e, pyast::ExprContext ctx) {
        std::visit([&](auto& k) {
            using T = std::remove_cvref_t<decltype(k)>;
            if constexpr (std::is_same_v<T, pyast::Name> || std::is_same_v<T, pyast::Attribute> ||
                          std::is_same_v<T, pyast::Subscript>) {
                k.ctx = ctx;
            } else if constexpr (std::is_same_v<T, pyast::Tuple> || std::is_same_v<T, pyast::List>) {
                k.ctx = ctx;
                for (auto& x : k.elts) set_ctx(*x, ctx);
            }
        }, e.kind);
    }

    // ---- expressions ---------------------------------------------------------
    pyast::ExprPtr expr(const Node* raw) {
        auto n = unwrap(raw);
        if (!n) fail(raw, "missing Python expression");
        const auto& k = n->kind;
        using enum pyast::BinOp;

        if (k == "expr") return conditional(n);
        if (k == "disjunction") return bool_chain(n, "or_tail", pyast::BoolOp::Or);
        if (k == "conjunction") return bool_chain(n, "inversion", pyast::BoolOp::And);
        if (k == "not_inversion")
            return mk(n->position, pyast::UnaryExpr{pyast::UnaryOp::Not, expr(value_of(n))});
        if (k == "comparison") return comparison(n);
        if (k == "bitwise_or") return fold(n, "bitor_tail", BitOr);
        if (k == "bitwise_xor") return fold(n, "bitxor_tail", BitXor);
        if (k == "bitwise_and") return fold(n, "bitand_tail", BitAnd);
        if (k == "shift_expr") return fold(n, "shift_tail", std::nullopt);
        if (k == "sum") return fold(n, "sum_tail", std::nullopt);
        if (k == "term") return fold(n, "term_tail", std::nullopt);
        if (k == "unary_expr") {
            auto op = unwrap(field(n, "op"));
            if (!op) fail(n, "missing unary operator");
            pyast::UnaryOp u;
            if (op->kind == "PLUS") u = pyast::UnaryOp::UAdd;
            else if (op->kind == "MINUS") u = pyast::UnaryOp::USub;
            else if (op->kind == "TILDE") u = pyast::UnaryOp::Invert;
            else fail(op, "unsupported unary operator '" + op->kind + "'");
            return mk(n->position, pyast::UnaryExpr{u, expr(field(n, "value"))});
        }
        if (k == "power") {
            auto base = expr(field(n, "base"));
            auto exponent = field(n, "exponent");
            if (absent(exponent)) return base;
            return mk(n->position, pyast::BinaryExpr{base, Pow, expr(value_of(exponent))});
        }
        if (k == "primary") {
            auto cur = expr(field(n, "base"));
            for (auto s : elements(field(n, "suffixes"))) cur = apply_suffix(cur, s);
            return cur;
        }
        return atom(n);
    }

    pyast::ExprPtr conditional(const Node* n) {
        auto body = expr(field(n, "value"));
        auto tail = field(n, "conditional");
        if (absent(tail)) return body;
        return mk(join(body->pos, tail->position),
                  pyast::IfExpr{expr(field(tail, "condition")), body, expr(field(tail, "otherwise"))});
    }

    pyast::ExprPtr bool_chain(const Node* n, std::string_view item_kind, pyast::BoolOp op) {
        std::vector<pyast::ExprPtr> values{expr(field(n, "first"))};
        for (auto item : find_all(field(n, "rest"), item_kind))
            values.push_back(expr(item_kind == "or_tail" ? value_of(item) : item));
        if (values.size() == 1) return std::move(values.front());
        return mk(n->position, pyast::BoolExpr{op, std::move(values)});
    }

    // Left-associative chain: first (tail)*. `fixed` is the operator of tails
    // that only carry the right operand (`{@}` tails); otherwise tails are {op, value}.
    pyast::ExprPtr fold(const Node* n, std::string_view tail_kind, std::optional<pyast::BinOp> fixed) {
        auto result = expr(field(n, "first"));
        for (auto t : find_all(field(n, "rest"), tail_kind)) {
            auto op = fixed ? *fixed : binop(field(t, "op"));
            auto rhs = expr(fixed ? value_of(t) : field(t, "value"));
            result = mk(join(result->pos, t->position), pyast::BinaryExpr{result, op, rhs});
        }
        return result;
    }

    pyast::ExprPtr comparison(const Node* n) {
        auto first = expr(field(n, "first"));
        auto tails = find_all(field(n, "rest"), "compare_tail");
        if (tails.empty()) return first;
        pyast::Compare c;
        c.left = first;
        for (auto t : tails) {
            c.ops.push_back(cmpop(field(t, "op")));
            c.comparators.push_back(expr(field(t, "value")));
        }
        return mk(join(first->pos, tails.back()->position), std::move(c));
    }

    pyast::ExprPtr apply_suffix(pyast::ExprPtr cur, const Node* raw) {
        auto s = unwrap(raw);
        if (!s) return cur;
        auto pos = join(cur->pos, s->position);
        if (s->kind == "attribute_suffix")
            return mk(pos, pyast::Attribute{cur, text_of(value_of(s))});
        if (s->kind == "index_suffix")
            return mk(pos, pyast::Subscript{cur, expr(value_of(s))});
        if (s->kind == "call_suffix") {
            pyast::Call call;
            call.func = cur;
            call_args(value_of(s), call.args, call.keywords);
            return mk(pos, std::move(call));
        }
        fail(s, "unsupported postfix '" + s->kind + "'");
    }

    void call_args(const Node* n, std::vector<pyast::ExprPtr>& args, std::vector<pyast::Keyword>& keywords) {
        for (auto a : find_all(n, "call_arg")) {
            auto u = unwrap(a);
            if (!u) continue;
            if (u->kind == "keyword_arg")
                keywords.push_back({text_of(field(u, "name")), expr(field(u, "value")), u->position});
            else
                args.push_back(expr(u));
        }
    }

    pyast::ExprPtr atom(const Node* n) {
        const auto& k = n->kind;
        using pyast::ConstantKind;
        if (k == "ID") return mk(n->position, pyast::Name{text_of(n)});
        if (k == "NUMBER") {
            auto whole = scalar_field(n, "full"), frac = scalar_field(n, "floating");
            if (frac.empty()) return mk(n->position, pyast::Constant{ConstantKind::Int, whole});
            return mk(n->position, pyast::Constant{ConstantKind::Float, whole + "." + frac});
        }
        if (k == "STRING") return mk(n->position, pyast::Constant{ConstantKind::Str, scalar_field(n, "value")});
        if (k == "TRUE_BOOL") return mk(n->position, pyast::Constant{ConstantKind::Bool, "True"});
        if (k == "FALSE_BOOL") return mk(n->position, pyast::Constant{ConstantKind::Bool, "False"});
        if (k == "NONE_V") return mk(n->position, pyast::Constant{ConstantKind::None, "None"});
        if (k == "parenthesized_expr" || k == "parenthesized_content") {
            auto content = k == "parenthesized_content" ? n : value_of(n);
            if (absent(content)) return mk(n->position, pyast::Tuple{});
            auto first = expr(field(content, "first"));
            auto suffix = field(content, "rest");
            if (absent(suffix)) return first;            // (e) is just grouping
            pyast::Tuple t;                              // (e,) / (e, f, ...)
            t.elts.push_back(first);
            for (auto e : find_all(value_of(suffix), "expr")) t.elts.push_back(expr(e));
            return mk(n->position, std::move(t));
        }
        if (k == "lambda_expr") {
            pyast::Lambda l;
            auto params = field(n, "params");
            if (!absent(params))
                for (auto id : find_all(params, "ID")) l.params.push_back(value_of(id)->scalar);
            l.body = expr(field(n, "body"));
            return mk(n->position, std::move(l));
        }
        if (k == "list_expr") {
            pyast::List l;
            for (auto e : find_all(value_of(n), "expr")) l.elts.push_back(expr(e));
            return mk(n->position, std::move(l));
        }
        if (k == "dict_expr") {
            pyast::Dict d;
            for (auto item : find_all(value_of(n), "dict_item")) {
                d.keys.push_back(expr(field(item, "key")));
                d.values.push_back(expr(field(item, "value")));
            }
            return mk(n->position, std::move(d));
        }
        fail(n, "unsupported Python expression node '" + k + "'");
    }

    // `ID suffix*` used by assignment/del/for/with targets.
    pyast::ExprPtr target(const Node* n, pyast::ExprContext ctx) {
        auto cur = mk(n->position, pyast::Name{text_of(field(n, "name"))});
        for (auto s : elements(field(n, "suffixes"))) cur = apply_suffix(cur, s);
        set_ctx(*cur, ctx);
        return cur;
    }

    // ---- statements ----------------------------------------------------------
    void statement(const Node* raw, Body& out) {
        auto n = unwrap(raw);
        if (!n) return;
        if (n->kind == "simple_stmts") simple_stmts(n, out);
        else out.push_back(compound(n));
    }

    void simple_stmts(const Node* n, Body& out) {
        for (auto s : find_all(field(n, "first"), "simple_stmt")) out.push_back(simple(s));
        for (auto s : find_all(field(n, "rest"), "simple_stmt")) out.push_back(simple(s));
    }

    Body block(const Node* raw) {
        Body out;
        auto n = unwrap(raw);
        if (!n) fail(raw, "missing block");
        if (n->kind == "indented_block") {
            for (auto s : find_all(n, "statement")) statement(s, out);
        } else if (n->kind == "simple_stmts") {
            simple_stmts(n, out);
        } else {
            fail(n, "unexpected block node '" + n->kind + "'");
        }
        return out;
    }

    Body else_body(const Node* clause) { // else_clause / finally_clause {value: block}
        return absent(clause) ? Body{} : block(value_of(clause));
    }

    pyast::StmtPtr simple(const Node* raw) {
        auto n = unwrap(raw);
        if (!n) fail(raw, "missing statement");
        const auto& k = n->kind;
        const auto& pos = n->position;
        if (k == "ispa_match_stmt") {
            const auto token = find_first(n, "ISPA_MATCH");
            if (!token || !value_of(token)) fail(n, "missing ISPA match source");
            return mks(pos, pyast::IspaMatch{value_of(token)->scalar});
        }
        if (k == "expr_stmt") return expr_stmt(n);
        if (k == "return_stmt") {
            auto v = value_of(n);
            return mks(pos, pyast::Return{absent(v) ? nullptr : expr(v)});
        }
        if (k == "raise_stmt") {
            pyast::Raise r;
            if (auto e = field(n, "exception"); !absent(e)) r.exc = expr(e);
            if (auto c = field(n, "cause"); !absent(c)) r.cause = expr(value_of(c));
            return mks(pos, std::move(r));
        }
        if (k == "del_stmt") {
            pyast::Delete d;
            for (auto f : {"first", "rest"})
                for (auto t : find_all(field(n, f), "assignment_target"))
                    d.targets.push_back(target(t, pyast::ExprContext::Del));
            return mks(pos, std::move(d));
        }
        if (k == "pass_stmt") return mks(pos, pyast::Pass{});
        if (k == "break_stmt") return mks(pos, pyast::Break{});
        if (k == "continue_stmt") return mks(pos, pyast::Continue{});
        if (k == "assert_stmt") {
            pyast::Assert a;
            a.test = expr(field(n, "condition"));
            if (auto m = find_first(field(n, "message"), "expr")) a.msg = expr(m);
            return mks(pos, std::move(a));
        }
        if (k == "global_stmt" || k == "nonlocal_stmt") {
            std::vector<std::string> names{text_of(field(n, "first"))};
            for (auto id : find_all(field(n, "rest"), "ID")) names.push_back(value_of(id)->scalar);
            if (k == "global_stmt") return mks(pos, pyast::Global{std::move(names)});
            return mks(pos, pyast::Nonlocal{std::move(names)});
        }
        if (k == "import_name") return import_name(n);
        if (k == "import_from") return import_from(n);
        fail(n, "unsupported Python statement '" + k + "'");
    }

    pyast::StmtPtr expr_stmt(const Node* n) {
        auto tgt = expr(field(n, "target"));
        auto a = unwrap(field(n, "assignment"));
        if (!a) return mks(n->position, pyast::ExprStmt{tgt});
        set_ctx(*tgt, pyast::ExprContext::Store);
        if (a->kind == "assignment_tail") {
            auto op = unwrap(field(a, "op"));
            if (!op) fail(a, "missing assignment operator");
            auto value = expr(field(a, "value"));
            if (auto aug = augop(op)) return mks(n->position, pyast::AugAssign{tgt, *aug, value});
            return mks(n->position, pyast::Assign{{tgt}, value});
        }
        if (a->kind == "annotated_assignment") {
            pyast::AnnAssign x;
            x.target = tgt;
            x.annotation = expr(field(a, "annotation"));
            if (auto v = find_first(field(a, "value"), "expr")) x.value = expr(v);
            return mks(n->position, std::move(x));
        }
        fail(a, "unsupported assignment form '" + a->kind + "'");
    }

    pyast::StmtPtr import_name(const Node* n) {
        pyast::Import imp;
        for (auto a : find_all(value_of(n), "dotted_as_name")) {
            auto alias = field(a, "alias");
            imp.names.push_back({dotted(field(a, "name")), absent(alias) ? std::string{} : text_of(alias), a->position});
        }
        return mks(n->position, std::move(imp));
    }

    pyast::StmtPtr import_from(const Node* n) {
        pyast::ImportFrom imp;
        auto rel = field(n, "mod");
        if (!rel) rel = field(n, "module");
        imp.level = find_all(field(rel, "relative"), "DOT").size();
        if (auto name = field(rel, "name"); !absent(name)) imp.module_name = dotted(name);
        auto targets = unwrap(field(n, "names"));
        if (targets && targets->kind == "parenthesized_imports") targets = value_of(targets);
        if (!targets) fail(n, "missing import targets");
        if (targets->kind == "STAR") {
            imp.names.push_back({"*", {}, targets->position});
        } else {
            for (auto item : find_all(targets, "import_name_item")) {
                auto alias = field(item, "alias");
                imp.names.push_back({text_of(field(item, "name")), absent(alias) ? std::string{} : text_of(alias), item->position});
            }
        }
        return mks(n->position, std::move(imp));
    }

    pyast::StmtPtr compound(const Node* n) {
        const auto& k = n->kind;
        const auto& pos = n->position;
        if (k == "if_stmt") return if_stmt(n);
        if (k == "while_stmt")
            return mks(pos, pyast::While{expr(field(n, "condition")), block(field(n, "body")),
                                         else_body(field(n, "otherwise"))});
        if (k == "for_stmt") {
            auto targets = find_all(field(n, "targets"), "assignment_target");
            if (targets.empty()) fail(n, "missing for-loop target");
            pyast::ExprPtr tgt;
            if (targets.size() == 1) {
                tgt = target(targets.front(), pyast::ExprContext::Store);
            } else {
                pyast::Tuple t;
                for (auto x : targets) t.elts.push_back(target(x, pyast::ExprContext::Store));
                t.ctx = pyast::ExprContext::Store;
                tgt = mk(join(t.elts.front()->pos, t.elts.back()->pos), std::move(t));
            }
            return mks(pos, pyast::For{tgt, expr(field(n, "iterable")), block(field(n, "body")),
                                       else_body(field(n, "otherwise")), !absent(field(n, "async"))});
        }
        if (k == "with_stmt") {
            pyast::With w;
            w.is_async = !absent(field(n, "async"));
            for (auto item : find_all(field(n, "items"), "with_item")) {
                pyast::WithItem wi;
                wi.pos = item->position;
                wi.context = expr(field(item, "context"));
                if (auto as = field(item, "target"); !absent(as))
                    wi.vars = target(find_first(value_of(as), "assignment_target"), pyast::ExprContext::Store);
                w.items.push_back(std::move(wi));
            }
            w.body = block(field(n, "body"));
            return mks(pos, std::move(w));
        }
        if (k == "try_stmt") return try_stmt(n);
        if (k == "decorated_def" || k == "function_def" || k == "class_def") return definition(n);
        if (k == "function_header" || k == "class_header") return definition(n);
        fail(n, "unsupported Python statement '" + k + "'");
    }

    pyast::StmtPtr if_stmt(const Node* n) {
        Body orelse = else_body(field(n, "otherwise"));
        auto elifs = find_all(field(n, "elifs"), "elif_clause");
        for (auto it = elifs.rbegin(); it != elifs.rend(); ++it) {
            auto c = *it;
            Body wrapper;
            wrapper.push_back(mks(c->position, pyast::If{expr(field(c, "condition")),
                                                          block(field(c, "body")), std::move(orelse)}));
            orelse = std::move(wrapper);
        }
        return mks(n->position, pyast::If{expr(field(n, "condition")), block(field(n, "body")), std::move(orelse)});
    }

    pyast::StmtPtr try_stmt(const Node* n) {
        pyast::Try t;
        t.body = block(field(n, "body"));
        auto tail = unwrap(field(n, "handlers"));
        if (!tail) fail(n, "try statement without except or finally");
        if (tail->kind == "finally_clause") {
            t.finalbody = block(value_of(tail));
        } else if (tail->kind == "except_handlers") {
            for (auto h : find_all(field(tail, "handlers"), "except_clause")) {
                pyast::ExceptHandler eh;
                eh.pos = h->position;
                if (auto e = field(h, "exception"); !absent(e)) eh.type = expr(e);
                if (auto a = field(h, "alias"); !absent(a)) eh.name = text_of(value_of(a));
                eh.body = block(field(h, "body"));
                t.handlers.push_back(std::move(eh));
            }
            t.orelse = else_body(field(tail, "otherwise"));
            t.finalbody = else_body(field(tail, "finalizer"));
        } else {
            fail(tail, "unexpected try tail '" + tail->kind + "'");
        }
        return mks(n->position, std::move(t));
    }

    // decorated_def {decorators, definition} or a bare function_header / class_header.
    pyast::StmtPtr definition(const Node* n) {
        std::vector<pyast::ExprPtr> decorators;
        const Node* def = n;
        if (n->kind != "function_header" && n->kind != "class_header") {
            for (auto d : find_all(field(n, "decorators"), "decorator")) decorators.push_back(expr(value_of(d)));
            def = unwrap(field(n, "definition"));
            if (!def) fail(n, "decorators without a definition");
        }
        if (def->kind == "function_header") {
            pyast::FunctionDef f;
            f.name = text_of(field(def, "name"));
            f.is_async = !absent(field(def, "async"));
            f.decorators = std::move(decorators);
            for (auto p : find_all(field(def, "params"), "parameter")) {
                pyast::Param prm;
                prm.name = text_of(field(p, "name"));
                prm.pos = p->position;
                if (auto a = field(p, "annotation"); !absent(a)) prm.annotation = expr(value_of(a));
                if (auto d = field(p, "def"); !absent(d)) prm.default_value = expr(value_of(d));
                f.params.push_back(std::move(prm));
            }
            if (auto r = field(def, "returns"); !absent(r)) f.returns = expr(value_of(r));
            f.body = block(field(def, "body"));
            return mks(n->position, std::move(f));
        }
        if (def->kind == "class_header") {
            pyast::ClassDef c;
            c.name = text_of(field(def, "name"));
            c.decorators = std::move(decorators);
            if (auto b = field(def, "bases"); !absent(b)) call_args(value_of(b), c.bases, c.keywords);
            c.body = block(field(def, "body"));
            return mks(n->position, std::move(c));
        }
        fail(def, "unsupported declaration '" + def->kind + "'");
    }
};
}
// END-BUILDER

// BEGIN-DUMP
namespace {
class PyDumper {
public:
    std::string out;

    void expr(const pyast::ExprPtr& e) { if (e) expr(*e); else out += "None"; }
    void expr(const pyast::Expr& e) { std::visit([&](const auto& k) { node(k); }, e.kind); }
    void stmt(const pyast::StmtPtr& s) { if (s) stmt(*s); else out += "None"; }
    void stmt(const pyast::Stmt& s) { std::visit([&](const auto& k) { node(k); }, s.kind); }
    void module_(const pyast::Module& m) { out += "(Module "; body(m.body); out += ')'; }

private:
    static std::string_view name(pyast::ExprContext c) {
        switch (c) { case pyast::ExprContext::Load: return "Load"; case pyast::ExprContext::Store: return "Store"; default: return "Del"; }
    }
    static std::string_view name(pyast::BoolOp o) { return o == pyast::BoolOp::And ? "And" : "Or"; }
    static std::string_view name(pyast::UnaryOp o) {
        switch (o) { case pyast::UnaryOp::Invert: return "Invert"; case pyast::UnaryOp::Not: return "Not";
                     case pyast::UnaryOp::UAdd: return "UAdd"; default: return "USub"; }
    }
    static std::string_view name(pyast::BinOp o) {
        using enum pyast::BinOp;
        switch (o) {
            case Add: return "Add"; case Sub: return "Sub"; case Mult: return "Mult"; case MatMult: return "MatMult";
            case Div: return "Div"; case Mod: return "Mod"; case Pow: return "Pow"; case LShift: return "LShift";
            case RShift: return "RShift"; case BitOr: return "BitOr"; case BitXor: return "BitXor";
            case BitAnd: return "BitAnd"; default: return "FloorDiv";
        }
    }
    static std::string_view name(pyast::CmpOp o) {
        using enum pyast::CmpOp;
        switch (o) {
            case Eq: return "Eq"; case NotEq: return "NotEq"; case Lt: return "Lt"; case LtE: return "LtE";
            case Gt: return "Gt"; case GtE: return "GtE"; case Is: return "Is"; case IsNot: return "IsNot";
            case In: return "In"; default: return "NotIn";
        }
    }

    void list(const std::vector<pyast::ExprPtr>& v) {
        out += '[';
        for (std::size_t i = 0; i < v.size(); ++i) { if (i) out += ' '; expr(v[i]); }
        out += ']';
    }
    void body(const std::vector<pyast::StmtPtr>& v) {
        out += '[';
        for (std::size_t i = 0; i < v.size(); ++i) { if (i) out += ' '; stmt(v[i]); }
        out += ']';
    }
    void keywords(const std::vector<pyast::Keyword>& v) {
        out += '[';
        for (std::size_t i = 0; i < v.size(); ++i) { if (i) out += ' '; out += v[i].arg + "="; expr(v[i].value); }
        out += ']';
    }
    void aliases(const std::vector<pyast::Alias>& v) {
        out += '[';
        for (std::size_t i = 0; i < v.size(); ++i) {
            if (i) out += ' ';
            out += v[i].name;
            if (!v[i].asname.empty()) out += " as " + v[i].asname;
        }
        out += ']';
    }
    void names(const std::vector<std::string>& v) {
        out += '[';
        for (std::size_t i = 0; i < v.size(); ++i) { if (i) out += ' '; out += v[i]; }
        out += ']';
    }
    void sp() { out += ' '; }

    // expressions
    void node(const pyast::Name& x) { out += "(Name " + x.id + ' '; out += name(x.ctx); out += ')'; }
    void node(const pyast::Constant& x) {
        using enum pyast::ConstantKind;
        switch (x.kind) {
            case None: out += "(Constant None)"; break;
            case Bool: out += "(Constant " + x.value + ")"; break;
            case Int: out += "(Constant int " + x.value + ")"; break;
            case Float: out += "(Constant float " + x.value + ")"; break;
            case Str: out += "(Constant str '" + x.value + "')"; break;
        }
    }
    void node(const pyast::Attribute& x) { out += "(Attribute "; expr(x.value); out += ' ' + x.attr + ' '; out += name(x.ctx); out += ')'; }
    void node(const pyast::Subscript& x) { out += "(Subscript "; expr(x.value); sp(); expr(x.slice); sp(); out += name(x.ctx); out += ')'; }
    void node(const pyast::Call& x) { out += "(Call "; expr(x.func); sp(); list(x.args); sp(); keywords(x.keywords); out += ')'; }
    void node(const pyast::UnaryExpr& x) { out += "(Unary "; out += name(x.op); sp(); expr(x.operand); out += ')'; }
    void node(const pyast::BinaryExpr& x) { out += "(Binary "; out += name(x.op); sp(); expr(x.left); sp(); expr(x.right); out += ')'; }
    void node(const pyast::BoolExpr& x) { out += "(Bool "; out += name(x.op); sp(); list(x.values); out += ')'; }
    void node(const pyast::Compare& x) {
        out += "(Compare "; expr(x.left); out += " [";
        for (std::size_t i = 0; i < x.ops.size(); ++i) { if (i) out += ' '; out += name(x.ops[i]); }
        out += "] "; list(x.comparators); out += ')';
    }
    void node(const pyast::Lambda& x) {
        out += "(Lambda";
        for (const auto& name : x.params) out += " " + name;
        sp(); expr(x.body); out += ')';
    }
    void node(const pyast::IfExpr& x) { out += "(IfExpr "; expr(x.test); sp(); expr(x.body); sp(); expr(x.orelse); out += ')'; }
    void node(const pyast::Tuple& x) { out += "(Tuple "; list(x.elts); sp(); out += name(x.ctx); out += ')'; }
    void node(const pyast::List& x) { out += "(List "; list(x.elts); sp(); out += name(x.ctx); out += ')'; }
    void node(const pyast::Dict& x) { out += "(Dict "; list(x.keys); sp(); list(x.values); out += ')'; }

    // statements
    void node(const pyast::FunctionDef& x) {
        out += x.is_async ? "(AsyncFunctionDef " : "(FunctionDef "; out += x.name + " [";
        for (std::size_t i = 0; i < x.params.size(); ++i) {
            if (i) out += ' ';
            out += "(" + x.params[i].name + ' '; expr(x.params[i].annotation); sp(); expr(x.params[i].default_value); out += ')';
        }
        out += "] "; expr(x.returns); sp(); body(x.body); sp(); list(x.decorators); out += ')';
    }
    void node(const pyast::ClassDef& x) {
        out += "(ClassDef " + x.name + ' '; list(x.bases); sp(); keywords(x.keywords); sp(); body(x.body); sp(); list(x.decorators); out += ')';
    }
    void node(const pyast::Return& x) { out += "(Return "; expr(x.value); out += ')'; }
    void node(const pyast::Delete& x) { out += "(Delete "; list(x.targets); out += ')'; }
    void node(const pyast::Assign& x) { out += "(Assign "; list(x.targets); sp(); expr(x.value); out += ')'; }
    void node(const pyast::AugAssign& x) { out += "(AugAssign "; expr(x.target); sp(); out += name(x.op); sp(); expr(x.value); out += ')'; }
    void node(const pyast::AnnAssign& x) { out += "(AnnAssign "; expr(x.target); sp(); expr(x.annotation); sp(); expr(x.value); out += ')'; }
    void node(const pyast::For& x) {
        out += x.is_async ? "(AsyncFor " : "(For "; expr(x.target); sp(); expr(x.iter); sp(); body(x.body); sp(); body(x.orelse); out += ')';
    }
    void node(const pyast::While& x) { out += "(While "; expr(x.test); sp(); body(x.body); sp(); body(x.orelse); out += ')'; }
    void node(const pyast::If& x) { out += "(If "; expr(x.test); sp(); body(x.body); sp(); body(x.orelse); out += ')'; }
    void node(const pyast::With& x) {
        out += x.is_async ? "(AsyncWith [" : "(With [";
        for (std::size_t i = 0; i < x.items.size(); ++i) { if (i) out += ' '; out += '('; expr(x.items[i].context); sp(); expr(x.items[i].vars); out += ')'; }
        out += "] "; body(x.body); out += ')';
    }
    void node(const pyast::Raise& x) { out += "(Raise "; expr(x.exc); sp(); expr(x.cause); out += ')'; }
    void node(const pyast::Try& x) {
        out += "(Try "; body(x.body); out += " [";
        for (std::size_t i = 0; i < x.handlers.size(); ++i) {
            if (i) out += ' ';
            out += "(except "; expr(x.handlers[i].type); out += ' ' + (x.handlers[i].name.empty() ? std::string("_") : x.handlers[i].name) + ' ';
            body(x.handlers[i].body); out += ')';
        }
        out += "] "; body(x.orelse); sp(); body(x.finalbody); out += ')';
    }
    void node(const pyast::Assert& x) { out += "(Assert "; expr(x.test); sp(); expr(x.msg); out += ')'; }
    void node(const pyast::Import& x) { out += "(Import "; aliases(x.names); out += ')'; }
    void node(const pyast::ImportFrom& x) {
        out += "(ImportFrom " + std::string(x.level, '.') + x.module_name + ' '; aliases(x.names); out += ')';
    }
    void node(const pyast::Global& x) { out += "(Global "; names(x.names); out += ')'; }
    void node(const pyast::Nonlocal& x) { out += "(Nonlocal "; names(x.names); out += ')'; }
    void node(const pyast::ExprStmt& x) { out += "(Expr "; expr(x.value); out += ')'; }
    void node(const pyast::Pass&) { out += "(Pass)"; }
    void node(const pyast::Break&) { out += "(Break)"; }
    void node(const pyast::Continue&) { out += "(Continue)"; }
    void node(const pyast::IspaMatch& x) { out += "(IspaMatch source='" + x.source + "')"; }
};
}
std::string UAST::py::dump(const Expr& e) { PyDumper d; d.expr(e); return std::move(d.out); }
std::string UAST::py::dump(const Stmt& s) { PyDumper d; d.stmt(s); return std::move(d.out); }
std::string UAST::py::dump(const Module& m) { PyDumper d; d.module_(m); return std::move(d.out); }
// END-DUMP

namespace {
struct SnapshotFailure {
    std::string message;
    std::size_t offset;
};

UAST::PythonNode parse_snapshot(const std::string& source, const UAST::SourcePosition& position, bool expression) {
    auto failureOffset = [&](const auto& parser) {
        if (!parser.getErrors().empty() && parser.getErrors().front().offset <= source.size())
            return parser.getErrors().front().offset;
        if (!parser.getContextErrors().empty() && parser.getContextErrors().front().offset <= source.size())
            return parser.getContextErrors().front().offset;
        return source.size();
    };
    try {
        PyLexer lexer;
        ActionParser parser;
        PyRawParser::Lexer::lazy_iterator begin(lexer, source.c_str());
        if (expression) {
            auto result = parser.parseExpression(begin);
            if (!result.status || !parser.getErrors().empty() || !parser.getContextErrors().empty())
                throw SnapshotFailure{parser.getErrors().empty() ? "invalid Python expression" : parser.getErrors().front().message,
                                      failureOffset(parser)};
            auto end = result.it;
            if ((*end).template holds<PyRawParser::Types::NEWLINE>()) ++end;
            if (!end.isEnd()) throw SnapshotFailure{"trailing tokens in Python expression", failureOffset(parser)};
            return Snapshot{source}(result.node, position);
        }
        auto result = parser.main(begin);
        if (!result.status || !result.it.isEnd() || !parser.getErrors().empty() || !parser.getContextErrors().empty())
            throw SnapshotFailure{parser.getErrors().empty() ? "invalid Python action or trailing tokens" : parser.getErrors().front().message,
                                  failureOffset(parser)};
        return Snapshot{source}(result.node, position);
    } catch (const SnapshotFailure& error) {
        const auto offset = std::min(error.offset, source.size());
        std::size_t line = position.line ? position.line : 1;
        std::size_t column = position.column ? position.column : 1;
        for (std::size_t i = 0; i < offset; ++i) {
            if (source[i] == '\n') { ++line; column = 1; }
            else ++column;
        }
        throw std::runtime_error(position.file + ":" + std::to_string(line) + ":" +
                                 std::to_string(column) + ": " + error.message);
    } catch (const std::exception& error) {
        throw std::runtime_error(position.file + ":" + std::to_string(position.line) + ":" + std::to_string(position.column) + ": " + error.what());
    }
}
}

UAST::PythonNode UAST::parsePythonSnapshot(const std::string& source, const SourcePosition& position, bool expression) {
    return parse_snapshot(source, position, expression);
}
UAST::py::ExprPtr UAST::parsePythonExpr(const std::string& source, const SourcePosition& position) {
    return PyBuilder{}.expression(parse_snapshot(source, position, true));
}
UAST::py::Module UAST::parsePythonModule(const std::string& source, const SourcePosition& position) {
    return PyBuilder{}.build_module(parse_snapshot(source, position, false));
}
UAST::py::ExprPtr UAST::convertPythonExpr(const Node<ExternalPythonExpression>& syntax, const SourcePosition& position) {
    return PyBuilder{}.expression(Snapshot{std::string_view(syntax.start(), syntax.length())}(syntax, position));
}
UAST::py::Module UAST::convertPythonModule(const Node<ExternalPythonStatements>& syntax, const SourcePosition& position) {
    return PyBuilder{}.build_module(Snapshot{std::string_view(syntax.start(), syntax.length())}(syntax, position));
}
