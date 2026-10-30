# Flex and Bison migration

Install the release tools with `ispa-pkg tool flex-to-ispa` and
`ispa-pkg tool bison-to-ispa`, or install the Python project with `pip install .`.

```sh
flex-to-ispa scanner.l -o scanner.isc
bison-to-ispa parser.y -o parser.isc
```

Each command writes an ISPA grammar and a `.migration.json` report. Use
`--report FILE` to choose the report path. Reports preserve the complete input,
actions and migration diagnostics. Review them before running generated grammars.

Flex conversion supports named regular expression definitions, literals,
character classes, grouping, alternatives and `*`, `+`, `?`. Simple `return TOKEN;`
actions name tokens, and empty actions become skipped rules. Start conditions,
anchors, trailing context, counted repetition, negated and POSIX classes need
manual conversion and are reported. Match priority and longest-match behavior
need validation. C/C++ actions are preserved for manual migration.

Bison conversion supports productions, alternatives, empty productions, token
aliases and preserved semantic actions. Precedence annotations are reported;
types, declarations, parser options and semantic actions need manual migration.
Provide the corresponding lexer rules and reconcile literal/token names when
combining outputs. The original C++ source converter has been removed; ANTLR
companion source conversion remains available through `antlr-to-ispa --code`.
