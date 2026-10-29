# cpp-core: `--stdlib core`

The language-neutral runtime (`parser/stdlibs/python/ispa_std/out`, package **target-core**) lowered to C++ by ispa itself,
plus a thin native facade. A parser generated with `--stdlib core` behaves like one generated against the hand-written
`ispastdlib` (same AST, diagnostics and recovery); only the implementation is shared with the other targets.

    ispa -l cpp -a LL --stdlib core grammar.isc -o out

| piece | role |
|---|---|
| `ispa_core_lowered.{h,cpp}` | the Python runtime lowered to C++ (generated: `python3 regen.py --ispa <ispa>`) |
| `ispa_core/*.hpp` | the facade. Forwards every `ispastdlib` header, bridges typed iterators/nodes to the core, and keeps the native residue: typed cursors and nodes, `custom_failure`/`fail_context`, counted repetitions, the DFA hook |
| `ispa-stdlib.yaml` | module description read by `--stdlib` (`module`, `namespace`, `include`) |

`--stdlib` takes a directory containing `ispa-stdlib.yaml`, or the `module` name of an installed package directory
(searched under the ispa-pkg library root). `--stdlib-name` overrides the namespace. `--static` is not implemented yet.

Packages (`ispa-pkg install cpp-target`): `cpp-target` requires `target-core`; its `cpp-core` directory is built into
`libcpp-target-core.a` with C++23 when a toolchain allows it. `ispa-pkg --export` adds its headers and archive to
`ISPA_CPP_INCLUDE_DIRS` / `ISPA_CPP_LIBRARIES`.

Not available through the facade yet: advanced-rule contexts (`rule_context`), and LR (`lr_parse`/`lr_conflicts`).
Parity is checked by `tools/core-parity`.
