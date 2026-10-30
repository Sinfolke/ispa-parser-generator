#include <gtest/gtest.h>
#include <ispa/parser.h>
import Algorithm.API;
import Algorithm.Cache;
import AST.Builder;
import AST.API;
import AST.Pass;
import AST.Tree;
import AST.types;
import LangAPI;
import LangRepr.Converter;
import LexerBuilder;
import ParserIR.IR;
import ParserIR.API;
import Dump;
import args;
import cpuf.dlib;
import std;

namespace {
auto pluginGrammar(const char* source) -> AST::Tree {
    IspaParser parser;
    AST::Builder builder(parser.parse(source), "plugin.isc");
    builder.build();
    auto tree = builder.get();
    AST::TreePass pass(tree);
    pass.work();
    return tree;
}
auto convert(const Algorithm::Descriptor& algorithm, AST::Tree& tree,
             Algorithm::Metadata& metadata,
             const std::filesystem::path& output = {}) -> std::string {
    static Args settings{};
    dumper.setArgsPointer(&settings);
    AST::TreeMap changed;
    auto result = algorithm.build({tree, changed}, metadata);
    if (!result.output || !result.to_lang_repr) throw std::runtime_error("Missing conversion");
    LexerBuilder lexer(tree);
    lexer.build();
    const std::string name = "PluginTest";
    Algorithm::ConversionRequest request{lexer, tree, LangAPI::Language::Cpp, name};
    auto holder = result.to_lang_repr(result.output.get(), request);
    LangRepr::Converter converter(holder, "cpp", name, false);
    converter.init();
    converter.construct();
    std::string text;
    for (const auto& [file, content] : converter.get()) {
        text += file + '\n' + content;
        if (!output.empty()) {
            const auto path = output / file;
            std::filesystem::create_directories(path.parent_path());
            std::ofstream(path) << content;
        }
    }
    return text;
}
}

TEST(AlgorithmPlugin, LLRepeatedRulesAndTokensAcceptWhitespaceSeparators) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    auto tree = pluginGrammar(
        "A:'a'; B:'b'; main: @ item+ B @ A* '!' @{rules, tokens}; item: @ A {@};");
    const auto directory = std::filesystem::temp_directory_path() /
        ("ispa-repeat-spaces-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::filesystem::remove_all(path); }
    } cleanup{directory};
    Algorithm::Metadata metadata;
    convert(*descriptor, tree, metadata, directory);
    std::ofstream(directory / "run.cpp") << R"(
#include "PluginTest.h"
int main() {
    PluginTest::Parser parser;
    const auto& multiple = parser.parse("a a\nb a\na !");
    if (multiple.rules.size() != 2 || multiple.tokens.size() != 2) return 1;
    const auto& single = parser.parse("a b !");
    if (single.rules.size() != 1 || !single.tokens.empty()) return 2;
}
)";
    const auto command = std::string("clang++ -std=c++23 -I\"") +
        std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp\" -I\"" + directory.string() +
        "\" \"" + (directory / "PluginTest.cpp").string() + "\" \"" +
        (directory / "run.cpp").string() + "\" \"" + std::string(ROOT_DIR) +
        "/parser/stdlibs/target/target-cpp/src/core.cpp\" \"" + std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp/src/ispa_core_lowered.cpp\" -o \"" + (directory / "run").string() + "\"";
    ASSERT_EQ(std::system(command.c_str()), 0);
    EXPECT_EQ(std::system(('"' + (directory / "run").string() + '"').c_str()), 0);
}

TEST(AlgorithmPlugin, CapturedLexerTextAccumulatesInsideParameterizedToken) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    const auto directory = std::filesystem::temp_directory_path() /
        ("ispa-root-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::filesystem::remove_all(path); }
    } cleanup{directory};
    auto tree = pluginGrammar(R"isc(
STRING<quote>:
    &q ${quote} @ !([\n\r] | $q)* $q

    {@}
;

QUOTE_1: '"';
QUOTE_2: '\'';
main:
    @ STRING<QUOTE_1>

    @{str1}
;
)isc");
    Algorithm::Metadata metadata;
    convert(*descriptor, tree, metadata, directory);
    std::ofstream(directory / "run.cpp") << R"(
#include "PluginTest.h"
int main() {
    PluginTest::Parser parser;
    const auto tree = parser.parse("\"Hello\"");
    return tree.str1.data().value == "Hello" ? 0 : 1;
}
)";
    const auto command = std::string("clang++ -std=c++23 -I\"") + ROOT_DIR +
        "/parser/stdlibs/target/target-cpp\" -I\"" + directory.string() + "\" \"" +
        (directory / "PluginTest.cpp").string() + "\" \"" +
        (directory / "run.cpp").string() + "\" \"" + ROOT_DIR +
        "/parser/stdlibs/target/target-cpp/src/core.cpp\" \"" + std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp/src/ispa_core_lowered.cpp\" -o \"" + (directory / "run").string() + "\"";
    ASSERT_EQ(std::system(command.c_str()), 0);
    EXPECT_EQ(std::system(('"' + (directory / "run").string() + '"').c_str()), 0);
}

TEST(AlgorithmPlugin, LLRuleArgumentsAndIntegerBounds) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    auto tree = pluginGrammar(R"(
A: 'a'; main: A child&(value_A); child&(arg): A; item&(count: int): @ A=count {@};
astParam&(number: int) {
    ctx.ast.number = number
    match A;
}
regularParam&(number: int): A &= { number: number };
)");
    const auto directory = std::filesystem::temp_directory_path() /
        ("ispa-rule-parameters-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::filesystem::remove_all(path); } } cleanup{directory};
    Algorithm::Metadata metadata;
    convert(*descriptor, tree, metadata, directory);
    std::ofstream(directory / "run.cpp") << R"(
#include "PluginTest.h"
struct Harness : PluginTest::Parser {
    auto counted(const char* input, long long count) {
        PluginTest::Lexer lexer;
        return item(PluginTest::Lexer::lazy_iterator(lexer, input), count);
    }
    auto astValue(const char* input, long long number) {
        PluginTest::Lexer lexer;
        return astParam(PluginTest::Lexer::lazy_iterator(lexer, input), number);
    }
    auto regularValue(const char* input, long long number) {
        PluginTest::Lexer lexer;
        return regularParam(PluginTest::Lexer::lazy_iterator(lexer, input), number);
    }
};
int main() {
    PluginTest::Parser parser;
    parser.parse("aa");
    Harness harness;
    const auto result = harness.counted("aa", 2);
    if (!result.status || result.node.data().value.size() != 2) return 1;
    const auto ast = harness.astValue("a", 7);
    if (!ast.status || ast.node.data().number != 7) return 2;
    const auto regular = harness.regularValue("a", 9);
    return regular.status && regular.node.data().number == 9 ? 0 : 3;
}
)";
    const auto command = std::string("clang++ -std=c++23 -I\"") + ROOT_DIR +
        "/parser/stdlibs/target/target-cpp\" -I\"" + directory.string() + "\" \"" +
        (directory / "PluginTest.cpp").string() + "\" \"" +
        (directory / "run.cpp").string() + "\" \"" + ROOT_DIR +
        "/parser/stdlibs/target/target-cpp/src/core.cpp\" \"" + std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp/src/ispa_core_lowered.cpp\" -o \"" + (directory / "run").string() + "\"";
    ASSERT_EQ(std::system(command.c_str()), 0);
    EXPECT_EQ(std::system(('"' + (directory / "run").string() + '"').c_str()), 0);
}

