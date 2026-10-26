#include <gtest/gtest.h>
#include <ispastdlib/lr_conflicts.hpp>
#include <ispa/parser.h>
import AST.Builder;
import UAST;
import Modules;
import AST.Tree;
import AST.Pass;
import LRParser;
import LALRParser;
import std;
namespace {
auto grammar(const char* text) -> AST::Tree {
    Parser::Parser parser;
    AST::Builder builder(parser.parse(text), "conflict.isc");
    builder.build();
    auto tree = builder.get();
    AST::TreePass pass(tree);
    pass.work();
    return tree;
}
}
TEST(LRDiagnostics, ShiftReduceIncludesBothRulesMembersAndPositions) {
    auto tree = grammar("A: 'a'; main: main main | A;");
    LRParser parser(tree);
    ASSERT_TRUE(parser.hasConflicts());
    ASSERT_FALSE(parser.getConflicts().front().origins.empty());
    try { parser.requireConflictFree(); FAIL(); }
    catch (const std::runtime_error& error) {
        const std::string report = error.what();
        EXPECT_NE(report.find("SHIFT: rule 'main', member"), std::string::npos);
        EXPECT_NE(report.find("REDUCE: rule 'main', member"), std::string::npos);
        EXPECT_NE(report.find("conflict.isc"), std::string::npos);
        EXPECT_NE(report.find("end of production"), std::string::npos);
    }
}
TEST(LRDiagnostics, ReduceReduceIncludesBothCompetingRules) {
    auto tree = grammar("A: 'a'; a: A; b: A; main: a | b;");
    LALRParser parser(tree);
    ASSERT_TRUE(parser.hasConflicts());
    try { parser.requireConflictFree(); FAIL(); }
    catch (const std::runtime_error& error) {
        const std::string report = error.what();
        EXPECT_NE(report.find("REDUCE: rule 'a', member"), std::string::npos);
        EXPECT_NE(report.find("REDUCE: rule 'b', member"), std::string::npos);
    }
}
TEST(LRDiagnostics, InsertedWhitespaceDoesNotCreateShiftReduceConflicts) {
    // After A, WS could previously belong to item or to main's boundary.
    auto tree = grammar("A: 'a'; B: 'b'; item: A B?; main: item item;");
    LRParser parser(tree);
    EXPECT_TRUE(parser.skipsAutomaticWhitespace());
    EXPECT_FALSE(parser.hasConflicts());
    LALRParser lalr(tree);
    EXPECT_FALSE(lalr.hasConflicts());
    for (const auto& production : parser.getProductions())
        for (const auto& name : production.rhs) EXPECT_NE(name, (LRBase::Name{"__WS"}));
}

TEST(LRDiagnostics, KeepsSourceFileLabelsWhenInputsAreCombined) {
    Parser::Parser tokens_parser, rules_parser;
    auto modules = tokens_parser.parse("A: 'a';").value.data();
    const auto& rules = rules_parser.parse("main: main main | A;").value.data();
    modules.value.insert(modules.value.end(), rules.value.begin(), rules.value.end());
    AST::Builder builder(modules);
    builder.setSourceFiles({"tokens.isc", "rules.isc"});
    builder.build();
    auto tree = builder.get();
    AST::TreePass pass(tree);
    pass.work();
    LRParser parser(tree);
    try { parser.requireConflictFree(); FAIL(); }
    catch (const std::runtime_error& error) {
        EXPECT_NE(std::string(error.what()).find("at rules.isc"), std::string::npos);
    }
}

