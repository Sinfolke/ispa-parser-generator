#!/usr/bin/env bash
# Build the ISPA generator for the browser (WebAssembly) and package the playground runtime.
#
# Requires: an activated Emscripten SDK (EMSDK set, emcc on PATH), VCPKG_ROOT, CMake 4.5+, Ninja.
# usage: scripts/build-wasm.sh [build-dir]       (default: build-wasm)
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="$(realpath -m "${1:-$root/build-wasm}")"
: "${EMSDK:?activate the Emscripten SDK first (source emsdk_env.sh)}"
: "${VCPKG_ROOT:?set VCPKG_ROOT to a vcpkg checkout}"

# Host compiler flags (e.g. from oneAPI setups) must never leak into a cross build.
unset CC CXX CFLAGS CXXFLAGS LDFLAGS CPATH C_INCLUDE_PATH CPLUS_INCLUDE_PATH LIBRARY_PATH LD_LIBRARY_PATH

triplet=wasm32-emscripten-ispa
"$VCPKG_ROOT/vcpkg" install \
    --x-manifest-root="$root/cmake/wasm" \
    --x-install-root="$build/vcpkg_installed" \
    --triplet "$triplet" \
    --overlay-triplets="$root/cmake/triplets"
prefix="$build/vcpkg_installed/$triplet"

emcmake cmake -S "$root" -B "$build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$prefix" \
    -DCMAKE_FIND_ROOT_PATH="$prefix" \
    -DCMAKE_CXX_FLAGS_RELEASE="-Oz -DNDEBUG" \
    -DISPA_WASM_SIZE_OPT=ON
cmake --build "$build" --target ispa -j"${JOBS:-3}"

python3 "$root/scripts/package-playground.py" "$build"