TEST(AlgorithmPlugin, LLGrammarTemplatesKeepTheirNodeTypesAndForwardArguments) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    auto tree = pluginGrammar(R"(
A:'a'; B:'b'; leaf: @ A {@}; outer<T>: @ T {@};
WRAP<T>: '[' @ ${T} ']' {@};
forwarded<U>: @ outer<U> {@};
unused<V>: V;
main: @ forwarded<leaf> @ outer<B> @ WRAP<A> @ WRAP<B> @{first, second, third, fourth};
repeat<T>&(count: int): @ T=count {@};
driver {
    n = 2
    match repeat<B>&(n);
}
)");
    const auto directory = std::filesystem::temp_directory_path() /
        ("ispa-rule-templates-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::filesystem::remove_all(path); } } cleanup{directory};
    Algorithm::Metadata metadata;
    convert(*descriptor, tree, metadata, directory);
    std::ofstream(directory / "run.cpp") << R"(
#include "PluginTest.h"
#include <type_traits>
struct Harness : PluginTest::Parser {
    auto combined(const char* input) {
        PluginTest::Lexer lexer;
        return driver(PluginTest::Lexer::lazy_iterator(lexer, input));
    }
};
int main() {
    PluginTest::Parser parser;
    const auto& result = parser.parse("ab[a][b]");
    static_assert(std::is_same_v<std::remove_cvref_t<decltype(result.first.data())>,
        PluginTest::Types::forwarded<PluginTest::Types::leaf>>);
    static_assert(std::is_same_v<std::remove_cvref_t<decltype(result.second.data())>,
        PluginTest::Types::outer<PluginTest::Types::B>>);
    static_assert(std::is_same_v<std::remove_cvref_t<decltype(result.third.data())>,
        PluginTest::Types::WRAP<PluginTest::Types::A>>);
    if (result.first.data().value.data().value.empty() || result.second.data().value.empty()) return 1;
    if (result.third.data().value.empty() || result.fourth.data().value.empty()) return 4;
    if (result.third.data().value.startpos() != 3 || result.third.data().value.length() != 1) return 5;
    Harness harness;
    if (!harness.combined("bb").status || !harness.getErrors().empty()) return 3;
    return parser.getErrors().empty() ? 0 : 2;
}
)";
    const auto command = std::string("clang++ -std=c++23 -I\"") + ROOT_DIR +
        "/parser/stdlibs/target/target-cpp\" -I\"" + directory.string() + "\" \"" +
        (directory / "PluginTest.cpp").string() + "\" \"" +
        (directory / "run.cpp").string() + "\" \"" + ROOT_DIR +
        "/parser/stdlibs/target/target-cpp/src/core.cpp\" \"" + std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp/src/ispa_core_lowered.cpp\" -o \"" + (directory / "run").string() + "\"";
    ASSERT_EQ(std::system(command.c_str()), 0);
    EXPECT_EQ(std::system(('"' + (directory / "run").string() + '"').c_str()), 0);
}

TEST(AlgorithmPlugin, LLTypedLexerParametersSupportRegularAndAdvancedTokens) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    auto tree = pluginGrammar(R"(
FIRST&(text: str, number: int): 'a' &= { text: text number: number };
SECOND&(number: int, text: str) {
    match 'b';
    ctx.ast.text = text
    ctx.ast.number = number
}
main: FIRST SECOND;
)");
    const auto directory = std::filesystem::temp_directory_path() /
        ("ispa-lexer-parameters-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::filesystem::remove_all(path); } } cleanup{directory};
    Algorithm::Metadata metadata;
    convert(*descriptor, tree, metadata, directory);
    std::ofstream(directory / "run.cpp") << R"(
#include "PluginTest.h"
int main() {
    PluginTest::Lexer lexer;
    lexer.setFIRSTParameters("alpha", 3);
    const char* first = "a";
    lexer.setInput(first);
    const auto one = lexer.makeToken(first).to_node<PluginTest::Types::FIRST>();
    if (one.data().text != "alpha" || one.data().number != 3) return 1;
    lexer.setSECONDParameters(4, "beta");
    const char* second = "b";
    lexer.setInput(second);
    const auto two = lexer.makeToken(second).to_node<PluginTest::Types::SECOND>();
    return two.data().text == "beta" && two.data().number == 4 ? 0 : 2;
}
)";
    const auto command = std::string("clang++ -std=c++23 -I\"") + ROOT_DIR +
        "/parser/stdlibs/target/target-cpp\" -I\"" + directory.string() + "\" \"" +
        (directory / "PluginTest.cpp").string() + "\" \"" +
        (directory / "run.cpp").string() + "\" \"" + ROOT_DIR +
        "/parser/stdlibs/target/target-cpp/src/core.cpp\" \"" + std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp/src/ispa_core_lowered.cpp\" -o \"" + (directory / "run").string() + "\"";
    ASSERT_EQ(std::system(command.c_str()), 0);
    EXPECT_EQ(std::system(('"' + (directory / "run").string() + '"').c_str()), 0);
}

TEST(AlgorithmPlugin, LLTokenVariableCaptureFeedsAction) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    auto tree = pluginGrammar(R"(
WORD {
    match &captured [a-z]+;
    ctx.ast.text = captured
}
PAIR {
    match &captured ('1' '2');
    ctx.ast.text = captured
}
main: WORD | PAIR;
)");
    const auto directory = std::filesystem::temp_directory_path() /
        ("ispa-lexer-capture-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::filesystem::remove_all(path); }
    } cleanup{directory};
    Algorithm::Metadata metadata;
    convert(*descriptor, tree, metadata, directory);
    std::ofstream(directory / "run.cpp") << R"(
#include "PluginTest.h"
int main() {
    PluginTest::Lexer lexer;
    const char* pos = "hello";
    lexer.setInput(pos);
    const auto token = lexer.makeToken(pos).to_node<PluginTest::Types::WORD>();
    if (token.data().text != "hello") return 1;
    const char* pair = "12";
    lexer.setInput(pair);
    const auto grouped = lexer.makeToken(pair).to_node<PluginTest::Types::PAIR>();
    return grouped.data().text == "12" ? 0 : 2;
}
)";
    const auto command = std::string("clang++ -std=c++23 -I\"") + ROOT_DIR +
        "/parser/stdlibs/target/target-cpp\" -I\"" + directory.string() + "\" \"" +
        (directory / "PluginTest.cpp").string() + "\" \"" + (directory / "run.cpp").string() +
        "\" \"" + ROOT_DIR + "/parser/stdlibs/target/target-cpp/src/core.cpp\" \"" + std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp/src/ispa_core_lowered.cpp\" -o \"" +
        (directory / "run").string() + "\"";
    ASSERT_EQ(std::system(command.c_str()), 0);
    EXPECT_EQ(std::system(('"' + (directory / "run").string() + '"').c_str()), 0);
}

TEST(AlgorithmPlugin, LLLexerFailBlockHandlesDeadDfaTransition) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    auto tree = pluginGrammar(R"(
WORD: %invalid [a-z]+;
main: WORD;
fail invalid {
    ctx.error("invalid character")
}
)");
    const auto directory = std::filesystem::temp_directory_path() /
        ("ispa-lexer-failure-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::filesystem::remove_all(path); }
    } cleanup{directory};
    Algorithm::Metadata metadata;
    convert(*descriptor, tree, metadata, directory);
    std::ofstream(directory / "run.cpp") << R"(
#include "PluginTest.h"
#include <string>
int main() {
    PluginTest::Lexer lexer;
    const char* pos = "?";
    lexer.setInput(pos);
    try { lexer.makeToken(pos); }
    catch (const std::exception& error) {
        return std::string(error.what()).find("invalid character") != std::string::npos ? 0 : 2;
    }
    return 1;
}
)";
    const auto command = std::string("clang++ -std=c++23 -I\"") + ROOT_DIR +
        "/parser/stdlibs/target/target-cpp\" -I\"" + directory.string() + "\" \"" +
        (directory / "PluginTest.cpp").string() + "\" \"" + (directory / "run.cpp").string() +
        "\" \"" + ROOT_DIR + "/parser/stdlibs/target/target-cpp/src/core.cpp\" \"" + std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp/src/ispa_core_lowered.cpp\" -o \"" +
        (directory / "run").string() + "\"";
    ASSERT_EQ(std::system(command.c_str()), 0);
    EXPECT_EQ(std::system(('"' + (directory / "run").string() + '"').c_str()), 0);
}

TEST(AlgorithmPlugin, LLLexerRuleFailureBindingResolvesBareMarker) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    auto tree = pluginGrammar(R"(
WORD: % [a-z]+ %{invalid, another};
main: WORD;
fail invalid { ctx.error("bound lexer failure") }
fail another { ctx.error("second lexer failure") }
)");
    Algorithm::Metadata metadata;
    const auto generated = convert(*descriptor, tree, metadata);
    EXPECT_NE(generated.find("bound lexer failure"), std::string::npos);
    EXPECT_NE(generated.find("second lexer failure"), std::string::npos);
    EXPECT_NE(generated.find("semantic_action_exec"), std::string::npos);
}

TEST(AlgorithmPlugin, LLLexerFailBlockReceivesTokenParameter) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    auto tree = pluginGrammar(R"(
WORD&(message: str): %invalid&(message) [a-z]+;
main: WORD;
fail invalid&(reason) { ctx.error(reason) }
)");
    const auto directory = std::filesystem::temp_directory_path() /
        ("ispa-lexer-failure-param-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::filesystem::remove_all(path); }
    } cleanup{directory};
    Algorithm::Metadata metadata;
    convert(*descriptor, tree, metadata, directory);
    std::ofstream(directory / "run.cpp") << R"(
