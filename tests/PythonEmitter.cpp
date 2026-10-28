#include <gtest/gtest.h>
import std;
import LangAPI;
import LangRepr.Holder;
import LangRepr.Converter;

TEST(PythonEmitter, ArithmeticControlFlowAndDebugExecute) {
    using namespace LangAPI;
    const auto symbol = [](const char* name) { return Symbol::createExpression(Symbol{name}); };
    const auto integer = [](long long value) { return Int::createExpression(Int{.value = value}); };
    const auto binary = [](Expression left, ExpressionElement op, const Expression& right) {
        left.emplace_back(op);
        left.insert(left.end(), right.begin(), right.end());
        return left;
    };
    Declarations declarations;
    for (const auto& [name, operation] : {
        std::pair{"divide", ExpressionElement::Divide}, {"modulo", ExpressionElement::Modulo}}) {
        Function function{.type = ValueType::Undef, .name = name,
            .parameters = {{ValueType::Undef, "a"}, {ValueType::Undef, "b"}},
            .statements = Return::createStatements(Return{.value = binary(symbol("a"), operation, symbol("b"))})};
        declarations.push_back(Function::createDeclaration(function));
    }
    Function reconstruct{.type = ValueType::Undef, .name = "reconstruct",
        .parameters = {{ValueType::Undef, "a"}, {ValueType::Undef, "b"}}};
    auto quotient = binary(symbol("a"), ExpressionElement::Divide, symbol("b"));
    auto product = binary(quotient, ExpressionElement::Multiply, symbol("b"));
    reconstruct.statements = Return::createStatements(Return{.value = binary(product,
        ExpressionElement::Add, binary(symbol("a"), ExpressionElement::Modulo, symbol("b")))});
    declarations.push_back(Function::createDeclaration(reconstruct));
    Function compound{.type = ValueType::Undef, .name = "compound",
        .parameters = {{ValueType::Undef, "a"}, {ValueType::Undef, "b"}},
        .statements = {
            VariableAssignment::createStatement(VariableAssignment{.name = Symbol{"a"},
                .type = OperatorType::Divide, .value = symbol("b")}),
            Return::createStatement(Return{.value = symbol("a")})}};
    declarations.push_back(Function::createDeclaration(compound));
    Function loop{.type = ValueType::Int, .name = "loop"};
    loop.statements.push_back(Variable::createStatement(Variable{.name = "n", .type = ValueType::Int, .value = integer(0)}));
    VariableAssignment increment{.name = Symbol{"n"}, .type = OperatorType::Add, .value = integer(1)};
    DoWhile body{binary(symbol("n"), ExpressionElement::Lower, integer(3)),
        Statements{VariableAssignment::createStatement(increment),
            Expression::createStatement(Expression{ExpressionValue{Continue{}}})}};
    loop.statements.push_back(DoWhile::createStatement(body));
    loop.statements.push_back(Return::createStatement(Return{.value = symbol("n")}));
    declarations.push_back(Function::createDeclaration(loop));

    Switch choice;
    choice.expression = symbol("value");
    choice.cases.push_back({Int{.value = 0}, {}});
    choice.cases.push_back({Int{.value = 1}, Return::createStatements(Return{.value = integer(7)})});
    Function select{.type = ValueType::Int, .name = "select", .parameters = {{ValueType::Int, "value"}},
        .statements = {Switch::createStatement(choice), Return::createStatement(Return{.value = integer(9)})}};
    declarations.push_back(Function::createDeclaration(select));
    Array debug;
    debug.values.push_back(DFADebug::createExpression(DFADebug{
        .token_name = {"tokens", "WORD"}, .position_in_token = 2,
        .call = 3, .group = 4, .rule_run = 5, .offset = 6, .length = 7, .ch = '\n'}));
    declarations.push_back(Variable::createDeclaration(Variable{
        .name = "debug", .type = ValueType::Array, .value = Array::createExpression(debug)}));

    LangRepr::Holder holder{declarations};
    LangRepr::Converter converter(holder, "python", "Emitter", false, false);
    converter.init();
    converter.construct();
    const auto directory = std::filesystem::temp_directory_path() /
        ("ispa-python-emitter-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory);
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::filesystem::remove_all(path); } } cleanup{directory};
    for (const auto& [name, source] : converter.get()) std::ofstream(directory / name) << source;
    std::ofstream(directory / "run.py") << R"py(
import Emitter as e
assert e.divide(5.0, 2.0) == 2.5
assert e.divide(-5, 2) == -2
assert e.divide(5, -2) == -2
assert e.divide(2**80 + 1, 3) == (2**80 + 1) // 3
assert e.modulo(-5, 2) == -1
assert e.reconstruct(-5, 2) == -5
assert e.compound(-5, 2) == -2
assert e.compound(5.0, 2.0) == 2.5
assert e.loop() == 3
assert e.select(0) == 9 and e.select(1) == 7
d = e.debug[0]
assert (d.token_name, d.position_in_token, d.call, d.group, d.rule_run, d.offset, d.length, d.ch) == ('tokens.WORD', 2, 3, 4, 5, 6, 7, '\n')
)py";
    const std::string command = "PYTHONDONTWRITEBYTECODE=1 PYTHONPATH=\"" + std::string(ROOT_DIR) +
        "/parser/stdlibs/target\" timeout 15s python3 \"" + (directory / "run.py").string() + "\"";
    EXPECT_EQ(std::system(command.c_str()), 0);
}
