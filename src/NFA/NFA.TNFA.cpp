module NFA.TNFA;
import AST.API;
import LangAPI;
import LLIR.Builder.Base;
import logging;
import corelib;
import cpuf.op;
import cpuf.printf;
import std;

using namespace NFA::IR;

namespace NFA::TNFA {
    // Appends one fresh state and returns its index. Every wire* helper uses
    // this instead of "current_state + 1", so two members wired from the same
    // state never share intermediate states.
    template <typename States>
    std::size_t allocStateIn(States &all_states) {
        all_states.emplace_back();
        return all_states.size() - 1;
    }

    auto generateVariable(const TokenID &source) {
        std::ostringstream v;
        v << source.member << "#" << source.token_name;
        if (source.group != NULL_STATE) {
            v << "$group" << source.group;
        }
        if (source.call != NULL_STATE) {
            v << "$call" << source.call;
        }
        if (!source.alt.empty()) {
            v << "$alt[";
            for (const auto &alt : source.alt) {
                v << alt << ":";
            }
            v.seekp(-1, std::ios_base::cur); // Remove the trailing ':'
            v << "]";
        }
        v << '{' << source.position_in_token << '}';
        return v.str();
    }

    // A capture with an Array value keeps every iteration (history register); any other is one span.
    auto captureIsList(const TokenID &capture) -> bool {
        return LLIR::BuilderBase::deduceVarTypeByRuleMember(capture.member).getValueType() ==
               LangAPI::ValueType::Array;
    }

    // Whether `id` is (transitively) inside `capture`. `TokenID::capture`
    // is stored outer-to-inner (the order the parser descends through
    // nested capture groups), which is exactly the order BEGIN/END must
    // fire in, so callers walk it directly instead of collecting into an
    // unordered set and losing that order.
    auto has_capture(const TokenID &id, const TokenID *capture) -> bool {
        return std::find(
            id.capture.begin(), id.capture.end(), capture
        ) != id.capture.end();
    }

    using BoundaryEvents = decltype(CaptureBoundaries{}.events);

    // One incoming edge of a TokenID: which predecessor reaches it and which
    // BEGIN/END events THAT predecessor (and only that one) has to fire.
    struct IncomingEdge {
        std::string pred;        // generateVariable(pred); empty for a root
        BoundaryEvents events;
    };
    struct IncomingEdges {
        std::unordered_map<std::string, std::vector<IncomingEdge>> by_next;
        // next-keys whose predecessors disagree on the events to fire.
        std::unordered_set<std::string> split;
    };

    auto event_signature(const BoundaryEvents &events) -> std::string {
        std::string sig;
        for (const auto &e : events) {
            sig += e.kind == CaptureBoundaries::Kind::Begin ? 'B' : 'E';
            sig += generateVariable(e.capture);
            sig += '\n';
        }
        return sig;
    }

