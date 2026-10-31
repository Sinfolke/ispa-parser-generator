#include <gtest/gtest.h>
#include <output/Parser.h>
import UAST.Parser;
import AST.Builder;
import AST.Tree;
import AST.BackendGuard;
import std;
namespace {
AST::Tree convert(const char* source) {
    Parser::Parser parser;
    AST::Builder builder(parser.parse(source), "feature.isc");
    builder.build();
    return builder.get();
}
}

TEST(UAST, BackendGuardAllowsSupportedGrammar) {
    auto tree = convert("T: @ ('a' | 'b')+ {@}; main: @ T {@};");
    EXPECT_NO_THROW(AST::requireCppSupport(tree));
    auto counted = convert("A:'a'; main: A=x A=3 A* => saved;");
    EXPECT_NO_THROW(AST::requireCppSupport(counted));
    auto advanced_token = convert("T { ctx.ast.seen: bool = True\n match 'x'; } main: T;");
    EXPECT_NO_THROW(AST::requireCppSupport(advanced_token, true));
    // lexing does not depend on the parsing algorithm: LR gets the same token actions
    EXPECT_NO_THROW(AST::requireCppSupport(advanced_token, false));
    auto empty_token = convert("T { ctx.ast.seen: bool = True } main: T;");
    EXPECT_THROW(AST::requireCppSupport(empty_token, true), std::runtime_error);
    EXPECT_THROW(AST::requireCppSupport(empty_token, false), std::runtime_error);
    auto nested_token = convert("T { match 'x'; ctx.ast.seen: bool = True } U: T; main: U;");
    EXPECT_THROW(AST::requireCppSupport(nested_token, true), std::runtime_error);
}

TEST(UAST, LRBackendGuardAllowsMatchOnlyAdvancedRule) {
    auto advanced_match = convert("main { match 'a'; match 'b'; }");
    EXPECT_NO_THROW(AST::requireCppSupport(advanced_match, false));
    // statements run at the reduction of their mid-rule nonterminal
    auto advanced_action = convert("main { match 'a'; ctx.ast.seen: bool = True }");
    EXPECT_NO_THROW(AST::requireCppSupport(advanced_action, false));
    auto advanced_binding = convert("A: 'a'; main { match A -> first; ctx.ast.seen = first }");
    EXPECT_NO_THROW(AST::requireCppSupport(advanced_binding, false));
}

TEST(UAST, BackendGuardRejectsUnsupportedFeaturesWithPositions) {
    for (const auto* source : {
        "T: $name;", "T: @=('a');", "T: % 'a';",
        "T: 'a' %{handler};", "T: 'a'+=3;", "T: 'a' => count;",
        "T: 'a' {py};", "T: \\s;"
    }) {
        auto tree = convert(source);
        try {
            AST::requireCppSupport(tree);
            FAIL() << source;
        } catch (const std::runtime_error& error) {
            const std::string message = error.what();
            EXPECT_NE(message.find("feature.isc:1:"), std::string::npos) << message;
            EXPECT_NE(message.find("is not yet supported by the C++ backend"), std::string::npos) << message;
        }
    }
}
