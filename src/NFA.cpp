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
    const bool is_repeating = (member.quantifier == '+' || member.quantifier == '*');

    std::size_t entry_state = body.start;
    std::size_t exit_state  = body.end;
    std::size_t end_action_idx = NULL_STATE;

    if (has_store) {
        auto r_begin = name_;
        auto r_end = name_;
        r_begin.push_back("r" + std::to_string(registers_count++));
        r_begin.push_back("begin");
        r_end.push_back("r" + std::to_string(registers_count++));
        r_end.push_back(is_repeating ? "push" : "end");

        const Action close_action = is_repeating ? Action::PUSH : Action::END;

        action_table.push_back(ActionState {
            .action = Action::BEGIN,
            .variable = LangAPI::Variable {.name = corelib::text::join(r_begin, "_")},
            .next_nfa_state = body.start
        });
        std::size_t begin_action_idx = action_table.size() - 1;

        action_table.push_back(ActionState {
            .action = close_action,
            .variable = LangAPI::Variable {.name = corelib::text::join(r_end, "_")},
            .next_nfa_state = end
        });
        end_action_idx = action_table.size() - 1;

        states[start].epsilon_transitions.insert({begin_action_idx, TableType::Action});
        states[body.end].epsilon_transitions.insert({end_action_idx, TableType::Action});

        entry_state = start;
        exit_state = body.end;

        value_types.push_back(LLIR::BuilderBase::deduceVarTypeByRuleMember(member));

    } else {
        states[start].epsilon_transitions.insert({body.start, TableType::DFA});
        states[body.end].epsilon_transitions.insert({end, TableType::DFA});
    }

    // Quantifier Routing ('?', '+', '*') using LR Table transitions
    std::size_t loop_target = is_repeating
                              ? (has_store ? entry_state : body.start)
                              : body.start;

    switch (member.quantifier) {
        case '?':
            states[start].epsilon_transitions.insert({end, TableType::DFA});
            break;
        case '+':
            if (has_store) {
                auto loop_entry = action_table[end_action_idx];
                loop_entry.next_nfa_state = loop_target;
                action_table.push_back(std::move(loop_entry));
                states[exit_state].epsilon_transitions.insert({action_table.size() - 1, TableType::Action});
            } else {
                states[exit_state].epsilon_transitions.insert({loop_target, TableType::DFA});
            }
            break;
        case '*':
            states[start].epsilon_transitions.insert({end, TableType::DFA});
            if (has_store) {
                auto loop_entry = action_table[end_action_idx];
                loop_entry.next_nfa_state = loop_target;
                action_table.push_back(std::move(loop_entry));
                states[exit_state].epsilon_transitions.insert({action_table.size() - 1, TableType::Action});
            } else {
                states[exit_state].epsilon_transitions.insert({loop_target, TableType::DFA});
            }
            break;
        default:
            break;
    }

    // Last Member / Accept Marking with Unique Representation Hook
    if (isLastMember) {
        markAccept(end, end, member, nestedReduction);
    }

    return {start, end};
}
void NFA::markAccept(
    std::size_t state_id,
    std::size_t next_state,
    const AST::RuleMember &member,
    bool nestedReduction
) {
    TokenBinding binding;

    binding.token_id = *accept_index;

    // FIX: Set true whenever we generate a semantic reduction,
    // or decouple is_unique_representation from whether target_semantic_state exists.
    binding.is_unique_representation = true;

    SemanticState state;
    if (!member.prefix.empty()) {
        if (dtb->isRegularDataBlock()) {
            const auto &data_block = dtb->getRegDataBlock();
            auto type = value_types[0];
            std::string array_name = (type.isValueType() && (type.getValueType() == LangAPI::ValueType::Array || type.getValueType() == LangAPI::ValueType::FixedSizeArray) ? "vec_values" : "values");
            LangAPI::StorageSymbol ss;
            ss = {LangAPI::StorageOffset {.offset = LangAPI::Int::createExpression(LangAPI::Int {.value = 0})}};
            ss.what = LangAPI::Symbol::createExpression(LangAPI::Symbol {array_name});
            state.statements.push_back(LangAPI::Variable {
                .name = "value",
                .type = type,
                .value = LangAPI::GetVariant::createExpression(LangAPI::GetVariant {.type = std::make_shared<LangAPI::Type>(type), .sym = LangAPI::StorageSymbol::createExpression(ss)})
            });
            ss.what = LangAPI::Symbol::createExpression(LangAPI::Symbol {array_name});
            ss.path = {LangAPI::ArrayMethodCall {.method = LangAPI::ArrayMethods::Pop}};
            state.statements.push_back(LangAPI::StorageSymbol::createStatement(ss));
            state.instance_value.name = LangAPI::Symbol {name_};
            state.instance_value.args.push_back(LangAPI::Symbol::createExpression(LangAPI::Symbol { "value" }));
        } else if (dtb->isTemplatedDataBlock()) {
            const auto &data_block = dtb->getTemplatedDataBlock();
            for (long long i = static_cast<long long>(data_block.names.size()) - 1; i >= 0; --i) {
                const auto &key = data_block.names[static_cast<std::size_t>(i)];
                const auto u_idx = static_cast<std::size_t>(i);
                LangAPI::Type type = (u_idx < value_types.size()) ? value_types[u_idx] : LangAPI::Type{};

                std::string array_name = (type.isValueType() &&
                    (type.getValueType() == LangAPI::ValueType::Array ||
                     type.getValueType() == LangAPI::ValueType::FixedSizeArray)) ? "vec_values" : "values";

                LangAPI::StorageSymbol ss;
                ss = {LangAPI::StorageOffset {.offset = LangAPI::Int::createExpression(LangAPI::Int {.value = 0})}};
                ss.what = LangAPI::Symbol::createExpression(LangAPI::Symbol {array_name});
                state.statements.push_back(LangAPI::Variable {
                    .name = key,
                    .type = type,
                    .value = LangAPI::GetVariant::createExpression(LangAPI::GetVariant {.type = std::make_shared<LangAPI::Type>(type), .sym = LangAPI::StorageSymbol::createExpression(ss)})
                });
                ss.what = LangAPI::Symbol::createExpression(LangAPI::Symbol {array_name});
                ss.path = {LangAPI::ArrayMethodCall {.method = LangAPI::ArrayMethods::Pop}};
                state.statements.push_back(LangAPI::StorageSymbol::createStatement(ss));
                state.instance_value.name = LangAPI::Symbol {name_};
                state.instance_value.args.push_back(LangAPI::Symbol::createExpression(LangAPI::Symbol { key }));
            }
        } else {
            state.instance_value = LangAPI::Inheritance {.name = name_};
        }
    } else {
        // Fallback for prefix-less tokens/rules
        state.instance_value = LangAPI::Inheritance {.name = name_};
    }

    std::reverse(state.instance_value.args.begin(), state.instance_value.args.end());
    state.next_state = DFATarget {nestedReduction ? next_state : NULL_STATE};

    semantic_table.push_back(state);
    const std::size_t reduce_idx = semantic_table.size() - 1;

    binding.reduce_rule_id = *accept_index;
    binding.target_semantic_state = reduce_idx;

    // Assigned for ALL accepting rules
    states[state_id].accept_binding = binding;
}
void NFA::handleTerminal(const AST::RuleMember &member, const stdu::vector<std::string> &name, const std::size_t &start, const std::size_t &end, bool &isLastMember, bool addStoreActions) {
    if (addStoreActions && !member.prefix.empty()) {
        cst_node_close_propagate.push_back(end);
    }

    std::size_t body_start = states.size();
    states.emplace_back();
    std::size_t body_end   = states.size();
    states.emplace_back();
    states[body_start].transitions[name] = {{body_end, TableType::DFA}};

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
        states[last].epsilon_transitions.insert({fragment.start, TableType::DFA});
        last = fragment.end;
    }
    states[last].epsilon_transitions.insert({body_end, TableType::DFA});

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

        states[body_end].epsilon_transitions.insert({fragment.start, TableType::DFA});
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
        states[current].transitions[str[i]] = {{next, TableType::DFA}};
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
                states[body_start].transitions[static_cast<char>(c)] = {{body_end, TableType::DFA}};
            }
            if (c == max)
                break;
        }
    } else {
        for (char c : chars) {
            states[body_start].transitions[c] = {{body_end, TableType::DFA}};
        }
        for (char c : escaped) {
            char ec = corelib::text::getEscapedFromChar(c);
            states[body_start].transitions[ec] = {{body_end, TableType::DFA}};
        }
        for (auto [from, to] : csequence.diapasons) {
            for (char c = from; c <= to; ++c) {
                states[body_start].transitions[c] = {{body_end, TableType::DFA}};
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

    const std::size_t entry = states.size();
    states.emplace_back();
    const std::size_t start = states.size();
    states.emplace_back();
    const std::size_t end   = states.size();
    states.emplace_back();

    // Bridge the public entry unconditionally into the internal start state.
    states[entry].epsilon_transitions.insert({start, TableType::DFA});

    if (member.isName()) {
        const auto &name = member.getName();
        if ((is_char_table && tree.getTreeMap().contains(name.name)) || !name.isTerminal()) {
            auto it = fragment_cache.find(name.name);
            if (it != fragment_cache.end()) {
                if (isLastMember) {
                    markAccept(it->second.end, it->second.end, member, true);
                }
                return {it->second.start, it->second.end};
            }

            if (!processing.insert(name.name).second)
                return {NULL_STATE, NULL_STATE};

            handleNonTermnal(member, name.name, start, end, isLastMember, addStoreActions);

            processing.erase(name.name);
            fragment_cache[name.name] = {entry, end};
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

            states[body_start].epsilon_transitions.insert({fragment.start, TableType::DFA});
            states[fragment.end].epsilon_transitions.insert({body_end, TableType::DFA});

            if (!option_ptr->prefix.empty()) {
                any_option_has_prefix = true;
                if (isLastMember && !group_has_prefix) {
                    markAccept(fragment.end, body_end, *option_ptr, false);
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
            states[start].transitions[static_cast<char>(c)] = {{end, TableType::DFA}};
        }
        states[start].transitions[static_cast<char>(std::numeric_limits<unsigned char>::max())] = {{end, TableType::DFA}};
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
                states[prev_end].epsilon_transitions.insert({start, TableType::DFA});
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
        if (last_state != NULL_STATE && last_state < states.size()) {
            states[last_state].rule_name = name_;
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

void NFA::addSpaceSkip() {
    /*
     * Do not loop whitespace directly back to `place` when its epsilon
     * closure contains an Action edge.  Such a loop re-enters the closure
     * and executes BEGIN again.
     *
     * Instead, construct an action-free projection of the epsilon closure:
     *
     *     place --whitespace--> skip_state --whitespace--> skip_state
     *
     * skip_state contains the consuming transitions reachable through normal
     * DFA epsilon edges, but deliberately excludes Action/Semantic edges.
     * Thus skipped whitespace can never re-fire BEGIN, while the next real
     * character follows the same content transitions as the original entry.
     */
    for (const auto place : add_space_skip_places) {
        std::unordered_set<std::size_t> whitespace_chars;

        if (is_char_table) {
            for (const auto c : constants::whitespace_chars)
                whitespace_chars.insert(static_cast<unsigned char>(c));
        }

        // A whitespace character that is already a valid content transition
        // must not be converted into a skip transition.
        std::unordered_set<std::size_t> skip_chars;
        for (const auto c : whitespace_chars) {
            std::unordered_set<std::size_t> visited;
            if (!investigateHasNext(place, static_cast<char>(c), visited))
                skip_chars.insert(c);
        }

        if (skip_chars.empty())
            continue;

        NFA::state skip_state;
        std::unordered_set<std::size_t> visited;

        std::function<void(std::size_t)> collect =
            [&](std::size_t id) {
                if (!visited.insert(id).second)
                    return;

                const auto &source = states[id];

                // Preserve all consuming transitions.
                for (const auto &[key, targets] : source.transitions) {
                    auto &dst = skip_state.transitions[key];
                    dst.insert(dst.end(), targets.begin(), targets.end());
                }

                // Traverse only ordinary DFA epsilon edges.  Action and
                // Semantic edges are intentionally excluded.
                for (const auto &epsilon : source.epsilon_transitions) {
                    if (epsilon.table_type == TableType::DFA)
                        collect(epsilon.next);
                }
            };

        collect(place);

        const std::size_t skip_state_id = states.size();
        states.emplace_back(std::move(skip_state));

        auto &skip = states[skip_state_id];
        for (const auto c : skip_chars) {
            skip.transitions[static_cast<char>(c)] = {
                {skip_state_id, TableType::DFA}
            };
        }

        auto &state = states[place];
        for (const auto c : skip_chars) {
            state.transitions[static_cast<char>(c)] = {
                {skip_state_id, TableType::DFA}
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
        std::size_t target = e.next;

        if (e.table_type == TableType::Action) {
            target = action_table.at(e.next).next_nfa_state;
        } else if (e.table_type == TableType::Semantic) {
            continue;
        }

        if (target == NULL_STATE)
            continue;

        acceptMapVisitState(target, current_binding, visited);
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
            switch (t.table_type) {
                case NFA::TableType::Action:
                    os << "action_table[" << t.next << "], ";
                    break;
                case NFA::TableType::Semantic:
                    os << "semantic_table[" << t.next << "], ";
                    break;
                default:
                    os << t.next << ", ";
                    break;
            }
        }
        os << "\n";
    }

    if (s.accept_binding.has_value()) {
        os << "\n\taccept token_id -> " << s.accept_binding->token_id;
        if (s.accept_binding->is_unique_representation) {
            os << " [REDUCE: semantic_table[" << s.accept_binding->target_semantic_state.value_or(0) << "]"
               << ", rule " << s.accept_binding->reduce_rule_id.value_or(0) << "]";
        }
        os << "\n";
    }

    if (!s.rule_name.empty()) {
        os << "\tdata: \n";
        os << "\t\t[name]: " << s.rule_name << "\n";
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const NFA& nfa) {
    for (std::size_t i = 0; i < nfa.getStates().size(); ++i) {
        os << "State " << i << ":\n" << nfa.getStates()[i] << "\n";
    }
    return os;
}