#include "PluginTest.h"
#include <string>
int main() {
    PluginTest::Lexer lexer;
    lexer.setWORDParameters("parameterized failure");
    const char* pos = "?";
    lexer.setInput(pos);
    try { lexer.makeToken(pos); }
    catch (const std::exception& error) {
        return std::string(error.what()).find("parameterized failure") != std::string::npos ? 0 : 2;
    }
    return 1;
}
)";
    const auto command = std::string("clang++ -std=c++23 -I\"") + ROOT_DIR +
        "/parser/stdlibs/target/target-cpp\" -I\"" + directory.string() + "\" \"" +
        (directory / "PluginTest.cpp").string() + "\" \"" + (directory / "run.cpp").string() +
        "\" \"" + ROOT_DIR + "/parser/stdlibs/target/target-cpp/src/core.cpp\" \"" + std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp/src/ispa_core_lowered.cpp\" -o \"" +
        (directory / "run").string() + "\"";
    ASSERT_EQ(std::system(command.c_str()), 0);
    EXPECT_EQ(std::system(('"' + (directory / "run").string() + '"').c_str()), 0);
}

TEST(AlgorithmPlugin, LLMidTokenDynamicPatternAndNegatedGroup) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    Parser::Parser syntax;
    AST::Builder builder(syntax.parse("REPEAT: &q [a-z]+ ':' $q;"), "plugin.isc");
    builder.build();
    auto tree = builder.get();
    Parser::Parser quote_syntax;
    AST::Builder quote_builder(quote_syntax.parse("QUOTED: '#' &q [a-z]+ ':' !($q | '!')* $q;"), "plugin.isc");
    quote_builder.build();
    auto quote_tree = quote_builder.get();
    for (auto &[name, rule] : quote_tree.getTreeMap())
        tree.getTreeMap().emplace(name, std::move(rule));
    Parser::Parser negative_syntax;
    AST::Builder negative_builder(negative_syntax.parse("NEGATED: '~' &q [a-z]+ ':' !$q;"), "plugin.isc");
    negative_builder.build();
    auto negative_tree = negative_builder.get();
    for (auto &[name, rule] : negative_tree.getTreeMap())
        tree.getTreeMap().emplace(name, std::move(rule));
    Parser::Parser capture_syntax;
    AST::Builder capture_builder(capture_syntax.parse("DYNAMIC_CAPTURE: '^' &q [a-z]+ ':' &copy $q ':' $copy;"), "plugin.isc");
    capture_builder.build();
    auto capture_tree = capture_builder.get();
    for (auto &[name, rule] : capture_tree.getTreeMap())
        tree.getTreeMap().emplace(name, std::move(rule));
    ASSERT_TRUE(tree.getTreeMap().contains({"REPEAT"}));
    ASSERT_TRUE(tree.getTreeMap().contains({"QUOTED"}));
    ASSERT_TRUE(tree.getTreeMap().contains({"NEGATED"}));
    ASSERT_TRUE(tree.getTreeMap().contains({"DYNAMIC_CAPTURE"}));
    AST::Rule main;
    AST::RuleMemberOp choices;
    for (const auto* name : {"REPEAT", "QUOTED", "NEGATED", "DYNAMIC_CAPTURE"})
        choices.options.push_back(std::make_shared<AST::RuleMember>(AST::RuleMember{
            .value = AST::RuleMemberName{.name = {name}}}));
    main.rule_members.push_back(std::make_shared<AST::RuleMember>(AST::RuleMember{
        .value = std::move(choices)}));
    tree.getTreeMap()[{"main"}] = std::move(main);
    AST::TreePass pass(tree);
    pass.work();
    const auto directory = std::filesystem::temp_directory_path() /
        ("ispa-dynamic-lexer-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::filesystem::remove_all(path); }
    } cleanup{directory};
    Algorithm::Metadata metadata;
    convert(*descriptor, tree, metadata, directory);
    std::ofstream(directory / "run.cpp") << R"(
#include "PluginTest.h"
int main() {
    PluginTest::Lexer lexer;
    const char* first = "abc:abc";
    const char* pos = first;
    lexer.setInput(first);
    if (lexer.makeToken(pos).empty() || pos - first != 7) return 1;
    const char* second = "#abc:yyyabc";
    pos = second;
    lexer.setInput(second);
    if (lexer.makeToken(pos).empty() || pos - second != 11) return 2;
    const char* adjacent = "#abc:abc";
    pos = adjacent;
    lexer.setInput(adjacent);
    if (lexer.makeToken(pos).empty() || pos - adjacent != 8) return 3;
    const char* single = "~abc:x";
    pos = single;
    lexer.setInput(single);
    if (lexer.makeToken(pos).empty() || pos - single != 6) return 4;
    const char* captured = "^abc:abc:abc";
    pos = captured;
    lexer.setInput(captured);
    if (lexer.makeToken(pos).empty() || pos - captured != 12) return 5;
    int rejected_index = 6;
    for (const char* rejected : {"abc:abd", "#abc:!abc", "~abc:abc", "^abc:abc:abd"}) {
        pos = rejected;
        lexer.setInput(rejected);
        try {
            if (!lexer.makeToken(pos).empty()) return rejected_index;
        } catch (const std::exception&) {
        }
        ++rejected_index;
    }
    return 0;
}
)";
    const auto command = std::string("clang++ -std=c++23 -I\"") + ROOT_DIR +
        "/parser/stdlibs/target/target-cpp\" -I\"" + directory.string() + "\" \"" +
        (directory / "PluginTest.cpp").string() + "\" \"" +
        (directory / "run.cpp").string() + "\" \"" + ROOT_DIR +
        "/parser/stdlibs/target/target-cpp/src/core.cpp\" \"" + std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp/src/ispa_core_lowered.cpp\" -o \"" + (directory / "run").string() + "\"";
    ASSERT_EQ(std::system(command.c_str()), 0);
    EXPECT_EQ(std::system(('"' + (directory / "run").string() + '"').c_str()), 0);
}

TEST(AlgorithmPlugin, LLTemplateDynamicPatternsKeepEachQuoteAndCapture) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    auto tree = pluginGrammar(R"(
STRING<quote>:
    &q ${quote} @ !([\n\r] | $q)* $q {@}
;
QUOTE_1: '"';
QUOTE_2: '\'';
main:
    @ STRING<QUOTE_1> '+' @ STRING<QUOTE_2>
    @{a, b}
;
)");
    const auto directory = std::filesystem::temp_directory_path() /
        ("ispa-template-dynamic-lexer-" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::filesystem::remove_all(path); }
    } cleanup{directory};
    Algorithm::Metadata metadata;
    convert(*descriptor, tree, metadata, directory);
    std::ofstream(directory / "run.cpp") << R"(
#include "PluginTest.h"
int main() {
    PluginTest::Parser parser;
    const auto tree = parser.parse("\"Hello\" + 'World'");
    return tree.a.data().value == "Hello" && tree.b.data().value == "World" ? 0 : 1;
}
)";
    const auto command = std::string("clang++ -std=c++23 -I\"") + ROOT_DIR +
        "/parser/stdlibs/target/target-cpp\" -I\"" + directory.string() + "\" \"" +
        (directory / "PluginTest.cpp").string() + "\" \"" +
        (directory / "run.cpp").string() + "\" \"" + ROOT_DIR +
        "/parser/stdlibs/target/target-cpp/src/core.cpp\" \"" + std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp/src/ispa_core_lowered.cpp\" -o \"" + (directory / "run").string() + "\"";
    ASSERT_EQ(std::system(command.c_str()), 0);
    EXPECT_EQ(std::system(('"' + (directory / "run").string() + '"').c_str()), 0);
}

TEST(AlgorithmPlugin, LLLexerFailBlockDeletesOrInserts) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    auto tree = pluginGrammar(R"(
TOKEN: 'a' %recover 'b';
main: TOKEN;
fail recover {
    ctx.error("incomplete token")
    if ctx.at('?'):
        ctx.delete()
    else:
        ctx.insert()
}
)");
    const auto directory = std::filesystem::temp_directory_path() /
        ("ispa-lexer-recover-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::filesystem::remove_all(path); }
    } cleanup{directory};
    Algorithm::Metadata metadata;
    convert(*descriptor, tree, metadata, directory);
    std::ofstream(directory / "run.cpp") << R"(
#include "PluginTest.h"
int main() {
    PluginTest::Lexer inserted;
    inserted.makeTokens("a");
    if (inserted.getErrors().size() != 1 || inserted.getTokens().size() != 2) return 1;
    PluginTest::Lexer deleted;
    deleted.makeTokens("a?");
    if (deleted.getErrors().size() != 1 || deleted.getTokens().size() != 1) return 2;
    return 0;
}
)";
    const auto command = std::string("clang++ -std=c++23 -I\"") + ROOT_DIR +
        "/parser/stdlibs/target/target-cpp\" -I\"" + directory.string() + "\" \"" +
        (directory / "PluginTest.cpp").string() + "\" \"" + (directory / "run.cpp").string() +
        "\" \"" + ROOT_DIR + "/parser/stdlibs/target/target-cpp/src/core.cpp\" \"" + std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp/src/ispa_core_lowered.cpp\" -o \"" +
        (directory / "run").string() + "\"";
    ASSERT_EQ(std::system(command.c_str()), 0);
    EXPECT_EQ(std::system(('"' + (directory / "run").string() + '"').c_str()), 0);
}