namespace {
using Member = std::shared_ptr<AST::RuleMember>;
using Members = stdu::vector<Member>;
auto named(std::string name, char quantifier = '\0') -> Member {
    return std::make_shared<AST::RuleMember>(AST::RuleMember{
        .quantifier = quantifier, .value = AST::RuleMemberName{.name = {std::move(name)}}});
}
auto grouped(Members values, char quantifier = '\0') -> Member {
    return std::make_shared<AST::RuleMember>(AST::RuleMember{
        .quantifier = quantifier, .value = AST::RuleMemberGroup{std::move(values)}});
}
auto choice(Members options, char quantifier = '\0') -> Member {
    return std::make_shared<AST::RuleMember>(AST::RuleMember{
        .quantifier = quantifier, .value = AST::RuleMemberOp{std::move(options)}});
}
auto normalizedTree(Members members) -> AST::Tree {
    AST::TreeMap rules;
    rules[AST::Name{"main"}].rule_members = std::move(members);
    return AST::Tree(std::string{}, AST::SpacemodeStates::MANUAL, std::move(rules));
}
// Earley recognition checks the resulting language independently of LR conflict
// resolution, including epsilon productions and recursive generated helpers.
bool accepts(AST::Tree& tree, std::initializer_list<const char*> tokens) {
    const auto& rules = tree.getInitialItemSet();
    using Item = std::tuple<AST::Name, std::size_t, std::size_t, std::size_t>;
    std::vector<AST::Name> input;
    for (const char* token : tokens) input.push_back(AST::Name{token});
    std::vector<std::set<Item>> chart(input.size() + 1);
    const AST::Name main{"main"};
    for (std::size_t alt = 0; alt < rules.at(main).size(); ++alt) chart[0].insert({main, alt, 0, 0});
    for (std::size_t pos = 0; pos < chart.size(); ++pos) {
        bool changed;
        do {
            changed = false;
            const auto snapshot = chart[pos];
            for (const auto& [lhs, alt, dot, start] : snapshot) {
                const auto& members = rules.at(lhs).at(alt).rule_members;
                if (dot == members.size()) {
                    const auto parents = chart[start];
                    for (const auto& [parent, branch, next, begin] : parents) {
                        const auto& rhs = rules.at(parent).at(branch).rule_members;
                        if (next < rhs.size() && rhs[next]->getName().name == lhs)
                            changed |= chart[pos].insert({parent, branch, next + 1, begin}).second;
                    }
                } else {
                    if (!members[dot]->isName() || members[dot]->quantifier != '\0')
                        throw std::logic_error("Initial production contains an unnormalized member");
                    const auto& symbol = members[dot]->getName().name;
                    if (rules.contains(symbol)) {
                        for (std::size_t branch = 0; branch < rules.at(symbol).size(); ++branch)
                            changed |= chart[pos].insert({symbol, branch, 0, pos}).second;
                    } else if (pos < input.size() && symbol == input[pos]) {
                        chart[pos + 1].insert({lhs, alt, dot + 1, start});
                    }
                }
            }
        } while (changed);
    }
    for (const auto& [lhs, alt, dot, start] : chart.back())
        if (lhs == main && start == 0 && dot == rules.at(main).at(alt).rule_members.size()) return true;
    return false;
}
}

