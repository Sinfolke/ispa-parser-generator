module LLIR.Rule.MemberBuilder;
import LLIR.CllBuilder;
import logging;
import corelib;
import cpuf.hex;
import cpuf.op;
import cpuf.printf;
import Dump;
import NFA.IR;
import NFA.TNFA;
import DFA;
import constants;
import std;
// helper functions
void LLIR::GroupBuilder::pushBasedOnQuantifier(
    MemberBuilder &builder,
    const AST::RuleMember &rule,
    LangAPI::Variable &shadow_var,
    LangAPI::Variable &uvar,
    const LangAPI::Variable &var,
    char quantifier
    ) {
    if (!uvar.name.empty()) {
        uvar.type = deduceUvarType(var, shadow_var);
        statements.push_back(LangAPI::Variable::createStatement(uvar));
    }
    if (quantifier == '*' || quantifier == '+') {
        LangAPI::While loop;
        loop.expr = LangAPI::Bool::createExpression(LangAPI::Bool { .value = true });
        loop.stmt = builder.getData();
        builder.getData().clear();
        processExitStatements(loop.stmt);
        // raiseVarsTop(data, loop.block, true, false, false);
        // raiseVarsTop(data, loop.else_block, true, false, false);
        if (quantifier == '+') {
            handle_plus_qualifier(rule, std::move(loop), uvar, var, shadow_var);
        } else {
            statements.push_back(LangAPI::While::createStatement(loop));
        }
    } else if (quantifier == '?') {
        LangAPI::While loop;
        loop.expr = LangAPI::Bool::createExpression(LangAPI::Bool { .value = true });
        loop.stmt = builder.getData();
        builder.getData().clear();
        processExitStatements(loop.stmt);
        statements.push_back(LangAPI::DoWhile::createStatement(loop));
    }
}

auto LLIR::NameBuilder::pushBasedOnQualifier(
    const AST::RuleMember &rule,
    LangAPI::Expression &expr,
    LangAPI::Statements &stmt,
    LangAPI::Variable &uvar,
    const LangAPI::Variable &var,
    const LangAPI::Variable &svar,
    const LangAPI::Statement &call,
    char quantifier,
    stdu::vector<std::string> &name,
    bool add_shadow_var
) -> LangAPI::Variable {
    (void)name;
    (void)add_shadow_var;

    statements.insert(statements.end(), stmt.begin(), stmt.end());
    LangAPI::Variable shadow_variable;
    if (!uvar.name.empty()) {
        uvar.type = deduceUvarType(var, shadow_variable);
        statements.push_back(LangAPI::Variable::createStatement(uvar));
    }

    LangAPI::Variable extracted = uvar;
    if ((insideLoop || quantifier == '+' || quantifier == '*') &&
        extracted.name.empty()) {
        extracted = createEmptyVariable("uvar" + generateVariableName());
        extracted.type = deduceUvarType(var, {});
        statements.push_back(LangAPI::Variable::createStatement(extracted));
    }
    LangAPI::Statements shadow_append;
    if (insideLoop || quantifier == '+' || quantifier == '*') {
        shadow_variable = add_shadow_variable(statements, shadow_append, extracted);
    }

    LangAPI::Statements successful;
    // FIX: a successful call must mark svar true. Without this, every
    // generated call site that later tests svar (e.g. OpBuilder's
    // "if (!selected.svar) return {};") fails unconditionally, even
    // when the called rule actually matched.
    successful.push_back(assignSvar(svar, true));
    successful.push_back(LangAPI::AssignCounter::createStatement(
        LangAPI::AssignCounter{
            .v = std::make_shared<LangAPI::Expression>(
                LangAPI::StorageSymbol::createExpression(
                    LangAPI::StorageSymbol{
                        LangAPI::Symbol::createExpression(
                            LangAPI::Symbol{var.name}),
                        stdu::vector<LangAPI::StorageSymbol::PathPart>{"it"}
                    }))
        }));
    createAssignUvarBlock(successful, extracted, var, shadow_variable);
    successful.insert(successful.end(), shadow_append.begin(),
                      shadow_append.end());

    if (quantifier == '*' || quantifier == '+') {
        LangAPI::Statements loop_body{call};
        LangAPI::Expression failed = expr;
        failed.insert(failed.begin(), LangAPI::ExpressionValue{
            LangAPI::ExpressionElement::Not});
        failed.insert(failed.begin() + 1, LangAPI::ExpressionValue{
            LangAPI::ExpressionElement::GroupOpen});
        failed.push_back(LangAPI::ExpressionValue{
            LangAPI::ExpressionElement::GroupClose});
        loop_body.push_back(LangAPI::If::createStatement(
            LangAPI::If{std::move(failed), LangAPI::Break::createStatements(LangAPI::Break{})}));
        loop_body.insert(loop_body.end(), successful.begin(), successful.end());
        auto loop = LangAPI::While{
            LangAPI::Bool::createExpression(LangAPI::Bool{.value = true}),
            std::move(loop_body)
        };
        if (quantifier == '+') {
            handle_plus_qualifier(rule, std::move(loop), uvar, var,
                                  shadow_variable);
        } else {
            statements.push_back(LangAPI::While::createStatement(loop));
        }
    } else {
        statements.push_back(call);
        LangAPI::Expression failed = expr;
        failed.insert(failed.begin(), LangAPI::ExpressionValue{
            LangAPI::ExpressionElement::Not});
        failed.insert(failed.begin() + 1, LangAPI::ExpressionValue{
            LangAPI::ExpressionElement::GroupOpen});
        failed.push_back(LangAPI::ExpressionValue{
            LangAPI::ExpressionElement::GroupClose});
        if (quantifier == '?') {
            statements.push_back(LangAPI::If::createStatement(
                LangAPI::If{expr, std::move(successful)}));
        } else {
            statements.push_back(LangAPI::If::createStatement(
                LangAPI::If{std::move(failed), LangAPI::Return::createStatements(LangAPI::Return{})}));
            statements.insert(statements.end(), successful.begin(),
                              successful.end());
        }
    }
    return shadow_variable;
}
auto LLIR::NameBuilder::createAssignUvarBlock(LangAPI::Statements &statements, const LangAPI::Variable &uvar, const LangAPI::Variable &var, const LangAPI::Variable &shadow_var) -> void {
    if (!uvar.name.empty()) {
        statements.push_back(LangAPI::VariableAssignment::createStatement(LangAPI::VariableAssignment {
                .name = LangAPI::Symbol {uvar.name},
                .value = LangAPI::StorageSymbol::createExpression(LangAPI::StorageSymbol {
                    LangAPI::Symbol::createExpression(LangAPI::Symbol {var.name}), stdu::vector<LangAPI::StorageSymbol::PathPart> {LangAPI::IspaLibSymbol {.exports = LangAPI::StdlibExports::MatchResultValue}}
                })
            })
        );
    }
}
void LLIR::MemberBuilder::buildMember(const AST::RuleMember &member) {
    std::unique_ptr<BuilderBase> builder;
    if (member.isGroup()) {
        builder = std::make_unique<GroupBuilder>(*this, member);
    } else if (member.isCsequence()) {
        builder = std::make_unique<CsequenceBuilder>(*this, member);
    } else if (member.isString()) {
        builder = std::make_unique<StringBuilder>(*this, member);
    // } else if (member.isHex()) {
    //     builder = std::make_unique<HexBuilder>(*this, member);
    // } else if (member.isBin()) {
    //     builder = std::make_unique<BinBuilder>(*this, member);
    } else if (member.isName()) {
        builder = std::make_unique<NameBuilder>(*this, member);
    // } else if (member.isEscaped()) {
    //     builder = std::make_unique<EscapedBuilder>(*this, member);
    } else if (member.isNospace()) {
        builder = std::make_unique<NospaceBuilder>(*this);
    } else if (member.isOp()) {
        builder = std::make_unique<OpBuilder>(*this, member);
    } else if (member.isAny()) {
        builder = std::make_unique<AnyBuilder>(*this, member);
    } else if (member.isCll()) {
        builder = std::make_unique<CllBuilder>(*this, member.getCll());
    } else if (member.empty()) {
        throw Error("Empty rule");
    } else throw Error("Undefined rule");
    builder->build();
    exports_list.insert(exports_list.end(), builder->getReturnVars().begin(), builder->getReturnVars().end());
    statements.insert(statements.end(), builder->getData().begin(), builder->getData().end());
    isFirst = false;
}
auto LLIR::MemberBuilder::build() -> void {
    isToken = corelib::text::isUpper(fullname.back());
    if (rules == nullptr) {
        buildMember(*rule);
        return;
    }
    for (const auto &mem_ptr : *rules) {
        buildMember(*mem_ptr);
    }
}

