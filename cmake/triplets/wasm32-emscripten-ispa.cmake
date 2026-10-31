# WebAssembly build of the generator. Mirrors vcpkg's wasm32-emscripten triplet but chain-loads
# a toolchain that enables exceptions and C++23.
set(VCPKG_ENV_PASSTHROUGH_UNTRACKED EMSCRIPTEN_ROOT EMSDK PATH)

if(NOT DEFINED ENV{EMSCRIPTEN_ROOT})
    find_path(EMSCRIPTEN_ROOT "emcc")
else()
    set(EMSCRIPTEN_ROOT "$ENV{EMSCRIPTEN_ROOT}")
endif()
if(NOT EMSCRIPTEN_ROOT)
    if(NOT DEFINED ENV{EMSDK})
        message(FATAL_ERROR "The emcc compiler not found in PATH")
    endif()
    set(EMSCRIPTEN_ROOT "$ENV{EMSDK}/upstream/emscripten")
endif()

set(VCPKG_TARGET_ARCHITECTURE wasm32)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Emscripten)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../toolchains/emscripten-ispa.cmake")
