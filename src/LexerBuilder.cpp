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
import NFA.TNFA.API;
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
    registers_count = dfa.getRegisterCount();
    output_count = dfa.getOutputCount();

}
static void collectPrefixed(
    const stdu::vector<std::shared_ptr<AST::RuleMember>> &members,
    stdu::vector<const AST::RuleMember*> &out
) {
    for (const auto &member_ptr : members) {
        const auto &member = *member_ptr;

        if (!member.prefix.empty()) {
            // captured as a single field, whatever it is inside — don't descend
            out.push_back(&member);
            continue;
        }

        if (member.isGroup()) {
            collectPrefixed(member.getGroup().values, out); // uncaptured grouping: just sequencing

        } else if (member.isOp()) {
            const auto &options = member.getOp().options;
            if (options.empty())
                continue;

            std::vector<stdu::vector<const AST::RuleMember*>> per_option;
            per_option.reserve(options.size());
            for (const auto &option : options) {
                stdu::vector<const AST::RuleMember*> collected;
                collectPrefixed({option}, collected);
                per_option.push_back(std::move(collected));
            }

            const std::size_t slots = per_option.front().size();
            for (const auto &option_members : per_option)
                if (option_members.size() != slots)
                    throw Error(
                        "LexerBuilder: alternatives of '|' disagree on captured field count ({} vs {})",
                        slots, option_members.size()
                    );

            for (std::size_t i = 0; i < slots; ++i)
                out.push_back(per_option.front()[i]);
        }
        // else: an uncaptured leaf contributes nothing
    }
}
auto LexerBuilder::getDataBlocks() const -> LLIR::DataBlockList {
    LLIR::DataBlockList list;
    LLIR::BuilderData bd(ast, nullptr);
    LLIR::BuilderDataWrapper bdw(bd); // needed by deduceVarTypeByRuleMember, e.g. for nested-token members

    for (const auto &[name, rule] : ast) {
        if (corelib::text::isLower(name.back()))
            continue; // parser rule, not a token

        LLIR::DataBlock dtb;
        stdu::vector<const AST::RuleMember*> members;
        collectPrefixed(rule.rule_members, members);

        if (!members.empty()) {
            if (rule.data_block.isRegularDataBlock()) {
                if (members.size() != 1)
                    throw Error("LexerBuilder: token '{}' has a regular data block but {} captured members",
                                name, members.size());
                dtb.value = std::make_pair(LangAPI::Expression {}, LLIR::BuilderBase::deduceVarTypeByRuleMember(*members[0]));
            } else if (!rule.data_block.empty()) {
                const auto &field_names = rule.data_block.getTemplatedDataBlock().names;
                if (field_names.size() != members.size())
                    throw Error("LexerBuilder: token '{}' declares {} data block fields but has {} captured members",
                                name, field_names.size(), members.size());

                LLIR::inclosed_map inclosed_map;
                std::size_t i = 0;
                for (const auto &field_name : field_names)
                    inclosed_map.emplace(field_name, std::make_pair(
                        LangAPI::Expression {},
                        LLIR::BuilderBase::deduceVarTypeByRuleMember(*members[i++])
                    ));
                dtb.value = inclosed_map;
            }
        }

        list.emplace(name, dtb);
    }
    return list;
}