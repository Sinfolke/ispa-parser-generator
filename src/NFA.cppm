export module NFA;
import AST.API;
import AST.Tree;
import LangAPI;
import hash;
import dstd;

export class NFA {
public:
  static constexpr auto NULL_STATE = std::numeric_limits<std::size_t>::max();
  static constexpr std::size_t NESTED_REDUCE_ID_BASE = 1'000'000;
  using TransitionKey = std::variant<stdu::vector<std::string>, char>;
  enum class StoreCstNode { CST_NODE, CST_GROUP, CST_CONDITION };
  enum class TableType { DFA, Action, Semantic };
  enum class Action { UNDEF, BEGIN, END, PUSH };
  enum class SemanticAction { UNDEF, REDUCE };

  struct ActionTarget {
    std::size_t id;
    auto operator==(const ActionTarget &other) const -> bool = default;
    auto operator<(const ActionTarget &other) const -> bool {
      return id < other.id;
    };

  private:
    friend struct uhash;
    auto members() const { return std::tie(id); }
  };

  struct SemanticTarget {
    std::size_t id;
    auto operator==(const SemanticTarget &other) const -> bool = default;
    auto operator<(const SemanticTarget &other) const -> bool {
      return id < other.id;
    };

  private:
    friend struct uhash;
    auto members() const { return std::tie(id); }
  };

  struct DFATarget {
    std::size_t id;
    auto operator==(const DFATarget &other) const -> bool = default;
    auto operator<(const DFATarget &other) const -> bool {
      return id < other.id;
    };

  private:
    friend struct uhash;
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

  struct TemplatedDataBlockValue : uhash {
    StoreCstNode type;
    std::size_t cst_index;
    const AST::RuleMember *AST = nullptr;
    auto operator==(const TemplatedDataBlockValue &other) const -> bool {
      return type == other.type && cst_index == other.cst_index &&
             AST == other.AST;
    }

  private:
    friend struct ::uhash;
    auto members() const { return std::tie(type, cst_index, AST); }
  };
  struct ActionState {
    Action action = Action::UNDEF;
    std::string variable;
    std::size_t next_nfa_state = NULL_STATE;
    std::variant<DFATarget, ActionTarget, SemanticTarget> next_state;
    Action wrapped_action;
    std::string debug_note;
    auto operator==(const ActionState &other) const -> bool = default;
    auto operator<(const ActionState &other) const -> bool {
      if (action != other.action) {
        return action < other.action;
      }
      if (variable != other.variable) {
        return variable < other.variable;
      }
      return next_state < other.next_state;
    };

  private:
    friend struct ::uhash;
    auto members() const { return std::tie(action, variable, next_state); }
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
  using ActionChain = stdu::vector<std::variant<ActionState, SemanticState>>;
  using TemplatedDataBlock =
      utype::unordered_map<std::string, TemplatedDataBlockValue>;
  using DataBlock =
      std::variant<std::monostate, TemplatedDataBlock, TemplatedDataBlockValue>;
  using ActionTable = stdu::vector<ActionState>;
  using SemanticTable = stdu::vector<SemanticState>;

  // Internal NFA transition used during graph construction
  struct TransitionValue {
    std::size_t next = NULL_STATE;
    std::size_t priority = 0;
    ActionChain actions;
    // for debug purposes; Does not ever affect generation
    std::string fragment;
    AST::RuleMember *AST_member = nullptr;
    auto operator==(const TransitionValue &other) const -> bool = default;

  private:
    friend struct ::uhash;
    auto members() const { return std::tie(next, priority); }
  };
  struct state {
    utype::unordered_map<TransitionKey, stdu::vector<TransitionValue>>
        transitions;
    std::optional<TokenBinding> accept_binding = std::nullopt;
    utype::unordered_set<TransitionValue> epsilon_transitions;
    std::string debug_note;
    auto operator==(const state &other) const -> bool = default;

  private:
    friend struct ::uhash;
    auto members() const {
      return std::tie(transitions, accept_binding, epsilon_transitions);
    }
  };

