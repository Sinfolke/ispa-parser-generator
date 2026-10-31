# Auto-inline of pass-through rules

ISPA removes a rule that only forwards to another rule, and parses the forwarded rule directly.
This saves a function call, a recovery frame, a recovery site and a wrapper node per use of the rule.

## What is inlined

A rule is a *pass-through* when **all** of these hold:

```
name:
    @ other
    {@}
;
```

- its body is exactly one member: a plain reference to another **nonterminal** rule (not a token, no
  template arguments, no `*`, `+`, `?`, no `!`), captured with a bare `@`;
- its data block is exactly `{@}` (no `@{names}`, no typed or keyed data block, no Python expression);
- it has no actions, fail blocks, `%` fail bindings, variable captures or counters;
- it has no parameters and no template parameters, and so does the rule it forwards to;
- it is not named `main` (the entry rule), is not itself a template, and is not an external rule;
- it does not forward to itself, directly or through a cycle of pass-throughs;
- it does not carry `#[noinline]`.

Chains collapse: if `a` forwards to `b` and `b` forwards to `c`, both `a` and `b` become `c`.

Rules that look similar but are **not** inlined: a rule with alternatives (`@ a | b | c`), a rule that
names its capture (`@{value}` or `@value other`), a rule that adds a second member, a rule with an action,
and a rule that forwards to a token. Those wrap or choose something, so they stay.

## What changes for you

Nothing in the grammar's behaviour changes for valid input. Three things are visible:

1. **The type is an alias.** For `name: @ other {@}`, `Types::name` is now the same type as `Types::other`,
   in C++ (`using`), Python and Rust. Code that only names the type or passes it around keeps compiling.
   There is no `name` wrapper node any more, so code that read `node.value` of a `name` to reach the
   `other` now already holds the `other` value: drop that `.value` step.
2. **Diagnostics name the surviving rule.** A recovery message that used to say `in name` says `in other`,
   and expected-lists contain `other`'s first tokens. Debug scopes (`ispadbg`) show `other`.
3. **Exports keep working.** A rule removed this way is still exported under its own name; an importing
   module sees the same symbol and `Types::name` alias.

## Opting out

Put `#[noinline]` before the rule to keep it as a real rule: a real function, its own node, its own
recovery frame and its own name in diagnostics.

```
#[noinline]
name:
    @ other
    {@}
;
```

Use it when something outside the grammar depends on the wrapper: a debugger breakpoint on the rule, code that
reads the wrapper node, or recovery messages you want to keep.
`#[inline]` and `#[noinline]` on the same rule is error `GRM103`.

## Checking what happened

Edit-time effects are easy to see: dump the tree with `--dump treeAPI` (the removed rules are absent), or read
the generated `Types` namespace, where an inlined rule appears as a `using` alias of the rule it forwards to.
The grammar's `.ispa-export` file still lists every rule you wrote.
