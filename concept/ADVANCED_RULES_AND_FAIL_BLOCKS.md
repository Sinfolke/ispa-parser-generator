# Advanced Rules and Fail Blocks

This guide records what advanced rules and fail blocks can do in the current
implementation, how the repository's grammars use them, and where they could
be extended. It is based on the ISPA grammar, the Python grammar, the LL parser
runtime, and the Python-to-LangAPI action lowering. It does not define new
syntax or promise support outside the stated backend and lexer conditions.

## Availability and execution model

Advanced parser rules and fail blocks execute with the LL C++ backend. Their
action bodies use ISPA's supported Python subset and are translated to C++;
they do not invoke a Python interpreter. LR-family parser actions are not
supported. Advanced token rules also execute with LL C++ generation, but have
token-specific restrictions described below. The ordinary rule and token
syntax remains the best fit when a construct can be expressed declaratively.

An advanced parser rule is a braced body containing `match` statements and
Python statements. A match accepts ordinary rule patterns and can bind their
results:

```isc
letter: 'a';
item {
    ctx.ast.count = 0
    match letter -> value;
    ctx.ast.value = value
    ctx.ast.count += 1
}
```

Bindings and local variables persist across sequential Python blocks in the
same invocation. The advanced rule's `ctx.ast` is its result and can be
captured by a caller like a regular rule result. Bare `return` and
`return ctx.ast` return that result early. `match` is the declarative bridge:
it invokes normal grammar matching while the action code handles decisions
that depend on matched values or parser state.

An ISPA `match` can also appear inside a Python `if` or `while` suite in an
advanced rule. It runs at that point in the suite, and its capture is available
to later statements in the same action:

```isc
item {
    if ctx.at('x'):
        match @ 'x' -> token;
        ctx.ast.value = token
}
```

The semicolon belongs to the ISPA match clause. The surrounding suite uses
Python indentation. Literal patterns in these clauses register lexer tokens
automatically. Python `for` is parsed by the Python grammar but is not yet
lowered by the action generator; use `while` for a loop that needs an embedded
match.

The action language supports the Python constructs lowered by ISPA, not every
Python feature. AST fields can be inferred from literals and supported local
expressions; annotate fields when inference is insufficient, for example
`ctx.ast.count: int = compute_count()`. Supported primitive field annotations
are `int`, `float`, `bool`, and `str`. Imported Python helper files and their
supported typed functions/classes can be used from actions; their code is
compiled to C++ as well.

## Parser action context

| Member | Meaning | Writable? |
|---|---|---|
| `ctx.pos` | Current token cursor; moving it changes where subsequent matching starts | Yes |
| `ctx.ast` | Result object for the current rule or the owning rule during recovery | Yes |
| `ctx.start_pos` | Cursor at entry to the owning rule | No |
| `ctx.rule` | Qualified name of the owning rule | No |
| `ctx.failed_rule` | Qualified name of the expected production in a fail block | No; fail blocks only |

Saving and restoring `ctx.pos` allows bounded lookahead or trying an alternate
interpretation after a match. Moving it freely can make a rule skip input or
loop; use a progress check when writing repetition in action code. `ctx.ast`
is invocation-local. Contexts and references into them must not escape the
action invocation.

`ctx.at(value, offset)` checks the token at `offset` positions ahead without
moving the cursor. The offset is optional and defaults to zero (the current
token); values above 64 throw. Bare token identifiers such as `ID` match the
token name, while quoted strings such as `"="` match source text. A bound
variable takes precedence over a token identifier and is used as a source
text value. It is available in advanced parser rules
and fail blocks, and checks return false at or beyond EOF.

`ctx.peek(offset)` returns a copy of the current token or a token up to 64
positions ahead without moving the cursor. `ctx.take()` returns the current
token and advances by one; at EOF both return an empty result. `ctx.prev()`
rewinds one token and returns it. At the start of the current rule it returns
an empty result and leaves the cursor unchanged. The returned
value has `text`, `name`, `kind`, `offset`, `length`, `line`, `column`, and `raw`
fields plus `empty()`. `text` and `raw` remain available after restoring a
checkpoint. These methods are available in advanced parser rules and fail
blocks. In comparisons, `token.name == ID` checks the token identity, while
`token.name == "word"` checks the token's source text. A bound variable named
`ID` is evaluated as a runtime value. Use a checkpoint to try a
production-local interpretation:

