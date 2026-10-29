# ANTLR C++14 conversion test

Tested `antlr/grammars-v4`, commit
`2766c2eed04f285dc763617c9459650892fcf5b9`, using
`cpp/CPP14Parser.g4` and its `CPP14Lexer.g4` token vocabulary.

```sh
git clone --depth 1 --filter=blob:none --sparse https://github.com/antlr/grammars-v4.git /tmp/antlr-cpp-grammars
git -C /tmp/antlr-cpp-grammars sparse-checkout set cpp
antlr-to-ispa /tmp/antlr-cpp-grammars/cpp/CPP14Parser.g4 -o /tmp/CPP14.isc
cmake-build-release/ispa -l python --check-only /tmp/CPP14.isc
ANTLR_CPP_GRAMMAR=/tmp/antlr-cpp-grammars/cpp/CPP14Parser.g4 python -m unittest tests/test_antlr_cpp_grammar.py -v
```

Results:

- Conversion succeeded: 191 parser rules and 171 lexer/fragment rules.
- Token vocabulary resolved automatically; no undefined-rule, duplicate-rule,
  or dependency warnings.
- ISPA syntax and semantic checking succeeded with zero diagnostics.
- Conversion emitted 13 migration warnings and preserved two predicates in
  the JSON report.

Required migration work consists of explicit EOF handling, six non-greedy
quantifiers, one labeled capture, two predicates calling
`this.IsPureSpecifierAllowed()`, two hidden-channel commands, and the
`CPP14ParserBase` superclass option. The predicates require the base parser's
runtime behavior, not just syntax translation.

This validates conversion and ISPA checking. It does not establish behavioral
equivalence or demonstrate a generated parser parsing C++ source. Left recursion
is deliberately preserved pending generator support, and the reported semantic
differences must be resolved before claiming a working C++ parser.
