export module args;
import CLI;
import LangAPI;
import dstd;
import std;
import cpuf.printf;
export struct Args {
    enum class Algorithm {
        LL, LR0, LR1, LALR, ELR
    };
    Algorithm algorithm = Algorithm::LL;
    LangAPI::Language language;
    std::string language_str;
    std::vector<std::string> dir;
    std::vector<std::string> files;
    // debug set to true by default in debug builds
#ifdef DEBUG
    bool debug = true;
#else
    bool debug = false;
#endif
    bool version = false;
    // Toggles debug provenance tracking (TokenID / CharOrigin) inside the
    // TNFA. On by default; turn off to build a leaner TNFA whose transitions
    // carry no source-mapping information. The DFA compares this provenance
    // unconditionally, so with it disabled every target simply carries an
    // empty trail and the comparison degenerates cleanly.
    bool nfa_debug = true;
    std::string output;
    std::unordered_set<std::string> dump;
    std::string dump_dir;
    bool dump_all = false;
    bool dump_nfa_from_rule = false;

};
export Args parse_args(int argc, char** argv);