TEST(InitialItemSet, RepeatsEveryAlternativeInAGroup) {
    auto original = grouped({choice({named("A"), named("B")})}, '*');
    auto tree = normalizedTree({original});
    EXPECT_TRUE(accepts(tree, {}));
    EXPECT_TRUE(accepts(tree, {"B", "B"}));
    EXPECT_TRUE(accepts(tree, {"B", "A", "B"}));
    EXPECT_TRUE(accepts(tree, {"A", "B", "A"}));
    EXPECT_FALSE(accepts(tree, {"C"}));
    EXPECT_TRUE(original->isGroup());
    EXPECT_EQ(original->quantifier, '*');
}
TEST(InitialItemSet, NestedAlternativesKeepTheEnclosingSuffix) {
    auto tree = normalizedTree({grouped({choice({named("A"), named("B")})}), named("C")});
    EXPECT_TRUE(accepts(tree, {"A", "C"}));
    EXPECT_TRUE(accepts(tree, {"B", "C"}));
    EXPECT_FALSE(accepts(tree, {"B"}));
    EXPECT_FALSE(accepts(tree, {"A"}));
}
TEST(InitialItemSet, AlternativeBranchesHaveIndependentQuantifierHelpers) {
    auto tree = normalizedTree({choice({grouped({named("A", '?'), named("X")}),
                                       grouped({named("B", '?'), named("Y")})})});
    EXPECT_TRUE(accepts(tree, {"X"}));
    EXPECT_TRUE(accepts(tree, {"A", "X"}));
    EXPECT_TRUE(accepts(tree, {"Y"}));
    EXPECT_TRUE(accepts(tree, {"B", "Y"}));
    EXPECT_FALSE(accepts(tree, {"B", "X"}));
    EXPECT_FALSE(accepts(tree, {"A", "Y"}));
}
TEST(InitialItemSet, AppliesQuantifiersToAlternativeMembers) {
    for (const char quantifier : {'?', '*', '+'}) {
        auto tree = normalizedTree({choice({named("A"), named("B")}, quantifier)});
        EXPECT_EQ(accepts(tree, {}), quantifier != '+');
        EXPECT_TRUE(accepts(tree, {"A"}));
        EXPECT_TRUE(accepts(tree, {"B"}));
        EXPECT_EQ(accepts(tree, {"B", "A", "B"}), quantifier != '?');
    }
}
TEST(InitialItemSet, DoesNotAppendToUserRulesWithHelperLikeNames) {
    auto tree = normalizedTree({named("A", '?')});
    const AST::Name reserved{"main", "__q0"};
    tree.getTreeMap()[reserved].rule_members = {named("Z")};
    EXPECT_TRUE(accepts(tree, {}));
    EXPECT_TRUE(accepts(tree, {"A"}));
    EXPECT_FALSE(accepts(tree, {"Z"}));
    ASSERT_EQ(tree.getInitialItemSet().at(reserved).size(), 1u);
    EXPECT_EQ(tree.getInitialItemSet().at(reserved).front().rule_members.front()->getName().name, (AST::Name{"Z"}));
}
TEST(InitialItemSet, PreservesOccurrenceMetadataAndParentDataBlock) {
    for (const bool group : {false, true}) {
        auto member = group ? grouped({named("A"), named("B")}) : named("A", '?');
        member->prefix.present = true;
        member->prefix.name = "captured";
        member->position = {"capture.isc", 20, 5, 3, 4};
        member->features.push_back(AST::VariableCapture{"binding", member->position});
        auto tree = normalizedTree({member});
        const AST::Name main{"main"};
        auto& source = tree.getTreeMap().at(main);
        source.data_block.type = AST::captureType(*member);
        source.position = member->position;
        const auto expected_type = source.data_block.type;
        const auto& rule = tree.getInitialItemSet().at(main).front();
        ASSERT_EQ(rule.rule_members.size(), 1u);
        const auto& ref = *rule.rule_members.front();
        EXPECT_TRUE(ref.isName());
        EXPECT_TRUE(ref.prefix.present);
        EXPECT_EQ(ref.prefix.name, "captured");
        EXPECT_EQ(ref.position.file, "capture.isc");
        EXPECT_EQ(ref.position.line, 3u);
        EXPECT_EQ(ref.features.size(), 1u);
        EXPECT_EQ(rule.data_block.type, expected_type);
        for (const auto& helper : tree.getInitialItemSet().at(ref.getName().name)) {
            EXPECT_FALSE(helper.isTopLevel);
            EXPECT_EQ(helper.position.file, "capture.isc");
            EXPECT_EQ(helper.features.size(), 1u);
            EXPECT_EQ(helper.original_rules, main);
        }
    }
}
TEST(InitialItemSet, CopiesDataBlockToEveryCompleteRuleAlternative) {
    auto member = choice({named("A"), named("B")});
    member->prefix.present = true;
    auto tree = normalizedTree({member});
    const AST::Name main{"main"};
    auto& source = tree.getTreeMap().at(main);
    source.data_block.type = AST::captureType(*member);
    source.position = {"alternatives.isc", 0, 10, 1, 1};
    const auto expected = source.data_block.type;
    const auto& alternatives = tree.getInitialItemSet().at(main);
    ASSERT_EQ(alternatives.size(), 2u);
    for (const auto& rule : alternatives) {
        EXPECT_EQ(rule.data_block.type, expected);
        EXPECT_EQ(rule.position.file, "alternatives.isc");
        ASSERT_EQ(rule.rule_members.size(), 1u);
        EXPECT_TRUE(rule.rule_members.front()->prefix.present);
    }
}