  struct StateRange {
    std::size_t start;
    std::size_t end;
    bool valid() const { return start != NULL_STATE && end != NULL_STATE; }
    bool invalid() const { return !valid(); }
  };

private:
  AST::Tree &tree;
  const stdu::vector<std::shared_ptr<AST::RuleMember>> *rules = nullptr;
  const AST::RuleMember *member = nullptr;
  const AST::RuleMember *current_member = nullptr;
  const AST::DataBlock *dtb;
  stdu::vector<std::string> name_;
  DataBlock nfadtb;
  stdu::vector<state> states;
  stdu::vector<std::size_t> add_space_skip_places;
  stdu::vector<std::pair<std::size_t, std::size_t>> group_close_propagate;
  stdu::vector<std::size_t> cst_node_close_propagate;
  bool no_add_space_skip_next = false;
  bool store_entire_group = false;
  bool buildingNestedRule = false;
  utype::unordered_set<stdu::vector<std::string>> processing;
  utype::unordered_map<stdu::vector<std::string>, StateRange> fragment_cache;
  // Highest state index that belonged to a cached fragment at the moment
  // it was cached (inclusive). Construction is single-threaded recursive
  // descent, so every state pushed between a name's `entry` allocation
  // and its `fragment_cache[name] = {entry, end}` store belongs
  // exclusively to that fragment - this lets a cache HIT rebuild an
  // independent, rebased copy of exactly that state range instead of
  // aliasing the original states (see buildStateFragment).
  utype::unordered_map<stdu::vector<std::string>, std::size_t>
      fragment_cache_extent;
  std::size_t *accept_index;
  std::size_t nested_accept_counter = 0;
  std::size_t nested_count = 0;
  std::size_t group_count = 0;
  bool is_char_table = false;
  bool first = true;
  bool isWhitespaceToken = false;
  std::unordered_map<std::size_t, TokenBinding> accept_map;
  stdu::vector<LangAPI::Type> value_types;
  std::size_t registers_count = 0;
  std::size_t priority_counter = 0;

  // Build methods
  auto applyQuantifierAndActions(const AST::RuleMember &member,
                                 std::size_t start, std::size_t end,
                                 StateRange body, bool isLastMember,
                                 bool addStoreActions, bool nestedReduction,
                                 bool collapseIterationBegin = false)
      -> StateRange;
  void handleTerminal(
    const AST::RuleMember &member,
    const stdu::vector<std::string> &name,
    const std::size_t &start,
    const std::size_t &end,
    bool isLastMember,
    bool addStoreActions,
    bool nestedReduction
  );
  void handleNonTermnal(
    const AST::RuleMember &member,
    const stdu::vector<std::string> &name,
    const std::size_t &start,
    const std::size_t &end,
    bool isLastMember,
    bool addStoreActions,
    bool nestedReduction
);
  void handleGroup(
    const AST::RuleMember &member,
    const stdu::vector<std::shared_ptr<AST::RuleMember>> &group,
    const std::size_t &start,
    const std::size_t &end,
    bool isLastMember,
    bool addStoreActions,
    bool nestedReduction
);
  void handleString(
    const AST::RuleMember &member,
    const std::string &str,
    const std::size_t &start,
    const std::size_t &end,
    bool isLastMember,
    bool addStoreActions,
    bool nestedReduction
);
  void handleCsequence(
    const AST::RuleMember &member,
    const AST::RuleMemberCsequence &csequence,
    const std::size_t &start,
    const std::size_t &end,
    bool isLastMember,
    bool addStoreActions,
    bool nestedReduction
);
  void handleAny(
    const AST::RuleMember &member,
    const std::size_t &start,
    const std::size_t &end,
    bool isLastMember,
    bool addStoreActions,
    bool nestedReduction
);
  auto buildStateFragment(const AST::RuleMember &member, bool isLastMember,
                          bool addStoreActions, bool nestedReduction = false)
      -> StateRange;
  auto investigateHasNext(std::size_t place, char c,
                          std::unordered_set<std::size_t> &visited) -> bool;
  auto investigateHasNext(std::size_t place,
                          const stdu::vector<std::string> &name,
                          std::unordered_set<std::size_t> &visited) -> bool;
  void addSpaceSkip();
  // Marks `state_id` as an accept state for `member`, and, if the token isn't a
  // unique/literal representation, pushes a REDUCE entry into the Semantic
  // table.
  void markAccept(std::size_t state_id, std::size_t next_state,
                  const AST::RuleMember &member, bool nestedReduction,
                  bool is_repeating);
  void acceptMapVisitState(std::size_t index,
                           std::optional<TokenBinding> current_binding,
                           std::unordered_set<std::size_t> &visited);
  void getStatesToPropagate(std::size_t state_id,
                            std::unordered_set<std::size_t> &result);
  auto getStatesToPropagate(std::size_t id) -> std::unordered_set<std::size_t>;
  void generateTemplatedDataBlockFromSingleRule(
      const AST::RuleMember &mem, TemplatedDataBlock &templated_data_block,
      std::size_t &prefix_index, std::size_t &index, std::size_t &group_index);
  void generateTemplatedDataBlockFromRules(
      const stdu::vector<std::shared_ptr<AST::RuleMember>> &rules,
      TemplatedDataBlock &templated_data_block, std::size_t &prefix_index,
      std::size_t &index, std::size_t &group_index);
  void generateSingleDataBlockFromRules(
      const stdu::vector<std::shared_ptr<AST::RuleMember>> &rules,
      TemplatedDataBlockValue &single_data_block, bool &isAlreadyConstructed);

