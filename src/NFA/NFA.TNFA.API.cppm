export module NFA.TNFA.API;
import LangAPI;
import AST.API;
import NFA.IR.API;
import hash;
import dstd;
import std;

export namespace NFA::TNFA {
    inline constexpr auto NULL_STATE = std::numeric_limits<std::size_t>::max();
    inline constexpr std::size_t NESTED_REDUCE_ID_BASE = 1'000'000;
    enum class TableType { DFA, Action, Semantic };
    // SET .. APPEND_NEXT are the DFA's register operations (same numbering as the runtime's
    // API::Action); BEGIN/END/PUSH are the capture events the TNFA builder wires.
    enum class Action { UNDEF, SET, SET_NEXT, COPY, APPEND, APPEND_NEXT, BEGIN, END, PUSH };
    enum class SemanticAction { UNDEF, REDUCE };

    // A literal rule member that matches more than one character (today,
    // only `String`) is unrolled into one state/edge per character rather
    // than one state/edge for the whole literal - that's what makes this a
    // genuine character-level TNFA. CharOrigin is the per-character debug
    // tag that makes that unrolling reversible for a human: every state and
    // edge born from unrolling the same literal site shares `rule_run`, and
    // `offset`/`length` say where in that literal this particular character
    // sits. A debugger walks a chain of states with the same `rule_run` in
    // increasing `offset` order and displays it as the one rule it came
    // from, instead of one node per character.
    struct CharOrigin {
        IR::TokenID source;
        std::size_t rule_run = NULL_STATE; // shared by every char unrolled from the same literal site
        std::size_t offset = 0;            // index of this character within that literal (0-based)
        std::size_t length = 1;            // total number of characters in that literal
        char ch = '\0';                    // the literal character this state/edge represents
        auto operator==(const CharOrigin &other) const -> bool = default;
        // Ordering intentionally mirrors members() (the identity fields);
        // `source` is a debug back-pointer and is deliberately excluded so
        // ordering stays stable/independent of pointer addresses.
        auto operator<(const CharOrigin &other) const -> bool {
            return std::tie(rule_run, offset, length, ch) < std::tie(other.rule_run, other.offset, other.length, other.ch);
        }

    private:
        friend struct ::uhash;
        auto members() const { return std::tie(rule_run, offset, length, ch); }
    };

    // Debug provenance is a *vector* on every index-based target because
    // subset construction/minimization routinely merges several distinct
    // NFA edges - each with its own grammar site - into one DFA-index
    // transition. The index remains the runtime identity; this vector
    // exists purely so a debugger/emitter can walk back to every source
    // grammar location that contributed to it. It intentionally does NOT
    // participate in `members()` (hash/identity): two targets that point
    // at the same index are still the same target even if their debug
    // trails differ.
    using DebugOrigins = stdu::vector<IR::TokenID>;
    struct ActionTarget {
        std::size_t id;
        IR::TokenID debug;
        std::optional<CharOrigin> char_origin;
        auto operator==(const ActionTarget &other) const -> bool = default;
        auto operator<(const ActionTarget &other) const -> bool {
            return id < other.id;
        };

    private:
        friend struct ::uhash;
        auto members() const { return std::tie(id); }
    };

    struct SemanticTarget {
        std::size_t id;
        IR::TokenID debug;
        std::optional<CharOrigin> char_origin;
        auto operator==(const SemanticTarget &other) const -> bool = default;
        auto operator<(const SemanticTarget &other) const -> bool {
            return id < other.id;
        };

    private:
        friend struct ::uhash;
        auto members() const { return std::tie(id); }
    };

    struct DFATarget {
        std::size_t id;
        IR::TokenID debug;
        std::optional<CharOrigin> char_origin;
        auto operator==(const DFATarget &other) const -> bool = default;
        auto operator<(const DFATarget &other) const -> bool {
            return id < other.id;
        };

    private:
        friend struct ::uhash;
        auto members() const { return std::tie(id); }
    };
    struct TokenBinding {
        std::size_t token_id = NULL_STATE;
        std::optional<std::size_t> target_semantic_state = std::nullopt;
        std::optional<std::size_t> reduce_rule_id = std::nullopt;
        bool is_unique_representation = false;