void LLIR::GroupBuilder::build() {
    const auto& quantifier = rule.quantifier;
    const auto& group = rule.getGroup().values;

    const bool is_repeated =
        quantifier == '*' || quantifier == '+';

    const bool was_inside_loop = insideLoop;
    const bool needs_shadow = was_inside_loop || is_repeated;

    auto var = createEmptyVariable(
        "group" + generateVariableName()
    );

    const bool uvar_is_named = !rule.prefix.name.empty();

    auto uvar = createEmptyVariable(
        uvar_is_named
            ? rule.prefix.name
            : "uvar" + generateVariableName()
    );

    auto svar = createSuccessVariable();

    // Build the group's members in the correct loop context.
    insideLoop = needs_shadow;

    MemberBuilder builder(*this, group);
    builder.build();

    insideLoop = was_inside_loop;

    const auto& return_vars = builder.getReturnVars();

    exports_list.insert(
        exports_list.end(),
        return_vars.begin(),
        return_vars.end()
    );

    // Remove an unnecessary trailing whitespace skip.
    if (!builder.getData().empty() &&
        builder.getData().back().isExpression()) {

        const auto& expr =
            builder.getData().back().getExpression();

        if (expr.size() == 1 &&
            expr.back().isSkipSpaces()) {
            builder.pop();
        }
    }

    // Deduce the actual value type before array promotion.
    var.type = deduceVarTypeByRuleMember(rule);

    LangAPI::Statements fetch_var_statements;

    // Keep element type and result type distinct.
    LangAPI::Type element_type = var.type;

    if (var.type == LangAPI::ValueType::Array &&
        !var.type.template_parameters.empty()) {
        element_type = std::get<LangAPI::Type>(
            var.type.template_parameters.front()
        );
    }

    switch (element_type.getValueType()) {
        case LangAPI::ValueType::String: {
            for (const auto& ret : return_vars) {
                if (ret.var.name.empty())
                    continue;

                fetch_var_statements.push_back(
                    LangAPI::VariableAssignment::createStatement(LangAPI::VariableAssignment {
                        .name = LangAPI::Symbol{var.name},
                        .value = LangAPI::Symbol::createExpression(
                            LangAPI::Symbol{ret.var.name}
                        )
                    })
                );
            }
            break;
        }

        // case LangAPI::ValueType::Token:
        // case LangAPI::ValueType::Rule:
        // case LangAPI::ValueType::Variant:
        default: {
            // Never dereference an empty return list.
            if (return_vars.empty() ||
                return_vars.front().var.name.empty()) {
                var.type = {LangAPI::ValueType::Undef};
                break;
            }

            const auto& ret = return_vars.front();

            // Preserve a previously deduced array type.
            if (var.type != LangAPI::ValueType::Array) {
                var.type = ret.var.type;

                if (var.type.isValueType()) {
                    undoRuleResult(var.type.getValueType());
                }
            }

            fetch_var_statements.push_back(
                LangAPI::VariableAssignment::createStatement(LangAPI::VariableAssignment {
                    .name = LangAPI::Symbol{var.name},
                    .value = LangAPI::Symbol::createExpression(
                        LangAPI::Symbol{ret.var.name}
                    )
                })
            );
            break;
        }
    }

    // Keep var as one iteration's value; shadow_var collects repeated values.
    if (var.type.getValueType() == LangAPI::ValueType::Undef) {
        var = {};
        uvar = {};
    } else {
        uvar.type = var.type;
    }
    // Assign the final type, not an intermediate one.

    // Construct group success expression.
    LangAPI::If group_success_condition{};

    stdu::vector<std::string> used_vars;

    bool first = true;

    for (const auto& ret : return_vars) {
        if (ret.quantifier == '*' ||
            ret.quantifier == '?' ||
            ret.svar.name.empty()) {
            continue;
        }

        if (!first) {
            group_success_condition.expr.push_back(
                LangAPI::ExpressionValue{
                    LangAPI::ExpressionElement::And
                }
            );
        }

        used_vars.push_back(ret.svar.name);

        group_success_condition.expr.push_back(
            LangAPI::Symbol::createExpressionValue(
                LangAPI::Symbol{ret.svar.name}
            )
        );

        first = false;
    }

    // Declare the result before emitting operations.
    if (var.type != LangAPI::ValueType::Undef) {
        statements.push_back(
            LangAPI::Variable::createStatement(var)
        );
    }

    statements.push_back(
        LangAPI::Variable::createStatement(svar)
    );

    // Shadow variables are needed for repeated/grouped values.
    LangAPI::Variable shadow_var;
    LangAPI::Statements shadow_var_assign_block;

    if (needs_shadow &&
        var.type != LangAPI::ValueType::Undef &&
        var.type != LangAPI::ValueType::String) {

        shadow_var = add_shadow_variable(
            statements,
            shadow_var_assign_block,
            var
        );
    }

    const std::string pos_counter_name =
        "begin" + generateVariableName();

    statements.push_back(
        LangAPI::PushPosCounter::createStatement(LangAPI::PushPosCounter {
            .name = pos_counter_name
        })
    );

    // Repeated groups accumulate each successful iteration, not after the loop.
    if (is_repeated && !shadow_var_assign_block.empty()) {
        LangAPI::If iteration_success = group_success_condition;
        iteration_success.stmt = fetch_var_statements;
        iteration_success.stmt.insert(
            iteration_success.stmt.end(),
            shadow_var_assign_block.begin(),
            shadow_var_assign_block.end()
        );
        if (iteration_success.expr.empty()) {
            builder.getData().insert(builder.getData().end(),
                iteration_success.stmt.begin(), iteration_success.stmt.end());
        } else {
            builder.getData().push_back(LangAPI::If::createStatement(iteration_success));
        }
    }

    // Emit the group's parsing operations.
    pushBasedOnQuantifier(
        builder,
        rule,
        shadow_var,
        uvar,
        var,
        quantifier
    );

    for (const auto& success_name : used_vars) {
        raiseVarsTop(
            statements,
            statements,
            success_name,
            true,
            false,
            true
        );
    }

    // Only perform value extraction after successful parsing.
    auto& success_statements = group_success_condition.stmt;

    // Fetch group values before reading the result variable.
    if (!is_repeated) {
        success_statements.insert(
            success_statements.end(),
            fetch_var_statements.begin(),
            fetch_var_statements.end()
        );
    }

    // Initialize/update shadow values before using them.
    if (!is_repeated) {
        success_statements.insert(
            success_statements.end(),
            shadow_var_assign_block.begin(),
            shadow_var_assign_block.end()
        );
    }

    success_statements.push_back(
        LangAPI::VariableAssignment::createStatement(LangAPI::VariableAssignment {
            .name = LangAPI::Symbol{svar.name},
            .value = LangAPI::Bool::createExpression(LangAPI::Bool {
                .value = true
            })
        })
    );

    const bool has_assignable_value =
        var.type != LangAPI::ValueType::Undef ||
        !shadow_var.name.empty();

    if (has_assignable_value) {
        const auto& source_name =
            shadow_var.name.empty()
                ? var.name
                : shadow_var.name;

        success_statements.push_back(
            LangAPI::VariableAssignment::createStatement(LangAPI::VariableAssignment {
                .name = LangAPI::Symbol{uvar.name},
                .value = LangAPI::Symbol::createExpression(
                    LangAPI::Symbol{source_name}
                )
            })
        );
    }

    success_statements.push_back(
        LangAPI::PopPosCounter::createStatement(LangAPI::PopPosCounter {})
    );

    if (group_success_condition.expr.empty()) {
        statements.insert(
            statements.end(),
            success_statements.begin(),
            success_statements.end()
        );
    } else {
        statements.push_back(
            LangAPI::If::createStatement(
                group_success_condition
            )
        );
    }

    pushConvResult(
        rule,
        var,
        uvar,
        svar,
        shadow_var,
        quantifier
    );
}

