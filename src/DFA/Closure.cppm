export module DFA.closure;

import NFA;

import hash;
import dstd;
import std;

export namespace DFA {
    struct FiredAction {
        NFA::TableType table_type;
        std::size_t table_index;
        auto operator==(const FiredAction &other) const -> bool = default;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(table_type, table_index);
        }
    };

    class Closure {
        const NFA &nfa;
        stdu::vector<std::size_t> closure;
        stdu::vector<FiredAction> fired;

        void epsilonClosure(const stdu::vector<std::size_t> &source);
        void move(const stdu::vector<std::size_t> &src, const NFA::TransitionKey &sym);

        friend struct ::uhash;
        auto members() const {
            // IMPORTANT: identity (hashing/equality) must be based on the
            // underlying NFA state set alone. `fired` is a side-channel journal
            // of which Action/Semantic epsilon edges were walked while
            // computing this closure -- an artifact of the traversal, not part
            // of what the closure *is*. Two closures that land on the same
            // state set via different action/semantic edges (e.g. successive
            // iterations of a `*`/`+` quantifier loop) are the SAME DFA state
            // and must collapse together in dfa_state_map/closure_cache;
            // including `fired` here breaks that and causes state explosion
            // for anything cyclic, fragmenting the action/semantic tables.
            return std::tie(closure);
        }

    public:
        Closure(const NFA &nfa, const stdu::vector<std::size_t> *current = nullptr);
        Closure(const NFA &nfa, const stdu::vector<std::size_t> &current);
        Closure(const NFA &nfa, const stdu::vector<std::size_t> &current, const NFA::TransitionKey &symbol);

        auto begin() { return closure.begin(); }
        auto end() { return closure.end(); }
        auto begin() const { return closure.begin(); }
        auto end() const { return closure.end(); }
        auto cbegin() const { return closure.cbegin(); }
        auto cend() const { return closure.cend(); }
        auto rbegin() const { return closure.rbegin(); }
        auto rend() const { return closure.rend(); }

        auto &get()       { return closure; }
        auto contains(std::size_t state) const -> bool;
        const auto &get() const { return closure; }
        auto empty() const { return closure.empty(); }

        auto &firedActions() const { return fired; }

        auto operator==(const Closure &other) const {
            return closure == other.closure;
        }
    };
}