TEST(AlgorithmPlugin, LLAdvancedContextsExecuteAndReturnTheirAst) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    auto tree = pluginGrammar(R"(
A: 'a'; B: 'b'; X: 'x';
main: @ item {@};
item {
    ctx.ast.total = 1
    match A -> first;
    ctx.ast.first = first
    saved = ctx.pos
    ctx.pos = ctx.start_pos
    ctx.ast.total += 2
    match A;
    ctx.pos = saved
    match %oops&(first) B;
    ctx.ast.total += 4
    return ctx.ast
    ctx.ast.total = 999
}

fail oops&(previous) {
    ctx.ast.failed = ctx.failed_rule
    ctx.ast.owner = ctx.rule
    ctx.ast.recovered = True
    ctx.ast.had_previous = not previous.empty()
    ctx.ast.named: bool = ctx.at(X)
    ctx.ast.named_string: bool = ctx.at("x")
    ctx.ast.wrong_literal: bool = ctx.at("X")
    ctx.pos += 1
}
regular: @ A suffix @{token} %{missing};
suffix: B;
fail missing {
    ctx.ast.saw_token = not ctx.ast.token.empty()
    ctx.ast.expected = ctx.failed_rule
    prior = ctx.prev()
    ctx.ast.prior_is_a: bool = prior.name == A
    ctx.ast.prior_literal: bool = prior.name == "a"
    ctx.take()
}
starts: @ start_failure {@};
optional_rule: @ start_failure? {@};
start_failure {
    match %at_start A;
    ctx.ast.finished = True
}
fail at_start {
    ctx.ast.recovered = True
    ctx.pos += 1
}
)");
    const auto directory = std::filesystem::temp_directory_path() /
        ("ispa-action-context-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::filesystem::remove_all(path); } } cleanup{directory};
    Algorithm::Metadata metadata;
    const auto generated = convert(*descriptor, tree, metadata, directory);
    EXPECT_NE(generated.find("fail_context("), std::string::npos);
    std::ofstream(directory / "run.cpp") << R"(
#include "PluginTest.h"
struct Harness : PluginTest::Parser {
    auto parseRegular(const char* text) {
        PluginTest::Lexer lexer;
        return regular(PluginTest::Lexer::lazy_iterator(lexer, text));
    }
    auto parseStarts(const char* text) {
        PluginTest::Lexer lexer;
        return starts(PluginTest::Lexer::lazy_iterator(lexer, text));
    }
    auto parseOptional(const char* text) {
        PluginTest::Lexer lexer;
        return optional_rule(PluginTest::Lexer::lazy_iterator(lexer, text));
    }
};
int main() {
    PluginTest::Parser parser;
    const auto& good = parser.parse("ab").value.data();
    if (good.total != 7 || good.first.empty() || !good.failed.empty()) return 1;
    const auto& repaired = parser.parse("axb").value.data();
    if (repaired.total != 7 || !repaired.recovered || !repaired.had_previous || repaired.failed != "B" || repaired.owner != "item") return 2;
    if (!repaired.named || !repaired.named_string || repaired.wrong_literal) return 8;
    const auto& again = parser.parse("ab").value.data();
    if (again.total != 7 || !again.failed.empty()) return 3;
    PluginTest::Lexer lexer;
    lexer.setInput("axb");
    lexer.makeTokens();
    if (parser.parse(lexer).value.data().total != 7) return 4;
    Harness harness;
    const auto partial = harness.parseRegular("a");
    if (!partial.status || !partial.node.data().saw_token || partial.node.data().expected != "suffix" ||
        !partial.node.data().prior_is_a || !partial.node.data().prior_literal) return 5;
    const auto initial = harness.parseStarts("xa");
    if (!initial.status || !initial.node.data().value.data().recovered || !initial.node.data().value.data().finished) return 6;
    const auto absent = harness.parseOptional("b");
    if (!absent.status || !absent.node.data().value.empty()) return 7;
}
)";
    const auto command = std::string("clang++ -std=c++23 -I\"") + ROOT_DIR +
        "/parser/stdlibs/target/target-cpp\" -I\"" + directory.string() + "\" \"" + (directory / "PluginTest.cpp").string() +
        "\" \"" + (directory / "run.cpp").string() + "\" \"" + ROOT_DIR +
        "/parser/stdlibs/target/target-cpp/src/core.cpp\" \"" + std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp/src/ispa_core_lowered.cpp\" -o \"" + (directory / "run").string() + "\"";
    ASSERT_EQ(std::system(command.c_str()), 0);
    EXPECT_EQ(std::system(('"' + (directory / "run").string() + '"').c_str()), 0);
}

TEST(AlgorithmPlugin, LLAdvancedLookaheadCanInspectThenRestoreTokens) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    auto tree = pluginGrammar(R"(
A: 'a'; B: 'b';
main {
    ctx.mark("start")
    first = ctx.take()
    second = ctx.peek()
    ctx.ast.first: str = first.text
    ctx.ast.second: str = second.text
    ctx.ast.first_id: bool = first.name == A
    ctx.ast.first_literal: bool = first.name == "a"
    ctx.ast.first_wrong_literal: bool = first.name == "A"
    ctx.restore("start")
    ctx.commit("start")
    ctx.ast.at_id: bool = ctx.at(A)
    ctx.ast.at_literal: bool = ctx.at("a")
    ctx.ast.at_wrong_literal: bool = ctx.at("A")
    A = "a"
    ctx.ast.local_literal: bool = ctx.at(A)
    ctx.ast.local_name_equal: bool = first.name == A
    match A;
    match B;
}

)");
    const auto directory = std::filesystem::temp_directory_path() /
        ("ispa-action-lookahead-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::filesystem::remove_all(path); } } cleanup{directory};
    Algorithm::Metadata metadata;
    convert(*descriptor, tree, metadata, directory);
    std::ofstream(directory / "run.cpp") << R"(
#include "PluginTest.h"
int main() {
    PluginTest::Parser parser;
    const auto& value = parser.parse("ab");
    return value.first == "a" && value.second == "b" && value.first_id &&
        value.first_literal && !value.first_wrong_literal && value.at_id &&
        value.at_literal && !value.at_wrong_literal && value.local_literal &&
        !value.local_name_equal ? 0 : 1;
}
)";
    const auto command = std::string("clang++ -std=c++23 -I\"") + ROOT_DIR +
        "/parser/stdlibs/target/target-cpp\" -I\"" + directory.string() + "\" \"" +
        (directory / "PluginTest.cpp").string() + "\" \"" + (directory / "run.cpp").string() +
        "\" \"" + ROOT_DIR + "/parser/stdlibs/target/target-cpp/src/core.cpp\" \"" + std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp/src/ispa_core_lowered.cpp\" -o \"" +
        (directory / "run").string() + "\"";
    ASSERT_EQ(std::system(command.c_str()), 0);
    EXPECT_EQ(std::system(('"' + (directory / "run").string() + '"').c_str()), 0);
}

TEST(AlgorithmPlugin, LLRegularFailBlockCanInspectPreviousToken) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    auto tree = pluginGrammar(R"(
A: 'a'; B: 'b';
main: @ A B %{missing};
fail missing {
    prior = ctx.prev()
    ctx.ast.prior_id: bool = prior.name == A
    ctx.ast.prior_text: bool = prior.name == "a"
    ctx.take()
    ctx.insert()
}
)");
    const auto directory = std::filesystem::temp_directory_path() /
        ("ispa-regular-prev-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::filesystem::remove_all(path); } } cleanup{directory};
    Algorithm::Metadata metadata;
    convert(*descriptor, tree, metadata, directory);
    std::ofstream(directory / "run.cpp") << R"(
#include "PluginTest.h"
int main() {
    PluginTest::Parser parser;
    const auto& result = parser.parse("a");
    return result.prior_id && result.prior_text ? 0 : 1;
}
)";
    const auto command = std::string("clang++ -std=c++23 -I\"") + ROOT_DIR +
        "/parser/stdlibs/target/target-cpp\" -I\"" + directory.string() + "\" \"" +
        (directory / "PluginTest.cpp").string() + "\" \"" + (directory / "run.cpp").string() +
        "\" \"" + ROOT_DIR + "/parser/stdlibs/target/target-cpp/src/core.cpp\" \"" + std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp/src/ispa_core_lowered.cpp\" -o \"" +
        (directory / "run").string() + "\"";
    ASSERT_EQ(std::system(command.c_str()), 0);
    EXPECT_EQ(std::system(('"' + (directory / "run").string() + '"').c_str()), 0);
}

