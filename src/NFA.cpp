module NFA;
import logging;
import corelib;
import cpuf.op;
import cpuf.printf;
import constants;
import AST.API;
import LLIR.RuleBuilder;
import LLIR.Builder.Base;
import logging;
import std;

namespace {
    std::string summarizeAcceptBinding(const std::optional<NFA::TokenBinding>& binding);
    std::string describeMemberContext(const AST::RuleMember* member);
    std::string describeQuantifierReason(const AST::RuleMember& member);
    std::string describeNestedTokenBoundary(
        bool nestedReduction,
        bool buildingNestedRule,
        bool isRepeating
    );
}

auto NFA::applyQuantifierAndActions(
    const AST::RuleMember &member,
    std::size_t start,
    std::size_t end,
    StateRange body,
    bool isLastMember,
    bool addStoreActions,
    bool nestedReduction,
    bool collapseIterationBegin
) -> StateRange
{
    Tlog::Branch b(logger, "NFA/applyQuantifierAndActions");

    logger.log(
        "applyQuantifierAndActions: addStoreActions={}, prefix='{}', nestedReduction={}",
        addStoreActions,
        member.prefix,
        nestedReduction
    );

    const bool has_store = addStoreActions && !member.prefix.empty();
    const bool is_repeating =
        (member.quantifier == '+' || member.quantifier == '*') &&
        !member.isCsequence();

    const std::string member_ctx = describeMemberContext(&member);

    states[start].debug_note =
        "fragment entry: " + member_ctx + "; " +
        describeQuantifierReason(member);

    states[end].debug_note =
        "fragment exit: " + member_ctx + "; " +
        describeQuantifierReason(member);

    auto make_action = [](
        Action action,
        const std::string &variable,
        std::size_t target
    ) -> ActionChain {
        return ActionChain {
            ActionState {
                .action = action,
                .variable = variable,
                .next_nfa_state = target,
                .next_state = DFATarget {target}
            }
        };
    };

    auto add_epsilon = [&](std::size_t source,
                           std::size_t target,
                           ActionChain actions = {}) {
        states[source].epsilon_transitions.insert({
            target,
            next_priority(),
            std::move(actions)
        });
    };

    std::string register_name;
    Action close_action = Action::UNDEF;

    if (has_store) {
        auto r = name_;
        r.push_back("r" + std::to_string(registers_count++));
        r.push_back(is_repeating ? "push" : "end");
        register_name = corelib::text::join(r, "_");
        close_action = is_repeating ? Action::PUSH : Action::END;

        // The capture BEGIN is an action on the selected epsilon edge.
        add_epsilon(
            start,
            body.start,
            make_action(
                Action::BEGIN,
                register_name,
                body.start
            )
        );
    } else {
        add_epsilon(start, body.start);
    }

    const std::size_t loop_target =
        collapseIterationBegin ? body.start : start;

    /*
     * A captured zero-width body is special.
     *
     * We retain exactly one empty occurrence and keep the normal
     * consuming repetition paths. The important TDFA property here is
     * that BEGIN/END/PUSH/REDUCE all live on the epsilon transition
     * which actually selects that path.
     */
    bool nullable_body = false;

    if (has_store && is_repeating) {
        std::vector<std::size_t> pending{body.start};
        std::unordered_set<std::size_t> visited;
        visited.reserve(states.size());

        while (!pending.empty()) {
            const std::size_t current = pending.back();
            pending.pop_back();

            if (!visited.insert(current).second)
                continue;

            if (current == body.end) {
                nullable_body = true;
                break;
            }

            for (const auto &epsilon : states[current].epsilon_transitions) {
                if (epsilon.next != NULL_STATE)
                    pending.push_back(epsilon.next);
            }
        }
    }

    auto completion_actions = [&](std::size_t target) -> ActionChain {
        ActionChain actions;

        if (has_store) {
            actions.push_back(ActionState {
                .action = close_action,
                .variable = register_name,
                .next_nfa_state = target,
                .next_state = DFATarget {target}
            });
        }

        return actions;
    };

    auto loop_actions = [&](std::size_t target) -> ActionChain {
        if (!has_store)
            return {};

        return ActionChain {
            ActionState {
                .action = Action::PUSH,
                .variable = register_name,
                .next_nfa_state = target,
                .next_state = DFATarget {target}
            }
        };
    };

    /*
     * markAccept must see the completion edge before it can append the
     * REDUCE action. Therefore build the edge first, then decorate it.
     */
    auto add_plain_completion = [&](std::size_t source, std::size_t target) {
        ActionChain actions = completion_actions(target);
        states[source].epsilon_transitions.insert({
            target,
            next_priority(),
            std::move(actions)
        });

        if (isLastMember) {
            markAccept(
                source,
                target,
                member,
                nestedReduction,
                is_repeating
            );
        }
    };

    if (nullable_body) {
        switch (member.quantifier) {
        case '*':
        case '+':
            add_epsilon(
                body.end,
                loop_target,
                loop_actions(loop_target)
            );
            add_plain_completion(body.end, end);
            break;

        default:
            add_plain_completion(body.end, end);
            break;
        }
    } else {
        switch (member.quantifier) {
        case '?':
            // Optional bypass skips the capture and therefore cannot
            // produce a reduction for this member.
            add_epsilon(start, end);
            add_plain_completion(body.end, end);
            break;

        case '+':
            add_epsilon(
                body.end,
                loop_target,
                loop_actions(loop_target)
            );
            add_plain_completion(body.end, end);
            break;

        case '*':
            add_epsilon(
                body.end,
                loop_target,
                loop_actions(loop_target)
            );
            add_plain_completion(body.end, end);
            // Zero iterations do not complete this member/token.
            add_epsilon(start, end);
            break;

        default:
            add_plain_completion(body.end, end);
            break;
        }
    }

    if (isLastMember) {
        states[end].debug_note +=
            "; last-member accept; " +
            describeNestedTokenBoundary(
                nestedReduction,
                buildingNestedRule,
                is_repeating
            );
    }

    return {start, end};
}

