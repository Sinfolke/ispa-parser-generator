export module UAST.PyAST;
import std;

// Typed Python syntax tree for the actions embedded in ISPA grammars.
//
// The shape follows CPython's `ast` module, restricted to what the ISPA Python
// grammar (parser/python) can currently produce. Children are held through
// shared_ptr so the recursive types stay complete; a null pointer means "absent"
// (e.g. `return` without a value). Positions are absolute positions in the host
// grammar file, like every other UAST node.
export namespace UAST {
struct SourcePosition {
    std::string file;
    std::size_t offset = std::string::npos, length = 0, line = 0, column = 0;

    auto members() { return std::tie(file, offset, length, line, column); }
    auto members() const { return std::tie(file, offset, length, line, column); }
    bool operator==(const SourcePosition& other) const { return members() == other.members(); }
    bool operator<(const SourcePosition& other) const { return members() < other.members(); }
};

namespace py {
enum class ExprContext { Load, Store, Del };
enum class BoolOp { And, Or };
enum class BinOp { Add, Sub, Mult, MatMult, Div, Mod, Pow, LShift, RShift, BitOr, BitXor, BitAnd, FloorDiv };
enum class UnaryOp { Invert, Not, UAdd, USub };
enum class CmpOp { Eq, NotEq, Lt, LtE, Gt, GtE, Is, IsNot, In, NotIn };
enum class ConstantKind { None, Bool, Int, Float, Str };

struct Expr;
struct Stmt;
using ExprPtr = std::shared_ptr<Expr>;
using StmtPtr = std::shared_ptr<Stmt>;
using ExprList = std::vector<ExprPtr>;
using StmtList = std::vector<StmtPtr>;
// ---- expressions
struct Name {
    std::string id;
    ExprContext ctx = ExprContext::Load;

    auto members() { return std::tie(id, ctx); }
    auto members() const { return std::tie(id, ctx); }
    bool operator==(const Name& other) const { return members() == other.members(); }
    bool operator<(const Name& other) const { return members() < other.members(); }
};
// `value` is the source text: "True"/"False"/"None" for those kinds, the digits
// for Int ("12"), "12.5" for Float, and the raw text between the quotes for Str
// (escape sequences are NOT decoded).
struct Constant {
    ConstantKind kind = ConstantKind::None;
    std::string value;

    auto members() { return std::tie(kind, value); }
    auto members() const { return std::tie(kind, value); }
    bool operator==(const Constant& other) const { return members() == other.members(); }
    bool operator<(const Constant& other) const { return members() < other.members(); }
};
struct Attribute {
    ExprPtr value;
    std::string attr;
    ExprContext ctx = ExprContext::Load;

    auto members() { return std::tie(value, attr, ctx); }
    auto members() const { return std::tie(value, attr, ctx); }
    bool operator==(const Attribute& other) const { return members() == other.members(); }
    bool operator<(const Attribute& other) const { return members() < other.members(); }
};
struct Subscript {
    ExprPtr value,
            slice;
    ExprContext ctx = ExprContext::Load;

    auto members() { return std::tie(value, slice, ctx); }
    auto members() const { return std::tie(value, slice, ctx); }
    bool operator==(const Subscript& other) const { return members() == other.members(); }
    bool operator<(const Subscript& other) const { return members() < other.members(); }
};
struct Keyword {
    std::string arg;
    ExprPtr value;
    SourcePosition pos;

    auto members() { return std::tie(arg, value, pos); }
    auto members() const { return std::tie(arg, value, pos); }
    bool operator==(const Keyword& other) const { return members() == other.members(); }
    bool operator<(const Keyword& other) const { return members() < other.members(); }
};
struct Call {
    ExprPtr func;
    ExprList args;
    std::vector<Keyword> keywords;

    auto members() { return std::tie(func, args, keywords); }
    auto members() const { return std::tie(func, args, keywords); }
    bool operator==(const Call& other) const { return members() == other.members(); }
    bool operator<(const Call& other) const { return members() < other.members(); }
};
struct UnaryExpr {
    UnaryOp op{};
    ExprPtr operand;

    auto members() { return std::tie(op, operand); }
    auto members() const { return std::tie(op, operand); }
    bool operator==(const UnaryExpr& other) const { return members() == other.members(); }
    bool operator<(const UnaryExpr& other) const { return members() < other.members(); }
};
struct BinaryExpr {
    ExprPtr left;
    BinOp op{};
    ExprPtr right;

    auto members() { return std::tie(left, op, right); }
    auto members() const { return std::tie(left, op, right); }
    bool operator==(const BinaryExpr& other) const { return members() == other.members(); }
    bool operator<(const BinaryExpr& other) const { return members() < other.members(); }
};
// `a or b or c` is one BoolExpr with three values, like CPython.
struct BoolExpr {
    BoolOp op{};
    ExprList values;

