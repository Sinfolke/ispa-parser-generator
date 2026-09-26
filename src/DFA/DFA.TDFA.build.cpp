// DFA::build() as a TDFA construction: tdfa_core does the determinisation, this file adapts the
// TNFA/Closure to it. BEGIN/END become register operations; capture k ends up in registers 2k
// and 2k+1 when a token is accepted. The accept sequence ends at the token's semantic state.
module DFA;

import DFA.TDFA;
import DFA.States;
import DFA.closure;
import hash;
import logging;
import corelib;
import cpuf.printf;
import dstd;
import std;

namespace DFA {
namespace {

namespace TNFA = NFA::TNFA;
using RawAction = TNFA::RawAction;
using NextTarget = std::variant<TNFA::DFATarget, ActionSequence>;

// ---------------------------------------------------------------------------
// Captures -> tags
// ---------------------------------------------------------------------------

struct CaptureTable {
    tdfa::Layout layout;
};

auto isTagAction(const TNFA::ActionState &a) -> bool {
    return a.action == TNFA::Action::BEGIN || a.action == TNFA::Action::END;
}

// Capture k (ActionState::operand_a) owns tags, and output registers, 2k and 2k+1.
auto buildCaptureTable(const TNFA::TNFABuilder &nfa) -> CaptureTable {
    const std::size_t count = nfa.getCaptureCount();
    std::vector<char> list(count, 0);

    const auto visit = [&](const TNFA::ActionChain &chain) {
        for (const auto &raw : chain)
            if (const auto *a = std::get_if<TNFA::ActionState>(&raw); a && isTagAction(*a)) {
                Assert(a->operand_a < count, "DFA::build: capture number out of range");
                list[a->operand_a] |= a->list_capture;
            }
    };
    for (const auto &state : nfa.getStates()) {
        for (const auto &[key, targets] : state.transitions)
            for (const auto &tv : targets)
                visit(tv.actions);
        for (const auto &tv : state.epsilon_transitions)
            visit(tv.actions);
    }

    CaptureTable table;
    table.layout.tags = 2 * count;
    table.layout.captures.resize(count);
    for (std::size_t k = 0; k < count; ++k)
        table.layout.captures[k] = {list[k] != 0, 2 * k, 2 * k + 1};
    return table;
}

// ---------------------------------------------------------------------------
// Debug provenance (unchanged from the old build())
// ---------------------------------------------------------------------------

auto collectDebugOrigins(const std::vector<const NFA::IR::TokenID *> &sources) -> TNFA::DebugOrigins {
    TNFA::DebugOrigins out;
    for (const auto *source : sources)
        if (std::ranges::none_of(out, [&](const NFA::IR::TokenID &e) { return e == *source; }))
            out.push_back(*source);
    return out;
}

auto collectCharOrigins(const std::vector<std::optional<TNFA::CharOrigin>> &sources)
    -> stdu::vector<std::optional<TNFA::CharOrigin>> {
    stdu::vector<std::optional<TNFA::CharOrigin>> out;
    for (const auto &source : sources)
        if (std::ranges::none_of(out, [&](const auto &e) { return e == source; }))
            out.push_back(source);
    return out;
}

// ---------------------------------------------------------------------------
// TNFA provider for tdfa::determinize
// ---------------------------------------------------------------------------

struct StepExtra {
    std::vector<RawAction> passthrough; // SemanticState & co, fired order, as before
    TNFA::DebugOrigins debug;
    stdu::vector<std::optional<TNFA::CharOrigin>> char_origin;
    std::optional<TNFA::TokenBinding> binding; // accept only
};

class TnfaProvider {
    TNFA::TNFABuilder &nfa;
    const CaptureTable &caps;
    // First closure that produced a given NFA subset (accept paths come from it,
    // exactly as dfa_closures did in the old build()).
    std::map<std::vector<std::size_t>, Closure> closures;

    auto eventsOf(const ActionPath &path, std::size_t edge_ops, bool after_edge) const
        -> std::vector<tdfa::Event> {
        std::vector<tdfa::Event> out;
        for (std::size_t i = 0; i < path.size(); ++i) {
            const auto *a = std::get_if<TNFA::ActionState>(&path[i].raw);
            if (!a || !isTagAction(*a))
                continue;
            const auto &cap = caps.layout.captures[a->operand_a];
            const bool begin = a->action == TNFA::Action::BEGIN;
            tdfa::Event ev;
            ev.tag = cap.list || begin ? cap.begin_tag : cap.end_tag;
            ev.append = cap.list;
            // Actions on the consuming edge fire before or after the consumed
            // character (`before_char`); actions on the epsilon path behind it
            // fire after it; the start closure and the accept path consume
            // nothing, so they fire "before".
            ev.next = after_edge ? (i < edge_ops ? !a->before_char : true) : false;
            out.push_back(ev);
        }
        return out;
    }

    static auto positionOf(const std::vector<std::size_t> &subset, std::size_t nfa_state) -> std::size_t {
        return static_cast<std::size_t>(std::ranges::lower_bound(subset, nfa_state) - subset.begin());
    }