void NFA::markAccept(
    std::size_t state_id,
    std::size_t next_state,
    const AST::RuleMember &member,
    bool nestedReduction,
    bool is_repeating
) {
    TokenBinding binding;

    binding.token_id = nestedReduction
        ? NESTED_REDUCE_ID_BASE + nested_accept_counter++
        : *accept_index;

    binding.is_unique_representation = true;
    binding.reduce_rule_id = *accept_index;

    SemanticState state;

    using SymbolFactory =
        std::function<LangAPI::StorageSymbol()>;

    auto symbol_of = [](std::string name) -> SymbolFactory {
        return [name] {
            LangAPI::StorageSymbol s;

            s.what =
                LangAPI::Symbol::createExpression(
                    LangAPI::Symbol {name}
                );

            return s;
        };
    };

    auto values_slot_at =
        [](std::string array, std::size_t offset) -> SymbolFactory {
            return [array, offset] {
                LangAPI::StorageSymbol s;

                s.what =
                    LangAPI::Symbol::createExpression(
                        LangAPI::Symbol {array}
                    );

                s.path = {
                    LangAPI::StorageOffset {
                        .offset =
                            LangAPI::Int::createExpression(
                                LangAPI::Int {
                                    .value =
                                        static_cast<long long>(offset)
                                }
                            )
                    }
                };

                return s;
            };
        };

    auto unwrap_token =
        [&](const SymbolFactory &source,
            const std::string &name,
            LangAPI::Statements &out) {

        LangAPI::Variable token_v {
            .name = name + "_token",
            .type = LangAPI::Type {
                LangAPI::Symbol {"Token"}
            },
            .value =
                LangAPI::GetVariant::createExpression(
                    LangAPI::GetVariant {
                        .type =
                            std::make_shared<LangAPI::Type>(
                                LangAPI::Type {
                                    LangAPI::Symbol {"Token"}
                                }
                            ),
                        .sym =
                            LangAPI::StorageSymbol::createExpression(
                                source()
                            )
                    }
                )
        };

        out.push_back(
            LangAPI::Variable::createStatement(token_v)
        );

        return token_v.name;
    };

    auto extract_string_or_char =
        [&](const SymbolFactory &source,
            const std::string &target_name,
            LangAPI::Statements &out) {

        LangAPI::If type_check {
            LangAPI::CheckVariant::createExpression(
                LangAPI::CheckVariant {
                    .type =
                        std::make_shared<LangAPI::Type>(
                            LangAPI::ValueType::Char
                        ),
                    .sym =
                        LangAPI::StorageSymbol::createExpression(
                            source()
                        )
                }
            )
        };

        LangAPI::Variable stored_char {
            .name = target_name + "_stored",
            .type = LangAPI::ValueType::Char,
            .value =
                LangAPI::GetVariant::createExpression(
                    LangAPI::GetVariant {
                        .type =
                            std::make_shared<LangAPI::Type>(
                                LangAPI::ValueType::Char
                            ),
                        .sym =
                            LangAPI::StorageSymbol::createExpression(
                                source()
                            )
                    }
                )
        };

        type_check.stmt.push_back(
            LangAPI::Variable::createStatement(stored_char)
        );

        type_check.stmt.push_back(
            LangAPI::VariableAssignment::createStatement(
                LangAPI::VariableAssignment {
                    .name =
                        LangAPI::Symbol {target_name},
                    .value =
                        LangAPI::CharToStringConstructor::createExpression(
                            LangAPI::CharToStringConstructor {
                                .what =
                                    LangAPI::Symbol::createExpression(
                                        LangAPI::Symbol {
                                            stored_char.name
                                        }
                                    )
                            }
                        )
                }
            )
        );

        type_check.else_stmt.push_back(
            LangAPI::VariableAssignment::createStatement(
                LangAPI::VariableAssignment {
                    .name =
                        LangAPI::Symbol {target_name},
                    .value =
                        LangAPI::GetVariant::createExpression(
                            LangAPI::GetVariant {
                                .type =
                                    std::make_shared<LangAPI::Type>(
                                        LangAPI::ValueType::String
                                    ),
                                .sym =
                                    LangAPI::StorageSymbol::createExpression(
                                        source()
                                    )
                            }
                        )
                }
            )
        );

        out.push_back(type_check);
    };

    auto extract_variant_alternative =
        [&](const SymbolFactory &source,
            const std::string &target_name,
            const stdu::vector<LangAPI::Type> &alternatives,
            LangAPI::Statements &out) {

        LangAPI::Statements chain;

        for (std::size_t i = alternatives.size(); i-- > 0; ) {
            const auto &alt = alternatives[i];

            LangAPI::If type_check {
                LangAPI::CheckVariant::createExpression(
                    LangAPI::CheckVariant {
                        .type =
                            std::make_shared<LangAPI::Type>(alt),
                        .sym =
                            LangAPI::StorageSymbol::createExpression(
                                source()
                            )
                    }
                )
            };

            type_check.stmt.push_back(
                LangAPI::VariableAssignment::createStatement(
                    LangAPI::VariableAssignment {
                        .name =
                            LangAPI::Symbol {target_name},
                        .value =
                            LangAPI::GetVariant::createExpression(
                                LangAPI::GetVariant {
                                    .type =
                                        std::make_shared<LangAPI::Type>(alt),
                                    .sym =
                                        LangAPI::StorageSymbol::createExpression(
                                            source()
                                        )
                                }
                            )
                    }
                )
            );

            type_check.else_stmt = std::move(chain);

            chain = LangAPI::Statements{};
            chain.push_back(
                LangAPI::If::createStatement(type_check)
            );
        }

        for (auto &stmt : chain)
            out.push_back(stmt);
    };

    auto assign_from =
        [](const SymbolFactory &source,
           const LangAPI::Type &t,
           const std::string &target_name) {

        return LangAPI::VariableAssignment::createStatement(
            LangAPI::VariableAssignment {
                .name =
                    LangAPI::Symbol {target_name},
                .value =
                    LangAPI::GetVariant::createExpression(
                        LangAPI::GetVariant {
                            .type =
                                std::make_shared<LangAPI::Type>(t),
                            .sym =
                                LangAPI::StorageSymbol::createExpression(
                                    source()
                                )
                        }
                    )
            }
        );
    };

    auto create_variable_for_access_repeating =
        [&](const LangAPI::Type &t,
            std::string name,
            std::size_t offset,
            const AST::RuleMember &field_member) {

        LangAPI::Statements statements;

        auto v_type =
            LangAPI::Type {
                LangAPI::ValueType::Array,
                LangAPI::Type {
                    LangAPI::ValueType::Variant,
                    LangAPI::Type {
                        LangAPI::Symbol {"Token"}
                    },
                    LangAPI::Type {
                        LangAPI::ValueType::Char
                    },
                    LangAPI::Type {
                        LangAPI::ValueType::String
                    }
                }
            };

        LangAPI::Variable v {
            .name = name + "_with_variant",
            .type = v_type
        };

        LangAPI::Variable v_actual {
            .name = name,
            .type = t
        };

        LangAPI::Variable i {
            .name = "i_" + name,
            .type = LangAPI::ValueType::Int,
            .value =
                LangAPI::Int::createExpression(
                    LangAPI::Int {.value = 0}
                )
        };

        auto vec_values_size = [] {
            LangAPI::StorageSymbol s;

            s.what =
                LangAPI::Symbol::createExpression(
                    LangAPI::Symbol {"vec_values"}
                );

            s.path = {
                LangAPI::ArrayMethodCall {
                    .method =
                        LangAPI::ArrayMethods::Size
                }
            };

            return s;
        };

        LangAPI::While array_loop {
            LangAPI::Expression {
                LangAPI::ExpressionValue {
                    LangAPI::Symbol {"i_" + name}
                },
                LangAPI::ExpressionValue {
                    LangAPI::ExpressionElement::NotEqual
                },
                LangAPI::StorageSymbol::createExpressionValue(
                    vec_values_size()
                )
            }
        };

        auto element_at_i = [name] {
            LangAPI::StorageSymbol s;

            s.what =
                LangAPI::Symbol::createExpression(
                    LangAPI::Symbol {
                        name + "_with_variant"
                    }
                );

            s.path = {
                LangAPI::StorageOffset {
                    .offset =
                        LangAPI::Symbol::createExpression(
                            LangAPI::Symbol {
                                "i_" + name
                            }
                        )
                }
            };

            return s;
        };

        LangAPI::Statements loop_body;

        if (field_member.isName() &&
            field_member.getName().isTerminal()) {

            const auto token_name =
                unwrap_token(
                    element_at_i,
                    name,
                    loop_body
                );

            const std::string term_name =
                name + "_term";

            loop_body.push_back(
                LangAPI::Variable::createStatement(
                    LangAPI::Variable {
                        .name = term_name,
                        .type = t
                    }
                )
            );

            if (t.getValueType() ==
                LangAPI::ValueType::String) {

                extract_string_or_char(
                    symbol_of(token_name),
                    term_name,
                    loop_body
                );
            } else {
                loop_body.push_back(
                    assign_from(
                        symbol_of(token_name),
                        t,
                        term_name
                    )
                );
            }

            LangAPI::StorageSymbol array_push;

            array_push.what =
                LangAPI::Symbol::createExpression(
                    LangAPI::Symbol {name}
                );

            array_push.path = {
                LangAPI::ArrayMethodCall {
                    .method =
                        LangAPI::ArrayMethods::Push,
                    .args = {
                        LangAPI::Symbol::createExpression(
                            LangAPI::Symbol {term_name}
                        )
                    }
                }
            };

            loop_body.push_back(
                LangAPI::StorageSymbol::createStatement(
                    array_push
                )
            );
        } else {
            LangAPI::StorageSymbol array_push;

            array_push.what =
                LangAPI::Symbol::createExpression(
                    LangAPI::Symbol {name}
                );

            array_push.path = {
                LangAPI::ArrayMethodCall {
                    .method =
                        LangAPI::ArrayMethods::Push,
                    .args = {
                        LangAPI::GetVariant::createExpression(
                            LangAPI::GetVariant {
                                .type =
                                    std::make_shared<LangAPI::Type>(t),
                                .sym =
                                    LangAPI::StorageSymbol::createExpression(
                                        element_at_i()
                                    )
                            }
                        )
                    }
                }
            };

            loop_body.push_back(
                LangAPI::StorageSymbol::createStatement(
                    array_push
                )
            );
        }

        loop_body.push_back(
            LangAPI::Expression::createStatement(
                LangAPI::Expression {
                    LangAPI::ExpressionValue {
                        LangAPI::Symbol {
                            "i_" + name
                        }
                    },
                    LangAPI::ExpressionValue {
                        LangAPI::ExpressionElement::PlusPlus
                    }
                }
            )
        );

        array_loop.stmt = loop_body;

        statements.push_back(v);
        statements.push_back(v_actual);
        statements.push_back(i);
        statements.push_back(array_loop);

        return statements;
    };

    auto create_variable_for_access =
        [&](const LangAPI::Type &t,
            std::string name,
            std::size_t offset,
            const AST::RuleMember &field_member) {

        LangAPI::Statements statements;

        statements.push_back(
            LangAPI::Variable {
                .name = name,
                .type = t
            }
        );

        auto values_slot =
            values_slot_at("values", offset);

        const bool is_repeating_csequence_variant =
            (field_member.isCsequence() &&
             (field_member.quantifier == '+' ||
              field_member.quantifier == '*')) ||
            (t.isValueType() &&
             t.getValueType() ==
                 LangAPI::ValueType::Variant);

        if (is_repeating_csequence_variant) {
            if (t.getValueType() ==
                LangAPI::ValueType::String) {

                extract_string_or_char(
                    values_slot,
                    name,
                    statements
                );
            } else if (
                t.getValueType() ==
                LangAPI::ValueType::Variant
            ) {
                const auto token_name =
                    unwrap_token(
                        values_slot,
                        name,
                        statements
                    );

                auto token_symbol =
                    symbol_of(token_name);

                stdu::vector<LangAPI::Type> alternatives;

                for (auto &tp : t.template_parameters)
                    alternatives.push_back(
                        std::get<LangAPI::Type>(tp)
                    );

                extract_variant_alternative(
                    token_symbol,
                    name,
                    alternatives,
                    statements
                );
            } else {
                statements.push_back(
                    assign_from(
                        values_slot,
                        t,
                        name
                    )
                );
            }
        } else if (
            field_member.isName() &&
            field_member.getName().isTerminal()
        ) {
            const auto token_name =
                unwrap_token(
                    values_slot,
                    name,
                    statements
                );

            auto token_symbol =
                symbol_of(token_name);

            if (t.getValueType() ==
                LangAPI::ValueType::String) {

                extract_string_or_char(
                    token_symbol,
                    name,
                    statements
                );
            } else if (
                t.getValueType() ==
                LangAPI::ValueType::Variant
            ) {
                stdu::vector<LangAPI::Type> alternatives;

                for (auto &tp : t.template_parameters)
                    alternatives.push_back(
                        std::get<LangAPI::Type>(tp)
                    );

                extract_variant_alternative(
                    token_symbol,
                    name,
                    alternatives,
                    statements
                );
            } else {
                statements.push_back(
                    assign_from(
                        token_symbol,
                        t,
                        name
                    )
                );
            }
        } else {
            statements.push_back(
                assign_from(
                    values_slot,
                    t,
                    name
                )
            );
        }

        LangAPI::StorageSymbol pop;

        pop.what =
            LangAPI::Symbol::createExpression(
                LangAPI::Symbol {"values"}
            );

        pop.path = {
            LangAPI::ArrayMethodCall {
                .method =
                    LangAPI::ArrayMethods::Pop
            }
        };

        statements.push_back(
            LangAPI::StorageSymbol::createStatement(pop)
        );

        return statements;
    };

    if (dtb->isRegularDataBlock()) {
        const auto &data_block =
            dtb->getRegDataBlock();

        LangAPI::Type type =
            LLIR::BuilderBase::deduceVarTypeByRuleMember(
                member
            );

        LangAPI::Statements insert_statements;

        if (is_repeating) {
            insert_statements =
                create_variable_for_access_repeating(
                    type,
                    "value",
                    0,
                    member
                );
        } else {
            insert_statements =
                create_variable_for_access(
                    type,
                    "value",
                    0,
                    member
                );
        }

        state.statements.insert(
            state.statements.end(),
            insert_statements.begin(),
            insert_statements.end()
        );

        state.instance_value.name =
            LangAPI::Symbol {name_};

        state.instance_value.args.push_back(
            LangAPI::Symbol::createExpression(
                LangAPI::Symbol {"value"}
            )
        );
    } else if (dtb->isTemplatedDataBlock()) {
        const auto &data_block =
            dtb->getTemplatedDataBlock();

        stdu::vector<AST::RuleMember*> members_with_prefix;

        for (const auto &m : *rules) {
            if (!m->prefix.empty())
                members_with_prefix.push_back(&(*m));
        }

        for (long long i =
                 static_cast<long long>(
                     data_block.names.size()
                 ) - 1;
             i >= 0;
             --i) {

            const auto &key =
                data_block.names[
                    static_cast<std::size_t>(i)
                ];

            const auto u_idx =
                static_cast<std::size_t>(i);

            LangAPI::Type type =
                LLIR::BuilderBase::deduceVarTypeByRuleMember(
                    *members_with_prefix[u_idx]
                );

            LangAPI::Statements insert_statements;

            if (is_repeating) {
                insert_statements =
                    create_variable_for_access_repeating(
                        type,
                        key,
                        u_idx,
                        *members_with_prefix[u_idx]
                    );
            } else {
                insert_statements =
                    create_variable_for_access(
                        type,
                        key,
                        u_idx,
                        *members_with_prefix[u_idx]
                    );
            }

            state.statements.insert(
                state.statements.end(),
                insert_statements.begin(),
                insert_statements.end()
            );

            state.instance_value.name =
                LangAPI::Symbol {name_};

            state.instance_value.args.push_back(
                LangAPI::Symbol::createExpression(
                    LangAPI::Symbol {key}
                )
            );
        }
    } else {
        state.instance_value =
            LangAPI::Inheritance {
                .name = name_
            };
    }

    std::reverse(
        state.instance_value.args.begin(),
        state.instance_value.args.end()
    );

    state.nfa_index = state_id;
    state.next_state = DFATarget {next_state};

    /*
     * The binding belongs to the state reached after the reduction.
     * The REDUCE itself belongs to the epsilon transition which reaches
     * that state. This is the TDFA representation: state identity says
     * what was accepted, while the transition carries the ordered tag.
     */
    states[next_state].accept_binding = binding;

    bool decorated = false;

    for (auto it = states[state_id].epsilon_transitions.begin();
         it != states[state_id].epsilon_transitions.end();
         ++it) {
        if (it->next != next_state)
            continue;

        auto transition = *it;
        transition.actions.push_back(state);

        states[state_id].epsilon_transitions.erase(it);
        states[state_id].epsilon_transitions.insert(
            std::move(transition)
        );

        decorated = true;
        break;
    }

    if (!decorated) {
        states[state_id].epsilon_transitions.insert({
            next_state,
            next_priority(),
            ActionChain {state}
        });
    }

    states[state_id].debug_note +=
        "; accept: " +
        describeMemberContext(&member) +
        "; " +
        describeNestedTokenBoundary(
            nestedReduction,
            buildingNestedRule,
            is_repeating
        );

    value_types.clear();
}

