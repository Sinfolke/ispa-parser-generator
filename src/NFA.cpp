module NFA;
import logging;
import corelib;
import cpuf.op;
import cpuf.printf;
import constants;
import AST.API;
import LLIR.RuleBuilder;
import LLIR.Builder.Base;
import std;

auto NFA::applyQuantifierAndActions(
    const AST::RuleMember &member,
    std::size_t start,
    std::size_t end,
    StateRange body,
    bool isLastMember,
    bool addStoreActions,
    bool nestedReduction) -> StateRange
{
    Tlog::Branch b(logger, "NFA/applyQuantifierAndActions");
    logger.log(
        "applyQuantifierAndActions: addStoreActions={}, prefix='{}'",
        addStoreActions,
        member.prefix
    );
    const bool has_store = addStoreActions && !member.prefix.empty();
    const bool is_repeating = (member.quantifier == '+' || member.quantifier == '*') && !member.isCsequence();

    std::size_t entry_state = body.start;
    std::size_t exit_state  = body.end;
    std::size_t end_action_state = NULL_STATE;
    std::size_t loop_action_state = NULL_STATE;

    if (has_store) {
        auto r_begin = name_;
        auto r_end = name_;
        r_begin.push_back("r" + std::to_string(registers_count++));
        r_begin.push_back("begin");
        r_end.push_back("r" + std::to_string(registers_count++));
        r_end.push_back(is_repeating ? "push" : "end");

        const Action close_action = is_repeating ? Action::PUSH : Action::END;

        // 1. Begin action before body entry, hosted in its own intermediate state
        std::size_t begin_action_state = states.size();
        states.emplace_back();
        states[begin_action_state].actions.push_back(ActionState {
            .action = Action::BEGIN,
        });
        states[begin_action_state].epsilon_transitions.insert({body.start});
        states[start].epsilon_transitions.insert({begin_action_state});

        // 2. End / Push action ONLY on transition to final 'end' state
        end_action_state = states.size();
        states.emplace_back();
        states[end_action_state].actions.push_back(ActionState {
            .action = close_action,
            .variable = LangAPI::Variable {.name = corelib::text::join(r_end, "_")}
        });
        states[end_action_state].epsilon_transitions.insert({end});

        // 3. Loop-back push action re-enters 'start' to re-trigger BEGIN for repeat iterations
        if (is_repeating) {
            loop_action_state = states.size();
            states.emplace_back();
            states[loop_action_state].actions.push_back(ActionState {
                .action = Action::PUSH,
                .variable = LangAPI::Variable {.name = corelib::text::join(r_end, "_")}
            });
            states[loop_action_state].epsilon_transitions.insert({start});
        }

        value_types.push_back(LLIR::BuilderBase::deduceVarTypeByRuleMember(member));
    } else {
        states[start].epsilon_transitions.insert({body.start});
    }

    const std::size_t exit_target = (end_action_state != NULL_STATE) ? end_action_state : end;
    const std::size_t loop_target = (loop_action_state != NULL_STATE) ? loop_action_state : body.start;

    // ----------------------------------------------------------------
    // A captured zero-width body is special.  The ordinary `*` NFA has
    // both an epsilon bypass and an epsilon loop:
    //
    //     start -> end                 (zero iterations)
    //     body.end -> loop_target      (another iteration)
    //     body.end -> exit_target      (finish)
    //
    // When the body consumes no character at all, those edges describe
    // two different semantic histories at exactly the same input
    // position: no capture vs. an empty capture.  Worse, the loop can
    // repeat forever without consuming input.
    //
    // For a CAPTURED, PURELY ZERO-WIDTH body we define one empty
    // occurrence as the deterministic result.  This is important because
    // an empty capture is a real value and must reach `values` through the
    // normal BEGIN/PUSH(or END) machinery.  We therefore keep the normal
    // body -> exit path, but remove the zero-iteration bypass and the
    // zero-width loop.
    //
    // A nullable body which ALSO has consuming transitions is deliberately
    // left alone: it still needs the normal repetition graph because a
    // later iteration may consume input.
    // ----------------------------------------------------------------
    bool nullable_body = false;

    if (has_store && is_repeating) {
        // Determine whether body.end can be reached from body.start without
        // consuming a character.  We deliberately do NOT require the entire
        // body to be epsilon-only: a nested token may have both consuming and
        // nullable alternatives.
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

    // 4. Handle Quantifier Repetition and Optional Bypasses
    if (nullable_body) {
        // A captured nullable repetition must not have a zero-iteration
        // bypass competing with its empty iteration.  The first epsilon
        // iteration is the empty value and must be materialized by BEGIN/PUSH
        // (or BEGIN/END for the non-looping form).
        //
        // Keep the loop so a consuming alternative can still repeat.  The
        // closure path guard prevents the same epsilon-only action cycle from
        // being traversed indefinitely; therefore exactly one zero-width
        // occurrence is retained while consuming iterations remain possible.
        switch (member.quantifier) {
        case '*':
            // No zero-iteration bypass.
            states[body.end].epsilon_transitions.insert({loop_target});
            states[body.end].epsilon_transitions.insert({exit_target});
            break;

        case '+':
            // `+` already requires one iteration.  Its existing loop/exit
            // edges are exactly what we need.
            states[body.end].epsilon_transitions.insert({loop_target});
            states[body.end].epsilon_transitions.insert({exit_target});
            break;

        default:
            states[body.end].epsilon_transitions.insert({exit_target});
            break;
        }
    } else {
        switch (member.quantifier) {
        case '?':
            // Optional bypass jumps straight to 'end' (skips BEGIN and END)
            states[start].epsilon_transitions.insert({end});
            states[body.end].epsilon_transitions.insert({exit_target});
            break;
        case '+':
            // Loop back via loop_target (emits PUSH and restarts at 'start')
            states[body.end].epsilon_transitions.insert({loop_target});
            states[body.end].epsilon_transitions.insert({exit_target});
            break;
        case '*':
            // Optional bypass jumps straight to 'end'
            states[start].epsilon_transitions.insert({end});
            // Loop back via loop_target
            states[body.end].epsilon_transitions.insert({loop_target});
            states[body.end].epsilon_transitions.insert({exit_target});
            break;
            default:
                states[body.end].epsilon_transitions.insert({exit_target});
                break;
        }
    }

    // Last Member / Accept Marking
    if (isLastMember) {
        markAccept(end, end, member, nestedReduction, is_repeating);
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
    binding.token_id = *accept_index;
    binding.is_unique_representation = true;
    binding.reduce_rule_id = *accept_index;

    SemanticState state;
    auto create_variable_for_access_repeating = [&](const LangAPI::Type &t, std::string name, std::size_t offset) {
        LangAPI::Statements statements;
        auto v_type = LangAPI::Type {LangAPI::ValueType::Array, LangAPI::Type {LangAPI::ValueType::Variant, LangAPI::Type {LangAPI::Symbol {"Token"}}, LangAPI::Type {LangAPI::ValueType::Char}, LangAPI::Type {LangAPI::ValueType::String}}};
        LangAPI::StorageSymbol ss;
        ss.what = LangAPI::Symbol::createExpression(LangAPI::Symbol {"vec_values"});
        ss.path = {LangAPI::StorageOffset {.offset = LangAPI::Int::createExpression(LangAPI::Int {.value = static_cast<long long>(offset)})}};
        LangAPI::Variable v {
            .name = name + "_with_variant",
            .type = v_type,
        };
        LangAPI::Variable v_actual {
            .name = name,
            .type = t,
        };
        LangAPI::Variable i {.name = "i_" + name, .type = LangAPI::ValueType::Int, .value = LangAPI::Int::createExpression(LangAPI::Int {.value = 0})};
        LangAPI::StorageSymbol loop_access;
        loop_access.what = LangAPI::Symbol::createExpression(LangAPI::Symbol {"vec_values"});
        loop_access.path = {LangAPI::ArrayMethodCall {.method = LangAPI::ArrayMethods::Size}};
        LangAPI::While array_loop {
            LangAPI::Expression {
                LangAPI::ExpressionValue { LangAPI::Symbol {"i_" + name} },
                LangAPI::ExpressionValue {LangAPI::ExpressionElement::NotEqual},
                LangAPI::StorageSymbol::createExpressionValue(loop_access)
            }
        };
        LangAPI::StorageSymbol push_access;
        push_access.what = LangAPI::Symbol::createExpression(LangAPI::Symbol {name + "_with_variant"});
        push_access.path = {LangAPI::StorageOffset {.offset = LangAPI::Symbol::createExpression( LangAPI::Symbol { "i_" + name})}};
        LangAPI::StorageSymbol array_push;
        array_push.what = LangAPI::Symbol::createExpression(LangAPI::Symbol {name});
        array_push.path = {
            LangAPI::ArrayMethodCall {
                .method = LangAPI::ArrayMethods::Push, .args = {
                    LangAPI::GetVariant::createExpression(LangAPI::GetVariant {
                        .type = std::make_shared<LangAPI::Type>(t),
                        .sym = LangAPI::StorageSymbol::createExpression(push_access)
                    })}
            }
        };
        array_loop.stmt.push_back(
            LangAPI::Expression::createStatement(
                LangAPI::Expression { LangAPI::ExpressionValue { LangAPI::Symbol { "i_" + name } }, LangAPI::ExpressionValue {LangAPI::ExpressionElement::PlusPlus}}
            )
        );
        // box all statements
        array_loop.stmt = LangAPI::StorageSymbol::createStatements(array_push);
        statements.push_back(v);
        statements.push_back(v_actual);
        statements.push_back(i);
        statements.push_back(array_loop);
        return statements;
    };
    auto create_variable_for_access = [&](const LangAPI::Type &t, std::string name, std::size_t offset) {
        LangAPI::Statements statements;
        LangAPI::StorageSymbol ss;
        ss.what = LangAPI::Symbol::createExpression(LangAPI::Symbol {"values"});
        ss.path = {LangAPI::StorageOffset {.offset = LangAPI::Int::createExpression(LangAPI::Int {.value = static_cast<long long>(offset)})}};
        statements.push_back(LangAPI::Variable {
            .name = name,
            .type = t
        });
        if (member.isCsequence() && (member.quantifier == '+' || member.quantifier == '*') || t.isValueType() && t.getValueType() == LangAPI::ValueType::Variant) {
            if (t.getValueType() == LangAPI::ValueType::String) {
                std::cout << "ss: " << ss << std::endl;
                LangAPI::If type_check {LangAPI::CheckVariant::createExpression(LangAPI::CheckVariant {.type = std::make_shared<LangAPI::Type>(LangAPI::ValueType::Char), .sym = LangAPI::StorageSymbol::createExpression(ss)})};
                // ---------
                // This is necessary to make further ss symbols be non-empty. Reason is undefined
                // ---------
                ss.what = LangAPI::Symbol::createExpression(LangAPI::Symbol {"values"});
                ss.path = {LangAPI::StorageOffset {.offset = LangAPI::Int::createExpression(LangAPI::Int {.value = static_cast<long long>(offset)})}};
                // ---------
                LangAPI::Variable true_branch_v {
                    .name = name + "_stored",
                    .type = LangAPI::ValueType::Char,
                    .value = LangAPI::GetVariant::createExpression(LangAPI::GetVariant {.type = std::make_shared<LangAPI::Type>(LangAPI::ValueType::Char), .sym = LangAPI::StorageSymbol::createExpression(ss)})
                };
                LangAPI::VariableAssignment true_branch_v_str {
                    .name = LangAPI::Symbol {name},
                    .value = LangAPI::CharToStringConstructor::createExpression(LangAPI::CharToStringConstructor {.what = LangAPI::Symbol::createExpression(LangAPI::Symbol {name + "_stored"})})
                };
                type_check.stmt.push_back(LangAPI::Variable::createStatement(true_branch_v));
                type_check.stmt.push_back(LangAPI::VariableAssignment::createStatement(true_branch_v_str));
                // ---------
                // This is necessary to make further ss symbols be non-empty. Reason is undefined
                // ---------
                ss.what = LangAPI::Symbol::createExpression(LangAPI::Symbol {"values"});
                ss.path = {LangAPI::StorageOffset {.offset = LangAPI::Int::createExpression(LangAPI::Int {.value = static_cast<long long>(offset)})}};
                // ---------
                type_check.else_stmt.push_back(LangAPI::VariableAssignment::createStatement(LangAPI::VariableAssignment {
                    .name = LangAPI::Symbol {name},
                    .value = LangAPI::GetVariant::createExpression(LangAPI::GetVariant {.type = std::make_shared<LangAPI::Type>(LangAPI::ValueType::String), .sym = LangAPI::StorageSymbol::createExpression(ss)})
                }));
                statements.push_back(type_check);
            } else {
                statements.push_back(
                    LangAPI::VariableAssignment::createStatement(LangAPI::VariableAssignment {
                    .name = LangAPI::Symbol {name},
                    .value = LangAPI::GetVariant::createExpression(LangAPI::GetVariant {.type = std::make_shared<LangAPI::Type>(LangAPI::ValueType::String), .sym = LangAPI::StorageSymbol::createExpression(ss)})
                }));
            }
        } else {
            statements.push_back(LangAPI::VariableAssignment::createStatement(LangAPI::VariableAssignment {
                .name = LangAPI::Symbol {name},
                .value = LangAPI::GetVariant::createExpression(LangAPI::GetVariant {.type = std::make_shared<LangAPI::Type>(t), .sym = LangAPI::StorageSymbol::createExpression(ss)})
            }));
        }
        LangAPI::StorageSymbol ss_push;
        ss_push.what = LangAPI::Symbol::createExpression(LangAPI::Symbol {"values"});
        ss_push.path = {LangAPI::ArrayMethodCall {.method = LangAPI::ArrayMethods::Pop}};
        statements.push_back(LangAPI::StorageSymbol::createStatement(ss_push));
        return statements;
    };
    if (!member.prefix.empty()) {
        if (dtb->isRegularDataBlock()) {
            const auto &data_block = dtb->getRegDataBlock();
            // Safe fallback lookup for single value type
            LangAPI::Type type = !value_types.empty() ? value_types.back() : LangAPI::Type{};
            LangAPI::Statements insert_statements;
            if (is_repeating) {
                insert_statements = create_variable_for_access_repeating(type, "value", 0);
            } else {
                insert_statements = create_variable_for_access(type, "value", 0);
            }
            std::cout << "insert_statements: " << insert_statements << std::endl;
            state.statements.insert(state.statements.end(), insert_statements.begin(), insert_statements.end());
            state.instance_value.name = LangAPI::Symbol {name_};
            state.instance_value.args.push_back(LangAPI::Symbol::createExpression(LangAPI::Symbol { "value" }));
        } else if (dtb->isTemplatedDataBlock()) {
            const auto &data_block = dtb->getTemplatedDataBlock();
            for (long long i = static_cast<long long>(data_block.names.size()) - 1; i >= 0; --i) {
                const auto &key = data_block.names[static_cast<std::size_t>(i)];
                const auto u_idx = static_cast<std::size_t>(i);
                LangAPI::Type type = (u_idx < value_types.size()) ? value_types[u_idx] : LangAPI::Type{};
                LangAPI::Statements insert_statements;
                if (is_repeating) {
                    insert_statements = create_variable_for_access_repeating(type, key, i);
                } else {
                    insert_statements = create_variable_for_access(type, key, i);
                }
                state.statements.insert(state.statements.end(), insert_statements.begin(), insert_statements.end());
                state.instance_value.name = LangAPI::Symbol {name_};
                state.instance_value.args.push_back(LangAPI::Symbol::createExpression(LangAPI::Symbol { key }));
            }
        } else {
            state.instance_value = LangAPI::Inheritance {.name = name_};
        }
    } else {
        state.instance_value = LangAPI::Inheritance {.name = name_};
    }

    std::reverse(state.instance_value.args.begin(), state.instance_value.args.end());

    // Attach semantic state directly to this state's internal actions sequence
    states[state_id].actions.push_back(state);
    states[state_id].accept_binding = binding;

    if (nestedReduction) {
        states[state_id].epsilon_transitions.insert({next_state});
    }

    // Flush accumulated types so they don't leak into subsequent rules
    value_types.clear();
}
void NFA::handleTerminal(const AST::RuleMember &member, const stdu::vector<std::string> &name, const std::size_t &start, const std::size_t &end, bool &isLastMember, bool addStoreActions) {
    if (addStoreActions && !member.prefix.empty()) {
        cst_node_close_propagate.push_back(end);
    }

    std::size_t body_start = states.size();
    states.emplace_back();
    std::size_t body_end   = states.size();
    states.emplace_back();

    const auto &called_terminal = tree[name];
    NFA called_terminal_nfa(tree, name, &called_terminal.data_block, called_terminal.rule_members, name == constants::whitespace, is_char_table, accept_index);
    called_terminal_nfa.build();
    auto &called_terminal_nfa_states = called_terminal_nfa.getStates();

    if (called_terminal_nfa_states.empty()) {
        states[body_start].epsilon_transitions.insert({body_end});
        applyQuantifierAndActions(member, start, end, {body_start, body_end}, isLastMember, addStoreActions, false);
        return;
    }

    /*
     * ------------------------------------------------------------
     * Nested token support: embed the referenced terminal's own NFA
     * fragment inside this one.
     *
     * Every copied state must be rebased into THIS NFA's index space
     * (mirrors DFA::mergeTwoNFA in functionality.cpp). The previous
     * version computed a rebased copy (`new_state`) and then threw it
     * away, appending the UN-rebased `called_terminal_nfa_states`
     * instead. Those states' transitions/epsilon edges still pointed
     * at the called terminal's own local (0-based) state numbers,
     * which collided with unrelated states already present earlier in
     * the outer NFA. That fabricated extra epsilon paths into those
     * unrelated states carrying unrelated action sequences - exactly
     * the divergence DFA::build() rejects with "NFA destination state
     * has multiple epsilon paths with different action sequences".
     * ------------------------------------------------------------
     */
    const std::size_t offset = states.size();
    std::vector<std::size_t> exit_states;

    for (std::size_t i = 0; i < called_terminal_nfa_states.size(); ++i) {
        NFA::state new_state = called_terminal_nfa_states[i];

        for (auto &[symbol, targets] : new_state.transitions) {
            for (auto &target : targets) {
                if (target.next != NULL_STATE) {
                    target.next += offset;
                }
            }
        }

        decltype(new_state.epsilon_transitions) rebased_epsilon;
        for (auto target : new_state.epsilon_transitions) {
            if (target.next != NULL_STATE) {
                target.next += offset;
            }
            rebased_epsilon.insert(target);
        }
        new_state.epsilon_transitions = std::move(rebased_epsilon);

        for (auto &act_var : new_state.actions) {
            std::visit([&](auto &act) {
                using T = std::decay_t<decltype(act)>;
                if constexpr (std::is_same_v<T, ActionState>) {
                    if (act.next_nfa_state != NULL_STATE) {
                        act.next_nfa_state += offset;
                    }
                    if (std::holds_alternative<DFATarget>(act.next_state)) {
                        auto &t = std::get<DFATarget>(act.next_state);
                        if (t.id != NULL_STATE) {
                            t.id += offset;
                        }
                    }
                } else if constexpr (std::is_same_v<T, SemanticState>) {
                    if (act.nfa_index != NULL_STATE) {
                        act.nfa_index += offset;
                    }
                    if (std::holds_alternative<DFATarget>(act.next_state)) {
                        auto &t = std::get<DFATarget>(act.next_state);
                        if (t.id != NULL_STATE) {
                            t.id += offset;
                        }
                    }
                }
            }, act_var);
        }

        /*
         * The called terminal's own accept/REDUCE boundary must not
         * survive as an independent acceptance point once embedded:
         * this fragment only stands for "match this sub-pattern", not
         * "the outer token is complete here". The real accept marker
         * for THIS terminal is placed on `end` below, via
         * applyQuantifierAndActions() -> markAccept(). Record which
         * states used to be exits so the fragment can still be wired
         * to `body_end`, then drop the binding so it can't be
         * mistaken for a second, conflicting acceptance point.
         */
        if (new_state.accept_binding.has_value()) {
            exit_states.push_back(offset + i);
            new_state.accept_binding.reset();
        }

        states.emplace_back(std::move(new_state));
    }

    states[body_start].epsilon_transitions.insert({offset});

    if (exit_states.empty()) {
        // Defensive fallback: the referenced terminal produced no
        // explicit accept marker (shouldn't normally happen for a
        // terminal fragment). Wire the last appended state as the
        // exit rather than silently dropping the connection to
        // body_end and leaving this fragment a dead end.
        states[offset + called_terminal_nfa_states.size() - 1].epsilon_transitions.insert({body_end});
    } else {
        for (const auto exit_state : exit_states) {
            states[exit_state].epsilon_transitions.insert({body_end});
        }
    }

    applyQuantifierAndActions(member, start, end, {body_start, body_end}, isLastMember, addStoreActions, false);
}

void NFA::handleNonTermnal(const AST::RuleMember &member, const stdu::vector<std::string> &name, const std::size_t &start, const std::size_t &end, bool isLastMember, bool addStoreActions) {
    std::size_t body_start = states.size();
    states.emplace_back();
    std::size_t body_end   = states.size();
    states.emplace_back();
    std::size_t last = body_start;

    const auto &prod_rules = tree[name];
    for (const auto &prod_ptr : prod_rules.rule_members) {
        auto fragment = buildStateFragment(*prod_ptr, false, addStoreActions);
        if (fragment.invalid())
            continue;
        states[last].epsilon_transitions.insert({fragment.start});
        last = fragment.end;
    }
    states[last].epsilon_transitions.insert({body_end});

    // NFA.cpp inside NFA::handleNonTermnal
    applyQuantifierAndActions(member, start, end, {body_start, body_end}, isLastMember, addStoreActions, true);
}

void NFA::handleGroup(const AST::RuleMember &member,
                     const stdu::vector<std::shared_ptr<AST::RuleMember>> &group,
                     const std::size_t &start,
                     const std::size_t &end,
                     bool isLastMember,
                     bool addStoreActions)
{
    std::size_t body_start = states.size();
    states.emplace_back();
    std::size_t body_end   = body_start;

    for (const auto &sub_ptr : group) {
        auto cached_no_space = no_add_space_skip_next;
        if (member.quantifier == '+' || member.quantifier == '*') {
            no_add_space_skip_next = true;
        }

        auto fragment = buildStateFragment(*sub_ptr, false, addStoreActions);
        no_add_space_skip_next = cached_no_space;

        if (fragment.invalid())
            continue;

        states[body_end].epsilon_transitions.insert({fragment.start});
        body_end = fragment.end;
    }

    applyQuantifierAndActions(member, start, end, {body_start, body_end}, isLastMember, addStoreActions, false);
}

void NFA::handleString(const AST::RuleMember &member, const std::string &str, const std::size_t &start, const std::size_t &end, bool isLastMember, bool addStoreActions) {
    if (addStoreActions)
        cst_node_close_propagate.push_back(end);

    std::size_t body_start = states.size();
    states.emplace_back();
    std::size_t current    = body_start;

    for (std::size_t i = 0; i < str.size(); ++i) {
        std::size_t next = states.size();
        states.emplace_back();
        states[current].transitions[str[i]] = {{next}};
        current = next;
    }
    std::size_t body_end = current;

    applyQuantifierAndActions(member, start, end, {body_start, body_end}, isLastMember, addStoreActions, false);
}

void NFA::handleCsequence(const AST::RuleMember &member, const AST::RuleMemberCsequence &csequence, const std::size_t &start, const std::size_t &end, bool isLastMember, bool addStoreActions) {
    if (addStoreActions)
        cst_node_close_propagate.push_back(end);

    std::size_t body_start = states.size();
    states.emplace_back();
    std::size_t body_end   = states.size();
    states.emplace_back();

    const auto &chars = csequence.characters;
    const auto &escaped = csequence.escaped;

    if (csequence.negative) {
        constexpr auto max = std::numeric_limits<unsigned char>::max();
        std::bitset<max + 1> prohibited;
        for (char c : chars) prohibited.set(static_cast<unsigned char>(c));
        for (char c : escaped) prohibited.set(static_cast<unsigned char>(corelib::text::getEscapedFromChar(c)));
        for (auto [from, to] : csequence.diapasons) {
            for (char c = from; c <= to; ++c)
                prohibited.set(static_cast<unsigned char>(c));
        }
        for (unsigned char c = std::numeric_limits<unsigned char>::min();; ++c) {
            if (!prohibited.test(c)) {
                states[body_start].transitions[static_cast<char>(c)] = {{body_end}};
            }
            if (c == max)
                break;
        }
    } else {
        for (char c : chars) {
            states[body_start].transitions[c] = {{body_end}};
        }
        for (char c : escaped) {
            char ec = corelib::text::getEscapedFromChar(c);
            states[body_start].transitions[ec] = {{body_end}};
        }
        for (auto [from, to] : csequence.diapasons) {
            for (char c = from; c <= to; ++c) {
                states[body_start].transitions[c] = {{body_end}};
            }
        }
    }

    applyQuantifierAndActions(member, start, end, {body_start, body_end}, isLastMember, addStoreActions, false);
}

auto NFA::buildStateFragment(const AST::RuleMember &member, bool isLastMember, bool addStoreActions) -> StateRange {
    if (member.isNospace()) {
        no_add_space_skip_next = true;
        return {NULL_STATE, NULL_STATE};
    }
    bool is_repeting = member.quantifier == '+' || member.quantifier == '*';
    const std::size_t entry = states.size();
    states.emplace_back();
    const std::size_t start = states.size();
    states.emplace_back();
    const std::size_t end   = states.size();
    states.emplace_back();

    // Bridge the public entry unconditionally into the internal start state.
    states[entry].epsilon_transitions.insert({start});

    if (member.isName()) {
        const auto &name = member.getName();
        if ((is_char_table && tree.getTreeMap().contains(name.name)) || !name.isTerminal()) {
            auto it = fragment_cache.find(name.name);
            if (it != fragment_cache.end()) {
                /*
                 * ------------------------------------------------------------
                 * Cache hit: do NOT hand back the cached state ids directly.
                 *
                 * The cache exists so a name doesn't need its AST walked
                 * again, but the resulting NFA states must still be
                 * INDEPENDENT per occurrence. Returning the same ids for
                 * every reference means every repeated use of the same
                 * name within one production collapses onto the identical
                 * physical states. Two occurrences reached via different
                 * preceding action histories (e.g. a rule referencing the
                 * same terminal twice, such as `SYMBOL ... SYMBOL`) then
                 * converge epsilon-wise on one state carrying two
                 * incompatible action histories - exactly the ambiguity
                 * DFA::build() rejects with "NFA destination state has
                 * multiple epsilon paths with different action sequences".
                 *
                 * So: rebuild a fresh, rebased copy of the cached state
                 * range on every hit (mirrors the rebasing DFA::mergeTwoNFA
                 * does when merging separate NFAs), and wire/mark-accept
                 * against the copy instead of the original.
                 * ------------------------------------------------------------
                 */
                const std::size_t low  = it->second.start;
                const std::size_t high = fragment_cache_extent.at(name.name);
                const std::size_t dest_offset = states.size();

                const auto in_range = [&](std::size_t id) {
                    return id != NULL_STATE && id >= low && id <= high;
                };
                const auto rebase = [&](std::size_t id) {
                    return in_range(id) ? (id - low + dest_offset) : id;
                };

                for (std::size_t src = low; src <= high; ++src) {
                    NFA::state new_state = states[src];

                    for (auto &[symbol, targets] : new_state.transitions) {
                        for (auto &target : targets) {
                            target.next = rebase(target.next);
                        }
                    }

                    decltype(new_state.epsilon_transitions) rebased_epsilon;
                    for (auto target : new_state.epsilon_transitions) {
                        target.next = rebase(target.next);
                        rebased_epsilon.insert(target);
                    }
                    new_state.epsilon_transitions = std::move(rebased_epsilon);

                    for (auto &act_var : new_state.actions) {
                        std::visit([&](auto &act) {
                            using T = std::decay_t<decltype(act)>;
                            if constexpr (std::is_same_v<T, ActionState>) {
                                act.next_nfa_state = rebase(act.next_nfa_state);
                                if (std::holds_alternative<DFATarget>(act.next_state)) {
                                    auto &t = std::get<DFATarget>(act.next_state);
                                    t.id = rebase(t.id);
                                }
                            } else if constexpr (std::is_same_v<T, SemanticState>) {
                                act.nfa_index = rebase(act.nfa_index);
                                if (std::holds_alternative<DFATarget>(act.next_state)) {
                                    auto &t = std::get<DFATarget>(act.next_state);
                                    t.id = rebase(t.id);
                                }
                            }
                        }, act_var);
                    }

                    if (new_state.accept_binding.has_value()) {
                        auto &binding = *new_state.accept_binding;
                        if (binding.target_semantic_state.has_value()) {
                            *binding.target_semantic_state = rebase(*binding.target_semantic_state);
                        }
                    }

                    states.emplace_back(std::move(new_state));
                }

                const std::size_t copied_start = dest_offset;
                const std::size_t copied_end   = rebase(it->second.end);

                if (isLastMember) {
                    // Bridge copied end to a distinct new state before marking accept
                    std::size_t accept_state = states.size();
                    states.emplace_back();
                    states[copied_end].epsilon_transitions.insert({accept_state});
                    markAccept(accept_state, accept_state, member, true, is_repeting);
                    return {copied_start, accept_state};
                }
                return {copied_start, copied_end};
            }

            if (!processing.insert(name.name).second)
                return {NULL_STATE, NULL_STATE};

            handleNonTermnal(member, name.name, start, end, isLastMember, addStoreActions);

            processing.erase(name.name);
            fragment_cache[name.name] = {entry, end};
            fragment_cache_extent[name.name] = states.size() - 1;
        } else {
            handleTerminal(member, name.name, start, end, isLastMember, addStoreActions);
        }
    } else if (member.isOp()) {
        const auto &op = member.getOp();
        auto cached_no_space_skip = no_add_space_skip_next;
        auto cached_group_count = group_count;
        bool was_group = false;

        bool was_storing_group = store_entire_group;
        const bool group_has_prefix = !member.prefix.empty();
        if (group_has_prefix) {
            store_entire_group = true;
        }

        std::size_t body_start = states.size();
        states.emplace_back();
        std::size_t body_end = states.size();
        states.emplace_back();

        bool any_option_has_prefix = false;

        for (const auto &option_ptr : op.options) {
            no_add_space_skip_next = cached_no_space_skip;
            group_count = cached_group_count;
            if (option_ptr->isGroup())
                was_group = true;
            auto fragment = buildStateFragment(*option_ptr, false, addStoreActions);
            if (fragment.invalid())
                continue;

            states[body_start].epsilon_transitions.insert({fragment.start});
            states[fragment.end].epsilon_transitions.insert({body_end});

            if (!option_ptr->prefix.empty()) {
                any_option_has_prefix = true;
                if (isLastMember && !group_has_prefix) {
                    markAccept(fragment.end, body_end, *option_ptr, false, is_repeting);
                }
            }
        }

        store_entire_group = was_storing_group;

        if (was_group && cached_group_count == group_count) {
            group_count++;
        }

        const bool suppress_default_accept = !group_has_prefix && any_option_has_prefix;
        applyQuantifierAndActions(
            member, start, end, {body_start, body_end},
            isLastMember && !suppress_default_accept,
            addStoreActions, false
        );
    } else if (member.isGroup()) {
        handleGroup(member, member.getGroup().values, start, end, isLastMember, addStoreActions);
    } else if (member.isString()) {
        handleString(member, member.getString().value, start, end, isLastMember, addStoreActions);
    } else if (member.isCsequence()) {
        handleCsequence(member, member.getCsequence(), start, end, isLastMember, addStoreActions);
    } else if (member.isAny()) {
        for (unsigned char c = std::numeric_limits<unsigned char>::min(); c != std::numeric_limits<unsigned char>::max(); c++) {
            states[start].transitions[static_cast<char>(c)] = {{end}};
        }
        states[start].transitions[static_cast<char>(std::numeric_limits<unsigned char>::max())] = {{end}};
    } else {
        std::visit([](auto &m) {
            throw Error("Undefined member: {}", typeid(m).name());
        }, member.value);
    }

    if (!no_add_space_skip_next)
        add_space_skip_places.push_back(entry);
    no_add_space_skip_next = false;
    return {entry, end};
}

void NFA::build(bool addStoreActions) {
    if (isWhitespaceToken)
        addStoreActions = false;

    std::size_t last_state = NULL_STATE;
    std::size_t prev_end = NULL_STATE;

    if (rules != nullptr && !rules->empty()) {
        for (std::size_t i = 0; i < rules->size(); ++i) {
            bool is_last = (i == rules->size() - 1);
            current_member = (*rules)[i].get();
            auto [start, end] = buildStateFragment(*(*rules)[i], is_last, addStoreActions);

            if (prev_end != NULL_STATE && start != NULL_STATE) {
                // Connect previous member's exit state directly to current member's start state
                states[prev_end].epsilon_transitions.insert({start});
            }

            if (end != NULL_STATE) {
                prev_end = end;
            }

            if (is_last) {
                last_state = end;
            }
        }
        if (states.empty()) {
            throw Error("NFA/LR table cannot be empty");
        }
    } else if (member != nullptr) {
        current_member = member;
        last_state = buildStateFragment(*member, true, addStoreActions).end;
    } else {
        throw Error("NFA rules/member cannot be null");
    }

    if (addStoreActions) {
        if (dtb == nullptr) {
            nfadtb = std::monostate {};
        } else if (dtb->isTemplatedDataBlock()) {
            TemplatedDataBlock templated_data_block;
            std::size_t prefix_index = 0;
            std::size_t index = 0;
            std::size_t group_index = 0;
            generateTemplatedDataBlockFromRules(*rules, templated_data_block, prefix_index, index, group_index);
            nfadtb = templated_data_block;
        } else if (dtb->isRegularDataBlock()) {
            TemplatedDataBlockValue single_value_data_block;
            bool isAlreadyConstructed = false;
            generateSingleDataBlockFromRules(*rules, single_value_data_block, isAlreadyConstructed);
            nfadtb = single_value_data_block;
        } else {
            nfadtb = std::monostate {};
        }
    }

    if (!isWhitespaceToken) {
        addSpaceSkip();
    }
    buildAcceptMap();
    (*accept_index)++;
}

void NFA::getStatesToPropagate(std::size_t state_id, std::unordered_set<std::size_t> &result) {
    const auto &state = states[state_id];
    if (state.transitions.empty() && state.epsilon_transitions.empty())
        return;
    result.insert(state_id);
    for (const auto &epsilon : state.epsilon_transitions) {
        if (result.contains(epsilon.next))
            continue;
        getStatesToPropagate(epsilon.next, result);
    }
}

auto NFA::getStatesToPropagate(std::size_t id) -> std::unordered_set<std::size_t> {
    std::unordered_set<std::size_t> result;
    getStatesToPropagate(id, result);
    return result;
}

auto NFA::investigateHasNext(std::size_t place, char c, std::unordered_set<std::size_t> &visited) -> bool {
    for (const auto &[name, next] : states[place].transitions) {
        if (std::holds_alternative<char>(name)) {
            auto this_c = std::get<char>(name);
            if (this_c == c) {
                return true;
            }
        }
    }
    const auto &e_transitios = states[place].epsilon_transitions;
    return std::ranges::any_of(e_transitios.begin(), e_transitios.end(), [&](const auto &x) {
        if (visited.contains(x.next))
            return false;
        visited.insert(x.next);
        return investigateHasNext(x.next, c, visited);
    });
}

auto NFA::investigateHasNext(std::size_t place, const stdu::vector<std::string> &name, std::unordered_set<std::size_t> &visited) -> bool {
    for (const auto &[n, next] : states[place].transitions) {
        if (std::holds_alternative<stdu::vector<std::string>>(n)) {
            const auto &this_c = std::get<stdu::vector<std::string>>(n);
            if (this_c == name) {
                return true;
            }
        }
    }
    const auto &e_transitios = states[place].epsilon_transitions;
    return std::ranges::any_of(e_transitios.begin(), e_transitios.end(), [&](const auto &x) {
        if (visited.contains(x.next))
            return false;
        visited.insert(x.next);
        return investigateHasNext(x.next, name, visited);
    });
}

void NFA::addSpaceSkip()
{
    for (const auto place : add_space_skip_places) {
        std::unordered_set<std::size_t> whitespace_chars;

        if (is_char_table) {
            for (const auto c : constants::whitespace_chars) {
                whitespace_chars.insert(
                    static_cast<unsigned char>(c)
                );
            }
        }

        // Only whitespace which cannot be consumed normally at `place`
        // may be treated as ignorable whitespace.
        std::unordered_set<std::size_t> skip_chars;

        for (const auto c : whitespace_chars) {
            std::unordered_set<std::size_t> visited;

            if (!investigateHasNext(
                    place,
                    static_cast<char>(c),
                    visited))
            {
                skip_chars.insert(c);
            }
        }

        if (skip_chars.empty())
            continue;

        /*
         * IMPORTANT:
         *
         * Do NOT copy the transitions reachable through epsilon
         * closure here.
         *
         * Doing that flattens:
         *
         *     place -> BEGIN -> body
         *
         * into:
         *
         *     skip -> body
         *
         * which causes BEGIN to execute after the next character
         * has already been consumed.
         *
         * Instead, the skip state consumes whitespace and then
         * epsilon-transitions back to `place`.
         *
         * Therefore the normal epsilon closure is re-entered:
         *
         *     whitespace
         *          |
         *          v
         *       skip_state
         *          |
         *          ε
         *          v
         *        place
         *          |
         *          ε
         *          v
         *        BEGIN
         *          |
         *          ε
         *          v
         *        body
         *
         * Thus BEGIN still happens before the next input character.
         */

        const std::size_t skip_state_id = states.size();

        states.emplace_back();

        auto &skip_state = states[skip_state_id];

        // Consume another whitespace character.
        for (const auto c : skip_chars) {
            skip_state.transitions[static_cast<char>(c)] = {
                {skip_state_id}
            };
        }

        /*
         * After consuming whitespace, return to the original
         * position in the NFA.
         *
         * This is the critical part of the fix.
         */
        skip_state.epsilon_transitions.insert({place});

        /*
         * Redirect whitespace from the original place into the
         * whitespace loop.
         *
         * Non-whitespace transitions remain completely untouched.
         */
        auto &state = states[place];

        for (const auto c : skip_chars) {
            state.transitions[static_cast<char>(c)] = {
                {skip_state_id}
            };
        }
    }
}
void NFA::acceptMapVisitState(std::size_t index, std::optional<TokenBinding> current_binding, std::unordered_set<std::size_t>& visited) {
    if (!visited.insert(index).second)
        return;

    if (states[index].accept_binding.has_value()) {
        current_binding = states[index].accept_binding;
    }

    if (current_binding.has_value()) {
        accept_map[index] = current_binding.value();
    }

    for (const auto &e : states[index].epsilon_transitions) {
        if (e.next != NULL_STATE) {
            acceptMapVisitState(e.next, current_binding, visited);
        }
    }
}

void NFA::buildAcceptMap() {
    accept_map.clear();
    for (std::size_t i = 0; i < states.size(); ++i) {
        if (states[i].accept_binding.has_value()) {
            std::unordered_set<std::size_t> local_visited;
            acceptMapVisitState(i, states[i].accept_binding, local_visited);
        }
    }
}

void NFA::generateTemplatedDataBlockFromSingleRule(const AST::RuleMember &mem, TemplatedDataBlock &templated_data_block, std::size_t &prefix_index, std::size_t &index, std::size_t &group_index) {
    if (!mem.prefix.empty()) {
        if (dtb->getTemplatedDataBlock().names.size() <= prefix_index) {
            return;
        }
        const auto &name = dtb->getTemplatedDataBlock().names[prefix_index++];
        if (mem.isGroup()) {
            templated_data_block.emplace(name, TemplatedDataBlockValue {.type = StoreCstNode::CST_GROUP, .cst_index = group_index++, .AST = &mem});
        } else if (mem.isOp()) {
            templated_data_block.emplace(name, TemplatedDataBlockValue {.type = StoreCstNode::CST_CONDITION, .cst_index = index++, .AST = &mem});
        } else {
            templated_data_block.emplace(name, TemplatedDataBlockValue {.type = StoreCstNode::CST_NODE, .cst_index = index++, .AST = &mem});
        }
    }

    if (mem.isGroup()) {
        generateTemplatedDataBlockFromRules(mem.getGroup().values, templated_data_block, prefix_index, index, group_index);
    } else if (mem.isOp()) {
        std::size_t start_prefix_index = prefix_index;
        std::size_t max_prefix_index = prefix_index;
        std::size_t start_index = index;
        std::size_t max_index = index;
        std::size_t start_group_index = group_index;
        std::size_t max_group_index = group_index;

        for (const auto &opt_ptr : mem.getOp().options) {
            std::size_t current_prefix_index = start_prefix_index;
            std::size_t current_index = start_index;
            std::size_t current_group_index = start_group_index;

            generateTemplatedDataBlockFromSingleRule(*opt_ptr, templated_data_block, current_prefix_index, current_index, current_group_index);

            max_prefix_index = std::max(max_prefix_index, current_prefix_index);
            max_index = std::max(max_index, current_index);
            max_group_index = std::max(max_group_index, current_group_index);
        }
        prefix_index = max_prefix_index;
        index = max_index;
        group_index = max_group_index;
    }
}

void NFA::generateTemplatedDataBlockFromRules(const stdu::vector<std::shared_ptr<AST::RuleMember>> &rules, TemplatedDataBlock &templated_data_block, std::size_t &prefix_index, std::size_t &index, std::size_t &group_index) {
    for (const auto &mem_ptr : rules) {
        generateTemplatedDataBlockFromSingleRule(*mem_ptr, templated_data_block, prefix_index, index, group_index);
    }
}

void NFA::generateSingleDataBlockFromRules(const stdu::vector<std::shared_ptr<AST::RuleMember>> &rules, TemplatedDataBlockValue &single_data_block, bool &isAlreadyConstructed) {
    if (isAlreadyConstructed)
        return;
    for (const auto &mem_ptr : rules) {
        auto &mem = *mem_ptr;
        if (!mem.prefix.empty()) {
            single_data_block = {
                .type = mem.isGroup() ? StoreCstNode::CST_GROUP : mem.isOp() ? StoreCstNode::CST_CONDITION : StoreCstNode::CST_NODE,
                .cst_index = 0,
                .AST = &mem,
            };
            isAlreadyConstructed = true;
            return;
        }
        if (mem.isGroup()) {
            generateSingleDataBlockFromRules(mem.getGroup().values, single_data_block, isAlreadyConstructed);
            if (isAlreadyConstructed)
                return;
        } else if (mem.isOp()) {
            generateSingleDataBlockFromRules(mem.getOp().options, single_data_block, isAlreadyConstructed);
            if (isAlreadyConstructed)
                return;
        }
    };
}

std::ostream& operator<<(std::ostream& os, const NFA::state& s) {
    if (s.transitions.empty()) {
        os << "\t(none)\n";
    } else {
        for (const auto& [key, targets] : s.transitions) {
            std::visit([&os, &targets](auto &key) {
                if constexpr (std::is_same_v<std::decay_t<decltype(key)>, char>) {
                    os << "\t '" << corelib::text::getEscapedAsStr(key, false) << "' [LR Goto] -> State ";
                } else {
                    os << "\t '" << key << "' [LR Goto] -> State ";
                }
                for (const auto &t : targets) {
                    os << t.next << " ";
                }
            }, key);
            os << '\n';
        }
    }

    os << "\te -> ";
    if (s.epsilon_transitions.empty()) {
        os << "(none)\n";
    } else {
        for (const auto &t : s.epsilon_transitions) {
            os << t.next << ", ";
        }
        os << "\n";
    }

    if (!s.actions.empty()) {
        os << "\tactions -> \n";
        for (const auto &a : s.actions) {
            std::visit([&os](auto &&arg) {
                os << "\t\t" << arg << '\n';
            }, a);
        }
    }

    if (s.accept_binding.has_value()) {
        os << "\n\taccept token_id -> " << s.accept_binding->token_id;
        if (s.accept_binding->is_unique_representation) {
            os << " [REDUCE: rule " << s.accept_binding->reduce_rule_id.value_or(0) << "]";
        }
        os << "\n";
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const NFA& nfa) {
    for (std::size_t i = 0; i < nfa.getStates().size(); ++i) {
        os << "State " << i << ":\n" << nfa.getStates()[i] << "\n";
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const NFA::ActionState& s) {
    switch (s.action) {
        case NFA::Action::UNDEF: os << "UNDEF"; break;
        case NFA::Action::BEGIN: os << "BEGIN"; break;
        case NFA::Action::END:   os << "END"; break;
        case NFA::Action::PUSH:  os << "PUSH"; break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const NFA::SemanticState& s) {
    os << s.statements << '\n' << s.instance_value;
    return os;
}