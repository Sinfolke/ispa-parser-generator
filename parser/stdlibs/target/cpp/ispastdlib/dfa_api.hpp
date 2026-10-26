/**
 * ispastdlib/dfa_api.hpp - DFA::API tables, DFADebug, Captures, TdfaLayout
 * Part of the split ISPA std lib; include <ispastdlib.hpp> for everything.
 */
#pragma once
#include <ispastdlib/core.hpp>

namespace ISPA_STD {
    namespace DFA::API {
        struct DFADebug;
        inline auto null_state = std::numeric_limits<std::size_t>::max();
        inline auto lexer_delete_state = null_state - 1;
        // Same numbering as NFA::TNFA::Action, so the converter emits the value as is.
        enum class Action { UNDEF, SET, SET_NEXT, COPY, APPEND, APPEND_NEXT };
        template<std::size_t Classes>
        using State = std::array<std::size_t, Classes>;
        template<std::size_t States, std::size_t Classes>
        using Table = std::array<State<Classes>, States>;
        using CharToClass = std::array<std::size_t, 256>;
        // Row: {op, a, b, next_state}.
        //   SET / SET_NEXT        reg[a] = position (NEXT: one past the character being consumed)
        //   COPY                  reg[a] = reg[b]
        //   APPEND / APPEND_NEXT  reg[a] = append(reg[b], position)   (b == null_state: empty list)
        template<std::size_t States>
        using LRTable = std::array<State<4>, States>;
        template<std::size_t Size>
        using DFADebugTable = std::array<DFADebug, Size>;
        using DebugIndex = std::unordered_map<long long, std::unordered_map<long long, long long>>;

        /*
         * Can be emit by parser generator.
         * This turns the DFA walker from class -> next state into something can be reversed to actual grammar structures
        */
        struct DFADebug {
            std::string member;
            std::string token_name;
            long long position_in_token;
            long long call;
            long long group;

            long long rule_run = null_state;
            long long offset = 0;
            long long length = 1;
            char ch = '\0';

            auto operator<(const DFADebug &other) const {
                return std::tie(member, token_name, position_in_token, call, group, rule_run, offset, length, ch) < std::tie(other.member, other.token_name, other.position_in_token, other.call, other.group, other.rule_run, other.offset, other.length, other.ch);
            }
            auto operator==(const DFADebug &other) const{
                return std::tie(member, token_name, position_in_token, call, group, rule_run, offset, length, ch) == std::tie(other.member, other.token_name, other.position_in_token, other.call, other.group, other.rule_run, other.offset, other.length, other.ch);
            }
        };

        // Capture k owns registers 2k (begin, or the head of a list capture's history)
        // and 2k + 1 (end). A register that was never written is `unset`.
        inline constexpr std::ptrdiff_t unset = -1;

        struct HistoryNode {
            std::ptrdiff_t pos;  // offset from the start of the token
            std::ptrdiff_t prev; // previous node, `unset` ends the list
        };

        class Captures {
            const char *base_ = nullptr;
            const std::ptrdiff_t *out_ = nullptr;
            const HistoryNode *history_ = nullptr;
            long long first_line_ = 1;

            auto begin_off(std::size_t k) const { return out_[2 * k]; }
            auto end_off(std::size_t k) const { return out_[2 * k + 1]; }

        public:
            Captures() = default;
            Captures(const char *base, const std::ptrdiff_t *out, const HistoryNode *history, long long first_line)
                : base_(base), out_(out), history_(history), first_line_(first_line) {}

            // scalar capture: both boundaries were recorded
            auto is_set(std::size_t k) const -> bool { return begin_off(k) != unset && end_off(k) != unset; }
            auto begin(std::size_t k) const -> const char * { return base_ + begin_off(k); }
            auto end(std::size_t k) const -> const char * { return base_ + end_off(k); }
            auto view(std::size_t k) const -> std::string_view {
                return std::string_view(begin(k), static_cast<std::size_t>(end_off(k) - begin_off(k)));
            }
            auto text(std::size_t k) const -> std::string { return std::string(view(k)); }
            auto character(std::size_t k) const -> char { return *begin(k); }

            // list capture: one entry per iteration, in input order
            auto list_views(std::size_t k) const -> std::vector<std::string_view> {
                std::vector<std::ptrdiff_t> positions;
                for (auto h = begin_off(k); h != unset; h = history_[h].prev)
                    positions.push_back(history_[h].pos);
                std::reverse(positions.begin(), positions.end());
                std::vector<std::string_view> spans;
                for (std::size_t i = 0; i + 1 < positions.size(); i += 2)
                    spans.emplace_back(base_ + positions[i], static_cast<std::size_t>(positions[i + 1] - positions[i]));
                return spans;
            }
            template<typename T = std::string>
            auto list(std::size_t k) const -> std::vector<T> {
                std::vector<T> out;
                for (const auto v : list_views(k)) {
                    if constexpr (std::is_same_v<T, char>)
                        out.push_back(v.empty() ? '\0' : v.front());
                    else
                        out.push_back(T(v));
                }
                return out;
            }

            auto line(std::size_t k) const -> long long {
                return first_line_ + std::count(base_, begin(k), '\n');
            }
            auto column(std::size_t k) const -> long long {
                const char *b = begin(k);
                const char *line_start = b;
                while (line_start > base_ && line_start[-1] != '\n')
                    --line_start;
                return (b - line_start) + 1;
            }
        };

        template<std::size_t Registers, std::size_t Outputs>
        struct TdfaLayout {
            static constexpr std::size_t registers = Registers;
            static constexpr std::size_t outputs = Outputs;
        };
    }
} // namespace ISPA_STD
