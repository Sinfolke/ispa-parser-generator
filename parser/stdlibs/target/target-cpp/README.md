# target-cpp

The C++ runtime is generated from `parser/stdlibs/python/ispa_std`.
`ispa_core/` provides the typed interface; `ispastdlib/` provides native interoperability
needed by lowered Python declarations. Generated parsers use `ISPA_CORE` by default.

Install with `ispa-pkg install target-cpp` (requires `target-core`). The package builds
`libispa-core.a` containing the lowered runtime and native support. With `ispa` on PATH,
installation and `ispa-pkg --build target-cpp` regenerate the runtime from installed Python sources.
Without the generator, installation builds the shipped generated sources.

Run `python3 regen.py --ispa /path/to/ispa` to regenerate repository sources.
`--library PATH` selects another Python source root containing `ispa_std/`.

Use `ispa --export` to emit shell variables and refresh the CMake and JSON manifests.
`ISPA_CPP_CORE_LIBRARIES` names the built runtime archive.
