module LLIR.CllBuilder;
import LLIR.IR;
import LLIR.Rule.MemberBuilder;
import LLIR.RValueBuilder;
import logging;
import cpuf.printf;
import dstd;
import std;

auto LLIR::CllExprBuilder::CllExprGroupToIR(const AST::CllExpr &group) -> LangAPI::Expression {
    LangAPI::Expression expr;
    auto expression = CllExprLogicalToIR(group.value);
    expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::GroupOpen });
    expr.insert(expr.end(), expression.begin(), expression.end());
    expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::GroupClose });
    return expr;
}
auto LLIR::CllExprBuilder::CllExprValueToIR(const AST::CllExprValue &value) -> LangAPI::Expression {
    LangAPI::Expression expr;
    if (value.isGroup()) {
        expr = CllExprGroupToIR(value.getGroup().expr);
    } else if (value.isMethodCall()) {
        CllMethodCallBuilder builder(*this, value.getMethodCall());
        builder.build();
        expr = LangAPI::StorageSymbol::createExpression(std::move(builder.get()));
    } else if (value.isFunctionCall()) {
        CllFunctionBuilder builder(*this, value.getFunctionCall());
        builder.build();
        expr = LangAPI::FunctionCall::createExpression(std::move(builder.get()));
    } else if (value.isVariable()) {
        const auto &v = value.getVariable();
        LangAPI::Symbol variable_mention;
        BuilderBase::pushVariablePrefix(expr, v.pre_increament);
        auto it = std::find_if(vars.begin(), vars.end(), [&](const LangAPI::Variable &var) { return var.name == v.name; });
        if (it == vars.end()) {
            throw Error("Undefined variable in AST: {}", v.name);
        }
        if (it->type.isValueType() && it->type.getValueType() == LangAPI::ValueType::TokenResult || it->type.getValueType() == LangAPI::ValueType::RuleResult) {
            expr.push_back(LangAPI::StorageSymbol::createExpressionValue(LangAPI::StorageSymbol {
                    LangAPI::Symbol::createExpression(LangAPI::Symbol {v.name}), stdu::vector<LangAPI::StorageSymbol::PathPart> {LangAPI::IspaLibSymbol {.exports = LangAPI::StdlibExports::MatchResultValue}}
            }));
        } else {
            expr.push_back(LangAPI::Symbol::createExpressionValue(LangAPI::Symbol {v.name}));
        }
        if (v.braceExpression.has_value()) {
            expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::SquareBraceOpen });
            CllExprBuilder brace_expr(*this, v.braceExpression.value());
            expr.insert(expr.end(), brace_expr.get().begin(), brace_expr.get().end());
            expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::SquareBraceClose });
        }

        BuilderBase::pushVariablePrefix(expr, v.post_increament);
    } else if (value.isrvalue()) {
        if (rvalueBuilder == nullptr) {
            rvalueBuilder = std::make_unique<RValueBuilder>(RValueBuilder(*this, value.getrvalue()));
            rvalueBuilder->build();
        }
        expr = LangAPI::RValue::createExpression(rvalueBuilder->get());
    } else throw Error("Undefined expression member in AST");
    return expr;
}
auto LLIR::CllExprBuilder::CllExprTermToIR(const AST::CllExprTerm &term) -> LangAPI::Expression {
    // if (rule.name == Parser::Rules::cll_function_call)
    auto cond = CllExprValueToIR(term.value);
    for (int i = 0; i < term.rights.size(); i++) {
        cond.push_back(LangAPI::ExpressionValue { BuilderBase::CllOpToExpr(term.rights[i].first) });
        auto res = CllExprValueToIR(term.rights[i].second);
        cond.insert(cond.end(), res.begin(), res.end());
    }
    return cond;
}
auto LLIR::CllExprBuilder::CllExprAdditionToIR(const AST::CllExprAddition &addition) -> LangAPI::Expression {
    // if (rule.name == Parser::Rules::cll_function_call)
    LangAPI::Expression cond = CllExprTermToIR(addition.value);
    for (int i = 0; i < addition.rights.size(); i++) {
        cond.push_back(LangAPI::ExpressionValue { BuilderBase::CllOpToExpr(addition.rights[i].first) });
        auto res = CllExprTermToIR(addition.rights[i].second);
        cond.insert(cond.end(), res.begin(), res.end());
    }
    return cond;
}
auto LLIR::CllExprBuilder::CllExprCompareToIR(const AST::CllExprCompare &compare) -> LangAPI::Expression {
    LangAPI::Expression cond = CllExprAdditionToIR(compare.value);
    for (int i = 0; i < compare.rights.size(); i++) {
        cond.push_back(LangAPI::ExpressionValue { BuilderBase::CllCompareOpToExpr(compare.rights[i].first) });
        auto res = CllExprAdditionToIR(compare.rights[i].second);
        cond.insert(cond.end(), res.begin(), res.end());
    }
    return cond;
}
auto LLIR::CllExprBuilder::CllExprLogicalToIR(const AST::CllExprLogical &logical) -> LangAPI::Expression {
    LangAPI::Expression cond = CllExprCompareToIR(logical.value);
    for (int i = 0; i < logical.rights.size(); i++) {
        cond.push_back(LangAPI::ExpressionValue { BuilderBase::CllLogicalOpToIR(logical.rights[i].first)});
        auto res = CllExprCompareToIR(logical.rights[i].second);
        cond.insert(cond.end(), res.begin(), res.end());
    }
    return cond;
}
auto LLIR::CllExprBuilder::deduceTypeFromExprValue(const AST::CllExprValue &value) -> LangAPI::Type {
    if (value.isFunctionCall()) {
        // todo - get function call type
        throw Error("Not implemented");
    } else if (value.isGroup()) {
        CllExprBuilder builder(*this, value.getGroup().expr);
        builder.build();
        return builder.deduceType();
    } else if (value.isMethodCall()) {
        // todo - get method call type
        throw Error("Not implemented");
    } else if (value.isVariable()) {
        auto find_it = std::find_if(vars.begin(), vars.end(), [&value](const LangAPI::Variable &var) { return var.name == value.getVariable().name; });
        if (find_it == vars.end())
            throw Error("Not found variable to deduce type from expr: {}",  value.getVariable().name);
        auto type = find_it->type;
        if (type.isValueType())
            BuilderBase::undoRuleResult(type.getValueType());
        return type;
    } else if (value.isrvalue()) {
        if (rvalueBuilder == nullptr) {
            rvalueBuilder = std::make_unique<RValueBuilder>(RValueBuilder(*this, value.getrvalue()));
            rvalueBuilder->build();
        }
        return rvalueBuilder->deduceType();
    } else
        throw Error("Undefined expr value member");
    return {};
}
auto LLIR::CllExprBuilder::deduceTypeFromExprTerm(const AST::CllExprTerm &term) -> LangAPI::Type {
    // type is explicitly based on value. We may not check others in addition
    return deduceTypeFromExprValue(term.value);
}
auto LLIR::CllExprBuilder::deduceTypeFromExprAddition(const AST::CllExprAddition &addition) -> LangAPI::Type {
    // same as with term
    return deduceTypeFromExprTerm(addition.value);
}
auto LLIR::CllExprBuilder::deduceTypeFromExprCompare(const AST::CllExprCompare &compare) -> LangAPI::Type {
    // if any comparasion exists it is boolean
    if (compare.rights.size() != 0)
        return {LangAPI::ValueType::Bool};
    return deduceTypeFromExprAddition(compare.value);
}
auto LLIR::CllExprBuilder::deduceTypeFromExprLogical(const AST::CllExprLogical &logical) -> LangAPI::Type {
    // if &&/|| exists it is always boolean
    if (logical.rights.size() != 0)
        return {LangAPI::ValueType::Bool};
    return deduceTypeFromExprCompare(logical.value);
}
auto LLIR::CllExprBuilder::deduceType() -> LangAPI::Type {
    return deduceTypeFromExprLogical(expr.value);
}
auto LLIR::CllFunctionBuilder::FunctionBodyCallToIR(const AST::CllFunctionBodyCall &body) -> stdu::vector<LangAPI::Expression> {
    stdu::vector<LangAPI::Expression> newExpr;
    for (const auto &expr : body.expr) {
        CllExprBuilder builder(*this, expr);
        builder.build();
        newExpr.push_back(builder.get());
    }
    return newExpr;
}


