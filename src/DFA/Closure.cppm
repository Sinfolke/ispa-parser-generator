export module DFA.closure;

import NFA.IR.API;
import NFA.TNFA.API;
import NFA.TNFA;
import hash;
import cpuf.op;
import dstd;
import std;

// Same bridge as DFA.API: keep every `NFA::Foo` spelling in this file and
// its implementation working against the new NFA::TNFA namespace.
export namespace NFA {
    using namespace NFA::TNFA;
}

export namespace DFA {

    // A single tag-firing occurrence: the BEGIN/END/PUSH/REDUCE owned by
    // one NFA state, encountered while walking a (now unique) epsilon
    // route through the closure. This is the direct analogue of a
    // Laurikari/TDFA "tag" -- a register-set operation attached to an
    // NFA transition. Nothing about this struct changes from the
    // backtracking version: what changes is that a Closure now keeps
    // AT MOST ONE of these paths per NFA state, chosen deterministically
    // by priority, rather than every path that happens to exist.
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

    using ActionPath = std::vector<FiredAction>;

    // One consuming NFA edge that is taken on a symbol, before its epsilon
    // closure is computed. `actions` are the actions of that consuming edge
    // only. Seeds are always returned in priority order.
    struct SymbolSeed {
        std::size_t priority = 0;
        std::size_t source = NFA::TNFA::NULL_STATE;
        std::size_t target = NFA::TNFA::NULL_STATE;
        ActionPath actions;
        const NFA::IR::TokenID *source_link = nullptr;
        std::optional<NFA::TNFA::CharOrigin> char_origin;
    };

    class Closure {
        const NFA::TNFA::TNFABuilder *nfa;

        // The actual epsilon-closure: the NFA states reachable from the
        // seed(s) purely via epsilon edges.
        stdu::vector<std::size_t> closure;
        std::set<std::size_t> sorted_unique_closure;
        ActionPath transition_actions;
        std::unordered_map<std::size_t,ActionPath> terminal_actions_for;
        /*
         * TDFA core.
         *
         * For each state in the closure, the SINGLE tag path by which
         * it is reached.
         *
         * Under a Thompson construction every pair of epsilon edges
         * leaving the same state already carries (or can carry) a
         * priority: which alternative is preferred, which side of a
         * loop is preferred, etc. epsilonClosure() below performs a
         * priority-ordered DFS and commits a state's tag path the
         * FIRST time that state is reached. Any later, lower-priority
         * route to the same state is simply discarded.
         *
         * That is the entire disambiguation mechanism. There is no
         * runtime component: by the time subset construction reads
         * this map, ambiguity has already been resolved. Two epsilon
         * routes to the same NFA state are never "genuinely different
         * and both valid" here -- one is preferred by construction,
         * exactly as a backtracking regex engine would try alternatives
         * in order and commit to the first match.
         */
        std::unordered_map<std::size_t, ActionPath> actions_for;

        // TDFA support: the order in which the priority DFS committed the
        // states, and which seed (index into the seeded list) reached each.
        std::vector<std::size_t> discovery;
        std::unordered_map<std::size_t, std::size_t> seed_of;

        void epsilonClosure(
            const stdu::vector<std::size_t> &source
        );
        void epsilonClosure(
            const std::vector<std::pair<std::size_t, ActionPath>> &seeded_source
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
            const NFA::TNFA::TNFABuilder *nfa,
            const stdu::vector<std::size_t> *current = nullptr
        );

        Closure(
            const NFA::TNFA::TNFABuilder *nfa,
            const stdu::vector<std::size_t> &current
        );

        // Seed several NFA states at once, each carrying a pre-existing
        // tag path (e.g. the tail of a path threaded across a
        // subroutine/nested-token call boundary). Seeds are listed in
        // PRIORITY ORDER: if two seeds name the same NFA state, the
        // earlier one wins -- same convention as everywhere else in
        // this codebase where insertion order encodes preference.
        //
        // This is no longer used to defer "divergent" action sequences
        // (see migration notes) -- there is nothing left to defer.
        Closure(
            const NFA::TNFA::TNFABuilder *nfa,
            const std::vector<std::pair<std::size_t, ActionPath>> &seeded_current
        );

        Closure(
            const NFA::TNFA::TNFABuilder *nfa,
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

        /*
         * The single, deterministic tag path by which `state` is
         * reached in this closure. Empty (not an error) if `state`
         * fires no actions on its winning route, or is not part of
         * the closure at all.
         *
         * There is deliberately no "getPathsForState" (plural) and no
         * throwing "getUniquePathForState" any more: under priority-
         * ordered construction there is exactly one path, always, so
         * an API shaped around "maybe several, maybe throw" no longer
         * matches reality.
         */
        auto getActionsForState(
            std::size_t state
        ) const -> const ActionPath &;

        auto operator==(const Closure &other) const {
            return closure == other.closure;
        }
        auto getTransitionActions() const -> const ActionPath &;

        // States in the order the priority DFS committed them.
        auto getDiscoveryOrder() const -> const std::vector<std::size_t> & { return discovery; }
        // Index (into the seeded list the closure was built from) of the seed
        // whose path reaches `state`.
        auto getSeedIndex(std::size_t state) const -> std::size_t { return seed_of.at(state); }

        // All consuming edges leaving `current` on `symbol`, priority ordered.
        static auto collectSeeds(
            const NFA::TNFA::TNFABuilder *nfa,
            const std::vector<std::size_t> &current,
            const NFA::TransitionKey &symbol
        ) -> std::vector<SymbolSeed>;
        auto getTerminalActionsForState(std::size_t state) const -> const ActionPath &;
    };
}