```isc
item {
    ctx.mark("start")
    token = ctx.take()
    if token.text == "name":
        ctx.commit("start")
        ctx.ast.name = token.text
    else:
        ctx.restore("start")
}
```

`ctx.collect_braced()` starts after a matched opening `{`, copies tokens up to
the matching `}`, and advances the original cursor past that closing token.
Nested braces remain in the returned vector. An unmatched brace raises an
error. With a lazy source cursor, strings and comments do not affect brace
depth. A rule can then parse that vector independently:

```isc
outer {
    match '{';
    tokens = ctx.collect_braced()
    match(tokens) inner -> parsed;
    ctx.ast.value = parsed
}
inner: 'x' | '}';
```

`match(tokens) inner` requires one unquantified parser rule. It runs the rule
against a temporary eager token stream and requires the rule to consume that
whole stream. The original `ctx.pos` is unaffected by this match. With `->`
the result is an empty node on failure, so an action can inspect `.empty()`;
without a capture, failure raises an error. The closing delimiter must be a
recognized token in the grammar for `collect_braced()` to see it.
When the target rule contains an external parser, the generated call uses the
original source span bounded by the collected tokens. That span must be a
contiguous slice of the original input.

Named checkpoints provide a bounded alternative to storing raw cursors:

```isc
probe {
    ctx.mark("start")
    match PREFIX -> prefix;
    if not ctx.at("OPEN", 2):
        ctx.restore("start")
    else:
        ctx.commit("start")
}
```

`mark`, `restore`, and `commit` require a nonempty string literal label.
`restore(label)` moves to the most recent matching checkpoint and keeps that
checkpoint available for another attempt, while discarding newer checkpoints.
`commit(label)` discards that checkpoint and any newer ones. Checkpoints are
local to one action context, may be nested, and are limited to 32 live entries;
unknown labels and limit overflow are errors. Restoring rewinds the cursor;
it does not undo AST writes or diagnostics, so defer those writes until a
branch is committed. Use distinct labels for nested choices so their lifetime
is clear.

The `depends` declaration is accepted by advanced-rule grammar syntax, but the
current AST/backend guard rejects advanced-rule dependencies. Treat external
semantic dependencies declared there as unavailable until lowering supports
them. Ordinary imported helper modules are the supported reusable-code path.

## Fail block binding and context

A fail block is declared by name and may take the same grammar parameter forms
as other named constructs:

```isc
fail missing_value {
    ctx.error("expected a value")
    ctx.insert()
}

value: %missing_value ID;
```

A required match can select a handler inline with `%handler` or use a
rule-level `%{handler}` binding. Fail blocks run only for required LL parser
matches selected by those bindings. Optional and zero-or-more matches do not
invoke them. The handler shares the owning rule's AST and failure cursor;
`ctx.start_pos` and `ctx.rule` refer to the owning rule, while
`ctx.failed_rule` identifies the expected production. A handler can therefore
both explain the failure and repair the parse state or partial AST.

Fail blocks support parameterized names and invocation arguments as supported
by the common grammar parameter syntax. Recovery is bounded by the runtime's
shared skip/retry limits. An explicit abort is terminal for that recovery
attempt.

### Diagnostics and recovery operations

`ctx.error("message")` adds a diagnostic to the parser's errors for the
current parse. `ctx.error("message", "CODE")` also assigns a stable
machine-readable code. The diagnostic records its message, expected rule,
source offset and length, recovery decision, and code. To point at a different
known byte range, pass both explicitly:

```isc
ctx.error("unexpected escape", "LEX001", escape_offset, 2)
```

Explicit spans use zero-based byte offsets in the original parser input and
are bounds checked. Without them, the runtime uses the failure cursor's token
span (or EOF position). `ctx.note("message")` adds a structured note to the
most recent diagnostic created by this fail block; it requires a preceding
`ctx.error`. Diagnostics can then select one of these recovery operations:

