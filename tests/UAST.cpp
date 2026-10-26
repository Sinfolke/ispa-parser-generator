#include <gtest/gtest.h>
#include <ispa/parser.h>
import UAST;
import Modules;
import UAST.Parser;
import UAST.Python;
import AST.Builder;
import AST.API;
import AST.Tree;
import AST.Pass;
import dstd;
import std;

namespace {
AST::Tree convert(const char* source) {
    Parser::Parser parser;
    AST::Builder builder(parser.parse(source), "feature.isc");
    builder.build();
    return builder.get();
}
template<class T> const T& feature(const AST::RuleMember& member) {
    for (const auto& f : member.features)
        if (const auto* value = std::get_if<T>(&f)) return *value;
    throw std::runtime_error("missing expected feature");
}
}

TEST(UAST, PlainEbnfAndCapture) {
    Parser::Parser parser;
    auto syntax = parser.parse("WORD: @ 'a'+ {@};");
    AST::Builder builder(syntax);
    builder.build();
    auto tree = builder.get();
    const auto& rule = tree.getTreeMap().at({"WORD"});
    ASSERT_EQ(rule.rule_members.size(), 1u);
    const auto& member = *rule.rule_members.front();
    EXPECT_TRUE(member.prefix.present);
    EXPECT_EQ(member.quantifier, '+');
    ASSERT_TRUE(member.isString());
    EXPECT_EQ(member.getString().value, "a");
    ASSERT_TRUE(rule.data_block.isRegularDataBlock());
    EXPECT_TRUE(std::holds_alternative<AST::At>(rule.data_block.getRegDataBlock()));
    ASSERT_TRUE(rule.data_block.type.has_value());
    EXPECT_EQ(rule.data_block.type->kind, AST::DataType::Kind::Character);
}

TEST(UAST, LiteralChoiceCaptureUsesFinalPromotedTokenName) {
    auto tree = convert("main: @ unpack | ';' {@}; unpack: 'x'; other: ';';");
    AST::TreePass::literalsToToken(tree);
    const auto& rule = tree.getTreeMap().at({"main"});
    const auto& choice = *rule.rule_members.front();
    ASSERT_TRUE(choice.isOp());
    const auto& token = choice.getOp().options.at(1)->getName().name;
    ASSERT_EQ(token, (stdu::vector<std::string>{"SEMICOLON"}));
    ASSERT_TRUE(rule.data_block.type.has_value());
    const auto& type = *rule.data_block.type;
    ASSERT_EQ(type.kind, AST::DataType::Kind::Variant);
    ASSERT_EQ(type.parameters.size(), 2u);
    EXPECT_EQ(type.parameters[0].kind, AST::DataType::Kind::Rule);
    EXPECT_EQ(type.parameters[0].name, (stdu::vector<std::string>{"unpack"}));
    EXPECT_EQ(type.parameters[1].kind, AST::DataType::Kind::Token);
    EXPECT_EQ(type.parameters[1].name, token);
}

TEST(UAST, TokenPromotionRefreshesParserFieldsAndPreservesLexerCaptureTypes) {
    auto tree = convert("main: @ 'x' @ ('a' | 'b')+ @{literal, choices}; WORD: @ 'w'+ {@};");
    AST::TreePass pass(tree);
    pass.work();
    const auto& fields = tree.getTreeMap().at({"main"}).data_block.fields;
    ASSERT_EQ(fields.size(), 2u);
    EXPECT_EQ(fields[0].second.kind, AST::DataType::Kind::Token);
    EXPECT_EQ(fields[0].second.name, tree.getTreeMap().at({"main"}).rule_members[0]->getName().name);
    const auto& choices = fields[1].second;
    ASSERT_EQ(choices.kind, AST::DataType::Kind::Array);
    ASSERT_EQ(choices.parameters.size(), 1u);
    EXPECT_EQ(choices.parameters.front().kind, AST::DataType::Kind::Variant);
    EXPECT_EQ(tree.getTreeMap().at({"WORD"}).data_block.type->kind, AST::DataType::Kind::Character);
    AST::inferDataTypes(tree.getTreeMap().at({"main"}));
    EXPECT_EQ(tree.getTreeMap().at({"main"}).data_block.fields.size(), 2u);
}