TEST(AlgorithmPlugin, LLAdvancedTokenActionsRunAtAcceptance) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    auto tree = pluginGrammar(R"isc(
TOKEN {
    ctx.ast.count: int = 7
    match 'x';
    ctx.ast.count += 2
    ctx.ast.active = True
    ctx.ast.owner: str = ctx.rule
    ctx.ast.offset: int = ctx.start_pos
}

main: @ TOKEN {@};
)isc");
    const auto directory = std::filesystem::temp_directory_path() /
        ("ispa-token-action-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::filesystem::remove_all(path); } } cleanup{directory};
    Algorithm::Metadata metadata;
    convert(*descriptor, tree, metadata, directory);
    std::ofstream(directory / "run.cpp") << R"cpp(
#include "PluginTest.h"
int main() {
    PluginTest::Lexer lexer;
    const auto& tokens = lexer.makeTokens("x");
    if (tokens.size() != 2 || !tokens.front().holds<PluginTest::Types::TOKEN>()) return 1;
    const auto& value = tokens.front().get<PluginTest::Types::TOKEN>();
    if (value.count != 9 || !value.active || value.owner != "TOKEN" || value.offset != 0) return 2;
}
)cpp";
    const auto command = std::string("clang++ -std=c++23 -I\"") + std::string(ROOT_DIR) +
        "/parser/stdlibs/target/target-cpp\" -I\"" + directory.string() + "\" \"" +
        (directory / "PluginTest.cpp").string() + "\" \"" +
        (directory / "run.cpp").string() + "\" \"" + std::string(ROOT_DIR) +
        "/parser/stdlibs/target/target-cpp/src/core.cpp\" \"" + std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp/src/ispa_core_lowered.cpp\" -o \"" + (directory / "run").string() + "\"";
    ASSERT_EQ(std::system(command.c_str()), 0);
    EXPECT_EQ(std::system(('"' + (directory / "run").string() + '"').c_str()), 0);
}

TEST(AlgorithmPlugin, LLAdvancedTokensEmitIndentAndDedent) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    auto tree = pluginGrammar(R"isc(
INDENT: '';
DEDENT: '';
A: 'a';
B: 'b';
IGNORE { match 'z'; ctx.skip() }
NEWLINE {
    match '\n' [ \t]*;
    ctx.shared.stack: list[int]
    if ctx.shared.stack.size() == 0:
        ctx.shared.stack.append(0)
    width = 0
    i = 1
    while i < ctx.text.size():
        if ctx.text.substr(i, 1) == ' ':
            width += 1
        if ctx.text.substr(i, 1) == '\t':
            width = (width / 8 + 1) * 8
        i += 1
    if width > ctx.shared.stack[ctx.shared.stack.size() - 1]:
        ctx.shared.stack.append(width)
        ctx.emit('INDENT')
    else:
        while width < ctx.shared.stack[ctx.shared.stack.size() - 1] and ctx.shared.stack.size() > 1:
            ctx.shared.stack.pop()
            ctx.emit('DEDENT')
        if width != ctx.shared.stack[ctx.shared.stack.size() - 1]:
            ctx.error('invalid dedent')
}
END_INPUT {
    match '';
    ctx.shared.stack: list[int]
    while ctx.shared.stack.size() > 1:
        ctx.shared.stack.pop()
        ctx.emit('DEDENT')
}
main: A;
)isc");
    const auto directory = std::filesystem::temp_directory_path() /
        ("ispa-layout-action-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::filesystem::remove_all(path); } } cleanup{directory};
    Algorithm::Metadata metadata;
    convert(*descriptor, tree, metadata, directory);
    std::ofstream(directory / "run.cpp") << R"cpp(
#include "PluginTest.h"
int main() {
    PluginTest::Lexer lexer;
    const auto& tokens = lexer.makeTokens("a\n  b\n    b\n  b\na\n");
    const std::size_t expected[] = {
        PluginTest::Tokens::A::id, PluginTest::Tokens::NEWLINE::id,
        PluginTest::Tokens::INDENT::id, PluginTest::Tokens::B::id, PluginTest::Tokens::NEWLINE::id,
        PluginTest::Tokens::INDENT::id, PluginTest::Tokens::B::id, PluginTest::Tokens::NEWLINE::id,
        PluginTest::Tokens::DEDENT::id, PluginTest::Tokens::B::id, PluginTest::Tokens::NEWLINE::id,
        PluginTest::Tokens::DEDENT::id, PluginTest::Tokens::A::id, PluginTest::Tokens::NEWLINE::id
    };
    if (tokens.size() != std::size(expected) + 1) return 1;
    for (std::size_t i = 0; i < std::size(expected); ++i)
        if (tokens[i].meta().name() != expected[i]) return 2;
    if (tokens[2].meta().startpos() != 4 || tokens[2].meta().line() != 2) return 5;
    PluginTest::Lexer lazy_lexer;
    PluginTest::Lexer::lazy_iterator lazy(lazy_lexer, "a\n  b\n    b\n  b\na\n");
    for (std::size_t i = 0; i < std::size(expected); ++i, ++lazy)
        if (lazy.isEnd() || lazy->meta().name() != expected[i]) return 8;
    if (!lazy.isEnd()) return 9;
    PluginTest::Lexer at_eof;
    const auto& unfinished = at_eof.makeTokens("a\n  b");
    if (unfinished.size() < 2 || unfinished[unfinished.size() - 2].meta().name() != PluginTest::Tokens::DEDENT::id) return 3;
    PluginTest::Lexer skipped;
    const auto& without_ignored = skipped.makeTokens("azb");
    if (without_ignored.size() != 3 || without_ignored[0].meta().name() != PluginTest::Tokens::A::id ||
        without_ignored[1].meta().name() != PluginTest::Tokens::B::id) return 4;
    PluginTest::Lexer tabs;
    const auto& tabbed = tabs.makeTokens("a\n\tb\n");
    if (tabbed.size() != 7 || tabbed[2].meta().name() != PluginTest::Tokens::INDENT::id ||
        tabbed[5].meta().name() != PluginTest::Tokens::DEDENT::id) return 6;
    try {
        PluginTest::Lexer invalid;
        invalid.makeTokens("a\n    b\n  b");
        return 7;
    } catch (const std::runtime_error&) {}
}
)cpp";
    const auto command = std::string("clang++ -std=c++23 -I\"") + std::string(ROOT_DIR) +
        "/parser/stdlibs/target/target-cpp\" -I\"" + directory.string() + "\" \"" +
        (directory / "PluginTest.cpp").string() + "\" \"" +
        (directory / "run.cpp").string() + "\" \"" + std::string(ROOT_DIR) +
        "/parser/stdlibs/target/target-cpp/src/core.cpp\" \"" + std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp/src/ispa_core_lowered.cpp\" -o \"" + (directory / "run").string() + "\"";
    ASSERT_EQ(std::system(command.c_str()), 0);
    EXPECT_EQ(std::system(('"' + (directory / "run").string() + '"').c_str()), 0);
}

TEST(AlgorithmPlugin, LLSharedTokenDeclarationsRejectConflictingTypes) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    auto tree = pluginGrammar(R"isc(
A { match 'a'; ctx.shared.level: int }
B { match 'b'; ctx.shared.level: str }
main: A B;
)isc");
    const auto directory = std::filesystem::temp_directory_path() /
        ("ispa-shared-types-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::filesystem::remove_all(path); } } cleanup{directory};
    Algorithm::Metadata metadata;
    try {
        convert(*descriptor, tree, metadata, directory);
        FAIL() << "conflicting ctx.shared declarations were accepted";
    } catch (const std::invalid_argument& error) {
        EXPECT_NE(std::string(error.what()).find("conflicting types for ctx.shared.level"), std::string::npos);
    }
}

TEST(AlgorithmPlugin, CapturedLiteralChoiceRetainsRuleAndTokenPayloads) {
    for (const std::string name : {"LL", "LR", "LALR"}) {
        SCOPED_TRACE(name);
        dlib library("libispa-algorithm-" + name);
        const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
        auto tree = pluginGrammar("main: @ unpack | ';' {@}; unpack: 'x'; other: ';';");
        const auto directory = std::filesystem::temp_directory_path() /
            ("ispa-literal-choice-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        struct Cleanup {
            std::filesystem::path path;
            ~Cleanup() { std::filesystem::remove_all(path); }
        } cleanup{directory};
        Algorithm::Metadata metadata;
        convert(*descriptor, tree, metadata, directory);
        std::ofstream(directory / "run.cpp") << R"(
#include "PluginTest.h"
int main() {
    PluginTest::Parser parser;
    using Unpack = ISPA_STD::Node<PluginTest::Types::unpack>;
    using Semicolon = ISPA_STD::Node<PluginTest::Types::SEMICOLON>;
    if (!std::holds_alternative<Unpack>(parser.parse("x").value)) return 1;
    const auto& result = parser.parse(";").value;
    if (!std::holds_alternative<Semicolon>(result)) return 2;
    if (std::get<Semicolon>(result).empty()) return 3;
}
)";
        const auto command = std::string("clang++ -std=c++23 -I\"") +
            std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp\" -I\"" + directory.string() +
            "\" \"" + (directory / "PluginTest.cpp").string() + "\" \"" +
            (directory / "run.cpp").string() + "\" \"" + std::string(ROOT_DIR) +
            "/parser/stdlibs/target/target-cpp/src/core.cpp\" \"" + std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp/src/ispa_core_lowered.cpp\" -o \"" + (directory / "run").string() + "\"";
        ASSERT_EQ(std::system(command.c_str()), 0);
        EXPECT_EQ(std::system(('"' + (directory / "run").string() + '"').c_str()), 0);
    }
}