| Operation | Effect |
|---|---|
| `ctx.skip()` | Consume one token and continue the handler |
| `ctx.insert()` | Accept the required match as missing, leaving the cursor in place |
| `ctx.replace()` | Skip one token, then accept the required match as missing |
| `ctx.retry()` | Retry the required match at the current cursor |
| `ctx.abort()` | Stop recovery |

If no operation is requested, no cursor movement means insertion and cursor
movement means retry. Use explicit operations when intent is not obvious.
`ctx.replace()` is a compact one-token repair; use `skip()` or synchronization
when more input must be discarded. After `ctx.abort()`, later recovery calls
cannot reverse the abort.

`ctx.at(value)` checks the current token. Quoted strings and bound runtime
variables match source text; bare token identifiers match token names.
Qualified token names accept `.`; unqualified names match the final component,
and qualified names match the corresponding suffix. Bound variables take
precedence over token identifiers.

`ctx.sync("}", ";")` scans source text to a boundary while balancing nested
delimiters and respecting quoted strings and comments. It is for lazy source
cursors only, scans at most 4096 source bytes, and shares the recovery skip
budget. A listed semicolon is a hard stop, including inside an unclosed
container. The boundary is left for its owner. Follow with `insert()` to
accept a missing value at that boundary, or `retry()` to try matching there.

`ctx.sync_tokens(NEWLINE, DEDENT)` performs the analogous operation over
tokens. It works with eager and lazy streams and can preserve synthetic tokens
such as indentation markers. Bare identifiers match token names; quoted strings
and bound variables match source text. It does not balance delimiters, stops at EOF,
and leaves the matching boundary in place. Both synchronization methods require
one or more nonempty string literals. Exhausting scan or skip limits aborts
recovery.

## Advanced token rules

LL C++ token rules may combine a `match` pattern with Python action statements.
Actions run in source order after the complete token has been accepted and its
captures are available. An advanced token must contain a match and cannot be
embedded inside another token. Token actions cannot move `ctx.pos` or return
early; they cannot affect which token pattern wins.

Token contexts expose `ctx.ast`, read-only `ctx.start_pos` and `ctx.rule`, plus:

| Member | Purpose |
|---|---|
| `ctx.text` | Accepted token's source text |
| `ctx.line_prefix` | Source text before the token on its current line |
| `ctx.next_char` | Character after the token, or an empty string at EOF |
| `ctx.shared.<field>` | Mutable lexer-instance state shared across token actions |
| `ctx.emit('TOKEN')` | Queue a zero-width token after the current token |
| `ctx.skip()` | Suppress the current token |
| `ctx.error('message')` | Reject the input with a lexer error |

Shared fields need a type annotation or an inferable literal assignment, and
declarations across token rules must agree. Their state lasts for the lexer
instance. `emit` requires a token-name string literal; an unqualified name is
resolved in the current grammar module and a dotted name can select another
module. An advanced `END_INPUT` matching the empty string can run actions at
EOF before queued tokens and the EOF marker are emitted.

### Grammar example: indentation state

The Python lexer uses `ctx.shared.bracket_depth` in opening and closing
delimiter token actions. This records whether newlines occur inside brackets,
which can influence indentation and newline token behavior. More generally,
shared state plus `ctx.line_prefix`, `ctx.next_char`, `ctx.skip()` and
`ctx.emit()` can implement indentation stacks, newline suppression, and
zero-width INDENT/DEDENT tokens without a custom lexer subclass. Such behavior
must be scoped carefully: a mutable lexer field carries state across tokens,
so each token action must preserve stack/depth invariants.

## Patterns already used by repository grammars

The ISPA grammar itself provides concrete recovery examples:

* `ast_field_separator` reports the missing `:` and uses `replace()` for
  likely mistaken `=` or `=>` separators, otherwise `insert()`.
* `ast_field_expression` reports a malformed expression, scans to `}` or `;`,
  and inserts the missing expression without consuming the enclosing boundary.
* `dynamic_expression` uses source synchronization across `}`, `)` and `;` to
  preserve the remainder of the grammar after a malformed embedded expression.

