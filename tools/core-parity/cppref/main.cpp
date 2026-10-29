#include "json.h"
#include "dump.hpp"
#include <algorithm>
#include <atomic>
#include <execinfo.h>
#include <cxxabi.h>
#include <map>
#include <string>
#include <vector>
#include <cstdlib>
#include <new>
static std::atomic<unsigned long long> g_allocs{0}, g_bytes{0};
static unsigned long long g_size_hist[2049];
static bool g_sampling = false; static std::map<std::string, unsigned long long>* g_stacks = nullptr; static thread_local bool t_in_sample = false;
static void sample_stack() {
    if (t_in_sample) return; t_in_sample = true;
    void* frames[14]; int n = backtrace(frames, 14); char** syms = backtrace_symbols(frames, n);
    std::string key;
    int kept = 0;
    for (int i = 1; i < n && kept < 4; ++i) {
        std::string line = syms[i]; auto open = line.find('('); auto plus = line.find('+', open);
        std::string name = open != std::string::npos && plus != std::string::npos ? line.substr(open + 1, plus - open - 1) : line;
        int status = 0; char* dem = abi::__cxa_demangle(name.c_str(), nullptr, nullptr, &status);
        std::string pretty = dem ? dem : name; std::free(dem);
        if (pretty.rfind("std::", 0) == 0 || pretty.find("operator new") != std::string::npos || pretty.find("__gnu_cxx") != std::string::npos) continue;
        if (pretty.size() > 110) pretty = pretty.substr(0, 110);
        key += " <- " + pretty; ++kept;
    }
    std::free(syms);
    if (!g_stacks) g_stacks = new std::map<std::string, unsigned long long>();
    ++(*g_stacks)[key]; t_in_sample = false;
}
void* operator new(std::size_t n) { if (g_sampling && (g_allocs % 256) == 0) sample_stack(); ++g_size_hist[n > 2048 ? 2048 : n]; ++g_allocs; g_bytes += n; if (void* p = std::malloc(n ? n : 1)) return p; throw std::bad_alloc(); }
void* operator new[](std::size_t n) { ++g_allocs; g_bytes += n; if (void* p = std::malloc(n ? n : 1)) return p; throw std::bad_alloc(); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
struct SampleInit { SampleInit() { g_sampling = std::getenv("ALLOC_SAMPLE") != nullptr; } } g_sample_init;
struct AllocReport { ~AllocReport() { if (std::getenv("ALLOC_REPORT")) { std::fprintf(stderr, "ALLOCS %llu BYTES %llu\n", g_allocs.load(), g_bytes.load()); if (g_stacks) { std::vector<std::pair<unsigned long long, std::string>> rows; for (auto& [k, v] : *g_stacks) rows.emplace_back(v, k); std::sort(rows.rbegin(), rows.rend()); for (std::size_t i = 0; i < rows.size() && i < 14; ++i) std::fprintf(stderr, "STACK %llu%s\n", rows[i].first, rows[i].second.c_str()); }
        if (std::getenv("ALLOC_HIST")) for (int i = 0; i <= 2048; ++i) if (g_size_hist[i] > 20000) std::fprintf(stderr, "SIZE %d x %llu\n", i, g_size_hist[i]); } } } g_alloc_report;
#include <sstream>
#include <iostream>
#include <sstream>
int main(int argc, char** argv) {
    const bool lazy = getenv("LAZY") != nullptr;
    for (int i = 1; i < argc; ++i) {
        json::Parser parser;
        json::Lexer lexer;
        lexer.setInput(argv[i]);
        std::string outcome;
        try {
            if (!lazy) lexer.makeTokens();
            auto& tree = lazy ? parser.parse(argv[i]) : parser.parse(lexer);
            std::ostringstream os;
            dump(os, tree);
            outcome = os.str();
        } catch (const std::exception& e) { outcome = std::string("exception: ") + e.what(); }
        std::cout << "INPUT " << (i - 1) << "\n" << outcome << "\n";
        for (const auto& d : parser.getErrors()) std::cout << "ERR " << d.rule << " | " << d.message << "\n";
    }
}
