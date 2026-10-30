# Regenerating the self-hosted bootstrapped parsers (parser/ispa, parser/python, parser/ispa_dep_scanner)
# and target-cpp/ispa_core_lowered.{h,cpp} needs an `ispa` binary: either one already on PATH / given via
# -DISPA_BOOTSTRAP_BIN=..., or the latest release downloaded here so a clean checkout never needs to ship
# those generated multi-MB sources in git.
#
# Opt-in: -DISPA_REGENERATE_BOOTSTRAP=ON. Off by default, so a normal configure just uses the checked-in
# (or previously regenerated) bootstrapped/ sources, same as before this file existed.

option(ISPA_REGENERATE_BOOTSTRAP "Download/locate an ispa binary and regenerate the bootstrapped parsers + target-cpp lowered sources" OFF)
set(ISPA_BOOTSTRAP_BIN "" CACHE FILEPATH "Path to an ispa binary to use for regeneration (skips the download)")
set(ISPA_BOOTSTRAP_VERSION "latest" CACHE STRING "Release tag to download when ISPA_BOOTSTRAP_BIN is not set (e.g. v1.2.3)")

function(_ispa_bootstrap_platform out_var)
    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)$")
        set(${out_var} "ispa-linux-x64" PARENT_SCOPE)
    elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin" AND CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$")
        set(${out_var} "ispa-macos-arm64" PARENT_SCOPE)
    elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
        set(${out_var} "ispa-macos-x64" PARENT_SCOPE)
    else()
        set(${out_var} "" PARENT_SCOPE)
    endif()
endfunction()

