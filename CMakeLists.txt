cmake_minimum_required(VERSION 4.5.0-rc1 FATAL_ERROR)
set(CMAKE_CXX_MODULE_STD ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
if(APPLE)
    set(CMAKE_BUILD_RPATH "@loader_path")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-stdlib=libc++)
    add_link_options(-stdlib=libc++)
endif()

project(ISPA LANGUAGES CXX)

if(EMSCRIPTEN)
    # Emscripten disables C++ exceptions by default; ISPA relies on them.
    add_compile_options(-fwasm-exceptions)
    add_link_options(-fwasm-exceptions)
endif()

set(PROGRAM_VERSION "dev" CACHE STRING "Version shown by the program")

if(MSVC)
    add_compile_options(/bigobj)
endif()
# =============================================
# Compile Definitions
# =============================================
add_compile_definitions(
        __SOURCE_ROOT__="${CMAKE_CURRENT_SOURCE_DIR}"
        PROGRAM_VERSION="${PROGRAM_VERSION}"
        O_LEVEL=${O_LEVEL}
        DSTD_NO_DEBUG=1
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_options(-g)
    endif()
endif()

if(NOT DEFINED O_LEVEL OR "${O_LEVEL}" STREQUAL "")
    set(O_LEVEL 0)
endif()

# =============================================
# Helper Functions
# =============================================
function(get_file_range output_var)
    cmake_parse_arguments(ARG "" "EXCLUDE" "" ${ARGN})
    set(result "")
    foreach(path IN LISTS ARG_UNPARSED_ARGUMENTS)
        file(GLOB_RECURSE temp CONFIGURE_DEPENDS ${path})
        list(APPEND result ${temp})
    endforeach()
    list(REMOVE_DUPLICATES result)
    if(ARG_EXCLUDE)
        list(FILTER result EXCLUDE REGEX "${ARG_EXCLUDE}")
    endif()
    set(${output_var} ${result} PARENT_SCOPE)
endfunction()
function(get_file_range_non_recurse output_var)
    cmake_parse_arguments(ARG "" "EXCLUDE" "" ${ARGN})

    # Read existing contents from parent scope if available
    set(result "${${output_var}}")

    foreach(path IN LISTS ARG_UNPARSED_ARGUMENTS)
        # Use GLOB to match only the specified directory (non-recursive)
        file(GLOB temp CONFIGURE_DEPENDS ${path})
        list(APPEND result ${temp})
    endforeach()

    if(ARG_EXCLUDE AND result)
        list(FILTER result EXCLUDE REGEX "${ARG_EXCLUDE}")
    endif()

    if(result)
        list(REMOVE_DUPLICATES result)
    endif()

    set(${output_var} "${result}" PARENT_SCOPE)
endfunction()
# =============================================
# Fetch & Find Dependencies
# =============================================

find_package(Boost REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
if(NOT EMSCRIPTEN)
    find_package(ICU REQUIRED COMPONENTS uc)
endif()

include(cmake/BootstrapIspa.cmake)
ispa_bootstrap_regenerate()

add_subdirectory(parser)

# =============================================
# ISPA Modules Library
# =============================================
add_library(ispacore STATIC)
if(EMSCRIPTEN)
    set(ISPA_PIC OFF)  # one static executable: PIC would change the wasm feature set per target
else()
    set(ISPA_PIC ON)
endif()
set_target_properties(ispacore PROPERTIES
    POSITION_INDEPENDENT_CODE ${ISPA_PIC}
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    BUILD_RPATH "${CMAKE_CURRENT_BINARY_DIR}"
)

get_file_range(ISPA_MODULE_INTERFACES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cppm
        ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/*.cppm
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.cppm
        ${CMAKE_CURRENT_SOURCE_DIR}/external/*.cppm
)

# Collect implementation sources and exclude main entry point
get_file_range(ISPA_MODULE_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/external/cpuf/*.cpp
)
get_file_range_non_recurse(ISPA_MODULE_INTERFACES ${CMAKE_CURRENT_SOURCE_DIR}/lib/converter/*.cppm)
get_file_range_non_recurse(ISPA_MODULE_INTERFACES ${CMAKE_CURRENT_SOURCE_DIR}/lib/algorithm/*.cppm)
target_sources(ispacore
        PUBLIC
            FILE_SET cxx_modules TYPE CXX_MODULES FILES ${ISPA_MODULE_INTERFACES}
        PRIVATE
            ${ISPA_MODULE_SOURCES}
)

target_include_directories(ispacore PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/parser/stdlibs/new
        ${CMAKE_CURRENT_SOURCE_DIR}/external
        ${CMAKE_CURRENT_SOURCE_DIR}/parser/python/bootstrapped
)

target_link_libraries(ispacore PUBLIC Boost::boost CLI11::CLI11 ISPA::Parser ISPA::DepScanner)
if(NOT EMSCRIPTEN)
    target_link_libraries(ispacore PUBLIC ICU::uc)
endif()
set_target_properties(ispacore PROPERTIES
        DISABLE_PRECOMPILE_HEADERS ON
)
# =============================================
# CLI TOOLS
# =============================================
function(add_cli dir)
    if(ARGN)
        set(name ${ARGV0})
    else()
        set(name ${dir})
    endif()
    if (NOT ${name} STREQUAL "ispa")
        set(name "ispa-${name}")
    endif()
    get_file_range(${name}_SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/cli/${dir}/*.cpp"
    )
    add_executable(${name} ${${name}_SOURCES})
    target_link_libraries(${name} PRIVATE ispacore)
endfunction()
add_cli(ispa)
target_link_libraries(ispa PRIVATE yaml-cpp::yaml-cpp)
# Ship the standard library beside the binary so `import std.*` works without ispa-pkg.
add_custom_command(TARGET ispa POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/parser/stdlibs/grammar $<TARGET_FILE_DIR:ispa>/libraries/std/ispalib/grammar
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/parser/stdlibs/python $<TARGET_FILE_DIR:ispa>/libraries/std/ispalib/python
        COMMAND ${CMAKE_COMMAND} -E env --unset=LD_LIBRARY_PATH
                $<TARGET_FILE:ispa> --precompile $<TARGET_FILE_DIR:ispa>/libraries/std/ispalib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/parser/stdlibs/grammar
        DESTINATION share/ispa/libraries/std/ispalib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/parser/stdlibs/python
        DESTINATION share/ispa/libraries/std/ispalib)
# =============================================
# ISPA ALGORITHMS/CONVERTERS FUNCTIONS
# =============================================

function(add_ispa_lib type lib)
    get_file_range(${type}_${lib}_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/lib/${type}/${lib}/*.cpp
    )
    get_file_range(${type}_${lib}_MODULES
            ${CMAKE_CURRENT_SOURCE_DIR}/lib/${type}/${lib}/*.cppm
    )

    set(target_name "ispa-${type}-${lib}")

    if(EMSCRIPTEN)
        # No dlopen in the browser: plugins are linked into ispa and src/wasm_plugins.cpp
        # answers the loader's dlopen/dlsym calls.
        add_library(${target_name} STATIC)
    else()
        add_library(${target_name} SHARED)
    endif()

    target_sources(${target_name}
            PUBLIC
            FILE_SET cxx_modules TYPE CXX_MODULES FILES ${${type}_${lib}_MODULES}
            PRIVATE
            ${${type}_${lib}_SOURCES}
    )

    # Link language target to shared converter modules and ispa_modules
    target_link_libraries(${target_name}
            PRIVATE
            ispacore
    )

    set_target_properties(${target_name} PROPERTIES
            WINDOWS_EXPORT_ALL_SYMBOLS ON
            DISABLE_PRECOMPILE_HEADERS ON
    )

    # Algorithms are loaded through their descriptor, never linked into the host.
    if(EMSCRIPTEN)
        target_link_libraries(ispa PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,${target_name}>")
    elseif(type STREQUAL "algorithm")
        add_dependencies(ispa ${target_name})
    else()
        target_link_libraries(ispa PRIVATE ${target_name})
    endif()
endfunction()

# =============================================
# Converter Shared Library
# =============================================
function(add_converter language)
    add_ispa_lib(converter ${language})
endfunction()

if(EMSCRIPTEN)
    add_converter(python)
else()
    add_converter(cpp)
    add_converter(python)
    add_converter(rust)
endif()
# =============================================
# Algorithms Shared Library
# =============================================
function(add_algorithm alg)
    add_ispa_lib(algorithm ${alg})

    if(ARGN)
        foreach(dep IN LISTS ARGN)
            target_link_libraries(ispa-algorithm-${alg} PRIVATE ispa-algorithm-${dep})
        endforeach()
    endif()
endfunction()

add_algorithm(LL)
if(NOT EMSCRIPTEN)
    add_algorithm(LR)
    add_algorithm(LALR LR)
    add_algorithm(LR_STAR LR)
endif()
if(EMSCRIPTEN)
    target_link_options(ispa PRIVATE
        "-Wl,--wrap=dlopen" "-Wl,--wrap=dlsym" "-Wl,--wrap=dlclose"
        "-sALLOW_MEMORY_GROWTH=1" "-sSTACK_SIZE=5MB"
        "$<$<BOOL:${ISPA_WASM_SIZE_OPT}>:-Oz>"
        "-sMODULARIZE=1" "-sEXPORT_ES6=1" "-sEXIT_RUNTIME=0"
        "-sEXPORTED_RUNTIME_METHODS=['FS','callMain']")
    set_target_properties(ispa PROPERTIES SUFFIX ".mjs")
    return()
endif()
# =============================================
# Testing
# =============================================
enable_testing()
find_package(GTest REQUIRED)

get_file_range(TEST_SOURCE_FILES EXCLUDE "tests/compile/" "tests/*.cpp")
get_file_range(TEST_MODULE_FILES EXCLUDE "tests/compile/" "tests/*.cppm")
list(REMOVE_ITEM TEST_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/tests/Diagnostics.cpp")

add_executable(diagnostics_tests tests/Diagnostics.cpp)
target_link_libraries(diagnostics_tests PRIVATE ispacore GTest::gtest_main)

add_executable(tests ${TEST_SOURCE_FILES})
add_dependencies(tests ispa)
target_sources(tests
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        ${TEST_MODULE_FILES}
)

target_include_directories(tests PRIVATE
        "tests/lib"
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(tests PRIVATE
        ROOT_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        TEST_ROOT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests"
        BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

target_link_libraries(tests
        PRIVATE
        ispacore
        ispa-algorithm-LR
        ispa-algorithm-LALR
        ispa-algorithm-LR_STAR
        GTest::gtest
        GTest::gtest_main
        yaml-cpp::yaml-cpp
)

include(GoogleTest)
gtest_discover_tests(diagnostics_tests)
gtest_discover_tests(tests)
