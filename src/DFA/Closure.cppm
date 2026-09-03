export module DFA.closure;

import NFA;

import hash;
import dstd;
import std;

export namespace DFA {

    struct FiredAction {
        // NFA state which owns the action.
        std::size_t owner;

        NFA::TableType table_type;
        std::size_t table_index;

        std::variant<
            NFA::ActionState,
            NFA::SemanticState
        > raw;

        auto operator==(const FiredAction &other) const -> bool = default;
        auto operator<(const FiredAction &other) const -> bool {
            if (owner != other.owner) {
                return owner < other.owner;
            }
            if (table_type != other.table_type) {
                return table_type < other.table_type;
            }
            if (table_index != other.table_index) {
                return table_index < other.table_index;
            }
            return raw < other.raw;
        }
    private:
        friend struct ::uhash;

        auto members() const {
            return std::tie(
                owner,
                table_type,
                table_index,
                raw
            );
        }
    };

    struct TransitionPath {
        std::size_t target;
        std::vector<FiredAction> actions;

        auto operator==(const TransitionPath &other) const -> bool = default;
    };

    class Closure {
        const NFA *nfa;

        // The actual epsilon-closure.
        stdu::vector<std::size_t> closure;

        /*
         * For each state in the closure, keep the ACTION PATHS by which
         * that state can be reached.
         *
         * Do NOT merge these paths into one action set.
         *
         * Example:
         *
         *     A -> BEGIN -> X
         *     A ------------> X
         *
         * X has two paths:
         *
         *     []
         *     [BEGIN]
         *
         * They are different semantic paths.
         */
        std::unordered_map<
            std::size_t,
            std::vector<std::vector<FiredAction>>
        > paths;

        void epsilonClosure(
            const stdu::vector<std::size_t> &source
        );
        void epsilonClosure(
            const std::vector<std::pair<std::size_t, std::vector<FiredAction>>> &seeded_source
        );
        void move(
            const stdu::vector<std::size_t> &src,
            const NFA::TransitionKey &sym
        );

        friend struct ::uhash;

        auto members() const {
            // Closure identity is ONLY the NFA state subset.
            return std::tie(closure);
        }

    public:
        Closure(
            const NFA *nfa,
            const stdu::vector<std::size_t> *current = nullptr
        );

        Closure(
            const NFA *nfa,
            const stdu::vector<std::size_t> &current
        );
        Closure(
            const NFA *nfa,
            const std::vector<std::pair<std::size_t, std::vector<FiredAction>>> &seeded_current
        );
        Closure(
            const NFA *nfa,
            const stdu::vector<std::size_t> &current,
            const NFA::TransitionKey &symbol
        );

        auto begin() { return closure.begin(); }
        auto end() { return closure.end(); }

        auto begin() const { return closure.begin(); }
        auto end() const { return closure.end(); }

        auto cbegin() const { return closure.cbegin(); }
        auto cend() const { return closure.cend(); }

        auto rbegin() const { return closure.rbegin(); }
        auto rend() const { return closure.rend(); }

        auto &get() { return closure; }
        const auto &get() const { return closure; }

        auto empty() const {
            return closure.empty();
        }

        auto contains(std::size_t state) const -> bool;
        auto getPaths() { return paths; }
        /*
         * All distinct epsilon paths reaching `state`.
         *
         * This is deliberately NOT a single merged action sequence.
         */
        auto getPathsForState(
            std::size_t state
        ) const -> const std::vector<std::vector<FiredAction>>&;

        /*
         * Return the unique action path reaching `state`.
         *
         * Throws if the NFA has genuinely conflicting action paths.
         *
         * This is useful when a transition requires exactly one
         * deterministic action sequence.
         */
        auto getUniquePathForState(
            std::size_t state
        ) const -> const std::vector<FiredAction>&;

        auto operator==(const Closure &other) const {
            return closure == other.closure;
        }
    };
}