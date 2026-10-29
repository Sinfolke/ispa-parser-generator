/**
 * ispastdlib/platform.hpp - OS headers for capture_stacktrace().
 * Include it ONLY in the translation unit that defines ISPA_STD::capture_stacktrace().
 * (windows.h in every generated file is expensive and pollutes macros.)
 */
#pragma once
#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#else
#include <execinfo.h>
#include <cstdlib>
#endif
