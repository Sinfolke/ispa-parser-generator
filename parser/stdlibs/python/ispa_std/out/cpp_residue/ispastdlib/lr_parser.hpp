/**
 * ispastdlib/lr_parser.hpp - LRParser_base / ELRParser_base (needs ISPA_EXPERIMENTAL)
 * Part of the split ISPA std lib; include <ispastdlib.hpp> for everything.
 */
#pragma once
#include <ispastdlib/parser_base.hpp>

namespace ISPA_STD {
#ifdef ISPA_EXPERIMENTAL
    template <class TOKEN_T, class RULE_T, class MAIN_NODE, class Token, class Action, class ActionTable, class GotoTable, class RulesTable>
    class LRParser_base : public LLParser_base<TOKEN_T, RULE_T, MAIN_NODE, Token> {
    protected:
        std::vector<std::pair<std::variant<TOKEN_T, RULE_T>, std::size_t>> stack;
        template <class IT>
        void shift(IT& pos, std::size_t state) {
            stack.push_back({pos->name(), state});
            pos++;
        }
        void reduce(const std::size_t rules_id, const GotoTable &goto_table, const RulesTable rules_table) {
            const auto &rule_data = rules_table[rules_id];
            const auto &rule_name = rule_data.first;
            const auto &reduce_size = rule_data.second;
            if (stack.size() < reduce_size) {
                throw std::runtime_error("Stack underflow during reduce");
            }
            stack.erase(stack.end() - reduce_size, stack.end());
            printf("Reduce: goto_table[%d][%d]\n", (int) stack.back().second, (int) rule_name);
            // Perform the reduction
            const auto& goto_entry = goto_table[stack.back().second][static_cast<std::size_t>(rule_name)];
            if (!goto_entry.has_value()) {
                throw std::runtime_error("Invalid GOTO after reduction");
            }

            std::size_t next_state = goto_entry.value();
            stack.push_back({rule_name, next_state});
        }
        virtual std::string TokensToString(TOKEN_T token) = 0;
        virtual std::string RulesToString(RULE_T rule) = 0;
        template<class IT>
        void parseFromPos(IT& pos, const ActionTable &action_table, const GotoTable &goto_table, RulesTable rules_table) {
            stack.push_back({TOKEN_T::NONE, 0});
            while(true) {
                auto &current_state = stack.back().second;
                const auto &action = action_table[current_state][(std::size_t) pos->name()];
                printf("Token name: %s", TokensToString(pos->name()).c_str());
                if (pos->data().has_value()) {
                    printf("[%s]", std::any_cast<std::string>(pos->data()).c_str());
                }
                printf(", state: %zu\n", current_state);
                if (action.has_value()) {
                    auto& act = action.value();
                    printf("action: %d, next state: %zu\n", (int) act.type, act.state);
                    if (act.type == Action::SHIFT)
                        shift(pos, act.state);
                    else if (act.type == Action::REDUCE)
                        reduce(act.state, goto_table, rules_table);
                    else if (act.type == Action::ACCEPT)
                        break;
                    else
                        throw std::runtime_error("Error state");
                } else {
                    throw std::runtime_error(("Action is not defined. stack size: " + std::to_string(stack.size())).c_str());
                }
            }
            printf("Accepted. distance: %zu\n", pos.distance());
            stack.clear();
        }
    };
    template <class TOKEN_T, class RULE_T, class MAIN_NODE, class Token, class Action, class ActionTable, class GotoTable, class RulesTable, class DFATable>
    class ELRParser_base : public LRParser_base<TOKEN_T, RULE_T, MAIN_NODE, Token, Action, ActionTable, GotoTable, RulesTable> {
    private:
        // cache tokens because of lazy iterator which makes tokens on dereference
        std::deque<Node<TOKEN_T, Token>> dfa_token_cache;
    protected:
        template <class IT>
        void shift(IT& pos, std::size_t state) {
            if (dfa_token_cache.empty()) {
                printf("Pushing directly\n");
                this->stack.push_back({pos->name(), state});
                pos++;
            } else {
                printf("Pushing from DFA cache, next token: ");
                printf("%s", TokensToString(pos->name()).c_str());
                if (pos->data().has_value()) {
                    printf("[%s]", std::any_cast<std::string>(pos->data()).c_str());
                }
                printf("\n");
                this->stack.push_back({dfa_token_cache.front().name(), state});
                dfa_token_cache.pop_back();
            }
        }
        template<class IT>
        const std::optional<Action>& getAction(IT &pos, const ActionTable &action_table) {
            auto &current_state = this->stack.back().second;
            return dfa_token_cache.empty() ? action_table[current_state][(std::size_t) pos->name()] : action_table[current_state][(std::size_t) dfa_token_cache.front().name()];
        }
        template<class IT>
        const Action* resolveDFA(IT &pos, std::size_t dfa_index, const DFATable &dfa_table) {
            const Action* initial_action = nullptr;
            printf("Resolving conflict in DFA table\n");
            std::size_t current_dfa_length = dfa_token_cache.size();
            for (std::size_t offset = 0;; offset++) {
                if (offset >= current_dfa_length)
                    dfa_token_cache.push_back(*pos++);
                const auto &[action, table] = dfa_table[dfa_index];
                std::size_t i = 1;
                while(table[i].first != dfa_token_cache[offset].name() && table[i].second != 0) i++;
                const auto &go_state = table[i].second;
                if (initial_action == nullptr) {
                    initial_action = &action;
                }
                if (go_state == 0) {
                    if (table[0].second != 0) {
                        dfa_index = table[0].second;
                        continue;
                    }
                    if (action.type == Action::ERR) {
                        printf("Returning initial action %d, state %zu\n", (int) initial_action->type, initial_action->state);
                        return initial_action;
                    }
                    printf("returning action %d, state %zu\n", (int) action.type, action.state);
                    return &action;
                }
                dfa_index = go_state;

            }
        }
        template<class IT>
        void peformAction(IT &pos, Action act, GotoTable goto_table, RulesTable rules_table, DFATable dfa_table) {
            switch (act.type)
            {
            case Action::SHIFT:
                shift(pos, act.state);
                break;
            case Action::REDUCE:
                this->reduce(act.state, goto_table, rules_table);
                break;
            case Action::DFA_RESOLVE: {
                const auto resolved = resolveDFA(pos, act.state, dfa_table);
                if (!resolved) throw std::runtime_error("Unresolvable DFA lookahead");
                peformAction(pos, *resolved, goto_table, rules_table, dfa_table);
                break;
            }
            default:
                throw std::runtime_error("Error action");
            }
        }
        template<class IT>
        void parseFromPos(IT& pos, const ActionTable &action_table, const GotoTable &goto_table, RulesTable rules_table, DFATable dfa_table) {
            this->stack.push_back({TOKEN_T::NONE, 0});
            while(true) {
                auto &current_state = this->stack.back().second;
                const auto &action = getAction(pos, action_table);
                if (dfa_token_cache.empty()) {
                    printf("Token name: %s", TokensToString(pos->name()).c_str());
                    if (pos->data().has_value()) {
                        printf("[%s]", std::any_cast<std::string>(pos->data()).c_str());
                    }
                    printf(", state: %zu\n", current_state);
                } else {
                    printf("Token name: %s", TokensToString(dfa_token_cache.front().name()).c_str());
                    if (pos->data().has_value()) {
                        printf("[%s]", std::any_cast<std::string>(pos->data()).c_str());
                    }
                    printf(", state: %zu\n", current_state);
                }

                if (action.has_value()) {
                    auto& act = action.value();
                    printf("action: %d, next state: %zu\n", (int) act.type, act.state);
                    if (act.type == Action::ACCEPT)
                        break;
                    peformAction(pos, act, goto_table, rules_table, dfa_table);
                } else {
                    throw std::runtime_error(("Action is not defined. stack size: " + std::to_string(this->stack.size())).c_str());
                }
            }
            printf("Accepted. distance: %zu\n", pos.distance());
            // clear
            this->stack.clear();
            dfa_token_cache.clear();
        }
    };
#endif
} // namespace ISPA_STD