        auto operator==(const TokenBinding &other) const -> bool = default;

    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(token_id, target_semantic_state, reduce_rule_id,
                            is_unique_representation);
        }
    };
    struct ActionState {
        Action action = Action::UNDEF;
        std::string variable;
        std::size_t next_nfa_state = NULL_STATE;
        std::variant<DFATarget, ActionTarget, SemanticTarget> next_state;
        Action wrapped_action = Action::UNDEF;
        std::string debug_note;
        // Register operands of SET .. APPEND_NEXT; on BEGIN/END operand_a is the capture number.
        std::size_t operand_a = NULL_STATE;
        std::size_t operand_b = NULL_STATE;
        // BEGIN/END: fires before the consumed character, not after it.
        bool before_char = false;
        // BEGIN/END: the capture keeps every iteration (an Array value).
        bool list_capture = false;
        auto operator==(const ActionState &other) const -> bool = default;
        auto operator<(const ActionState &other) const -> bool {
            if (action != other.action) {
                return action < other.action;
            }
            if (variable != other.variable) {
                return variable < other.variable;
            }
            if (operand_a != other.operand_a) {
                return operand_a < other.operand_a;
            }
            if (operand_b != other.operand_b) {
                return operand_b < other.operand_b;
            }
            return next_state < other.next_state;
        };

    private:
        friend struct ::uhash;
        auto members() const { return std::tie(action, variable, next_state, operand_a, operand_b, before_char, list_capture); }
    };

    struct SemanticState {
        LangAPI::Inheritance instance_value;
        LangAPI::Statements statements;
        std::variant<DFATarget, ActionTarget, SemanticTarget> next_state;
        std::size_t nfa_index = NULL_STATE;
        std::string debug_note;
        auto operator==(const SemanticState &other) const -> bool = default;
        auto operator<(const SemanticState &other) const -> bool {
            if (instance_value != other.instance_value) {
                return instance_value < other.instance_value;
            }
            return next_state < other.next_state;
        };

    private:
        friend struct ::uhash;
        auto members() const { return std::tie(instance_value, next_state); }
    };
    using RawAction = std::variant<ActionState, SemanticState>;
    using ActionChain = stdu::vector<RawAction>;
    using DelayedActionChain = stdu::vector<std::pair<RawAction, std::size_t>>;
    using ActionTable = stdu::vector<ActionState>;
    using SemanticTable = stdu::vector<SemanticState>;
    // Transitions are named after the literal/reference the source site
    // represents (a string literal, a char-class label, "." for any, or the
    // joined token_name for a call to another token). Kept as plain text so
    // later stages (DFA subset construction) can treat it opaquely.
    using TransitionKey = char;

    struct TransitionValue {
        std::size_t next = NULL_STATE;
        std::string next_name;      // name of the target state - a debugger should key off
                                     // this, not the raw `next` index, to identify where an
                                     // edge goes
        std::size_t priority = 0;
        ActionChain actions;   // BEGIN/PUSH/END etc., invented at this stage
        std::string fragment;       // human-readable debug label
        IR::TokenID source;          // rich link back to IR / AST
        std::optional<CharOrigin> char_origin; // set on every character-level edge; lets a
                                                // debugger recognize and re-merge a run of
                                                // characters unrolled from one literal
        auto operator==(const TransitionValue &other) const -> bool = default;

    private:
        friend struct ::uhash;
        auto members() const { return std::tie(next, priority); }
    };

    struct State {
        std::size_t id = NULL_STATE;   // stable, unique per state (also = index)
        std::string name;              // stable, human-readable, unique identifier for this
                                        // state - this, not `id`, is what a debugger should
                                        // index by
        utype::unordered_map<TransitionKey, stdu::vector<TransitionValue>> transitions;
        utype::unordered_set<TransitionValue> epsilon_transitions;
        std::optional<TokenBinding> accept_binding = std::nullopt;
        IR::TokenID origin;              // which TokenID this state was built from
        std::optional<CharOrigin> char_origin; // set when this state is one character of a
                                                // literal unrolled by TNFABuilder (i.e. it has
                                                // no TokenID of its own in the IR)
        std::string debug_note;
        auto operator==(const State &other) const -> bool = default;
    };
    std::ostream &
operator<<(
    std::ostream &os,
    const NFA::TNFA::CharOrigin &o
);
    std::ostream &
operator<<(
    std::ostream &os,
    const NFA::TNFA::ActionTarget &t
);
    std::ostream &
operator<<(
    std::ostream &os,
    const NFA::TNFA::SemanticTarget &t
);
    std::ostream &
operator<<(
    std::ostream &os,
    const NFA::TNFA::DFATarget &t
);
    std::ostream &
operator<<(
    std::ostream &os,
    const NFA::TNFA::TokenBinding &b
);
    std::ostream &
operator<<(
    std::ostream &os,
    const NFA::TNFA::ActionState &s
);
    std::ostream &
operator<<(
    std::ostream &os,
    const NFA::TNFA::SemanticState &s
);
    std::ostream &
operator<<(
    std::ostream &os,
    const NFA::TNFA::TransitionValue &t
);
    std::ostream &
operator<<(
    std::ostream &os,
    const NFA::TNFA::State &s
);
}

