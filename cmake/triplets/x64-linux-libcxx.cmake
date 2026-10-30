set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CMAKE_SYSTEM_NAME Linux)

set(VCPKG_CRT_LINKAGE dynamic)
# Static so the `ispa` executable (and the other CLI tools) don't need yaml-cpp/Boost/ICU/CLI11 .so files
# alongside them at runtime (previously: "libyaml-cpp.so.0.8: cannot open shared object file"). The
# ispa-converter-*/ispa-algorithm-* plugins stay shared regardless: that's our own add_library(... SHARED),
# not this triplet's linkage for third-party vcpkg dependencies.
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_C_FLAGS "-fPIC")
set(VCPKG_CXX_FLAGS "-fPIC")
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE
        "${CMAKE_CURRENT_LIST_DIR}/../toolchains/clang-libcxx.cmake")