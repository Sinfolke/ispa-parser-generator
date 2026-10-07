# Typed AST Formatting Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Generate recursively formatted AST strings and stream output for every typed struct created from grammar data blocks.

**Architecture:** `LangAPI::Class` owns optional `to_str_func` and `output_func` function ASTs. `LangRepr::ConstructTypes` builds those ASTs from the typed class's field names and types. The C++ declaration converter routes `to_str_func` through its existing function conversion path and buffers `output_func` until the relevant types are complete, then emits free `operator<<` overloads that delegate to `to_string()`.

**Tech Stack:** C++23 modules, LangAPI AST nodes, LangRepr construction pipeline, dynamically loaded C++2 converter, CMake, clang++ generated-code tests.

**Spec:** `docs/superpowers/specs/2026-09-27-typed-ast-formatting-design.md`

## Global Constraints

- Formatting functions must be represented in the LangAPI AST rather than emitted as ad hoc converter text.
- Construct formatting functions in `LangRepr` while typed `FlatTypes` classes are assembled.
- Emit the member as `std::string to_string() const` and emit stream formatting as a free `std::ostream& operator<<(std::ostream&, const Struct&)`.
- Struct headings, `is_const`, `op`, source spans, recursive values, and ambiguity-safe field labels must match the approved output design.
- Preserve existing user changes in the working tree; no unrelated stdlib/runtime refactor is in scope.

## Review Focus

1. **Monostate/empty variant:** formatting must handle an unselected variant without calling a nonexistent alternative formatter; test empty variants in Task 4.
2. **Empty and singleton sequences:** connectors and indentation must remain valid when a node has zero or one children; test both in Task 4.
3. **Nested variants and typed nodes:** the active alternative must be formatted recursively and preserve its node coordinates; test a nested variant in Task 4.
4. **Repeated/ambiguous child types:** labels must remain present when eliding them would make sibling values indistinguishable; test same-typed siblings in Task 4.
5. **Absent or zero-length source spans:** positions must be omitted rather than rendered as invalid coordinates; test a node with unavailable positions in Task 4.

---

### Task 1: Extend LangAPI Class with Function AST Slots

**Files:**
- Modify: `src/LangAPI.cppm` — add `std::optional<Function> to_str_func` and `std::optional<Function> output_func` to `LangAPI::Class`; include both in equality, ordering, and `members()`.
- Modify: `src/LangAPI.cpp` — include both optional functions in `operator<<(std::ostream&, const Class&)` output.

**Interfaces:**
- Produces: `LangAPI::Class::to_str_func` and `LangAPI::Class::output_func`, each an optional `LangAPI::Function` AST value.

- [ ] **Step 1: Add the optional formatting function AST fields**

Add `std::optional<Function>` fields. Update `Class::operator==`, `Class::operator<`, and `Class::members()` so equality, ordering, and hashing observe both function AST values. Update the LangAPI class stream operator to print populated function slots. Leave class constructions that do not define formatting functions unchanged; the optional fields remain disengaged by default. Validate these changes through the end-to-end generated-code test in Task 4 rather than adding a standalone LangAPI test.

### Task 2: Construct Formatting Function ASTs in LangRepr

**Files:**
- Modify: `src/LangRepr/ConstructTypes.cpp` — build formatting functions for each typed class after its data fields and rewritten field types are known.
- Modify: `src/LangRepr/ConstructTypes.cppm` only if helper declarations are needed by the module interface.

**Interfaces:**
- Consumes: `LangAPI::Class::to_str_func` and `output_func` from Task 1.
- Produces: populated function ASTs with LangAPI statements/expressions; the AST-side `str` function is translated by C++2 to the generated `to_string() const` member, and the output function represents an `operator<<` overload.

- [ ] **Step 1: Build `to_str_func` and `output_func` as LangAPI function ASTs for each typed class**

While `ConstructTypes` has each class's name, data members, and final LangAPI types, construct the ASTs there. The `to_str_func` body builds the approved tree representation using field-aware expressions and statements: the type name is the heading; `is_const` and `op` use the approved compact annotations; nested structs/variants/sequences recurse; scalar leaves are rendered as values; and source spans are appended only when valid. Preserve field labels for ambiguous/repeated children. Build `output_func` as the stream-output function AST, with the stream expression delegating to the class string conversion. Populate both optional fields on every generated typed struct. Validate through the generated parser compilation and runtime assertions in Task 4; do not add unit tests for LangRepr or generator internals.

### Task 3: Route Function ASTs Through the C++2 Converter