TEST(AlgorithmPlugin, EverySupportedAlgorithmSuppliesItsOwnConversion) {
    for (const std::string name : {"LL", "LR", "LALR"}) {
        SCOPED_TRACE(name);
        dlib library("libispa-algorithm-" + name);
        const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
        ASSERT_NE(descriptor, nullptr);
        EXPECT_EQ(descriptor->version, Algorithm::interface_version);
        EXPECT_EQ(descriptor->id, name);
        auto tree = pluginGrammar("A:'a'; main: A;");
        Algorithm::Metadata metadata;
        EXPECT_FALSE(convert(*descriptor, tree, metadata).empty());
    }
}

TEST(AlgorithmPlugin, LLRestoresCacheAndRebuildsChangedOrCorruptInput) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    auto first = pluginGrammar("A:'a'; B:'b'; main: A;");
    Algorithm::Metadata metadata;
    const auto cold = convert(*descriptor, first, metadata);
    ASSERT_FALSE(metadata.cache.empty());
    EXPECT_EQ(convert(*descriptor, first, metadata), cold);
    auto changed = pluginGrammar("A:'a'; B:'b'; main: B;");
    const auto rebuilt = convert(*descriptor, changed, metadata);
    EXPECT_NE(rebuilt, cold);
    metadata.cache.at("productions") = "truncated";
    EXPECT_EQ(convert(*descriptor, changed, metadata), rebuilt);
}

TEST(AlgorithmPlugin, HostPersistsOpaqueBinaryMetadataAndPrunesRemovedKeys) {
    const auto directory = std::filesystem::temp_directory_path() /
        ("ispa-plugin-cache-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::filesystem::remove_all(path); }
    } cleanup{directory};
    Algorithm::Metadata metadata{{{"first", std::string("a\0b", 3)}, {"second", "old"}}};
    Algorithm::saveMetadata(directory, metadata);
    EXPECT_EQ(Algorithm::loadMetadata(directory).cache, metadata.cache);
    metadata.cache.erase("second");
    metadata.cache["first"] = "updated";
    Algorithm::saveMetadata(directory, metadata);
    EXPECT_EQ(Algorithm::loadMetadata(directory).cache, metadata.cache);
    EXPECT_FALSE(std::filesystem::exists(directory / "second"));
    metadata.cache["../escape"] = "invalid";
    EXPECT_THROW(Algorithm::saveMetadata(directory, metadata), std::invalid_argument);
}

TEST(AlgorithmPlugin, LRFamilyPersistsTablesAndRegeneratesSemanticOutput) {
    for (const std::string name : {"LR", "LALR"}) {
        SCOPED_TRACE(name);
        dlib library("libispa-algorithm-" + name);
        const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
        auto tree = pluginGrammar("A:'a'; main:@ A {@};");
        Algorithm::Metadata metadata;
        const auto cold = convert(*descriptor, tree, metadata);
        ASSERT_TRUE(metadata.cache.contains("tables"));
        EXPECT_FALSE(metadata.cache.at("tables").empty());
        EXPECT_EQ(convert(*descriptor, tree, metadata), cold);
        auto edited = pluginGrammar("A:'a'; main:@ A @{renamed};");
        const auto incremental = convert(*descriptor, edited, metadata);
        Algorithm::Metadata fresh;
        EXPECT_EQ(incremental, convert(*descriptor, edited, fresh));
        EXPECT_NE(incremental, cold);
        metadata.cache["tables"] = "corrupt";
        EXPECT_EQ(convert(*descriptor, edited, metadata), incremental);
    }
}

TEST(AlgorithmPlugin, LLBuildsStructuredMembersAndCaptureMetadata) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    // Whitespace emits no assignment; groups and choices emit control flow.
    // Neither should require the Builder to find a parser call in a statement.
    for (const char* source : {
        "A:'a'; B:'b'; main:@ (A B) {@};",
        "A:'a'; B:'b'; main:@ (A | B) {@};",
        "A:'a'; main:@ A* {@};",
        "A:'a'; main:@ A? {@};",
        "A:'a'; main:@ (A)? {@};",
        "A:'a'; main:@ (A)* {@};",
        "A:'a'; main:@ item+ {@}; item:@ A {@};"
    }) {
        SCOPED_TRACE(source);
        auto tree = pluginGrammar(source);
        Algorithm::Metadata metadata;
        AST::TreeMap changed;
        auto built = descriptor->build({tree, changed}, metadata);
        const auto& ir = *static_cast<ParserIR::IR*>(built.output.get());
        const auto production_it = std::find_if(ir.getData().begin(), ir.getData().end(),
            [](const auto& value) { return value.name == std::vector<std::string>{"main"}; });
        ASSERT_NE(production_it, ir.getData().end());
        const auto& production = *production_it;
        ASSERT_TRUE(production.block.is_regular_data_block());
        const auto& capture = production.block.getRegularDataBlock().first;
        ASSERT_FALSE(capture.empty());
        ASSERT_TRUE(capture.front().isRvalue());
        ASSERT_TRUE(capture.front().getRValue().isSymbol());
        bool declared = false;
        for (const auto& statement : production.rule_members)
            if (statement.isVariable() &&
                LangAPI::Symbol{statement.getVariable().name} == capture.front().getRValue().getSymbol())
                declared = true;
        EXPECT_TRUE(declared);
        const auto cold = convert(*descriptor, tree, metadata);
        EXPECT_FALSE(cold.empty());
        EXPECT_EQ(cold.find("Recovery::handler<ISPA_STD::Recovery::Tokens,"), std::string::npos);
        EXPECT_EQ(cold.find("Recovery::Policy>"), std::string::npos);
        EXPECT_EQ(convert(*descriptor, tree, metadata), cold);
    }
}

TEST(AlgorithmPlugin, LLGeneratedRecoveryAndRuleCallbacksCompile) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    auto tree = pluginGrammar(
        "A:'a'; B:'b'; main: item item? item* item+ A+ B* "
        "@ (A B) @ ('x' | 'y') (B @ A)* @{pair, kind, nested}; item:@ A {@}; "
        "recursive: B recursive? A;");
    const auto directory = std::filesystem::temp_directory_path() /
        ("ispa-ll-compile-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::filesystem::remove_all(path); }
    } cleanup{directory};
    Algorithm::Metadata metadata;
    convert(*descriptor, tree, metadata, directory);
    const auto command = std::string("clang++ -std=c++23 -fsyntax-only -I\"") +
        std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp\" -I\"" + directory.string() +
        "\" \"" + (directory / "PluginTest.cpp").string() + "\"";
    EXPECT_EQ(std::system(command.c_str()), 0);
}

TEST(AlgorithmPlugin, LLRecoveryFindsNestedAndEnclosingTerminalsBeyondFollowOne) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    auto tree = pluginGrammar(
        "A:'a'; B:'b'; C:'c'; D:'d'; E:'e'; X:'x'; "
        "main: A item @ E {@}; item: B deep; deep: first | second; "
        "first: C D; second: '(' B ')';");
    const auto directory = std::filesystem::temp_directory_path() /
        ("ispa-ll-recovery-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::filesystem::remove_all(path); }
    } cleanup{directory};
    Algorithm::Metadata metadata;
    convert(*descriptor, tree, metadata, directory);
    std::ofstream(directory / "run.cpp") << R"(
