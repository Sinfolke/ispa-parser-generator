module;
#include <tsl/ordered_map.h>
module LangRepr.ConstructParser;

import cpuf.printf;
import cpuf.op;
import NFA.TNFA.API;
import Parser.Base;
import DFA;
import UAST.PyAST;

import corelib;

import std;

namespace LangRepr {
    static LangAPI::Statement assignCompatible(
        LangAPI::Expression destination,
        LangAPI::Expression source
    ) {
        return LangAPI::Expression::createStatement(
            LangAPI::IspaLibFunctionCall::createExpression(
                LangAPI::IspaLibFunctionCall{
                    .symbol = LangAPI::IspaLibSymbol{
                        .exports = LangAPI::StdlibExports::AssignCompatible
                    },
                    .args = {
                        std::move(destination),
                        std::move(source)
                    }
                }
            )
        );
    }
    auto ConstructParser::createParserClass(std::string main_node) -> LangAPI::Class {
        LangAPI::IspaLibSymbol parser_s {.exports = LangAPI::StdlibExports::Parser};
        parser_s.template_parameters.push_back(std::make_shared<LangAPI::Type>(LangAPI::Symbol {"Types", main_node}));
        parser_s.template_parameters.push_back(std::make_shared<LangAPI::Type>(LangAPI::Symbol {"Token"}));
        return LangAPI::Class {
            .name = "Parser",
            .inherit_members = {std::make_pair(LangAPI::Visibility::Public, parser_s)},
            .default_visibility = LangAPI::Visibility::Private
        };
    }
    // auto ConstructParser::constructTokenMachineDFA(LangAPI::Class &parser_class) -> void {
    //     std::size_t count = 0;
    //     const auto &dfas = ir.getDfas().get();
    //     auto states = ir.getDfas().getStateSet();
    //     // 1. Construct the State Variables
    //     for (const auto &state : states.state_set) {
    //         const auto [dfa_idx, local_state_index] = states.state_in_dfa_location_map.at(count);
    //         const auto &dfa = dfas.at(dfa_idx);
    //
    //         const int transition_size = state.transitions.size();
    //         auto s = getStateType(DFA::DfaType::Token);
    //         // FIX: Ensure this is exported as a State, not a single Transition
    //         s.exports = LangAPI::StdlibExports::DfaTokenState;
    //         s.template_parameters.push_back(makeIntRValue(transition_size));
    //
    //         stdu::vector<LangAPI::Expression> transitions;
    //         // Construct standard token transitions (No CST metadata, No nested DFAs)
    //         for (const auto &[symbol, transition] : state.transitions) {
    //             transitions.push_back(
    //                 LangAPI::IspaLibDfaTransition::createExpression(LangAPI::IspaLibDfaTransition {
    //                     .symbol = std::get<stdu::vector<std::string>>(symbol),
    //                     .next = transition.next,
    //                     .accept = transition.accept_index,
    //                     .transition_type = { LangAPI::StdlibExports::DfaTokenTransition },
    //                     .is_refferring_char_table = false
    //                 })
    //             );
    //         }
    //         parser_class.data.push_back(
    //             std::make_pair(
    //                 std::make_shared<LangAPI::Declaration>(LangAPI::Variable::createDeclaration(LangAPI::Variable {
    //                     .name = std::string("dfa_state_") + std::to_string(count),
    //                     .type = s,
    //                     .value = LangAPI::Array::createExpression(LangAPI::Array { .values = transitions }),
    //                     .is_static = true
    //                 })),
    //                 LangAPI::Visibility::Private
    //             )
    //         );
    //         ++count;
    //     }
    //
    //     // 2. Construct the DFA Tables
    //     for (std::size_t dfa_count = 0; dfa_count < dfas.size(); ++dfa_count) {
    //         const auto &dfa = dfas.at(dfa_count);
    //         std::vector<LangAPI::Expression> dfa_table_states;
    //
    //         for (std::size_t state_count = 0; state_count < dfa.get().size(); ++state_count) {
    //             const auto &state_id = states.location_in_set.at(std::make_pair(dfa_count, state_count));
    //             const auto &state = states.state_set.get().at(state_id);
    //
    //             LangAPI::IspaLibSymbol span_type = {
    //                 .exports = LangAPI::StdlibExports::DfaSpanTokenTableState,
    //                 .template_parameters = {std::make_shared<LangAPI::Type>(Token)}
    //             };
    //
    //             dfa_table_states.push_back(LangAPI::RValue::createExpression(
    //                 LangAPI::RValue {LangAPI::Inheritance {
    //                     .name = span_type,
    //                     .args = {
    //                         LangAPI::Int::createExpression(LangAPI::Int {.value = static_cast<long long>(state.else_goto)}),
    //                         LangAPI::Int::createExpression(LangAPI::Int {.value = static_cast<long long>(state.else_goto_accept)}),
    //                         // FIX: Match the class name ("Parser") and the variable name ("dfa_state_") from Loop 1
    //                         LangAPI::Symbol::createExpression(LangAPI::Symbol {"Parser", "dfa_state_" + std::to_string(state_id)})
    //                     }
    //                 }}
    //             ));
    //         }
    //
    //         auto table_params = std::vector<std::variant<std::shared_ptr<LangAPI::Type>, std::shared_ptr<LangAPI::RValue>>> {
    //             std::make_shared<LangAPI::RValue>(
    //                 LangAPI::Int::createRValue(LangAPI::Int {.value = (long long) dfa.get().size()})
    //             )
    //         };
    //
    //         LangAPI::Variable dfa_table_var {
    //             .name = "dfa_table_" + std::to_string(dfa_count),
    //             .type = LangAPI::Type {
    //                 LangAPI::IspaLibSymbol {
    //                     LangAPI::StdlibExports::DfaTokenTable,
    //                     table_params
    //                 }
    //             },
    //             .value = LangAPI::Array::createExpression(LangAPI::Array { .values = dfa_table_states }),
    //             .is_static = true
    //         };
    //
    //         // FIX: Use Variable::createDeclaration instead of Statement::createDeclaration
    //         parser_class.data.emplace_back(
    //             std::make_shared<LangAPI::Declaration>(LangAPI::Variable::createDeclaration(dfa_table_var)),
    //             LangAPI::Visibility::Private
    //         );
    //     }
    // }
    auto ConstructParser::finalizeReturnStatement(const LangAPI::Statement &stmt) -> LangAPI::Statement {
        bool pushed = false;
        if (stmt.isExpression()) {
            const auto &expr = stmt.getExpression();
            if (expr.back().isReturn()) {
                auto ret = expr.front().getReturn();
                if (ret.value.empty()) ret.value = LangAPI::EmptyInitializer::createExpression(LangAPI::EmptyInitializer {});
                return LangAPI::Return::createStatement(ret);
            }
            // else if (expr.back().isReportError()) {
            //     return {};
            // }
        } else if (stmt.isIf()) {
            LangAPI::If new_if;
            new_if.expr = stmt.getIf().expr;
            for (const auto &if_stmt : stmt.getIf().stmt) {
                new_if.stmt.push_back(finalizeReturnStatement(if_stmt));
            }
            for (const auto &else_stmt : stmt.getIf().else_stmt) {
                new_if.else_stmt.push_back(finalizeReturnStatement(else_stmt));
            }
            return LangAPI::If::createStatement(new_if);
        } else if (stmt.isSwitch()) {
            LangAPI::Switch sw;
            sw.expression = stmt.getSwitch().expression;
            for (auto case_stmt : stmt.getSwitch().cases) {
                for (auto &stmt : case_stmt.second) {
                    stmt = finalizeReturnStatement(stmt);
                }
                sw.cases.push_back(case_stmt);
            }
            return LangAPI::Switch::createStatement(sw);
        } else if (stmt.isWhile()) {
            LangAPI::While new_do_while;
            new_do_while.expr = stmt.getWhile().expr;
            for (const auto &dw_stmt : stmt.getWhile().stmt) {
                new_do_while.stmt.push_back(finalizeReturnStatement(dw_stmt));
            }
            return LangAPI::While::createStatement(new_do_while);
        } else if (stmt.isDoWhile()) {
            LangAPI::DoWhile new_while;
            new_while.expr = stmt.getDoWhile().expr;
            for (const auto &dw_stmt : stmt.getDoWhile().stmt) {
                new_while.stmt.push_back(finalizeReturnStatement(dw_stmt));
            }
            return LangAPI::DoWhile::createStatement(new_while);
        }
        return ConstructBase::ensureTypesNs(stmt);
    }
    auto ConstructParser::finalizeTypes(const LangAPI::Statement &stmt) -> LangAPI::Statement {
        bool pushed = false;
        if (stmt.isVariable()) {
            auto v = stmt.getVariable();
            if (v.type.isValueType() && (v.type == LangAPI::ValueType::TokenResult || v.type == LangAPI::ValueType::RuleResult)) {
                v.type.template_parameters.push_back(LangAPI::Type {LangAPI::Symbol {"IT"}});
            }
            return v;
        } else if (stmt.isIf()) {
            LangAPI::If new_if;
            new_if.expr = stmt.getIf().expr;
            for (const auto &if_stmt : stmt.getIf().stmt) {
                new_if.stmt.push_back(finalizeTypes(if_stmt));
            }
            for (const auto &else_stmt : stmt.getIf().else_stmt) {
                new_if.else_stmt.push_back(finalizeTypes(else_stmt));
            }
            return LangAPI::If::createStatement(new_if);
        }  else if (stmt.isSwitch()) {
            LangAPI::Switch sw;
            sw.expression = stmt.getSwitch().expression;
            for (auto case_stmt : stmt.getSwitch().cases) {
                for (auto &stmt : case_stmt.second) {
                    stmt = finalizeTypes(stmt);
                }
                sw.cases.push_back(case_stmt);
            }
            return LangAPI::Switch::createStatement(sw);
        } else if (stmt.isWhile()) {
            LangAPI::While new_do_while;
            new_do_while.expr = stmt.getWhile().expr;
            for (const auto &dw_stmt : stmt.getWhile().stmt) {
                new_do_while.stmt.push_back(finalizeTypes(dw_stmt));
            }
            return LangAPI::While::createStatement(new_do_while);
        } else if (stmt.isDoWhile()) {
            LangAPI::DoWhile new_while;
            new_while.expr = stmt.getDoWhile().expr;
            for (const auto &dw_stmt : stmt.getDoWhile().stmt) {
                new_while.stmt.push_back(finalizeTypes(dw_stmt));
            }
            return LangAPI::DoWhile::createStatement(new_while);
        }
        return ConstructBase::ensureTypesNs(stmt);
    }
    auto ConstructParser::constructParser() -> void {
        if (tree.contains({"main"})) for (const auto& feature : tree[{"main"}].features)
            if (const auto* declaration = std::get_if<AST::DeclarationSyntax>(&feature);
                declaration && (!declaration->parameters.empty() || !declaration->templateParameters.empty()))
                throw std::invalid_argument("the parser entry rule main cannot require arguments");
        auto parser = createParserClass();
        const auto appendRuntimeParameters = [](LangAPI::Function& function, const AST::DeclarationSyntax* declaration) {
            if (!declaration) return;
            for (std::size_t i = 0; i < declaration->parameters.size(); ++i) {
                LangAPI::Type type;
                if (i < declaration->parameterAnnotations.size())
                    if (const auto* annotation = std::get_if<UAST::py::Expr>(&declaration->parameterAnnotations[i].code))
                        type = Parser::ParserBase::convertPythonType(std::make_shared<UAST::py::Expr>(*annotation));
                if (type == LangAPI::ValueType::Undef) {
                    const auto type_name = "ISPAParam" + std::to_string(i);
                    function.template_parameters.emplace_back(type_name, stdu::vector<LangAPI::Type>{});
                    type = LangAPI::Type{LangAPI::Symbol{type_name}};
                }
                if (type == LangAPI::ValueType::Void)
                    throw std::invalid_argument("rule parameter cannot have type None: " + declaration->parameters[i]);
                function.parameters.emplace_back(std::move(type), "ispa_action_" + declaration->parameters[i]);
            }
        };
        if (lr) constructLRParser(parser);
        // constructTokenMachineDFA(parser);
        if (!lr) for (const auto &prod : ir) {
            const auto& source_rule = tree[prod.name];
            auto debug_position = source_rule.position;
            if (debug_position.line == 0 && !debug_position.file.empty()) {
                    std::ifstream input(debug_position.file);
                    if (input) {
                        const std::string leaf = prod.name.back();
                        std::string line;
                        std::size_t row = 0;
                        while (std::getline(input, line)) {
                            ++row;
                            const auto at = line.find(leaf);
                            if (at == std::string::npos ||
                                (at > 0 && (std::isalnum(static_cast<unsigned char>(line[at - 1])) || line[at - 1] == '_')))
                                continue;
                            if (std::any_of(line.begin(), line.begin() + static_cast<std::ptrdiff_t>(at),
                                [](unsigned char ch) { return !std::isspace(ch) && ch != '#'; })) continue;
                            auto end = at + leaf.size();
                            while (end < line.size() && std::isspace(static_cast<unsigned char>(line[end]))) ++end;
                            if (end == line.size() || (line[end] != ':' && line[end] != '{' && line[end] != '<')) continue;
                            debug_position.line = row;
                            debug_position.column = at + 1;
                            break;
                        }
                    }
            }
            if (debug_position.line == 0) { debug_position.line = 1; debug_position.column = 1; }
            const auto debug_scope = [&](std::string_view kind) {
                std::ostringstream marker;
                marker << "ISPA_DEBUG_SCOPE " << kind << ' '
                       << std::quoted(corelib::text::join(prod.name, ".")) << ' '
                       << std::quoted(debug_position.file) << ' '
                       << debug_position.line << ' ' << debug_position.column;
                return LangAPI::Comment::createStatement(LangAPI::Comment{.value = marker.str()});
            };
            const auto debug_entry = [&] {
                std::ostringstream marker;
                marker << "ISPA_DEBUG_LINE " << std::quoted(debug_position.file) << ' '
                       << debug_position.line << ' ' << debug_position.column << " pos";
                return LangAPI::Comment::createStatement(LangAPI::Comment{.value = marker.str()});
            };
            const AST::DeclarationSyntax *declaration = nullptr;
            if (tree.contains(prod.name)) for (const auto &feature : tree[prod.name].features)
                if (const auto *syntax = std::get_if<AST::DeclarationSyntax>(&feature)) declaration = syntax;
            LangAPI::Type raw_rule_type{LangAPI::Symbol{prod.name}};
            if (declaration) for (const auto& parameter : declaration->templateParameters)
                raw_rule_type.template_parameters.emplace_back(LangAPI::Type{LangAPI::Symbol{parameter}});
            LangAPI::Type rule_type = ensureTypesNs(raw_rule_type);
            LangAPI::Type fun_type = ensureTypesNs(LangAPI::Type {LangAPI::ValueType::RuleResult, rule_type, LangAPI::Type {LangAPI::Symbol {"IT"}}});
            if (tree.contains(prod.name) && tree[prod.name].external) {
                const auto& external = *tree[prod.name].external;
                LangAPI::Function function;
                function.name = Parser::ParserBase::fun_name(prod.name);
                function.type = fun_type;
                function.parameters = {{LangAPI::Type{LangAPI::Symbol{"IT"}}, "pos"}};
                function.template_parameters = {{"IT", {
                    LangAPI::IspaLibSymbol{.exports = LangAPI::StdlibExports::LexerTokenIterator},
                    LangAPI::IspaLibSymbol{.exports = LangAPI::StdlibExports::LexerLazyIterator}}}};
                if (declaration) for (const auto& parameter : declaration->templateParameters)
                    function.template_parameters.insert(function.template_parameters.end() - 1,
                        std::make_pair(parameter, stdu::vector<LangAPI::Type>{}));
                appendRuntimeParameters(function, declaration);
                function.statements = LangAPI::Return::createStatements(LangAPI::Return{
                    .value = LangAPI::FunctionCall::createExpression(LangAPI::FunctionCall{
                        .name = std::make_shared<LangAPI::Symbol>(LangAPI::Symbol{"::ISPA_STD", "external_match"}),
                        .template_parameters = {std::make_shared<LangAPI::Type>(LangAPI::Type{LangAPI::Symbol{"::" + external.cpp_binding}})},
                        .args = {LangAPI::Symbol::createExpression(LangAPI::Symbol{"pos"})}})});
                function.statements.insert(function.statements.begin(), debug_scope("rule"));
                function.statements.insert(function.statements.begin() + 1, debug_entry());
                parser.data.emplace_back(std::make_shared<LangAPI::Declaration>(LangAPI::Function::createDeclaration(function)), LangAPI::Visibility::Protected);
                continue;
            }
            LangAPI::Statements stmts;
            stmts.push_back(debug_scope("rule"));
            stmts.push_back(debug_scope("production"));
            stmts.push_back(debug_entry());
            LangAPI::Variable result_v;
            result_v.type = rule_type;
            result_v.name = "result";
            result_v.value = LangAPI::EmptyInitializer::createExpression(LangAPI::EmptyInitializer{});
            stmts.push_back(LangAPI::Variable::createStatement(result_v));
            const bool advanced = std::ranges::any_of(source_rule.features, [](const auto& feature) {
                return std::holds_alternative<AST::AdvancedRule>(feature);
            });
            const auto has_failure = [&](auto&& self, const AST::RuleMember& member) -> bool {
                for (const auto& feature : member.features) if (std::holds_alternative<AST::FailMarker>(feature)) return true;
                if (member.isGroup()) for (const auto& child : member.getGroup().values) if (self(self, *child)) return true;
                if (member.isOp()) for (const auto& child : member.getOp().options) if (self(self, *child)) return true;
                return false;
            };
            const bool needs_cursor = advanced || std::ranges::any_of(source_rule.features, [](const auto& feature) {
                return std::holds_alternative<AST::FailBinding>(feature);
            }) || std::ranges::any_of(source_rule.rule_members, [&](const auto& member) { return has_failure(has_failure, *member); });
            if (needs_cursor) stmts.push_back(LangAPI::Variable{.name = "start_pos", .type = LangAPI::Type{LangAPI::Symbol{"IT"}},
                .value = LangAPI::Symbol::createExpression(LangAPI::Symbol{"pos"})});
            if (advanced) stmts.push_back(LangAPI::Variable{.name = "ctx", .type = LangAPI::Type{LangAPI::ValueType::Undef},
                .value = LangAPI::FunctionCall::createExpression(LangAPI::FunctionCall{
                    .name = std::make_shared<LangAPI::Symbol>("rule_context"),
                    .args = {LangAPI::Symbol::createExpression(LangAPI::Symbol{"pos"}),
                        LangAPI::Symbol::createExpression(LangAPI::Symbol{"result"}),
                        LangAPI::Symbol::createExpression(LangAPI::Symbol{"start_pos"}),
                        LangAPI::String::createExpression(LangAPI::String{.value = corelib::text::join(prod.name, ".")}),
                        LangAPI::Symbol::createExpression(LangAPI::Symbol{"TokensToString"})}})});
            for (const auto &stmt : prod.rule_members) {
                stmts.push_back(finalizeReturnStatement(finalizeTypes(stmt)));
            }
            if (prod.block.is_inclosed_map()) {
                const auto ib = prod.block.getInclosedMap();
                for (const auto &[name, expr] : ib) {
                    if (expr.first.empty()) continue; // initialized directly through ctx.ast
                    if ((name == "value" && source_rule.data_block.type) ||
                        std::ranges::any_of(source_rule.data_block.fields, [&](const auto& field) { return field.first == name; }))
                        continue; // capture was published when its match completed
                    LangAPI::StorageSymbol s;
                    s.what = LangAPI::Symbol::createExpression(LangAPI::Symbol {"result"});
                    s.path.push_back(name);
                    stmts.push_back(
                        assignCompatible(
                            LangAPI::StorageSymbol::createExpression(s),
                            ensureTypesNs(expr.first)
                        )
                    );
                }
            } else if (prod.block.is_regular_data_block() && !prod.block.getRegularDataBlock().first.empty() && !source_rule.data_block.type) {
                auto v = LangAPI::StorageSymbol {"value"};
                v.what = LangAPI::Symbol::createExpression(LangAPI::Symbol {"result"});
                stmts.push_back(
                    assignCompatible(
                        LangAPI::StorageSymbol::createExpression(v),
                        ConstructBase::ensureTypesNs(prod.block.getRegularDataBlock().first)
                    )
                );
            }
            LangAPI::Variable match_result;
            match_result.type = fun_type;
            match_result.name = "match_result";
            stmts.push_back(LangAPI::Variable::createStatement(match_result));
            LangAPI::StorageSymbol match_status;
            match_status.what = LangAPI::Symbol::createExpression(LangAPI::Symbol {"match_result"});
            match_status.path.push_back("status");
            stmts.push_back(LangAPI::VariableAssignment::createStatement(LangAPI::VariableAssignment {
                .name = match_status,
                .value = LangAPI::Bool::createExpression(LangAPI::Bool {.value = true})
            }));
            LangAPI::StorageSymbol match_data;
            match_data.what = LangAPI::Symbol::createExpression(
                LangAPI::Symbol{"match_result"}
            );
            match_data.path.push_back("node");
            match_data.path.push_back(
                LangAPI::FunctionCall{
                    .name = std::make_shared<LangAPI::Symbol>("set_data"),
                    .args = {
                        LangAPI::Symbol::createExpression(
                            LangAPI::Symbol{"result"}
                        )
                    }
                }
            );
            stmts.push_back(
                LangAPI::StorageSymbol::createStatement(
                    std::move(match_data)
                )
            );
            LangAPI::StorageSymbol match_it;
            match_it.what = LangAPI::Symbol::createExpression(LangAPI::Symbol {"match_result"});
            match_it.path.push_back("it");
            stmts.push_back(LangAPI::VariableAssignment::createStatement(LangAPI::VariableAssignment {
                .name = match_it,
                .value = LangAPI::Symbol::createExpression(LangAPI::Symbol {"pos"})
            }));
            stmts.push_back(LangAPI::Return::createStatement(LangAPI::Return { .value = LangAPI::Symbol::createExpression(LangAPI::Symbol {"match_result"}) }));
            LangAPI::Function prod_fun {
                .type = fun_type,
                .name = Parser::ParserBase::fun_name(prod.name),
                .parameters = {std::make_pair(LangAPI::Type {LangAPI::Symbol {"IT"}}, "pos")},
                .statements = stmts,
                .template_parameters = {std::make_pair("IT", stdu::vector<LangAPI::Type> {
                    LangAPI::IspaLibSymbol {.exports = LangAPI::StdlibExports::LexerTokenIterator},
                    LangAPI::IspaLibSymbol {.exports = LangAPI::StdlibExports::LexerLazyIterator}
                })},
            };
            if (declaration) for (const auto& parameter : declaration->templateParameters)
                prod_fun.template_parameters.insert(prod_fun.template_parameters.end() - 1,
                    std::make_pair(parameter, stdu::vector<LangAPI::Type>{}));
            appendRuntimeParameters(prod_fun, declaration);
            parser.data.push_back(std::make_pair(std::make_shared<LangAPI::Declaration>(std::move(prod_fun)), LangAPI::Visibility::Protected));
        }

        if (!lr) {
            std::set<stdu::vector<std::string>> template_candidates;
            for (const auto& [name, rule] : tree.getTreeMap()) if (rule.isTemplatePlaceholder)
                for (const auto& member : rule.rule_members)
                    if (member->isOp()) for (const auto& option : member->getOp().options)
                        if (option->isName()) template_candidates.insert(option->getName().name);
            for (const auto& candidate : template_candidates) {
                if (!tree.contains(candidate)) continue;
                for (const auto& feature : tree[candidate].features)
                    if (const auto* syntax = std::get_if<AST::DeclarationSyntax>(&feature);
                        syntax && (!syntax->parameters.empty() || !syntax->templateParameters.empty()))
                        throw std::invalid_argument("grammar template argument requires parameters: " +
                            corelib::text::join(candidate, "."));
                LangAPI::Type node_type = ensureTypesNs(LangAPI::Type{LangAPI::Symbol{candidate}});
                LangAPI::Type tag_type{LangAPI::ValueType::TypeTag};
                tag_type.template_parameters.emplace_back(node_type);
                LangAPI::Type result_type{LangAPI::ValueType::RuleResult, node_type,
                    LangAPI::Type{LangAPI::Symbol{"IT"}}};
                LangAPI::Function function;
                function.name = "ispa_match_template";
                function.type = result_type;
                function.parameters = {{tag_type, "tag"}, {LangAPI::Type{LangAPI::Symbol{"IT"}}, "pos"}};
                function.template_parameters = {{"IT", {
                    LangAPI::IspaLibSymbol{.exports = LangAPI::StdlibExports::LexerTokenIterator},
                    LangAPI::IspaLibSymbol{.exports = LangAPI::StdlibExports::LexerLazyIterator}}}};
                if (AST::RuleMemberName{.name = candidate}.isTerminal()) {
                    LangAPI::IspaLibFunctionCall match;
                    match.symbol = LangAPI::IspaLibSymbol{.exports = LangAPI::StdlibExports::ParserOptional};
                    match.template_parameters.push_back(std::make_shared<LangAPI::Type>(node_type));
                    match.args = {LangAPI::Symbol::createExpression(LangAPI::Symbol{"pos"})};
                    function.statements.push_back(LangAPI::Variable{.name = "node", .type = LangAPI::Type{LangAPI::ValueType::Rule, node_type},
                        .value = LangAPI::IspaLibFunctionCall::createExpression(std::move(match))});
                    LangAPI::StorageSymbol empty;
                    empty.what = LangAPI::Symbol::createExpression(LangAPI::Symbol{"node"});
                    empty.path.emplace_back(LangAPI::FunctionCall{
                        .name = std::make_shared<LangAPI::Symbol>("empty")});
                    LangAPI::Expression status{LangAPI::ExpressionValue{LangAPI::ExpressionElement::Not}};
                    auto empty_call = LangAPI::StorageSymbol::createExpression(std::move(empty));
                    status.insert(status.end(), empty_call.begin(), empty_call.end());
                    function.statements.push_back(LangAPI::Return::createStatement(LangAPI::Return{
                        .value = LangAPI::Inheritance::createExpression(LangAPI::Inheritance{
                            .name = std::make_shared<LangAPI::Type>(result_type),
                            .args = {std::move(status),
                                LangAPI::Symbol::createExpression(LangAPI::Symbol{"node"}),
                                LangAPI::Symbol::createExpression(LangAPI::Symbol{"pos"})}})}));
                } else {
                    function.statements = LangAPI::Return::createStatements(LangAPI::Return{
                        .value = LangAPI::FunctionCall::createExpression(LangAPI::FunctionCall{
                            .name = std::make_shared<LangAPI::Symbol>(Parser::ParserBase::fun_name(candidate)),
                            .args = {LangAPI::Symbol::createExpression(LangAPI::Symbol{"pos"})}})});
                }
                parser.data.emplace_back(std::make_shared<LangAPI::Declaration>(
                    LangAPI::Function::createDeclaration(std::move(function))), LangAPI::Visibility::Protected);
            }
        }

        auto add_stream_parse_wrapper = [&](
            std::string name,
            std::string iterator_type,
            stdu::vector<LangAPI::Expression> iterator_args,
            LangAPI::Statements pre_statements = {}
        ) {
            auto iterator = LangAPI::FunctionCall::createExpression(
                LangAPI::FunctionCall{
                    .name = std::make_shared<LangAPI::Symbol>(
                        LangAPI::Symbol{"Lexer", std::move(iterator_type)}
                    ),
                    .args = std::move(iterator_args)
                }
            );

            auto main_call = LangAPI::FunctionCall::createExpression(
                LangAPI::FunctionCall{
                    .name = std::make_shared<LangAPI::Symbol>("main"),
                    .args = {std::move(iterator)}
                }
            );
            LangAPI::StorageSymbol main_data;
            main_data.what = std::move(main_call);
            main_data.path.emplace_back("node");
            main_data.path.emplace_back(
                LangAPI::FunctionCall{
                    .name = std::make_shared<LangAPI::Symbol>("data")
                }
            );
            pre_statements.push_back(
                LangAPI::VariableAssignment::createStatement(
                        LangAPI::VariableAssignment{
                            .name = LangAPI::Symbol{"tree"},
                            .value = LangAPI::StorageSymbol::createExpression(
                                std::move(main_data)
                            )
                    }
            ));
            auto parse_wrapper = LangAPI::Function{
                .type = LangAPI::Type{LangAPI::ValueType::Void},
                .name = std::move(name),
                .statements = {
                    std::move(pre_statements),
                },
                .override = true,
                .is_wrap_function = true
            };

            parser.data.emplace_back(
                std::make_shared<LangAPI::Declaration>(
                    LangAPI::Function::createDeclaration(
                        std::move(parse_wrapper)
                    )
                ),
                LangAPI::Visibility::Public
            );
        };
        auto add_lazy_parse_wrapper = [&](
            std::string name,
            std::string iterator_type,
            stdu::vector<LangAPI::Expression> iterator_args,
            LangAPI::Statements pre_statements = {}
        ) {
            auto iterator = LangAPI::FunctionCall::createExpression(
                LangAPI::FunctionCall{
                    .name = std::make_shared<LangAPI::Symbol>(
                        LangAPI::Symbol{"Lexer", std::move(iterator_type)}
                    ),
                    .args = std::move(iterator_args)
                }
            );
            auto main_call = LangAPI::FunctionCall::createExpression(
                LangAPI::FunctionCall{
                    .name = std::make_shared<LangAPI::Symbol>("main"),
                    .args = {std::move(iterator)}
                }
            );
            pre_statements.push_back(LangAPI::Return::createStatement(LangAPI::Return {
                    .value = std::move(main_call)
                }
            ));
            auto parse_wrapper = LangAPI::Function{
                .type = LangAPI::Type{LangAPI::ValueType::RuleResult, LangAPI::Type {LangAPI::Symbol {"Types", "main"}}, LangAPI::Type {LangAPI::Symbol {"Lexer", "lazy_iterator"}}},
                .name = std::move(name),
                .statements = {
                    std::move(pre_statements),
                },
                .override = true,
                .is_wrap_function = true
            };

            parser.data.emplace_back(
                std::make_shared<LangAPI::Declaration>(
                    LangAPI::Function::createDeclaration(
                        std::move(parse_wrapper)
                    )
                ),
                LangAPI::Visibility::Public
            );
        };
        LangAPI::Expression lexer_reference;
        lexer_reference.push_back(LangAPI::Symbol::createRValue(LangAPI::Symbol {"lexer"}));
        add_stream_parse_wrapper("parseFromTokens", "iterator", {lexer_reference});
        parser.data.emplace_back(std::make_shared<LangAPI::Declaration>(
            LangAPI::Variable::createDeclaration(LangAPI::Variable{
                .name = "lazy_lexer_", .type = LangAPI::Type{LangAPI::Symbol{"Lexer"}}
            })), LangAPI::Visibility::Private);
        LangAPI::Statements pre_statements;
        LangAPI::StorageSymbol ss {
            LangAPI::Symbol::createExpression(LangAPI::Symbol {"lazy_lexer_"}), stdu::vector<LangAPI::StorageSymbol::PathPart> {LangAPI::FunctionCall {.name = std::make_shared<LangAPI::Symbol>("clearTokens")}}
        };
        pre_statements.push_back(LangAPI::StorageSymbol::createStatement(ss));
        add_lazy_parse_wrapper("lazyParse", "lazy_iterator", {
            LangAPI::Symbol::createExpression(LangAPI::Symbol {"lazy_lexer_"}),
            LangAPI::Symbol::createExpression(LangAPI::Symbol {"text"})
        }, pre_statements);
        holder.push(parser);
    }
}
