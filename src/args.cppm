export module args;
import CLI;
import LangAPI;
import dstd;
import std;
import cpuf.printf;
import cpuf.op;
export struct Args {
    enum class Algorithm {
        LL, LR, LALR, LR_STAR
    };
    using Language = LangAPI::Language;
    Algorithm algorithm = Algorithm::LL;
    Language language = Language::Cpp;
    std::string language_str;
    std::vector<std::string> dir;
    // Internal: managed package directories discovered beside this installation.
    std::vector<std::string> library_dirs;
    std::string executable_dir;
    std::vector<std::string> files;
    std::vector<std::pair<std::string, std::string>> dependencies;
    bool no_dep = false;
    // Internal: keep every exported module when generating a dependency.
    bool dependency_build = false;
    bool debug = false;
    bool logs = false;
    bool version = false;
    bool nfa_debug = false;
    std::string output;
    std::unordered_set<std::string> dump;
    std::string dump_dir;
    bool dump_all = false;
    bool dump_nfa_from_rule = false;
    bool incremental = false;
    bool auto_conflicts = true;
    std::size_t conflict_lookahead = 3;
};
export Args parse_args(int argc, char** argv);
export auto to_string(Args::Algorithm algorithm) -> std::string;
export auto to_string(Args::Language language) -> std::string;