TEST(UAST, OwnsParserDataAndPositions) {
    UAST::main syntax;
    {
        std::string source = "WORD: 'a';";
        Parser::Parser parser;
        syntax = parser.parse(source.c_str()).value.data();
    }
    const auto& declaration = std::get<UAST::Node<UAST::grammar>>(syntax.value.at(0));
    EXPECT_EQ(UAST::positionOf(declaration.data().name.data().primary, "owned.isc").file, "owned.isc");
    EXPECT_EQ(UAST::positionOf(declaration.data().name.data().primary, "owned.isc").offset, 0u);
    EXPECT_EQ(UAST::positionOf(declaration.data().name.data().primary, "owned.isc").line, 1u);
    EXPECT_EQ(UAST::positionOf(declaration.data().name.data().primary, "owned.isc").column, 1u);
    EXPECT_FALSE(declaration.empty());
    EXPECT_EQ(declaration.data().name.data().primary.data().value, "WORD");
    AST::Builder builder(syntax);
    EXPECT_NO_THROW(builder.build());
}

TEST(UAST, FailBindingsAndMarkers) {
    auto tree = convert("T: % 'a' %handler 'b' %{handler, fallback};");
    const auto& rule = tree.getTreeMap().at({"T"});
    ASSERT_EQ(rule.rule_members.size(), 2u);
    EXPECT_TRUE(feature<AST::FailMarker>(*rule.rule_members[0]).name.empty());
    EXPECT_EQ(feature<AST::FailMarker>(*rule.rule_members[1]).name.front(), "handler");
    ASSERT_EQ(rule.features.size(), 1u);
    const auto& binding = std::get<AST::FailBinding>(rule.features.front());
    EXPECT_EQ(binding.names.size(), 2u);
    EXPECT_EQ(binding.position.file, "feature.isc");
}

TEST(UAST, RegionCapture) {
    auto tree = convert("T: @=('a' 'b');");
    const auto& member = *tree.getTreeMap().at({"T"}).rule_members.front();
    ASSERT_TRUE(member.isGroup());
    EXPECT_EQ(member.getGroup().values.size(), 2u);
    EXPECT_EQ(feature<AST::RegionCapture>(member).capture, '@');
    auto failTree = convert("T: %=('a' 'b');");
    const auto& fail = *failTree.getTreeMap().at({"T"}).rule_members.front();
    EXPECT_EQ(feature<AST::RegionCapture>(fail).capture, '%');
}

TEST(UAST, DynamicPattern) {
    auto tree = convert("T: $name;");
    const auto& member = *tree.getTreeMap().at({"T"}).rule_members.front();
    EXPECT_EQ(feature<AST::DynamicPattern>(member).name, "name");
    EXPECT_FALSE(member.empty());
}

TEST(UAST, KeepsCombinedCapturesAndDeclarationParameters) {
    auto tree = convert("#[inline] T<type>>(arg): &value @node 'a';");
    const auto& rule = tree.getTreeMap().at({"T"});
    const auto& member = *rule.rule_members.front();
    EXPECT_EQ(member.prefix.name, "node");
    EXPECT_EQ(feature<AST::VariableCapture>(member).name, "value");
    const auto& declaration = std::get<AST::DeclarationSyntax>(rule.features.front());
    EXPECT_EQ(declaration.modifiers.front(), "inline");
    EXPECT_EQ(declaration.parameters.front(), "arg");
    EXPECT_EQ(declaration.templateParameters.front(), "type");
}

TEST(UAST, PreservesUnknownReferences) {
    auto tree = convert("T: externallyQualifiedName;");
    const auto& member = *tree.getTreeMap().at({"T"}).rule_members.front();
    ASSERT_TRUE(member.isName());
    EXPECT_EQ(member.getName().name.front(), "externallyQualifiedName");
}

