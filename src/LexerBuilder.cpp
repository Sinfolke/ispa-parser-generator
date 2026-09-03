module LexerBuilder;
import corelib;
import LLIR.Builder;
import LLIR.Builder.Base;
import LLIR.Builder.Data;
import LLIR.Builder.DataWrapper;
import LLIR.Rule.MemberBuilder;
import LLIR.RuleBuilder;
import LLIR.CllBuilder;
import NFA;
import DFA.API;
import DFA.functionality;
import DFA;
import Dump;
import logging;
import constants;
import cpuf.printf;
import std;

void accumulateNestedNames(stdu::vector<std::shared_ptr<AST::RuleMember>> members, stdu::vector<stdu::vector<std::string>> &names) {
    for (const auto &mem_ptr : members) {
        auto &mem = *mem_ptr;
        if (mem.isGroup())
            accumulateNestedNames(mem.getGroup().values, names);
        if (mem.isOp())
            accumulateNestedNames(mem.getOp().options, names);
        if (mem.isName())
            names.push_back(mem.getName().name);
    }
}
void LexerBuilder::build() {
    stdu::vector<NFA> nfas;
    std::size_t accept_index = 0;
    for (const auto &[name, rule] : ast) {
        if (corelib::text::isLower(name.back())) {
            continue;
        }
        // token here
        NFA nfa(ast, name, &rule.data_block, rule.rule_members, name == constants::whitespace, true, &accept_index);
        nfa.build(true);
        if (dumper.shouldDump("NFA")) {
            std::ofstream dumpNFAFile(dumper.makeDumpPath("NFA"), std::ios::app);
            if (!dumpNFAFile.is_open())
                throw Error("failed to open DFA for dump");
            dumpNFAFile << "token: " << corelib::text::join(name, "::") << "\n";
            dumpNFAFile << nfa;
            dumpNFAFile.close();
        }
        nfas.push_back(nfa);
    }
    if (nfas.empty()) {
        throw Error("Your grammar does not have any DFA-based token");
    }
    auto dfa_meta = DFA::build(ast, nfas);
    dfa = std::get<0>(dfa_meta);
    lr_table = std::get<1>(dfa_meta);
    semantic_table = std::get<2>(dfa_meta);
    max_registers_count = std::get<3>(dfa_meta);

}
auto LexerBuilder::getDataBlocks() const -> LLIR::DataBlockList {
    LLIR::DataBlockList list;
    for (const auto &[name, rule] : ast) {
        if (corelib::text::isLower(name.back()))
            continue;
        // token here
        LLIR::DataBlock dtb;
        stdu::vector<const AST::RuleMember*> members;
        for (const auto &member_ptr : rule.rule_members) {
            auto &member = *member_ptr;
            if (member.prefix.empty())
                continue;
            members.push_back(&member);
        }
        if (members.size() > 0) {
            if (rule.data_block.isRegularDataBlock()) {
                dtb.value = std::make_pair(LangAPI::Expression {}, LLIR::BuilderBase::deduceVarTypeByRuleMember(*members[0]));
            } else if (!rule.data_block.empty()){
                LLIR::inclosed_map inclosed_map;
                std::size_t member_counter = 0;
                LLIR::BuilderData bd(ast, nullptr);
                LLIR::BuilderDataWrapper bdw(bd);
                for (const auto &name : rule.data_block.getTemplatedDataBlock().names) {
                    inclosed_map.emplace(name, std::make_pair(LangAPI::Expression {}, LLIR::BuilderBase::deduceVarTypeByRuleMember(*rule.rule_members[member_counter++])));
                }
                dtb.value = inclosed_map;
            }
        }
        list.emplace(name, dtb);
    }
    return list;
}