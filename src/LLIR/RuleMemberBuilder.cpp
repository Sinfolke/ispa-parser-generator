module LLIR.Rule.MemberBuilder;
import LLIR.CllBuilder;
import logging;
import corelib;
import cpuf.hex;
import cpuf.op;
import cpuf.printf;
import Dump;
import NFA_OLD;
import DFA.functionality;
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
    bool add_shadow_var
) -> LangAPI::Variable {
       //block.push_back({IR::types::ASSIGN_VARIABLE, IR::variable_assign {svar.name, IR::var_assign_types::ASSIGN, IR::var_assign_values::_TRUE}});
    LangAPI::Variable shadow_variable;
    LangAPI::Statements shadow_var_assign_block;
    if (insideLoop || quantifier == '+' || quantifier == '*') {
        shadow_variable = add_shadow_variable(stmt, shadow_var_assign_block, var);
        add_shadow_var = false;
    }
    if (!uvar.name.empty()) {
        uvar.type = deduceUvarType(var, shadow_variable);
        statements.push_back(LangAPI::Variable::createStatement(uvar));
    }
    switch (quantifier) {
        case '+':
            stmt.push_back(call);
            createAssignUvarBlock(stmt, uvar, var, shadow_variable);
            stmt.insert(stmt.end(), shadow_var_assign_block.begin(), shadow_var_assign_block.end());
            handle_plus_qualifier(rule, LangAPI::ConditionalElement {.expr = expr, .stmt = stmt}, uvar, var, shadow_variable);
            break;
        case '*': {
            stmt.push_back(call);
            createAssignUvarBlock(stmt, uvar, var, shadow_variable);
            stmt.insert(stmt.end(), shadow_var_assign_block.begin(), shadow_var_assign_block.end());
            statements.push_back(LangAPI::While::createStatement(LangAPI::While {expr, stmt}));
            break;
        }
        case '?':
            createAssignUvarBlock(stmt, uvar, var, shadow_variable);
            stmt.insert(stmt.end(), shadow_var_assign_block.begin(), shadow_var_assign_block.end());
            statements.push_back(LangAPI::If::createStatement(LangAPI::If {expr, stmt}));
            break;
        default:
        {
            // add the negative into condition
            createAssignUvarBlock(stmt, uvar, var, shadow_variable);
            expr.insert(expr.begin(), LangAPI::ExpressionValue { LangAPI::ExpressionElement::Not });
            expr.insert(expr.begin() + 1, LangAPI::ExpressionValue { LangAPI::ExpressionElement::GroupOpen });
            expr.push_back(LangAPI::ExpressionValue { LangAPI::ExpressionElement::GroupClose });
            // add exit statement
            LangAPI::Statements blk;
            // if (/* *has_symbol_follow */false) {
            //     ErrorIR::IR error(tree, rule, *symbol_follow, dfas, isFirst);
            //     blk = {error.lowerToLLIR(*variable_count)};
            // } else {
            blk = LangAPI::Return::createStatements(LangAPI::Return {});
                // if (!isFirst) {
                //     blk.insert(blk.begin(), {LLIR::types::ERR, getErrorName(rule)});
                // }
            // }

            // if (!isFirst) {
            //     blk.insert(blk.begin(), {LLIR::types::ERR, getErrorName(rule)});
            // }
            statements.insert(statements.end(), stmt.begin(), stmt.end());
            statements.push_back(LangAPI::If::createStatement(LangAPI::If {expr, blk}));
            auto ending_block = createDefaultStatements(var, svar);
            statements.insert(statements.end(), ending_block.begin(), ending_block.end());
            statements.insert(statements.end(), shadow_var_assign_block.begin(), shadow_var_assign_block.end());
            break;
        }
    }
    return shadow_variable;
}
void LLIR::MemberBuilder::buildMember(const AST::RuleMember &member) {
    addSpaceSkip = true;
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
    if (addSpaceSkip)
        statements.push_back(LangAPI::SkipSpaces::createStatement(LangAPI::SkipSpaces {.isToken = isToken}));
}
auto LLIR::MemberBuilder::build() -> void {
    bool isFirst = true;
    std::size_t pos = 0;
    isToken = corelib::text::isUpper(fullname.back());
    if (rules == nullptr) {
        buildMember(*rule);
        if (isFirst) {
            addSpaceSkipFirst = addSpaceSkip;
        }
        isFirst = false;
        pos++;
        return;
    }
    for (const auto &mem_ptr : *rules) {
        // if (mem.isName() && *has_symbol_follow) {
        //     *symbol_follow = getLookaheadTerminals(mem, *fullname);
        //     symbol_follow->emplace_back(stdu::vector<std::string>{}, getNextTerminal(rules, pos));
        // }
        buildMember(*mem_ptr);
        if (isFirst) {
            addSpaceSkipFirst = addSpaceSkip;
        }
        isFirst = false;
        pos++;
    }
}
void LLIR::GroupBuilder::build() {
    auto var = createEmptyVariable("group" + generateVariableName());
    auto uvar = !rule.prefix.name.empty() ? createEmptyVariable(rule.prefix.name) : createEmptyVariable("");
    auto svar = createSuccessVariable();
    auto prev_insideLoop = insideLoop;
    const auto &quantifier = rule.quantifier;
    const auto &group = rule.getGroup().values;
    if (quantifier == '*' || quantifier == '+')
        insideLoop = true;
    MemberBuilder builder(*this, group);
    builder.build();
    exports_list.insert(exports_list.end(), builder.getReturnVars().begin(), builder.getReturnVars().end());
    insideLoop = prev_insideLoop;
    // remove the previous space skip if there was \s0
    if (builder.getAddSpaceSkipFirst()) {
        removePrevSpaceSkip();
    }

    var.type = deduceVarTypeByRuleMember(rule);
    uvar.type = var.type;
    if ((quantifier == '*' || quantifier == '+') && var.type != LangAPI::ValueType::Undef && var.type != LangAPI::ValueType::String) {
        var.type.template_parameters = {{var.type}};
        var.type.type = LangAPI::ValueType::Array;
    }
    LangAPI::Statements fetch_var_statements;
    switch (var.type == LangAPI::ValueType::Array ? std::get<LangAPI::Type>(var.type.template_parameters[0]).getValueType() : var.type.getValueType()) {
        case LangAPI::ValueType::String:
            // it is a string so add all values
            for (const auto &v : builder.getReturnVars()) {
                if (v.var.name.empty())
                    continue;
                fetch_var_statements.push_back(LangAPI::VariableAssignment::createStatement(LangAPI::VariableAssignment {.name = LangAPI::Symbol {var.name}, .value = LangAPI::Symbol::createExpression(LangAPI::Symbol { v.var.name })}));
            }
            break;
        case LangAPI::ValueType::Token:
        case LangAPI::ValueType::Rule:
        case LangAPI::ValueType::Variant:
            // it is token so perform a single assign
            fetch_var_statements.push_back(LangAPI::VariableAssignment::createStatement(LangAPI::VariableAssignment {.name = LangAPI::Symbol {var.name}, .value = LangAPI::Symbol::createExpression(LangAPI::Symbol {builder.getReturnVars()[0].var.name})}));

            var.type = builder.getReturnVars()[0].var.type;
            if (var.type.isValueType())
                undoRuleResult(var.type.getValueType());
            break;
        default:
            var.type = {LangAPI::ValueType::Undef};
            break;
    }
    std::string pos_counter_name = "begin" + generateVariableName();
    LangAPI::If group_success_condition = {};
    stdu::vector<std::string> used_vars;
    //cpuf::printf("success_vars.size(): %d\n", success_vars.size());
    if (!builder.getReturnVars().empty()) {
        bool first = true;
        for (auto el : builder.getReturnVars()) {
            if (el.quantifier == '*' || el.quantifier == '?' || el.svar.name.empty())
                continue;
            if (!first)
                group_success_condition.expr.push_back(LangAPI::ExpressionValue {LangAPI::ExpressionElement::And});
            used_vars.push_back(el.svar.name);
            group_success_condition.expr.push_back(LangAPI::Symbol::createExpressionValue(LangAPI::Symbol { el.svar.name}));
            first = false;
        }
    }
    // for (int i = 0; i < node_ret.size(); i++) {
    //     if (i != 0)
    //         svar_expr.push_back({LLIR::condition_types::AND});
    //     svar_expr.push_back({LLIR::condition_types::VARIABLE, node_ret[i].svar});
    // }

    if (!builder.getData().empty() && builder.getData().back().isExpression()) {
        const auto &expr = builder.getData().back().getExpression();
        if (expr.size() == 1 && expr.back().isSkipSpaces())
            builder.pop();
    }
    statements.insert(statements.end(), fetch_var_statements.begin(), fetch_var_statements.end());
    LangAPI::Variable shadow_var;
    LangAPI::Statements shadow_var_assign_block;
    if ((insideLoop || quantifier == '*' || quantifier == '+') && (var.type != LangAPI::ValueType::Undef && var.type != LangAPI::ValueType::String)) {
        shadow_var = add_shadow_variable(builder.getData(), shadow_var_assign_block, var);
    }
    group_success_condition.stmt = {
        LangAPI::VariableAssignment::createStatement(LangAPI::VariableAssignment {.name = LangAPI::Symbol {svar.name}, .value = LangAPI::Bool::createExpression(LangAPI::Bool { .value = true })}),
    };
    createAssignUvarBlock(group_success_condition.stmt, uvar, var, shadow_var);
    group_success_condition.stmt.push_back(LangAPI::PopPosCounter::createStatement(LangAPI::PopPosCounter {}));
    if (var.type != LangAPI::ValueType::Undef) {
        statements.push_back(LangAPI::Variable::createStatement(var));
    }
    statements.push_back(LangAPI::Variable::createStatement(svar));
    statements.push_back(LangAPI::PushPosCounter::createStatement(LangAPI::PushPosCounter {.name = pos_counter_name}));
    pushBasedOnQuantifier(builder, rule, shadow_var, uvar, var, quantifier);
    for (const auto &svar : used_vars) {
        raiseVarsTop(statements, statements, svar, true, false, true);
    }
    group_success_condition.stmt.insert(group_success_condition.stmt.end(), shadow_var_assign_block.begin(), shadow_var_assign_block.end());
    if (group_success_condition.expr.empty()) {
        statements.insert(statements.end(), group_success_condition.stmt.begin(), group_success_condition.stmt.end());
    } else {
        statements.push_back(LangAPI::If::createStatement(group_success_condition));
    }
    pushConvResult(rule, var, uvar, svar, shadow_var, rule.quantifier);
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
    auto uvar = !rule.prefix.name.empty() ? createEmptyVariable(rule.prefix.name) : createEmptyVariable("");
    auto var = createEmptyVariable(corelib::text::join(name, "_") + generateVariableName());
    auto svar = createSuccessVariable();
    LangAPI::Variable shadow_var;
    bool isCallingToken = corelib::text::isUpper(name.back());
    // if (*has_symbol_follow) {
    //     symbol_follow->back().first = name;
    // }
    LangAPI::Symbol type_name {name};
    if (!isToken && isCallingToken) {
        var.type = { LangAPI::ValueType::Token, LangAPI::Type {type_name} };
    } else {
        var.type =  { isCallingToken ? LangAPI::ValueType::TokenResult : LangAPI::ValueType::RuleResult, LangAPI::Type { type_name } };
    }
    uvar.type = var.type;
    shadow_var.type.type = LangAPI::ValueType::Array;
    shadow_var.type.template_parameters = {var.type};
    LangAPI::Statements statements;
    std::size_t variable_index_in_statements = statements.size();
    statements.push_back(LangAPI::Variable::createStatement(var));
    statements.push_back(LangAPI::Variable::createStatement(svar));
    if (isCallingToken) {
        LangAPI::Symbol compare_sym = {name};
        compare_sym.path.insert(compare_sym.path.begin(), "Tokens");
        LangAPI::Expression expr = {
            LangAPI::Pos::createExpressionValue(LangAPI::Pos {.dereference = true}),
            LangAPI::ExpressionValue { LangAPI::ExpressionElement::Equal },
            LangAPI::Symbol::createExpressionValue(compare_sym)
        };
        shadow_var = BuilderBase::pushBasedOnQualifier(rule, expr, statements, uvar, var, svar, rule.quantifier, true);
    } else {
        LangAPI::Expression expr;
        auto call = createDefaultCall(statements, var, corelib::text::join(name, "_"), expr);
        statements.push_back(call);
        shadow_var = pushBasedOnQualifier(rule, expr, statements, uvar, var, svar, call, rule.quantifier);
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
        stmt.insert(stmt.begin(), LangAPI::ReportError::createStatement(LangAPI::ReportError {.message = getErrorName(rm)}));;
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
void LLIR::OpBuilder::build() {
    const auto &op = rule.getOp().options;
    auto var = createEmptyVariable("");
    var.type = deduceVarTypeByRuleMember(rule) ;
    undoRuleResult(var.type.getValueType());

    if (rule.prefix.name.empty()) {
        var.name = generateVariableName();
    } else if (!rule.prefix.is_key_value) {
        var.name = rule.prefix.name;
    }
    auto svar = createSuccessVariable();
    svar.value = LangAPI::Bool::createExpression(LangAPI::Bool {.value = true});;
    statements.push_back(LangAPI::Variable::createStatement(var));
    statements.push_back(LangAPI::Variable::createStatement(svar));
    if (corelib::text::isLower(fullname.back())) {
        std::size_t accept_index = 0;
        std::size_t priority_counter = 0;
        NFA::NFA nfa(tree, fullname, nullptr, op, fullname == constants::whitespace, false, &accept_index, &priority_counter);
        auto dfa = DFA::DFA(&nfa);
        if (dfa.get().size() == 2) { // first state plus end state
            // optimize to single switch instead of DFA lookup
            LangAPI::Switch ss { .expression = LangAPI::Pos::createExpression(LangAPI::Pos {.dereference = true}) };
            auto state = dfa.get()[0];
            statements.push_back(LangAPI::SkipSpaces::createStatement(LangAPI::SkipSpaces {.isToken = isToken}));
            for (const auto &t : state.transitions) {
                if (std::holds_alternative<stdu::vector<std::string>>(t.first) && std::get<stdu::vector<std::string>>(t.first) == constants::whitespace)
                    continue;
                auto target_state_opt = std::visit([](const auto &target) -> std::optional<std::size_t> {
                    if constexpr (requires { target.dfa_state_id; }) {
                        return target.dfa_state_id;
                    }
                    return std::nullopt;
                }, t.second);
                if (!target_state_opt.has_value())
                    continue;
                const auto target_state = target_state_opt.value();
                if (std::holds_alternative<char>(t.first)) {
                    auto c = std::get<char>(t.first);
                    if (std::any_of(constants::whitespace_chars.begin(), constants::whitespace_chars.end(), [&](char _c) { return _c == c; }) && target_state == 0) {
                        continue;
                    }
                }
                ss.cases.emplace_back();
                if (std::holds_alternative<stdu::vector<std::string>>(t.first)) {
                    ss.cases.back().first = LangAPI::Symbol::createRValue(LangAPI::Symbol {{"Tokens", corelib::text::join(std::get<stdu::vector<std::string>>(t.first), "_")}});
                } else {
                    ss.cases.back().first = LangAPI::Char::createRValue(LangAPI::Char {.value = std::get<char>(t.first)});
                }
                Assert(target_state < dfa.get().size(), "DFA transition target is out of range");
                const auto &target = dfa.get()[target_state];
                Assert(target.accept_binding.has_value(), "NO_ACCEPT shouldn't be here");
                Assert(target.accept_binding->token_id != NFA::NULL_STATE, "NO_ACCEPT shouldn't be here");
                MemberBuilder builder(*this, *op[target.accept_binding->token_id]);
                builder.build();
                auto &ss_case = ss.cases.back();
                ss_case.second = builder.getData();
                ss_case.second.emplace_back(LangAPI::VariableAssignment::createStatement(LangAPI::VariableAssignment {
                    .name = LangAPI::Symbol {var.name},
                    .value = LangAPI::Symbol::createExpression(LangAPI::Symbol {
                        builder.getReturnVars().back().uvar.name
                    })
                }));
            }
            statements.push_back(LangAPI::Switch::createStatement(ss));
        } else {
            auto dfa_index = dfas->size();
            dfas->push_back(std::move(dfa));
            auto dfa_call_result = createEmptyVariable("dfa_lookup_result" + generateVariableName());
            dfa_call_result.type.type = LangAPI::ValueType::Int;
            LangAPI::Type lookup = LangAPI::ValueType::Variant;
            for (const auto &member_ptr : op) {
                if (member_ptr->isName()) {
                    lookup.template_parameters.push_back(LangAPI::Type {LangAPI::Symbol {member_ptr->getName().name}});
                }
            }
            statements.push_back(LangAPI::Variable::createStatement(dfa_call_result));
            statements.push_back(LangAPI::DfaLookup::createStatement(LangAPI::DfaLookup {.dfa_count = dfa_index, .return_type = lookup, .output_name = dfa_call_result.name}));
            LangAPI::Switch ss {.expression = LangAPI::Symbol::createExpression(LangAPI::Symbol { dfa_call_result.name }) };
            for (int i = 0; i < op.size(); ++i) {
                ss.cases.emplace_back();
                auto &cs = ss.cases.back();
                cs.first = LangAPI::Int::createRValue(LangAPI::Int {.value = i});
                if (op[i]->isName() && op[i]->getName().isTerminal()) {
                    // insert variable assignment
                    //cs.block.push_back(assignSvar(svar, var_assign_values::True));
                    cs.second.emplace_back(LangAPI::VariableAssignment::createStatement(LangAPI::VariableAssignment {.name = LangAPI::Symbol {var.name}, .value = LangAPI::Pos::createExpression(LangAPI::Pos {.dereference = true})}));
                } else if (op[i]->isName() && op[i]->getName().isNonterminal()) {
                    const auto &nonterminal = op[i]->getName().name;
                    cs.second.emplace_back(LangAPI::VariableAssignment::createStatement(
                        LangAPI::VariableAssignment {
                            .name = LangAPI::Symbol {var.name},
                            .value = LangAPI::FunctionCall::createExpression(
                                LangAPI::FunctionCall {
                                    .name = std::make_shared<LangAPI::Symbol>(corelib::text::join(nonterminal, "_")),
                                    .args = {
                                        LangAPI::Pos::createExpression(LangAPI::Pos {.dereference = false})
                                    }
                                }
                            )
                        }
                    ));
                } else {
                    MemberBuilder builder(*this, *op[i]);
                    builder.build();
                    cs.second = std::move(builder.getData());
                }
            }
            statements.push_back(LangAPI::Switch::createStatement(ss));
        }
    }
    pushConvResult(rule,  var, var, svar, {}, rule.quantifier);
}