    void remember(Closure &&closure) {
        std::vector<std::size_t> key(closure.get().begin(), closure.get().end());
        closures.try_emplace(std::move(key), std::move(closure));
    }

public:
    using Extra = StepExtra;

    TnfaProvider(TNFA::TNFABuilder &nfa, const CaptureTable &caps) : nfa(nfa), caps(caps) {}

    auto start() -> tdfa::Closed<Extra> {
        std::vector<std::size_t> entries;
        for (const auto &[_, entry] : nfa.getTokenEntries())
            entries.push_back(entry);
        std::ranges::sort(entries);
        entries.erase(std::unique(entries.begin(), entries.end()), entries.end());

        std::vector<std::pair<std::size_t, ActionPath>> seeded;
        for (const auto entry : entries)
            seeded.emplace_back(entry, ActionPath{});
        Closure closure(&nfa, seeded);

        tdfa::Closed<Extra> out;
        for (const std::size_t state : closure.getDiscoveryOrder())
            out.items.push_back({state, tdfa::NIL, eventsOf(closure.getActionsForState(state), 0, false)});
        remember(std::move(closure));
        return out;
    }

    auto symbols(const std::vector<std::size_t> &subset) -> std::vector<char> {
        std::set<char> out;
        for (const auto s : subset)
            for (const auto &[sym, targets] : nfa.getStates().at(s).transitions)
                if (std::ranges::any_of(targets, [](const auto &t) { return t.next != TNFA::NULL_STATE; }))
                    out.insert(sym);
        return {out.begin(), out.end()};
    }

    auto move(const std::vector<std::size_t> &subset, char symbol) -> tdfa::Closed<Extra> {
        const auto seeds = Closure::collectSeeds(&nfa, subset, symbol);
        if (seeds.empty())
            return {};

        std::vector<std::pair<std::size_t, ActionPath>> seeded;
        seeded.reserve(seeds.size());
        for (const auto &seed : seeds)
            seeded.emplace_back(seed.target, seed.actions);
        Closure closure(&nfa, seeded);
        if (closure.get().empty())
            return {};

        tdfa::Closed<Extra> out;
        for (const std::size_t state : closure.getDiscoveryOrder()) {
            const auto &seed = seeds.at(closure.getSeedIndex(state));
            out.items.push_back({state,
                                 positionOf(subset, seed.source),
                                 eventsOf(closure.getActionsForState(state), seed.actions.size(), true)});
        }

        // Non-tag actions keep the old behaviour: fired in closure order.
        for (const auto &fired : closure.getTransitionActions()) {
            const auto *a = std::get_if<TNFA::ActionState>(&fired.raw);
            if (!a || !isTagAction(*a))
                out.extra.passthrough.push_back(fired.raw);
        }

        std::vector<const NFA::IR::TokenID *> origins;
        std::vector<std::optional<TNFA::CharOrigin>> char_origins;
        for (const auto &seed : seeds) {
            origins.push_back(seed.source_link);
            char_origins.push_back(seed.char_origin);
        }
        out.extra.debug = collectDebugOrigins(origins);
        out.extra.char_origin = collectCharOrigins(char_origins);

        remember(std::move(closure));
        return out;
    }

