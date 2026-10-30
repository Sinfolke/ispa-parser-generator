# Dependency parser

`grammar.isc` owns source structure: module declarations, imports, features,
detach directives, rule headers and nested declarations. It imports names,
parameters and lexical types from `parser/ispa/grammar/common.isc`. Module paths
use dots; rule paths use the shared dotted name syntax. Rule bodies are opaque
except for balanced delimiters and nested rule boundaries.

`parser.h` supplies lexical handling for opaque Python action blocks and comment
trivia. `Modules::preprocess` consumes the generated tree for discovery and symbol
resolution; it does not tokenize or parse source text.

Regenerate the checked-in LL parser from the repository root:

```sh
cmake-build-debug/ispa -l cpp -a LL --debug false --no-nfa-debug \
  -o parser/ispa_dep_scanner/bootstrapped/DepParser \
  --dir parser/ispa_dep_scanner/grammar --dir parser/ispa/grammar \
  --dep python parser/python/bootstrapped/PyRawParser \
  parser/ispa_dep_scanner/entry.isc
```
