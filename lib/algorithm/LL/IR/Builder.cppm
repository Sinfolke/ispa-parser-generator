export module LLIR.Builder;
import LangAPI;
import ParserIR.API;
import ParserIR.IR;
import AST.Tree;
import AST.API;
import AST.types;
import DFA;
import dstd;
import std;

namespace LLIR {
using namespace ParserIR;
    export class Builder {
        stdu::vector<Production> data;
        stdu::vector<DFA::DFA> dfas;
    public:
        Builder(AST::Tree &tree, int tokensOnly = -1);
        Builder(
            AST::Tree &tree,
            const AST::TreeMap &rules_to_build,
            const std::unordered_map<std::string, Production> &cache,
            const std::function<void(std::size_t, std::size_t)>& progress = {}
        );
        auto get() -> IR;
        auto getRawData() -> stdu::vector<Production>&;
        auto getRawData() const -> const stdu::vector<Production>&;
    };
}