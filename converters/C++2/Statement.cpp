module Cpp.Statement;

import Cpp.CoreFunctions;
import Converter.Statement;
import corelib;
import std;
Cpp::Statement::Statement(Converter::Writer &output) : Converter::Statement(output) {
    Core::stmts_converter = this;
}

auto Cpp::Statement::createVariable(const LangAPI::Variable &v) -> void {
    Core::output->write("{} {}", Core::convertType(v.type), v.name);
    if (!v.value.empty()) {
        Core::output->dwrite(" = {}", Core::convertExpression(v.value));
    }
    Core::output->dwriteln(";");
}
auto Cpp::Statement::createIf(const LangAPI::Expression &expression) -> void {
    Core::output->writeln("if ({}) {", Core::convertExpression(expression));
    Core::output->increaseIndentation();
}
auto Cpp::Statement::closeIf() -> void {
    Core::output->decreaseIndentation();
    Core::output->writeln("}");
}
auto Cpp::Statement::createWhile(const LangAPI::Expression &expression) -> void {
    Core::output->writeln("while ({}) {", Core::convertExpression(expression));
    Core::output->increaseIndentation();
}
auto Cpp::Statement::closeWhile() -> void {
    Core::output->decreaseIndentation();
    Core::output->writeln("}");
}
auto Cpp::Statement::openDoWhile() -> void {
    Core::output->writeln("do {");
    Core::output->increaseIndentation();
}
auto Cpp::Statement::closeDoWhile(const LangAPI::Expression &expression) -> void {
    Core::output->decreaseIndentation();
    Core::output->writeln("} while ({});", Core::convertExpression(expression));
}
auto Cpp::Statement::createSwitch(const LangAPI::Expression &expression) -> void {
    Core::output->writeln("switch ({}) {", Core::convertExpression(expression));
    Core::output->increaseIndentation();
}
auto Cpp::Statement::createCase(const LangAPI::RValue &rvalue) -> void {
    Core::output->writeln("case {}: {", Core::convertRValue(rvalue));
    Core::output->increaseIndentation();
}
auto Cpp::Statement::closeCase() -> void {
    Core::output->writeln("break;");
    Core::output->decreaseIndentation();
    Core::output->writeln("}");
}
auto Cpp::Statement::closeSwitch() -> void {
    Core::output->decreaseIndentation();
    Core::output->writeln("}");
}
auto Cpp::Statement::createExpression(const LangAPI::Expression &expression) -> void {
    Core::output->writeln("{};", Core::convertExpression(expression));
}

auto Cpp::Statement::createThrow(const LangAPI::Throw &t) -> void {
    Core::output->writeln("throw {};", Core::convertExpression(t.throw_value));
}

Converter::Writer &Cpp::Statement::getWriter() {
    return Core::h_file;
}
Converter::Writer &Cpp::Statement::getWriter() const {
    return Core::h_file;
}

extern "C" Cpp::Statement* create_cpp_statement(Converter::Writer *output) {
    return new Cpp::Statement(*Core::output);
}
extern "C" void delete_cpp_statement(Cpp::Statement *decl) {
    delete decl;
}