    auto accept(const std::vector<std::size_t> &subset) -> std::optional<tdfa::Accept<Extra>> {
        std::optional<TNFA::TokenBinding> best;
        std::size_t best_state = TNFA::NULL_STATE;
        for (const std::size_t s : subset) {
            const auto it = nfa.getAcceptMap().find(s);
            std::optional<TNFA::TokenBinding> binding = it != nfa.getAcceptMap().end()
                                                            ? std::make_optional(it->second)
                                                            : nfa.getStates().at(s).accept_binding;
            if (binding && (!best || binding->token_id > best->token_id)) {
                best = binding;
                best_state = s;
            }
        }
        if (!best)
            return std::nullopt;

        const Closure &closure = closures.at(subset);
        tdfa::Accept<Extra> out;
        out.config = positionOf(subset, best_state);
        out.extra.binding = best;

        // The path INTO the accepting state was already applied by the
        // transition that entered this DFA state; only what happens on the
        // terminal edge (END + semantic) is left to do here.
        const ActionPath &terminal = closure.getTerminalActionsForState(best_state);
        out.events = eventsOf(terminal, 0, false);

        const auto add_unique = [&](const RawAction &raw) {
            if (std::ranges::find(out.extra.passthrough, raw) == out.extra.passthrough.end())
                out.extra.passthrough.push_back(raw);
        };
        for (const auto *path : {&closure.getActionsForState(best_state), &terminal})
            for (const auto &fired : *path) {
                const auto *a = std::get_if<TNFA::ActionState>(&fired.raw);
                if (!a || !isTagAction(*a))
                    add_unique(fired.raw);
            }
        return out;
    }
};

// ---------------------------------------------------------------------------
// tdfa::Op -> TNFA action
// ---------------------------------------------------------------------------

auto toRaw(const tdfa::Op &op) -> RawAction {
    TNFA::ActionState a;
    std::string note;
    switch (op.kind) {
    case tdfa::OpKind::Set:
        a.action = op.next ? TNFA::Action::SET_NEXT : TNFA::Action::SET;
        note = std::format("r{} = pos{}", op.a, op.next ? "+1" : "");
        break;
    case tdfa::OpKind::Append:
        a.action = op.next ? TNFA::Action::APPEND_NEXT : TNFA::Action::APPEND;
        note = std::format("r{} = append({}, pos{})", op.a,
                           op.b == tdfa::NIL ? std::string("nil") : std::format("r{}", op.b),
                           op.next ? "+1" : "");
        break;
    case tdfa::OpKind::Copy:
        a.action = TNFA::Action::COPY;
        note = std::format("r{} = r{}", op.a, op.b);
        break;
    }
    a.operand_a = op.a;
    a.operand_b = op.b; // tdfa::NIL == TNFA::NULL_STATE
    a.debug_note = std::move(note);
    return a;
}

auto toActions(const std::vector<tdfa::Op> &ops, const std::vector<RawAction> &passthrough)
    -> stdu::vector<RawAction> {
    stdu::vector<RawAction> out;
    for (const auto &op : ops)
        out.push_back(toRaw(op));
    for (const auto &raw : passthrough)
        out.push_back(raw);
    return out;
}

auto makeSequence(const stdu::vector<RawAction> &actions,
                  std::size_t target,
                  TNFA::DebugOrigins debug,
                  stdu::vector<std::optional<TNFA::CharOrigin>> char_origin) -> NextTarget {
    Assert(target != TNFA::NULL_STATE, "DFA::build: consuming transition has NULL target");

    if (actions.empty()) {
        return TNFA::DFATarget{
            .id = target,
            .debug = debug.empty() ? NFA::IR::TokenID{} : std::move(debug.back()),
            .char_origin = char_origin.empty() ? std::nullopt : char_origin.back(),
        };
    }
    return ActionSequence{
        .actions = actions,
        .terminal_dfa_target = target,
        .debug = std::move(debug),
        .char_origin = std::move(char_origin),
    };
}

} // namespace

auto DFA::build() -> const States<StateWithActions> & {
    Tlog::Branch b(logger, "DFA.log");

    states_with_actions.clear();
    action_table.clear();
    semantic_table.clear();
    register_count = 0;
    output_count = 0;

    const CaptureTable caps = buildCaptureTable(nfa);
    TnfaProvider provider(nfa, caps);

    std::size_t registers = 0;
    auto dfa = tdfa::determinize(provider, caps.layout, registers);
    register_count = registers;
    output_count = caps.layout.tags;

    for (std::size_t i = 0; i < dfa.size(); ++i) {
        const std::size_t created = states_with_actions.makeNew();
        Assert(created == i, "DFA::build: state index mismatch");
    }

    for (std::size_t i = 0; i < dfa.size(); ++i) {
        const auto &src = dfa[i];
        auto &dst = states_with_actions[i];

        if (src.accepting) {
            dst.accept_binding = src.accept_extra.binding;
            auto actions = toActions(src.accept_ops, src.accept_extra.passthrough);
            if (!actions.empty())
                dst.accept_action = ActionSequence{
                    .actions = std::move(actions),
                    .terminal_dfa_target = TNFA::NULL_STATE,
                    .debug = {},
                };
        }

        for (const auto &edge : src.edges) {
            dst.transitions[edge.sym] = makeSequence(
                toActions(edge.ops, edge.extra.passthrough),
                edge.target,
                edge.extra.debug,
                edge.extra.char_origin);
        }
    }

    // ---- validate -----------------------------------------------------------
    for (std::size_t i = 0; i < states_with_actions.size(); ++i) {
        const auto &state = states_with_actions.get().at(i);
        for (const auto &[symbol, transition] : state.transitions) {
            std::visit(
                [&](const auto &target) {
                    using T = std::decay_t<decltype(target)>;
                    if constexpr (std::is_same_v<T, TNFA::DFATarget>) {
                        Assert(target.id != TNFA::NULL_STATE, "DFA::build: NULL DFA target");
                        Assert(target.id < states_with_actions.size(), "DFA::build: DFA target out of range");
                    } else if constexpr (std::is_same_v<T, ActionSequence>) {
                        Assert(target.terminal_dfa_target != TNFA::NULL_STATE,
                               "DFA::build: consuming ActionSequence has NULL target");
                        Assert(target.terminal_dfa_target < states_with_actions.size(),
                               "DFA::build: ActionSequence target out of range");
                    }
                },
                transition);
        }
        if (state.accept_action.has_value())
            Assert(state.accept_action->terminal_dfa_target == TNFA::NULL_STATE,
                   "DFA::build: accepting action has DFA target");
    }

    std::cout << "TDFA: " << states_with_actions.size() << " states, " << register_count << " registers ("
              << output_count << " outputs)" << std::endl;
    return states_with_actions;
}

} // namespace DFA
