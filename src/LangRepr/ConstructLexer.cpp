module LangRepr.ConstructLexer;

import LLIR.Builder.Base;
import cpuf.printf;
import NFA_OLD;
import DFA.API;
import LangAPI;
import logging;
import corelib;
import std;

// -----------------------------------------------------------------------
// ARCHITECTURE NOTE
//
// Exactly two tables are emitted:
//
//   1. dfa_table       -- the actual (unified, classified) DFA. No FCDT,
//                          no per-token spans. State 0 is the single
//                          shared start state for every token.
//   2. char_class_table -- raw byte -> class id, shared globally across
//                          every state in dfa_table.
//
// No third table for acceptance. Instead, wherever a transition would
// have pointed at a materialized "empty"/accepting placeholder state, the
// target is encoded directly as:
//
//       state_count + 1 + <token enum value>
//
// i.e. any `next` value >= state_count is not a real state index at all;
// it's an accept marker, and the matched token id is `next - state_count
// - 1`. Genuinely dead transitions (no match, no accept) keep using
// DFA::NULL_STATE, which is far outside this range, so the three cases
// (real state / accept sentinel / dead) never collide:
//
//     next <  state_count           -> real continuing state
//     next == DFA::NULL_STATE       -> dead transition
//     otherwise (next > state_count)-> accept for token (next - state_count - 1)
//
// This removes the need to ever materialize placeholder "empty" states,
// and removes any separate accept_table/construct_table -- the token id
// is recoverable by arithmetic directly from the transition that was
// actually taken, with zero extra storage.
//
// RUNTIME TYPE ASSUMPTION: DFAAPI::Table<N> as given constrains state
// count == class count via a single template parameter. That only holds
// by coincidence; this file assumes the corrected two-parameter form:
//
//   template<std::size_t Classes> using State = std::array<std::size_t, Classes>;
//   template<std::size_t States, std::size_t Classes>
//   using Table = std::array<State<Classes>, States>;
//
// The runtime's DfaLookup lowering must decode accept sentinels using the
// SAME formula used here to encode them -- these two sides have to agree
// bit-for-bit.
// -----------------------------------------------------------------------

namespace LangRepr {

    auto ConstructLexer::makeCharClassTableDecl(const DFA::CharClassTable &table)
        -> std::pair<std::shared_ptr<LangAPI::Declaration>, LangAPI::Visibility> {

        LangAPI::Array arr;
        arr.values.reserve(256);
        for (std::size_t c = 0; c < 256; ++c) {
            arr.values.push_back(
                LangAPI::Int::createExpression(LangAPI::Int {
                    .value = static_cast<long long>(table.char_to_class[c])
                })
            );
        }

        LangAPI::IspaLibSymbol class_table_symbol {.exports = LangAPI::StdlibExports::DfaClassTable};

        LangAPI::Variable var {
            .name = "char_class_table",
            .type = LangAPI::Type {class_table_symbol},
            .value = LangAPI::Array::createExpression(arr),
            .is_static = true
        };

        return {
            std::make_shared<LangAPI::Declaration>(LangAPI::Variable::createDeclaration(var)),
            LangAPI::Visibility::Private
        };
    }

