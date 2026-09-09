cmake_minimum_required(VERSION 4.0 FATAL_ERROR)
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")
set(CMAKE_CXX_MODULE_STD ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-stdlib=libc++)
    add_link_options(-stdlib=libc++)
endif()

project(ISPA LANGUAGES CXX)

# =============================================
# Compile Definitions
# =============================================
add_compile_definitions(
        __SOURCE_ROOT__="${CMAKE_CURRENT_SOURCE_DIR}"
        PROGRAM_VERSION="0.1.0 beta"
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

# =============================================
# Fetch & Find Dependencies
# =============================================
include(FetchContent)
FetchContent_Declare(
        RopeString
        GIT_REPOSITORY https://github.com/Sinfolke/Rope-String
        GIT_TAG main
)
FetchContent_MakeAvailable(RopeString)

find_package(Boost REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)

# =============================================
# ISPA Modules Library
# =============================================
add_library(ispa_modules STATIC)
set_target_properties(ispa_modules PROPERTIES POSITION_INDEPENDENT_CODE ON)

get_file_range(ISPA_MODULE_INTERFACES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cppm
        ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/*.cppm
        ${CMAKE_CURRENT_SOURCE_DIR}/parser/*.cppm
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.cppm
        ${CMAKE_CURRENT_SOURCE_DIR}/converters/C++2/*.cppm
        ${CMAKE_CURRENT_SOURCE_DIR}/converters/*.cppm
        ${CMAKE_CURRENT_SOURCE_DIR}/external/*.cppm
)

# Collect implementation sources and exclude main entry point
get_file_range(ISPA_MODULE_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/parser/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/external/cpuf/*.cpp
        EXCLUDE ".*/main\\.cpp$"
)

target_sources(ispa_modules
        PUBLIC
            FILE_SET cxx_modules TYPE CXX_MODULES FILES ${ISPA_MODULE_INTERFACES}
        PRIVATE
            ${ISPA_MODULE_SOURCES}
)

target_include_directories(ispa_modules PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/parser
        ${CMAKE_CURRENT_SOURCE_DIR}/parser/stdlibs
        ${CMAKE_CURRENT_SOURCE_DIR}/external
        ${CMAKE_CURRENT_SOURCE_DIR}/converters
)

target_link_libraries(ispa_modules PUBLIC Boost::boost Rope CLI11::CLI11)

# =============================================
# Converter Shared Library
# =============================================
get_file_range(CPP_CONVERTER_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/converters/C++2/*.cpp
)

add_library(ispa-converter-cpp SHARED ${CPP_CONVERTER_SOURCES})
target_link_libraries(ispa-converter-cpp PRIVATE ispa_modules)
target_include_directories(ispa-converter-cpp PRIVATE converters/C++)
set_target_properties(ispa-converter-cpp PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

# =============================================
# Main Executable
# =============================================
add_executable(ispa ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(ispa PRIVATE ispa_modules ispa-converter-cpp)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_link_libraries(ispa PRIVATE c++ c++abi m)
endif()

# =============================================
# Testing
# =============================================
enable_testing()
find_package(GTest REQUIRED)

get_file_range(TEST_SOURCE_FILES EXCLUDE "tests/compile/" "tests/*.cpp")
get_file_range(TEST_MODULE_FILES EXCLUDE "tests/compile/" "tests/*.cppm")

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
        ispa_modules
        GTest::gtest
        GTest::gtest_main
        yaml-cpp::yaml-cpp
)

include(GoogleTest)
gtest_discover_tests(tests)