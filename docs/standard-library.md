# Standard library

Install grammar modules and compile-time Python helpers:

```sh
ispa-pkg install std
```

The package uses the existing public catalog and versioned library release.
New additions become publicly available when a library release containing them
is published. Installing a checkout through a local catalog can test unreleased changes.

```isc
import {INTEGER, HEX_INTEGER} from std.numbers;
import py {clamp} from stdmath;
```

Python modules are ISPA compiler inputs. They use `langapi` intrinsics and are
not standalone CPython packages. Install the separate `python-target` package
for generated Python parser runtime support.

## Grammar modules

| Module | Tokens/rules |
| --- | --- |
| `std.numbers` | `UNSIGNED`, signed `INTEGER`, prefixed `HEX_INTEGER`, `BINARY_INTEGER`, `OCTAL_INTEGER`, `DECIMAL` with optional fraction and exponent |
| `std.whitespace` | `HORIZONTAL`, `NEWLINE` (LF, CRLF, CR), `WHITESPACE` |
| `std.names` | `ID`, `qualified_name<sep>` |
| `std.literals` | `SIMPLE_NUMBER`, `STRING<quote>`, `ID` |
| `std.collections` | `list<member, separator>`, `key_value<key, value, kv_sep, sep>` |

New number tokens preserve source text. They do not convert bases or enforce
word boundaries: add surrounding grammar constraints when required. Decimal
fractions require digits on both sides of the dot. Numeric separators, NaN and
infinity are intentionally outside these rules. Whitespace tokens do not
implicitly skip input or manage indentation.

## Python helpers

| Import module | Functions |
| --- | --- |
| `stdfun` | Existing `range`, `sum`, `min`, `max`, `abs`, `all`, `any`, `reversed`, `sorted`, mapping-style `zip` |
| `stdmath` | `clamp`, `sign`, `gcd`, half-open `in_range` |
| `stdseq` | Stable `unique`, `count`, first-index `find`, `take`, `drop`, `product` |
| `stdtext` | `starts_with`, `ends_with`, first-offset `find`, `contains`, `repeat`, `join` |

Sequence functions operate on integer lists and never mutate their input.
`find` returns -1 when absent. Empty products are 1. Negative `take` and `repeat`
counts produce empty results; negative `drop` counts behave as zero. `clamp`
requires ordered bounds. Integer overflow follows the selected target's integer
representation. String indexing follows the target's string representation;
these functions do not promise Unicode normalization or grapheme indexing.

`unique` uses quadratic comparisons; other sequence operations are linear.
String search uses a simple literal scan, without regex syntax.