    auto members() { return std::tie(op, values); }
    auto members() const { return std::tie(op, values); }
    bool operator==(const BoolExpr& other) const { return members() == other.members(); }
    bool operator<(const BoolExpr& other) const { return members() < other.members(); }
};
// `a < b <= c`: ops.size() == comparators.size().
struct Compare {
    ExprPtr left;
    std::vector<CmpOp> ops;
    ExprList comparators;

    auto members() { return std::tie(left, ops, comparators); }
    auto members() const { return std::tie(left, ops, comparators); }
    bool operator==(const Compare& other) const { return members() == other.members(); }
    bool operator<(const Compare& other) const { return members() < other.members(); }
};
// `body if test else orelse`
struct IfExpr {
    ExprPtr test,
            body,
            orelse;

    auto members() { return std::tie(test, body, orelse); }
    auto members() const { return std::tie(test, body, orelse); }
    bool operator==(const IfExpr& other) const { return members() == other.members(); }
    bool operator<(const IfExpr& other) const { return members() < other.members(); }
};
// `lambda a, b: body`: parameters are unannotated names.
struct Lambda {
    std::vector<std::string> params;
    ExprPtr body;

    auto members() { return std::tie(params, body); }
    auto members() const { return std::tie(params, body); }
    bool operator==(const Lambda& other) const { return members() == other.members(); }
    bool operator<(const Lambda& other) const { return members() < other.members(); }
};
struct Tuple {
    ExprList elts;
    ExprContext ctx = ExprContext::Load;

    auto members() { return std::tie(elts, ctx); }
    auto members() const { return std::tie(elts, ctx); }
    bool operator==(const Tuple& other) const { return members() == other.members(); }
    bool operator<(const Tuple& other) const { return members() < other.members(); }
};
struct List {
    ExprList elts; 
    ExprContext ctx = ExprContext::Load;

    auto members() { return std::tie(elts, ctx); }
    auto members() const { return std::tie(elts, ctx); }
    bool operator==(const List& other) const { return members() == other.members(); }
    bool operator<(const List& other) const { return members() < other.members(); }
};
struct Dict {
    ExprList keys, 
             values;

    auto members() { return std::tie(keys, values); }
    auto members() const { return std::tie(keys, values); }
    bool operator==(const Dict& other) const { return members() == other.members(); }
    bool operator<(const Dict& other) const { return members() < other.members(); }
};

using ExprKind = std::variant<
    Name, Constant, Attribute, Subscript, Call, UnaryExpr, BinaryExpr,
    BoolExpr, Compare, IfExpr, Lambda, Tuple, List, Dict
>;
struct Expr {
    SourcePosition pos;
    ExprKind kind;
    template<class T> const T* get() const { return std::get_if<T>(&kind); }
    template<class T> T* get() { return std::get_if<T>(&kind); }

    auto members() { return std::tie(pos, kind); }
    auto members() const { return std::tie(pos, kind); }
    bool operator==(const Expr& other) const { return members() == other.members(); }
    bool operator<(const Expr& other) const { return members() < other.members(); }
};

// ---- statements and declarations
struct Param {
    std::string name;
    ExprPtr annotation,
            default_value;
    SourcePosition pos;

    auto members() { return std::tie(name, annotation, default_value, pos); }
    auto members() const { return std::tie(name, annotation, default_value, pos); }
    bool operator==(const Param& other) const { return members() == other.members(); }
    bool operator<(const Param& other) const { return members() < other.members(); }
};
struct FunctionDef {
    std::string name;
    std::vector<Param> params;
    ExprPtr returns;
    StmtList body;
    ExprList decorators;
    bool is_async = false;

    auto members() { return std::tie(name, params, returns, body, decorators, is_async); }
    auto members() const { return std::tie(name, params, returns, body, decorators, is_async); }
    bool operator==(const FunctionDef& other) const { return members() == other.members(); }
    bool operator<(const FunctionDef& other) const { return members() < other.members(); }
};
struct ClassDef {
    std::string name;
    ExprList bases;
    std::vector<Keyword> keywords;
    StmtList body;
    ExprList decorators;

    auto members() { return std::tie(name, bases, keywords, body, decorators); }
    auto members() const { return std::tie(name, bases, keywords, body, decorators); }
    bool operator==(const ClassDef& other) const { return members() == other.members(); }
    bool operator<(const ClassDef& other) const { return members() < other.members(); }
};
struct Return {
    ExprPtr value;

