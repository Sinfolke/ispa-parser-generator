# cpp_residue

Headers that stay C++. Everything else from `ispastdlib` is ported to `ispa_std/`.
Typed headers need the original `ispastdlib/` directory on the include path as well
(they include `node.hpp`, `parser_base.hpp`, ...). All files compile with `g++ -std=c++17`.

| Header | Status | Why it stays C++ |
|---|---|---|
| `core.hpp`, `platform.hpp` | C++-only | `std::cerr` exception types, compiler/OS macros. Python side uses `failures.py` + `prim.fail`. |
| `traits.hpp`, `variant_io.hpp` | C++-only | SFINAE/detection idioms and `std::variant` streaming; replaced by `has_*` methods and `reflect.py`. |
| `token_flow.hpp`, `python_collections.hpp`, `span.hpp` (ostream ops) | C++-only | C++ container adapters and `operator<<`. |
| `raw_token.hpp`, `assign.hpp`, `to_string.hpp` | PARTIAL | Static-type-directed templates. Ported as `type_id` / `Shape` / dynamic `to_string`; typed originals kept for C++ targets. |
| `lr_machine.hpp` (`lr_convert`) | PARTIAL | Type-directed conversion; driver ported to `lr.py`, conversion uses `Shape`. |
| `lr_parser.hpp` | C++-only | Legacy `ISPA_EXPERIMENTAL` driver with `printf` tracing; superseded by `lr.py`. |
| `std_prims.hpp` (new) | NEW | C++ side of `prim.fail` / `prim.attempt`, same ErrorKind numbers as `errors.py`. |
| `debug_transport.hpp` (new) | NEW | getenv / write(2) / read(2) / mutex adapters for `debug.configure(sink, control)`. |
| `file_io.hpp` (new) | NEW | `makeTokensFromFile` file read. |

PORTED (no residue): lexer, dfa_api, dfa_run, parser_base, parser_ll, ispa_recovery(+_context),
recovery_scan, external, counter, node, ast_printer, ispa_debug (framing/stepping), lr_conflicts.