void NFA::handleTerminal(
    const AST::RuleMember &member,
    const stdu::vector<std::string> &name,
    const std::size_t &start,
    const std::size_t &end,
    bool isLastMember,
    bool addStoreActions,
    bool nestedReduction
) {
    const std::size_t body_start = states.size();
    states.emplace_back();

    const std::size_t body_end = states.size();
    states.emplace_back();

    const auto &called_terminal = tree[name];

    // A terminal reference is one contiguous fragment in the parent NFA.
    // Build the referenced rule completely in a temporary NFA first.
    //
    // IMPORTANT:
    //   The old implementation copied nested_nfa.states once per member.
    //   Since nested_nfa.states is cumulative, for A B C it copied:
    //
    //       A
    //       A B
    //       A B C
    //
    //   into the parent. Apart from duplicating the graph, this duplicated
    //   every nested REDUCE/SemanticState and created many competing TDFA
    //   histories. A nested rule must be copied exactly once.
    std::size_t nested_first = NULL_STATE;
    std::size_t nested_last = NULL_STATE;

    if (!called_terminal.rule_members.empty()) {
        Tlog::Branch b(
            logger,
            "NFA/" + corelib::text::join(name, "::")
        );

        NFA nested_nfa {
            tree,
            name,
            &called_terminal.data_block,
            called_terminal.rule_members,
            isWhitespaceToken,
            is_char_table,
            accept_index,
            true
        };

        // Build the complete referenced rule inside nested_nfa.
        for (std::size_t i = 0;
             i < called_terminal.rule_members.size();
             ++i) {

            const auto &nested_member =
                *called_terminal.rule_members[i];

            const bool nested_is_last =
                i + 1 == called_terminal.rule_members.size();

            nested_nfa.current_member = &nested_member;

            auto fragment = nested_nfa.buildStateFragment(
                nested_member,
                nested_is_last,
                addStoreActions,
                nested_is_last
            );

            if (fragment.invalid())
                continue;

            // Keep the nested rule's member sequence inside nested_nfa.
            // Do not connect/copy it into the parent yet.
            if (nested_first == NULL_STATE) {
                nested_first = fragment.start;
            } else {
                nested_nfa.states[nested_last].epsilon_transitions.insert({
                    fragment.start,
                    nested_nfa.next_priority()
                });
            }

            nested_last = fragment.end;

            logger.log(
                "{} -> ({}, {})",
                nested_member,
                fragment.start,
                fragment.end
            );
        }

        if (nested_first != NULL_STATE &&
            nested_last != NULL_STATE) {

            const std::size_t offset = states.size();

            // Copy the complete nested graph exactly once. All references
            // which are local to the temporary NFA must be rebased to the
            // parent state numbering before the state is installed.
            for (auto nested_state : nested_nfa.states) {
                for (auto &[symbol, targets] : nested_state.transitions) {
                    for (auto &transition : targets)
                        transition.next += offset;
                }

                decltype(nested_state.epsilon_transitions) rebased_epsilon;

                for (auto transition : nested_state.epsilon_transitions) {
                    transition.next += offset;

                    for (auto &action : transition.actions) {
                        std::visit(
                            [&](auto &state) {
                                using T = std::decay_t<decltype(state)>;

                                if constexpr (
                                    std::is_same_v<T, ActionState>
                                ) {
                                    if (state.next_nfa_state != NULL_STATE)
                                        state.next_nfa_state += offset;

                                    // DFATarget here is still an NFA-local
                                    // destination at this stage. Other target
                                    // variants are DFA table references and
                                    // must not be rebased.
                                    if (std::holds_alternative<DFATarget>(
                                            state.next_state)) {
                                        auto &target =
                                            std::get<DFATarget>(
                                                state.next_state
                                            );
                                        if (target.id != NULL_STATE)
                                            target.id += offset;
                                    }
                                } else if constexpr (
                                    std::is_same_v<T, SemanticState>
                                ) {
                                    if (state.nfa_index != NULL_STATE)
                                        state.nfa_index += offset;

                                    if (std::holds_alternative<DFATarget>(
                                            state.next_state)) {
                                        auto &target =
                                            std::get<DFATarget>(
                                                state.next_state
                                            );
                                        if (target.id != NULL_STATE)
                                            target.id += offset;
                                    }
                                }
                            },
                            action
                        );
                    }

                    rebased_epsilon.insert(std::move(transition));
                }

                nested_state.epsilon_transitions =
                    std::move(rebased_epsilon);

                if (nested_state.accept_binding.has_value() &&
                    nested_state.accept_binding->target_semantic_state.has_value()) {
                    auto &target =
                        *nested_state.accept_binding->target_semantic_state;
                    if (target != NULL_STATE)
                        target += offset;
                }

                states.emplace_back(std::move(nested_state));
            }

            // Enter the referenced rule from the terminal-reference body.
            states[body_start].epsilon_transitions.insert({
                offset + nested_first,
                next_priority()
            });

            // Convert the nested rule's final state into the terminal
            // reference's body end. The nested REDUCE remains on the
            // transition which completed the nested rule itself.
            states[offset + nested_last].epsilon_transitions.insert({
                body_end,
                next_priority()
            });
        } else {
            // Every member was suppressed (e.g. #nospace). Keep the
            // terminal reference nullable rather than leaving body_start
            // disconnected.
            states[body_start].epsilon_transitions.insert({
                body_end,
                next_priority()
            });
        }
    } else {
        // Empty terminal rule.
        states[body_start].epsilon_transitions.insert({
            body_end,
            next_priority()
        });
    }

    const bool collapse_iteration_begin =
        called_terminal.rule_members.size() == 1 &&
        called_terminal.rule_members.front()->prefix.empty();

    // The semantic completion of the referenced rule and the completion of
    // this terminal reference are intentionally separate:
    //
    //   nested member -> REDUCE(nested rule) -> body_end
    //   body_end       -> REDUCE(calling rule, when applicable)
    //
    // Therefore the incoming nestedReduction flag belongs only to this
    // terminal reference; the final member above gets nestedReduction=true.
    applyQuantifierAndActions(
        member,
        start,
        end,
        {body_start, body_end},
        isLastMember,
        addStoreActions,
        nestedReduction,
        collapse_iteration_begin
    );
}

