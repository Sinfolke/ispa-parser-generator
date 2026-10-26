/**
 * ispastdlib/dfa_run.hpp - DFA::run (the TDFA interpreter)
 * Part of the split ISPA std lib; include <ispastdlib.hpp> for everything.
 */
#pragma once
#include <ispastdlib/ispa_debug.hpp>
#include <ispastdlib/dfa_api.hpp>
#include <ispastdlib/core.hpp>

namespace ISPA_STD {
    namespace DFA::detail {
        // Debug trace of one DFA step.
        template<std::size_t debug_size>
        inline void trace_step(
            const API::DFADebugTable<debug_size> &debug_array,
            const API::DebugIndex &debug_index,
            std::size_t state,
            std::size_t cls,
            char current_ch,
            const char *pos,
            bool immediate_action
        ) {
            const API::DFADebug *debug = nullptr;

            // 2. Compute 1D index using base offset + char class
            // 1. Safe lookup without mutating map or inserting null keys
            if (debug_size > 0) {
                if (auto state_it = debug_index.find(state); state_it != debug_index.end()) {
                    if (auto cls_it = state_it->second.find(cls); cls_it != state_it->second.end()) {
                        debug = &debug_array[cls_it->second];
                    }
                }
            }

            if (debug) {
                std::stringstream ss;
                ss << debug->token_name << ": ";
                std::cout << ss.str() << debug->member;
                if (debug->call != -1) {
                    std::cout << "$call" << debug->call;
                }
                if (debug->group != -1) {
                    std::cout << "$group" << debug->group;
                }
                std::cout << "; " << debug->offset + 1 << "/" << debug->length << ";\n";
                std::cout << std::string(ss.str().size() + debug->offset, ' ');

                const auto &mem = debug->member;
                if (mem.size() >= 2 && mem.front() == '[') {
                    bool escaped = false;

                    for (std::size_t i = 1; i < mem.size(); ++i) {
                        char c = mem[i];

                        if (escaped) {
                            escaped = false;
                            if (current_ch == c) {
                                std::cout << std::string(i, ' ') << "^\n";
                                break;
                            }
                            continue; // Skip further range/escape processing for this character
                        }

                        if (c == '\\') {
                            escaped = true;
                            continue; // Skip to next character after setting escape flag
                        }

                        if (current_ch == c) {
                            std::cout << std::string(i, ' ') << "^\n";
                            break;
                        }

                        // Check range e.g., a-z
                        if (c == '-' && i > 1 && (i + 1) < mem.size() && mem[i + 1] != ']') {
                            char range_begin = mem[i - 1];
                            char range_end = mem[i + 1];

                            if (current_ch >= range_begin && current_ch <= range_end) {
                                std::cout << std::string(i - 1, ' ') << "^ ^\n";
                                break;
                            }
                        }
                    }
                } else {
                    std::cout << "^\n";
                }
            } else {
                std::cout << "State " << state << " char '"
                          << *(immediate_action ? pos + 1 : pos)
                          << "'\n";
            }
        }
    }
namespace DFA {
    template<
        typename Token,
        typename SemanticFunc,
        std::size_t table_states,
        std::size_t table_classes,
        std::size_t action_table_states,
        std::size_t debug_size,
        std::size_t registers,
        std::size_t outputs
    >
    auto run(
        const char* &pos,
        long long start_pos,
        long long &scanning_line,
        const API::Table<table_states, table_classes> &table,
        const API::CharToClass &class_table,
        const API::LRTable<action_table_states> &action_table,
        SemanticFunc semantic,
        const API::DFADebugTable<debug_size> &debug_array,
        const API::DebugIndex &debug_index,
        API::TdfaLayout<registers, outputs>
    ) -> Token {
        static_assert(registers >= outputs, "output registers are registers 0 .. outputs-1");

        const char *const start = pos;
        const long long start_line = scanning_line;
        ispa::debug::MatchScope debug_scan("lexer scan",
            ispa::debug::enabled() ? (*pos ? std::string(1, *pos) : std::string("<end of input>")) : std::string{});

        std::array<std::ptrdiff_t, (registers > 0 ? registers : 1)> regs;
        regs.fill(API::unset);
        std::vector<API::HistoryNode> history;

        std::size_t state = 0;
        bool immediate_action = false;
        const char* last_pos = pos;
        std::size_t steps_without_progress = 0;
#ifdef ISPA_DEBUG_REGISTERS
        // Watch one register across the whole run — set this to whatever
        // register index you're chasing (12, per the TNFA.build trace for
        // ID's capture). Prints every SET/SET_NEXT/COPY/APPEND that touches
        // it, plus its value at every semantic dispatch, so you can see
        // exactly when (or whether) it gets written, and what it holds when
        // a semantic handler reads it.
        constexpr std::size_t WATCH_REG = 12;
        auto dump_regs = [&](const char *tag) {
            std::cerr << "[regtrace] " << tag << " state=" << state << " pos_off=" << (pos - start) << " regs=[";
            for (std::size_t i = 0; i < registers; ++i) {
                std::cerr << (regs[i] == API::unset ? std::string("_") : std::to_string(regs[i]));
                if (i + 1 < registers) std::cerr << ",";
            }
            std::cerr << "]\n";
        };
        dump_regs("start");
#endif

        while (state != API::null_state) {
            const char current_ch = *(immediate_action && *pos != '\0' ? pos + 1 : pos);
            if (debug_size > 1) { // because generator emits one <UNDEFINED> table
                detail::trace_step(debug_array, debug_index, state,
                                   class_table[static_cast<unsigned char>(current_ch)], current_ch, pos, immediate_action);
            }

            if (state < table.size()) {
                if (immediate_action) {
                    immediate_action = false;
                    if (*pos == '\n')
                        scanning_line++;
                    if (*pos != '\0')
                        ++pos;
                }
                const std::size_t next = table[state][class_table[static_cast<unsigned char>(*pos)]];
                if (next == API::null_state) {
#ifdef ISPA_DEBUG_REGISTERS
                    std::cerr << "[regtrace] DEAD END at state=" << state
                              << " char='" << *pos << "'\n";
                    dump_regs("dead-end");
#endif
                    break;
                }
                state = next;

                if (next < table.size() && *pos != '\0') {
                    if (*pos == '\n')
                        scanning_line++;
                    ++pos;
                } else {
                    immediate_action = true;
                }
            } else if (state < table.size() + action_table.size()) {
                const auto &row = action_table[state - table.size()];
                const auto action = static_cast<API::Action>(row[0]);
                const auto here = static_cast<std::ptrdiff_t>(pos - start);

                if (row[1] >= registers || (row[2] != API::null_state && row[2] >= registers))
                    throw std::runtime_error("DFA: register out of range");

#ifdef ISPA_DEBUG_REGISTERS
                const bool watching = (row[1] == WATCH_REG) ||
                                      (row[2] != API::null_state && row[2] == WATCH_REG);
                if (watching) {
                    std::cerr << "[regtrace] ACTION action=" << static_cast<int>(action)
                              << " dst=r" << row[1]
                              << " src=" << (row[2] == API::null_state ? "nil" : "r" + std::to_string(row[2]))
                              << " here=" << here << '\n';
                }
#endif

                switch (action) {
                    case API::Action::SET:
                    case API::Action::SET_NEXT:
                        regs[row[1]] = here + (action == API::Action::SET_NEXT ? 1 : 0);
                        break;
                    case API::Action::COPY:
                        regs[row[1]] = regs[row[2]];
                        break;
                    case API::Action::APPEND:
                    case API::Action::APPEND_NEXT:
                        history.push_back({
                            here + (action == API::Action::APPEND_NEXT ? 1 : 0),
                            row[2] == API::null_state ? API::unset : regs[row[2]]
                        });
                        regs[row[1]] = static_cast<std::ptrdiff_t>(history.size()) - 1;
                        break;
                    default:
                        throw std::runtime_error("DFA: unknown action; Report this error to github");
                }
#ifdef ISPA_DEBUG_REGISTERS
                if (watching) dump_regs("after-action");
#endif
                state = row[3];
            } else {
                const API::Captures captures(start, regs.data(), history.data(), start_line);
#ifdef ISPA_DEBUG_REGISTERS
                std::cerr << "[regtrace] SEMANTIC dispatch idx="
                          << (state - table.size() - action_table.size())
                          << " pos_off=" << (pos - start);
                dump_regs("pre-semantic");
#endif
                auto result = semantic(
                    state - table.size() - action_table.size(),
                    captures, start_pos, start, static_cast<long long>(pos - start), start_line
                );
                state = static_cast<std::size_t>(std::get<0>(result));
                if (state == API::lexer_delete_state) {
                    if (*pos != '\0') ++pos;
                    debug_scan.success();
                    return Token {};
                }
                if (state == API::null_state) {
                    debug_scan.success();
                    return std::get<1>(std::move(result));
                }
                if (pos == last_pos) {
                    if (++steps_without_progress > 100) {
                        throw IspaInternalExcept(
                            "DFA: possible zero-width transition cycle at state {}, offset {}", "Lexer TDFA generation", true, state, std::to_string(pos - start)
                        );
                    }
                } else {
                    last_pos = pos;
                    steps_without_progress = 0;
                }
            }
        }
        debug_scan.outcome("DFA found no accepting token");
        return Token {};
    }
} // namespace DFA
} // namespace ISPA_STD
