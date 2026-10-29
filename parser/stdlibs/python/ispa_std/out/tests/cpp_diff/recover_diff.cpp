// Differential harness for Recovery::recover over a vector-backed cursor.
#include <ispastdlib/ispa_recovery.hpp>
#include <ispastdlib/raw_token.hpp>
#include <iostream>
#include <sstream>
namespace ISPA_STD { std::string capture_stacktrace() { return ""; } }
using namespace ISPA_STD;
using Tok = RawToken<int>;
struct It {
    const std::vector<Tok>* v; size_t i;
    bool isEnd() const { return i >= v->size(); }
    const Tok& operator*() const { return (*v)[i]; }
    It& operator++() { ++i; return *this; }
};
static std::vector<size_t> readv(std::istringstream& in) {
    int n; in >> n; std::vector<size_t> out;
    for (int i = 0; i < n; ++i) { long long x; in >> x; out.push_back(x < 0 ? Recovery::eof_token : (size_t)x); }
    return out;
}
int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        int ntok; in >> ntok;
        std::vector<Tok> toks;
        for (int i = 0; i < ntok; ++i) {
            long long kind; in >> kind;
            toks.push_back(Tok(RawTokenMeta(i * 2, nullptr, nullptr, 1, 1, 1, (size_t)kind), 0));
        }
        Recovery::Plan plan;
        plan.rule = "r";
        plan.expected = readv(in);
        plan.continuation = readv(in);
        plan.follow = readv(in);
        plan.explicit_sync = readv(in);
        int ai, ad, ar, ms; in >> ai >> ad >> ar >> ms;
        plan.allow_insertion = ai; plan.allow_deletion = ad; plan.allow_replacement = ar; plan.max_skip = ms;
        Recovery::State st;
        int nanch; in >> nanch;
        for (int i = 0; i < nanch; ++i) st.anchors.push_back(readv(in));
        int errs, skipped, maxerr, maxskip; in >> errs >> skipped >> maxerr >> maxskip;
        for (int i = 0; i < errs; ++i) st.diagnostics.emplace_back();
        st.total_skipped = skipped; st.max_errors = maxerr; st.max_total_skipped = maxskip;
        It it{&toks, 0};
        auto d = Recovery::recover(it, st, plan);
        std::cout << (int)d.mode << " " << (long long)(d.synthesized == Recovery::eof_token ? -1 : (long long)d.synthesized)
                  << " " << (long long)(d.unexpected == Recovery::eof_token ? -1 : (long long)d.unexpected)
                  << " " << d.skipped << " " << it.i << " " << st.total_skipped << " " << st.diagnostics.size() << "\n";
    }
}
