# Let CMake select the compiler from:
# -DCMAKE_C_COMPILER / -DCMAKE_CXX_COMPILER
# or CC / CXX environment variables.

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Chainloading bypasses vcpkg's Linux toolchain, so forward its flags explicitly.
# Makefile-based ports such as ICU consume these flags, not CMake's PIC property.
set(CMAKE_C_FLAGS_INIT "${VCPKG_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS_INIT "${VCPKG_CXX_FLAGS} -fPIC -stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-stdlib=libc++")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-stdlib=libc++")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-stdlib=libc++")