  inline void increase_accept_index() { ++(*accept_index); }

public:
  NFA(AST::Tree &tree, const stdu::vector<std::string> &name,
      const AST::DataBlock *dtb,
      const stdu::vector<std::shared_ptr<AST::RuleMember>> &rules,
      bool isWhitespaceToken, bool is_char_table, std::size_t *accept_index_ptr)
      : tree(tree), name_(name), rules(&rules), dtb(dtb),
        isWhitespaceToken(isWhitespaceToken), is_char_table(is_char_table),
        accept_index(accept_index_ptr) {}
  NFA(AST::Tree &tree, const stdu::vector<std::string> &name,
      const AST::DataBlock *dtb, const AST::RuleMember &member,
      bool isWhitespaceToken, bool is_char_table, std::size_t *accept_index_ptr)
      : tree(tree), name_(name), member(&member), dtb(dtb),
        isWhitespaceToken(isWhitespaceToken), is_char_table(is_char_table),
        accept_index(accept_index_ptr) {}
  NFA(AST::Tree &tree, const stdu::vector<std::string> &name,
      const AST::Rule &rule, bool isWhitespaceToken, bool is_char_table,
      std::size_t *accept_index_ptr)
      : tree(tree), name_(name), rules(&rule.rule_members),
        dtb(&rule.data_block), isWhitespaceToken(isWhitespaceToken),
        is_char_table(is_char_table), accept_index(accept_index_ptr) {}
  NFA(AST::Tree &tree, const stdu::vector<std::string> &name,
      const AST::DataBlock *dtb,
      const stdu::vector<std::shared_ptr<AST::RuleMember>> &rules,
      bool isWhitespaceToken, bool is_char_table, std::size_t *accept_index_ptr,
      bool buildingNestedRule)
      : tree(tree), rules(&rules), dtb(dtb), name_(name),
        buildingNestedRule(buildingNestedRule),
        is_char_table(is_char_table), isWhitespaceToken(isWhitespaceToken),
        accept_index(accept_index_ptr) {}
  void build(bool addStoreActions = true);
  void buildAcceptMap();
  auto getRegistersCount() { return registers_count; }
  auto getRegistersCount() const  { return registers_count; }
  auto &getStates() { return states; }
  auto &getStates() const { return states; }
  auto &getAcceptMap() const { return accept_map; }
  auto &isCharNfa() const { return is_char_table; }
  auto &getName() const { return name_; }
  auto &getDtb() const { return nfadtb; }
  inline std::size_t next_priority() { return priority_counter++; }
};

export std::ostream &operator<<(std::ostream &os, const NFA::state &s);
export std::ostream &operator<<(std::ostream &os, const NFA &states);
export std::ostream &operator<<(std::ostream &os,
                                const NFA::ActionState &states);
export std::ostream &operator<<(std::ostream &os,
                                const stdu::vector<NFA::ActionState> &states);
export std::ostream &operator<<(std::ostream &os,
                                const NFA::SemanticState &states);
export std::ostream &operator<<(std::ostream &os,
                                const stdu::vector<NFA::SemanticState> &states);
export std::ostream &operator<<(std::ostream &os,
                                const NFA::TableType &type);
export std::ostream &operator<<(std::ostream &os,
                                const NFA::Action &type);
export std::ostream &operator<<(std::ostream &os,
                                const NFA::DFATarget &tg);
export std::ostream &operator<<(std::ostream &os,
                                const NFA::ActionTarget &tg);
export std::ostream &operator<<(std::ostream &os,
                                const NFA::SemanticTarget &tg);
export std::ostream &operator<<(std::ostream &os,
                                const NFA::TokenBinding &binding);