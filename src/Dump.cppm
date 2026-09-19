export module Dump;
import args;
import std;
class Dump {
    const Args *args = nullptr;
    std::filesystem::path path = "";
public:
    Dump(const Args *args, const std::filesystem::path &path) : path(path), args(args) {}
    Dump() = default;
    bool shouldDump(const std::string &s) const {
        return args->dump_all || args->dump.contains(s);
    }
    auto makeDumpPath(const std::string &s) const {
        return args->dump_dir + "/" + s + ".txt";
    }
    bool isNfaDebug() const {
        return args->nfa_debug;
    }
    void initDumpDirectory() const;
    void setArgsPointer(const Args *args) {
        this->args = args;
    }
    void setDumpDirectory(const std::filesystem::path &path) {
        this->path = path;
    }
};
export extern Dump dumper;