void LLIR::CsequenceBuilder::build() {
    //cpuf::printf("csequence\n");
    const auto &csequence = rule.getCsequence();

    auto uvar = !rule.prefix.name.empty() ? createEmptyVariable(rule.prefix.name) : createEmptyVariable("");
    auto var = createEmptyVariable(generateVariableName());
    auto svar = createSuccessVariable();
    LangAPI::Expression expr;

    if (csequence.negative) {
        expr = {
            LangAPI::ExpressionValue { LangAPI::ExpressionElement::Not},
            LangAPI::ExpressionValue { LangAPI::ExpressionElement::GroupOpen}
        };
    }

    bool first = true;
    std::size_t count = 0;
    auto push_comparasion_with_character_to_expression = [&](LangAPI::Char c) {
        if (!first)
            expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::Or });
        expr.push_back(LangAPI::Pos::createExpressionValue(LangAPI::Pos {.dereference = true}));
        expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::Equal });
        expr.push_back(LangAPI::Char::createExpressionValue(c));
    };
    for (const auto c : csequence.characters) {
        push_comparasion_with_character_to_expression(LangAPI::Char {.value = c});
        first = false;
    }
    for (const auto c : csequence.escaped) {
        push_comparasion_with_character_to_expression(LangAPI::Char {.value = c, .escaped = true});
        first = false;
    }
    for (const auto &[from, to] : csequence.diapasons) {
        if (!first)
            expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::Or});
        expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::GroupOpen});
        expr.push_back(LangAPI::Pos::createExpressionValue(LangAPI::Pos {.dereference = true}));
        expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::HigherOrEqual});
        expr.push_back(LangAPI::Char::createExpressionValue(LangAPI::Char {.value = from}));
        expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::And});
        expr.push_back(LangAPI::Pos::createExpressionValue(LangAPI::Pos {.dereference = true}));
        expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::LowerOrEqual});
        expr.push_back(LangAPI::Char::createExpressionValue(LangAPI::Char {.value = to}));
        expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::GroupClose});
        first = false;
    }
    if (csequence.negative) {
        if (rule.quantifier == '+' || rule.quantifier == '*') {
            expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::LowerOrEqual});
            expr.push_back(LangAPI::Pos::createExpressionValue(LangAPI::Pos {.dereference = true}));
            expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::NotEqual});
            expr.push_back(LangAPI::Char::createExpressionValue(LangAPI::Char {.value = '0', .escaped = true}));
        }
        expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::GroupClose});
    }
    if (rule.quantifier == '\0')
        var.type = { LangAPI::ValueType::Char };
    else
        var.type = {LangAPI::ValueType::String};
    statements.push_back(LangAPI::Variable::createStatement(var));
    statements.push_back(LangAPI::Variable::createStatement(svar));
    auto stmt = createDefaultStatements(var, svar);
    auto shadow_var = pushBasedOnQualifier(rule, expr, stmt, uvar, var, svar, rule.quantifier, false);
    pushConvResult(rule, var, uvar, svar, shadow_var, rule.quantifier);
}
void LLIR::StringBuilder::build() {
    const auto &str = rule.getString();
    const auto &value = str.value;
    auto uvar = !rule.prefix.name.empty() ? createEmptyVariable(rule.prefix.name) : createEmptyVariable("");
    auto var = createEmptyVariable(generateVariableName());
    auto svar = createSuccessVariable();
    LangAPI::Expression expr;
    if (value.size() == 0)
        return;
    if (str.count_strlen() == 1) {
        // micro optimization - compare as single character for single character strings
        expr = {
            LangAPI::Pos::createExpressionValue(LangAPI::Pos {.dereference = true}),
            LangAPI::ExpressionValue { LangAPI::ExpressionElement::Equal},
            value[0] == '\\' ? LangAPI::Char::createExpressionValue(LangAPI::Char {.value = value[1]}) : LangAPI::Char::createExpressionValue(LangAPI::Char {.value = value[1], .escaped = true})
        };
        var.type.type = LangAPI::ValueType::Char;
    } else {
        expr = {
            LangAPI::StringCompare::createExpressionValue(LangAPI::StringCompare {.str = LangAPI::String { .value = value }, .is_string = true})
        };
        var.type = {LangAPI::ValueType::Char};
    }
    if (corelib::text::isAllAlpha(value)) {
        expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::And});
        expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::Not});
        expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::GroupOpen});

        // current >= 'a'
        expr.push_back(LangAPI::Pos::createExpressionValue(LangAPI::Pos {.dereference = true, .offset = str.count_strlen()}));
        expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::HigherOrEqual});
        expr.push_back(LangAPI::Char::createExpressionValue(LangAPI::Char {.value = 'a', .escaped = true}));

        expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::And});

        // current <= 'z'
        expr.push_back(LangAPI::Pos::createExpressionValue(LangAPI::Pos {.dereference = true, .offset = str.count_strlen()}));
        expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::LowerOrEqual});
        expr.push_back(LangAPI::Char::createExpressionValue(LangAPI::Char {.value = 'z', .escaped = true}));

        expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::Or});

        // current >= 'A'
        expr.push_back(LangAPI::Pos::createExpressionValue(LangAPI::Pos {.dereference = true, .offset = str.count_strlen()}));
        expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::HigherOrEqual});
        expr.push_back(LangAPI::Char::createExpressionValue(LangAPI::Char {.value = 'A', .escaped = true}));

        expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::And});

        // current <= 'Z'
        expr.push_back(LangAPI::Pos::createExpressionValue(LangAPI::Pos {.dereference = true, .offset = str.count_strlen()}));
        expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::LowerOrEqual});
        expr.push_back(LangAPI::Char::createExpressionValue(LangAPI::Char {.value = 'Z', .escaped = true}));

        expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::Or});

        // current >= '0'
        expr.push_back(LangAPI::Pos::createExpressionValue(LangAPI::Pos {.dereference = true, .offset = str.count_strlen()}));
        expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::HigherOrEqual});
        expr.push_back(LangAPI::Char::createExpressionValue(LangAPI::Char {.value = '0', .escaped = true}));

        expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::And});

        // current <= '9'
        expr.push_back(LangAPI::Pos::createExpressionValue(LangAPI::Pos {.dereference = true, .offset = str.count_strlen()}));
        expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::LowerOrEqual});
        expr.push_back(LangAPI::Char::createExpressionValue(LangAPI::Char {.value = '9', .escaped = true}));

        expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::Or});

        // current == '_'
        expr.push_back(LangAPI::Pos::createExpressionValue(LangAPI::Pos {.dereference = true, .offset = str.count_strlen()}));
        expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::Equal});
        expr.push_back(LangAPI::Char::createExpressionValue(LangAPI::Char {.value = '_', .escaped = true}));

        expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::GroupClose});
    }
    LangAPI::Statements block = createDefaultStatements(var, svar);
    statements.push_back(LangAPI::Variable::createStatement(var));
    statements.push_back(LangAPI::Variable::createStatement(svar));
    pushBasedOnQualifier(rule, expr, block, uvar, var, svar, rule.quantifier, false);
    pushConvResult(rule, var, uvar, svar, {}, rule.quantifier);
}
// void LLIR::HexBuilder::build() {
//     //cpuf::printf("hex\n");
//     auto data = rule.getHex().hex_chars;
//     LangAPI::Expression expr = {};
//     auto uvar = !rule.prefix.name.empty() ? createEmptyVariable(rule.prefix.name) : createEmptyVariable("");
//     auto var = createEmptyVariable(generateVariableName());
//     auto svar = createSuccessVariable();
//     var.type = {LangAPI::ValueType::String};
//     LangAPI::Statements block = createDefaultStatements(var, svar);
//     bool is_first = true, is_negative = false;
//     if (rule.quantifier == '\0') {
//         expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::Not});
//         expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::GroupOpen});
//         is_negative = true;
//     }
//     if (data.size() % 2 != 0)
//         data.insert(data.begin(), '0');
//     for (std::size_t i = 0; i < data.size(); i += 2) {
//         std::string hex(data.data() + i, 2);
//         if (!is_first)
//             expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::And});
//         is_first = false;
//         expr.push_back(LangAPI::Pos::createExpressionValue(LangAPI::Pos {.dereference = true, .offset = i}));
//         expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::Equal});
//         expr.push_back(Hex::createExpressionValue(Hex {.hex = hex}));
//     }
//     if (is_negative) {
//         expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::GroupClose});
//     }
//     //cpuf::printf("hex_open\n");
//     statements.push_back(LangAPI::Variable::createStatement(var));
//     statements.push_back(LangAPI::Variable::createStatement(svar));
//     auto shadow_var = pushBasedOnQualifier(rule, expr, block, uvar, var, svar, rule.quantifier, false);
//     pushConvResult(rule, var, uvar, svar, shadow_var, rule.quantifier);
// }
// void LLIR::BinBuilder::build() {
//     //cpuf::printf("hex\n");
//     auto data = rule.getBin().bin_chars;
//     LangAPI::Expression expr = {};
//     auto uvar = !rule.prefix.name.empty() ? createEmptyVariable(rule.prefix.name) : createEmptyVariable("");
//     auto var = createEmptyVariable(generateVariableName());
//     auto svar = createSuccessVariable();
//     LangAPI::Statements block = {
//         {LLIR::types::ASSIGN_VARIABLE, LLIR::variable_assign {var.name, LLIR::var_assign_types::ADD, LLIR::var_assign_values::CURRENT_POS_SEQUENCE}},
//         {LLIR::types::INCREASE_POS_COUNTER},
//     };
//     bool is_first = true, is_negative = false;
//     if (rule.quantifier == '\0') {
//         expr.push_back({LLIR::condition_types::NOT});
//         expr.push_back({LLIR::condition_types::GROUP_OPEN});
//         is_negative = true;
//     }
//     while (data.size() % 8 != 0)
//         data.insert(data.begin(), '0');
//     for (std::size_t i = 0; i < data.size(); i += 8) {
//         std::string bin(data.data() + i, 8);
//         auto as_hex = hex::from_binary(bin);
//         as_hex.erase(as_hex.begin(), as_hex.begin() + 2);
//         if (!is_first)
//             expr.push_back({LLIR::condition_types::AND});
//         is_first = false;
//         expr.push_back({LLIR::condition_types::CURRENT_CHARACTER, i});
//         expr.push_back({LLIR::condition_types::EQUAL});
//         expr.push_back({LLIR::condition_types::HEX, as_hex});
//     }
//     if (is_negative) {
//         expr.push_back({LLIR::condition_types::GROUP_CLOSE});
//     }
//     push({LLIR::types::VARIABLE, var});
//     push({LLIR::types::VARIABLE, svar});
//     auto shadow_var = pushBasedOnQualifier(rule, expr, block, uvar, var, svar, rule.quantifier, false);
//     pushConvResult(rule, var, uvar, svar, shadow_var, rule.quantifier);
// }
void LLIR::NameBuilder::build() {
    // cpuf::printf("Rule_other");
    auto name = rule.getName().name;
    //cpuf::printf(", name: %s\n", name_str);
    // A rule inside a repetition needs a value variable even when anonymous:
    // its extracted node is the element appended to the shadow array.
    auto uvar = createEmptyVariable(
        !rule.prefix.name.empty() ? rule.prefix.name :
        (insideLoop || rule.quantifier == '*' || rule.quantifier == '+')
            ? "uvar" + generateVariableName() : ""
    );
    auto var = createEmptyVariable(corelib::text::join(name, "_") + generateVariableName());
    auto svar = createSuccessVariable();
    LangAPI::Variable shadow_var;
    bool isCallingToken = corelib::text::isUpper(name.back());
    // if (*has_symbol_follow) {
    //     symbol_follow->back().first = name;
    // }
    LangAPI::Symbol type_name {name};
    if (isCallingToken) {
        var.type = { LangAPI::ValueType::Token, LangAPI::Type {type_name} };
        uvar.type = var.type;
    } else {
        var.type =  { isCallingToken ? LangAPI::ValueType::TokenResult : LangAPI::ValueType::RuleResult, LangAPI::Type { type_name } };
        uvar.type = { isCallingToken ? LangAPI::ValueType::Token : LangAPI::ValueType::Rule, LangAPI::Type { type_name } };
    }
    shadow_var.type.type = LangAPI::ValueType::Array;
    shadow_var.type.template_parameters = {uvar.type};
    LangAPI::Statements statements;
    statements.push_back(LangAPI::Variable::createStatement(var));
    statements.push_back(LangAPI::Variable::createStatement(svar));
    if (isCallingToken) {
        LangAPI::Symbol token_name{name};
        token_name.path.insert(token_name.path.begin(), "Types");
        LangAPI::Expression expr = {
            LangAPI::CheckVariant{
                .type = std::make_shared<LangAPI::Type>(
                    LangAPI::Type{LangAPI::ValueType::Token,
                                  LangAPI::Type{LangAPI::Symbol{token_name}}}),
                .sym = LangAPI::Pos::createExpression(
                    LangAPI::Pos{.dereference = true})
            }
        };

        // Variables must be declared once, outside the match condition.
        this->statements.insert(this->statements.end(),
                                statements.begin(), statements.end());
        if (!uvar.name.empty()) {
            this->statements.push_back(
                LangAPI::Variable::createStatement(uvar));
        }

        // Match body: capture current token, mark success and consume ONCE.
        // Never advance the input before evaluating CheckVariant.
        LangAPI::Statements body = createDefaultStatements(var, svar, name);

        LangAPI::Statements append_to_shadow;
        if (insideLoop || rule.quantifier == '*' || rule.quantifier == '+') {
            // The vector is declared outside the conditional/loop.  Only
            // successful matches append their captured token.
            shadow_var = add_shadow_variable(this->statements,
                                             append_to_shadow, var);
        }
        body.insert(body.end(), append_to_shadow.begin(),
                    append_to_shadow.end());
        BuilderBase::createAssignUvarBlock(body, uvar, var, {});
        body.insert(body.end(),
                    append_to_shadow.begin(),
                    append_to_shadow.end());
        switch (rule.quantifier) {
            case '?':
                this->statements.push_back(LangAPI::If::createStatement(
                    LangAPI::If{expr, std::move(body)}));
                break;
            case '*':
                this->statements.push_back(LangAPI::While::createStatement(
                    LangAPI::While{expr, std::move(body)}));
                break;
            case '+':
                handle_plus_qualifier(rule,
                    LangAPI::ConditionalElement{.expr = expr,
                                                .stmt = std::move(body)},
                    uvar, var, shadow_var);
                break;
            default: {
                expr.insert(expr.begin(), LangAPI::ExpressionValue{
                    LangAPI::ExpressionElement::Not});
                expr.insert(expr.begin() + 1, LangAPI::ExpressionValue{
                    LangAPI::ExpressionElement::GroupOpen});
                expr.push_back(LangAPI::ExpressionValue{
                    LangAPI::ExpressionElement::GroupClose});
                this->statements.push_back(LangAPI::If::createStatement(
                    LangAPI::If{std::move(expr), LangAPI::Return::createStatements(LangAPI::Return{})}));
                this->statements.insert(this->statements.end(),
                                        body.begin(), body.end());
                break;
            }
        }
    } else {
        LangAPI::Expression expr;
        // Keep declarations outside the quantifier/conditional.
        // createDefaultCall only constructs the invocation and status test.
        auto call = createDefaultCall(statements, var,
                                      corelib::text::join(name, "_"), expr);
        shadow_var = pushBasedOnQualifier(rule, expr, statements, uvar, var,
                                          svar, call, rule.quantifier, name);
    }
    pushConvResult(rule, var, uvar, svar, shadow_var, rule.quantifier);
}
void LLIR::NospaceBuilder::build() {
    addSpaceSkip = false;
    removePrevSpaceSkip();
}
// void LLIR::EscapedBuilder::build() {
//     //cpuf::printf("Rule_escaped\n");
//     const auto &escaped_c = rule.getEscaped();
//
//     LangAPI::Expression expression;
//     switch (escaped_c.c) {
//         case 's':
//             // do not add skip of spaces
//             addSpaceSkip = false;
//             removePrevSpaceSkip();
//             //cpuf::printf("ON_EXPRESSION\n")
//             expression = {
//                 LangAPI::Pos::createExpressionValue(LangAPI::Pos {.dereference = true}),
//                 LangAPI::ExpressionValue { LangAPI::ExpressionElement::NotEqual},
//                 LangAPI::Char::createExpressionValue(LangAPI::Char {.value = ' '})
//             };
//             expression = {
//                 {LLIR::condition_types::CURRENT_CHARACTER, (std::size_t) 0},
//                 {LLIR::condition_types::NOT_EQUAL},
//                 {LLIR::condition_types::CHARACTER, ' '}
//             };
//             break;
//         default:
//             throw Error("Undefined char");
//
//     }
//     //cpuf::printf("escaped_open\n");
//     auto uvar = !rule.prefix.name.empty() ? createEmptyVariable(rule.prefix.name) : createEmptyVariable("");
//     auto var = createEmptyVariable(generateVariableName());
//     auto svar = createSuccessVariable();
//     var.type = {LLIR::var_types::CHAR};
//     LangAPI::Statements block = {{LLIR::types::EXIT}};
//     if (!isFirst) {
//         block.insert(block.begin(), {LLIR::types::ERR, getErrorName(rule)});
//     }
//     auto block_after = createDefaultStatements(var, svar);
//     push({LLIR::types::VARIABLE, var});
//     push({LLIR::types::VARIABLE, svar});
//     auto shadow_var = pushBasedOnQualifier(rule, expression, block, uvar, var, svar, rule.quantifier, true);
//     push({LLIR::types::IF, LLIR::condition{expression, block}});
//     add(block_after);
//     //cpuf::printf("escaped_close\n");
//     pushConvResult(rule, var, uvar, svar, shadow_var, rule.quantifier);
// }
void LLIR::AnyBuilder::build() {
    //cpuf::printf("Rule_any\n");
    if (!isToken)
        throw Error("AnyBuilder invoked on non-terminal: {}", fullname);
    auto var = rule.prefix.name.empty() ? createEmptyVariable(generateVariableName()) : createEmptyVariable(rule.prefix.name);
    auto svar = createSuccessVariable();
    var.type = {LangAPI::ValueType::Char};
    LangAPI::Statements stmt = LangAPI::Return::createStatements(LangAPI::Return {});;
    if (!isFirst) {
        auto rm = AST::RuleMember {.value = AST::RuleMemberAny() };
        // stmt.insert(stmt.begin(), LangAPI::ReportError::createStatement(LangAPI::ReportError {.message = getErrorName(rm)}));;
    }
    LangAPI::Statements stmt_after = createDefaultStatements(var, svar);
    LangAPI::Expression expression = {
        LangAPI::Pos::createExpressionValue(LangAPI::Pos {.dereference = true}),
        LangAPI::ExpressionValue { LangAPI::ExpressionElement::Equal },
        isToken ? LangAPI::Char::createExpressionValue(LangAPI::Char {.value = '0', .escaped = true}) : LangAPI::Symbol::createExpressionValue(LangAPI::Symbol {{"Tokens", "NONE"}}),
    };
    statements.push_back(LangAPI::Variable::createStatement(var));
    statements.push_back(LangAPI::Variable::createStatement(svar));
    statements.push_back(LangAPI::If::createStatement(LangAPI::If {expression, stmt}));;
    statements.insert(statements.end(), stmt_after.begin(), stmt_after.end());
    pushConvResult(rule, var, {}, svar, {}, rule.quantifier);
}
/*
 * build PEG style parser. Right now DFA based is preffered
 * But may come back later
 */

