#include <gtest/gtest.h>
#include <PyRawParser.h>
import Parser.Base;
import ParserIR.API;
import AST.API;
import UAST.Python;
import LangAPI;
import LangRepr.Holder;
import LangRepr.Converter;
import std;

namespace {
const UAST::SourcePosition position{"conversion.isc", 0, 0, 7, 3};
LangAPI::Statements executableStatements(LangAPI::Statements input) {
    LangAPI::Statements result;
    for (auto& statement : input) {
        if (statement.isComment() && statement.getComment().value.starts_with("ISPA_DEBUG_")) continue;
        if (statement.isIf()) {
            auto& branch = statement.getIf();
            branch.stmt = executableStatements(branch.stmt);
            branch.else_stmt = executableStatements(branch.else_stmt);
        } else if (statement.isWhile() || statement.isDoWhile()) {
            auto& loop = statement.getWhileOrDoWhile();
            loop.stmt = executableStatements(loop.stmt);
        }
        result.push_back(std::move(statement));
    }
    return result;
}
auto expression(const std::string& source) {
    return Parser::ParserBase::convertPython(*UAST::parsePythonExpr(source, position));
}
auto statements(const std::string& source) {
    return executableStatements(Parser::ParserBase::convertPython(UAST::parsePythonModule(source, position)));
}
}
TEST(PythonConversion, ExternalExpressionUsesOriginalParserTypeAndSource) {
    using Binding = PyRawParser::Exports::ExpressionBlock;
    static_assert(std::is_same_v<Binding::node_type, PyRawParser::Types::expr>);
    const std::string source = "host\n(a + 2) * b} trailing";
    auto result = Binding::parse(source.c_str(), source.c_str() + 5);
    ASSERT_TRUE(result.status);
    EXPECT_EQ(result.node.start(), source.c_str() + 5);
    EXPECT_EQ(result.node.line(), 2u);
    EXPECT_EQ(std::string_view(result.end), "} trailing");
    const auto syntax = UAST::convertPythonExpr(result.node, {"extern.isc", 5, result.node.length(), 2, 1});
    ASSERT_TRUE(syntax);
    EXPECT_FALSE(Parser::ParserBase::convertPython(*syntax).empty());
}

TEST(PythonConversion, ExternalStatementsPreserveIndentationAndHostBoundary) {
    using Binding = PyRawParser::Exports::StatementsBlock;
    static_assert(std::is_same_v<Binding::node_type, PyRawParser::Types::main>);
    const std::string source = "p{x = 1\nif x:\n    x = x + 1\n} NEXT";
    const auto result = Binding::parse(source.c_str(), source.c_str());
    ASSERT_TRUE(result.status);
    EXPECT_EQ(std::string_view(result.end), " NEXT");
    const auto syntax = UAST::convertPythonModule(result.node, {"extern.isc", 2, result.node.length(), 1, 3});
    EXPECT_EQ(syntax.body.size(), 2u);
    const char invalid[] = "p{x = }";
    EXPECT_THROW(Binding::parse(invalid, invalid), std::runtime_error);
}

TEST(PythonConversion, RawStatementsRespectPythonBoundariesAndBaseIndent) {
    using Binding = PyRawParser::Exports::RawStatements;
    const std::string source = R"(
    value = {'brace': '}', 'word': 'match A;'}
    # } match B;
    if value:
        value = 2
    match A -> captured;
})";
    auto parsed = Binding::parse(source.c_str(), source.c_str());
    ASSERT_TRUE(parsed.status);
    EXPECT_TRUE(std::string_view(parsed.end).starts_with("match A"));
    auto syntax = UAST::convertPythonModule(parsed.node, {"raw.isc", parsed.node.startpos(), parsed.node.length(), parsed.node.line(), parsed.node.column()});
    EXPECT_EQ(syntax.body.size(), 2u);
    EXPECT_FALSE(Binding::starts(source.c_str(), parsed.end));
    EXPECT_FALSE(Binding::starts(source.c_str(), " }"));
}