    // Emits the single unified transition table. Each entry is a plain
    // size_t, sentinel-encoded per the scheme above -- no wrapper type,
    // no per-transition metadata struct, since acceptance now lives
    // entirely in the numeric value rather than needing its own field.
    auto ConstructLexer::makeDfaTableDecl(
        const stdu::vector<DFA::State<DFA::ClassTransitions>> &states,
        std::size_t state_count,
        std::size_t class_count
    ) -> std::pair<std::shared_ptr<LangAPI::Declaration>, LangAPI::Visibility> {

        stdu::vector<LangAPI::Expression> rows;
        rows.reserve(state_count);

        for (const auto &state : states) {
            stdu::vector<LangAPI::Expression> row;
            row.reserve(class_count);

            for (std::size_t cls = 0; cls < class_count; ++cls) {
                LangAPI::RValue encoded;

                if (cls < state.transitions.size()) {
                    const auto &t = state.transitions[cls];
                    if (std::holds_alternative<NFA::DFATarget>(t)) {
                        if (std::get<NFA::DFATarget>(t).id == NFA::NULL_STATE) {
                            encoded = LangAPI::RValue {LangAPI::IspaLibSymbol {.exports = LangAPI::StdlibExports::DfaNullState}};
                        } else {
                            encoded = LangAPI::Int::createRValue(LangAPI::Int {.value = static_cast<long long>(std::get<NFA::DFATarget>(t).id)});
                        }
                    } else if (std::holds_alternative<NFA::ActionTarget>(t)) {
                        // t.next is final_st -- the DFA state to continue to
                        // AFTER this action/semantic chain resolves -- NOT the
                        // index of which lr_table entry to invoke. That real
                        // index lives in t.accept_index (populated from
                        // target.id in classify()). The runtime doesn't need
                        // final_st pre-baked into this sentinel at all: once
                        // it looks up lr_table[accept_index] and executes it,
                        // THAT entry's own next_state field (already correctly
                        // resolved/rebased by DFA.cpp's optimize/minimize
                        // passes) tells it what to do next -- continue to
                        // another action, a semantic reduction, or a DFA
                        // state -- chaining progressively at runtime rather
                        // than needing to be resolved in advance here.
                        // Embedding final_st here instead produced an
                        // arbitrary DFA-state-sized number in the slot the
                        // runtime treats as an action-table index, indexing
                        // clean off the end of the real (much smaller) table.
                        encoded = LangAPI::Int::createRValue(LangAPI::Int {.value = static_cast<long long>(std::get<NFA::ActionTarget>(t).id + state_count)});
                    } else if (std::holds_alternative<NFA::SemanticTarget>(t)) {
                        // Same fix, mirrored for semantic_table.
                        encoded = LangAPI::Int::createRValue(LangAPI::Int {.value = static_cast<long long>(std::get<NFA::SemanticTarget>(t).id + state_count + lexer_builder.getLRTable().size())});
                    } else {
                        throw Error("Unknown table type {}", t.index());
                    }
                }
                row.push_back(LangAPI::Int::createExpression(encoded));
            }
            rows.push_back(LangAPI::Array::createExpression(LangAPI::Array {.values = row}));
        }

        LangAPI::IspaLibSymbol dfa_table_symbol {.exports = LangAPI::StdlibExports::DfaTable};
        dfa_table_symbol.template_parameters.push_back(
            std::make_shared<LangAPI::RValue>(LangAPI::Int {.value = static_cast<long long>(state_count)})
        );
        dfa_table_symbol.template_parameters.push_back(
            std::make_shared<LangAPI::RValue>(LangAPI::Int {.value = static_cast<long long>(class_count)})
        );

        LangAPI::Variable var {
            .name = "dfa_table",
            .type = LangAPI::Type {dfa_table_symbol},
            .value = LangAPI::Array::createExpression(LangAPI::Array {.values = rows}),
            .is_static = true
        };

        return {
            std::make_shared<LangAPI::Declaration>(LangAPI::Variable::createDeclaration(var)),
            LangAPI::Visibility::Private
        };
    }