    // Capture boundaries are per EDGE (pred -> next), not per node. Merging
    // them per node piled the ENDs of every predecessor onto the one edge that
    // consumes the shared successor, so `"!="` reaching a joined successor
    // fired END for `"=="` too, and the join edge itself carried nothing.
    auto generate_incoming_edges(const Token &token) -> IncomingEdges {
        IncomingEdges result;

        utype::unordered_set<TokenID> reachable;
        for (const auto &[sym, next_transitions] : token.transitions) {
            for (const auto &next : next_transitions) {
                if (!next.member.empty() && !(next == sym))
                    reachable.insert(next);
            }
        }

        auto add = [&](const std::string &next_key, std::string pred, BoundaryEvents events) {
            auto &list = result.by_next[next_key];
            for (const auto &e : list) {
                if (e.pred == pred)
                    return;
            }
            list.push_back(IncomingEdge{std::move(pred), std::move(events)});
        };

        for (const auto &[sym, next_transitions] : token.transitions) {
            const std::string sym_key = generateVariable(sym);

            if (!reachable.contains(sym) && !sym.capture.empty()) {
                BoundaryEvents events;
                for (const auto *cap : sym.capture) {
                    events.push_back({
                        .kind = CaptureBoundaries::Kind::Begin,
                        .position = CaptureBoundaries::Position::Entry,
                        .capture = *cap,
                    });
                }
                add(sym_key, "", std::move(events));
            }

            for (const auto &next : next_transitions) {
                if (next.member.empty())
                    continue; // ACCEPT: handled directly in visit(), not here.

                const std::string next_key = generateVariable(next);
                if (next_key == sym_key)
                    continue; // self-loop: wired by wireSelfLoop, never via visit()

                BoundaryEvents events;
                for (auto it = sym.capture.rbegin(); it != sym.capture.rend(); ++it) {
                    if (!has_capture(next, *it)) {
                        events.push_back({
                            .kind = CaptureBoundaries::Kind::End,
                            .position = CaptureBoundaries::Position::Entry,
                            .capture = **it,
                        });
                    }
                }
                for (const auto *cap : next.capture) {
                    if (!has_capture(sym, cap)) {
                        events.push_back({
                            .kind = CaptureBoundaries::Kind::Begin,
                            .position = CaptureBoundaries::Position::Entry,
                            .capture = *cap,
                        });
                    }
                }
                add(next_key, sym_key, std::move(events));
            }
        }

        for (const auto &[key, list] : result.by_next) {
            const auto first = event_signature(list.front().events);
            for (const auto &e : list) {
                if (event_signature(e.events) != first) {
                    result.split.insert(key);
                    break;
                }
            }
        }
        return result;
    }
    void TNFABuilder::build() {
        Tlog::Branch b(logger, "NFA.TNFA/build");

        for (const auto &[name, token] : ir.get()) {
            ExpansionStack expansion_stack;
            token_entry.emplace(name, states.size());
            wireToken(name, token, expansion_stack);
        }
        if (!debug) {
            for (auto &s : states) {
                for (auto &tv : s.transitions) {
                    for (auto &t : tv.second) {
                        t.char_origin = std::nullopt;
                        t.source = {};
                    }
                }
            }
        }
    }
    auto TNFABuilder::wireString(
        std::size_t current_state,
        const TokenID &source
    ) -> std::size_t {
        const std::string &str = source.member.getString().value;
        std::size_t i = 0;
        for (const auto c : str) {
            // Always a FRESH state. Deriving it from current_state (+1) made
            // every alternative that starts at the same state reuse the same
            // physical states by depth ("int" / "str" / "bool" shared 18..20),
            // which accepted cross-products like "itr".
            const std::size_t next = allocStateIn(states);

            states[current_state].transitions[c].emplace_back(
                TransitionValue{
                    .next = next,
                    .priority = next_priority(),
                    .source = source,
                    .char_origin = debug
                        ? std::optional<CharOrigin>{CharOrigin{
                            .source = source,
                            .rule_run = current_state,
                            .offset = i,
                            .length = str.size(),
                            .ch = c
                        }}
                        : std::nullopt
                }
            );
            ++i;

            current_state = next;
        }

        return current_state;
    }
    auto TNFABuilder::wireAny(std::size_t current_state, const TokenID &source) -> std::size_t {
        const std::size_t next = allocStateIn(states);
        for (int c = 0; c < ALPHABET_SIZE; ++c) {
            states[current_state].transitions[c].emplace_back(
                TransitionValue{
                    .next = next,
                    .priority = next_priority(),
                    .source = source,
                    .char_origin = debug
                        ? std::optional<CharOrigin>{CharOrigin {
                            .source = source,
                            .rule_run = current_state,
                            .offset = 0,
                            .length = 1,
                            .ch = (char) c
                        }}
                        : std::nullopt
                }
            );
        }
        return next;
    }
    auto TNFABuilder::wireCsequence(std::size_t current_state, const TokenID &source) -> std::size_t {
        const auto &cseq = source.member.getCsequence();
        std::array<bool, ALPHABET_SIZE> chars_to_wire;
        chars_to_wire.fill(cseq.negative);
        for (const auto c : cseq.characters) {
            chars_to_wire[c] = !cseq.negative;
        }
        for (const auto c : cseq.escaped) {
            chars_to_wire[corelib::text::getEscapedFromChar(c)] = !cseq.negative;
        }
        for (const auto [from, to] : cseq.diapasons) {
            for (char c = from; c <= to; ++c) {
                chars_to_wire[c] = !cseq.negative;
            }
        }

        const std::size_t next = allocStateIn(states);
        for (int c = 0; c < ALPHABET_SIZE; ++c) {
            if (chars_to_wire[c]) {
                states[current_state].transitions[(char) c].emplace_back(
                    TransitionValue{
                        .next = next,
                        .priority = next_priority(),
                        .source = source,
                        .char_origin = debug
                            ? std::optional<CharOrigin>{CharOrigin {
                                .source = source,
                                .rule_run = current_state,
                                .offset = 0,
                                .length = 1,
                                .ch = (char) c
                            }}
                            : std::nullopt
                    }
                );
            }
        }
        return next;
    }
    auto TNFABuilder::wireEscaped(std::size_t current_state, const TokenID &source) -> std::size_t {
        const auto &esc = source.member.getEscaped();
        const char c = corelib::text::getEscapedFromChar(esc.c);
        const std::size_t next = allocStateIn(states);
        states[current_state].transitions[c].emplace_back(
            TransitionValue{
                .next = next,
                .priority = next_priority(),
                .source = source,
                .char_origin = debug
                    ? std::optional<CharOrigin>{CharOrigin {
                        .source = source,
                        .rule_run = current_state,
                        .offset = 0,
                        .length = 1,
                        .ch = c
                    }}
                    : std::nullopt
            }
        );
        return next;
    }
    // Fires `actions` once, without consuming input, then continues from a
    // fresh state. Kept in its own table (not `transitions`) so cloning
    // operations that only walk the char-indexed table — e.g. wireSelfLoop —
    // can never duplicate them.
    auto TNFABuilder::wireEpsilonActions(
        std::size_t current_state,
        ActionChain actions
    ) -> std::size_t {
        const std::size_t next = allocStateIn(states);
        states[current_state].epsilon_transitions.insert(
            TransitionValue{
                .next = next,
                .priority = next_priority(),
                .actions = std::move(actions),
            }
        );
        return next;
    }