TEST(PythonConversion, ArithmeticPreservesGrouping) {
    const auto value = expression("(a + b) * c");
    ASSERT_FALSE(value.empty());
    EXPECT_EQ(value.front().getExpressionElement(), LangAPI::ExpressionElement::GroupOpen);
    int adds = 0, multiplies = 0;
    for (const auto& item : value) if (item.isExpressionElement()) {
        adds += item.getExpressionElement() == LangAPI::ExpressionElement::Add;
        multiplies += item.getExpressionElement() == LangAPI::ExpressionElement::Multiply;
    }
    EXPECT_EQ(adds, 1);
    EXPECT_EQ(multiplies, 1);
}

TEST(PythonConversion, StringLiteralsBecomeDecodedNeutralValues) {
    const auto value = expression(R"py('quote" slash\\ newline\n hex\x41 unicode\u00e9')py");
    ASSERT_EQ(value.size(), 1u);
    EXPECT_EQ(value.front().getRValue().getString().value, "quote\" slash\\ newline\n hexA unicodeé");
}

TEST(PythonConversion, RecoveredSyntaxCannotBecomeExecutableActions) {
    for (const auto* source : {"1 +", "f(1", "[1", "{'k': }"})
        EXPECT_THROW(UAST::parsePythonExpr(source, position), std::runtime_error) << source;
    for (const auto* source : {"x =\ny = 2\n", "if True\n    pass\n"})
        EXPECT_THROW(UAST::parsePythonModule(source, position), std::runtime_error) << source;
    for (const auto* source : {"1 + }", "f(1, +)}"})
        EXPECT_THROW(PyRawParser::Exports::ExpressionBlock::parse(source, source), std::runtime_error) << source;
    const char* source = "x =\ny = 2\n}";
    EXPECT_THROW(PyRawParser::Exports::RawStatements::parse(source, source), std::runtime_error);
}
TEST(PythonConversion, CallsAndStorage) {
    auto call = expression("make(1, True)").front().getFunctionCall();
    ASSERT_EQ(call.args.size(), 2u);
    EXPECT_EQ(call.args[0].front().getRValue().getInt().value, 1);
    EXPECT_TRUE(call.args[1].front().getRValue().getBool().value);
    auto attribute = expression("node.value").front().getRValue().getStorageSymbol();
    EXPECT_EQ(std::get<std::string>(attribute.path.front()), "value");
    auto index = expression("values[2]").front().getRValue().getStorageSymbol();
    EXPECT_EQ(std::get<LangAPI::StorageOffset>(index.path.front()).offset.front().getRValue().getInt().value, 2);
}
TEST(PythonConversion, AssignmentAndControlFlow) {
    const auto output = statements("x: int = 1\nx = x + 1\nif x > 1:\n    x += 2\nelse:\n    x -= 1\nwhile x < 10:\n    x += 1\nreturn x\n");
    ASSERT_EQ(output.size(), 5u);
    EXPECT_TRUE(output[0].isVariable());
    EXPECT_TRUE(output[1].getExpression().front().isVariableAssignment());
    EXPECT_EQ(output[2].getIf().else_stmt.size(), 1u);
    EXPECT_EQ(output[3].getWhile().stmt.size(), 1u);
    EXPECT_TRUE(output[4].getExpression().front().isReturn());
}
TEST(PythonConversion, DebugCheckpointsSurroundExecutableStatements) {
    const auto module = UAST::parsePythonModule("capture = 5\n", position);
    const auto output = Parser::ParserBase::convertPython(module, {"capture"});
    ASSERT_EQ(output.size(), 3u);
    ASSERT_TRUE(output.front().isComment());
    EXPECT_TRUE(output.front().getComment().value.starts_with("ISPA_DEBUG_LINE "));
    EXPECT_TRUE(output[1].getExpression().front().isVariableAssignment());
    ASSERT_TRUE(output.back().isComment());
    EXPECT_NE(output.back().getComment().value.find("@after"), std::string::npos);
}
TEST(PythonConversion, DeclarationsKeepTypesAndParameters) {
    auto module = UAST::parsePythonModule("def add(x: int, y: int) -> int:\n    return x + y\nclass Box:\n    value: int = 0\n", position);
    auto output = Parser::ParserBase::convertPythonDeclarations(module);
    ASSERT_EQ(output.size(), 2u);
    const auto& f = output[0].getFunction();
    EXPECT_EQ(f.name, "add");
    EXPECT_TRUE(f.type == LangAPI::ValueType::Int);
    EXPECT_EQ(f.parameters.size(), 2u);
    EXPECT_EQ(output[1].getClass().data.size(), 1u);
}
TEST(PythonConversion, RejectsUnsupportedSyntaxWithPosition) {
    try {
        expression("a ** b");
        FAIL() << "expected unsupported operator";
    } catch (const std::invalid_argument& error) {
        EXPECT_NE(std::string(error.what()).find("conversion.isc:7:3:"), std::string::npos);
    }
    EXPECT_THROW(expression("a < f() < c"), std::invalid_argument);
    EXPECT_THROW(statements("while True:\n    pass\nelse:\n    pass\n"), std::invalid_argument);
}
TEST(PythonConversion, DataBlockPreservesInitializer) {
    AST::DataBlock block;
    block.value = AST::RegularDataBlock{*UAST::parsePythonExpr("42", position)};
    auto output = ParserIR::dataBlock(block);
    ASSERT_TRUE(output.is_regular_data_block());
    EXPECT_EQ(output.getRegularDataBlock().first.front().getRValue().getInt().value, 42);
}

TEST(PythonConversion, ContainersAndExistingBindings) {
    EXPECT_EQ(expression("[1, 2]").front().getRValue().getArray().values.size(), 2u);
    EXPECT_EQ(expression("(1, 2)").front().getRValue().getMakeTuple().args.size(), 2u);
    EXPECT_EQ(expression("{'key': 1}").front().getRValue().getMap().keys.size(), 1u);
    auto module = UAST::parsePythonModule("capture = 5\n", position);
    auto output = executableStatements(Parser::ParserBase::convertPython(module, {"capture"}));
    ASSERT_EQ(output.size(), 1u);
    EXPECT_TRUE(output.front().getExpression().front().isVariableAssignment());
    EXPECT_THROW(statements("if True:\n    x = 1\n"), std::invalid_argument);
    EXPECT_THROW(statements("break\n"), std::invalid_argument);
}
TEST(PythonConversion, BoundedSourcePositions) {
    const auto value = UAST::parsePythonExpr("a + b", position);
    const auto& right = value->get<UAST::py::BinaryExpr>()->right;
    EXPECT_EQ(right->pos.offset, 4u);
    EXPECT_EQ(right->pos.line, 7u);
    EXPECT_EQ(right->pos.column, 7u);
}

TEST(PythonConversion, DeclarationsReachExistingCodeEmitter) {
    const auto module = UAST::parsePythonModule("def add(x: int, y: int) -> int:\n    return x + y\n", position);
    LangRepr::Holder holder{Parser::ParserBase::convertPythonDeclarations(module)};
    LangRepr::Converter converter(holder, "cpp", "PythonConversionTest", false);
    converter.init();
    converter.construct();
    std::string output;
    for (const auto& [name, source] : converter.get()) output += source;
    EXPECT_NE(output.find("add("), std::string::npos);
    EXPECT_NE(output.find("return "), std::string::npos);
    EXPECT_NE(output.find(" + "), std::string::npos);
}

TEST(PythonConversion, ActionContextValidatesMetadataAndPreservesLocals) {
    const auto action = [](const std::string& source) {
        AST::PythonAction result;
        result.code = UAST::parsePythonModule(source, position);
        return result;
    };
    std::set<std::string> locals;
    EXPECT_NO_THROW(Parser::ParserBase::convertPythonAction(action("saved = ctx.pos\nctx.ast.count: int = 3\n"), locals));
    EXPECT_TRUE(locals.contains("saved"));
    const auto restore = executableStatements(Parser::ParserBase::convertPythonAction(action("ctx.pos = saved\n"), locals));
    EXPECT_TRUE(restore.front().getExpression().front().isVariableAssignment());
    for (const auto* source : {"ctx = 1\n", "ctx: int = 1\n", "ctx.rule = 'x'\n", "ctx.start_pos += 1\n", "x = ctx.unknown\n", "x = ctx.failed_rule\n"})
        EXPECT_THROW(Parser::ParserBase::convertPythonAction(action(source), locals), std::invalid_argument) << source;
    EXPECT_NO_THROW(Parser::ParserBase::convertPythonAction(action("name = ctx.failed_rule\nreturn\n"), locals, true));
    EXPECT_NO_THROW(Parser::ParserBase::convertPythonAction(action("ctx.sync('}', ';')\nctx.insert()\n"), locals, true));
    EXPECT_NO_THROW(Parser::ParserBase::convertPythonAction(action("x = ctx.at(ID) or ctx.at(common.ID) or ctx.at('=')\n"), locals, true));
    EXPECT_NO_THROW(Parser::ParserBase::convertPythonAction(action("ctx.sync_tokens('NEWLINE', 'DEDENT')\nctx.insert()\n"), locals, true));
    EXPECT_NO_THROW(Parser::ParserBase::convertPythonAction(action("ctx.sync_tokens(NEWLINE, DEDENT)\n"), locals, true));
    for (const auto* source : {"ctx.sync()\n", "ctx.sync('')\n", "ctx.sync(42)\n",
                               "ctx.sync_tokens()\n", "ctx.sync_tokens('')\n", "ctx.sync_tokens(42)\n"})
        EXPECT_THROW(Parser::ParserBase::convertPythonAction(action(source), locals, true), std::invalid_argument);
    std::set<std::string> token_locals;
    EXPECT_NO_THROW(Parser::ParserBase::convertPythonAction(
        action("ctx.ast.count: int = 1\nctx.ast.owner: str = ctx.rule\nctx.shared.stack = [0]\nctx.emit('INDENT')\nctx.skip()\n"), token_locals,
        false, {}, nullptr, "TOKEN"));
    for (const auto* source : {"ctx.pos = 1\n", "ctx.start_pos = 1\n", "ctx.rule = 'other'\n",
                               "return\n", "ctx.emit('bad-name')\n"})
        EXPECT_THROW(Parser::ParserBase::convertPythonAction(
            action(source), token_locals, false, {}, nullptr, "TOKEN"), std::invalid_argument);
}

TEST(PythonConversion, ActionContextCanReadAndConsumeLookaheadTokens) {
    const auto action = [](const std::string& source) {
        AST::PythonAction result;
        result.code = UAST::parsePythonModule(source, position);
        return result;
    };
    std::set<std::string> locals;
    EXPECT_NO_THROW(Parser::ParserBase::convertPythonAction(
        action("next_token = ctx.peek(1)\ntext = next_token.text\ncurrent = ctx.take()\n"), locals, true));
    EXPECT_THROW(Parser::ParserBase::convertPythonAction(action("ctx.peek(1, 2)\n"), locals, true), std::invalid_argument);
    EXPECT_THROW(Parser::ParserBase::convertPythonAction(action("ctx.take(1)\n"), locals, true), std::invalid_argument);
    EXPECT_NO_THROW(Parser::ParserBase::convertPythonAction(action("previous = ctx.prev()\n"), locals, true));
    EXPECT_THROW(Parser::ParserBase::convertPythonAction(action("ctx.prev(1)\n"), locals, true), std::invalid_argument);
    EXPECT_NO_THROW(Parser::ParserBase::convertPythonAction(
        action("same = previous.name == ID\nliteral = previous.name == 'id'\n"), locals, true));
    locals.insert("ID");
    EXPECT_NO_THROW(Parser::ParserBase::convertPythonAction(action("same = previous.name == ID\n"), locals, true));
}

TEST(PythonConversion, RecoveryTokenNamesRemainDistinctFromLiteralSpellings) {
    const auto convert = [](const std::string& code) {
        AST::PythonAction action;
        action.code = UAST::parsePythonModule(code, position);
        std::set<std::string> locals;
        return executableStatements(Parser::ParserBase::convertPythonAction(action, locals, true));
    };
    const auto named = convert("ctx.sync_tokens(NEWLINE, DEDENT)\n");
    const auto& named_call = std::get<LangAPI::FunctionCall>(
        named.front().getExpression().front().getRValue().getStorageSymbol().path.back());
    ASSERT_EQ(named_call.args.size(), 2u);
    for (const auto& argument : named_call.args) {
        const auto& boundary = std::get<LangAPI::FunctionCall>(argument.front().getRValue().getStorageSymbol().path.back());
        EXPECT_EQ(std::get<std::string>(std::get<std::shared_ptr<LangAPI::Symbol>>(boundary.name)->path.back()), "token_boundary");
    }
    const auto literal = convert("ctx.sync_tokens(';')\n");
    const auto& literal_call = std::get<LangAPI::FunctionCall>(
        literal.front().getExpression().front().getRValue().getStorageSymbol().path.back());
    EXPECT_EQ(literal_call.args.front().front().getRValue().getString().value, ";");
}

TEST(PythonConversion, CollectionMethodsUsePortableIR) {
    for (const auto* source : {"xs.append(1)", "xs.extend([2, 3])", "xs.insert(-1, 4)", "xs.remove(2)",
        "xs.pop()", "xs.pop(0)", "xs.clear()", "xs.copy()", "xs.count(2)", "xs.index(2, 0, 4)",
        "xs.reverse()", "xs.sort(reverse=True)", "xs.sort(key=f, reverse=True)", "d.get('k', 0)", "d.setdefault('k', 0)",
        "d.update({'k': 1})", "d.update(k=1)", "d.fromkeys(['k'], 0)", "d.keys()", "d.values()", "d.items()", "d.pop('k', 0)"}) {
        const auto value = expression(source);
        ASSERT_EQ(value.size(), 1u) << source;
        const auto& storage = value.front().getRValue().getStorageSymbol();
        EXPECT_TRUE(std::holds_alternative<LangAPI::ArrayMethodCall>(storage.path.back())) << source;
    }
    for (const auto* source : {"xs.append()", "xs.clear(1)", "xs.index()", "xs.sort(unknown=f)", "d.get('k')", "d.popitem()"})
        EXPECT_THROW(expression(source), std::invalid_argument) << source;
}

TEST(PythonConversion, ClassesLowerConstructorsAndInstanceMethods) {
    const auto module = UAST::parsePythonModule(R"py(class Box:
    def __init__(self, value: int):
        self.value = value
    def read(self) -> int:
        return self.value
    @staticmethod
    def twice(value: int) -> int:
        return value * 2
)py", position);
    const auto declarations = Parser::ParserBase::convertPythonDeclarations(module);
    const auto& cls = declarations.front().getClass();
    ASSERT_EQ(cls.data.size(), 3u);
    EXPECT_EQ(cls.data[0].first->getVariable().name, "value");
    ASSERT_TRUE(cls.constructor);
    const auto& constructor = *cls.constructor;
    EXPECT_EQ(constructor.name, "Box");
    EXPECT_EQ(constructor.parameters.size(), 1u);
    EXPECT_TRUE(cls.data[2].first->getFunction().is_static);
    LangRepr::Holder holder{declarations};
    LangRepr::Converter converter(holder, "cpp", "ClassConversionTest", false);
    converter.init(); converter.construct();
    std::string output;
    for (const auto& [name, source] : converter.get()) output += source;
    EXPECT_NE(output.find("Box(long long value)"), std::string::npos);
    EXPECT_NE(output.find("(*this)"), std::string::npos);
    EXPECT_EQ(output.find("auto Box("), std::string::npos);
}
