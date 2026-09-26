export module DFA;

import NFA.IR.API;
import NFA.TNFA.API;
import NFA.TNFA;
import DFA.API;
import DFA.States;
import DFA.closure;

import LangAPI;

import hash;
import logging;
import corelib;
import cpuf.op;
import dstd;
import std; // Added for std::variant

export namespace DFA {
// -------------------------------------

struct ClassifiedDFA {
  CharClassTable table;
  States<State<ClassTransitions>> states = {nullptr};

  friend auto operator<<(std::ostream &os, const ClassifiedDFA &dfa) -> std::ostream &;
};

class DFA {
  States<State<>> states;
  States<StateWithActions> states_with_actions;
  NFA::TNFA::TNFABuilder &nfa;
  stdu::vector<NFA::ActionState> action_table;
  stdu::vector<NFA::SemanticState> semantic_table;
  // Number of TDFA registers the DFA needs (set by build()).
  std::size_t register_count = 0;
  std::size_t output_count = 0;
  auto sameAcceptBinding(const State<> &a, const State<> &b) -> bool;
  auto initialClass(const StateWithActions &s) -> std::size_t;
  void optimizeRegistersAndLRTable();
  void optimizeSemanticTable();
  auto clear() -> void;
  auto check_dfa() -> void;

public:
  DFA(NFA::TNFA::TNFABuilder *nfa) : nfa(*nfa), states(nfa), states_with_actions(nfa) {}
  auto build() -> const States<StateWithActions> &;
  auto minimize() -> States<State<>>;
  auto classify() -> ClassifiedDFA;
  auto &get() { return states; }
  auto &get() const { return states; }
  auto &getActionTable() { return action_table; }
  auto &getActionTable() const { return action_table; }
  auto &getSemanticTable() { return semantic_table; }
  auto &getSemanticTable() const { return semantic_table; }
  auto getRegisterCount() const -> std::size_t { return register_count; }
  auto getOutputCount() const -> std::size_t { return output_count; }

  auto operator=(const DFA &other) {
    states = std::move(other.states);
    states_with_actions = std::move(other.states_with_actions);
    nfa = other.nfa;
    action_table = std::move(other.action_table);
    semantic_table = std::move(other.semantic_table);
    register_count = other.register_count;
    output_count = other.output_count;
  };
};

auto operator<<(std::ostream &os, const DFA &dfa) -> std::ostream &;
auto operator<<(std::ostream &os, const ClassifiedDFA &dfa) -> std::ostream &;
} // namespace DFA