module;
#include <tsl/ordered_map.h>
export module LexerBuilder;
import LangAPI;
import AST.API;
import AST.Tree;
import LangAPI;
import LLIR.API;
import LLIR.IR;
import AST.Tree;
import NFA.TNFA.API;
import DFA;
import hash;
import dstd;
import std;
export class LexerBuilder {
public:
    static constexpr auto DFA_NOT_COMPATIBLE = std::numeric_limits<std::size_t>::max();
    using DfaCompatibleTable = utype::unordered_map<stdu::vector<std::string>, std::size_t>;
    using NameToDfaMap = utype::unordered_map<stdu::vector<std::string>, std::size_t>;
    using DispatchNamesInvolve = tsl::ordered_map<stdu::vector<stdu::vector<std::string>>, NameToDfaMap, uhash>;


private:
    AST::Tree &ast;
    DFA::ClassifiedDFA dfa;
    LLIR::IR function_ir;
    DfaCompatibleTable dfa_compatible_table;
    DispatchNamesInvolve dispatch_names_involve;
    NameToDfaMap name_to_dfa;
    std::vector<std::size_t> token_type;
    std::size_t highest_states_count = 0;
    std::size_t highest_transition_count = 0;
    stdu::vector<NFA::TNFA::ActionState> action_table;
    stdu::vector<NFA::TNFA::SemanticState> semantic_table;
    std::size_t max_registers_count = 0;
    bool isTopLevel(const stdu::vector<std::string> &name);
public:
    LexerBuilder(AST::Tree &ast) : ast(ast) {};
    void build();
    auto& getDFA() { return dfa; }
    auto& getDFA() const { return dfa; }
    auto& getActionTable() { return action_table; }
    auto& getActionTable() const { return action_table; }
    auto& getSemanticTable() { return semantic_table; }
    auto& getSemanticTable() const { return semantic_table; }
    auto& getDfaCompatibleTable() { return dfa_compatible_table; }
    auto& getDispatchNamesInvolve() { return dispatch_names_involve; }
    auto& getFunctionsIR() { return function_ir; }
    auto& getDfaCompatibleTable() const { return dfa_compatible_table; }
    auto& getDispatchNamesInvolve() const { return dispatch_names_involve; }
    auto& getNameToDFAIndex() const { return name_to_dfa; }
    auto& getFunctionsIR() const  { return function_ir; }
    auto& getMaxStatesCount() const{ return highest_states_count; }
    auto& getMaxTransitionCount() const { return highest_transition_count; }
    auto& getMaxRegistersCount() const { return max_registers_count; }
    auto getDataBlocks() const -> LLIR::DataBlockList;
};