cmake_minimum_required(VERSION 3.22)
project(ispa_parser LANGUAGES CXX)

# Do not change the global C++ standard when embedded as a subproject.
option(ISPA_BUILD_EXECUTABLES "Build the ISPA and Python parser runners" ${PROJECT_IS_TOP_LEVEL})
option(ISPA_BUILD_SELF_PARSER "Build the ISPA grammar parser" ON)
option(ISPA_BUILD_DEP_PARSER "Build the dependency scanner" ON)
option(ISPA_BUILD_PYTHON_PARSER "Build the Python grammar parser" ON)
option(ISPA_ENABLE_PCH "Enable precompiled headers for parsers" ON)

set(ISPA_PARSER_STDLIB "core" CACHE STRING "Runtime the in-tree parsers are generated against: ispastdlib (hand-written C++) or core (ispa_std lowered to C++ plus the cpp-core facade)")
set_property(CACHE ISPA_PARSER_STDLIB PROPERTY STRINGS ispastdlib core)
if(NOT ISPA_PARSER_STDLIB MATCHES "^(ispastdlib|core)$")
    message(FATAL_ERROR "Invalid ISPA_PARSER_STDLIB='${ISPA_PARSER_STDLIB}' (expected ispastdlib or core)")
endif()

set(ISPA_BOOTSTRAPPED_DIR "" CACHE PATH "Override the ISPA grammar bootstrap directory (for example ispa/bootstrapped_ll)")

set(ISPA_PARSER_MODE "AUTO" CACHE STRING "Parser sources: AUTO, INCREMENTAL, or BOOTSTRAPPED")
set_property(CACHE ISPA_PARSER_MODE PROPERTY STRINGS AUTO INCREMENTAL BOOTSTRAPPED)
if(NOT ISPA_PARSER_MODE MATCHES "^(AUTO|INCREMENTAL|BOOTSTRAPPED)$")
    message(FATAL_ERROR "Invalid ISPA_PARSER_MODE='${ISPA_PARSER_MODE}' (expected AUTO, INCREMENTAL or BOOTSTRAPPED)")
endif()

function(ispa_apply_compile_options target)
    target_compile_features(${target} PRIVATE cxx_std_20)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /MP /bigobj)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(${target} PRIVATE
                -Wno-switch
                -Wno-self-assign-overloaded
                -Wno-unused-but-set-variable
                -Wno-unused-lambda-capture
                -Wno-deprecated-copy
            )
        endif()
    endif()
endfunction()