    auto TNFABuilder::wireMember(
        std::size_t current_state,
        const TokenID &source,
        std::unordered_map<std::string, CaptureBoundaries> &capture_boundaries
    ) -> std::size_t {

        // Everything wired below lives at indices >= first_new, plus edges
        // leaving current_state itself.
        const std::size_t first_new = states.size();

        std::size_t end_state;
        if (source.member.isString()) {
            end_state = wireString(current_state, source);
        } else if (source.member.isAny()) {
            end_state = wireAny(current_state, source);
        } else if (source.member.isCsequence()) {
            end_state = wireCsequence(current_state, source);
        } else if (source.member.isEscaped()) {
            end_state = wireEscaped(current_state, source);
        } else if (source.member.isNospace()) {
            return current_state;
        } else throw Error("NFA.TNFA.cpp: wireMember: cannot wire member {}", source.member);
        ActionChain entry_actions;
        ActionChain exit_actions;

        // Split by Position, not by Kind: an End event can mean "this
        // member's own capture closes here" (Exit) or "some other capture
        // closes the moment this member is entered" (Entry) — see
        // generate_capture_boundaries. Kind only decides BEGIN vs END on
        // the emitted action; Position decides which physical transition
        // it lands on. Order within each list is preserved as computed,
        // so nested captures still fire in correct nesting order.
        if (const auto it = capture_boundaries.find(generateVariable(source)); it != capture_boundaries.end()) {

            for (const auto &event : it->second.events) {
                ActionState action{
                    .action = event.kind == CaptureBoundaries::Kind::Begin
                        ? Action::BEGIN
                        : Action::END,
                    .variable = generateVariable(event.capture),
                    .operand_a = captureId(event.capture),
                    .before_char = event.position == CaptureBoundaries::Position::Entry,
                    .list_capture = captureIsList(event.capture),
                };

                if (event.position == CaptureBoundaries::Position::Entry) {
                    entry_actions.push_back(std::move(action));
                } else {
                    exit_actions.push_back(std::move(action));
                }
            }
        }

        // Entry actions belong on the transition(s) consuming this member's
        // FIRST character, i.e. the ones leaving `current_state`. Exit
        // actions belong on the transition(s) consuming its LAST character,
        // i.e. the ones ARRIVING at `end_state`.
        //
        // States are allocated fresh now, so "end_state - 1" no longer names
        // the predecessor (for a one-character member it is current_state,
        // which can be anywhere). The predecessor is found by looking for
        // this member's edges that land on end_state, among the states this
        // call created plus current_state.
        auto attach_from = [&](std::size_t state_id, const ActionChain &to_attach,
                               std::optional<std::size_t> only_into) {
            if (to_attach.empty()) return;
            for (auto &[_, transitions] : states[state_id].transitions) {
                for (auto &t : transitions) {
                    if (!(t.source == source)) continue;
                    if (only_into && t.next != *only_into) continue;
                    t.actions.insert(
                        t.actions.end(),
                        to_attach.begin(),
                        to_attach.end()
                    );
                }
            }
        };

        attach_from(current_state, entry_actions, std::nullopt);

        if (!exit_actions.empty()) {
            attach_from(current_state, exit_actions, end_state);
            for (std::size_t s = first_new; s < end_state; ++s)
                attach_from(s, exit_actions, end_state);
        }

        return end_state;
    }
    void TNFABuilder::wireSelfLoop(std::size_t pred_state, std::size_t state) {
        auto transitions_copy = states[pred_state].transitions;
        for (auto &[c, tv] : transitions_copy) {
            for (const auto &t : tv) {
                if (t.next != state) continue;
                auto loop_edge = t;
                loop_edge.next = state;
                states[state].transitions[c].emplace_back(std::move(loop_edge));
            }
        }
    }
    auto TNFABuilder::wireToken(
        const stdu::vector<std::string> &name,
        const Token &token,
        ExpansionStack &expansion_stack
    ) -> void {
        Tlog::Branch b(logger, "NFA.TNFA/wireToken");

        // Canonical identity (member + token_name + group + call + position) ->
        // the FSM state reached AFTER that TokenID has been matched. Keying on
        // this is what lets the same call-site (e.g. __WSTOKEN$call[1]) collapse
        // onto one physical state no matter how many sources reference it, and
        // lets a self-referencing alt become a real back-edge instead of being
        // re-wired from scratch.
        std::unordered_map<std::string, std::size_t> end_of;

        // key -> the state `id` was first wired FROM. Needed so a later
        // predecessor that reaches an already-wired id (a shared successor,
        // or a loop back to an earlier element) can be joined to it.
        std::unordered_map<std::string, std::size_t> entry_of;

        // Index the edge table once by canonical key instead of re-scanning it
        // for every alt.
        std::unordered_map<std::string, const stdu::vector<TokenID> *> outgoing;
        for (const auto &transition : token.transitions) {
            outgoing.emplace(generateVariable(transition.first), &transition.second);
        }
        const auto incoming = generate_incoming_edges(token);
        static const BoundaryEvents no_events;
        auto events_for = [&](const std::string &key, const std::string &pred) -> const BoundaryEvents & {
            if (const auto it = incoming.by_next.find(key); it != incoming.by_next.end()) {
                for (const auto &e : it->second) {
                    if (e.pred == pred)
                        return e.events;
                }
            }
            return no_events;
        };
        auto to_actions = [&](const BoundaryEvents &events) {
            ActionChain chain;
            for (const auto &event : events) {
                chain.push_back(ActionState{
                    .action = event.kind == CaptureBoundaries::Kind::Begin ? Action::BEGIN : Action::END,
                    .variable = generateVariable(event.capture),
                    .operand_a = captureId(event.capture),
                    .before_char = event.position == CaptureBoundaries::Position::Entry,
                    .list_capture = captureIsList(event.capture),
                });
            }
            return chain;
        };
        // (from, to, pred) triples that already have their join epsilon.
        std::set<std::tuple<std::size_t, std::size_t, std::string>> joined;

        std::function<std::size_t(std::size_t, const TokenID &, const std::string &)> visit;
        visit = [&](std::size_t current_state, const TokenID &id, const std::string &pred) -> std::size_t {
            if (id.member.empty())
                return current_state;

            const std::string key = generateVariable(id);
            const bool is_split = incoming.split.contains(key);
            const BoundaryEvents &my_events = events_for(key, pred);
            const bool is_terminal =
                id.member.isString() ||
                id.member.isAny() ||
                id.member.isCsequence() ||
                id.member.isEscaped() ||
                id.member.isNospace();

            if (const auto cached = end_of.find(key); cached != end_of.end()) {
                // `id` is already wired: join this predecessor to it.
                if (const auto entry = entry_of.find(key);
                    entry != entry_of.end() && entry->second != current_state &&
                    joined.emplace(current_state, entry->second, pred).second) {
                    if (is_split && is_terminal) {
                        // The predecessors disagree on which captures to
                        // open/close when `id` is entered. BEGIN/END are entry
                        // actions of the consuming edge (the runtime appends
                        // the consumed char AFTER the action, so an END fired
                        // any earlier — e.g. on an epsilon at the end of the
                        // previous member — would cut the last char off the
                        // capture). So give this predecessor its OWN copy of
                        // id's first-character edges, carrying only its events.
                        const auto entry_transitions = states[entry->second].transitions;
                        for (const auto &[c, tvs] : entry_transitions) {
                            for (const auto &t : tvs) {
                                if (!(t.source == id))
                                    continue;
                                auto clone = t;
                                clone.actions = to_actions(my_events);
                                states[current_state].transitions[c].push_back(std::move(clone));
                            }
                        }
                    } else {
                        // Identical events for every predecessor: they sit on
                        // the shared consuming edge, a plain epsilon suffices.
                        states[current_state].epsilon_transitions.insert(
                            TransitionValue{
                                .next = entry->second,
                                .priority = next_priority(),
                            }
                        );
                    }
                }
                return cached->second;
            }

            std::size_t end_state;

            // First wiring: the events of THIS predecessor go on the
            // consuming edge(s); later predecessors get their own clone.
            std::unordered_map<std::string, CaptureBoundaries> edge_boundaries;
            if (!my_events.empty())
                edge_boundaries[key].events = my_events;

            if (is_terminal) {
                end_state = wireMember(current_state, id, edge_boundaries);

            } else {
                if (expansion_stack.contains(id))
                    return current_state;

                const auto it = ir.get().find(id.token_name);
                if (it == ir.get().end()) {
                    throw Error(
                        "NFA.TNFA: cannot recursively expand token '{}'",
                        id.token_name
                    );
                }

                expansion_stack.insert(id);

                end_state = current_state;
                const Token &nested = it->second;
                for (const auto &transition : nested.transitions) {
                    if (transition.first != id)
                        continue;
                    for (const auto &alt : transition.second) {
                        end_state = visit(current_state, alt, pred);
                    }
                }

                expansion_stack.erase(id);
            }

            // Record BEFORE walking our own outgoing edges...
            end_of.emplace(key, end_state);
            entry_of.emplace(key, current_state);

            ActionChain capture_actions;
            if (!is_terminal)
                capture_actions = to_actions(my_events);

            // Only a genuine nested-token reference (`@ SYMBOL`) reduces into its own
            // Token instance here. A plain group capture (`@ ( ... )` around raw
            // leaves of THIS token) is terminal and must never trigger a reduce — it
            // only ever contributes BEGIN/END action cells on the real edges below.
            if (!is_terminal && !id.capture.empty()) {
                markAccept(end_state, id, token, end_state, capture_actions, /*nested=*/true);
            }

            if (const auto edges = outgoing.find(key); edges != outgoing.end()) {
                for (const auto &alt : *edges->second) {
                    if (alt.member.empty()) {
                        // ACCEPT / sentinel: id completing here IS the token ending.
                        // Whatever captures are still open on `id` close specifically
                        // on THIS edge, regardless of whether `id` also has other
                        // (e.g. self-loop) successors that keep them open elsewhere.
                        ActionChain accept_capture_actions = capture_actions;
                        for (auto it = id.capture.rbegin(); it != id.capture.rend(); ++it) {
                            accept_capture_actions.push_back(ActionState{
                                .action = Action::END,
                                .variable = generateVariable(**it),
                                .operand_a = captureId(**it),
                                .list_capture = captureIsList(**it),
                            });
                        }
                        if (is_terminal || id.capture.empty()) {
                            markAccept(end_state, id, token, end_state, accept_capture_actions);
                        }
                        continue;
                    }
                    if (generateVariable(alt) == key) {
                        wireSelfLoop(current_state, end_state);
                        // wireSelfLoop clones the consuming edge together with
                        // the entry BEGIN/END actions it carries. A loop
                        // iteration opens/closes nothing (same captures on
                        // both sides), so those must not repeat per char.
                        for (auto &[c, tvs] : states[end_state].transitions) {
                            for (auto &t : tvs) {
                                if (t.next == end_state && t.source == id)
                                    t.actions.clear();
                            }
                        }
                        continue;
                    }
                    visit(end_state, alt, key);
                }
            }

            return end_state;
        };

        const std::size_t initial_state = states.size();
        states.emplace_back();
        std::unordered_set<std::size_t> ends;
        // Only TokenIDs that never appear as a destination alt anywhere in this
        // token are real entry points — everything else is reached while
        // walking someone else's alt list, and must not be re-rooted here.
        std::unordered_set<std::string> referenced;
        for (const auto &transition : token.transitions) {
            const std::string self_key = generateVariable(transition.first);
            for (const auto &alt : transition.second) {
                // A self-loop (`[ws]+`) must not make an id look "referenced":
                // its only referrer is itself, so it is still an entry point.
                if (!alt.member.empty() && generateVariable(alt) != self_key) {
                    referenced.insert(generateVariable(alt));
                }
            }
        }

        for (const auto &transition : token.transitions) {
            if (referenced.contains(generateVariable(transition.first)))
                continue;
            ends.insert(visit(initial_state, transition.first, std::string{}));
        }

        // Entry points that are also the target of a back-edge from another
        // id (e.g. the first element of `( A B )+`) are "referenced" and were
        // skipped above; if nothing reached them they are still unwired.
        // Wire those from the initial state, earliest position first.
        {
            std::vector<const TokenID *> unvisited;
            for (const auto &transition : token.transitions) {
                if (!end_of.contains(generateVariable(transition.first)))
                    unvisited.push_back(&transition.first);
            }
            std::stable_sort(unvisited.begin(), unvisited.end(),
                [](const TokenID *a, const TokenID *b) {
                    return a->position_in_token < b->position_in_token;
                });
            for (const auto *id : unvisited) {
                if (end_of.contains(generateVariable(*id)))
                    continue; // reached while wiring an earlier one
                ends.insert(visit(initial_state, *id, std::string{}));
            }
        }
    }
    auto TNFABuilder::captureId(const TokenID &capture) -> std::size_t {
        return capture_ids.emplace(generateVariable(capture), capture_ids.size()).first->second;
    }

