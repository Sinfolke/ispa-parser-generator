export module DFA.functionality;
import LangAPI;
import NFA;
import DFA.API;
import DFA;
import AST.Tree;
import dstd;
import std;
export namespace DFA {
    void mergeTwoNFA(NFA &first, NFA &second);
    auto mergeNFAS(const stdu::vector<NFA> &nfas) -> std::pair<NFA, std::size_t>;
    auto buildTokenDFA(const AST::Tree &ast, const NFA &nfa) -> DFA;
    auto build(const AST::Tree &ast, NFA &nfa) -> std::tuple<ClassifiedDFA, stdu::vector<NFA::ActionState>, stdu::vector<NFA::SemanticState>, std::size_t>;
    auto build(const AST::Tree &ast, const stdu::vector<NFA> &nfa_collection) -> std::tuple<ClassifiedDFA, stdu::vector<NFA::ActionState>, stdu::vector<NFA::SemanticState>, std::size_t>;
}