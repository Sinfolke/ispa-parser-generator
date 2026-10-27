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
import cpuf.op;
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
    std::optional<std::size_t> dynamic_state;
    std::vector<std::size_t> dynamic_captures;
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

    // Specificity of the entry transitions for each accepted token. This is a
    // header-free compatibility heuristic: a fixed literal usually has one
    // possible first character, whereas an identifier character class has many.
    // TODO: replace with an explicit priority stored in TokenBinding, derived
    // from the source grammar at TNFA construction time.
    std::map<std::size_t, std::size_t> accept_entry_width;

    void computeEntryWidths() {
        const auto &states = nfa.getStates();
        for (const auto &[name, entry] : nfa.getTokenEntries()) {
            if (entry >= states.size()) continue;
            std::set<std::size_t> seen;
            std::vector<std::size_t> pending{entry};
            std::set<char> first_characters;
            // Epsilon closure of the token's entry, stopping at consuming edges.
            while (!pending.empty()) {
                const auto state = pending.back();
                pending.pop_back();
                if (state >= states.size() || !seen.insert(state).second) continue;
                for (const auto &[character, edges] : states[state].transitions) {
                    if (std::ranges::any_of(edges, [](const auto &edge) {
                            return edge.next != TNFA::NULL_STATE;
                        })) first_characters.insert(character);
                }
                for (const auto &edge : states[state].epsilon_transitions) {
                    if (edge.next != TNFA::NULL_STATE) pending.push_back(edge.next);
                }
            }
            // Traverse the token's own reachable states and associate accepts.
            // The traversal intentionally follows consuming edges as well.
            seen.clear();
            pending.push_back(entry);
            while (!pending.empty()) {
                const auto state = pending.back();
                pending.pop_back();
                if (state >= states.size() || !seen.insert(state).second) continue;
                if (states[state].accept_binding) {
                    const auto id = states[state].accept_binding->token_id;
                    const auto width = first_characters.size();
                    auto [it, inserted] = accept_entry_width.emplace(id, width);
                    if (!inserted) it->second = std::min(it->second, width);
                }
                for (const auto &[character, edges] : states[state].transitions)
                    for (const auto &edge : edges)
                        if (edge.next != TNFA::NULL_STATE) pending.push_back(edge.next);
                for (const auto &edge : states[state].epsilon_transitions)
                    if (edge.next != TNFA::NULL_STATE) pending.push_back(edge.next);
            }
        }
    }

    auto eventsOf(const ActionPath &path, std::size_t edge_ops, bool after_edge) const
        -> std::vector<tdfa::Event> {
        std::vector<tdfa::Event> out;
        std::optional<std::size_t> dynamic_position;
        for (std::size_t i = 0; i < path.size(); ++i) {
            if (const auto *semantic = std::get_if<TNFA::SemanticState>(&path[i].raw);
                semantic && semantic->dynamic_source) {
                dynamic_position = semantic->dynamic_source->position_in_token;
                continue;
            }
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
            // Capture boundaries for earlier token members must be published
            // before a later dynamic matcher reads them, even if closure merged
            // their paths in the opposite order. Same-member boundaries after
            // the matcher stay deferred so captures begin after its prefix.
            const bool after_dynamic = dynamic_position &&
                (a->position_in_token == TNFA::NULL_STATE ||
                 a->position_in_token > *dynamic_position ||
                 (a->position_in_token == *dynamic_position && a->action == TNFA::Action::END));
            ev.next = after_edge ? (after_dynamic || (i < edge_ops ? !a->before_char : true)) : false;
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

    TnfaProvider(TNFA::TNFABuilder &nfa, const CaptureTable &caps) : nfa(nfa), caps(caps) {
        computeEntryWidths();
    }

    auto start() -> tdfa::Closed<Extra> {
        std::vector<std::size_t> entries;
        for (const auto &[_, entry] : nfa.getTokenEntries()) {
            entries.push_back(entry);
        }
        std::ranges::sort(entries);
        entries.erase(std::unique(entries.begin(), entries.end()), entries.end());

        std::vector<std::pair<std::size_t, ActionPath>> seeded;
        for (const auto entry : entries)
            seeded.emplace_back(entry, ActionPath{});
        Closure closure(&nfa, seeded);

        tdfa::Closed<Extra> out;
        for (const std::size_t state : closure.getDiscoveryOrder()) {
            out.items.push_back({state, tdfa::NIL, eventsOf(closure.getActionsForState(state), 0, false)});
        }
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

        // Several active TNFA paths can enter the same shared destination at
        // the same priority while closing different nested captures. Closure
        // keeps one path per destination, so merge tag actions per capture
        // boundary (BEGIN/END) before handing those seeds to it. For each
        // boundary, the earliest path remains authoritative.
        std::vector<ActionPath> seed_actions;
        seed_actions.reserve(seeds.size());
        for (const auto &seed : seeds)
            seed_actions.push_back(seed.actions);
        for (std::size_t i = 0; i < seeds.size(); ++i) {
            std::set<std::pair<std::size_t, TNFA::Action>> boundaries_present;
            for (const auto &fired : seed_actions[i])
                if (const auto *action = std::get_if<TNFA::ActionState>(&fired.raw);
                    action && isTagAction(*action))
                    boundaries_present.emplace(action->operand_a, action->action);
            for (std::size_t j = i + 1; j < seeds.size(); ++j) {
                if (seeds[j].target != seeds[i].target || seeds[j].priority != seeds[i].priority)
                    continue;
                std::set<std::pair<std::size_t, TNFA::Action>> imported;
                for (const auto &fired : seeds[j].actions) {
                    const auto *action = std::get_if<TNFA::ActionState>(&fired.raw);
                    if (!action || !isTagAction(*action))
                        continue;
                    const auto boundary = std::pair{action->operand_a, action->action};
                    if (boundaries_present.contains(boundary) || !imported.insert(boundary).second)
                        continue;
                    for (const auto &candidate : seeds[j].actions)
                        if (const auto *tag = std::get_if<TNFA::ActionState>(&candidate.raw);
                            tag && isTagAction(*tag) && tag->operand_a == action->operand_a &&
                            tag->action == action->action)
                            seed_actions[i].push_back(candidate);
                    boundaries_present.insert(boundary);
                }
            }
        }

        std::vector<std::pair<std::size_t, ActionPath>> seeded;
        seeded.reserve(seeds.size());
        for (std::size_t i = 0; i < seeds.size(); ++i)
            seeded.emplace_back(seeds[i].target, seed_actions[i]);
        Closure closure(&nfa, seeded);
        if (closure.get().empty())
            return {};

        tdfa::Closed<Extra> out;
        for (const std::size_t state : closure.getDiscoveryOrder()) {
            const auto &seed = seeds.at(closure.getSeedIndex(state));
            const auto &path = closure.getActionsForState(state);
            out.items.push_back({state,
                                 positionOf(subset, seed.source),
                                 eventsOf(path,
                                          seed_actions.at(closure.getSeedIndex(state)).size(), true)});
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
            for (const auto &fired : seed.actions)
                if (const auto *semantic = std::get_if<TNFA::SemanticState>(&fired.raw);
                    semantic && semantic->dynamic_source) {
                    if (!out.extra.dynamic_state) out.extra.dynamic_state = seed.target;
                    for (const auto &[_, capture] : semantic->dynamic_captures)
                        if (std::ranges::find(out.extra.dynamic_captures, capture) ==
                            out.extra.dynamic_captures.end())
                            out.extra.dynamic_captures.push_back(capture);
                }
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
            if (!binding) continue;
            const auto widthOf = [&](std::size_t id) {
                const auto it = accept_entry_width.find(id);
                return it == accept_entry_width.end()
                    ? std::numeric_limits<std::size_t>::max() : it->second;
            };
            // Both accepts are for the same consumed input (one TDFA state).
            // Prefer the more specific entry; preserve the old tie-break when
            // specificity is indistinguishable.
            if (!best || widthOf(binding->token_id) < widthOf(best->token_id) ||
                (widthOf(binding->token_id) == widthOf(best->token_id) &&
                 binding->token_id > best->token_id)) {
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
                if (path != &terminal) {
                    const auto *semantic = std::get_if<TNFA::SemanticState>(&fired.raw);
                    if (semantic && semantic->dynamic_source) continue;
                }
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
    const bool dynamic = std::ranges::any_of(passthrough, [](const RawAction &raw) {
        const auto *semantic = std::get_if<TNFA::SemanticState>(&raw);
        return semantic && semantic->dynamic_source;
    });
    std::vector<bool> after_dynamic(ops.size(), false);
    std::set<std::size_t> after_dynamic_registers;
    for (std::size_t i = 0; i < ops.size(); ++i) {
        const auto &op = ops[i];
        bool defer = op.next;
        const bool reads_b = (op.kind == tdfa::OpKind::Copy || op.kind == tdfa::OpKind::Append) &&
                             op.b != tdfa::NIL;
        if (!defer && reads_b && after_dynamic_registers.contains(op.b))
            defer = true;
        after_dynamic[i] = defer;
        if (defer)
            after_dynamic_registers.insert(op.a);
    }

    for (std::size_t i = 0; i < ops.size(); ++i)
        if (!dynamic || !after_dynamic[i]) out.push_back(toRaw(ops[i]));
    for (const auto &raw : passthrough)
        out.push_back(raw);
    if (dynamic)
        for (std::size_t i = 0; i < ops.size(); ++i)
            if (after_dynamic[i]) {
                auto op = ops[i];
                // The callback does not advance the scanner cursor. Preserve
                // the op's next-boundary flag so SET records the boundary
                // after the current input character once the callback returns.
                // Keep copies that read a deferred register after its write.
                out.push_back(toRaw(op));
            }
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
            auto actions = toActions(edge.ops, edge.extra.passthrough);
            dst.transitions[edge.sym] = makeSequence(
                std::move(actions),
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
    // std::cout << "TDFA: " << states_with_actions.size() << " states, " << register_count << " registers ("
    //           << output_count << " outputs)" << std::endl;
    return states_with_actions;
}

} // namespace DFA