    void TNFABuilder::markAccept(
        std::size_t state_id,
        const TokenID &tail,
        const Token &token,
        std::size_t next,
        ActionChain capture_actions,
        bool nested
    ) {
        TokenBinding binding{
            .token_id = state_id,
            .is_unique_representation = token.top_level,
        };

        SemanticState state;

        // Builds the reduction: reads captures through the `captures` parameter (a scalar capture is
        // text(k)/character(k), a list capture is list<T>(k)). Nested tokens are reduced inline.
        const std::string prefix =
            nested ? "n" + std::to_string(state_id) + "_" : std::string{};
        const auto local = [&](const std::string &name) { return prefix + name; };
        const auto type_name = tail.prev ? tail.prev->token_name : tail.token_name;

        const auto symbol = [](const std::string &name) {
            return LangAPI::Symbol::createExpression(LangAPI::Symbol{name});
        };
        const auto number = [](std::size_t v) {
            return LangAPI::Int::createExpression(LangAPI::Int{.value = static_cast<long long>(v)});
        };

        // captures.<method><template_types>(k)
        const auto capture_call =
            [&](const std::string &method,
                std::size_t k,
                const std::vector<LangAPI::Type> &template_types = {}) {
                LangAPI::FunctionCall call{
                    .name = std::make_shared<LangAPI::Symbol>(LangAPI::Symbol{method}),
                    .args = {number(k)}
                };
                for (const auto &t : template_types)
                    call.template_parameters.push_back(std::make_shared<LangAPI::Type>(t));
                LangAPI::StorageSymbol storage;
                storage.what = symbol("captures");
                storage.path = {std::move(call)};
                return LangAPI::StorageSymbol::createExpression(storage);
            };

        // The expression that produces a scalar capture as `t`.
        const auto scalar_expression =
            [&](const LangAPI::Type &t, std::size_t k, const std::string &what) {
                if (t.isValueType()) {
                    switch (t.getValueType()) {
                        case LangAPI::ValueType::Char:
                            return capture_call("character", k);
                        case LangAPI::ValueType::String:
                            return capture_call("text", k);
                        default:
                            break;
                    }
                }
                throw Error("NFA.TNFA: cannot read the capture of '{}' as this type", what);
            };

        const auto capture_of = [](const TokenID &sym, bool same_member) -> const TokenID * {
            for (const auto *cap : sym.capture) {
                if (cap->token_name != sym.token_name)
                    continue;
                if (same_member && !(cap->member == sym.member))
                    continue;
                return cap;
            }
            return nullptr;
        };

        // Declares `name` and fills it from the captures of member `sym`.
        const auto read_member =
            [&](const TokenID &sym,
                const TokenID *cap,
                const std::string &name,
                LangAPI::Statements &out) {
                const auto &member = sym.member;
                const LangAPI::Type t = LLIR::BuilderBase::deduceVarTypeByRuleMember(
                    cap ? cap->original_member : sym.original_member);

                // Nested token: its reduction was built when it was wired.
                if (member.isName() && !member.getName().isTerminal()) {
                    const auto it = nested_reductions.find(generateVariable(sym));
                    if (it == nested_reductions.end()) {
                        throw Error(
                            "NFA.TNFA: the reduction of nested token '{}' was not built before '{}'",
                            sym.token_name, tail.token_name
                        );
                    }
                    out.insert(out.end(), it->second.statements.begin(), it->second.statements.end());
                    out.push_back(LangAPI::Variable::createStatement(LangAPI::Variable{
                        .name = name,
                        .type = t,
                        .value = symbol(it->second.result),
                    }));
                    return;
                }

                if (!cap)
                    throw Error("NFA.TNFA: member of '{}' has no capture to read", sym.token_name);
                const std::size_t k = captureId(*cap);

                // Every iteration of a loop.
                if (captureIsList(*cap)) {
                    if (t.template_parameters.empty())
                        throw Error("NFA.TNFA: list capture of '{}' has no element type", sym.token_name);
                    const auto &elem = std::get<LangAPI::Type>(t.template_parameters.front());
                    if (!elem.isValueType() ||
                        (elem.getValueType() != LangAPI::ValueType::Char &&
                         elem.getValueType() != LangAPI::ValueType::String)) {
                        throw Error(
                            "NFA.TNFA: a list of nested tokens is not supported yet ('{}')",
                            sym.token_name
                        );
                    }
                    out.push_back(LangAPI::Variable::createStatement(LangAPI::Variable{
                        .name = name,
                        .type = t,
                        .value = capture_call("list", k, {elem}),
                    }));
                    return;
                }

                // A union of alternatives: each alternative is anchored by its own
                // capture and the one that was recorded wins.
                if (t.isValueType() && t.getValueType() == LangAPI::ValueType::Variant) {
                    out.push_back(LangAPI::Variable::createStatement(LangAPI::Variable{.name = name, .type = t}));

                    std::vector<const TokenID *> alternatives;
                    for (const auto *c : sym.capture)
                        if (c->token_name == sym.token_name)
                            alternatives.push_back(c);

                    LangAPI::Statements chain;
                    for (std::size_t i = alternatives.size(); i-- > 0;) {
                        const auto *alt = alternatives[i];
                        const std::size_t alt_k = captureId(*alt);
                        LangAPI::If check{capture_call("is_set", alt_k)};
                        check.stmt.push_back(LangAPI::VariableAssignment::createStatement(
                            LangAPI::VariableAssignment{
                                .name = LangAPI::Symbol{name},
                                .value = scalar_expression(
                                    LLIR::BuilderBase::deduceVarTypeByRuleMember(alt->original_member),
                                    alt_k, name),
                            }));
                        check.else_stmt = std::move(chain);
                        chain = LangAPI::Statements{};
                        chain.push_back(LangAPI::If::createStatement(check));
                    }
                    out.insert(out.end(), chain.begin(), chain.end());
                    return;
                }

                out.push_back(LangAPI::Variable::createStatement(LangAPI::Variable{
                    .name = name,
                    .type = t,
                    .value = scalar_expression(t, k, name),
                }));
            };

        if (token.data_block && token.data_block->isRegularDataBlock()) {
            std::cout << "Assigning at if{}" << std::endl;

            state.instance_value.name = LangAPI::Symbol{type_name};

            for (const auto &[sym, alternatives] : token.transitions) {
                if (sym.token_name == tail.token_name &&
                    !sym.member.empty() &&
                    !sym.capture.empty()) {
                    read_member(sym, capture_of(sym, false), local("value"), state.statements);
                    state.instance_value.args.push_back(symbol(local("value")));
                    break;
                }
            }
        } else if (
            token.data_block &&
            token.data_block->isTemplatedDataBlock()
        ) {
            std::cout << "Assigning at else if{}" << std::endl;

            const auto &data_block = token.data_block->getTemplatedDataBlock();

            stdu::vector<const TokenID *> members_with_prefix;
            for (const auto &[sym, alternatives] : token.transitions) {
                if (sym.token_name == tail.token_name &&
                    !sym.member.empty() &&
                    !sym.member.prefix.empty() &&
                    std::find(members_with_prefix.begin(), members_with_prefix.end(), &sym) ==
                        members_with_prefix.end()) {
                    members_with_prefix.push_back(&sym);
                }
            }

            std::sort(
                members_with_prefix.begin(),
                members_with_prefix.end(),
                [](const TokenID *a, const TokenID *b) {
                    return a->position_in_token < b->position_in_token;
                }
            );

            state.instance_value.name = LangAPI::Symbol{type_name};

            const std::size_t limit = std::min(data_block.names.size(), members_with_prefix.size());
            for (std::size_t i = limit; i-- > 0;) {
                const TokenID &sym = *members_with_prefix[i];
                const std::string name = local(data_block.names[i]);
                read_member(sym, capture_of(sym, true), name, state.statements);
                state.instance_value.args.push_back(symbol(name));
            }

            std::reverse(
                state.instance_value.args.begin(),
                state.instance_value.args.end()
            );
        } else {
            std::cout << "Assigning at else{}" << std::endl;
            state.instance_value =
                LangAPI::Inheritance{
                .name = LangAPI::Symbol{type_name}
                };
        }

        // Preserve the existing semantic ABI.
        std::reverse(
            state.instance_value.args.begin(),
            state.instance_value.args.end()
        );

        if (nested) {
            // not an accept point: inlined into the enclosing token's reduction
            NestedReduction reduction;
            reduction.statements = state.statements;
            reduction.result = local("result");
            reduction.statements.push_back(LangAPI::Variable::createStatement(LangAPI::Variable{
                .name = reduction.result,
                .type = LangAPI::Type{LangAPI::Symbol{type_name}},
                .value = LangAPI::Inheritance::createExpression(state.instance_value),
            }));
            nested_reductions[generateVariable(tail)] = std::move(reduction);
            return;
        }

        state.nfa_index = state_id;
        state.next_state = DFATarget{
            .id = next,
            .debug = debug
        };

        capture_actions.push_back(state);
        states[state_id].epsilon_transitions.insert(
            TransitionValue {.actions = std::move(capture_actions)}
        );

        states[state_id].accept_binding = binding;
        accept_map.emplace(state_id, binding);
    }
}
// ============================================================================
// Debug printing
// ============================================================================
std::ostream &
operator<<(
    std::ostream &os,
    const NFA::TNFA::TNFABuilder &b
) {
    const auto &states = b.getStates();

    auto print_uc = [&os](unsigned char uc) {
        if (std::isprint(static_cast<int>(uc))) {
            os << '\'' << static_cast<char>(uc) << '\'';
        } else {
            os << "\\x"
               << std::hex
               << std::setw(2)
               << std::setfill('0')
               << static_cast<unsigned>(uc)
               << std::dec
               << std::setfill(' ');
        }
    };

    for (std::size_t id = 0; id < states.size(); ++id) {
        os << "state #" << id << '\n';

        using TransitionList =
            std::remove_reference_t<
                decltype(states[id].transitions.begin()->second)
            >;

        std::vector<
            std::pair<unsigned char, const TransitionList *>
        > sorted_trans;

        sorted_trans.reserve(states[id].transitions.size());

        for (const auto &[ch, transitions] : states[id].transitions) {
            sorted_trans.emplace_back(
                static_cast<unsigned char>(ch),
                &transitions
            );
        }

        std::sort(
            sorted_trans.begin(),
            sorted_trans.end(),
            [](const auto &a, const auto &b) {
                return a.first < b.first;
            }
        );

        std::size_t i = 0;

        while (i < sorted_trans.size()) {
            const unsigned char start_uc = sorted_trans[i].first;
            unsigned char end_uc = start_uc;

            const auto &range_transitions =
                *sorted_trans[i].second;

            std::size_t j = i + 1;

            while (j < sorted_trans.size()) {
                const unsigned char current_uc =
                    sorted_trans[j].first;

                // Don't allow unsigned-char overflow at 0xff.
                const bool contiguous =
                    static_cast<unsigned>(current_uc) ==
                    static_cast<unsigned>(end_uc) + 1;

                const bool same_transitions =
                    *sorted_trans[j].second == range_transitions;

                if (!contiguous || !same_transitions) {
                    break;
                }

                end_uc = current_uc;
                ++j;
            }

            for (const auto &t : range_transitions) {
                os << "  --";

                if (start_uc == end_uc) {
                    print_uc(start_uc);
                } else {
                    os << '[';
                    print_uc(start_uc);
                    os << '-';
                    print_uc(end_uc);
                    os << ']';
                }

                os << "--> "
                   << t.next
                   << ' '
                   << t
                   << '\n';
            }

            i = j;
        }

        for (const auto &e : states[id].epsilon_transitions) {
            os << "  --epsilon--> "
               << e
               << '\n';
        }

        if (states[id].accept_binding) {
            os << "  (accept) "
               << *states[id].accept_binding
               << '\n';
        }

        if (id + 1 < states.size()) {
            os << '\n';
        }
    }

    return os;
}