void NFA::handleGroup(
    const AST::RuleMember &member,
    const stdu::vector<std::shared_ptr<AST::RuleMember>> &group,
    const std::size_t &start,
    const std::size_t &end,
    bool isLastMember,
    bool addStoreActions,
    bool nestedReduction
) {
    std::size_t body_start = states.size();
    states.emplace_back();

    std::size_t body_end = body_start;
    auto inspectGroupForCapture = [&](auto this_f, const stdu::vector<std::shared_ptr<AST::RuleMember>> &values) -> bool {
        for (const auto &m : values) {
            if (m->isGroup()) {
                if (this_f(this_f, m->getGroup().values)) {
                    return true;
                }
            } else if (m->isOp()) {
                if (this_f(this_f, m->getOp().options)) {
                    return true;
                }
            }
            if (!(m->isName() || m->isNospace())) {
                return true;
            }
        }
        return false;
    };
    for (const auto &sub_ptr : group) {
        auto cached_no_space =
            no_add_space_skip_next;

        if (member.quantifier == '+' ||
            member.quantifier == '*') {
            no_add_space_skip_next = true;
        }

        auto fragment =
            buildStateFragment(
                *sub_ptr,
                false,
                addStoreActions,
                false
            );

        no_add_space_skip_next =
            cached_no_space;

        if (fragment.invalid())
            continue;

        states[body_end].epsilon_transitions.insert({
            fragment.start,
            next_priority()
        });

        body_end = fragment.end;
    }

    applyQuantifierAndActions(
        member,
        start,
        end,
        {body_start, body_end},
        isLastMember,
        inspectGroupForCapture(inspectGroupForCapture, group),
        nestedReduction
    );
}

void NFA::handleString(
    const AST::RuleMember &member,
    const std::string &str,
    const std::size_t &start,
    const std::size_t &end,
    bool isLastMember,
    bool addStoreActions,
    bool nestedReduction
) {
    if (addStoreActions)
        cst_node_close_propagate.push_back(end);

    std::size_t body_start = states.size();
    states.emplace_back();

    std::size_t current = body_start;

    for (std::size_t i = 0; i < str.size(); ++i) {
        std::size_t next = states.size();
        states.emplace_back();

        states[current].transitions[str[i]] = {
            {next}
        };

        current = next;
    }

    std::size_t body_end = current;

    applyQuantifierAndActions(
        member,
        start,
        end,
        {body_start, body_end},
        isLastMember,
        addStoreActions,
        nestedReduction
    );
}