#include "PluginTest.h"
#include <iostream>
int main() {
    PluginTest::Parser parser;
    for (const char* input : {"axxcde", "axxde", "abxxe", "abcdxe", "abxx)e"}) {
        try {
            const auto& root = parser.parse(input);
            if (root.value.empty()) { std::cerr << "lost tail: " << input; return 1; }
            if (parser.getErrors().empty()) { std::cerr << "lost error: " << input; return 2; }
            if (parser.getErrors().front().message.find("expected") == std::string::npos) return 3;
            const auto offset = parser.getErrors().front().offset;
            if (offset >= std::char_traits<char>::length(input) || input[offset] != 'x') return 6;
        } catch (const std::exception& error) {
            std::cerr << input << ": " << error.what(); return 4;
        }
    }
    parser.parse("abcde");
    if (!parser.getErrors().empty() || !parser.getContextErrors().empty()) {
        for (const auto& error : parser.getErrors()) std::cerr << error.message << '\n';
        for (const auto& error : parser.getContextErrors()) std::cerr << error.message << '\n';
        return 5;
    }
}
)";
    const auto command = std::string("clang++ -std=c++23 -I\"") +
        std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp\" -I\"" + directory.string() +
        "\" \"" + (directory / "PluginTest.cpp").string() + "\" \"" +
        (directory / "run.cpp").string() + "\" \"" + std::string(ROOT_DIR) +
        "/parser/stdlibs/target/target-cpp/src/core.cpp\" \"" + std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp/src/ispa_core_lowered.cpp\" -o \"" + (directory / "run").string() + "\"";
    ASSERT_EQ(std::system(command.c_str()), 0);
    EXPECT_EQ(std::system(('"' + (directory / "run").string() + '"').c_str()), 0);
}

TEST(AlgorithmPlugin, LLQuantifiedRulesSkipAbsentChildrenWithoutRecovery) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    auto tree = pluginGrammar(
        "A:'a'; B:'b'; C:'c'; D:'d'; "
        "main: item? B item* C item+ D; item: leaf; leaf: A;");
    const auto directory = std::filesystem::temp_directory_path() /
        ("ispa-ll-optional-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::filesystem::remove_all(path); }
    } cleanup{directory};
    Algorithm::Metadata metadata;
    convert(*descriptor, tree, metadata, directory);
    std::ofstream(directory / "run.cpp") << R"(
#include "PluginTest.h"
int main() {
    for (const char* input : {"bcad", "abaacaad", "b c a d", "a b a a c a a d"}) {
        PluginTest::Parser parser;
        parser.parse(input);
        if (!std::as_const(parser).getErrors().empty()) return 1;
    }
    PluginTest::Parser parser;
    try { parser.parse("bca"); return 2; }
    catch (const std::logic_error&) {}
    if (parser.getErrors().empty()) return 3;
    parser.parse("bcad");
    if (!parser.getErrors().empty()) return 4;

    PluginTest::Lexer lexer;
    lexer.makeTokens("bca");
    try { parser.parse(lexer); return 5; }
    catch (const std::logic_error&) {}
    if (parser.getErrors().empty()) return 6;
    lexer.clearTokens();
    lexer.makeTokens("bcad");
    parser.parse(lexer);
    if (!parser.getErrors().empty()) return 7;
}

)";
    const auto command = std::string("clang++ -std=c++23 -I\"") +
        std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp\" -I\"" + directory.string() +
        "\" \"" + (directory / "PluginTest.cpp").string() + "\" \"" +
        (directory / "run.cpp").string() + "\" \"" + std::string(ROOT_DIR) +
        "/parser/stdlibs/target/target-cpp/src/core.cpp\" \"" + std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp/src/ispa_core_lowered.cpp\" -o \"" + (directory / "run").string() + "\"";
    ASSERT_EQ(std::system(command.c_str()), 0);
    EXPECT_EQ(std::system(('"' + (directory / "run").string() + '"').c_str()), 0);
}

TEST(AlgorithmPlugin, LLListCaptureStopsBeforeFollowingRuleWithSharedPrefix) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    auto tree = pluginGrammar(
        "A:'a'; B:'b'; main: @ item+ @ tail @{items, last}; "
        "item: @ plain | marked {@}; plain: '@'? @ A {@}; "
        "marked: '@' '=' @ A {@}; tail: '@' '{' @ B '}' {@};");
    const auto directory = std::filesystem::temp_directory_path() /
        ("ispa-ll-list-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::filesystem::remove_all(path); }
    } cleanup{directory};
    Algorithm::Metadata metadata;
    convert(*descriptor, tree, metadata, directory);
    std::ofstream(directory / "run.cpp") << R"(
#include "PluginTest.h"
int main() {
    PluginTest::Parser parser;
    if (parser.parse("a@{b}").items.size() != 1) return 1;
    if (parser.parse("a@a@=a@{b}").items.size() != 3) return 2;
    if (parser.parse("a @a @=a @{b}").items.size() != 3) return 3;
}

)";
    const auto command = std::string("clang++ -std=c++23 -I\"") +
        std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp\" -I\"" + directory.string() +
        "\" \"" + (directory / "PluginTest.cpp").string() + "\" \"" +
        (directory / "run.cpp").string() + "\" \"" + std::string(ROOT_DIR) +
        "/parser/stdlibs/target/target-cpp/src/core.cpp\" \"" + std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp/src/ispa_core_lowered.cpp\" -o \"" + (directory / "run").string() + "\"";
    ASSERT_EQ(std::system(command.c_str()), 0);
    EXPECT_EQ(std::system(('"' + (directory / "run").string() + '"').c_str()), 0);
}


TEST(AlgorithmPlugin, LLCountedTokensAndProductionsShareCountsInBothIteratorModes) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    auto tree = pluginGrammar(
        "__WS:' '; NEWLINE:'\\n'; A:'a'; B:'b'; C:'c'; "
        "main: @ A=x ':' @ body @ NEWLINE @ B=2 '!' @{first, body, newline, last}; "
        "body: @ item=x {@}; item: @ C {@}; "
        "aligned: @ __WS=x 'header' NEWLINE @ line+ '!' @{spaces, lines}; "
        "line: @ __WS=x 'item' NEWLINE {@};");
    const auto directory = std::filesystem::temp_directory_path() /
        ("ispa-counted-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::filesystem::remove_all(path); }
    } cleanup{directory};
    Algorithm::Metadata metadata;
    const auto cold = convert(*descriptor, tree, metadata, directory);
    EXPECT_EQ(convert(*descriptor, tree, metadata), cold);
    std::ofstream(directory / "run.cpp") << R"(
#include "PluginTest.h"
#include <iostream>
struct Exposed : PluginTest::Parser { using PluginTest::Parser::aligned; };
int main() {
    PluginTest::Parser parser;
    for (const bool lazy : {true, false}) {
        const auto run = [&](const char* input) -> const PluginTest::Types::main& {
            if (lazy) return parser.parse(input);
            static PluginTest::Lexer lexer;
            lexer.clearTokens();
            lexer.makeTokens(input);
            return parser.parse(lexer);
        };
        for (const auto& [input, count] : {std::pair{"aa:cc\nbb!", 2u},
                                         std::pair{"a:c\nb b!", 1u},
                                         std::pair{":\nbb!", 0u}}) {
            const auto& root = run(input);
            if (!parser.getErrors().empty()) return 1;
            if (root.first.size() != count || root.body.data().value.size() != count ||
                root.last.size() != 2) return 2;
        }
        for (const char* input : {"aa:c\nbb!", "a:cc\nbb!", "a:c\nb!", "a:c\nbbb!", "a:c \nbb!"}) {
            try { run(input); if (parser.getErrors().empty()) { std::cerr << input; return 3; } }
            catch (const std::logic_error&) {}
        }
        run("aaa:ccc\nbb!");
        if (!parser.getErrors().empty()) return 4;
        Exposed aligned;
        PluginTest::Lexer indentation;
        const char* input = "  header\n  item\n  item\n!";
        if (lazy) {
            auto result = aligned.aligned(PluginTest::Lexer::lazy_iterator(indentation, input));
            if (!result.status || !result.it.isEnd() || result.node.data().lines.size() != 2) return 5;
        } else {
            indentation.makeTokens(input);
            auto result = aligned.aligned(PluginTest::Lexer::iterator(indentation));
            if (!result.status || !result.it.isEnd() || result.node.data().lines.size() != 2) return 6;
        }
        if (!aligned.getErrors().empty()) return 7;
    }
}
)";
    const auto command = std::string("clang++ -std=c++23 -I\"") +
        std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp\" -I\"" + directory.string() +
        "\" \"" + (directory / "PluginTest.cpp").string() + "\" \"" +
        (directory / "run.cpp").string() + "\" \"" + std::string(ROOT_DIR) +
        "/parser/stdlibs/target/target-cpp/src/core.cpp\" \"" + std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp/src/ispa_core_lowered.cpp\" -o \"" + (directory / "run").string() + "\"";
    ASSERT_EQ(std::system(command.c_str()), 0);
    EXPECT_EQ(std::system(('"' + (directory / "run").string() + '"').c_str()), 0);
}

TEST(AlgorithmPlugin, LLCounterChangesInvalidateCachedProductions) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    auto first = pluginGrammar("A:'a'; main: A=2 '!';");
    auto second = pluginGrammar("A:'a'; main: A=3 '!';");
    Algorithm::Metadata metadata;
    const auto old = convert(*descriptor, first, metadata);
    const auto updated = convert(*descriptor, second, metadata);
    EXPECT_NE(old, updated);
    EXPECT_EQ(convert(*descriptor, second, metadata), updated);
}