namespace {
struct ResolutionValue { std::size_t terminals = 0; };
struct ResolutionToken { std::size_t id; std::size_t kind() const { return id; } };
struct ResolutionIterator {
    const std::vector<ResolutionToken>* tokens;
    std::size_t index = 0;
    bool isEnd() const { return index == tokens->size(); }
    const ResolutionToken& operator*() const { return tokens->at(index); }
    ResolutionIterator& operator++() { ++index; return *this; }
};
// Drive the real table builder with the runtime engine, independently of target
// semantic lowering. All terminal/reduction callbacks are counted for deferral.
struct ResolutionDriver : ISPA_STD::LRGeneralizedParser<ResolutionValue, ResolutionToken> {
    static inline const LRBase* table;
    static inline std::vector<LRBase::Name> terminals, nonterminals;
    static inline std::size_t semantic_calls = 0;
    void parseFromTokens() override {}
    auto lazyParse() -> ISPA_STD::MatchResult<ResolutionValue, ISPA_STD::Lexer_base<ResolutionToken>::lazy_iterator> override { return {}; }
    static auto lr_k() { return table->getDecisionLookaheadK(); }
    static auto window(const std::vector<std::size_t>& input, std::size_t n) -> LRBase::Lookahead {
        LRBase::Lookahead result;
        for (std::size_t i = 0; i < n; ++i)
            result.push_back(input[i] == std::numeric_limits<std::size_t>::max() ? LRBase::Name{"$"} : terminals.at(input[i]));
        return result;
    }
    static long long encode(const LRBase::Action& action) {
        if (action.type == LRBase::Action_type::SHIFT) return static_cast<long long>(action.state) + 1;
        if (action.type == LRBase::Action_type::REDUCE) return -static_cast<long long>(action.state) - 1;
        return 0;
    }
    static long long lr_action(std::size_t state, const std::vector<std::size_t>& input) {
        const auto prefix = window(input, table->getLookaheadK());
        const auto& normal = table->getKActionTable();
        if (const auto row = normal.find(state); row != normal.end())
            if (const auto cell = row->second.find(prefix); cell != row->second.end()) return encode(cell->second);
        for (const auto& resolution : table->getConflictResolutions()) {
            const auto& conflict = table->getConflicts().at(resolution.conflict);
            if (conflict.state != state || conflict.lookahead_sequence != prefix) continue;
            if (!resolution.lookahead) return std::numeric_limits<long long>::max();
            for (const auto& decision : resolution.decisions)
                if (decision.window == window(input, resolution.lookahead)) return encode(decision.action);
        }
        return -std::numeric_limits<long long>::max();
    }
    static auto lr_conflict(std::size_t state, const std::vector<std::size_t>& input) -> std::vector<long long> {
        const auto prefix = window(input, table->getLookaheadK());
        for (const auto& conflict : table->getConflicts()) if (conflict.state == state && conflict.lookahead_sequence == prefix) {
            std::vector<long long> result;
            for (const auto& action : conflict.actions) result.push_back(encode(action));
            return result;
        }
        return {};
    }
    static auto lr_length(std::size_t rule) { return table->getProductions().at(rule + 1).rhs.size(); }
    static auto lr_lhs(std::size_t rule) {
        const auto& name = table->getProductions().at(rule + 1).lhs;
        return static_cast<std::size_t>(std::find(nonterminals.begin(), nonterminals.end(), name) - nonterminals.begin());
    }
    static auto lr_goto(std::size_t state, std::size_t lhs) { return table->getGotoTable().at(state).at(nonterminals.at(lhs)); }
    static bool lr_space(std::size_t rule, std::size_t gap) {
        const auto& gaps = table->getProductions().at(rule + 1).whitespace_gaps;
        return gap < gaps.size() && gaps[gap];
    }
    static bool lr_is_space(const ResolutionToken& token) {
        return table->skipsAutomaticWhitespace() && terminals.at(token.id) == LRBase::Name{"__WS"};
    }
    static auto lr_rule_label(std::size_t rule) { return table->getProductions().at(rule + 1).lhs.back(); }
    static auto lr_shift(const ResolutionToken&) -> std::any { ++semantic_calls; return lr_node(ResolutionValue{1}); }
    static auto lr_reduce(std::size_t, const std::vector<std::any>& values) -> std::any {
        ++semantic_calls;
        ResolutionValue result;
        for (const auto& value : values) result.terminals += std::any_cast<ISPA_STD::Node<ResolutionValue>>(value).data().terminals;
        return lr_node(result);
    }
    auto run(const LRBase& parser, const std::vector<LRBase::Name>& input) {
        table = &parser;
        semantic_calls = 0;
        nonterminals.clear(); terminals.clear();
        for (const auto& production : table->getProductions())
            if (std::find(nonterminals.begin(), nonterminals.end(), production.lhs) == nonterminals.end()) nonterminals.push_back(production.lhs);
        for (const auto& production : table->getProductions()) for (const auto& name : production.rhs)
            if (std::find(nonterminals.begin(), nonterminals.end(), name) == nonterminals.end() &&
                std::find(terminals.begin(), terminals.end(), name) == terminals.end()) terminals.push_back(name);
        if (table->skipsAutomaticWhitespace()) terminals.push_back(LRBase::Name{"__WS"});
        std::vector<ResolutionToken> tokens;
        for (const auto& token : input) {
            const auto it = std::find(terminals.begin(), terminals.end(), token);
            if (it == terminals.end()) throw std::invalid_argument("Unknown test token");
            tokens.push_back(ResolutionToken{static_cast<std::size_t>(it - terminals.begin())});
        }
        return lr_parse_generalized<ResolutionDriver>(ResolutionIterator{&tokens}).node.data();
    }
};
}
TEST(LRResolution, ExtraLookaheadSeparatesConflictWithoutRuntimeBranching) {
    auto tree = grammar("A: 'a'; B: 'b'; C: 'c'; D: 'd'; x: A; main: (A B C) | (x B D);");
    LRParser parser(tree);
    ASSERT_TRUE(parser.hasConflicts());
    EXPECT_THROW(parser.requireConflictFree(), std::runtime_error);
    parser.resolveConflicts(3);
    EXPECT_FALSE(parser.needsRuntimeConflictResolution());
    EXPECT_EQ(parser.getDecisionLookaheadK(), 2u);
    ResolutionDriver driver;
    EXPECT_EQ(driver.run(parser, {{"A"}, {"B"}, {"C"}}).terminals, 3u);
    EXPECT_EQ(driver.run(parser, {{"A"}, {"B"}, {"D"}}).terminals, 3u);
}
TEST(LRResolution, RuntimeBranchingChoosesReduceWhenShiftEventuallyFails) {
    auto tree = grammar("A: 'a'; B: 'b'; C: 'c'; D: 'd'; x: A; main: (A B C) | (x B D);");
    LRParser parser(tree);
    parser.resolveConflicts(1); // deliberately force the runtime fallback
    ASSERT_TRUE(parser.needsRuntimeConflictResolution());
    ResolutionDriver driver;
    EXPECT_EQ(driver.run(parser, {{"A"}, {"B"}, {"C"}}).terminals, 3u);
    EXPECT_EQ(driver.run(parser, {{"A"}, {"B"}, {"D"}}).terminals, 3u);
    EXPECT_THROW(driver.run(parser, {{"A"}, {"B"}}), std::runtime_error);
    EXPECT_EQ(ResolutionDriver::semantic_calls, 0u);
}
TEST(LRResolution, UsesRuntimeFallbackBeyondBoundedLookahead) {
    auto tree = grammar("A: 'a'; B: 'b'; C: 'c'; D: 'd'; x: A; tail: B*; main: (A tail C) | (x tail D);");
    LRParser parser(tree);
    parser.resolveConflicts(3);
    ASSERT_TRUE(parser.needsRuntimeConflictResolution());
    std::vector<LRBase::Name> input{{"A"}};
    input.insert(input.end(), 2048, LRBase::Name{"B"});
    input.push_back({"D"});
    ResolutionDriver driver;
    EXPECT_EQ(driver.run(parser, input).terminals, input.size());
}
TEST(LRResolution, RejectsTrueAmbiguityWithoutExecutingSemanticActions) {
    auto tree = grammar("A: 'a'; a: A; b: A; main: a | b;");
    LALRParser parser(tree);
    parser.resolveConflicts(3);
    ASSERT_TRUE(parser.needsRuntimeConflictResolution());
    ResolutionDriver driver;
    try { driver.run(parser, {{"A"}}); FAIL(); }
    catch (const std::runtime_error& error) { EXPECT_NE(std::string(error.what()).find("Ambiguous LR input"), std::string::npos); }
    EXPECT_EQ(ResolutionDriver::semantic_calls, 0u);
}
TEST(LRResolution, ConflictFreeGrammarNeedsNoFallback) {
    auto tree = grammar("A: 'a'; main: A;");
    LRParser parser(tree);
    parser.resolveConflicts();
    EXPECT_FALSE(parser.needsRuntimeConflictResolution());
    EXPECT_TRUE(parser.getConflictResolutions().empty());
    EXPECT_EQ(parser.getDecisionLookaheadK(), 1u);
}
TEST(LRResolution, ResolvesBootstrapOrdinaryAndMultipleCaptures) {
    const auto project = Modules::preprocess({std::string(ROOT_DIR) + "/parser/ispa/grammar"});
    UAST::main syntax;
    std::vector<std::string> files;
    for (const auto& source : project.sources) {
        SCOPED_TRACE(source.file);
        IspaParser parser;
        const auto& parsed = parser.parse(source.grammar.c_str()).value.data();
        ASSERT_TRUE(parser.getErrors().empty()) << source.file;
        syntax.value.insert(syntax.value.end(), parsed.value.begin(), parsed.value.end());
        files.insert(files.end(), parsed.value.size(), source.file);
    }
    AST::Builder builder(syntax);
    builder.setSourceFiles(std::move(files));
    builder.setModules(project);
    builder.build();
    auto tree = builder.get();
    AST::TreePass pass(tree); pass.work();
    LRParser parser(tree);
    parser.resolveConflicts(3);
    EXPECT_TRUE(parser.automaticConflictResolution());
    ResolutionDriver driver;
    const std::vector<LRBase::Name> ordinary{{"modules", "common", "ID"}, {"COLON"}, {"COMMERCIAL_AT"}, {"modules", "rule", "value", "STRING"}, {"SEMICOLON"}};
    EXPECT_EQ(driver.run(parser, ordinary).terminals, ordinary.size());
    const std::vector<LRBase::Name> multiple{{"modules", "common", "ID"}, {"COLON"}, {"COMMERCIAL_AT"}, {"EQUALS_SIGN"}, {"LEFT_PARENTHESIS"}, {"modules", "rule", "value", "STRING"}, {"RIGHT_PARENTHESIS"}, {"SEMICOLON"}};
    EXPECT_EQ(driver.run(parser, multiple).terminals, multiple.size());
}