// auto LLIR::OpBuilder::createBlock(const stdu::vector<AST::RuleMember> &rules, std::size_t index, LLIR::variable &var, LLIR::variable &svar) -> LangAPI::Statements {
//     //[[assume(rules.size() >= 2)]];
//     if (index >= rules.size()) {
//         return {{LLIR::types::EXIT}};
//     }
//
//     LLIR::ConvertionResult success_var;
//     stdu::vector<LangAPI::Statements> blocks;
//     stdu::vector<LangAPI::Expression> conditions;
//     auto rule = rules[index++];
//     std::unique_ptr<LLIR::MemberBuilder> builder = nullptr;
//     if (rule.isGroup()) {
//         char new_qualifier;
//         if (rule.quantifier == '+')
//             new_qualifier = '*';
//         else if (rule.quantifier == '\0')
//             new_qualifier = '?';
//         auto prev_quantifier = rule.quantifier;
//         rule.quantifier = new_qualifier;
//         builder = std::make_unique<LLIR::MemberBuilder>(*this, rule);
//         builder->build();
//         rule.quantifier = prev_quantifier;
//     }
//     else {
//         builder = std::make_unique<LLIR::MemberBuilder>(*this, rule);
//         builder->build();
//     }
//     builder->getData();
//     stdu::vector<int> erase_indices;
//     stdu::vector<int> push_indices;
//     if (rule.isGroup()) {
//         builder->getData().back().type = LLIR::types::RESET_POS_COUNTER; // remove space skip
//         auto cond = LLIR::condition {
//             LangAPI::Expression {
//                 {LLIR::condition_types::NOT}, {LLIR::condition_types::VARIABLE, LLIR::var_refer {.var = success_var.svar}}
//             },
//             createBlock(rules, index, var, svar),
//         };
//         auto v = !success_var.shadow_var.name.empty() && var.type.type != LLIR::var_types::STRING ? success_var.shadow_var : success_var.var;
//         auto assign_type = v.type.type == LLIR::var_types::STRING ? LLIR::var_assign_types::ADD : LLIR::var_assign_types::ASSIGN;
//         if (!v.name.empty() && v.type.type != LLIR::var_types::UNDEFINED) {
//             cond.else_block = {{
//                 LLIR::types::ASSIGN_VARIABLE,
//                 LLIR::variable_assign
//                 {
//                     var.name,
//                     LLIR::var_assign_types::ASSIGN,
//                     LLIR::assign {
//                         LLIR::var_assign_values::VAR_REFER,
//                         LLIR::var_refer {.var = v }
//                     }
//                 }
//             }};
//         }
//         push({LLIR::types::IF, cond});
//     } else {
//         for (int i = 0; i < builder->getData().size(); i++) {
//             auto &el = builder->getData()[i];
//             if (el.type == LLIR::types::IF) {
//                 auto val = std::any_cast<LLIR::condition>(el.value);
//                 // get recursively nested block
//                 val.block = createBlock(rules, index, var, svar);
//                 // change condition and remove it's content into else blocks
//                 for (int j = i + 1; j < builder->getData().size(); j++) {
//                     auto el = builder->getData()[j];
//                     erase_indices.push_back(j);
//                     if (el.type != LLIR::types::SKIP_SPACES) {
//                         if (el.type == LLIR::types::ASSIGN_VARIABLE) {
//                             auto assignment = std::any_cast<LLIR::variable_assign>(el.value);
//                             assignment.assign_type = LLIR::var_assign_types::ASSIGN;
//                             el.value = assignment;
//                         }
//                         val.else_block.push_back(el);
//                     }
//                 }
//                 // push into else block an assignment to variable
//                 if (var.type.type == LLIR::var_types::ARRAY) {
//                     val.else_block.push_back({LLIR::types::METHOD_CALL, LLIR::method_call { var.name, {LLIR::function_call {"push", {stdu::vector<LangAPI::Expression> {{LLIR::expr {LLIR::condition_types::VARIABLE, LLIR::var_refer {.var = success_var.var}}}}}}}}});
//                 } else {
//                     auto v = !success_var.shadow_var.name.empty() && var.type.type != LLIR::var_types::STRING ? success_var.shadow_var : success_var.var;
//                     auto assign_type = v.type.type == LLIR::var_types::STRING ? LLIR::var_assign_types::ADD : LLIR::var_assign_types::ASSIGN;
//                     val.else_block.push_back({
//                         LLIR::types::ASSIGN_VARIABLE,
//                         LLIR::variable_assign
//                         {
//                             var.name,
//                             LLIR::var_assign_types::ASSIGN,
//                             LLIR::assign {
//                                 LLIR::var_assign_values::VAR_REFER,
//                                 LLIR::var_refer {.var = v}
//                             }
//                         }
//                     });
//                 }
//
//                 // update the value
//                 el.value = val;
//             }
//         }
//     }
//
//     for (auto it = erase_indices.rbegin(); it != erase_indices.rend(); ++it) {
//         builder->getData().erase(builder->getData().begin() + *it);
//     }
//     return builder->getData();
// }
//
// void LLIR::OpBuilder::build() {
//     // cpuf::printf("Rule_op\n");
//     auto &rules = rule.getOp();
//     auto uvar = !rule.prefix.name.empty() ? createEmptyVariable(rule.prefix.name) : createEmptyVariable("");
//     auto var = createEmptyVariable(generateVariableName());
//     auto svar = createSuccessVariable();
//     auto block = createDefaultBlock(var, svar);
//     // cpuf::printf("op prefix: %$\n", rule.prefix);
//     // Add success variable
//     var.type = {deduceVarTypeByProd(rule)};
//     if (insideLoop && var.type.type != LLIR::var_types::STRING) {
//         var.type.templ = {{var.type.type}};
//         var.type.type = LLIR::var_types::ARRAY;
//     }
//     if (var.type.type != LLIR::var_types::UNDEFINED) {
//         push({LLIR::types::VARIABLE, var});
//     }
//     push({LLIR::types::VARIABLE, svar});
//     if (!uvar.name.empty()) {
//         uvar.type = var.type;
//         push({LLIR::types::VARIABLE, uvar});
//     }
//     const auto res = createBlock(rules.options, 0, var, svar);
//     add(res);
//     block.erase(block.begin()); // remove variable assignment (it is done in else blocks)
//     block.erase(block.end() - 1);
//     // Append default block
//     add(block);
//     const auto shadow_var = createEmptyVariable("");
//     if (!uvar.name.empty())
//         push(createAssignUvarBlock(uvar, var, shadow_var));
//     pushConvResult(rule, var, uvar, svar, shadow_var, rule.quantifier);
// }

