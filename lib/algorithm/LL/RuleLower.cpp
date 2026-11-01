module RuleLower;

import MemLower;
import LangAPI.Context;
import UAST.PyAST;
import ParserIR.API;
import logging;
import corelib;
import std;

namespace {
    // Qualified names of the imported Python classes: calls to them construct objects.
    std::set<std::string> pythonClasses(const LangAPI::Declarations& declarations, const std::string& prefix = "") {
        std::set<std::string> names;
        for (const auto& declaration : declarations) {
            if (declaration.isNamespace()) {
                const auto& ns = declaration.getNamespace();
                const auto nested = pythonClasses(ns.declarations, prefix + ns.name + ".");
                names.insert(nested.begin(), nested.end());
            } else if (declaration.isClass()) {
                const auto& cls = declaration.getClass();
                std::string name = prefix;
                for (const auto& part : cls.name.path) if (const auto* value = std::get_if<std::string>(&part)) {
                    if (name.size() > prefix.size()) name += ".";
                    name += *value;
                }
                names.insert(name);
            }
        }
        return names;
    }
}

auto RuleLower::register_function(const LangAPI::Function &fun) -> void {
    decls.push_back(fun);
}

void RuleLower::build(const std::vector<std::string> name, const AST::Rule &rule) {
    LangAPI::Function fun;
    fun.name = fun_name(name);

    FunctionCtx fun_ctx;
    fun_ctx.statements = &fun.statements;
    fun_ctx.rule_name = AST::LLkToken(name.begin(), name.end());
    for (const auto& feature : rule.features)
        if (const auto* syntax = std::get_if<AST::DeclarationSyntax>(&feature))
            for (const auto& parameter : syntax->parameters) {
                if (parameter == "ctx") throw std::invalid_argument("ctx is reserved and cannot be a rule parameter");
                if (!fun_ctx.action_locals.insert(parameter).second)
                    throw std::invalid_argument("duplicate rule parameter: " + parameter);
            }

    auto hasCounter = [&](auto&& self, const AST::RuleMember& member) -> bool {
        for (const auto& feature : member.features)
            if (std::holds_alternative<AST::Counter>(feature) ||
                std::holds_alternative<AST::SharedCounterRegistration>(feature)) return true;
        if (member.isGroup()) for (const auto& child : member.getGroup().values) if (self(self, *child)) return true;
        if (member.isOp()) for (const auto& child : member.getOp().options) if (self(self, *child)) return true;
        return false;
    };
    if (std::ranges::any_of(rule.rule_members, [&](const auto& member) { return hasCounter(hasCounter, *member); })) {
        LangAPI::Variable counter_scope;
        counter_scope.name = "counter_bindings_scope";
        counter_scope.type = LangAPI::Type{LangAPI::Symbol{"auto"}};
        counter_scope.value = LangAPI::FunctionCall::createExpression(LangAPI::FunctionCall{
            .name = std::make_shared<LangAPI::Symbol>("counter_scope")});
        fun.statements.push_back(LangAPI::Variable::createStatement(counter_scope));
    }

    LangAPI::Variable frame;
    frame.name = "recovery_scope";
    frame.type = LangAPI::Type{LangAPI::Symbol{"auto"}};
    frame.value = recoveryFrame(fun_ctx.rule_name);
    fun.statements.push_back(LangAPI::Variable::createStatement(frame));

    std::size_t idx = 0;
    for (const auto &rule_member : rule.rule_members) {
        LangAPI::Statements member_statements;
        fun_ctx.statements = &member_statements;
        member_statements.push_back(LangAPI::Expression::createStatement(
            recoverySite(*tree, fun_ctx.rule_name, idx)));
        MemLower lower(ctx, fun_ctx, *tree, idx);
        const auto previous_captures = fun_ctx.captures.size();
        lower.build(*rule_member);
        // Publish captures as they become available, so a later fail block
        // observes the partial AST and its edits are not overwritten at return.
        for (std::size_t i = previous_captures; i < fun_ctx.captures.size(); ++i) {
            std::string field;
            if (rule.data_block.type) field = "value";
            else if (i < rule.data_block.fields.size()) field = rule.data_block.fields[i].first;
            else continue;
            const auto& meta = fun_ctx.captures[i].meta;
            const auto& value = meta.value.name.empty() ? meta.value_list : meta.value;
            LangAPI::StorageSymbol destination{field};
            destination.what = LangAPI::Symbol::createExpression(LangAPI::Symbol{"result"});
            member_statements.push_back(LangAPI::IspaLibFunctionCall::createStatement(LangAPI::IspaLibFunctionCall{
                .symbol = LangAPI::IspaLibSymbol{.exports = LangAPI::StdlibExports::AssignCompatible},
                .args = {LangAPI::StorageSymbol::createExpression(destination),
                    LangAPI::Symbol::createExpression(LangAPI::Symbol{value.name})}}));
        }
        LangAPI::Expression condition{LangAPI::ExpressionValue{LangAPI::ExpressionElement::Not}};
        const auto call = LangAPI::FunctionCall::createExpression(LangAPI::FunctionCall{
            .name = std::make_shared<LangAPI::Symbol>("unwinding")});
        condition.insert(condition.end(), call.begin(), call.end());
        condition.erase(condition.begin()); // unwinding(): leave this invocation before further actions
        fun.statements.push_back(LangAPI::If::createStatement(LangAPI::If{condition,
            LangAPI::Return::createStatements(LangAPI::Return{
                .value = LangAPI::FunctionCall::createExpression(LangAPI::FunctionCall{
                    .name = std::make_shared<LangAPI::Symbol>("action_result"),
                    .args = {LangAPI::Symbol::createExpression(LangAPI::Symbol{"result"}),
                        LangAPI::Symbol::createExpression(LangAPI::Symbol{"pos"})}})})}));
        fun.statements.insert(fun.statements.end(), member_statements.begin(), member_statements.end());
        ++idx;
    }
    fun_ctx.statements = &fun.statements;
    for (const auto& feature : rule.features) if (const auto* action = std::get_if<AST::PythonAction>(&feature);
        action && !action->target.empty()) {
        ParserIR::specializeTypedClass(tree->getPythonDeclarations(), *action, fun_ctx.action_types, fun.name);
        auto statements = convertPythonAction(*action, fun_ctx.action_locals, false,
            pythonClasses(tree->getPythonDeclarations()), &fun_ctx.action_objects);
        fun.statements.insert(fun.statements.end(), statements.begin(), statements.end());
    }

    // Member lowering hoists every value variable so that values produced
    // inside optional / repeated / chosen blocks stay visible to later members
    // and to the data block.
    fun.statements.insert(fun.statements.begin(),
                          fun_ctx.declarations.begin(), fun_ctx.declarations.end());
    register_function(fun);
    lowered_functions.insert_or_assign(name, LoweredFunction {
        .function = std::move(fun), .exports = std::move(fun_ctx.exports),
        .captures = std::move(fun_ctx.captures)
    });
}

void RuleLower::build(const AST::Tree &tree) {
    for (const auto &[name, rule] : tree.getTreeMap()) {
        if (corelib::text::isUpper(name.back())) {
            continue;
        }
        build(name, rule);
    }
}