void NFA::handleCsequence(
    const AST::RuleMember &member,
    const AST::RuleMemberCsequence &csequence,
    const std::size_t &start,
    const std::size_t &end,
    bool isLastMember,
    bool addStoreActions,
    bool nestedReduction
) {
    if (addStoreActions)
        cst_node_close_propagate.push_back(end);

    std::size_t body_start = states.size();
    states.emplace_back();

    std::size_t body_end = states.size();
    states.emplace_back();

    const auto &chars = csequence.characters;
    const auto &escaped = csequence.escaped;

    if (csequence.negative) {
        constexpr auto max =
            std::numeric_limits<unsigned char>::max();

        std::bitset<max + 1> prohibited;

        for (char c : chars)
            prohibited.set(
                static_cast<unsigned char>(c)
            );

        for (char c : escaped) {
            prohibited.set(
                static_cast<unsigned char>(
                    corelib::text::getEscapedFromChar(c)
                )
            );
        }

        for (auto [from, to] : csequence.diapasons) {
            for (char c = from; c <= to; ++c)
                prohibited.set(
                    static_cast<unsigned char>(c)
                );
        }

        for (
            unsigned char c =
                std::numeric_limits<unsigned char>::min();
            ;
            ++c
        ) {
            if (!prohibited.test(c)) {
                states[body_start]
                    .transitions[
                        static_cast<char>(c)
                    ] = {{body_end}};
            }

            if (c == max)
                break;
        }
    } else {
        for (char c : chars) {
            states[body_start].transitions[c] = {
                {body_end}
            };
        }

        for (char c : escaped) {
            char ec =
                corelib::text::getEscapedFromChar(c);

            states[body_start].transitions[ec] = {
                {body_end}
            };
        }

        for (auto [from, to] : csequence.diapasons) {
            for (char c = from; c <= to; ++c) {
                states[body_start].transitions[c] = {
                    {body_end}
                };
            }
        }
    }

    applyQuantifierAndActions(
        member,
        start,
        end,
        {body_start, body_end},
        isLastMember,
        addStoreActions,
        nestedReduction
    );
}
void NFA::handleAny(
    const AST::RuleMember &member,
    const std::size_t &start,
    const std::size_t &end,
    bool isLastMember,
    bool addStoreActions,
    bool nestedReduction
) {
    std::size_t body_start = states.size();
    states.emplace_back();

    std::size_t body_end = states.size();
    states.emplace_back();

    constexpr auto max =
        std::numeric_limits<unsigned char>::max();

    for (
        unsigned char c =
            std::numeric_limits<unsigned char>::min();
        ;
        ++c
    ) {
        states[body_start]
            .transitions[
                static_cast<char>(c)
            ] = {{body_end}};

        if (c == max)
            break;
    }

    applyQuantifierAndActions(
        member,
        start,
        end,
        {body_start, body_end},
        isLastMember,
        addStoreActions,
        nestedReduction
    );
}
auto NFA::buildStateFragment(
    const AST::RuleMember &member,
    bool isLastMember,
    bool addStoreActions,
    bool nestedReduction
) -> StateRange {
    // Whitespace NFAs are lexer infrastructure, not captured values.
    // `build()` already disables store actions for the top-level whitespace
    // NFA, but nested terminal expansion calls buildStateFragment() directly
    // and therefore bypasses that guard. Enforce the invariant at the
    // fragment boundary as well.
    if (isWhitespaceToken)
        addStoreActions = false;

    if (member.isNospace()) {
        no_add_space_skip_next = true;
        return {NULL_STATE, NULL_STATE};
    }

    bool is_repeting =
        member.quantifier == '+' ||
        member.quantifier == '*';

    const std::size_t entry = states.size();
    states.emplace_back();

    const std::size_t start = states.size();
    states.emplace_back();

    const std::size_t end = states.size();
    states.emplace_back();

    states[entry].debug_note =
        "entry for " +
        describeMemberContext(&member) +
        "; reason=" +
        describeQuantifierReason(member);

    states[start].debug_note =
        "fragment start for " +
        describeMemberContext(&member) +
        "; reason=" +
        describeQuantifierReason(member);

    states[end].debug_note =
        "fragment end for " +
        describeMemberContext(&member) +
        "; reason=" +
        describeQuantifierReason(member);

    states[entry].epsilon_transitions.insert({
        start,
        next_priority()
    });

    if (member.isName()) {
        const auto &name = member.getName();

        if (name.isNonterminal()) {
            // not implemented
            return {NULL_STATE, NULL_STATE};
        } else {
            handleTerminal(
                member,
                name.name,
                start,
                end,
                isLastMember,
                addStoreActions,
                nestedReduction
            );
        }
    } else if (member.isOp()) {
        const auto &op = member.getOp();

        auto cached_no_space_skip = no_add_space_skip_next;
        auto cached_group_count = group_count;
        bool was_group = false;
        bool was_storing_group = store_entire_group;

        const bool group_has_prefix = !member.prefix.empty();

        if (group_has_prefix)
            store_entire_group = true;

        std::size_t body_start = states.size();
        states.emplace_back();

        std::size_t body_end = states.size();
        states.emplace_back();

        bool any_option_has_prefix = false;

        for (const auto &option_ptr : op.options) {
            // Reserve the branch priority before constructing the option fragment.
            // This priority level MUST be used for both entry and exit epsilon transitions
            // to maintain strict branch precedence across the alternation.
            const std::size_t option_priority = next_priority();

            no_add_space_skip_next = cached_no_space_skip;
            group_count = cached_group_count;

            if (option_ptr->isGroup())
                was_group = true;

            /*
             * An option is not itself the completion of the
             * containing rule. The operator as a whole carries
             * the completion context.
             *
             * EXCEPTION: an option that is itself a name reference
             * to a token/terminal (e.g. #SYMBOL, #ESCAPE, #DIAPASON
             * inside a CSEQUENCE's `( #A | #B | #C )*`) is a complete
             * nested-token production every time it matches.
             */
            const bool option_is_token_ref =
                option_ptr->isName() &&
                option_ptr->getName().isTerminal();

            auto fragment = buildStateFragment(
                *option_ptr,
                option_is_token_ref,
                addStoreActions,
                option_is_token_ref
            );

            if (fragment.invalid())
                continue;

            // Connect branch entry from body_start
            states[body_start]
                .epsilon_transitions.insert({
                    fragment.start,
                    option_priority
                });

            // Connect branch exit to body_end.
            // FIXED: Reuse option_priority instead of calling next_priority() here.
            // Calling next_priority() after buildStateFragment() caused priority drift
            // due to sub-fragment priority allocations, breaking TDFA action prefix matching.
            states[fragment.end]
                .epsilon_transitions.insert({
                    body_end,
                    option_priority
                });

            if (!option_ptr->prefix.empty()) {
                any_option_has_prefix = true;

                if (isLastMember && !group_has_prefix) {
                    markAccept(
                        fragment.end,
                        body_end,
                        *option_ptr,
                        nestedReduction,
                        is_repeting
                    );
                }
            }
        }

        store_entire_group = was_storing_group;

        if (was_group && cached_group_count == group_count) {
            group_count++;
        }

        const bool suppress_default_accept =
            !group_has_prefix && any_option_has_prefix;

        applyQuantifierAndActions(
            member,
            start,
            end,
            {body_start, body_end},
            isLastMember && !suppress_default_accept,
            addStoreActions,
            nestedReduction
        );
    } else if (member.isGroup()) {
        handleGroup(
            member,
            member.getGroup().values,
            start,
            end,
            isLastMember,
            addStoreActions,
            nestedReduction
        );
    } else if (member.isString()) {
        handleString(
            member,
            member.getString().value,
            start,
            end,
            isLastMember,
            addStoreActions,
            nestedReduction
        );
    } else if (member.isCsequence()) {
        handleCsequence(
            member,
            member.getCsequence(),
            start,
            end,
            isLastMember,
            addStoreActions,
            nestedReduction
        );
    } else if (member.isAny()) {
        handleAny(
            member,
            start,
            end,
            isLastMember,
            addStoreActions,
            nestedReduction
        );
    } else {
        std::visit(
            [](auto &m) {
                throw Error(
                    "Undefined member: {}",
                    typeid(m).name()
                );
            },
            member.value
        );
    }

    if (!no_add_space_skip_next)
        add_space_skip_places.push_back(entry);

    no_add_space_skip_next = false;

    return {
        entry,
        end
    };
}

void NFA::build(bool addStoreActions) {
    if (isWhitespaceToken)
        addStoreActions = false;

    std::size_t last_state = NULL_STATE;
    std::size_t prev_end = NULL_STATE;

    if (rules != nullptr &&
        !rules->empty()) {

        for (std::size_t i = 0;
             i < rules->size();
             ++i) {

            bool is_last =
                i == rules->size() - 1;

            current_member =
                (*rules)[i].get();

            auto [start, end] =
                buildStateFragment(
                    *(*rules)[i],
                    is_last,
                    addStoreActions,
                    false
                );

            if (
                prev_end != NULL_STATE &&
                start != NULL_STATE
            ) {
                states[prev_end]
                    .epsilon_transitions.insert({
                        start,
                        next_priority()
                    });
            }

            if (end != NULL_STATE)
                prev_end = end;

            if (is_last)
                last_state = end;
        }

        if (states.empty()) {
            throw Error(
                "NFA/LR table cannot be empty"
            );
        }
    } else if (member != nullptr) {
        current_member = member;

        last_state =
            buildStateFragment(
                *member,
                true,
                addStoreActions,
                false
            ).end;
    } else {
        throw Error(
            "NFA rules/member cannot be null"
        );
    }

    if (addStoreActions) {
        if (dtb == nullptr) {
            nfadtb = std::monostate {};
        } else if (
            dtb->isTemplatedDataBlock()
        ) {
            TemplatedDataBlock templated_data_block;

            std::size_t prefix_index = 0;
            std::size_t index = 0;
            std::size_t group_index = 0;

            generateTemplatedDataBlockFromRules(
                *rules,
                templated_data_block,
                prefix_index,
                index,
                group_index
            );

            nfadtb =
                templated_data_block;
        } else if (
            dtb->isRegularDataBlock()
        ) {
            TemplatedDataBlockValue
                single_value_data_block;

            bool isAlreadyConstructed = false;

            generateSingleDataBlockFromRules(
                *rules,
                single_value_data_block,
                isAlreadyConstructed
            );

            nfadtb =
                single_value_data_block;
        } else {
            nfadtb = std::monostate {};
        }
    }

    if (!isWhitespaceToken)
        addSpaceSkip();

    buildAcceptMap();

    (*accept_index)++;
}