TEST(UAST, ExplicitParametersDoNotConsumeFollowingGrammarGroups) {
    auto tree = convert("main: member>(arg) (OTHER)*; member: 'a'; OTHER: 'b';");
    const auto& members = tree.getTreeMap().at({"main"}).rule_members;
    ASSERT_EQ(members.size(), 2u);
    ASSERT_TRUE(members[0]->isName());
    EXPECT_EQ(feature<AST::DeclarationSyntax>(*members[0]).parameters, (std::vector<std::string>{"arg"}));
    ASSERT_TRUE(members[1]->isGroup());
    EXPECT_EQ(members[1]->quantifier, '*');

    auto plain = convert("main: member (OTHER); member: 'a'; OTHER: 'b';");
    const auto& plain_members = plain.getTreeMap().at({"main"}).rule_members;
    ASSERT_EQ(plain_members.size(), 2u);
    EXPECT_TRUE(plain_members[0]->features.empty());
    EXPECT_TRUE(plain_members[1]->isGroup());
}

TEST(UAST, AmpersandParametersRetainTypeAnnotations) {
    auto tree = convert("ITEM&(value: str, count: int): 'x'; main: ITEM;");
    const auto& rule = tree.getTreeMap().at({"ITEM"});
    const auto& declaration = std::get<AST::DeclarationSyntax>(rule.features.front());
    EXPECT_EQ(declaration.parameters, (std::vector<std::string>{"value", "count"}));
    ASSERT_EQ(declaration.parameterAnnotations.size(), 2u);
    EXPECT_EQ(std::get<UAST::py::Expr>(declaration.parameterAnnotations[0].code).get<UAST::py::Name>()->id, "str");
    EXPECT_EQ(std::get<UAST::py::Expr>(declaration.parameterAnnotations[1].code).get<UAST::py::Name>()->id, "int");
    auto grouped = convert("main: item &(arg) (OTHER)*; item&(value): 'a'; OTHER:'b';");
    const auto& members = grouped.getTreeMap().at({"main"}).rule_members;
    ASSERT_EQ(members.size(), 2u);
    EXPECT_EQ(feature<AST::DeclarationSyntax>(*members[0]).parameters, (std::vector<std::string>{"arg"}));
    EXPECT_TRUE(members[1]->isGroup());
}

TEST(UAST, NestedSiblingRulesMayBeSeparatedByWhitespace) {
    auto tree = convert("outer: ID #a: ID;\n  #b: ID;\n; ID: 'x';");
    EXPECT_TRUE(tree.contains({"outer", "a"}));
    EXPECT_TRUE(tree.contains({"outer", "b"}));
}

TEST(UAST, ResolvesQualifiedAndScopedReferences) {
    auto tree = convert(
        "caller: rule->ast; ast: 'g'; "
        "rule: (ast | other) #ast: 'n'; #other: ast;;");
    const auto& rules = tree.getTreeMap();
    const auto check = [](const AST::RuleMember& member, auto expected) {
        ASSERT_TRUE(member.isName());
        EXPECT_EQ(member.getName().name, expected);
    };
    const auto nested = rules.at({"caller"}).rule_members.front()->getName().name;
    ASSERT_EQ(nested.size(), 2u);
    EXPECT_EQ(nested[0], "rule");
    EXPECT_EQ(nested[1], "ast");
    const auto& group = rules.at({"rule"}).rule_members.front()->getGroup();
    const auto& options = group.values.front()->getOp().options;
    check(*options[0], nested);
    check(*rules.at({"rule", "other"}).rule_members.front(), nested);
    auto other = nested;
    other.back() = "other";
    check(*options[1], other);
}

TEST(UAST, DoesNotResolveQualifiedReferencesBySuffix) {
    auto tree = convert("caller: missing->ast; ast: 'g';");
    const auto& name = tree.getTreeMap().at({"caller"}).rule_members.front()->getName().name;
    ASSERT_EQ(name.size(), 2u);
    EXPECT_EQ(name[0], "missing");
    EXPECT_EQ(name[1], "ast");
}

