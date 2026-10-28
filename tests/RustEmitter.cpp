#include <gtest/gtest.h>
import std;
import dstd;
import LangAPI;
import LangRepr.Holder;
import LangRepr.Converter;

namespace {
using namespace LangAPI;
auto symbol(const char* name) -> Expression { return Symbol::createExpression(Symbol{name}); }
auto integer(long long value) -> Expression { return Int::createExpression(Int{.value = value}); }
auto returned(Expression value) -> Statements { return Return::createStatements(Return{.value = std::move(value)}); }
auto call(const char* name, stdu::vector<Expression> args) -> Expression {
    return FunctionCall::createExpression(FunctionCall{
        .name = std::make_shared<Symbol>(Symbol{name}), .args = std::move(args)});
}
auto generate(const Declarations& declarations, bool incremental = false)
    -> std::vector<std::pair<std::string, std::string>> {
    LangRepr::Holder holder{declarations};
    LangRepr::Converter converter(holder, "rust", "Emitter", incremental, false);
    converter.init();
    converter.construct();
    return converter.get();
}
}

TEST(RustEmitter, NeutralValuesCallsBorrowingAndControlFlowExecute) {
    Declarations declarations;
    Function identity{.type = ValueType::String, .name = "identity",
        .parameters = {{ValueType::String, "value"}}, .statements = returned(symbol("value"))};
    declarations.push_back(identity);
    Function bump{.type = ValueType::Void, .name = "bump",
        .parameters = {{Type{ValueType::BorrowSemantic, Type{ValueType::Int}}, "number"}},
        .statements = VariableAssignment::createStatements(VariableAssignment{
            .name = Symbol{"number"}, .type = OperatorType::Add, .value = integer(3)})};
    declarations.push_back(bump);
    Function run{.type = ValueType::Int, .name = "run"};
    run.statements.push_back(Variable{.name = "number", .type = ValueType::Int, .value = integer(2)});
    run.statements.push_back(Expression::createStatement(call("bump", {symbol("number")})));
    run.statements.push_back(Return::createStatement(Return{.value = symbol("number")}));
    declarations.push_back(run);

    std::string raw = "quote\" backslash\\n newline\n tab\t";
    raw += '\0';
    raw += " café";
    Function literal{.type = ValueType::String, .name = "literal",
        .statements = returned(String::createExpression(String{.value = raw}))};
    declarations.push_back(literal);
    Function copies{.type = ValueType::String, .name = "copies"};
    copies.statements.push_back(Variable{.name = "text", .type = ValueType::String,
        .value = String::createExpression(String{.value = "value"})});
    copies.statements.push_back(Variable{.name = "first", .type = ValueType::String,
        .value = call("identity", {symbol("text")})});
    copies.statements.push_back(Return::createStatement(Return{.value = call("identity", {symbol("text")})}));
    declarations.push_back(copies);

    Function loop{.type = ValueType::Int, .name = "loop"};
    loop.statements.push_back(Variable{.name = "n", .type = ValueType::Int, .value = integer(0)});
    Expression condition = symbol("n");
    condition.emplace_back(ExpressionElement::Lower);
    condition.emplace_back(Int{.value = 3});
    loop.statements.push_back(DoWhile::createStatement(DoWhile{condition, {
        VariableAssignment::createStatement(VariableAssignment{
            .name = Symbol{"n"}, .type = OperatorType::Add, .value = integer(1)}),
        Expression::createStatement(Expression{ExpressionValue{Continue{}}})}}));
    loop.statements.push_back(Return::createStatement(Return{.value = symbol("n")}));
    declarations.push_back(loop);

    Function steps{.type = ValueType::Int, .name = "steps"};
    steps.statements.push_back(Variable{.name = "n", .type = ValueType::Int, .value = integer(1)});
    Expression stepped = symbol("n");
    stepped.emplace_back(ExpressionElement::PlusPlus);
    stepped.emplace_back(ExpressionElement::Add);
    stepped.emplace_back(ExpressionElement::PlusPlus);
    stepped.emplace_back(Symbol{"n"});
    steps.statements.push_back(Return::createStatement(Return{.value = stepped}));
    declarations.push_back(steps);

    Class counter{.name = "Counter"};
    counter.data.emplace_back(std::make_shared<Declaration>(Variable::createDeclaration(
        Variable{.name = "value", .type = ValueType::Int, .value = integer(4)})), Visibility::Public);
    Function read{.type = ValueType::Int, .name = "read", .statements = returned(symbol("value")), .is_const = true};
    counter.data.emplace_back(std::make_shared<Declaration>(Function::createDeclaration(read)), Visibility::Public);
    Function shadow{.type = ValueType::Int, .name = "shadow", .parameters = {{ValueType::Int, "value"}},
        .statements = returned(symbol("value")), .is_const = true};
    counter.data.emplace_back(std::make_shared<Declaration>(Function::createDeclaration(shadow)), Visibility::Public);
    declarations.push_back(counter);

    // Array-looking text is literal data, including when it appears inside a fixed array.
    Type array_type{ValueType::FixedSizeArray, Type{ValueType::String}};
    array_type.template_parameters.emplace_back(RValue{Int{.value = 1}});
    Array values;
    values.values.push_back(String::createExpression(String{.value = "vec![literal]"}));
    declarations.push_back(Function{.type = array_type, .name = "array",
        .statements = returned(Array::createExpression(values))});
    Declarations program{Namespace::createDeclaration(Namespace{.name = "Emitter", .declarations = declarations})};
    std::ostringstream before;
    before << program;
    const auto directory = std::filesystem::temp_directory_path() /
        ("ispa-rust-emitter-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory);
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::filesystem::remove_all(path); } } cleanup{directory};
    for (bool incremental : {false, true}) {
        const auto files = generate(program, incremental);
        for (const auto& [name, source] : files) {
            EXPECT_EQ(source.find("__ispa_rust_"), std::string::npos);
            std::filesystem::create_directories((directory / name).parent_path());
            std::ofstream(directory / name) << source;
        }
        std::ofstream(directory / "main.rs") << R"rs(
#[path = "Emitter.rs"] mod generated;
pub use generated::Emitter;
fn main() {
    assert_eq!(Emitter::run(), 5);
    assert_eq!(Emitter::copies(), "value");
    assert_eq!(Emitter::literal(), "quote\" backslash\\n newline\n tab\t\0 café");
    assert_eq!(Emitter::r#loop(), 3);
    assert_eq!(Emitter::steps(), 4);
    let counter = Emitter::Counter::default();
    assert_eq!(counter.read(), 4);
    assert_eq!(counter.shadow(9), 9);
    assert_eq!(Emitter::array(), [String::from("vec![literal]")]);
}
)rs";
        const auto command = "rustc --edition=2021 \"" + (directory / "main.rs").string() +
            "\" -o \"" + (directory / "run").string() + "\" && \"" + (directory / "run").string() + "\"";
        EXPECT_EQ(std::system(command.c_str()), 0) << "incremental=" << incremental;
    }
    std::ostringstream after;
    after << program;
    EXPECT_EQ(before.str(), after.str());
}

TEST(RustEmitter, RejectsLanguageSpecificTypesAndUnresolvedOverloads) {
    for (const char* name : {"std::ostream&", "auto&", "Vec<i64>", "__ispa_rust_ref"}) {
        Declarations program{Function::createDeclaration(Function{.type = Type{Symbol{name}}, .name = "invalid"})};
        EXPECT_THROW(generate(program), std::exception) << name;
    }
    Function function{.type = ValueType::Int, .name = "overload", .statements = returned(integer(1))};
    EXPECT_THROW(generate({Function::createDeclaration(function), Function::createDeclaration(function)}), std::exception);
}

TEST(RustEmitter, ReportsMissingParserRuntimeContract) {
    Class lexer{.name = "Lexer",
        .inherit_members = {{Visibility::Public, IspaLibSymbol{.exports = StdlibExports::Lexer}}}};
    try {
        generate({Class::createDeclaration(lexer)});
        FAIL() << "incomplete parser generation must be diagnosed";
    } catch (const std::exception& error) {
        EXPECT_NE(std::string(error.what()).find("semantic token-stream operations"), std::string::npos);
    }
}
