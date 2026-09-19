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

    auto generate_capture_boundaries(const Token &token) {
        std::unordered_map<std::string, CaptureBoundaries> result;

        utype::unordered_set<TokenID> reachable;
        for (const auto &[sym, next_transitions] : token.transitions) {
            for (const auto &next : next_transitions) {
                if (!next.member.empty())
                    reachable.insert(next);
            }
        }

        for (const auto &[sym, next_transitions] : token.transitions) {
            if (!reachable.contains(sym) && !sym.capture.empty()) {
                auto &boundary = result[generateVariable(sym)];
                for (const auto *cap : sym.capture) {
                    boundary.events.push_back({
                        .kind = CaptureBoundaries::Kind::Begin,
                        .position = CaptureBoundaries::Position::Entry,
                        .capture = *cap,
                    });
                }
            }

            for (const auto &next : next_transitions) {
                if (next.member.empty())
                    continue; // ACCEPT: handled directly in visit(), not here.

                auto &boundary = result[generateVariable(next)];

                for (auto it = sym.capture.rbegin(); it != sym.capture.rend(); ++it) {
                    if (!has_capture(next, *it)) {
                        boundary.events.push_back({
                            .kind = CaptureBoundaries::Kind::End,
                            .position = CaptureBoundaries::Position::Entry,
                            .capture = **it,
                        });
                    }
                }
                for (const auto *cap : next.capture) {
                    if (!has_capture(sym, cap)) {
                        boundary.events.push_back({
                            .kind = CaptureBoundaries::Kind::Begin,
                            .position = CaptureBoundaries::Position::Entry,
                            .capture = *cap,
                        });
                    }
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
        auto capture_boundaries = generate_capture_boundaries(token);
        std::function<std::size_t(std::size_t, const TokenID &)> visit;
        visit = [&](std::size_t current_state, const TokenID &id) -> std::size_t {
            if (id.member.empty())
                return current_state;

            const std::string key = generateVariable(id);

            if (const auto cached = end_of.find(key); cached != end_of.end()) {
                // `id` is already wired. Returning silently dropped this edge:
                // it only appeared to work while alternatives happened to share
                // physical states. Join this predecessor to the existing entry
                // with an epsilon edge (also gives loops their back-edge).
                if (const auto entry = entry_of.find(key);
                    entry != entry_of.end() && entry->second != current_state) {
                    bool linked = false;
                    for (const auto &e : states[current_state].epsilon_transitions) {
                        if (e.next == entry->second && e.actions.empty()) {
                            linked = true;
                            break;
                        }
                    }
                    if (!linked) {
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

            const bool is_terminal =
                id.member.isString() ||
                id.member.isAny() ||
                id.member.isCsequence() ||
                id.member.isEscaped() ||
                id.member.isNospace();

            if (is_terminal) {
                end_state = wireMember(current_state, id, capture_boundaries);

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
                        end_state = visit(current_state, alt);
                    }
                }

                expansion_stack.erase(id);
            }

            // Record BEFORE walking our own outgoing edges...
            end_of.emplace(key, end_state);
            entry_of.emplace(key, current_state);

            ActionChain capture_actions;
            if (!is_terminal) {
                if (const auto cb = capture_boundaries.find(generateVariable(id)); cb != capture_boundaries.end()) {
                    for (const auto &event : cb->second.events) {
                        capture_actions.push_back(ActionState{
                            .action = event.kind == CaptureBoundaries::Kind::Begin ? Action::BEGIN : Action::END,
                            .variable = generateVariable(event.capture),
                        });
                    }
                }
            }

            // Only a genuine nested-token reference (`@ SYMBOL`) reduces into its own
            // Token instance here. A plain group capture (`@ ( ... )` around raw
            // leaves of THIS token) is terminal and must never trigger a reduce — it
            // only ever contributes BEGIN/END action cells on the real edges below.
            if (!is_terminal && !id.capture.empty()) {
                markAccept(end_state, id, token, end_state, capture_actions);
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
                            });
                        }
                        if (is_terminal || id.capture.empty()) {
                            markAccept(end_state, id, token, end_state, accept_capture_actions);
                        }
                        continue;
                    }
                    if (generateVariable(alt) == key) {
                        wireSelfLoop(current_state, end_state);
                        continue;
                    }
                    visit(end_state, alt);
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
            for (const auto &alt : transition.second) {
                if (!alt.member.empty()) {
                    referenced.insert(generateVariable(alt));
                }
            }
        }

        for (const auto &transition : token.transitions) {
            if (referenced.contains(generateVariable(transition.first)))
                continue;
            ends.insert(visit(initial_state, transition.first));
        }
    }
    void TNFABuilder::markAccept(
        std::size_t state_id,
        const TokenID &tail,
        const Token &token,
        std::size_t next,
        ActionChain capture_actions
    ) {
        TokenBinding binding{
            .token_id = state_id,
            .is_unique_representation = token.top_level,
        };

        SemanticState state;

        using SymbolFactory =
            std::function<LangAPI::StorageSymbol()>;

        auto symbol_of =
            [](std::string name) -> SymbolFactory {
                return [name] {
                    LangAPI::StorageSymbol s;

                    s.what =
                        LangAPI::Symbol::createExpression(
                            LangAPI::Symbol{name}
                        );

                    return s;
                };
        };

        auto values_slot_at =
            [](std::string array,
               std::size_t offset) -> SymbolFactory {
                return [array, offset] {
                    LangAPI::StorageSymbol s;

                    s.what =
                        LangAPI::Symbol::createExpression(
                            LangAPI::Symbol{array}
                        );

                    s.path = {
                        LangAPI::StorageOffset{
                            .offset =
                                LangAPI::Int::createExpression(
                                    LangAPI::Int{
                                        .value =
                                            static_cast<long long>(offset)
                                    }
                                )
                        }
                    };

                    return s;
                };
        };

        auto unwrap_token =
            [&](const SymbolFactory &source,
                const std::string &name,
                LangAPI::Statements &out) {

                LangAPI::Variable token_v{
                    .name = name + "_token",
                    .type = LangAPI::Type{
                        LangAPI::Symbol{"Token"}
                    },
                    .value =
                        LangAPI::GetVariant::createExpression(
                            LangAPI::GetVariant{
                                .type =
                                    std::make_shared<LangAPI::Type>(
                                        LangAPI::Type{
                                            LangAPI::Symbol{"Token"}
                                        }
                                    ),
                                .sym =
                                    LangAPI::StorageSymbol::createExpression(
                                        source()
                                    )
                            }
                        )
                };

                out.push_back(
                    LangAPI::Variable::createStatement(token_v)
                );

                return token_v.name;
        };

        auto extract_string_or_char =
            [&](const SymbolFactory &source,
                const std::string &target_name,
                LangAPI::Statements &out) {

                LangAPI::If type_check{
                    LangAPI::CheckVariant::createExpression(
                        LangAPI::CheckVariant{
                            .type =
                                std::make_shared<LangAPI::Type>(
                                    LangAPI::ValueType::Char
                                ),
                            .sym =
                                LangAPI::StorageSymbol::createExpression(
                                    source()
                                )
                        }
                    )
                };

                LangAPI::Variable stored_char{
                    .name = target_name + "_stored",
                    .type = LangAPI::ValueType::Char,
                    .value =
                        LangAPI::GetVariant::createExpression(
                            LangAPI::GetVariant{
                                .type =
                                    std::make_shared<LangAPI::Type>(
                                        LangAPI::ValueType::Char
                                    ),
                                .sym =
                                    LangAPI::StorageSymbol::createExpression(
                                        source()
                                    )
                            }
                        )
                };

                type_check.stmt.push_back(
                    LangAPI::Variable::createStatement(stored_char)
                );

                type_check.stmt.push_back(
                    LangAPI::VariableAssignment::createStatement(
                        LangAPI::VariableAssignment{
                            .name = LangAPI::Symbol{target_name},
                            .value =
                                LangAPI::CharToStringConstructor::
                                createExpression(
                                    LangAPI::CharToStringConstructor{
                                        .what =
                                            LangAPI::Symbol::
                                            createExpression(
                                                LangAPI::Symbol{
                                                    stored_char.name
                                                }
                                            )
                                    }
                                )
                        }
                    )
                );

                type_check.else_stmt.push_back(
                    LangAPI::VariableAssignment::createStatement(
                        LangAPI::VariableAssignment{
                            .name = LangAPI::Symbol{target_name},
                            .value =
                                LangAPI::GetVariant::createExpression(
                                    LangAPI::GetVariant{
                                        .type =
                                            std::make_shared<LangAPI::Type>(
                                                LangAPI::ValueType::String
                                            ),
                                        .sym =
                                            LangAPI::StorageSymbol::
                                            createExpression(
                                                source()
                                            )
                                    }
                                )
                        }
                    )
                );

                out.push_back(type_check);
        };

        auto extract_variant_alternative =
            [&](const SymbolFactory &source,
                const std::string &target_name,
                const stdu::vector<LangAPI::Type> &alternatives,
                LangAPI::Statements &out) {

                LangAPI::Statements chain;

                for (std::size_t i = alternatives.size(); i-- > 0;) {
                    const auto &alt = alternatives[i];

                    LangAPI::If type_check{
                        LangAPI::CheckVariant::createExpression(
                            LangAPI::CheckVariant{
                                .type =
                                    std::make_shared<LangAPI::Type>(alt),
                                .sym =
                                    LangAPI::StorageSymbol::
                                    createExpression(source())
                            }
                        )
                    };

                    type_check.stmt.push_back(
                        LangAPI::VariableAssignment::
                        createStatement(
                            LangAPI::VariableAssignment{
                                .name = LangAPI::Symbol{target_name},
                                .value =
                                    LangAPI::GetVariant::
                                    createExpression(
                                        LangAPI::GetVariant{
                                            .type =
                                                std::make_shared<
                                                    LangAPI::Type>(alt),
                                            .sym =
                                                LangAPI::StorageSymbol::
                                                createExpression(source())
                                        }
                                    )
                            }
                        )
                    );

                    type_check.else_stmt = std::move(chain);

                    chain = LangAPI::Statements{};

                    chain.push_back(
                        LangAPI::If::createStatement(type_check)
                    );
                }

                for (auto &stmt : chain)
                    out.push_back(stmt);
        };

        auto assign_from =
            [](const SymbolFactory &source,
               const LangAPI::Type &t,
               const std::string &target_name) {

                return LangAPI::VariableAssignment::
                    createStatement(
                        LangAPI::VariableAssignment{
                            .name = LangAPI::Symbol{target_name},
                            .value =
                                LangAPI::GetVariant::
                                createExpression(
                                    LangAPI::GetVariant{
                                        .type =
                                            std::make_shared<
                                                LangAPI::Type>(t),
                                        .sym =
                                            LangAPI::StorageSymbol::
                                            createExpression(source())
                                    }
                                )
                        }
                    );
        };

        // Keep the two tested helpers from implementation #1.
        auto create_variable_for_access_repeating =
            [&](const LangAPI::Type &t,
                std::string name,
                std::size_t offset,
                const AST::RuleMember &field_member) {

                // <<< keep lines 638-869 of your current commented
                //     implementation unchanged >>>

                LangAPI::Statements statements;

                auto v_type =
                    LangAPI::Type{
                    LangAPI::ValueType::Array,
                    LangAPI::Type{
                        LangAPI::ValueType::Variant,
                        LangAPI::Type{LangAPI::Symbol{"Token"}},
                        LangAPI::Type{LangAPI::ValueType::Char},
                        LangAPI::Type{LangAPI::ValueType::String}
                    }
                    };

                LangAPI::Variable v{
                    .name = name + "_with_variant",
                    .type = v_type
                };

                LangAPI::Variable v_actual{
                    .name = name,
                    .type = t
                };

                LangAPI::Variable i{
                    .name = "i_" + name,
                    .type = LangAPI::ValueType::Int,
                    .value =
                        LangAPI::Int::createExpression(
                            LangAPI::Int{.value = 0}
                        )
                };

                auto vec_values_size = [] {
                    LangAPI::StorageSymbol s;
                    s.what =
                        LangAPI::Symbol::createExpression(
                            LangAPI::Symbol{"vec_values"}
                        );
                    s.path = {
                        LangAPI::ArrayMethodCall{
                            .method = LangAPI::ArrayMethods::Size
                        }
                    };
                    return s;
                };

                LangAPI::While array_loop{
                    LangAPI::Expression{
                        LangAPI::ExpressionValue{
                            LangAPI::Symbol{"i_" + name}
                        },
                        LangAPI::ExpressionValue{
                            LangAPI::ExpressionElement::NotEqual
                        },
                        LangAPI::StorageSymbol::
                            createExpressionValue(vec_values_size())
                    }
                };

                auto element_at_i = [name] {
                    LangAPI::StorageSymbol s;
                    s.what =
                        LangAPI::Symbol::createExpression(
                            LangAPI::Symbol{
                                name + "_with_variant"
                            }
                        );
                    s.path = {
                        LangAPI::StorageOffset{
                            .offset =
                                LangAPI::Symbol::createExpression(
                                    LangAPI::Symbol{"i_" + name}
                                )
                        }
                    };
                    return s;
                };

                LangAPI::Statements loop_body;

                if (field_member.isName() &&
                    field_member.getName().isTerminal()) {

                    const auto token_name =
                        unwrap_token(
                            element_at_i,
                            name,
                            loop_body
                        );

                    const std::string term_name =
                        name + "_term";

                    loop_body.push_back(
                        LangAPI::Variable::createStatement(
                            LangAPI::Variable{
                                .name = term_name,
                                .type = t
                            }
                        )
                    );

                    if (t.getValueType() ==
                        LangAPI::ValueType::String) {

                        extract_string_or_char(
                            symbol_of(token_name),
                            term_name,
                            loop_body
                        );
                        } else {
                            loop_body.push_back(
                                assign_from(
                                    symbol_of(token_name),
                                    t,
                                    term_name
                                )
                            );
                        }

                    LangAPI::StorageSymbol array_push;
                    array_push.what =
                        LangAPI::Symbol::createExpression(
                            LangAPI::Symbol{name}
                        );
                    array_push.path = {
                        LangAPI::ArrayMethodCall{
                            .method = LangAPI::ArrayMethods::Push,
                            .args = {
                                LangAPI::Symbol::createExpression(
                                    LangAPI::Symbol{term_name}
                                )
                            }
                        }
                    };

                    loop_body.push_back(
                        LangAPI::StorageSymbol::createStatement(
                            array_push
                        )
                    );
                    } else {
                        LangAPI::StorageSymbol array_push;
                        array_push.what =
                            LangAPI::Symbol::createExpression(
                                LangAPI::Symbol{name}
                            );
                        array_push.path = {
                            LangAPI::ArrayMethodCall{
                                .method = LangAPI::ArrayMethods::Push,
                                .args = {
                                    LangAPI::GetVariant::
                                        createExpression(
                                            LangAPI::GetVariant{
                                                .type =
                                                    std::make_shared<
                                                        LangAPI::Type>(t),
                                                .sym =
                                                    LangAPI::StorageSymbol::
                                                    createExpression(
                                                        element_at_i()
                                                    )
                                            }
                                        )
                                }
                            }
                        };

                        loop_body.push_back(
                            LangAPI::StorageSymbol::createStatement(
                                array_push
                            )
                        );
                    }

                loop_body.push_back(
                    LangAPI::Expression::createStatement(
                        LangAPI::Expression{
                            LangAPI::ExpressionValue{
                                LangAPI::Symbol{"i_" + name}
                            },
                            LangAPI::ExpressionValue{
                                LangAPI::ExpressionElement::PlusPlus
                            }
                        }
                    )
                );

                array_loop.stmt = loop_body;

                statements.push_back(v);
                statements.push_back(v_actual);
                statements.push_back(i);
                statements.push_back(array_loop);

                return statements;
        };

        auto create_variable_for_access =
            [&](const LangAPI::Type &t,
                std::string name,
                std::size_t offset,
                const AST::RuleMember &field_member) {

                // <<< keep lines 874-1046 of your commented
                //     implementation unchanged >>>

                LangAPI::Statements statements;

                statements.push_back(
                    LangAPI::Variable{
                        .name = name,
                        .type = t
                    }
                );

                auto values_slot =
                    values_slot_at("values", offset);

                const bool is_repeating_csequence_variant =
                    (
                        field_member.isCsequence() &&
                        (
                            field_member.quantifier == '+' ||
                            field_member.quantifier == '*'
                        )
                    ) ||
                    (
                        t.isValueType() &&
                        t.getValueType() ==
                            LangAPI::ValueType::Variant
                    );

                if (is_repeating_csequence_variant) {
                    if (t.getValueType() ==
                        LangAPI::ValueType::String) {

                        extract_string_or_char(
                            values_slot,
                            name,
                            statements
                        );
                        } else if (
                            t.getValueType() ==
                            LangAPI::ValueType::Variant
                        ) {
                            const auto token_name =
                                unwrap_token(
                                    values_slot,
                                    name,
                                    statements
                                );

                            auto token_symbol =
                                symbol_of(token_name);

                            stdu::vector<LangAPI::Type> alternatives;

                            for (auto &tp : t.template_parameters)
                                alternatives.push_back(
                                    std::get<LangAPI::Type>(tp)
                                );

                            extract_variant_alternative(
                                token_symbol,
                                name,
                                alternatives,
                                statements
                            );
                        } else {
                            statements.push_back(
                                assign_from(
                                    values_slot,
                                    t,
                                    name
                                )
                            );
                        }
                } else if (
                    field_member.isName() &&
                    field_member.getName().isTerminal()
                ) {
                    const auto token_name =
                        unwrap_token(
                            values_slot,
                            name,
                            statements
                        );

                    auto token_symbol =
                        symbol_of(token_name);

                    if (t.getValueType() ==
                        LangAPI::ValueType::String) {

                        extract_string_or_char(
                            token_symbol,
                            name,
                            statements
                        );
                        } else if (
                            t.getValueType() ==
                            LangAPI::ValueType::Variant
                        ) {
                            stdu::vector<LangAPI::Type> alternatives;

                            for (auto &tp : t.template_parameters)
                                alternatives.push_back(
                                    std::get<LangAPI::Type>(tp)
                                );

                            extract_variant_alternative(
                                token_symbol,
                                name,
                                alternatives,
                                statements
                            );
                        } else {
                            statements.push_back(
                                assign_from(
                                    token_symbol,
                                    t,
                                    name
                                )
                            );
                        }
                } else {
                    statements.push_back(
                        assign_from(
                            values_slot,
                            t,
                            name
                        )
                    );
                }

                LangAPI::StorageSymbol pop;

                pop.what =
                    LangAPI::Symbol::createExpression(
                        LangAPI::Symbol{"values"}
                    );

                pop.path = {
                    LangAPI::ArrayMethodCall{
                        .method = LangAPI::ArrayMethods::Pop
                    }
                };

                statements.push_back(
                    LangAPI::StorageSymbol::createStatement(pop)
                );

                return statements;
        };

        if (token.data_block && token.data_block->isRegularDataBlock()) {
            std::cout << "Assigning at if{}" << std::endl;

            const auto &data_block =
                token.data_block->getRegDataBlock();

            const AST::RuleMember *mem = nullptr;
            LangAPI::Type type;
            for (const auto &[sym, next] : token.transitions) {
                if (sym.token_name == tail.token_name &&
                    !sym.member.empty() &&
                    !sym.capture.empty()) {
                    for (const auto cap : sym.capture) {
                        if (cap->token_name == sym.token_name) {
                            type = LLIR::BuilderBase::deduceVarTypeByRuleMember(cap->member);
                            break;
                        }
                    }
                    mem = &sym.member;
                    break;
                }
            }
            state.instance_value.name =
                LangAPI::Symbol{tail.prev ? tail.prev->token_name : tail.token_name};
            if (mem) {
                LangAPI::Statements insert_statements =
                    create_variable_for_access(
                        type,
                        "value",
                        0,
                        *mem
                    );

                state.statements.insert(
                    state.statements.end(),
                    insert_statements.begin(),
                    insert_statements.end()
                );

                state.instance_value.args.push_back(
                    LangAPI::Symbol::createExpression(
                        LangAPI::Symbol{"value"}
                    )
                );
            }
        } else if (
            token.data_block &&
            token.data_block->isTemplatedDataBlock()
        ) {
            std::cout << "Assigning at else if{}" << std::endl;

            const auto &data_block =
                token.data_block->getTemplatedDataBlock();

            stdu::vector<const TokenID *> members_with_prefix;

            for (const auto &[sym, next] : token.transitions) {
                if (sym.token_name == tail.token_name &&
                    !sym.member.empty() &&
                    !sym.member.prefix.empty()) {

                    if (std::find(
                            members_with_prefix.begin(),
                            members_with_prefix.end(),
                            &sym
                        ) == members_with_prefix.end()) {

                        members_with_prefix.push_back(&sym);
                    }
                }
            }

            std::sort(
                members_with_prefix.begin(),
                members_with_prefix.end(),
                [](const TokenID *a, const TokenID *b) {
                    return a->position_in_token <
                           b->position_in_token;
                }
            );

            state.instance_value.name =
                LangAPI::Symbol{tail.prev ? tail.prev->token_name : tail.token_name};

            const std::size_t limit =
                std::min(
                    data_block.names.size(),
                    members_with_prefix.size()
                );

            for (
                long long i =
                    static_cast<long long>(limit) - 1;
                i >= 0;
                --i
            ) {
                const std::size_t u_idx =
                    static_cast<std::size_t>(i);

                const auto &key =
                    data_block.names[u_idx];

                LangAPI::Type type =
                    LLIR::BuilderBase::
                        deduceVarTypeByRuleMember(
                            members_with_prefix[u_idx]->member
                        );

                LangAPI::Statements insert_statements =
                    create_variable_for_access(
                        type,
                        key,
                        u_idx,
                        members_with_prefix[u_idx]->member
                    );

                state.statements.insert(
                    state.statements.end(),
                    insert_statements.begin(),
                    insert_statements.end()
                );

                state.instance_value.args.push_back(
                    LangAPI::Symbol::createExpression(
                        LangAPI::Symbol{key}
                    )
                );
            }

            std::reverse(
                state.instance_value.args.begin(),
                state.instance_value.args.end()
            );
        } else {
            std::cout << "Assigning at else{}" << std::endl;
            state.instance_value =
                LangAPI::Inheritance{
                .name = LangAPI::Symbol{tail.prev ? tail.prev->token_name : tail.token_name}
                };
        }

        // Preserve the existing semantic ABI.
        std::reverse(
            state.instance_value.args.begin(),
            state.instance_value.args.end()
        );
        std::cout << state.instance_value.name << ", prev.empty: " << (tail.prev == nullptr) << ", token: " << tail.token_name << " " << std::endl;
        state.nfa_index = state_id;
        state.next_state = DFATarget{
            .id = next,
            .debug = debug
        };

        // Any capture BEGIN/END markers the caller resolved for `tail`
        // (see the non-terminal branch in wireToken's `visit`) must fire
        // before the semantic reduction itself, in the order they were
        // computed, so nested captures close correctly before this token
        // is reduced.
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