The Python grammar uses `sync_tokens()` at commas, colons, statement
terminators, dedents, and closing delimiters when an expression is missing. It
uses insertion for common missing punctuation such as `)`, `]`, `}`, `:`, and
`else`. Its `statement_end` handler synchronizes to `NEWLINE` or `DEDENT` and
retries at a newline; `expected_block`, `header_colon`, and `header_rparen`
insert their missing construct. Its lexer also uses token actions and shared
bracket depth, indentation stacks, and EOF token emission. These are good
templates for grammars with nested expressions, delimiter-heavy syntax, or
layout-sensitive blocks. The dependency scanner grammar currently has no
advanced actions or fail blocks.

## Design opportunities

These opportunities follow from the current grammar and runtime surface. They
are use cases or possible extensions, not claims of already implemented
features.

### Parsing and AST construction

* **Context-sensitive interpretation:** match a syntactically ambiguous
  construct, inspect the capture or symbol table maintained by the caller,
  then build a type/name-specific AST. This is suitable for contextual names,
  declaration-vs-expression choices, and embedded DSL dispatch. The current
  `depends` declaration is not lowered, so use supported helpers or passed
  rule parameters for external information.
* **Stateful or staged parsing:** save `ctx.pos`, probe a construct, and restore
  the cursor when the probe does not fit; commit to the successful path by
  leaving the cursor advanced. A progress guard is needed for loops.
* **AST normalization while parsing:** create canonical nodes, flatten
  associative lists, attach synthetic/missing markers, or combine captures
  when those decisions depend on context. Keep plain structural repetition in
  regular rules where possible.
* **Semantic bookkeeping:** update the rule AST with scope, declaration,
  feature, or nesting information needed by later grammar actions. For
  invocation-local or cross-rule state, make its ownership explicit rather
  than relying on escaping `ctx` references.
* **Embedded syntax:** use a regular match to identify a host-language region
  and an external parser call for a separately generated grammar where the
  external parser constraints allow it. External parser calls currently need
  LL generation and lazy tokenization; they are not a general dynamic call
  facility from arbitrary Python expressions.

### Error reporting and recovery

* **Expected-token-specific messages:** branch on `ctx.failed_rule` and
  `ctx.at(...)` to report a domain term or a likely correction rather than a
  generic mismatch.
* **Local typo repair:** use `replace()` for a wrong single token, `insert()`
  for a missing delimiter, and `retry()` when the current token may begin the
  expected construct after another recovery step.
* **Nested construct recovery:** choose `sync()` for source-level boundaries
  that must respect strings/comments/nesting, or `sync_tokens()` when token
  categories and synthetic tokens are the reliable boundaries. Leave the
  synchronization token for its surrounding production.
* **Partial ASTs with explicit damage:** record a missing/invalid child in the
  owning AST while inserting, so tooling can continue to inspect later valid
  siblings. Consumers should have a clear representation for recovered nodes.
* **Recovery quality checks:** track diagnostics and parse progress in tests;
  ensure a recovery neither loops nor swallows the next statement/declaration.
  Runtime limits remain the final safety net, not a replacement for a
  well-chosen boundary.

### Further context API opportunities

The current context intentionally exposes a small surface. Further runtime and
lowering work could add capabilities such as read-only source-span values,
diagnostic severity and secondary note locations, recovery labels or scoped
boundary sets, and a standard invalid-node marker. Each addition needs
consistent behavior across token and parser contexts, generated targets, and
all parser backends. These are not implied by the APIs above.

## Current limitations to keep in view

* Parser advanced actions and fail blocks are LL-only in the current backend;
  LR-family support is absent.
* The action language is a translated Python subset, not arbitrary Python.
* Advanced token actions do not control matching and cannot move the parser
  cursor or return a token result early.
* Source `sync()` requires a lazy source cursor; use `sync_tokens()` when the
  parser has only token-stream access.
* Recovery is for required matches. Optional and zero-or-more matches do not
  dispatch fail blocks.
* Advanced-rule `depends` is parsed but rejected during lowering.
* Diagnostics are collected by the parser; fail blocks do not print or throw
  user-facing messages themselves.
