module NFA.TNFA.Interpreter;
import cpuf.op;
namespace NFA::Interpreter {
    struct ExecutionThread {
        std::size_t state_id{0};
        std::size_t input_pos{0};
        std::unordered_map<std::string, std::size_t> active_captures;
        std::unordered_map<std::string, std::string> captured_vars;
        std::unordered_map<std::string, stdu::vector<std::string>> array_vars;
        std::optional<LangAPI::Inheritance> pending_instance;
    };
    TNFAInterpreterResult TNFAInterpreter::run(std::size_t start_state_id,
                                                std::string_view input) const {
        if (start_state_id >= states.size()) {
            return TNFAInterpreterResult{.matched = false};
        }

        std::vector<ExecutionThread> active_threads;
        active_threads.push_back(ExecutionThread{
            .state_id = start_state_id,
            .input_pos = 0,
        });

        // Match transition character keys ('.' for wildcard, or exact char match)
        auto match_transition_key = [](TNFA::TransitionKey key, std::string_view remaining) -> std::size_t {
            if (remaining.empty()) {
                return 0;
            }
            if (key == '.' || key == remaining[0]) {
                return 1;
            }
            return 0;
        };

        // Process action chains (BEGIN, END, PUSH, and Semantic state instantiations)
        auto process_actions = [&](ExecutionThread &thread, const TNFA::ActionChain &actions) {
            for (const auto &act : actions) {
                if (std::holds_alternative<TNFA::ActionState>(act)) {
                    const auto &a = std::get<TNFA::ActionState>(act);
                    if (a.action == TNFA::Action::BEGIN) {
                        thread.active_captures[a.variable] = thread.input_pos;
                    } else if (a.action == TNFA::Action::END) {
                        if (auto it = thread.active_captures.find(a.variable); it != thread.active_captures.end()) {
                            std::size_t start = it->second;
                            std::size_t len = (thread.input_pos >= start) ? (thread.input_pos - start) : 0;
                            thread.captured_vars[a.variable] = std::string(input.substr(start, len));
                        }
                    } else if (a.action == TNFA::Action::PUSH) {
                        if (auto it = thread.active_captures.find(a.variable); it != thread.active_captures.end()) {
                            std::size_t start = it->second;
                            std::size_t len = (thread.input_pos >= start) ? (thread.input_pos - start) : 0;
                            thread.array_vars[a.variable].push_back(std::string(input.substr(start, len)));
                        }
                    }
                } else if (std::holds_alternative<TNFA::SemanticState>(act)) {
                    const auto &sem = std::get<TNFA::SemanticState>(act);
                    LangAPI::Inheritance instance = sem.instance_value;

                    // Substitute captured variable values into arguments
                    for (auto &arg : instance.args) {
                        for (auto &expr_v : arg) {
                            if (expr_v.isRvalue()) {
                                auto &rv = expr_v.getRValue();
                                if (rv.isSymbol()) {
                                    auto s = rv.getSymbol();
                                    if (s.path.size() == 1 && std::holds_alternative<std::string>(s.path[0])) {
                                        std::string var_name = std::get<std::string>(s.path[0]);
                                        if (auto it = thread.captured_vars.find(var_name); it != thread.captured_vars.end()) {
                                            arg = LangAPI::Symbol::createExpression(LangAPI::Symbol{it->second});
                                        }
                                    }
                                }
                            }
                        }
                    }
                    thread.pending_instance = std::move(instance);
                }
            }
        };

        // Expand epsilon closure while preventing infinite loops on cyclical transitions
        auto expand_epsilons = [&](std::vector<ExecutionThread> threads) {
            std::vector<ExecutionThread> expanded;

            struct QueueItem {
                ExecutionThread thread;
                std::unordered_set<std::size_t> visited_states;
            };

            std::vector<QueueItem> work_queue;
            work_queue.reserve(threads.size());
            for (auto &t : threads) {
                std::unordered_set<std::size_t> visited{t.state_id};
                work_queue.push_back(QueueItem{std::move(t), std::move(visited)});
            }

            while (!work_queue.empty()) {
                auto [current, visited] = std::move(work_queue.back());
                work_queue.pop_back();

                expanded.push_back(current);

                if (current.state_id >= states.size()) continue;

                const auto &st = states[current.state_id];
                for (const auto &eps_edge : st.epsilon_transitions) {
                    if (eps_edge.next >= states.size() || visited.contains(eps_edge.next)) {
                        continue;
                    }

                    ExecutionThread next_thread = current;
                    next_thread.state_id = eps_edge.next;
                    process_actions(next_thread, eps_edge.actions);

                    auto next_visited = visited;
                    next_visited.insert(eps_edge.next);

                    work_queue.push_back(QueueItem{std::move(next_thread), std::move(next_visited)});
                }
            }
            return expanded;
        };

        std::optional<TNFAInterpreterResult> best_match;

        while (!active_threads.empty()) {
            active_threads = expand_epsilons(std::move(active_threads));
            std::vector<ExecutionThread> next_step_threads;

            for (auto &thread : active_threads) {
                if (thread.state_id >= states.size()) continue;

                const auto &st = states[thread.state_id];

                // Check for accept state bindings
                if (st.accept_binding.has_value()) {
                    TNFAInterpreterResult res;
                    res.matched = true;
                    res.consumed_length = thread.input_pos;
                    res.final_state = st.id;
                    res.matched_token = st.accept_binding;
                    res.captured_variables = thread.captured_vars;
                    res.array_captures = thread.array_vars;

                    if (thread.pending_instance.has_value()) {
                        res.constructed_instance = thread.pending_instance;
                    } else {
                        LangAPI::Inheritance default_inst;
                        std::string joined_name;
                        for (std::size_t i = 0; i < st.origin.token_name.size(); ++i) {
                            if (i > 0) joined_name += '.';
                            joined_name += st.origin.token_name[i];
                        }
                        default_inst.name = LangAPI::Symbol{joined_name};
                        res.constructed_instance = default_inst;
                    }

                    if (!best_match || res.consumed_length > best_match->consumed_length) {
                        best_match = res;
                    }
                }

                // Match transitions against remaining input
                if (thread.input_pos < input.length()) {
                    std::string_view remaining = input.substr(thread.input_pos);

                    for (const auto &[key, edges] : st.transitions) {
                        std::size_t match_len = match_transition_key(key, remaining);

                        if (match_len > 0) {
                            for (const auto &edge : edges) {
                                if (edge.next >= states.size()) continue;

                                ExecutionThread next_thread = thread;
                                next_thread.state_id = edge.next;
                                next_thread.input_pos += match_len;
                                process_actions(next_thread, edge.actions);
                                next_step_threads.push_back(next_thread);
                            }
                        }
                    }
                }
            }

            active_threads = std::move(next_step_threads);
        }

        return best_match.value_or(TNFAInterpreterResult{.matched = false});
    }
  std::ostream& operator<<(std::ostream &os, TNFAInterpreterResult &res) {
      os << "variables = " << res.captured_variables << '\n';
      os << "consumed_length = " << res.consumed_length << '\n';
      os << "matched = " << res.matched << '\n';
      os << "array_captures = " << res.array_captures << '\n';
      os << "instance = " << res.constructed_instance << '\n';
      os << "matched_token = " << (res.matched_token.has_value() ? std::to_string(res.matched_token.value().token_id) : "<none>") << '\n';
      os << "final_state = " << res.final_state << '\n';
      return os;
    }
}
