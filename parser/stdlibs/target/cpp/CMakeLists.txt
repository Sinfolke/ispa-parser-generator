cmake_minimum_required(VERSION 3.22)
project(ispastdlib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------------------------------------------------
# Headers and Sources
# ------------------------------------------------------------
file(GLOB_RECURSE ISPA_STDLIB_HEADERS
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/ispastdlib.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/ispastdlib/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/ispastdlib/*.h"
)

file(GLOB_RECURSE ISPA_STDLIB_SOURCES
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# OS headers define macros such as IN that collide with generated token names.
# Only core.cpp may include them; never inject them through the runtime PCH.
list(REMOVE_ITEM ISPA_STDLIB_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/ispastdlib/platform.hpp"
)

# ------------------------------------------------------------
# Library Target
# ------------------------------------------------------------
if(ISPA_STDLIB_SOURCES)
    add_library(ispastdlib STATIC ${ISPA_STDLIB_SOURCES})
else()
    add_library(ispastdlib INTERFACE)
endif()

# Create alias
add_library(ispastdlib::ispastdlib ALIAS ispastdlib)

# ------------------------------------------------------------
# Target Configuration
# ------------------------------------------------------------
if(TARGET ispastdlib)
    target_include_directories(ispastdlib PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<INSTALL_INTERFACE:include>
    )

    target_compile_features(ispastdlib PUBLIC cxx_std_17)

    # Consumers may import std; injecting this PCH mixes textual standard
    # library declarations with the module's declarations (notably on Clang 20).
    target_precompile_headers(ispastdlib PRIVATE
            ${ISPA_STDLIB_HEADERS}
    )

    if(WIN32)
        # Keep min/max macros out of the stack-trace implementation and consumers
        # that include Windows headers themselves.
        target_compile_definitions(ispastdlib PUBLIC NOMINMAX)
        target_link_libraries(ispastdlib PRIVATE dbghelp)
    endif()

    if(MSVC)
        target_compile_options(ispastdlib PRIVATE /MP /bigobj)
    endif()
endif()