TEST(UAST, BoundedCountersAndSharedRegistration) {
    auto tree = convert("T: 'a'+=3 => count 'b'+=limit;");
    const auto& members = tree.getTreeMap().at({"T"}).rule_members;
    ASSERT_EQ(members.size(), 2u);
    EXPECT_EQ(feature<AST::Counter>(*members[0]).bound, "3");
    EXPECT_EQ(feature<AST::Counter>(*members[0]).kind, "number");
    EXPECT_EQ(feature<AST::Counter>(*members[1]).bound, "limit");
    EXPECT_EQ(feature<AST::SharedCounterRegistration>(*members[0]).name, "count");
}

TEST(UAST, DirectCountSyntaxPreservesFollowingMembers) {
    auto tree = convert("A:'a'; main: A=x A=3 A* A+;");
    const auto& members = tree.getTreeMap().at({"main"}).rule_members;
    ASSERT_EQ(members.size(), 4u);
    EXPECT_EQ(feature<AST::Counter>(*members[0]).bound, "x");
    EXPECT_EQ(feature<AST::Counter>(*members[1]).bound, "3");
    EXPECT_EQ(members[0]->quantifier, '*');
    EXPECT_TRUE(members[2]->features.empty());
    EXPECT_EQ(members[2]->quantifier, '*');
    EXPECT_EQ(members[3]->quantifier, '+');
}

TEST(UAST, PythonActionAndFailBlock) {
    Parser::Parser parser;
    auto syntax = parser.parse("fail recover>(value) {py} T: 'a';");
    AST::Builder builder(syntax);
    builder.build();
    auto tree = builder.get();
    ASSERT_EQ(tree.getFailBlocks().size(), 1u);
    const auto& fail = tree.getFailBlocks().front();
    EXPECT_EQ(fail.name.front(), "recover");
    EXPECT_EQ(fail.parameters.front(), "value");
    EXPECT_EQ(fail.action.python.kind, "Module");
    EXPECT_TRUE(fail.action.bindings.empty());
}

TEST(UAST, PythonSubtreeRetainsStatementsAndExpressions) {
    UAST::SourcePosition pos{"action.isc", 25, 0, 4, 2};
    const auto statements = UAST::parsePythonSnapshot("x = 3\nif x:\n    return x\n", pos, false);
    EXPECT_EQ(statements.kind, "main");
    ASSERT_FALSE(statements.children.empty());
    const auto expression = UAST::parsePythonSnapshot("x + 2", pos, true);
    EXPECT_EQ(expression.kind, "expr");
    EXPECT_EQ(expression.position.file, "action.isc");
}

TEST(UAST, EmbeddedExternalPythonActionsResumeGrammar) {
    IspaParser parser;
    const auto& syntax = parser.parse(R"(
fail recover {
x = {'brace': '}'}
if x:
    x = 2
}
T: ${(a + 2) * b} 'a'+=3;
U: 'b';
)");
    ASSERT_TRUE(parser.getErrors().empty());
    AST::Builder builder(syntax, "external-python.isc");
    builder.build();
    const auto tree = builder.get();
    ASSERT_EQ(tree.getFailBlocks().size(), 1u);
    EXPECT_EQ(tree.getFailBlocks().front().action.python.kind, "Module");
    EXPECT_TRUE(tree.getTreeMap().contains({"T"}));
    EXPECT_TRUE(tree.getTreeMap().contains({"U"}));
}

TEST(UAST, PythonExpressionInDynamicPatternAndCounter) {
    auto tree = convert("T: ${py} 'a'+=(py + 1);");
    const auto& members = tree.getTreeMap().at({"T"}).rule_members;
    ASSERT_EQ(members.size(), 2u);
    EXPECT_EQ(feature<AST::DynamicPattern>(*members[0]).expression.python.kind, "Name");
    EXPECT_EQ(feature<AST::Counter>(*members[1]).expression.python.kind, "BinaryExpr");
}

