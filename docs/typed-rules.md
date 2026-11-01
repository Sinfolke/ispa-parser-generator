# Typed rules

A rule normally gets an AST type that ISPA infers from its data block. A *typed rule* instead names a
Python class, and the rule's value is an instance of that class:

```
module main -> a;
import py {a_ast} from shapes;

ID: @ [a-z]+ {@};
NUM: @ [0-9]+ {@};

a | a_ast:
    @ NUM '>>' @ ID
    @{type, value}
;
```

```python
# shapes.py (do not name it ast.py: that shadows Python's own module)
class a_ast:
    type: Node
    value: Node
    def __init__(self, type: Node, value: Node) -> None:
        self.type = type
        self.value = value
```

The class is ordinary imported Python (see [native bindings](native-bindings.md)), so it is compiled into
the generated parser for every target. The rule value has the class type, and other rules capture it like
any other rule value.

## Syntax

`name | Class:` — the class follows the rule name (and its parameters, if any) after a `|`. It must be a
class made visible with `import py`. A dotted name (`module.Class`) works when the module was imported
whole. Only regular rules (`name: ... ;`) can be typed; advanced rules cannot.

## Data block

A typed rule always has a data block. `@{field, ...}` is the usual form; it names the `__init__` parameters that the
`@` captures fill, in order. The other forms are shorthands. They supply the arguments of `Class.__init__`:

| Data block | Call |
| --- | --- |
| `@{a, b}` | `Class(a=<1st @ capture>, b=<2nd @ capture>)` — names are `__init__` parameter names |
| `{@}` | `Class(<the @ capture>)` — needs exactly one `@` capture and a one-parameter `__init__` |
| `{expression}` | `Class(expression)` — a one-parameter `__init__` |
| `&={a: expression, b: expression}` | `Class(a=expression, b=expression)` |

Keywords may be written in any order. Generated code passes them positionally, in `__init__` order, so
every parameter must be given exactly once; unknown names, duplicates and missing parameters are errors
that name the rule and the class. Parameters with default values are not skipped.

`@` captures take the same values they would in an untyped rule: a token or rule reference gives that
rule's value, a quantified capture gives a list, and so on. The `__init__` annotations describe those
values.

## `Node`: the type of a capture

Annotate a parameter (and the field it fills) with `Node` to mean "whatever this capture is". Each typed rule
replaces `Node` with the real type of the capture it passes: `ISPA_STD::Node<NUM>` in C++, `Node<NUM>` in Rust, the
runtime `Node` in Python. The field takes the type of the `__init__` parameter of the same name, so declare
`type: Node` next to `__init__(self, type: Node, ...)`.

A captured rule or token value is the generated struct, not its text: use `node.value` for the struct's data and
`node.value.value` for a token's text (Python), so conversions such as `int(...)` belong in code that is Python-only.
A class used by two typed rules must receive the same capture types in both; otherwise generation fails.
Annotating with a concrete type (`int`, `str`, ...) is fine for fields the constructor computes.

## Targets and algorithms

C++, Rust and Python, with LL (`-a LL`) and LR (`-a LR`, `LALR`). The reduction of a typed rule binds each
`@` capture from the value stack and then runs the same constructor call that LL runs after the members. Captures
may be a rule or token, or a group, optional or repetition (`@ NUM+` gives `list[Node]`: annotate the parameter
`list[Node]`, `Node` inside it is replaced as a whole by the capture's type).

`tools/core-parity/rust_parity.py --grammar typed|lrtyped|typedlist|lrtypedlist` checks that the C++ and Rust parsers
print identical trees (valid and malformed input); LR and LL print the same tree for valid input (as for any
grammar, LL recovers from errors where LR reports them).

C++ detail: the class is emitted after the capture structs it holds and before the first struct that stores it, so
typed rules may chain (a class holding another typed rule's value). A class that holds a rule which stores that
same class (a recursive typed rule) is reported as an error. Generated ASTs print the class as its fields.