**Files:**
- Modify: `src/LangRepr/Converter.cppm` — traverse `to_str_func` using the existing declaration/statement function dispatch while the class is open.
- Modify: `converters/C++2/CoreFunctions.cppm` — declare converter-scoped storage and helpers for deferred output functions.
- Modify: `converters/C++2/Declarations.cppm` — add private/helper declarations only where the implementation requires them.
- Modify: `converters/C++2/Declarations.cpp` — collect output function pointers from classes and emit them after all types in their namespace are complete; route signatures and bodies through existing function/statement conversion.
- Modify: `converters/C++2/Init.cpp` only if file-finalization needs an emission hook beyond `Declarations::closeFile`.

**Interfaces:**
- Consumes: `std::optional<LangAPI::Function>` slots populated by Task 2.
- Produces: C++ `to_string() const` member and free `operator<<` declaration/definition. Deferred output-function pointers are scoped to a single conversion and may not outlive the owning LangAPI AST.

- [ ] **Step 1: Route `to_str_func` through normal function emission and defer `output_func` safely**

In `LangRepr::Converter::buildDeclaration`, emit the optional `to_str_func` through the same `createFunction` → `buildStatements` → `closeFunction` path used for class functions, while the class scope is active. In the C++2 declaration converter, collect each class's `output_func` pointer without copying it, track the owning namespace, and flush the collected functions only after the namespace's typed classes are complete. Emit the free overload signature in the generated header and the body in the generated source using the existing function and statement converters. Clear the collector at conversion start/end so one generated file cannot leak overloads into the next.

- [ ] **Step 2: Verify the generated parser through the practical integration test**

Run: `cmake --build cmake-build-debug --target tests && ctest --test-dir cmake-build-debug --output-on-failure`
Expected: the formatting fixture is generated and compiled, and the end-to-end test confirms the member and stream overload are callable. This practical generated-code path replaces separate assertions or unit tests for converter internals.

### Task 4: Verify Exact Tree Rendering End to End

**Files:**
- Create: `tests/input/parser/formatting.isc` — controlled grammar fixture with nested types, variants, sequence fields, operator/const annotations, and token positions.
- Create: `tests/compile/formatting.cpp` — bootloader that parses the fixture input and compares generated `to_string()` and stream output to the expected text.
- Modify: `tests/Cpp.cpp` — add an integration test that generates the fixture parser, compiles the formatting bootloader, and runs it.
- Modify: `tests/lib/CppParser.cppm` and `tests/lib/CppParser.cpp` only if a separate generated binary name or captured output helper is required; prefer bootloader exit-status assertions to avoid subprocess-output capture.

**Interfaces:**
- Consumes: complete generated LangAPI and C++ converter behavior from Tasks 1–3.
- Produces: a reproducible end-to-end regression with exact output assertions.

- [ ] **Step 1: Add a bootloader assertion for the expected output tree**

Have the bootloader obtain the generated typed AST value, compare `to_string()` with the exact approved tree text, stream the same value into `std::ostringstream`, and compare that output to the same expected text.

```cpp
const std::string expected =
    "Program\n"
    "└── VariableDeclaration [const]\n"
    "    ├── Identifier: total\n"
    "    └── BinaryExpression (+)\n"
    "        ├── Identifier: price  [line 1:15-1:20]\n"
    "        └── Identifier: tax    [line 1:23-1:26]";
if (ast.to_string() != expected) return 1;
std::ostringstream streamed;
streamed << ast;
if (streamed.str() != expected) return 2;
```

The fixture must also construct/check an empty variant, empty and singleton sequences, a nested variant node with positions, repeated same-typed children whose labels must remain, and a node without valid source positions. These cases should either be included in the same expected output or asserted separately in the bootloader.

- [ ] **Step 2: Run the integration test and confirm the output assertion fails before formatter implementation**

Run: `cmake --build cmake-build-debug --target tests && ctest --test-dir cmake-build-debug --output-on-failure`
Expected: FAIL with a nonzero formatting bootloader status or generated-C++ compilation error.

- [ ] **Step 3: Complete formatting behavior in the LangAPI function AST construction**

Fix only the generated AST expressions/statements that fail the focused assertion. Ensure empty variants do not visit an inactive alternative, sequence connectors distinguish last/non-last children, absent positions are omitted, and field labels remain for ambiguous siblings. Keep `operator<<` delegating to `to_string()` rather than duplicating formatting logic.

- [ ] **Step 4: Run focused and full verification**

Run: `cmake --build cmake-build-debug --target tests && ctest --test-dir cmake-build-debug --output-on-failure`
Expected: PASS; the bootloader verifies exact output through both APIs and the complete current test suite passes.

- [ ] **Step 5: Review the final diff without committing**

Run: `rtk git -C /mnt/5EE9F38E0E9F2DC9/ispa-parser diff --check` and `rtk git -C /mnt/5EE9F38E0E9F2DC9/ispa-parser status --short`.
Expected: no whitespace errors; only formatter implementation files/tests and the approved spec/plan are newly changed, with pre-existing user edits preserved. Do not commit unless explicitly requested.