// Alternative lookahead sets are computed and cached by AST::Tree.
bool LLIR::OpBuilder::checkLLkConflicts(
    const std::vector<std::set<LookaheadSeq>>& perAlt,
    std::vector<std::tuple<std::size_t, std::size_t, LookaheadSeq>>* conflicts
) {
    const auto overlaps = [](const LookaheadSeq& a, const LookaheadSeq& b) {
        return std::equal(a.begin(), a.begin() + std::min(a.size(), b.size()), b.begin());
    };

    bool ok = true;
    for (std::size_t i = 0; i < perAlt.size(); ++i) {
        for (std::size_t j = i + 1; j < perAlt.size(); ++j) {
            for (const auto& a : perAlt[i]) {
                for (const auto& b : perAlt[j]) {
                    if (!overlaps(a, b)) continue;
                    ok = false;
                    if (conflicts)
                        conflicts->emplace_back(i, j, a.size() <= b.size() ? a : b);
                }
            }
        }
    }
    return ok;
}

LangAPI::Switch LLIR::OpBuilder::buildDecisionTree(
    const std::vector<std::set<LookaheadSeq>>& perAlt,
    const std::vector<std::shared_ptr<AST::RuleMember>>& op,
    const std::vector<std::size_t>& candidates,
    std::size_t depth,
    std::size_t k,
    const LangAPI::Variable& result_var,
    const LangAPI::Variable& success_var
)
{
    LangAPI::Switch ss {
        .expression = LangAPI::Cast::createExpression(
            LangAPI::Cast {
                .type = std::make_shared<LangAPI::Type>(LangAPI::Type {
                    LangAPI::Symbol {"Tokens"}
                }),
                .what = LangAPI::StorageSymbol::createExpression(LangAPI::StorageSymbol {
                    LangAPI::Pos::createExpression(LangAPI::Pos {.dereference = true}),
                    stdu::vector<LangAPI::StorageSymbol::PathPart> {LangAPI::IspaLibSymbol {.exports = LangAPI::StdlibExports::VariantIndex}}
                })
            }
        )
    };

    std::map<
        stdu::vector<std::string>,
        std::set<std::size_t>
    > buckets;

    for (const auto index : candidates) {
        for (const auto& seq : perAlt[index]) {
            if (depth < seq.size()) {
                buckets[seq[depth]].insert(index);
            }
        }
    }

    for (const auto& [symbol, alternatives] : buckets) {
        ss.cases.emplace_back();
        auto& cs = ss.cases.back();

        auto rv_sym = LangAPI::Symbol{symbol};
        rv_sym.path.insert(
            rv_sym.path.begin(), "Tokens"
        );

        cs.first = LangAPI::Cast::createRValue(rv_sym);

        if (alternatives.size() == 1) {
            const auto index = *alternatives.begin();

            MemberBuilder builder(*this, *op[index]);
            builder.build();

            cs.second = builder.getData();

            const auto& returns = builder.getReturnVars();

            if (returns.empty()) {
                throw Error(
                    "OpBuilder: alternative {} has no return value",
                    index
                );
            }

            const auto& selected = returns.front();

            if (selected.var.name.empty()) {
                throw Error(
                    "OpBuilder: alternative {} has an unnamed result",
                    index
                );
            }

            // A selected rule must have succeeded before its node is
            // assigned to the outer result variant.
            if (!selected.svar.name.empty()) {
                LangAPI::Expression failed = {
                    LangAPI::ExpressionValue{
                        LangAPI::ExpressionElement::Not
                    },
                    LangAPI::Symbol::createExpressionValue(
                        LangAPI::Symbol{selected.svar.name}
                    )
                };

                cs.second.push_back(
                    LangAPI::If::createStatement(
                        LangAPI::If{
                            std::move(failed),
                            LangAPI::Return::createStatements(
                                LangAPI::Return{}
                            )
                        }
                    )
                );
            }

            LangAPI::Expression selected_value;

            const auto type = selected.var.type.getValueType();

            if (type == LangAPI::ValueType::RuleResult ||
                type == LangAPI::ValueType::TokenResult) {

                // MatchResult<...> -> Node<...>
                selected_value =
                    LangAPI::StorageSymbol::createExpression(
                        LangAPI::StorageSymbol{
                            LangAPI::Symbol::createExpression(
                                LangAPI::Symbol{selected.var.name}
                            ),
                            stdu::vector<
                                LangAPI::StorageSymbol::PathPart
                            >{
                                LangAPI::IspaLibSymbol{
                                    .exports =
                                        LangAPI::StdlibExports::MatchResultValue
                                }
                            }
                        }
                    );

            } else {
                // Already a Node or another directly assignable value.
                selected_value =
                    LangAPI::Symbol::createExpression(
                        LangAPI::Symbol{selected.var.name}
                    );
            }

            // Store the selected alternative in the outer variant.
            cs.second.push_back(
                LangAPI::VariableAssignment::createStatement(
                    LangAPI::VariableAssignment{
                        .name = LangAPI::Symbol{result_var.name},
                        .value = std::move(selected_value)
                    }
                )
            );

            // Mark the alternative as successful.
            cs.second.push_back(
                LangAPI::VariableAssignment::createStatement(
                    LangAPI::VariableAssignment{
                        .name = LangAPI::Symbol{success_var.name},
                        .value = LangAPI::Bool::createExpression(
                            LangAPI::Bool{.value = true}
                        )
                    }
                )
            );
        } else {
            if (depth + 1 >= k) {
                throw Error(
                    "Unresolved LL(k) decision at depth {}",
                    depth
                );
            }

            std::vector<std::size_t> narrowed(
                alternatives.begin(),
                alternatives.end()
            );

            cs.second.push_back(
                LangAPI::Switch::createStatement(
                    buildDecisionTree(
                        perAlt,
                        op,
                        narrowed,
                        depth + 1,
                        k,
                        result_var,
                        success_var
                    )
                )
            );
        }
    }

    return ss;
}

