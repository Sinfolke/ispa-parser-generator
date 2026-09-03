# Build and Toolchain Notes

This file supplements `docs/ARCHITECTURE.md` with the concrete build assumptions for the current project.

## Toolchain expectations

The repository’s README explicitly expects:

- Linux: Clang + libc++
- Windows: MSVC / cl.exe
- modern CMake with vcpkg integration

The root `CMakeLists.txt` enforces C++23 and module scanning, which is enough to exclude GCC from the primary supported path at present.

## Why the project is split into static and shared libs

The `ispa_modules` static library centralizes the parser engine and module interfaces. The generated C++ backend is then kept separate as a shared library (`ispa-converter-cpp`). This ensures the engine can be compiled once and the emitter can be loaded or rebuilt independently as a target-specific plugin.

## Dependencies

The build graph is intentionally small but specialized:

- Boost for generic utility support
- CLI11 for CLI parsing
- yaml-cpp for YAML-related tests and data handling
- RopeString imported via FetchContent
- GTest for the test suite

This combination keeps the engine focused on parser generation while allowing the project to validate and emit structured outputs without becoming a monolithic application.
