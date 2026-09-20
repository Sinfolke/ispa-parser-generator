module LexerBuilder;
import corelib;
import LLIR.Builder;
import LLIR.Builder.Base;
import LLIR.Builder.Data;
import LLIR.Builder.DataWrapper;
import LLIR.Rule.MemberBuilder;
import LLIR.RuleBuilder;
import LLIR.CllBuilder;
import NFA.IR;
import NFA.IR.Interpreter;
import NFA.TNFA;
import NFA.TNFA.Interpreter;
import NFA_OLD;
import DFA.API;
import DFA;
import Dump;
import args;
import logging;
import constants;
import cpuf.op;
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
bool LexerBuilder::isTopLevel(const stdu::vector<std::string> &name) {
    auto &use_places = ast.getUsePlacesTable();
    if (!use_places.contains(name))
        return true;
    bool used_in_non_terminal = false;
    for (const auto &place : use_places.at(name)) {
        if (corelib::text::isLower(place.back())) {
            used_in_non_terminal = true;
            break;
        }
    }
    return used_in_non_terminal;
}
void LexerBuilder::build() {
    NFA::InitialNFA initial_nfa(ast);
    initial_nfa.build();
    initial_nfa.unrollGroups();
    initial_nfa.unrollQuantifiers();
    initial_nfa.unrollNestedTokens();
    // 1. Generate exhaustive test cases using AstInputGenerator
    auto test_cases = ast.generateRandomTokenInputs(2);

    // 2. Instantiate interpreter

    // 3. Execute test inputs and print AST trees
    for (const auto &[rule_name, samples] : test_cases) {
        Tlog::Branch b(logger, "NFA-IR-TEST/" + corelib::text::join(rule_name, "/") + ".log");
        if (!initial_nfa.get().contains(rule_name)) {
            b.log("<optimized out of Top-List rules>");
            continue;
        }
        NFA::Interpreter::IRInterpreter interpreter(initial_nfa.get().at(rule_name));
        for (const auto &sample : samples) {
            logger.log("sample {}", sample);
            auto result = interpreter.parse(sample);
            logger.log("result: {}", result);
        }
    }
    if (dumper.shouldDump("NFA-IR")) {
        std::ofstream dumpNFAFile(dumper.makeDumpPath("NFA-IR"));
        if (!dumpNFAFile.is_open())
            throw Error("failed to open NFA-IR for dump");
        dumpNFAFile << initial_nfa;
    }

    // build actual NFA from NFA IR
    NFA::TNFA::TNFABuilder builder(initial_nfa, dumper.isNfaDebug());
    builder.build();
    if (dumper.shouldDump("NFA")) {
        std::ofstream dumpNFAFile(dumper.makeDumpPath("NFA"));
        if (!dumpNFAFile.is_open())
            throw Error("failed to open NFA for dump");
        dumpNFAFile << builder;
    }
    // 1. Instantiate interpreter ONCE outside the loop to avoid copying state vectors repeatedly
    NFA::Interpreter::TNFAInterpreter interpreter(builder.getStates());

    for (const auto &[rule_name, samples]: test_cases) {
        auto entry = builder.getEntry(rule_name);
        if (!entry.has_value()) {
            continue;
        }

        // 3. Avoid nested subdirectories in filenames unless directories are created explicitly
        const std::string sanitize_rule = corelib::text::join(rule_name, "_");
        Tlog::Branch b(logger, "TNFA-TEST/" + sanitize_rule + ".log");

        for (const auto &sample: samples) {
            logger.log("sample {}", sample);

            // 4. Pass 'sample' directly: sample.c_str() forces an unnecessary O(N) strlen calculation
            auto result = interpreter.run(entry.value(), sample);
            logger.log("result: {}", result);
        }
    }
    DFA::DFA dfa(&builder);
    dfa.build();
    dfa.minimize();
    auto classified = dfa.classify();
    this->dfa = std::move(classified);
    action_table = dfa.getActionTable();
    semantic_table = dfa.getSemanticTable();
    max_registers_count = 10;

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
                    auto t = LLIR::BuilderBase::deduceVarTypeByRuleMember(*members[member_counter++]);
                    inclosed_map.emplace(name, std::make_pair(LangAPI::Expression {}, t));
                }
                dtb.value = inclosed_map;
            }
        }
        list.emplace(name, dtb);
    }
    return list;
}