    auto members() { return std::tie(value); }
    auto members() const { return std::tie(value); }
    bool operator==(const Return& other) const { return members() == other.members(); }
    bool operator<(const Return& other) const { return members() < other.members(); }
};
struct Delete {
    ExprList targets;

    auto members() { return std::tie(targets); }
    auto members() const { return std::tie(targets); }
    bool operator==(const Delete& other) const { return members() == other.members(); }
    bool operator<(const Delete& other) const { return members() < other.members(); }
};
struct Assign {
    ExprList targets; 
    ExprPtr value;

    auto members() { return std::tie(targets, value); }
    auto members() const { return std::tie(targets, value); }
    bool operator==(const Assign& other) const { return members() == other.members(); }
    bool operator<(const Assign& other) const { return members() < other.members(); }
};
struct AugAssign {
    ExprPtr target; 
    BinOp op{}; 
    ExprPtr value;

    auto members() { return std::tie(target, op, value); }
    auto members() const { return std::tie(target, op, value); }
    bool operator==(const AugAssign& other) const { return members() == other.members(); }
    bool operator<(const AugAssign& other) const { return members() < other.members(); }
};
struct AnnAssign {
    ExprPtr target, 
            annotation, 
            value;

    auto members() { return std::tie(target, annotation, value); }
    auto members() const { return std::tie(target, annotation, value); }
    bool operator==(const AnnAssign& other) const { return members() == other.members(); }
    bool operator<(const AnnAssign& other) const { return members() < other.members(); }
};
struct For {
    ExprPtr target, 
            iter; 
    StmtList body, 
             orelse;
    bool is_async = false;

    auto members() { return std::tie(target, iter, body, orelse, is_async); }
    auto members() const { return std::tie(target, iter, body, orelse, is_async); }
    bool operator==(const For& other) const { return members() == other.members(); }
    bool operator<(const For& other) const { return members() < other.members(); }
};
struct While {
    ExprPtr test;
    StmtList body,
             orelse;

    auto members() { return std::tie(test, body, orelse); }
    auto members() const { return std::tie(test, body, orelse); }
    bool operator==(const While& other) const { return members() == other.members(); }
    bool operator<(const While& other) const { return members() < other.members(); }
};
// `elif` chains are nested: orelse holds a single If.
struct If {
    ExprPtr test;
    StmtList body,
             orelse;

    auto members() { return std::tie(test, body, orelse); }
    auto members() const { return std::tie(test, body, orelse); }
    bool operator==(const If& other) const { return members() == other.members(); }
    bool operator<(const If& other) const { return members() < other.members(); }
};
struct WithItem {
    ExprPtr context,
            vars;
    SourcePosition pos;

    auto members() { return std::tie(context, vars, pos); }
    auto members() const { return std::tie(context, vars, pos); }
    bool operator==(const WithItem& other) const { return members() == other.members(); }
    bool operator<(const WithItem& other) const { return members() < other.members(); }
};
struct With {
    std::vector<WithItem> items;
    StmtList body;
    bool is_async = false;

    auto members() { return std::tie(items, body, is_async); }
    auto members() const { return std::tie(items, body, is_async); }
    bool operator==(const With& other) const { return members() == other.members(); }
    bool operator<(const With& other) const { return members() < other.members(); }
};
struct Raise {
    ExprPtr exc,
            cause;

    auto members() { return std::tie(exc, cause); }
    auto members() const { return std::tie(exc, cause); }
    bool operator==(const Raise& other) const { return members() == other.members(); }
    bool operator<(const Raise& other) const { return members() < other.members(); }
};
struct ExceptHandler {
    ExprPtr type;
    std::string name;
    StmtList body;
             SourcePosition pos;

    auto members() { return std::tie(type, name, body, pos); }
    auto members() const { return std::tie(type, name, body, pos); }
    bool operator==(const ExceptHandler& other) const { return members() == other.members(); }
    bool operator<(const ExceptHandler& other) const { return members() < other.members(); }
};
struct Try {
    StmtList body;
    std::vector<ExceptHandler> handlers;
    StmtList orelse, finalbody;

    auto members() { return std::tie(body, handlers, orelse, finalbody); }
    auto members() const { return std::tie(body, handlers, orelse, finalbody); }
    bool operator==(const Try& other) const { return members() == other.members(); }
    bool operator<(const Try& other) const { return members() < other.members(); }
};
struct Assert {
    ExprPtr test,
            msg;

    auto members() { return std::tie(test, msg); }
    auto members() const { return std::tie(test, msg); }
    bool operator==(const Assert& other) const { return members() == other.members(); }
    bool operator<(const Assert& other) const { return members() < other.members(); }
};
// `import a.b as c` -> Alias{"a.b", "c"}; `from m import *` -> Alias{"*", ""}.
struct Alias {
    std::string name,
                asname;
    SourcePosition pos;

