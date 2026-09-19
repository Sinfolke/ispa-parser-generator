export module DFA.API;

import NFA.IR.API;
import NFA.TNFA.API;
import AST.Tree;
import hash;
import logging;
import cpuf.op;
import dstd;
import std;

// Bridge: every DFA source file still spells the NFA types as `NFA::Foo`
// (TransitionKey, ActionState, DFATarget, ...). Those now live in
// NFA::TNFA rather than the old flat NFA namespace. Rather than touch
// every call site's qualification, re-export the TNFA names into NFA::
// here so `NFA::TransitionKey` etc. keep resolving. This alias is the
// one thing that should disappear if DFA is ever ported to address
// NFA::TNFA::* directly.
export namespace NFA {
    using namespace NFA::TNFA;
}

export namespace DFA {
    inline constexpr auto NULL_STATE = std::numeric_limits<std::size_t>::max();
    struct TransitionValue {
        std::size_t next = NULL_STATE;
        stdu::vector<std::variant<NFA::ActionState, NFA::SemanticState>> actions;
        std::size_t accept_index = NULL_STATE;
        // Every NFA::TNFA edge that was folded into this DFA transition
        // (subset construction can merge several source-grammar sites onto
        // one (symbol, target) pair) - kept so a debugger/emitter can walk
        // back to every contributing grammar location. Not part of
        // identity: see `members()` below.
        NFA::DebugOrigins debug;
        std::optional<NFA::CharOrigin> char_origin;
        bool operator==(const TransitionValue &other) const = default;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(next, accept_index);
        }
    };
    struct ActionSequence {
        NFA::ActionChain actions;
        std::size_t terminal_dfa_target;
        // Same provenance vector as TransitionValue::debug, carried through
        // the intermediate (action-bearing) representation so it survives
        // until optimizeRegistersAndLRTable()/optimizeSemanticTable() fold
        // this into a final ActionTarget/SemanticTarget (which carry their
        // own `.debug`).
        NFA::DebugOrigins debug;
        stdu::vector<std::optional<NFA::TNFA::CharOrigin>> char_origin;
        auto operator==(const ActionSequence &other) const -> bool = default;
        auto operator<(const ActionSequence &other) const -> bool {
            if (actions != other.actions) {
                return actions < other.actions;
            }
            return terminal_dfa_target < other.terminal_dfa_target;
        };
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(actions, terminal_dfa_target);
        }
    };
    struct TransitionKeyExt {
        NFA::TransitionKey symbol;

        NFA::TableType table_type;

        // For NFA::ActionTarget:
        //   actual action opcode
        //   target_partition = partition of the next DFA state
        //
        // For NFA::SemanticTarget:
        //   action_id identifies the semantic table entry
        //
        // For NFA::DFATarget:
        //   target_partition identifies the next DFA state.
        NFA::Action action;
        std::size_t action_id;
        std::size_t target_partition;

        bool operator<(const TransitionKeyExt &other) const {
            return std::tie(
                symbol,
                table_type,
                action,
                action_id,
                target_partition
            ) < std::tie(
                other.symbol,
                other.table_type,
                other.action,
                other.action_id,
                other.target_partition
            );
        }

        bool operator==(const TransitionKeyExt &other) const {
            return std::tie(
                symbol,
                table_type,
                action,
                action_id,
                target_partition
            ) == std::tie(
                other.symbol,
                other.table_type,
                other.action,
                other.action_id,
                other.target_partition
            );
        }

    private:
        friend struct ::uhash;

        auto members() const {
            return std::tie(
                symbol,
                table_type,
                action,
                action_id,
                target_partition
            );
        }
    };

    using RawAction = std::variant<NFA::ActionState, NFA::SemanticState>;
    using NextTarget = std::variant<NFA::DFATarget, ActionSequence>;
    using TransitionTarget = std::variant<NFA::DFATarget, NFA::ActionTarget, NFA::SemanticTarget>;
    using TransitionTargetWithActions = std::variant<NFA::DFATarget, ActionSequence>;
    using Transitions = utype::unordered_map<NFA::TransitionKey, TransitionTarget>;
    using TransitionsWithActions = utype::unordered_map<NFA::TransitionKey, TransitionTargetWithActions>;
    struct StateWithActions {
        std::unordered_set<std::size_t> nfa_states;
        TransitionsWithActions transitions;
        std::optional<ActionSequence> accept_action;
        std::optional<NFA::TokenBinding> accept_binding = std::nullopt;
        bool operator==(const StateWithActions &other) const = default;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(nfa_states, transitions);
        }
    };
    template<typename TransitionType = Transitions>
    struct State {
        std::unordered_set<std::size_t> nfa_states;
        TransitionType transitions;
        std::optional<ActionSequence> accept_action;
        std::optional<NFA::TokenBinding> accept_binding = std::nullopt;
        NFA::DebugOrigins debug;
        bool operator==(const State &other) const = default;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(nfa_states, transitions);
        }
    };
    // NOTE: under NFA_OLD, TransitionKey was std::variant<char,
    // stdu::vector<std::string>>, and grammar-priority ordering was
    // derived by re-synthesizing a fake AST::RuleMember from the key
    // itself (compareNameWithCharacter/compareNameWithName). Under the
    // new NFA::TNFA, TransitionKey is a plain std::string label, so that
    // trick no longer applies. Priority ordering now reads the real
    // AST::RuleMember straight off each transition's debug provenance
    // (TransitionValue::debug / NFA::SourceLink::member) instead of
    // reconstructing one from the key text. Falls back to lexicographic
    // string comparison when a side carries no provenance (e.g. a
    // synthetic/merged key with nothing attached yet).
    class Comparator {
        const AST::Tree &tree;
    public:
        Comparator(const AST::Tree &tree) : tree(tree) {}
        auto compareBySourceLink(const NFA::IR::TokenID &a, const NFA::IR::TokenID &b) const -> bool;
        auto operator()(const NFA::TransitionKey &a, const NFA::TransitionKey &b) const -> bool;
        auto operator()(const std::pair<NFA::TransitionKey, TransitionValue> &a, const std::pair<NFA::TransitionKey, TransitionValue> &b) const -> bool;
    };

    // Unified Transition Target for pure DFA runtime

    // Mapper for output code generation
    class StateOffsetMapper {
    public:
        StateOffsetMapper(std::size_t dfa_count, std::size_t lr_count, std::size_t action_count)
            : dfa_size_(dfa_count), lr_size_(lr_count), action_size_(action_count) {}

        [[nodiscard]] std::size_t resolve(const TransitionTarget& target) const {
            return std::visit([this](auto&& arg) -> std::size_t {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, NFA::DFATarget>) {
                    return arg.id;
                } else if constexpr (std::is_same_v<T, NFA::ActionTarget>) {
                    return dfa_size_ + arg.id;
                } else if constexpr (std::is_same_v<T, NFA::SemanticTarget>) {
                    return dfa_size_ + lr_size_ + arg.id;
                }
            }, target);
        }

    private:
        std::size_t dfa_size_;
        std::size_t lr_size_;
        std::size_t action_size_;
    };
    using FullCharTable = std::array<TransitionValue, std::numeric_limits<unsigned char>::max() + 1>;
    using SeenSymbol = utype::unordered_map<stdu::vector<std::size_t>, std::size_t>;
    using WalkedState = utype::unordered_map<std::size_t, std::size_t>;
    using DfaEmptyStateMap = std::unordered_map<std::size_t, std::size_t>;
    using DfaIndexToEmptyStateMap = std::unordered_map<std::size_t, std::size_t>;

    using StateSet = stdu::vector<std::size_t>;

    // One shared table: maps every possible input byte to its equivalence
    // class id. Computed once per DFA, used by every state's ClassTransitions.
    struct CharClassTable {
        std::array<std::size_t, 256> char_to_class;
        std::size_t num_classes = 0;
    };

    // Per-state transition array, indexed by class id instead of by raw
    // char. Size == CharClassTable::num_classes, not 256.
    using ClassTransitions = std::vector<TransitionTarget>;

    auto operator<<(std::ostream &os, const TransitionValue &value) -> std::ostream &;
    auto operator<<(std::ostream &os, const ActionSequence &sequence) -> std::ostream &;
    auto operator<<(std::ostream &os, const TransitionKeyExt &key) -> std::ostream &;
    auto operator<<(std::ostream &os, const StateWithActions &state) -> std::ostream &;
    template<typename TransitionType>
    auto operator<<(std::ostream &os, const State<TransitionType> &state) -> std::ostream &;
    auto operator<<(std::ostream &os, const CharClassTable &table) -> std::ostream &;

}
