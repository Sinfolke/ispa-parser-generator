# antlr-to-ispa

Convert ANTLR 4 `.g4` files into ISPA `.isc` files using static analysis.
No input actions are executed. Left recursion and alternative order are
preserved, including mutually recursive rules.

```sh
ispa-pkg tool antlr-to-ispa
antlr-to-ispa Example.g4 -I grammars -o Example.isc
```

The installer selects the latest release asset for Linux x64, Windows x64,
macOS x64 or macOS arm64. `--version TAG` pins a release; `--bin-dir DIR`
chooses the installation directory; `--force` replaces an installed binary.
By default tools go into the `bin` sibling of the package library directory.
The executable and all language parsers are bundled by the existing Nuitka
release workflow. Source installation is `pip install '.[antlr-to-ispa]'`
with Python 3.10 or newer (the language pack's minimum version).

The converter writes both the grammar and `Example.migration.json`. Warnings
do not fail conversion: the terminal output and generated grammar end with
the changes required before running it. The report retains complete source
files, source spans, original actions, translated Python and integration status.
Malformed input or I/O failures return a nonzero status. Warnings return zero.

`--language` overrides `options { language=...; }`; the default is Java.
`--list-targets` lists Java, Cpp, CSharp, Python3, JavaScript, TypeScript, Go,
Swift, Dart, PHP and Rust. Imports and `tokenVocab` resolve sibling `.g4` files
and `-I` directories. Missing files and cyclic imports produce warnings.

## Companion source files

Use repeatable `--code FILE` arguments for helper/base-code files alongside the
grammar. The extension selects the source language; unknown extensions use the
grammar's action language. All eleven action languages use Tree-sitter for full
source-file parsing too.

```sh
antlr-to-ispa Example.g4 --code Helpers.java --code Utilities.java -o converted/Example.isc
```

Create the output directory first. This writes `antlr_helpers.py` and
`antlr_utilities.py` beside the grammar and inserts `import py antlr_helpers;`
and `import py antlr_utilities;`. Calls to known helper functions are rewritten
to those module names. For example, Java `Helpers.twice(21)` becomes
`antlr_helpers.Helpers_twice(21)` when converting this helper:

```java
class Helpers {
    public static int twice(int x) { return x * 2; }
}
```

The generated module contains:

```python
def Helpers_twice(x: int) -> int:
    return (x * 2)
```

The converter supports free functions, static helper methods (flattened into
module functions), supported function bodies and simple scalar global
initializers. It links known helper calls across supplied files, including
recursive calls. Python files retain their code, with imports to explicitly
supplied sibling modules renamed to the generated module names. Input files
and libraries are never executed during conversion.

Unknown libraries/imports, unresolved globals, unsupported declarations,
overloads, instance state and inheritance produce warnings. Foreign functions
that cannot be converted are omitted as a whole; their callers are also
reported when the missing function cannot be resolved. Complete source and
omitted declarations remain in the migration report, and the generated module
contains TODO comments. Python library imports are preserved with warnings:
they still need a generator-supported implementation or explicit API mapping.
No guessed library implementation or successful no-op stub is substituted.

Primitive signature types are translated. Untyped source functions produce
Python plus warnings to add the annotations ISPA requires for cross-target
lowering. As with actions, review numeric and runtime semantics before use.
Existing companion output files with different contents are protected; pass
`--overwrite-code` to replace them. Original input files are always protected.

## Translation boundaries

This is a conservative migration tool, not a complete ANTLR runtime emulator.
Tree-sitter parses foreign actions, then explicit AST handlers lower supported
local declarations, assignments, arithmetic/comparisons and conditionals to
Python. Every target, including Python, is parsed through Tree-sitter; Python's
AST is used to normalize the resulting Python source. The language pack is pinned to
an offline, bundled-parser release; conversion never downloads parsers.

Straight-line parser rules can embed translated actions in advanced ISPA rules,
including explicit token-label `.text` and `.int` bindings. Actions in choices,
predicates, named actions, lexer actions, rule parameters/returns/locals,
member/runtime access and external API calls are retained in the report with
migration warnings. Unsupported actions are rejected as a whole. Division,
coercive equality and other semantics requiring type information are not guessed.
Machine-integer overflow and other differences in numeric models still require
review when translating arithmetic.
Builtin calls also receive a target-validation warning: the current Python
generator can rename calls such as `int` to `int_`. The migration report calls
out the need for a helper or a generator mapping fix in that case.

Rule references, groups, alternatives, literals, ranges, character sets,
quantifiers, fragments and whole-rule `skip` are emitted directly. Lexer modes,
channels, conditional commands, non-greedy quantifiers, explicit EOF assertions,
Unicode properties, alternative labels and unsupported options are reported.
The output can intentionally contain unresolved references: consult the report
before generating or running a parser. ANTLR listener/visitor APIs are not
generated; define the desired ISPA AST captures for their consumers.

Token and fragment names are fully uppercased, with all grammar references
updated. Existing uppercase names are reserved; collisions receive numeric
suffixes. The report's `token_names` mapping records the allocated names.
ANTLR attribute references continue to use their original source names during
action analysis, so `$Number.text` still binds after `Number` becomes `NUMBER`.

## Layout and development

`cli/pkg/antlr_to_ispa/grammar/antlr.isc` describes ANTLR metasyntax and leaves
action bodies opaque. `scanner.py` and `parser.py` provide the source-preserving
Python bootstrap used by the CLI. No per-target language grammars are authored.
`model.py`, `analysis.py`, `actions.py`, `emitter.py` and `cli.py` separate the
IR, dependency/static checks, language lowering, output and user interface.
`companions.py` handles source-file declarations, helper linking, and Python
module generation.

```sh
python -m unittest tests/test_antlr_to_ispa.py tests/test_antlr_to_ispa_generated.py -v
python -m unittest tests/test_antlr_companions.py -v
cmake-build-release/ispa -l python --check-only cli/pkg/antlr_to_ispa/grammar/antlr.isc
```

New action mappings should operate on Tree-sitter nodes and fail atomically
when semantics are unknown; add behavior tests for every affected language.
The compiled smoke test converts an action in every supported language to
verify that Nuitka includes dynamically loaded parser modules.

References: [ANTLR targets](https://github.com/antlr/antlr4/blob/master/doc/targets.md),
[ANTLR actions](https://github.com/antlr/antlr4/blob/master/doc/actions.md),
[Tree-sitter language pack](https://github.com/Goldziher/tree-sitter-language-pack).
