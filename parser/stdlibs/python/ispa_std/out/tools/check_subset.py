"""Check that library modules stay inside the Python subset the ISPA frontend lowers.

The allowed AST node set is the union of what the original ispa_std modules
(algorithm, map, optional, span, string, utility, vector) use, plus the
documented extensions below. Modules listed in EXEMPT are host-side helpers
(reflect, prim_cpython, cpython, prim) that are not lowered.

usage: python3 -I tools/check_subset.py [package_dir]"""
import ast
import os
import sys

BASE = {
    "Add", "And", "AnnAssign", "Assign", "Attribute", "BinOp", "BoolOp", "Call", "ClassDef", "Compare",
    "Constant", "Eq", "Expr", "FloorDiv", "FunctionDef", "Gt", "GtE", "If", "ImportFrom", "In", "Lambda",
    "List", "Load", "Lt", "LtE", "Module", "Mult", "Name", "Not", "NotEq", "NotIn", "Or", "Return", "Store",
    "Sub", "Subscript", "Tuple", "USub", "UnaryOp", "While", "alias", "arg", "arguments",
}
# Documented extensions (see docs/BACKEND.md, "Subset extensions").
EXTENSIONS = {
    "ClassBases": "classes that inherit from another library class (Lexer_base, LLParser, FailHandler ...)",
}
EXEMPT = {"reflect.py", "prim_cpython.py", "cpython.py", "prim.py", "__init__.py"}
SKIP_ANNOTATION_NAMES = True


def check_file(path):
    problems = []
    tree = ast.parse(open(path).read(), path)
    annotation_ids = set()
    for node in ast.walk(tree):
        for field in ("returns", "annotation"):
            ann = getattr(node, field, None)
            if ann is not None:
                for sub in ast.walk(ann):
                    annotation_ids.add(id(sub))
    for node in ast.walk(tree):
        name = type(node).__name__
        if id(node) in annotation_ids:
            continue
        if name in BASE:
            if name == "Constant" and node.value is None:
                problems.append((node.lineno, "None literal"))
            if name == "Constant" and not isinstance(node.value, (str, int, bool)):
                if not (node.value is Ellipsis):
                    problems.append((node.lineno, "constant of type " + type(node.value).__name__))
            if name == "Call":
                if any(k for k in node.keywords):
                    problems.append((node.lineno, "keyword argument"))
            continue
        if name in ("Load", "Store"):
            continue
        if name == "Slice":
            continue
        if name in ("Is", "IsNot"):
            continue
        problems.append((getattr(node, "lineno", 0), "disallowed node " + name))
    for node in ast.walk(tree):
        if isinstance(node, ast.FunctionDef):
            if node.args.defaults or node.args.kw_defaults:
                problems.append((node.lineno, "default argument in " + node.name))
            if node.args.vararg or node.args.kwarg or node.args.kwonlyargs:
                problems.append((node.lineno, "varargs/kwargs in " + node.name))
            for sub in ast.walk(node):
                if sub is not node and isinstance(sub, ast.FunctionDef):
                    problems.append((sub.lineno, "nested function " + sub.name))
        if isinstance(node, ast.Subscript) and isinstance(node.slice, ast.Slice):
            problems.append((node.lineno, "slice"))
        if isinstance(node, ast.Compare):
            for op in node.ops:
                if type(op).__name__ in ("Is", "IsNot"):
                    problems.append((node.lineno, "is comparison"))
        if isinstance(node, ast.Call) and isinstance(node.func, ast.Name) and node.func.id in (
                "isinstance", "hasattr", "getattr", "callable", "str", "int", "ord", "chr", "dict", "set",
                "range", "enumerate", "zip", "min", "max", "sorted", "print"):
            problems.append((node.lineno, "builtin call " + node.func.id))
    return sorted(set(problems))


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), "..", "ispa_std")
    bad = 0
    for name in sorted(os.listdir(root)):
        if not name.endswith(".py") or name in EXEMPT:
            continue
        for line, why in check_file(os.path.join(root, name)):
            print("%s:%d: %s" % (name, line, why))
            bad += 1
    print("subset check:", "FAILED (%d)" % bad if bad else "ok")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
