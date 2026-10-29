// Differential harness: runs the ORIGINAL C++ DFA::run on a hand-built TDFA.
#include <ispastdlib/dfa_run.hpp>
#include <ispastdlib/dfa_api.hpp>
namespace ISPA_STD { std::string capture_stacktrace() { return ""; } }
using namespace ISPA_STD;
using namespace ISPA_STD::DFA::API;

static std::string unhex(const std::string& h) {
    std::string out;
    for (size_t i = 0; i + 1 < h.size(); i += 2) out += (char)std::stoi(h.substr(i, 2), nullptr, 16);
    return out;
}
int main() {
    constexpr auto N = null_state;
    // classes: 0 other, 1 letter, 2 digit, 3 space, 4 '+'
    CharToClass cls{};
    for (int c = 0; c < 256; ++c) cls[c] = 0;
    for (int c = 'a'; c <= 'z'; ++c) cls[c] = 1;
    for (int c = '0'; c <= '9'; ++c) cls[c] = 2;
    cls[' '] = 3; cls['\n'] = 3; cls['+'] = 4;
    // 5 table states, 3 action rows (5..7), semantic states from 8
    Table<5, 5> table{{
        {N, 5, 7, 3, 4},      // S0
        {6, 1, 1, 6, 6},      // S1 identifier body
        {9, 9, 2, 9, 9},      // S2 number body
        {10, 10, 10, 3, 10},  // S3 whitespace run
        {11, 11, 11, 11, 11}, // S4 after '+'
    }};
    LRTable<3> actions{{
        {static_cast<size_t>(Action::SET), 0, N, 1},
        {static_cast<size_t>(Action::SET), 1, N, 8},
        {static_cast<size_t>(Action::SET), 0, N, 2},
    }};
    auto semantic = [](size_t idx, const Captures& c, long long start_pos, const char*, long long len, long long) {
        std::string t;
        switch (idx) {
            case 0: t = "ID:" + c.text(0) + "@" + std::to_string(start_pos) + "+" + std::to_string(len); break;
            case 1: t = "NUM@" + std::to_string(start_pos) + "+" + std::to_string(len); break;
            case 2: t = ""; break;
            case 3: t = "PLUS"; break;
        }
        return std::make_tuple(null_state, t);
    };
    static const DFADebugTable<0> no_debug{};
    static const DebugIndex no_index{};
    std::string line;
    while (std::getline(std::cin, line)) {
        std::string src = unhex(line);
        const char* p = src.c_str();
        long long ln = 1;
        std::string out;
        while (*p) {
            const char* before = p;
            std::string tok = DFA::run<std::string>(p, p - src.c_str(), ln, table, cls, actions, semantic,
                                                    no_debug, no_index, TdfaLayout<3, 2>{});
            out += "[" + tok + "|" + std::to_string(p - src.c_str()) + "|" + std::to_string(ln) + "]";
            if (p == before) { out += "STUCK"; break; }
        }
        std::cout << out << "\n";
    }
}
