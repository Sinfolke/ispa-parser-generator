# Converter Layer Notes

This file complements `docs/ARCHITECTURE.md` by focusing only on the converter/export pipeline.

## Runtime model

The runtime conversion flow is centered on `LangRepr::Converter`:

- it receives a fully built `Holder` from `LangRepr::Construct`
- it resolves the target language shared library (`libispa-converter-<lang>`)
- it instantiates backend declaration and statement writer objects
- it traverses the declaration tree and emits formatted output

This keeps the public IR independent of the final serialization format.

## Current C++ emitter

The concrete C++ implementation is under `converters/C++2/` and includes:

- `Init.cppm` / `Init.cpp`: backend lifecycle
- `CoreFunctions.cppm` / `CoreFunctions.cpp`: type conversion, function call conversion, template handling
- `Declarations.cppm` / `Declarations.cpp`: declaration formatting
- `Statement.cppm` / `Statement.cpp`: statement emission

The `LangRepr::Converter` uses the dynamic library API to create `create_cpp_declarations` and `create_cpp_statement` objects, then serializes the final output into a generated header/source pair.

## Why this matters

The converter layer is intentionally backend-isolated: `AST`, `Semantic`, `LLIR`, and `LangAPI` represent the grammar semantics, while `converters/` only decide how to render those semantics in concrete syntax. That prevents the parser engine itself from becoming coupled to one output language.