void LLIR::CllBuilder::build() {
    addSpaceSkip = false;
    std::unique_ptr<BuilderBase> builder = nullptr;

    if (cll.isVar()) {
        builder = std::make_unique<CllVarBuilder>(*this, cll.getVar());
    } else if (cll.isIf()) {
        builder = std::make_unique<CllIfBuilder>(*this, cll.getIf());
    } else {
        throw Error("Undefined cll entry");
    }

    builder->build();
    statements.insert(statements.end(), builder->getData().begin(), builder->getData().end());
}
void LLIR::CllVarBuilder::build() {
    // get data section
    LangAPI::Variable variable {.name = var.name};
    if (var.type.type.empty()) {
        variable.type = CllTypeToIR(var.type);
        if (var.value.has_value()) {
            CllExprBuilder builder(*this, var.value.value());
            builder.build();
            variable.value = builder.get();
        }
    }
    statements.push_back(var.type.type.empty() ?
        LangAPI::VariableAssignment::createStatement(LangAPI::VariableAssignment {.name = LangAPI::Symbol {variable.name}, .value = std::move(variable.value)})
            :
        LangAPI::Variable::createStatement(variable)
    );
}
void LLIR::CllIfBuilder::build() {
    MemberBuilder rules_builder(*this, cond.stmt);
    CllExprBuilder expr_builder(*this, cond.expr);
    rules_builder.build();
    expr_builder.build();
    statements.push_back(LangAPI::If::createStatement(LangAPI::If {std::move(expr_builder.get()), std::move(rules_builder.getData())}));
}

void LLIR::CllExprBuilder::build() {
    result = CllExprLogicalToIR(expr.value);
}
void LLIR::CllFunctionBuilder::build() {
    result.name = std::make_shared<LangAPI::Symbol>(call.name);
    result.args = FunctionBodyCallToIR(call.body);
}
void LLIR::CllMethodCallBuilder::build() {
    LangAPI::StorageSymbol symbol;
    symbol.what = LangAPI::Symbol::createExpression(LangAPI::Symbol {call.name});
    CllFunctionBuilder fun(*this, call.body);
    fun.build();
    result.path = {fun.get()};
}