    auto members() { return std::tie(name, asname, pos); }
    auto members() const { return std::tie(name, asname, pos); }
    bool operator==(const Alias& other) const { return members() == other.members(); }
    bool operator<(const Alias& other) const { return members() < other.members(); }
};
struct Import {
    std::vector<Alias> names;

    auto members() { return std::tie(names); }
    auto members() const { return std::tie(names); }
    bool operator==(const Import& other) const { return members() == other.members(); }
    bool operator<(const Import& other) const { return members() < other.members(); }
};
// `from ..pkg.mod import x`: level == 2, module_name == "pkg.mod".
struct ImportFrom {
    std::string module_name;
    std::vector<Alias> names;
    std::size_t level = 0;

    auto members() { return std::tie(module_name, names, level); }
    auto members() const { return std::tie(module_name, names, level); }
    bool operator==(const ImportFrom& other) const { return members() == other.members(); }
    bool operator<(const ImportFrom& other) const { return members() < other.members(); }
};
struct Global {
    std::vector<std::string> names;

    auto members() { return std::tie(names); }
    auto members() const { return std::tie(names); }
    bool operator==(const Global& other) const { return members() == other.members(); }
    bool operator<(const Global& other) const { return members() < other.members(); }
};
struct Nonlocal {
    std::vector<std::string> names;

    auto members() { return std::tie(names); }
    auto members() const { return std::tie(names); }
    bool operator==(const Nonlocal& other) const { return members() == other.members(); }
    bool operator<(const Nonlocal& other) const { return members() < other.members(); }
};
struct ExprStmt {
    ExprPtr value;

    auto members() { return std::tie(value); }
    auto members() const { return std::tie(value); }
    bool operator==(const ExprStmt& other) const { return members() == other.members(); }
    bool operator<(const ExprStmt& other) const { return members() < other.members(); }
};
struct Pass {

    auto members() { return std::tie(); }
    auto members() const { return std::tie(); }
    bool operator==(const Pass& other) const { return members() == other.members(); }
    bool operator<(const Pass& other) const { return members() < other.members(); }
};
struct Break {

    auto members() { return std::tie(); }
    auto members() const { return std::tie(); }
    bool operator==(const Break& other) const { return members() == other.members(); }
    bool operator<(const Break& other) const { return members() < other.members(); }
};
struct Continue {

    auto members() { return std::tie(); }
    auto members() const { return std::tie(); }
    bool operator==(const Continue& other) const { return members() == other.members(); }
    bool operator<(const Continue& other) const { return members() < other.members(); }
};
struct IspaMatch {
    std::string source;

    auto members() { return std::tie(source); }
    auto members() const { return std::tie(source); }
    bool operator==(const IspaMatch& other) const { return members() == other.members(); }
    bool operator<(const IspaMatch& other) const { return members() < other.members(); }
};

using StmtKind = std::variant<
    FunctionDef, ClassDef, Return, Delete, Assign, AugAssign, AnnAssign,
    For, While, If, With, Raise, Try, Assert, Import, ImportFrom, Global,
    Nonlocal, ExprStmt, Pass, Break, Continue, IspaMatch
>;
struct Stmt {
    SourcePosition pos;
    StmtKind kind;
    template<class T> const T* get() const { return std::get_if<T>(&kind); }
    template<class T> T* get() { return std::get_if<T>(&kind); }

    auto members() { return std::tie(pos, kind); }
    auto members() const { return std::tie(pos, kind); }
    bool operator==(const Stmt& other) const { return members() == other.members(); }
    bool operator<(const Stmt& other) const { return members() < other.members(); }
};

// An action body: a sequence of statements.
struct Module {
    SourcePosition pos;
    StmtList body;

    auto members() { return std::tie(pos, body); }
    auto members() const { return std::tie(pos, body); }
    bool operator==(const Module& other) const { return members() == other.members(); }
    bool operator<(const Module& other) const { return members() < other.members(); }
};
// Declared signatures of already-loaded Python modules, keyed by qualified name. The converter uses the
// annotations to type expressions (call results, fields) that the source leaves implicit.
struct Signatures {
    struct Class {
        std::vector<std::string> bases;
        std::map<std::string, ExprPtr> fields, methods;
    };
    std::map<std::string, ExprPtr> functions;
    // Module variables: the annotation, or a synthesized one for literal initializers.
    std::map<std::string, ExprPtr> variables;
    std::map<std::string, Class> classes;
};
} // namespace py
}