TEST(UAST, NegatedLexerPatternConsumesOneAllowedCharacter) {
    IspaParser parser;
    const auto& syntax = parser.parse(R"(QUOTE: '"'; STRING<quote>: !(${quote} | [\n\r])+; main: STRING<QUOTE>;)" );
    ASSERT_TRUE(parser.getErrors().empty()) << parser.getErrors().front().message;
    AST::Builder builder(syntax, "feature.isc");
    builder.build();
    auto tree = builder.get();
    const AST::Rule* instance = nullptr;
    for (const auto& [_, rule] : tree.getTreeMap())
        if (rule.templateOrigin && !rule.templateOrigin->name.empty() && rule.templateOrigin->name.back() == "STRING")
            instance = &rule;
    ASSERT_NE(instance, nullptr);
    ASSERT_EQ(instance->rule_members.size(), 1u);
    const auto& member = *instance->rule_members.front();
    ASSERT_TRUE(member.isCsequence());
    EXPECT_TRUE(member.getCsequence().negative);
    const auto& excluded = member.getCsequence().characters;
    EXPECT_NE(std::ranges::find(excluded, '\"'), excluded.end());
    EXPECT_NE(std::ranges::find(excluded, '\n'), excluded.end());
    EXPECT_NE(std::ranges::find(excluded, '\r'), excluded.end());
}

TEST(UAST, NegatedParserPatternSelectsOtherTokens) {
    auto tree = convert("A: 'a'; B: 'b'; main: @ !A {@};");
    AST::TreePass pass(tree);
    pass.work();
    const auto& rule = tree.getTreeMap().at({"main"});
    const auto& member = *rule.rule_members.front();
    ASSERT_TRUE(member.isOp());
    bool found_b = false;
    for (const auto& option : member.getOp().options) {
        ASSERT_TRUE(option->isName());
        EXPECT_NE(option->getName().name, (stdu::vector<std::string>{"A"}));
        found_b |= option->getName().name == stdu::vector<std::string>{"B"};
    }
    EXPECT_TRUE(found_b);
    ASSERT_TRUE(rule.data_block.type.has_value());
    EXPECT_EQ(rule.data_block.type->name, (stdu::vector<std::string>{"B"}));
}

TEST(UAST, ConvertsBootstrapGrammar) {
    const auto root = std::string(ROOT_DIR);
    const auto project = Modules::preprocess({root + "/parser/ispa/grammar"},
        {root + "/parser/ispa/entry.isc"},
        {{"python", root + "/parser/python/bootstrapped/PyRawParser"}});
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
    ASSERT_NO_THROW(builder.build());
    auto tree = builder.get();
    EXPECT_GT(tree.getTreeMap().size(), 30u);
    EXPECT_TRUE(tree.contains({"modules", "rule", "regular", "value", "mul_capture"}));
    EXPECT_TRUE(tree.contains({"modules", "rule", "advanced", "advanced_rule"}));
    EXPECT_TRUE(tree.contains({"modules", "rule", "regular", "ast", "abbr"}));
    EXPECT_TRUE(tree.contains({"modules", "rule", "regular", "ast", "single_value"}));
}

TEST(UAST, AdvancedRuleKeepsPythonActions) {
    auto tree = convert("T { py }");
    const auto& features = tree.getTreeMap().at({"T"}).features;
    ASSERT_EQ(features.size(), 2u);
    EXPECT_TRUE(std::get<AST::AdvancedRule>(features[0]).syntax.kind.ends_with("advanced_rule"));
    EXPECT_EQ(std::get<AST::PythonAction>(features[1]).python.kind, "Module");
}

TEST(UAST, DirectPythonBodiesPreserveMatchBoundaries) {
    IspaParser parser;
    const auto& syntax = parser.parse(R"(
main {
    ctx.ast.label = "match }"
    # An unmatched brace in a Python comment: }
    if True:
        ctx.ast.count = 1
    match A;
    match A;
    ctx.ast.done = True
}
fail repair { ctx.ast.expected = ctx.failed_rule }
fail empty {}
A: 'a';
)");
    ASSERT_TRUE(parser.getErrors().empty());
    AST::Builder builder(syntax, "direct.isc");
    builder.build();
    const auto tree = builder.get();
    EXPECT_EQ(tree.getTreeMap().at({"main"}).rule_members.size(), 4u);
    ASSERT_EQ(tree.getFailBlocks().size(), 2u);
    EXPECT_TRUE(std::holds_alternative<UAST::py::Module>(tree.getFailBlocks()[1].action.code));
    for (const auto* old : {"main { p{ctx.ast.value = 1} }", "fail repair p{ctx.ast.value = 1} main: 'a';"}) {
        IspaParser invalid;
        bool rejected = false;
        try { invalid.parse(old); rejected = !invalid.getErrors().empty() || !invalid.getContextErrors().empty(); }
        catch (const std::exception&) { rejected = true; }
        EXPECT_TRUE(rejected) << old;
    }
}

TEST(UAST, PythonAstBlocksKeepTargets) {
    auto tree = convert("T: 'a' &= { result: py }; S: 'b' {py};");
    const auto& keyed = tree.getTreeMap().at({"T"}).features;
    ASSERT_EQ(keyed.size(), 1u);
    const auto& action = std::get<AST::PythonAction>(keyed.front());
    EXPECT_EQ(action.target, "result");
    EXPECT_EQ(action.python.kind, "Name");
    const auto& single = tree.getTreeMap().at({"S"}).features;
    ASSERT_EQ(single.size(), 1u);
    EXPECT_TRUE(std::get<AST::PythonAction>(single.front()).target.empty());
}

TEST(UAST, ExpressionFailBlocksPreserveFollowingDeclarationsAndRejectInvalidAst) {
    for (const auto* source : {
        "item: ${f({'k': 1}, +)} 'suffix'; next: 'n';",
        "item: 'x' &= { bad: f({'k': 1}, +) later: 2 }; next: 'n';",
        "item: ${f( + ; next: 'n';"
    }) {
        SCOPED_TRACE(source);
        IspaParser parser;
        const auto& syntax = parser.parse(source);
        ASSERT_FALSE(parser.getErrors().empty());
        EXPECT_NE(parser.getErrors().front().message.find("invalid Python expression"), std::string::npos);
        const auto& declarations = syntax.value.data().value;
        ASSERT_EQ(declarations.size(), 2u);
        const auto& next = std::get<UAST::Node<UAST::grammar>>(declarations.back());
        EXPECT_EQ(next.data().name.data().primary.data().value, "next");
        AST::Builder builder(syntax, "invalid.isc");
        EXPECT_THROW(builder.build(), std::runtime_error);
    }
}

TEST(UAST, DirectExpressionsPreserveFieldsAndRejectWrappedExpressions) {
    IspaParser parser;
    parser.parse("item: 'x' &= { first: 1 second: (2 + 3) }; next: ${count + 1};");
    EXPECT_TRUE(parser.getErrors().empty());
    for (const auto* source : {"item: ${p{1}};", "item: 'x' {p{1}};"}) {
        IspaParser invalid;
        bool rejected = false;
        try {
            invalid.parse(source);
            rejected = !invalid.getErrors().empty() || !invalid.getContextErrors().empty();
        } catch (const std::exception&) { rejected = true; }
        EXPECT_TRUE(rejected) << source;
    }
}

TEST(UAST, UnterminatedExpressionStringsReachBoundHandlerAndAbort) {
    IspaParser parser;
    EXPECT_THROW(parser.parse("item: ${'unterminated\n}; next: 'n';"), std::exception);
    ASSERT_FALSE(parser.getErrors().empty());
    EXPECT_NE(parser.getErrors().front().message.find("invalid Python expression in dynamic pattern"), std::string::npos);
    EXPECT_EQ(parser.getErrors().front().decision.mode, ISPA_STD::Recovery::Mode::Abort);
}

TEST(UAST, DataBlockMetadataUsesResolvedNamesAndFieldOrder) {
    auto tree = convert("main: @ WORD @ 'x' @{word, suffix}; WORD: 'w';");
    const auto& fields = tree.getTreeMap().at({"main"}).data_block.fields;
    ASSERT_EQ(fields.size(), 2u);
    EXPECT_EQ(fields[0].first, "word");
    EXPECT_EQ(fields[0].second.kind, AST::DataType::Kind::Token);
    ASSERT_EQ(fields[0].second.name.size(), 1u);
    EXPECT_EQ(fields[0].second.name.front(), "WORD");
    EXPECT_EQ(fields[1].first, "suffix");
    EXPECT_EQ(fields[1].second.kind, AST::DataType::Kind::Character);
}