# Resolves to a usable ispa binary path in ${ISPA_BOOTSTRAP_RESOLVED_BIN} (cache), downloading it on first
# use. Safe to call multiple times (add_subdirectory etc.): only downloads once per build tree.
function(ispa_bootstrap_resolve_binary out_var)
    if(ISPA_BOOTSTRAP_BIN)
        set(${out_var} "${ISPA_BOOTSTRAP_BIN}" PARENT_SCOPE)
        return()
    endif()
    if(ISPA_BOOTSTRAP_RESOLVED_BIN AND EXISTS "${ISPA_BOOTSTRAP_RESOLVED_BIN}")
        set(${out_var} "${ISPA_BOOTSTRAP_RESOLVED_BIN}" PARENT_SCOPE)
        return()
    endif()
    find_program(ISPA_SYSTEM_BIN ispa)
    if(ISPA_SYSTEM_BIN)
        set(ISPA_BOOTSTRAP_RESOLVED_BIN "${ISPA_SYSTEM_BIN}" CACHE INTERNAL "" FORCE)
        set(${out_var} "${ISPA_SYSTEM_BIN}" PARENT_SCOPE)
        return()
    endif()

    _ispa_bootstrap_platform(platform)
    if(NOT platform)
        message(FATAL_ERROR "ispa bootstrap: no ispa on PATH and no prebuilt release for "
                "${CMAKE_HOST_SYSTEM_NAME}/${CMAKE_HOST_SYSTEM_PROCESSOR}; set -DISPA_BOOTSTRAP_BIN=/path/to/ispa")
    endif()

    set(bootstrap_dir "${CMAKE_BINARY_DIR}/ispa-bootstrap")
    set(archive "${bootstrap_dir}/${platform}.tar.gz")
    set(unpack_dir "${bootstrap_dir}/unpacked")
    file(MAKE_DIRECTORY "${bootstrap_dir}")

    # GitHub's /releases/latest/download/<asset> and /releases/download/<tag>/<asset> URLs are stable
    # redirects; no release-metadata JSON needs to be fetched or parsed to resolve "latest".
    if(ISPA_BOOTSTRAP_VERSION STREQUAL "latest")
        set(download_url "https://github.com/Sinfolke/ispa-parser-generator/releases/latest/download/${platform}.tar.gz")
    else()
        set(download_url "https://github.com/Sinfolke/ispa-parser-generator/releases/download/${ISPA_BOOTSTRAP_VERSION}/${platform}.tar.gz")
    endif()

    if(NOT EXISTS "${archive}")
        message(STATUS "ispa bootstrap: downloading ${download_url}")
        file(DOWNLOAD "${download_url}" "${archive}" STATUS download_status TLS_VERIFY ON)
        list(GET download_status 0 download_code)
        if(NOT download_code EQUAL 0)
            file(REMOVE "${archive}")
            list(GET download_status 1 download_message)
            message(FATAL_ERROR "ispa bootstrap: download failed (${download_message}); "
                    "set -DISPA_BOOTSTRAP_BIN=/path/to/ispa to skip downloading")
        endif()
    endif()

    file(MAKE_DIRECTORY "${unpack_dir}")
    file(ARCHIVE_EXTRACT INPUT "${archive}" DESTINATION "${unpack_dir}")

    set(resolved_bin "${unpack_dir}/ispa")
    if(NOT EXISTS "${resolved_bin}")
        message(FATAL_ERROR "ispa bootstrap: archive did not contain an 'ispa' binary at its root (${unpack_dir})")
    endif()
    if(NOT WIN32)
        file(CHMOD "${resolved_bin}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif()

    set(ISPA_BOOTSTRAP_RESOLVED_BIN "${resolved_bin}" CACHE INTERNAL "" FORCE)
    set(${out_var} "${resolved_bin}" PARENT_SCOPE)
endfunction()

# Runs regen.py (target-cpp/ispa_core_lowered.{h,cpp}) and then regenerates the three bootstrapped
# parsers in dependency order (python's PyRawParser first; the self-parser and dep_scanner both --dep on it).
# Each step only overwrites what it generates; a failure stops the configure (FATAL_ERROR) since a half
# regenerated bootstrap tree is worse than the previous one.
function(ispa_bootstrap_regenerate)
    if(NOT ISPA_REGENERATE_BOOTSTRAP)
        return()
    endif()
    set(repo "${CMAKE_SOURCE_DIR}")

    # Each piece below is regenerated only if it is missing from the checkout (e.g. a shallow/sparse
    # clone, or one that deliberately gitignores it). Anything already committed/present is left alone:
    # this is also what breaks the chicken-and-egg problem (building ispa needs these, but regenerating
    # them needs a working ispa) for the normal case where they are just checked in.
    if(EXISTS "${repo}/parser/stdlibs/target/target-cpp/ispa_core_lowered.h"
            AND EXISTS "${repo}/parser/stdlibs/target/target-cpp/src/ispa_core_lowered.cpp"
            AND EXISTS "${repo}/parser/python/bootstrapped/PyRawParser.h"
            AND EXISTS "${repo}/parser/python/bootstrapped/PyRawParser.cpp"
            AND EXISTS "${repo}/parser/ispa/bootstrapped/Parser.h"
            AND EXISTS "${repo}/parser/ispa/bootstrapped/Parser.cpp"
            AND EXISTS "${repo}/parser/ispa_dep_scanner/bootstrapped/DepParser.h"
            AND EXISTS "${repo}/parser/ispa_dep_scanner/bootstrapped/DepParser.cpp")
        message(STATUS "ispa bootstrap: all bootstrapped sources already present, nothing to regenerate")
        return()
    endif()

    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    ispa_bootstrap_resolve_binary(ispa_bin)
    message(STATUS "ispa bootstrap: regenerating with ${ispa_bin}")

    set(env_cmd "${CMAKE_COMMAND}" -E env --unset=LD_LIBRARY_PATH)

    # 1) target-cpp: the ispa_std runtime lowered to C++.
    if(NOT EXISTS "${repo}/parser/stdlibs/target/target-cpp/ispa_core_lowered.h"
            OR NOT EXISTS "${repo}/parser/stdlibs/target/target-cpp/src/ispa_core_lowered.cpp")
        execute_process(
                COMMAND ${env_cmd} "${Python3_EXECUTABLE}" "${repo}/parser/stdlibs/target/target-cpp/regen.py" --ispa "${ispa_bin}"
                RESULT_VARIABLE r
        )
        if(NOT r EQUAL 0)
            message(FATAL_ERROR "ispa bootstrap: regen.py (target-cpp) failed")
        endif()
    endif()

    # 2) python/PyRawParser: other bootstrapped parsers --dep on it.
    if(NOT EXISTS "${repo}/parser/python/bootstrapped/PyRawParser.h"
            OR NOT EXISTS "${repo}/parser/python/bootstrapped/PyRawParser.cpp")
        execute_process(
                COMMAND ${env_cmd} "${Python3_EXECUTABLE}" "${repo}/parser/python/generate.py" "${ispa_bin}"
                        --output "${repo}/parser/python/bootstrapped"
                RESULT_VARIABLE r
        )
        if(NOT r EQUAL 0)
            message(FATAL_ERROR "ispa bootstrap: generate.py (PyRawParser) failed")
        endif()
    endif()

    # 3) the self-hosted grammar parser (parser/ispa).
    if(NOT EXISTS "${repo}/parser/ispa/bootstrapped/Parser.h"
            OR NOT EXISTS "${repo}/parser/ispa/bootstrapped/Parser.cpp")
        execute_process(
                COMMAND ${env_cmd} "${ispa_bin}" -l cpp -a LL --debug false --no-nfa-debug
                        -o "${repo}/parser/ispa/bootstrapped/Parser"
                        --dir "${repo}/parser/ispa/grammar"
                        --dep python "${repo}/parser/python/bootstrapped/PyRawParser"
                        "${repo}/parser/ispa/entry.isc"
                        --ai-recovery "${repo}/parser/ispa/ai.json"
                RESULT_VARIABLE r
        )
        if(NOT r EQUAL 0)
            message(FATAL_ERROR "ispa bootstrap: regenerating parser/ispa/bootstrapped/Parser failed")
        endif()
    endif()

    # 4) the dependency-scanner grammar (parser/ispa_dep_scanner), per its README.
    if(NOT EXISTS "${repo}/parser/ispa_dep_scanner/bootstrapped/DepParser.h"
            OR NOT EXISTS "${repo}/parser/ispa_dep_scanner/bootstrapped/DepParser.cpp")
    execute_process(
            COMMAND ${env_cmd} "${ispa_bin}" -l cpp -a LL --debug false --no-nfa-debug
                    -o "${repo}/parser/ispa_dep_scanner/bootstrapped/DepParser"
                    --dir "${repo}/parser/ispa_dep_scanner/grammar" --dir "${repo}/parser/ispa/grammar"
                    --dep python "${repo}/parser/python/bootstrapped/PyRawParser"
                    "${repo}/parser/ispa_dep_scanner/entry.isc"
                    "--ai-recovery ${repo}/parser/ispa_dep_scanner/ai.json"
            RESULT_VARIABLE r
    )
    if(NOT r EQUAL 0)
        message(FATAL_ERROR "ispa bootstrap: regenerating parser/ispa_dep_scanner/bootstrapped/DepParser failed")
    endif()
    endif()
endfunction()