TEST(InitialItemSet, CaptureTypesPreserveRepeatedNamesAndGroupFields) {
    auto tree = grammar("A: 'a'; B: 'b'; main: @ A+ (';' @ B)* @{first, second};");
    const auto& fields = tree.getTreeMap().at(AST::Name{"main"}).data_block.fields;
    ASSERT_EQ(fields.size(), 2u);
    for (const auto& [name, type] : fields) {
        EXPECT_EQ(type.kind, AST::DataType::Kind::Array);
        ASSERT_EQ(type.parameters.size(), 1u);
        EXPECT_EQ(type.parameters.front().kind, AST::DataType::Kind::Token);
    }
}

TEST(LRResolution, AllowsWhitespaceBetweenIterationsButPreservesFollowingNoSpace) {
    for (char quantifier : {'*', '+'}) {
        const auto source = std::string("A:'a'; B:'b'; main: @ A") + quantifier + " \\s0 B {@};";
        auto tree = grammar(source.c_str());
        LRParser parser(tree);
        parser.resolveConflicts();
        ResolutionDriver driver;
        EXPECT_EQ(driver.run(parser, {{"A"}, {"__WS"}, {"A"}, {"B"}}).terminals, 3u);
        EXPECT_EQ(driver.run(parser, {{"A"}, {"B"}}).terminals, 2u);
        EXPECT_THROW(driver.run(parser, {{"A"}, {"__WS"}, {"B"}}), std::runtime_error);
        if (quantifier == '*') EXPECT_EQ(driver.run(parser, {{"B"}}).terminals, 1u);
    }
}
