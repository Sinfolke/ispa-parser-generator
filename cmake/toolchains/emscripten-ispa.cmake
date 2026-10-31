# Emscripten toolchain for ISPA's dependencies. vcpkg only applies VCPKG_C_FLAGS/VCPKG_CXX_FLAGS
# for platforms it knows, which does not include Emscripten, so the flags are set here.
#  - C++ exceptions: Emscripten disables them by default and ISPA and its dependencies throw.
#  - C++23: CLI11 changes class layout with the language standard and must match the main build.
include("$ENV{EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
string(APPEND CMAKE_C_FLAGS_INIT " -fwasm-exceptions")
string(APPEND CMAKE_CXX_FLAGS_INIT " -fwasm-exceptions")
set(CMAKE_CXX_STANDARD 23)
