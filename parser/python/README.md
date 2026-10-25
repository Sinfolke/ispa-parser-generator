# Python parser recovery

The C++ LL bootstrap is generated from `entry.isc` and `grammar/`. Fail blocks
in the expression and statement grammars report specific diagnostics and accept
missing syntax. This preserves subsequent statements after inputs such as:

```python
x =
y = 2
```

Recovery also handles missing operands after operators, attribute names after
`.`, closing `)`, `]`, and `}`, dictionary separators, conditional `else`,
compound-header `:`, required blocks, and statement terminators. Missing closing
delimiters at EOF reach the parser rather than throwing in the lexer.

Use `PyParser` with `PyLexer` from `parser.hpp` to parse Python, including its
indentation and implicit line continuation. `parser.parse(lexer)` uses an eager
token stream; `parser.parseLazy(source)` uses a lazy stream. Both return a partial
tree after successful recovery. Inspect `getErrors()` and `getContextErrors()`
before using that tree. A subsequent parse clears both diagnostic collections,
including after a failure. Source strings must outlive nodes that reference them.

Each local diagnostic includes its message, source offset and length, owning
rule, expected production, and recovery decision. Missing captures remain empty;
recovery does not manufacture valid Python values. The external grammar entry
points and UAST conversion helpers reject recovered Python, so malformed actions
cannot silently become generated code.

The handlers use `ctx.error`, `ctx.insert`, `ctx.retry`, and
`ctx.sync_tokens("NEWLINE", "DEDENT", ...)`. Token synchronization preserves
synthetic indentation and stops before the first listed token or EOF. It shares
the parser's skip budget, works in both iterator modes, and does not balance
nested delimiters. Source-level `ctx.sync(...)` is intended for foreign payloads
whose tokenization should be bypassed; Python recovery uses the lexer instead.

Recovery is bounded and can still throw. Unterminated strings and inconsistent
dedents are lexer errors. An unclosed delimiter can suppress logical newlines
until EOF, so recovery does not guarantee preservation of later physical lines
inside that delimiter. The indentation lexer retains Python's tab expansion,
blank/comment lines, and bracket continuation behavior. Counted `__WS=x` matches
equal whitespace counts, but replacing that lexer requires additional handling
for those Python rules.

Regenerate the committed bootstrap after building the generator:

```sh
python3 parser/python/generate.py cmake-build-release/ispa
```

`--output /tmp/python-bootstrap` generates a separate copy. The script flattens
module/import directives to preserve the existing public `PyRawParser::Types`
names, appends the custom entry declarations from `exports.hpp`, and copies
the `ispa_python` external bindings from `ispa-compat.ispa-export`. Those
bindings refer to the entry points implemented in `extern.cpp` and are needed
by `parser/ispa/grammar/python.isc`. A direct modular generator run exports
different module names and cannot replace this compatibility bootstrap.
Generator dumps stay in a temporary directory. Rebuild consumers after regeneration.

For the Python target, generate the modular bootstrap with:

```sh
python3 parser/python/generate_python.py cmake-build-release/ispa
python3 parser/python/test.py
```

The Python generator emits the runtime imports and translates the generated
lexer actions itself. `generate_python.py` runs the generator into a fresh
temporary directory before copying the result. The direct command is:

```sh
cmake-build-release/ispa -l python -a LL --no-incremental \
  --dir parser/python/grammar parser/python/entry.isc \
  -o parser/python/bootstrappedPy/Parser
```

The generated `bootstrappedPy/Parser.py` runs the grammar's generated lexer and
its advanced `INDENT`/`DEDENT` actions. The Python target translates the
remaining C++ style expressions in those actions. The `ispa_std` Python lexer
helper flushes emitted tokens, handles comments and strings, and closes
indentation at EOF. Generated fail blocks report diagnostics and recover from
missing syntax, including a missing compound-header colon.