set(STDLIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/stdlibs/target/cpp")
if(NOT TARGET ispastdlib::ispastdlib)
    add_subdirectory("${STDLIB_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/ispastdlib_build")
endif()

set(ISPA_STDLIB_TARGET ispastdlib::ispastdlib)
if(ISPA_PARSER_STDLIB STREQUAL "core")
    # The lowered core is compiled once; parsers generated with --stdlib core include <ispa_core/...>.
    set(CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/stdlibs/target/cpp-core")
    if(NOT TARGET ispa_core)
        add_library(ispa_core STATIC "${CORE_DIR}/src/ispa_core_lowered.cpp")
        target_include_directories(ispa_core PUBLIC "${CORE_DIR}")
        target_compile_features(ispa_core PUBLIC cxx_std_17)
        target_link_libraries(ispa_core PUBLIC ispastdlib::ispastdlib)
    endif()
    set(ISPA_STDLIB_TARGET ispa_core)
endif()

function(add_ispa_generated_parser parser_name directory executable_name)
    set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${directory}")
    set(OUTPUT_DIR "${ROOT_DIR}/bootstrapped_incremental")
    set(INCREMENTAL_DIR "${OUTPUT_DIR}/incremental")
    set(BOOTSTRAPPED_DIR "${ROOT_DIR}/bootstrapped")
    if(parser_name STREQUAL "ispa" AND ISPA_BOOTSTRAPPED_DIR)
        set(BOOTSTRAPPED_DIR "${ISPA_BOOTSTRAPPED_DIR}")
    endif()

    file(GLOB_RECURSE ALL_INCREMENTAL_SOURCES CONFIGURE_DEPENDS
            "${INCREMENTAL_DIR}/*.cpp"
    )

    file(GLOB MATCHED_PARSER_HEADERS "${OUTPUT_DIR}/*Parser.h")

    set(HAVE_INCREMENTAL FALSE)
    if(MATCHED_PARSER_HEADERS AND ALL_INCREMENTAL_SOURCES)
        set(HAVE_INCREMENTAL TRUE)
    endif()

    file(GLOB MATCHED_BOOTSTRAPPED_HEADERS "${BOOTSTRAPPED_DIR}/*Parser.h")
    file(GLOB MATCHED_BOOTSTRAPPED_SOURCES "${BOOTSTRAPPED_DIR}/*Parser.cpp")

    set(HAVE_BOOTSTRAPPED FALSE)
    if(MATCHED_BOOTSTRAPPED_HEADERS AND MATCHED_BOOTSTRAPPED_SOURCES)
        set(HAVE_BOOTSTRAPPED TRUE)
    endif()

    if(ISPA_PARSER_MODE STREQUAL "INCREMENTAL")
        if(NOT HAVE_INCREMENTAL)
            message(FATAL_ERROR "${parser_name}: incremental parser requested, but output/*Parser.h or incremental sources are missing")
        endif()
        set(PARSER_SOURCE_MODE "INCREMENTAL")
    elseif(ISPA_PARSER_MODE STREQUAL "BOOTSTRAPPED")
        if(NOT HAVE_BOOTSTRAPPED)
            message(FATAL_ERROR "${parser_name}: bootstrapped parser requested, but bootstrapped/*Parser.h or *Parser.cpp is missing")
        endif()
        set(PARSER_SOURCE_MODE "BOOTSTRAPPED")
    elseif(HAVE_INCREMENTAL)
        set(PARSER_SOURCE_MODE "INCREMENTAL")
    elseif(HAVE_BOOTSTRAPPED)
        set(PARSER_SOURCE_MODE "BOOTSTRAPPED")
    else()
        message(FATAL_ERROR
                "${parser_name}: no complete parser sources found. Expected either "
                "${OUTPUT_DIR}/*Parser.h + incremental/*.cpp, or "
                "${BOOTSTRAPPED_DIR}/*Parser.h + *Parser.cpp"
        )
    endif()

    message(STATUS "${parser_name}: using ${PARSER_SOURCE_MODE} parser")

    if(PARSER_SOURCE_MODE STREQUAL "INCREMENTAL")
        set(AST_SOURCES)
        set(PARSER_SOURCES)
        set(OTHER_SOURCES)

        foreach(src IN LISTS ALL_INCREMENTAL_SOURCES)
            if(src MATCHES "[/\\\\]Parser[/\\\\]FlatTypes[/\\\\]")
                list(APPEND AST_SOURCES "${src}")
            elseif(src MATCHES "[/\\\\]Parser[/\\\\]Parser[/\\\\]")
                list(APPEND PARSER_SOURCES "${src}")
            else()
                list(APPEND OTHER_SOURCES "${src}")
            endif()
        endforeach()

        list(LENGTH AST_SOURCES AST_COUNT)
        list(LENGTH PARSER_SOURCES PARSER_COUNT)
        list(LENGTH OTHER_SOURCES OTHER_COUNT)
        message(STATUS "${parser_name}: AST=${AST_COUNT}, parser=${PARSER_COUNT}, support=${OTHER_COUNT}")

        set(OBJECT_TARGETS)
        foreach(kind IN ITEMS ast parser_impl support)
            if(kind STREQUAL "ast")
                set(SOURCES ${AST_SOURCES})
            elseif(kind STREQUAL "parser_impl")
                set(SOURCES ${PARSER_SOURCES})
            else()
                set(SOURCES ${OTHER_SOURCES})
            endif()

            if(NOT SOURCES)
                continue()
            endif()

            set(target "${parser_name}_${kind}")
            add_library(${target} OBJECT ${SOURCES})
            list(APPEND OBJECT_TARGETS "${target}")

            ispa_apply_compile_options(${target})
            target_include_directories(${target} PRIVATE
                    "${OUTPUT_DIR}"
                    "${ROOT_DIR}"
                    "${CMAKE_CURRENT_SOURCE_DIR}"
            )
            target_link_libraries(${target} PRIVATE ${ISPA_STDLIB_TARGET})

            if(ISPA_ENABLE_PCH)
                if(kind STREQUAL "parser_impl" AND EXISTS "${INCREMENTAL_DIR}/Parser/Parser/def.h")
                    target_precompile_headers(${target} PRIVATE
                            <ispastdlib.hpp>
                            "${INCREMENTAL_DIR}/Parser/Parser/def.h"
                    )
                else()
                    target_precompile_headers(${target} PRIVATE <ispastdlib.hpp>)
                endif()
            endif()
        endforeach()

        # The object libraries own compilation of individual generated units.
        # Rebuild the static archive from their object files.
        add_library(${parser_name}_lib STATIC)
        foreach(target IN LISTS OBJECT_TARGETS)
            target_sources(${parser_name}_lib PRIVATE "$<TARGET_OBJECTS:${target}>")
        endforeach()

        target_include_directories(${parser_name}_lib PUBLIC
                "${OUTPUT_DIR}"
                "${ROOT_DIR}"
                "${CMAKE_CURRENT_SOURCE_DIR}"
        )

    else()
        # Public headers in existing clients may use either <*Parser.h> or
        # <output/*Parser.h>. Stage a compatibility layout in the build tree.
        # This never writes into the source tree or into generated output/.
        set(BOOTSTRAP_INCLUDE_ROOT "${CMAKE_CURRENT_BINARY_DIR}/${parser_name}_bootstrap_include")
        file(MAKE_DIRECTORY "${BOOTSTRAP_INCLUDE_ROOT}/output")

        # Stage all matched header files dynamically into the bootstrap include root
        foreach(hdr IN LISTS MATCHED_BOOTSTRAPPED_HEADERS)
            get_filename_component(hdr_name "${hdr}" NAME)
            configure_file("${hdr}" "${BOOTSTRAP_INCLUDE_ROOT}/output/${hdr_name}" COPYONLY)
        endforeach()

        add_library(${parser_name}_lib STATIC ${MATCHED_BOOTSTRAPPED_SOURCES})
        ispa_apply_compile_options(${parser_name}_lib)
        target_include_directories(${parser_name}_lib PUBLIC
                "${BOOTSTRAP_INCLUDE_ROOT}"
                "${BOOTSTRAPPED_DIR}"
                "${ROOT_DIR}"
                "${CMAKE_CURRENT_SOURCE_DIR}"
        )
        if(ISPA_ENABLE_PCH)
            target_precompile_headers(${parser_name}_lib PRIVATE <ispastdlib.hpp>)
        endif()
    endif()

    # Consumers of the self parser (including the custom lexer) must use the
    # same generated namespace layout as the selected parser implementation.
    if(parser_name STREQUAL "ispa")
        if(PARSER_SOURCE_MODE STREQUAL "INCREMENTAL")
            set(SELECTED_PARSER_HEADERS ${MATCHED_PARSER_HEADERS})
        else()
            set(SELECTED_PARSER_HEADERS ${MATCHED_BOOTSTRAPPED_HEADERS})
        endif()
        foreach(header IN LISTS SELECTED_PARSER_HEADERS)
            file(READ "${header}" PARSER_HEADER_CONTENT)
            if(PARSER_HEADER_CONTENT MATCHES "Types::modules::common::ID")
                target_compile_definitions(${parser_name}_lib PUBLIC ISPA_MODULAR_PARSER=1)
            endif()
            if(PARSER_HEADER_CONTENT MATCHES "ISPA_PYTHON::|PyRawParser::")
                target_compile_definitions(${parser_name}_lib PUBLIC ISPA_EXTERNAL_PYTHON=1)
            endif()
        endforeach()
    endif()

    target_compile_features(${parser_name}_lib PUBLIC cxx_std_20)
    set_target_properties(${parser_name}_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_link_libraries(${parser_name}_lib PUBLIC ${ISPA_STDLIB_TARGET})

    if(ISPA_BUILD_EXECUTABLES)
        if(EXISTS "${ROOT_DIR}/main.cpp")
            add_executable(${executable_name} "${ROOT_DIR}/main.cpp")
            ispa_apply_compile_options(${executable_name})
            target_link_libraries(${executable_name} PRIVATE ${parser_name}_lib)
        else()
            message(STATUS "${parser_name}: main.cpp not found at ${ROOT_DIR}/main.cpp; CLI for this parser is not supported")
        endif()
    endif()
endfunction()

if(ISPA_BUILD_SELF_PARSER)
    add_ispa_generated_parser(ispa ispa isparun)
    add_library(ISPA::Parser ALIAS ispa_lib)
endif()

if(ISPA_BUILD_PYTHON_PARSER)
    add_ispa_generated_parser(python python pyrun)
    add_library(ISPA::PythonParser ALIAS python_lib)
    target_sources(python_lib PRIVATE python/extern.cpp)
    add_library(ISPA::PythonExternal ALIAS python_lib)
    if(TARGET ispa_lib)
        target_link_libraries(ispa_lib PUBLIC python_lib)
    endif()
endif()
if(ISPA_BUILD_DEP_PARSER)
    add_ispa_generated_parser(dep_scanner ispa_dep_scanner scanrun)
    add_library(ISPA::DepScanner ALIAS dep_scanner_lib)
endif()
message(STATUS "--------------------------------------")
message(STATUS "ISPA generated parsers")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "C++ standard: C++20")
message(STATUS "Build executables: ${ISPA_BUILD_EXECUTABLES}")
message(STATUS "Build ISPA parser: ${ISPA_BUILD_SELF_PARSER}")
message(STATUS "Build Python parser: ${ISPA_BUILD_PYTHON_PARSER}")
message(STATUS "Build Dep Scanner: ${ISPA_BUILD_DEP_PARSER}")
message(STATUS "Parser selection: ${ISPA_PARSER_MODE}")
message(STATUS "Precompiled headers: ${ISPA_ENABLE_PCH}")
message(STATUS "Stdlib directory: ${STDLIB_DIR}")
message(STATUS "--------------------------------------")