auto LLIR::OpBuilder::resolveLLk(
    const std::vector<std::shared_ptr<AST::RuleMember>>& op,
    const stdu::vector<std::string>& rule_name,
    AST::Tree& tree,
    std::size_t max_k
) -> LLkResolution {
    LLkResolution result;

    for (std::size_t k = 1; k <= max_k; ++k) {
        std::vector<std::set<LookaheadSeq>> perAlt;
        perAlt.reserve(op.size());
        bool valid = !op.empty();

        for (const auto& alt : op) {
            const auto lookahead = tree.getAlternativeLookahead(*alt, rule_name, k);
            std::set<LookaheadSeq> converted(lookahead.begin(), lookahead.end());
            if (converted.empty()) valid = false;
            for (const auto& seq : converted)
                if (seq.empty()) valid = false;
            perAlt.push_back(std::move(converted));
        }

        result.k = k;
        result.perAlt = std::move(perAlt);
        if (valid && checkLLkConflicts(result.perAlt)) {
            result.resolved = true;
            return result;
        }
    }

    result.resolved = false;
    return result;
}


void LLIR::OpBuilder::build() {
    const auto &op = rule.getOp().options;
    auto var = createEmptyVariable("");
    var.type = deduceVarTypeByRuleMember(rule);
    undoRuleResult(var.type.getValueType());
    var.name = generateVariableName();

    bool uvar_is_named = !rule.prefix.name.empty();
    LangAPI::Variable uvar;
    if (uvar_is_named && !rule.prefix.is_key_value) {
        var.name = rule.prefix.name;
        uvar = var;
    } else if (uvar_is_named) {
        uvar = createEmptyVariable(rule.prefix.name);
        uvar.type = var.type;
    } else {
        uvar = createEmptyVariable("");
        uvar.type = var.type;
    }

    auto svar = createSuccessVariable();
    svar.value = LangAPI::Bool::createExpression(LangAPI::Bool {.value = false});
    statements.push_back(LangAPI::Variable::createStatement(var));
    statements.push_back(LangAPI::Variable::createStatement(svar));
    if (corelib::text::isLower(fullname.back())) {
        auto resolution = resolveLLk(op, fullname, tree);

        if (!resolution.resolved) {
            throw Error("LL(k) conflict in rule {}: no unique lookahead for k = {}", fullname, resolution.k);
        }

        statements.push_back(LangAPI::SkipSpaces::createStatement(LangAPI::SkipSpaces{.isToken = isToken}));
        std::vector<std::size_t> candidates(op.size());
        std::iota(candidates.begin(), candidates.end(), 0);

        statements.push_back(
            LangAPI::Switch::createStatement(
                buildDecisionTree(
                    resolution.perAlt,
                    op,
                    candidates,
                    0,
                    resolution.k,
                    var,
                    svar
                )
            )
        );

        // FIX: if no case in the switch matched, svar is still false — the
        // alternation failed to find any viable lookahead at runtime (e.g.
        // an unexpected token). Without this check the function falls
        // through and unconditionally reports success with var/uvar left
        // at their default/unset values, exactly matching the symptom of
        // an empty variant in the printed tree.
        LangAPI::Expression not_matched = {
            LangAPI::ExpressionValue{LangAPI::ExpressionElement::Not},
            LangAPI::Symbol::createExpressionValue(LangAPI::Symbol{svar.name})
        };
        statements.push_back(LangAPI::If::createStatement(LangAPI::If{
            std::move(not_matched),
            LangAPI::Return::createStatements(LangAPI::Return{})
        }));
    }

    if (uvar_is_named && rule.prefix.is_key_value) {
        statements.push_back(LangAPI::Variable::createStatement(uvar));
        statements.push_back(LangAPI::VariableAssignment::createStatement(LangAPI::VariableAssignment {
            .name = LangAPI::Symbol {uvar.name},
            .value = LangAPI::Symbol::createExpression(LangAPI::Symbol {var.name})
        }));
    }

    pushConvResult(rule, var, uvar, svar, {}, rule.quantifier);
}