void NFA::getStatesToPropagate(
    std::size_t state_id,
    std::unordered_set<std::size_t> &result
) {
    const auto &state =
        states[state_id];

    if (
        state.transitions.empty() &&
        state.epsilon_transitions.empty()
    )
        return;

    result.insert(state_id);

    for (
        const auto &epsilon :
        state.epsilon_transitions
    ) {
        if (result.contains(epsilon.next))
            continue;

        getStatesToPropagate(
            epsilon.next,
            result
        );
    }
}

auto NFA::getStatesToPropagate(
    std::size_t id
) -> std::unordered_set<std::size_t> {
    std::unordered_set<std::size_t> result;

    getStatesToPropagate(
        id,
        result
    );

    return result;
}

auto NFA::investigateHasNext(
    std::size_t place,
    char c,
    std::unordered_set<std::size_t> &visited
) -> bool {
    for (
        const auto &[name, next] :
        states[place].transitions
    ) {
        if (
            std::holds_alternative<char>(name)
        ) {
            auto this_c =
                std::get<char>(name);

            if (this_c == c)
                return true;
        }
    }

    const auto &e_transitios =
        states[place].epsilon_transitions;

    return std::ranges::any_of(
        e_transitios.begin(),
        e_transitios.end(),
        [&](const auto &x) {
            if (visited.contains(x.next))
                return false;

            visited.insert(x.next);

            return investigateHasNext(
                x.next,
                c,
                visited
            );
        }
    );
}

auto NFA::investigateHasNext(
    std::size_t place,
    const stdu::vector<std::string> &name,
    std::unordered_set<std::size_t> &visited
) -> bool {
    for (
        const auto &[n, next] :
        states[place].transitions
    ) {
        if (
            std::holds_alternative<
                stdu::vector<std::string>
            >(n)
        ) {
            const auto &this_c =
                std::get<
                    stdu::vector<std::string>
                >(n);

            if (this_c == name)
                return true;
        }
    }

    const auto &e_transitios =
        states[place].epsilon_transitions;

    return std::ranges::any_of(
        e_transitios.begin(),
        e_transitios.end(),
        [&](const auto &x) {
            if (visited.contains(x.next))
                return false;

            visited.insert(x.next);

            return investigateHasNext(
                x.next,
                name,
                visited
            );
        }
    );
}

void NFA::addSpaceSkip() {
    for (const auto place :
         add_space_skip_places) {

        std::unordered_set<std::size_t>
            whitespace_chars;

        if (is_char_table) {
            for (
                const auto c :
                constants::whitespace_chars
            ) {
                whitespace_chars.insert(
                    static_cast<unsigned char>(c)
                );
            }
        }

        std::unordered_set<std::size_t>
            skip_chars;

        for (
            const auto c :
            whitespace_chars
        ) {
            std::unordered_set<std::size_t>
                visited;

            if (!investigateHasNext(
                    place,
                    static_cast<char>(c),
                    visited
                )) {
                skip_chars.insert(c);
            }
        }

        if (skip_chars.empty())
            continue;

        const std::size_t skip_state_id =
            states.size();

        states.emplace_back();

        auto &skip_state =
            states[skip_state_id];

        for (const auto c :
             skip_chars) {

            skip_state.transitions[
                static_cast<char>(c)
            ] = {
                {skip_state_id}
            };
        }

        skip_state.epsilon_transitions.insert({
            place,
            next_priority()
        });

        auto &state =
            states[place];

        for (const auto c :
             skip_chars) {

            state.transitions[
                static_cast<char>(c)
            ] = {
                {skip_state_id}
            };
        }
    }
}

void NFA::acceptMapVisitState(
    std::size_t index,
    std::optional<TokenBinding> current_binding,
    std::unordered_set<std::size_t>& visited
) {
    if (!visited.insert(index).second)
        return;

    if (
        states[index].accept_binding.has_value()
    ) {
        current_binding =
            states[index].accept_binding;
    }

    if (current_binding.has_value()) {
        accept_map[index] =
            current_binding.value();
    }

    for (
        const auto &e :
        states[index].epsilon_transitions
    ) {
        if (e.next != NULL_STATE) {
            acceptMapVisitState(
                e.next,
                current_binding,
                visited
            );
        }
    }
}

void NFA::buildAcceptMap() {
    accept_map.clear();

    for (
        std::size_t i = 0;
        i < states.size();
        ++i
    ) {
        if (
            states[i].accept_binding.has_value()
        ) {
            std::unordered_set<std::size_t>
                local_visited;

            acceptMapVisitState(
                i,
                states[i].accept_binding,
                local_visited
            );
        }
    }
}

void NFA::generateTemplatedDataBlockFromSingleRule(
    const AST::RuleMember &mem,
    TemplatedDataBlock &templated_data_block,
    std::size_t &prefix_index,
    std::size_t &index,
    std::size_t &group_index
) {
    if (!mem.prefix.empty()) {
        if (
            dtb->getTemplatedDataBlock()
                .names.size() <= prefix_index
        )
            return;

        const auto &name =
            dtb->getTemplatedDataBlock()
                .names[prefix_index++];

        if (mem.isGroup()) {
            templated_data_block.emplace(
                name,
                TemplatedDataBlockValue {
                    .type =
                        StoreCstNode::CST_GROUP,
                    .cst_index =
                        group_index++,
                    .AST = &mem
                }
            );
        } else if (mem.isOp()) {
            templated_data_block.emplace(
                name,
                TemplatedDataBlockValue {
                    .type =
                        StoreCstNode::CST_CONDITION,
                    .cst_index =
                        index++,
                    .AST = &mem
                }
            );
        } else {
            templated_data_block.emplace(
                name,
                TemplatedDataBlockValue {
                    .type =
                        StoreCstNode::CST_NODE,
                    .cst_index =
                        index++,
                    .AST = &mem
                }
            );
        }
    }

    if (mem.isGroup()) {
        generateTemplatedDataBlockFromRules(
            mem.getGroup().values,
            templated_data_block,
            prefix_index,
            index,
            group_index
        );
    } else if (mem.isOp()) {
        std::size_t start_prefix_index =
            prefix_index;

        std::size_t max_prefix_index =
            prefix_index;

        std::size_t start_index =
            index;

        std::size_t max_index =
            index;

        std::size_t start_group_index =
            group_index;

        std::size_t max_group_index =
            group_index;

        for (
            const auto &opt_ptr :
            mem.getOp().options
        ) {
            std::size_t current_prefix_index =
                start_prefix_index;

            std::size_t current_index =
                start_index;

            std::size_t current_group_index =
                start_group_index;

            generateTemplatedDataBlockFromSingleRule(
                *opt_ptr,
                templated_data_block,
                current_prefix_index,
                current_index,
                current_group_index
            );

            max_prefix_index =
                std::max(
                    max_prefix_index,
                    current_prefix_index
                );

            max_index =
                std::max(
                    max_index,
                    current_index
                );

            max_group_index =
                std::max(
                    max_group_index,
                    current_group_index
                );
        }

        prefix_index =
            max_prefix_index;

        index =
            max_index;

        group_index =
            max_group_index;
    }
}

void NFA::generateTemplatedDataBlockFromRules(
    const stdu::vector<
        std::shared_ptr<AST::RuleMember>
    > &rules,
    TemplatedDataBlock &templated_data_block,
    std::size_t &prefix_index,
    std::size_t &index,
    std::size_t &group_index
) {
    for (const auto &mem_ptr : rules) {
        generateTemplatedDataBlockFromSingleRule(
            *mem_ptr,
            templated_data_block,
            prefix_index,
            index,
            group_index
        );
    }
}

