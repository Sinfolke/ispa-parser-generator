# ISPA parser standard library

The grammar library is under `grammar/` and the Python helper modules used by
grammar actions are under `python/`. Modules are imported by their declared
module name, for example `import std.collections;` or
`import py {sep_seq_to_list} from ast;`.

Install these with `ispa-pkg install std`. The generated parser runtimes are
separate packages: `ispa-pkg install cpp-target` for C++ and
`ispa-pkg install python-target` for Python.

## Grammar modules

| Module | Contents |
| --- | --- |
| `std.names` | ASCII identifier token `ID` and dotted `qualified_name` rule |
| `std.separators` | `__WS` and `NEWLINE` lexer productions |
| `std.comments` | Parameterized line and block comment productions |
| `std.literals` | `NUMBER`, quoted `STRING`, and `ID` productions, with recovery handlers for malformed literals |
| `std.collections` | Parameterized separated-list and key/value-list productions; both accept an optional trailing separator |

Grammar productions are reusable templates. Import only the productions a
grammar needs to avoid token-name collisions. The literal forms are deliberately
small lexical building blocks; language-specific number, string, and identifier
rules can be defined by a consuming grammar when they differ.

## Python helper modules

`python/ast.py` provides `sep_seq_to_list` and `kv_to_dict`, matching the
captures produced by `std.collections`. `python/stdfun.py` provides common
integer and boolean helpers (`range`, `sum`, `min`, `max`, `sorted`, `reversed`,
`all`, and `any`), integer `abs`, and the two-list mapping helper `zip` used
by grammar actions. `range` implements the one-argument form; list ordering and
numeric helpers operate on integers, and boolean reductions operate on boolean
lists. `python/internals.py`
contains names recognized by the Python-to-C++ converter and is not a general
Python runtime module.

These modules target the existing restricted Python lowering for compiled
grammar actions. The list helpers use explicit element types where their
operations require them. `zip` retains its mapping behavior for compatibility
with AST construction helpers. They intentionally do not add modifier
handling, `ctx` extensions, or support for dynamic Python types.
