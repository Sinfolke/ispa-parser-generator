#pragma once
// ISPA_CORE starts as the original ispastdlib; pieces are replaced by core-backed implementations one by one,
// through customization points of the original headers.
namespace ISPA_CORE {
template<typename Token, typename... Arguments>
Token core_dfa_run(const char*& pos, long long start_pos, long long& scanning_line, Arguments&&... rest);
}
#define ISPA_STDLIB_DFA_RUN ::ISPA_CORE::core_dfa_run
#include <ispastdlib.hpp>
namespace ISPA_CORE { using namespace ISPA_STD; }
