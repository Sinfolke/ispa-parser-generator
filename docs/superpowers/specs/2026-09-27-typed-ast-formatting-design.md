# Typed AST Formatting Design

## Goal

Generate readable, recursively formatted AST text for every typed struct generated from grammar data blocks. The formatting must be usable both as a `std::string` and through `operator<<`, and the functions that define it must be represented in the LangAPI AST rather than emitted as ad hoc converter text.

## Agreed output shape

The renderer produces a tree with Unicode branch connectors and indentation, for example:

```text
Program
└── VariableDeclaration [const]
    ├── Identifier: total
    └── BinaryExpression (+)
        ├── Identifier: price  [line 1:15-1:20]
        └── Identifier: tax    [line 1:23-1:26]
```

Structs render as headings. Conventional fields can contribute compact annotations: a true `is_const` field becomes `[const]`, an `op` field can appear in parentheses, and Node source coordinates appear as a span annotation when available. Child fields should retain labels where omitting them could make the output ambiguous; labels may be elided for unambiguous scalar values. Nested typed values are rendered recursively.

## Architecture

### LangAPI AST

Extend `LangAPI::Class` with `std::optional<Function> to_str_func` and `std::optional<Function> output_func`. Include these fields in the class's comparison, ordering, hashing/member tuple, and stream/debug representation so AST identity and diagnostics remain consistent. The class AST owns both function nodes by value. The converter may collect pointers to populated `output_func` values and emit them after class declarations, provided the source AST remains alive and unchanged until emission; otherwise it must retain owned function copies.

### LangRepr construction

Construct the formatting functions in `LangRepr` while typed `FlatTypes` classes are assembled in `ConstructTypes` (or an earlier LangRepr layer only if that layer already owns the class field/type information needed). Generate a `to_str_func` for each typed struct using LangAPI function/statement/expression nodes. Its generated C++ member is `std::string to_string() const`; the LangAPI-side name may remain `str` if that is the existing AST convention, with the C++ converter mapping it to `to_string`.

The function body describes the tree rendering using the class's actual data fields and their LangAPI types. It handles nested typed structs recursively, variant alternatives, arrays/sequences, scalar leaves, optional source spans, and the agreed compact annotations. It avoids suppressing labels based only on field position; omission is permitted only when the resulting text remains unambiguous.

### C++ converter

`Declarations::createClass` uses the existing function converter to emit the member represented by `to_str_func`. It does not independently reconstruct the field formatting rules.

`output_func` represents the stream output overload in the LangAPI AST, but is emitted as a free `std::ostream& operator<<(std::ostream&, const Struct&)`, not as a class member. The converter collects the function AST while processing classes, then emits the overload declaration/definition at a point where the corresponding type is declared/complete. A retained pointer must have a clear owner and lifetime; shared ownership is preferred over a raw pointer. The stream overload delegates to the string conversion so the two output paths cannot drift.

The converter should emit any required `<ostream>`/string support headers through its existing import mechanism. It should reuse existing function conversion for signatures and bodies wherever possible, adding only the special placement/renaming behavior required by the two class slots.

## Validation

Add tests at the LangAPI/LangRepr and generated-C++ levels as appropriate:

1. Verify generated typed classes carry both function ASTs, with equality/order/debug output accounting for them.
2. Generate a representative nested AST that exercises a true `is_const`, an `op`, source positions, a scalar leaf, a nested typed node, a sequence, and a variant.
3. Compile the generated C++ and exercise both `to_string()` and `operator<<`.
4. Assert the output tree's labels, annotation placement, source spans, indentation, and branch connectors, including an ambiguity case where labels must be retained.

Existing user changes in the working tree must be preserved; no unrelated stdlib/runtime refactor is part of this feature.
