export module DFA;

import NFA_OLD;

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
  NFA::NFA &nfa;
  stdu::vector<NFA::ActionState> action_table;
  stdu::vector<NFA::SemanticState> semantic_table;
  void bindBeforeActions();
  auto sameAcceptBinding(const State<> &a, const State<> &b) -> bool;
  auto initialClass(const StateWithActions &s) -> std::size_t;
  auto refinementKey(const State<> &s,
                     const std::unordered_map<std::size_t, std::size_t> &partition_of)
      -> std::vector<TransitionKeyExt>;
  void optimizeRegistersAndLRTable();
  void optimizeSemanticTable();
  auto clear() -> void;
  auto getType() const -> DfaType;
  auto check_dfa() -> void;

public:
  DFA(NFA::NFA *nfa) : nfa(*nfa), states(nfa), states_with_actions(nfa) {}
  auto build() -> const States<StateWithActions> &;
  auto minimize() -> States<State<>>;
  auto classify() -> ClassifiedDFA;
  auto &get() { return states; }
  auto &get() const { return states; }
  auto &getLR() { return action_table; }
  auto &getLR() const { return action_table; }
  auto &getSemantic() { return semantic_table; }
  auto &getSemantic() const { return semantic_table; }
};

auto operator<<(std::ostream &os, const DFA &dfa) -> std::ostream &;
auto operator<<(std::ostream &os, const ClassifiedDFA &dfa) -> std::ostream &;
} // namespace DFA