    auto ConstructLexer::makeLRTableDecl(
        const stdu::vector<NFA::ActionState> &states,
        std::size_t state_count
    ) -> std::pair<std::shared_ptr<LangAPI::Declaration>, LangAPI::Visibility> {
        (void)state_count;

        constexpr std::size_t lr_columns = 3;
        const auto lr_state_count = states.size();

        stdu::vector<LangAPI::Expression> rows;
        rows.reserve(lr_state_count);
        std::unordered_map<std::string, std::size_t> register_ids;
        for (const auto &state : states) {
            stdu::vector<LangAPI::Expression> row;
            row.reserve(lr_columns);

            row.push_back(LangAPI::Int::createExpression(LangAPI::Int {
                .value = static_cast<long long>(state.action)
            }));
            if (!register_ids.contains(state.variable))
                register_ids[state.variable] = register_ids.size();
            row.push_back(LangAPI::RValue::createExpression(LangAPI::Int {.value = static_cast<long long>(register_ids[state.variable])}));
            std::visit([&](const auto &target) {
                using T = std::decay_t<decltype(target)>;
                if (std::is_same_v<T, NFA::DFATarget>) {
                    row.push_back(LangAPI::Int::createExpression(LangAPI::Int {
                        .value = static_cast<long long>(target.id)
                    }));
                } else if (std::is_same_v<T, NFA::ActionTarget>) {
                    row.push_back(LangAPI::Int::createExpression(LangAPI::Int {
                        .value = static_cast<long long>(target.id + state_count)
                    }));
                } else if (std::is_same_v<T, NFA::SemanticTarget>) {
                    row.push_back(LangAPI::Int::createExpression(LangAPI::Int {
                        .value = static_cast<long long>(target.id + state_count + lexer_builder.getLRTable().size())
                    }));
                } else {
                    throw Error("Unknown target type {}", (int) target.id);
                }

            }, state.next_state);


            rows.push_back(
                LangAPI::Array::createExpression(
                    LangAPI::Array {.values = row}
                )
            );
        }

        LangAPI::IspaLibSymbol lr_table_symbol {
            .exports = LangAPI::StdlibExports::DfaTable
        };
        lr_table_symbol.template_parameters.push_back(
            std::make_shared<LangAPI::RValue>(
                LangAPI::Int {.value = static_cast<long long>(lr_state_count)}
            )
        );
        lr_table_symbol.template_parameters.push_back(
            std::make_shared<LangAPI::RValue>(
                LangAPI::Int {.value = static_cast<long long>(lr_columns)}
            )
        );

        LangAPI::Variable var {
            .name = "lr_table",
            .type = LangAPI::Type {lr_table_symbol},
            .value = LangAPI::Array::createExpression(
                LangAPI::Array {.values = rows}
            ),
            .is_static = true
        };

        return {
            std::make_shared<LangAPI::Declaration>(
                LangAPI::Variable::createDeclaration(var)
            ),
            LangAPI::Visibility::Private
        };
    }
    auto ConstructLexer::makeSemanticSwitchFunction(const stdu::vector<NFA::SemanticState> semantic_table) -> LangAPI::Function {
        stdu::vector<LangAPI::Statements> semantic_table_statements;
        // change semantic table to raw Statements
        for (const auto &semantic_state : semantic_table) {
            LangAPI::Statements statements = semantic_state.statements;
            LangAPI::RValue next_state;
            std::visit([&](const auto &target) {
                using T = std::decay_t<decltype(target)>;
                if (std::is_same_v<T, NFA::DFATarget>) {
                    if (target.id == NFA::NULL_STATE) {
                        next_state = LangAPI::RValue {LangAPI::IspaLibSymbol {.exports = LangAPI::StdlibExports::DfaNullState}};
                    } else {
                        next_state = LangAPI::Int::createRValue(LangAPI::Int {.value = static_cast<long long>(target.id)});
                    }
                } else if (std::is_same_v<T, NFA::ActionTarget>) {
                    next_state = LangAPI::Int::createRValue(LangAPI::Int {.value = static_cast<long long>(target.id + lexer_builder.getDFA().states.size())});
                } else if (std::is_same_v<T, NFA::SemanticTarget>) {
                    next_state = LangAPI::Int::createRValue(LangAPI::Int {.value = static_cast<long long>(target.id + lexer_builder.getDFA().states.size() + lexer_builder.getLRTable().size())});
                } else {
                    throw Error("Unknown target type {}", (int) target.id);
                }

            }, semantic_state.next_state);
            const auto &instance_symbol = std::get<LangAPI::Symbol>(semantic_state.instance_value.name);
            LangAPI::Symbol tokens_enum_value;
            tokens_enum_value.path.push_back("Tokens");
            std::string acc;
            for (const auto &str : instance_symbol.path) {
                acc += std::get<std::string>(str) + "_";
            }
            acc.pop_back();
            tokens_enum_value.path.push_back(acc);
            statements.push_back(
                LangAPI::Return::createStatement(LangAPI::Return {.value = LangAPI::MakeTuple::createExpression(LangAPI::MakeTuple {.args = {
                    LangAPI::Int::createExpression(next_state),
                    LangAPI::IspaLibFunctionCall::createExpression(LangAPI::IspaLibFunctionCall {
                        .symbol = LangAPI::IspaLibSymbol {.exports = LangAPI::StdlibExports::TokenNodeConstruct, .template_parameters = {std::make_shared<LangAPI::Type>(std::get<LangAPI::Symbol>(semantic_state.instance_value.name))}},
                        .args = {
                            LangAPI::Symbol::createExpression(LangAPI::Symbol {"start_pos"}),
                            LangAPI::Symbol::createExpression(LangAPI::Symbol {"start"}),
                            LangAPI::Symbol::createExpression(LangAPI::Symbol {"length"}),
                            LangAPI::Symbol::createExpression(LangAPI::Symbol {"line"}),
                            LangAPI::Symbol::createExpression(tokens_enum_value),
                            LangAPI::Inheritance::createExpression(semantic_state.instance_value)
                        }
                    })
                }}
            )}));
            semantic_table_statements.push_back(std::move(statements));
        }
        LangAPI::Function fun {
            .type = LangAPI::Type {LangAPI::ValueType::Tuple, LangAPI::Type {LangAPI::ValueType::Int}, LangAPI::Type {LangAPI::Symbol {"Token"}}},
            .name = "semantic_action_exec",
            .parameters = {
                std::make_pair(LangAPI::Type {LangAPI::ValueType::Int}, "state"),
                std::make_pair(LangAPI::Type {LangAPI::ValueType::Int}, "start_pos"),
                std::make_pair(LangAPI::Type {LangAPI::ValueType::NonOwnedString}, "start"),
                std::make_pair(LangAPI::Type {LangAPI::ValueType::Int}, "length"),
                std::make_pair(LangAPI::Type {LangAPI::ValueType::Int}, "line"),
                std::make_pair(LangAPI::Type {LangAPI::ValueType::Reference, LangAPI::Type { LangAPI::ValueType::Array, LangAPI::Type {LangAPI::ValueType::Variant, LangAPI::RValue {LangAPI::Symbol {"Token"}}, LangAPI::ValueType::Char, LangAPI::ValueType::String }}}, "values"),
                std::make_pair(LangAPI::Type {LangAPI::ValueType::Reference, LangAPI::Type { LangAPI::ValueType::Array, LangAPI::Type {LangAPI::ValueType::Array, LangAPI::Type {LangAPI::ValueType::Variant, LangAPI::RValue {LangAPI::Symbol {"Token"}}, LangAPI::ValueType::Char, LangAPI::ValueType::String }}}}, "vec_values"),
            },
            .is_static = true
        };
        LangAPI::Switch switch_;
        switch_.expression = LangAPI::Symbol::createExpression(LangAPI::Symbol {"state"});
        long long state = 0;
        for (const auto &statements : semantic_table_statements) {
            switch_.cases.emplace_back(LangAPI::Int::createRValue(LangAPI::Int {.value = state++}), ensureTypesNs(statements));
        }
        fun.statements = LangAPI::Switch::createStatements(switch_);
        fun.statements.push_back(LangAPI::Throw::createStatement(LangAPI::Throw {.throw_value = LangAPI::IspaLibFunctionCall::createExpression(LangAPI::IspaLibFunctionCall {.symbol = LangAPI::IspaLibSymbol {.exports = LangAPI::StdlibExports::Error}, .args = {LangAPI::String::createExpression(LangAPI::String {.value = "Out of bound semantic action"})}})}));
        return fun;
    }
    auto ConstructLexer::constructLexer() -> void {
        holder.push(LangAPI::TypeAlias::createDeclaration(createTypeToken()));
        Tlog::Branch b(logger, "LangRepr/ConstructLexer.log");
        auto lexer = createLexerClass();

        const auto &dfa = lexer_builder.getDFA();
        const auto &class_table = dfa.table;
        const auto &states = dfa.states.get();
        const std::size_t state_count = states.size();
        const std::size_t class_count = class_table.num_classes;

        lexer.data.push_back(makeCharClassTableDecl(class_table));

        lexer.data.push_back(makeDfaTableDecl(states, state_count, class_count));

        lexer.data.push_back(makeLRTableDecl(lexer_builder.getLRTable(), state_count));
        lexer.data.push_back(std::make_pair(std::make_shared<LangAPI::Declaration>(LangAPI::Function::createDeclaration(makeSemanticSwitchFunction(lexer_builder.getSemanticTable()))), LangAPI::Visibility::Private));
        if (lexer_builder.getDFA().states.size() > 0) {
            lexer.data.push_back(std::make_pair(
                std::make_shared<LangAPI::Declaration>(LangAPI::Variable::createDeclaration(LangAPI::Variable {
                    .name = "init_done",
                    .type = LangAPI::ValueType::Bool,
                    .value = LangAPI::Bool::createExpression(LangAPI::Bool {.value = false})
                })),
                LangAPI::Visibility::Private
            ));
            lexer.data.push_back(std::make_pair(
                std::make_shared<LangAPI::Declaration>(LangAPI::Function::createDeclaration(LangAPI::Function {
                    .type = LangAPI::ValueType::Void, .name = "init", .statements = LangAPI::Return::createStatements(LangAPI::Return {}), .override = true
                })),
                LangAPI::Visibility::Private
            ));
            lexer.data.push_back(std::make_pair(
                std::make_shared<LangAPI::Declaration>(LangAPI::Function::createDeclaration(LangAPI::Variable {
                    .name = "values",
                    .type = LangAPI::Type { LangAPI::ValueType::Array, LangAPI::Type {LangAPI::ValueType::Variant, LangAPI::RValue {LangAPI::Symbol {"Token"}}, LangAPI::ValueType::Char, LangAPI::ValueType::String } },
                })),
                LangAPI::Visibility::Private
            ));
            lexer.data.push_back(std::make_pair(
                std::make_shared<LangAPI::Declaration>(LangAPI::Function::createDeclaration(LangAPI::Variable {
                    .name = "vec_values",
                    .type = LangAPI::Type { LangAPI::ValueType::Array, LangAPI::Type {LangAPI::ValueType::Array, LangAPI::Type {LangAPI::ValueType::Variant, LangAPI::RValue {LangAPI::Symbol {"Token"}}, LangAPI::ValueType::Char, LangAPI::ValueType::String}}}
                })),
                LangAPI::Visibility::Private
            ));
            lexer.data.push_back(std::make_pair(
                std::make_shared<LangAPI::Declaration>(LangAPI::Function::createDeclaration(LangAPI::Variable {
                    .name = "registers",
                    .type = LangAPI::Type { LangAPI::ValueType::FixedSizeArray, LangAPI::Type {LangAPI::ValueType::NonOwnedString}, LangAPI::Int::createRValue(LangAPI::Int {.value = static_cast<long long>(lexer_builder.getMaxRegistersCount())})}
                })),
                LangAPI::Visibility::Private
            ));
            lexer.data.push_back(std::make_pair(
                std::make_shared<LangAPI::Declaration>(LangAPI::Function::createDeclaration(LangAPI::Variable {
                    .name = "register_ids",
                    .type = LangAPI::Type { LangAPI::ValueType::FixedSizeArray, LangAPI::Type {LangAPI::ValueType::Int}, LangAPI::Int::createRValue(LangAPI::Int {.value = static_cast<long long>(lexer_builder.getMaxRegistersCount())})}
                })),
                LangAPI::Visibility::Private
            ));
        }

        lexer.data.push_back(
            std::make_pair(
                std::make_shared<LangAPI::Declaration>(LangAPI::Function::createDeclaration(LangAPI::Function {
                    .type = LangAPI::Type {Token},
                    .name = "makeToken",
                    .parameters = {std::make_pair(
                        LangAPI::Type {LangAPI::IspaLibSymbol {
                            .exports = LangAPI::StdlibExports::LexerMakeTokenParameter,
                            .Const = true, .Reference = true
                        }},
                        std::string("pos")
                    )},
                    .statements = LangAPI::Return::createStatements(
                        LangAPI::Return {
                            .value = LangAPI::FunctionCall::createExpression(LangAPI::FunctionCall {
                                .name = std::make_shared<LangAPI::Symbol>(LangAPI::Symbol {"lookup"}),
                                .args =
                                {
                                    LangAPI::Symbol::createExpression(LangAPI::Symbol {"dfa_table"}),
                                    LangAPI::Symbol::createExpression(LangAPI::Symbol {"char_class_table"}),
                                    LangAPI::Symbol::createExpression(LangAPI::Symbol {"lr_table"}),
                                    LangAPI::Symbol::createExpression(LangAPI::Symbol {"values"}),
                                    LangAPI::Symbol::createExpression(LangAPI::Symbol {"vec_values"}),
                                    LangAPI::Symbol::createExpression(LangAPI::Symbol {"registers"}),
                                    LangAPI::Symbol::createExpression(LangAPI::Symbol {"register_ids"}),
                                    LangAPI::Symbol::createExpression(LangAPI::Symbol {"semantic_action_exec"}),
                                    LangAPI::Symbol::createExpression(LangAPI::Symbol {"pos"}),
                                }
                            })
                        }
                    ),
                    .override = true,
                })),
                LangAPI::Visibility::Public
            )
        );

        holder.push(lexer);
    }
}