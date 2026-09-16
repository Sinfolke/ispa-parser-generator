module LangRepr.ConstructParser;

import LLIR.Builder.Base;
import cpuf.printf;
import NFA_OLD;
import DFA;

import corelib;

import std;

namespace LangRepr {
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
                ret.value = LangAPI::EmptyInitializer::createExpression(LangAPI::EmptyInitializer {});
                return LangAPI::Return::createStatement(ret);
            } else if (expr.back().isReportError()) {
                return {};
            }
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
    auto ConstructParser::constructParser() -> void {
        auto parser = createParserClass();
        // constructTokenMachineDFA(parser);
        for (const auto &prod : ir) {
            LangAPI::Type fun_type = ensureTypesNs(LangAPI::Type {LangAPI::Symbol {prod.name} }); // must be type as symbols are not automatically assigned
            LangAPI::Statements stmts;
            for (const auto &stmt : prod.members) {
                stmts.push_back(finalizeReturnStatement(stmt));
            }
            LangAPI::Variable result_v;
            result_v.type = ensureTypesNs(LangAPI::Type {LangAPI::Symbol {prod.name} });
            result_v.name = "result";
            stmts.push_back(LangAPI::Variable::createStatement(result_v));
            if (prod.block.is_inclosed_map()) {
                const auto ib = prod.block.getInclosedMap();
                for (const auto &[name, expr] : ib) {
                    LangAPI::StorageSymbol s;
                    s.what = LangAPI::Symbol::createExpression(LangAPI::Symbol {"result"});
                    s.path.push_back(name);
                    LangAPI::VariableAssignment variable_assignment;
                    variable_assignment.name = s;
                    variable_assignment.type = LangAPI::OperatorType::Assign;
                    variable_assignment.value = ConstructBase::ensureTypesNs(expr.first);
                    stmts.push_back(LangAPI::VariableAssignment::createStatement(variable_assignment));
                }
            } else if (prod.block.is_regular_data_block()) {
                auto v = LangAPI::StorageSymbol {"value"};
                v.what = LangAPI::Symbol::createExpression(LangAPI::Symbol {"result"});
                stmts.push_back(LangAPI::VariableAssignment::createStatement(LangAPI::VariableAssignment {
                    .name = v,
                    .value = ConstructBase::ensureTypesNs(prod.block.getRegularDataBlock().first)
                }));
            }
            stmts.push_back(LangAPI::Return::createStatement(LangAPI::Return { .value = LangAPI::Symbol::createExpression(LangAPI::Symbol {"result"}) }));
            LangAPI::Function prod_fun {
                .type = fun_type,
                .name = corelib::text::join(prod.name, "_"),
                .parameters = {std::make_pair(LangAPI::Type {LangAPI::Symbol {"IT"}}, "pos")},
                .statements = stmts,
                .template_parameters = {"IT"},
            };
            parser.data.push_back(std::make_pair(std::make_shared<LangAPI::Declaration>(std::move(prod_fun)), LangAPI::Visibility::Private));
        }
        holder.push(parser);
    }
}