TEST(AlgorithmPlugin, LLFixedCountsHaveFiniteLookaheadIncludingZero) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    for (const char* source : {
        "A:'a'; B:'b'; main: small | large; small: A=2 B; large: A=3 B;",
        "A:'a'; B:'b'; main: empty | full; empty: A=0 B; full: A B;"
    }) {
        auto tree = pluginGrammar(source);
        Algorithm::Metadata metadata;
        EXPECT_FALSE(convert(*descriptor, tree, metadata).empty());
    }
}

TEST(AlgorithmPlugin, LRFamilyRejectsCountedRepetitions) {
    for (const std::string name : {"LR", "LALR"}) {
        dlib library("libispa-algorithm-" + name);
        const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
        auto tree = pluginGrammar("A:'a'; main: A=x '!';");
        Algorithm::Metadata metadata;
        EXPECT_THROW(convert(*descriptor, tree, metadata), std::invalid_argument);
    }
}

TEST(AlgorithmPlugin, LRFamilyRejectsSemanticActionsThroughPluginEntryPoint) {
    for (const std::string name : {"LR", "LALR"}) {
        SCOPED_TRACE(name);
        dlib library("libispa-algorithm-" + name);
        const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
        auto tree = pluginGrammar("main { match 'a'; ctx.ast.seen: bool = True }");
        Algorithm::Metadata metadata;
        try {
            convert(*descriptor, tree, metadata);
            FAIL() << "an action was silently omitted";
        } catch (const std::runtime_error& error) {
            EXPECT_NE(std::string(error.what()).find("Python action"), std::string::npos);
        }
    }
}

TEST(AlgorithmPlugin, LRFamilyCompilesMatchOnlyAdvancedRules) {
    for (const std::string name : {"LR", "LALR"}) {
        SCOPED_TRACE(name);
        dlib library("libispa-algorithm-" + name);
        const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
        auto tree = pluginGrammar("main { match 'a'; match 'b'; }");
        const auto directory = std::filesystem::temp_directory_path() /
            ("ispa-lr-advanced-match-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        struct Cleanup { std::filesystem::path path; ~Cleanup() { std::filesystem::remove_all(path); } } cleanup{directory};
        Algorithm::Metadata metadata;
        EXPECT_NO_THROW(convert(*descriptor, tree, metadata, directory));
        std::ofstream(directory / "run.cpp") << R"cpp(
#include "PluginTest.h"
#include <stdexcept>
int main() {
    PluginTest::Parser parser;
    parser.parse("ab");
    try { parser.parse("a"); return 1; }
    catch (const std::runtime_error&) { return 0; }
}
)cpp";
        const auto command = std::string("clang++ -std=c++23 -I\"") + std::string(ROOT_DIR) +
            "/parser/stdlibs/target/target-cpp\" -I\"" + directory.string() + "\" \"" +
            (directory / "PluginTest.cpp").string() + "\" \"" +
            (directory / "run.cpp").string() + "\" \"" + std::string(ROOT_DIR) +
            "/parser/stdlibs/target/target-cpp/src/core.cpp\" \"" + std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp/src/ispa_core_lowered.cpp\" -o \"" + (directory / "run").string() + "\"";
        ASSERT_EQ(std::system(command.c_str()), 0);
        EXPECT_EQ(std::system(('"' + (directory / "run").string() + '"').c_str()), 0);
    }
}

TEST(AlgorithmPlugin, LRFamilyGeneratedParsersReportExpectedTokensAndRule) {
    for (const std::string name : {"LR", "LALR"}) {
        SCOPED_TRACE(name);
        dlib library("libispa-algorithm-" + name);
        const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
        auto tree = pluginGrammar("A:'a'; B:'b'; main: A;");
        const auto directory = std::filesystem::temp_directory_path() /
            ("ispa-lr-diagnostic-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        struct Cleanup { std::filesystem::path path; ~Cleanup() { std::filesystem::remove_all(path); } } cleanup{directory};
        Algorithm::Metadata metadata;
        convert(*descriptor, tree, metadata, directory);
        std::ofstream(directory / "run.cpp") << R"cpp(
#include "PluginTest.h"
#include <stdexcept>
#include <string>
int main() {
    PluginTest::Parser parser;
    try { parser.parse("b"); return 1; }
    catch (const std::runtime_error& error) {
        const std::string message = error.what();
        if (message.find("expected A") == std::string::npos ||
            message.find("in rule main") == std::string::npos) return 2;
    }
    if (parser.getErrors().size() != 1 || parser.getErrors()[0].offset != 0 ||
        parser.getErrors()[0].code != "lr-unexpected-token") return 3;
    parser.parse("a");
    if (!parser.getErrors().empty()) return 4;
}
)cpp";
        const auto command = std::string("clang++ -std=c++23 -I\"") + std::string(ROOT_DIR) +
            "/parser/stdlibs/target/target-cpp\" -I\"" + directory.string() + "\" \"" +
            (directory / "PluginTest.cpp").string() + "\" \"" +
            (directory / "run.cpp").string() + "\" \"" + std::string(ROOT_DIR) +
            "/parser/stdlibs/target/target-cpp/src/core.cpp\" \"" + std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp/src/ispa_core_lowered.cpp\" -o \"" + (directory / "run").string() + "\"";
        ASSERT_EQ(std::system(command.c_str()), 0);
        EXPECT_EQ(std::system(('"' + (directory / "run").string() + '"').c_str()), 0);
    }
}

TEST(AlgorithmPlugin, LLMatchesCompleteNestedTokenStreamsWithoutMovingOuterCursor) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    auto tree = pluginGrammar(R"(
main {
    match '{';
    tokens = ctx.collect_braced()
    if True:
        match(tokens) inner -> found;
        ctx.ast.value = found
}
inner: simple | nested;
simple: 'x';
nested: '{' 'x' '}';
)");
    const auto directory = std::filesystem::temp_directory_path() /
        ("ispa-stream-match-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::filesystem::remove_all(path); } } cleanup{directory};
    Algorithm::Metadata metadata;
    convert(*descriptor, tree, metadata, directory);
    std::ofstream(directory / "run.cpp") << R"(
#include "PluginTest.h"
int main() {
    PluginTest::Parser parser;
    if (parser.parse("{x}").value.empty()) return 1;
    if (parser.parse("{{x}}").value.empty()) return 2;
    if (!parser.parse("{xx}").value.empty()) return 3;
}
)";
    const auto command = std::string("clang++ -std=c++23 -I\"") + ROOT_DIR +
        "/parser/stdlibs/target/target-cpp\" -I\"" + directory.string() + "\" \"" +
        (directory / "PluginTest.cpp").string() + "\" \"" +
        (directory / "run.cpp").string() + "\" \"" + ROOT_DIR +
        "/parser/stdlibs/target/target-cpp/src/core.cpp\" \"" + std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp/src/ispa_core_lowered.cpp\" -o \"" + (directory / "run").string() + "\"";
    ASSERT_EQ(std::system(command.c_str()), 0);
    EXPECT_EQ(std::system(('"' + (directory / "run").string() + '"').c_str()), 0);
}

TEST(AlgorithmPlugin, LLMatchesLiteralInsidePythonBlock) {
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    auto tree = pluginGrammar(R"(
main {
    if True:
        match @ 'x'? -> token;
        if not token.empty():
            ctx.ast.value = token
}
)");
    const auto directory = std::filesystem::temp_directory_path() /
        ("ispa-nested-match-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::filesystem::remove_all(path); } } cleanup{directory};
    Algorithm::Metadata metadata;
    convert(*descriptor, tree, metadata, directory);
    std::ofstream(directory / "run.cpp") << R"(
#include "PluginTest.h"
int main() {
    PluginTest::Parser parser;
    if (parser.parse("x").value.empty()) return 1;
    if (!parser.parse("").value.empty()) return 2;
}
)";
    const auto command = std::string("clang++ -std=c++23 -I\"") + ROOT_DIR +
        "/parser/stdlibs/target/target-cpp\" -I\"" + directory.string() + "\" \"" +
        (directory / "PluginTest.cpp").string() + "\" \"" +
        (directory / "run.cpp").string() + "\" \"" + ROOT_DIR +
        "/parser/stdlibs/target/target-cpp/src/core.cpp\" \"" + std::string(ROOT_DIR) + "/parser/stdlibs/target/target-cpp/src/ispa_core_lowered.cpp\" -o \"" + (directory / "run").string() + "\"";
    ASSERT_EQ(std::system(command.c_str()), 0);
    EXPECT_EQ(std::system(('"' + (directory / "run").string() + '"').c_str()), 0);
}
