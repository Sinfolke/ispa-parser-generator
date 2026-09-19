export module NFA.TNFA;
import NFA.IR;
import NFA.IR.API;
import NFA.TNFA.API;
import AST.Tree;
import AST.API;
import hash;
import dstd;
import std;

// ---------------------------------------------------------------------------
// NFA::TNFA
//
// This stage consumes the already-flattened NFA::InitialAPI::Token graph
// (groups, quantifiers and nested tokens already unrolled by InitialNFA) and
// turns it into a plain TNFA:
//
//   - one NFA state per distinct TokenID "site" in the IR graph
//   - every state gets its own stable, unique numeric `id`
//   - transitions are keyed by NAME (the literal/reference the site
//     represents), not by index - and every transition carries a
//     `SourceLink` back to the exact TokenID/AST::RuleMember it came from,
//     so debugging and source-mapping stay rich all the way through
//   - Action/Semantic states are invented HERE (they do not exist in the
//     IR): register BEGIN/PUSH/END for captures, REDUCE for accept sites
//
// Because the IR has already resolved groups/quantifiers/alternation into a
// flat per-token graph, this stage never has to recurse into nested
// structure - it only has to walk TokenID -> vector<TokenID> edges.
// ---------------------------------------------------------------------------

using namespace NFA::IR;

export namespace NFA::TNFA {
  constexpr std::size_t ALPHABET_SIZE = std::numeric_limits<char>::max() + 1;
  using ExpansionStack = utype::unordered_set<TokenID>;
  struct CaptureBoundaries {
    enum class Kind { Begin, End };
    enum class Position { Entry, Exit };
    struct Event {
      Kind kind;
      Position position;
      TokenID capture;
    };
    stdu::vector<Event> events; // ordered — nesting order preserved
  };
  class TNFABuilder {
    const NFA::InitialNFA &ir;
    // When false, transitions/states are wired without debug provenance
    // (TokenID source + CharOrigin). The DFA still compares provenance
    // unconditionally; it just sees an empty trail everywhere.
    bool debug = true;
    stdu::vector<State> states;
    utype::unordered_map<TokenID, std::size_t> node_ids;
    utype::unordered_map<std::string, std::size_t> states_by_name;
    utype::unordered_map<stdu::vector<std::string>, std::size_t> token_entry;
    std::size_t registers_count = 0;
    std::size_t priority_counter = 0;
    std::unordered_map<std::size_t, TokenBinding> accept_map;
    std::size_t next_rule = 0;
    stdu::vector<SemanticState> semantic_table;
    auto wireString(std::size_t current_state, const TokenID &source) -> std::size_t;
    auto wireAny(std::size_t current_state, const TokenID &source) -> std::size_t;
    auto wireCsequence(std::size_t current_state, const TokenID &source) -> std::size_t;
    auto wireEscaped(std::size_t current_state, const TokenID &source) -> std::size_t;
    auto wireEpsilonActions(
        std::size_t current_state,
        ActionChain actions
    ) -> std::size_t;
    auto wireMember(std::size_t current_state, const TokenID &source, std::unordered_map<std::string, CaptureBoundaries> &capture_boundaries) -> std::size_t;
    void wireSelfLoop(std::size_t pred_state, std::size_t state);
    void wireToken(const stdu::vector<std::string> &name, const Token &token, ExpansionStack &expansion_stack);
      void markAccept(
          std::size_t state_id,
          const TokenID &tail,
          const Token &token,
          std::size_t next,
          ActionChain capture_boundaries
      );
    std::size_t next_priority() { return priority_counter++; }
    std::size_t next_state(std::size_t current_state = NULL_STATE) { return current_state == NULL_STATE ? states.size() : ++current_state; }
  public:
    explicit TNFABuilder(const NFA::InitialNFA &ir, bool debug = true) : ir(ir), debug(debug) {}
    void build();

    auto &getStates() { return states; }
    auto &getStates() const { return states; }
    auto &getAcceptMap() const { return accept_map; }
    // Entry state id for a given token name, if it was built.
    auto &getTokenEntries() const { return token_entry; }
    auto getEntry(const stdu::vector<std::string> &token_id) const -> std::optional<std::size_t> { return token_entry.contains(token_id) ? std::optional {token_entry.at(token_id)} : std::nullopt; }
    bool isCharNfa() { return true; }
    auto next_rule_run() { return next_rule++; };
    auto getSemanticTable() const { return semantic_table; }
    auto operator=(const TNFABuilder &tnfa) {
      debug = tnfa.debug;
      states = tnfa.states;
      node_ids = tnfa.node_ids;
      token_entry = tnfa.token_entry;
      registers_count = tnfa.registers_count;
      priority_counter = tnfa.priority_counter;
      semantic_table = tnfa.semantic_table;
      accept_map = tnfa.accept_map;
    };
  };
}

export std::ostream &operator<<(std::ostream &os, const NFA::TNFA::State &s);
export std::ostream &operator<<(std::ostream &os, const NFA::TNFA::TNFABuilder &b);