void NFA::generateSingleDataBlockFromRules(
    const stdu::vector<
        std::shared_ptr<AST::RuleMember>
    > &rules,
    TemplatedDataBlockValue &single_data_block,
    bool &isAlreadyConstructed
) {
    if (isAlreadyConstructed)
        return;

    for (const auto &mem_ptr : rules) {
        auto &mem = *mem_ptr;

        if (!mem.prefix.empty()) {
            single_data_block = {
                .type =
                    mem.isGroup()
                        ? StoreCstNode::CST_GROUP
                        : mem.isOp()
                            ? StoreCstNode::CST_CONDITION
                            : StoreCstNode::CST_NODE,

                .cst_index = 0,
                .AST = &mem,
            };

            isAlreadyConstructed = true;
            return;
        }

        if (mem.isGroup()) {
            generateSingleDataBlockFromRules(
                mem.getGroup().values,
                single_data_block,
                isAlreadyConstructed
            );

            if (isAlreadyConstructed)
                return;
        } else if (mem.isOp()) {
            generateSingleDataBlockFromRules(
                mem.getOp().options,
                single_data_block,
                isAlreadyConstructed
            );

            if (isAlreadyConstructed)
                return;
        }
    }
}

namespace {
    using StateIdMap =
        std::unordered_map<
            std::size_t,
            std::size_t
        >;

    std::string summarizeStateDebug(
        const NFA::state& s
    );

    std::string describeMemberContext(
        const AST::RuleMember* member
    ) {
        if (member == nullptr)
            return "<no-member>";

        std::ostringstream oss;

        oss << "member:";

        if (!member->prefix.name.empty()) {
            oss << member->prefix.name;
        } else {
            oss << "<unnamed>";
        }

        if (member->isName()) {
            oss << " name="
                << corelib::text::join(
                    member->getName().name,
                    "."
                );
        } else if (member->isString()) {
            oss << " string='"
                << member->getString().value
                << "'";
        } else if (member->isGroup()) {
            oss << " group";
        } else if (member->isOp()) {
            oss << " op";
        } else if (member->isCsequence()) {
            oss << " cseq";
        } else if (member->isAny()) {
            oss << " any";
        }

        if (member->quantifier != '\0') {
            oss << " quant='"
                << member->quantifier
                << "'";
        }

        if (member->isAutoGenerated)
            oss << " auto";

        return oss.str();
    }

    std::string describeQuantifierReason(
        const AST::RuleMember& member
    ) {
        switch (member.quantifier) {
        case '?':
            return
                "optional: zero-iteration bypass + body path";

        case '*':
            return
                "repeat: loop-back + optional zero-width exit";

        case '+':
            return
                "repeat at least once: loop-back after first body pass";

        default:
            return
                "single occurrence: exact one match";
        }
    }

    std::string describeNestedTokenBoundary(
        bool nestedReduction,
        bool buildingNestedRule,
        bool isRepeating
    ) {
        std::ostringstream oss;

        if (nestedReduction) {
            oss <<
                "nested boundary: interior completion, "
                "do not accept as outer token";
        } else if (buildingNestedRule) {
            oss <<
                "nested token boundary: real completion "
                "for nested rule";
        } else {
            oss <<
                "top-level boundary: final token accept";
        }

        if (isRepeating)
            oss << "; repeating capture";

        return oss.str();
    }

    std::size_t remapStateId(
        const StateIdMap& state_id_map,
        std::size_t original_id
    );

    std::string formatTransitionSummary(
        const NFA::TransitionValue& t,
        const StateIdMap* mapState = nullptr
    ) {
        std::ostringstream oss;

        const auto mapped_next =
            mapState
                ? remapStateId(*mapState, t.next)
                : t.next;

        oss << "next=" << mapped_next;

        if (t.AST_member != nullptr) {
            oss << " owner="
                << describeMemberContext(
                    t.AST_member
                );
        }

        if (!t.fragment.empty()) {
            oss << " frag="
                << t.fragment;
        }

        if (t.priority != 0) {
            oss << " prio="
                << t.priority;
        }

        if (!t.actions.empty()) {
            oss << " actions=";
            for (std::size_t i = 0; i < t.actions.size(); ++i) {
                if (i != 0)
                    oss << " -> ";
                std::visit(
                    [&](const auto& action) {
                        oss << action;
                    },
                    t.actions[i]
                );
            }
        }

        return oss.str();
    }

    std::string summarizeAcceptBinding(
        const std::optional<NFA::TokenBinding>& binding
    ) {
        if (!binding.has_value())
            return "accept=<none>";

        std::ostringstream oss;

        oss << "accept token_id="
            << binding->token_id;

        if (binding->target_semantic_state.has_value()) {
            oss << ", semantic="
                << binding->target_semantic_state.value();
        }

        if (binding->reduce_rule_id.has_value()) {
            oss << ", reduce_rule="
                << binding->reduce_rule_id.value();
        }

        if (binding->is_unique_representation)
            oss << ", unique=true";

        return oss.str();
    }

    std::string summarizeStateDebug(
        const NFA::state& s
    ) {
        std::string text;

        if (!s.transitions.empty()) {
            text +=
                "transitions=" +
                std::to_string(
                    s.transitions.size()
                );
        } else {
            text += "transitions=0";
        }

        text +=
            ", epsilon=" +
            std::to_string(
                s.epsilon_transitions.size()
            );

        if (s.accept_binding.has_value()) {
            text +=
                ", " +
                summarizeAcceptBinding(
                    s.accept_binding
                );
        }

        return text;
    }

    std::size_t remapStateId(
        const StateIdMap& state_id_map,
        std::size_t original_id
    ) {
        if (original_id == NFA::NULL_STATE)
            return original_id;

        const auto it =
            state_id_map.find(original_id);

        return it != state_id_map.end()
            ? it->second
            : original_id;
    }

    bool isMeaningfulState(
        const NFA::state& s
    ) {
        return
            !s.transitions.empty() ||
            !s.epsilon_transitions.empty() ||
            s.accept_binding.has_value();
    }

    std::size_t remapToMeaningfulState(
        const std::vector<NFA::state>& states,
        const std::unordered_set<std::size_t>& meaningful_ids,
        const StateIdMap& state_id_map,
        std::size_t original_id
    ) {
        if (
            original_id == NFA::NULL_STATE ||
            original_id >= states.size()
        ) {
            return remapStateId(
                state_id_map,
                original_id
            );
        }

        std::unordered_set<std::size_t> visited;
        stdu::vector<std::size_t> pending {
            original_id
        };

        while (!pending.empty()) {
            const std::size_t current =
                pending.back();
            pending.pop_back();

            if (!visited.insert(current).second)
                continue;

            if (meaningful_ids.contains(current)) {
                return remapStateId(
                    state_id_map,
                    current
                );
            }

            const auto& s = states[current];

            for (
                const auto& t :
                s.epsilon_transitions
            ) {
                if (
                    t.next != NFA::NULL_STATE &&
                    t.next < states.size()
                ) {
                    pending.push_back(t.next);
                }
            }
        }

        return remapStateId(
            state_id_map,
            original_id
        );
    }

    void printState(
        std::ostream& os,
        const NFA::state& s,
        const StateIdMap* state_id_map = nullptr
    ) {
        const auto mapState =
            [&](std::size_t id) {
                return state_id_map
                    ? remapStateId(
                        *state_id_map,
                        id
                    )
                    : id;
            };

        if (s.transitions.empty()) {
            os << "\t(none)\n";
        } else {
            for (
                const auto& [key, targets] :
                s.transitions
            ) {
                std::visit(
                    [&os, &targets, &mapState,
                     state_id_map](auto &key) {

                        if constexpr (
                            std::is_same_v<
                                std::decay_t<
                                    decltype(key)
                                >,
                                char
                            >
                        ) {
                            os
                                << "\t '"
                                << corelib::text::
                                    getEscapedAsStr(
                                        key,
                                        false
                                    )
                                << "' -> State ";
                        } else {
                            os
                                << "\t "
                                << key
                                << " -> State ";
                        }

                        for (const auto& t :
                             targets) {

                            os
                                << mapState(t.next)
                                << " ["
                                << t.next
                                << "]";

                            if (
                                t.AST_member != nullptr ||
                                !t.fragment.empty()
                            ) {
                                os
                                    << " {"
                                    << formatTransitionSummary(
                                        t,
                                        state_id_map
                                    )
                                    << "}";
                            }
                        }
                    },
                    key
                );

                os << '\n';
            }
        }

        os << "\te -> ";

        if (s.epsilon_transitions.empty()) {
            os << "(none)\n";
        } else {
            for (
                const auto& t :
                s.epsilon_transitions
            ) {
                os
                    << mapState(t.next)
                    << " ["
                    << formatTransitionSummary(
                        t,
                        state_id_map
                    )
                    << "], ";
            }

            os << "\n";
        }



        if (s.accept_binding.has_value()) {
            os
                << "\n\taccept token_id -> "
                << s.accept_binding->token_id;

            if (
                s.accept_binding
                    ->is_unique_representation
            ) {
                os
                    << " [REDUCE: rule "
                    << s.accept_binding
                        ->reduce_rule_id
                        .value_or(0)
                    << "]";
            }

            os << "\n";
        }
    }
}

