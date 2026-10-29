# cpp-core: `--stdlib core`

The language-neutral runtime (`parser/stdlibs/python/ispa_std`, package **target-core**) lowered to C++ by ispa itself,
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

Package (`ispa-pkg install cpp-core`): independent of `cpp-target`; it requires `target-core`, installs only this directory (plus the
few `ispastdlib` headers the facade still includes) and builds `libcpp-core.a` with C++17. `ispa-pkg --export` adds its
headers and archive to `ISPA_CPP_INCLUDE_DIRS` / `ISPA_CPP_LIBRARIES`, and lists the archive alone in `ISPA_CPP_CORE_LIBRARIES`.
`ispa-pkg --build cpp-core` rebuilds it.

LR runs in the core too (`ispa_core/lr_machine.hpp`, `lr_conflicts.hpp`): the generated static callbacks are adapted onto the lowered `LRGrammar<V>`, `V` being the parser's `LRValue`, which the core defines as closed tagged storage (`ispa_core/lr_value.hpp`, RawToken-style, no `std::any`) holding the rule nodes and shifted token nodes inline; `lr_recover` and `lr_semantic_reduce` are not available through the core (static_assert).
Parity is checked by `tools/core-parity`.