std::ostream& operator<<(
    std::ostream& os,
    const NFA::state& s
) {
    printState(os, s);
    return os;
}

std::ostream& operator<<(
    std::ostream& os,
    const NFA& nfa
) {
    const auto& states =
        nfa.getStates();

    if (states.empty())
        return os;

    std::unordered_set<std::size_t>
        meaningful_ids;
    meaningful_ids.reserve(states.size());

    for (
        std::size_t i = 0;
        i < states.size();
        ++i
    ) {
        if (isMeaningfulState(states[i]))
            meaningful_ids.insert(i);
    }

    std::unordered_map<
        std::size_t,
        std::size_t
    > state_id_map;

    stdu::vector<std::size_t> ordered;
    ordered.reserve(states.size());

    std::unordered_set<std::size_t> visited;

    auto getNeighbours =
        [&](std::size_t state_id) {

            stdu::vector<std::size_t> result;

            const auto& s =
                states[state_id];

            for (
                const auto& [_, targets] :
                s.transitions
            ) {
                for (const auto& t :
                     targets) {

                    if (
                        t.next != NFA::NULL_STATE &&
                        t.next < states.size()
                    ) {
                        result.push_back(
                            t.next
                        );
                    }
                }
            }

            for (
                const auto& t :
                s.epsilon_transitions
            ) {
                if (
                    t.next != NFA::NULL_STATE &&
                    t.next < states.size()
                ) {
                    result.push_back(
                        t.next
                    );
                }
            }



            std::unordered_set<
                std::size_t
            > seen;

            stdu::vector<std::size_t> unique;

            for (const auto id :
                 result) {
                if (seen.insert(id).second)
                    unique.push_back(id);
            }

            return unique;
        };

    std::function<void(std::size_t)>
        visit =
            [&](std::size_t id) {

                if (
                    id >= states.size() ||
                    !visited.insert(id).second
                ) {
                    return;
                }

                state_id_map[id] =
                    ordered.size();

                ordered.push_back(id);

                for (
                    const auto next :
                    getNeighbours(id)
                ) {
                    visit(next);
                }
            };

    visit(0);

    for (
        std::size_t i = 0;
        i < states.size();
        ++i
    ) {
        if (!visited.contains(i))
            visit(i);
    }

    for (
        std::size_t display_id = 0;
        display_id < ordered.size();
        ++display_id
    ) {
        const std::size_t original_id =
            ordered[display_id];

        if (!meaningful_ids.contains(original_id))
            continue;

        os
            << "State "
            << display_id
            << "["
            << original_id
            << "]:\n";

        printState(
            os,
            states[original_id],
            &state_id_map
        );

        os << "\n";
    }

    os << "\nState graph:\n";

    for (
        std::size_t display_id = 0;
        display_id < ordered.size();
        ++display_id
    ) {
        const std::size_t original_id =
            ordered[display_id];

        if (!meaningful_ids.contains(original_id))
            continue;

        const auto& state =
            states[original_id];

        std::vector<std::string> edges;

        for (
            const auto& [key, targets] :
            state.transitions
        ) {
            std::visit(
                [&](auto &k) {

                    std::ostringstream label;

                    if constexpr (
                        std::is_same_v<
                            std::decay_t<
                                decltype(k)
                            >,
                            char
                        >
                    ) {
                        label
                            << corelib::text::
                                getEscapedAsStr(
                                    k,
                                    false
                                );
                    } else {
                        label << k;
                    }

                    for (const auto& t :
                         targets) {

                        edges.push_back(
                            std::string("[") +
                            label.str() +
                            "] -> " +
                            std::to_string(
                                remapToMeaningfulState(
                                    states,
                                    meaningful_ids,
                                    state_id_map,
                                    t.next
                                )
                            ) +
                            " (orig=" +
                            std::to_string(
                                t.next
                            ) +
                            ")"
                        );
                    }
                },
                key
            );
        }

        for (
            const auto& t :
            state.epsilon_transitions
        ) {
            edges.push_back(
                std::string("ε -> ") +
                std::to_string(
                    remapToMeaningfulState(
                        states,
                        meaningful_ids,
                        state_id_map,
                        t.next
                    )
                ) +
                " (orig=" +
                std::to_string(t.next) +
                ")"
            );
        }

        os
            << "  "
            << display_id
            << "["
            << original_id
            << "]";

        if (edges.empty()) {
            os
                << " -> "
                << "(no outgoing edges)\n";
        } else {
            os << " -> {";

            for (
                std::size_t i = 0;
                i < edges.size();
                ++i
            ) {
                if (i != 0)
                    os << "; ";

                os << edges[i];
            }

            os << "}\n";
        }

    }

    return os;
}

std::ostream& operator<<(
    std::ostream& os,
    const NFA::TableType& type
) {
    switch (type) {
    case NFA::TableType::DFA:
        os << "DFA";
        break;
    case NFA::TableType::Action:
        os << "Action";
        break;
    case NFA::TableType::Semantic:
        os << "Semantic";
        break;
    }
    return os;
}

std::ostream& operator<<(
    std::ostream& os,
    const NFA::Action& type
) {
    switch (type) {
    case NFA::Action::UNDEF:
        os << "UNDEF";
        break;
    case NFA::Action::BEGIN:
        os << "BEGIN";
        break;
    case NFA::Action::END:
        os << "END";
        break;
    case NFA::Action::PUSH:
        os << "PUSH";
        break;
    }
    return os;
}

std::ostream& operator<<(
    std::ostream& os,
    const NFA::DFATarget& tg
) {
    return os << tg.id;
}

std::ostream& operator<<(
    std::ostream& os,
    const NFA::ActionTarget& tg
) {
    return os << tg.id;
}

std::ostream& operator<<(
    std::ostream& os,
    const NFA::SemanticTarget& tg
) {
    return os << tg.id;
}

std::ostream& operator<<(
    std::ostream& os,
    const NFA::TokenBinding& binding
) {
    os << "TokenBinding{";
    os << "token_id=" << binding.token_id;
    os << ", target_semantic_state=";
    if (binding.target_semantic_state) {
        os << *binding.target_semantic_state;
    } else {
        os << "nullopt";
    }
    os << ", reduce_rule_id=";
    if (binding.reduce_rule_id) {
        os << *binding.reduce_rule_id;
    } else {
        os << "nullopt";
    }
    os << ", is_unique_representation="
       << (binding.is_unique_representation ? "true" : "false")
       << '}';
    return os;
}

std::ostream& operator<<(
    std::ostream& os,
    const NFA::ActionState& s
) {
    switch (s.action) {
    case NFA::Action::UNDEF:
        os << "UNDEF";
        break;

    case NFA::Action::BEGIN:
        os << "BEGIN";
        break;

    case NFA::Action::END:
        os << "END";
        break;

    case NFA::Action::PUSH:
        os << "PUSH";
        break;
    }

    os
        << "; var: "
        << s.variable
        << ";";

    if (!s.debug_note.empty()) {
        os
            << " debug: "
            << s.debug_note;
    }

    return os;
}

std::ostream& operator<<(
    std::ostream& os,
    const NFA::SemanticState& s
) {
    os << "SemanticState: \n";

    os
        << "\t\t"
        << s.statements
        << '\n'
        << s.instance_value
        << '\n';

    if (!s.debug_note.empty()) {
        os
            << "\t\tdebug: "
            << s.debug_note
            << '\n';
    }

    return os;
}