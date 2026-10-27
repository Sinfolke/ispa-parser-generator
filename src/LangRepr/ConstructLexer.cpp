module LangRepr.ConstructLexer;

import cpuf.printf;
import cpuf.op;
import NFA.TNFA.API;
import NFA.IR.API;
import DFA.API;
import LangAPI;
import hash;
import logging;
import corelib;
import constants;
import Parser.Base;
import UAST.PyAST;
import std;

// -----------------------------------------------------------------------
// ARCHITECTURE NOTE
//
// Exactly two tables are emitted:
//
//   1. dfa_table       -- the actual (unified, classified) DFA. No FCDT,
//                          no per-token spans. State 0 is the single
//                          shared start state for every token.
//   2. char_class_table -- raw byte -> class id, shared globally across
//                          every state in dfa_table.
//
// No third table for acceptance. Instead, wherever a transition would
// have pointed at a materialized "empty"/accepting placeholder state, the
// target is encoded directly as:
//
//       state_count + 1 + <token enum value>
//
// i.e. any `next` value >= state_count is not a real state index at all;
// it's an accept marker, and the matched token id is `next - state_count
// - 1`. Genuinely dead transitions (no match, no accept) keep using
// DFA::NULL_STATE, which is far outside this range, so the three cases
// (real state / accept sentinel / dead) never collide:
//
//     next <  state_count           -> real continuing state
//     next == DFA::NULL_STATE       -> dead transition
//     otherwise (next > state_count)-> accept for token (next - state_count - 1)
//
// This removes the need to ever materialize placeholder "empty" states,
// and removes any separate accept_table/construct_table -- the token id
// is recoverable by arithmetic directly from the transition that was
// actually taken, with zero extra storage.
//
// RUNTIME TYPE ASSUMPTION: DFAAPI::Table<N> as given constrains state
// count == class count via a single template parameter. That only holds
// by coincidence; this file assumes the corrected two-parameter form:
//
//   template<std::size_t Classes> using State = std::array<std::size_t, Classes>;
//   template<std::size_t States, std::size_t Classes>
//   using Table = std::array<State<Classes>, States>;
//
// The runtime's DfaLookup lowering must decode accept sentinels using the
// SAME formula used here to encode them -- these two sides have to agree
// bit-for-bit.
// -----------------------------------------------------------------------
namespace {

    struct LexerFailureCall {
        const AST::FailBlock* block = nullptr;
        stdu::vector<std::string> arguments;
        bool operator==(const LexerFailureCall&) const = default;
    };
    struct LexerFailureSite {
        stdu::vector<std::string> token_name;
        stdu::vector<LexerFailureCall> calls;
        bool operator==(const LexerFailureSite&) const = default;
    };

    std::optional<LexerFailureSite> resolveLexerFailure(const AST::Tree& tree,
                                                        const stdu::vector<std::string>& token_name,
                                                        const AST::RuleMember* member = nullptr) {
        const auto rule = tree.getTreeMap().find(token_name);
        if (rule == tree.getTreeMap().end()) return std::nullopt;
        stdu::vector<std::pair<stdu::vector<std::string>, stdu::vector<std::string>>> handlers;
        bool marked = false;
        if (member) for (const auto& feature : member->features)
            if (const auto* marker = std::get_if<AST::FailMarker>(&feature)) {
                marked = true;
                if (!marker->name.empty()) handlers.emplace_back(marker->name, marker->parameters);
                break;
            }
        if (handlers.empty()) for (const auto& feature : rule->second.features)
            if (const auto* binding = std::get_if<AST::FailBinding>(&feature);
                binding && !binding->names.empty()) {
                for (const auto& name : binding->names) handlers.push_back({{name}, {}});
                break;
            }
        if (marked && handlers.empty())
            throw Error("Fail marker in {} has no bound fail block", token_name);
        if (handlers.empty()) return std::nullopt;
        LexerFailureSite site{token_name, {}};
        for (const auto& [handler, arguments] : handlers) {
            const AST::FailBlock* resolved = nullptr;
            for (std::size_t size = token_name.size();; --size) {
                stdu::vector<std::string> candidate(token_name.begin(), token_name.begin() + size);
                candidate.insert(candidate.end(), handler.begin(), handler.end());
                for (const auto& block : tree.getFailBlocks())
                    if (block.name == candidate) { resolved = &block; break; }
                if (resolved || size == 0) break;
            }
            if (!resolved && !arguments.empty())
                throw Error("Unknown parameterized lexer fail block {} in {}", handler, token_name);
            if (!resolved) { site.calls.push_back({nullptr, {}}); continue; }
            if (resolved->parameters.size() != arguments.size())
                throw Error("Lexer fail block {} expects {} arguments, got {}", handler,
                            resolved->parameters.size(), arguments.size());
            site.calls.push_back({resolved, arguments});
        }
        return site;
    }

    stdu::vector<LexerFailureSite> lexerFailureSites(const AST::Tree& tree) {
        stdu::vector<LexerFailureSite> selected;
        const auto add = [&](std::optional<LexerFailureSite> site) {
            if (site && std::ranges::find(selected, *site) == selected.end()) selected.push_back(*site);
        };
        const auto visit = [&](auto&& self, const AST::RuleMember& member,
                               const stdu::vector<std::string>& token_name) -> void {
            if (std::ranges::any_of(member.features, [](const auto& feature) {
                return std::holds_alternative<AST::FailMarker>(feature);
            })) add(resolveLexerFailure(tree, token_name, &member));
            if (member.isGroup()) for (const auto& child : member.getGroup().values) self(self, *child, token_name);
            if (member.isOp()) for (const auto& child : member.getOp().options) self(self, *child, token_name);
        };
        for (const auto& [name, rule] : tree.getTreeMap()) {
            if (name.empty() || !AST::RuleMemberName{.name = name}.isTerminal()) continue;
            add(resolveLexerFailure(tree, name));
            for (const auto& member : rule.rule_members) visit(visit, *member, name);
        }
        return selected;
    }

    std::size_t failureAt(const AST::Tree& tree,
                          const DFA::State<DFA::ClassTransitions>& state,
                          const stdu::vector<LexerFailureSite>& sites) {
        std::size_t chosen = NFA::NULL_STATE;
        for (const auto& origin : state.debug) {
            const auto site = resolveLexerFailure(tree, origin.token_name, &origin.member);
            if (!site) continue;
            const auto found = std::ranges::find(sites, *site);
            if (found == sites.end()) continue;
            const auto index = static_cast<std::size_t>(found - sites.begin());
            if (chosen != NFA::NULL_STATE && chosen != index) return sites.size();
            chosen = index;
        }
        if (chosen != NFA::NULL_STATE) return chosen;
        return sites.size() == 1 ? 0 : sites.size();
    }

    std::set<std::string> pythonClasses(const LangAPI::Declarations& declarations, const std::string& prefix = "") {
        std::set<std::string> names;
        for (const auto& declaration : declarations) {
            if (declaration.isNamespace()) {
                const auto& ns = declaration.getNamespace();
                const auto nested = pythonClasses(ns.declarations, prefix + ns.name + ".");
                names.insert(nested.begin(), nested.end());
            } else if (declaration.isClass()) {
                const auto& cls = declaration.getClass();
                std::string name = prefix;
                for (const auto& part : cls.name.path) if (const auto* value = std::get_if<std::string>(&part)) {
                    if (name.size() > prefix.size()) name += ".";
                    name += *value;
                }
                names.insert(name);
            }
        }
        return names;
    }

    // Cheap key for the part of DFADebug that comes from char_origin.
    struct OriginKey {
        bool has = false;
        std::size_t rule_run = 0, offset = 0, length = 0;
        char ch = '\0';
        bool operator==(const OriginKey &) const = default;
    };
    template<typename Target>
    OriginKey originKey(const Target &t) {
        if (!t.char_origin) return {};
        const auto &o = *t.char_origin;
        return { true, o.rule_run, o.offset, o.length, o.ch };
    }

    struct SlotEntry { OriginKey origin; std::size_t slot; };

    // Only called once per UNIQUE entry, so the token_name copy is paid once.
    template<typename Target>
    LangAPI::DFADebug makeDebug(const Target &target) {
        LangAPI::DFADebug debug {
            .member = target.debug.member,
            .token_name = target.debug.token_name,
            .position_in_token = target.debug.position_in_token,
            .call = target.debug.call,
            .group = target.debug.group,
        };
        if (target.char_origin) {
            const auto &o = *target.char_origin;
            debug.rule_run = o.rule_run;
            debug.offset = o.offset;
            debug.length = o.length;
            debug.ch = o.ch;
        }
        return debug;
    }

} // namespace
namespace LangRepr {

    auto ConstructLexer::makeCharClassTableDecl(const DFA::CharClassTable &table)
        -> std::pair<std::shared_ptr<LangAPI::Declaration>, LangAPI::Visibility> {

        LangAPI::Array arr;
        arr.values.reserve(256);
        for (std::size_t c = 0; c < 256; ++c) {
            arr.values.push_back(
                LangAPI::Int::createExpression(LangAPI::Int {
                    .value = static_cast<long long>(table.char_to_class.at(c))
                })
            );
        }

        LangAPI::IspaLibSymbol class_table_symbol {.exports = LangAPI::StdlibExports::DfaClassTable};

        LangAPI::Variable var {
            .name = "char_class_table",
            .type = LangAPI::Type {class_table_symbol},
            .value = LangAPI::Array::createExpression(arr),
            .is_static = true
        };

        return {
            std::make_shared<LangAPI::Declaration>(LangAPI::Variable::createDeclaration(var)),
            LangAPI::Visibility::Private
        };
    }

    auto ConstructLexer::makeDebugTable(
        const stdu::vector<DFA::State<DFA::ClassTransitions>> &states,
        std::size_t state_count,
        std::size_t class_count
    ) -> std::optional<std::pair<LangAPI::Declaration, LangAPI::Declaration>> {

        LangAPI::Array debug_array;   // unique DFADebug only, no free slots
        LangAPI::Map   debug_index;   // state_id -> (class_id -> slot in debug_array)

        // TokenID hashing is already used by the old code (`seen`), so no new hash
        // is needed. The bucket is tiny: it only distinguishes different origins
        // of the same TokenID.
        utype::unordered_map<NFA::IR::TokenID, stdu::vector<SlotEntry>> slots;

        constexpr std::size_t NONE = std::numeric_limits<std::size_t>::max();

        for (std::size_t s = 0; s < state_count && s < states.size(); ++s) {
            LangAPI::Map row;

            for (std::size_t c = 0; c < class_count && c < states[s].transitions.size(); ++c) {
                std::visit([&](const auto &target) {
                    using Target = std::decay_t<decltype(target)>;
                    if constexpr (std::is_same_v<Target, NFA::TNFA::DFATarget> ||
                                  std::is_same_v<Target, NFA::TNFA::ActionTarget> ||
                                  std::is_same_v<Target, NFA::TNFA::SemanticTarget>) {
                        if (target.id == NFA::NULL_STATE) return;
                        if (target.debug.member.empty() && !target.char_origin) return;

                        const OriginKey origin = originKey(target);
                        auto &bucket = slots[target.debug];

                        std::size_t slot = NONE;
                        for (const auto &e : bucket) {
                            if (e.origin == origin) { slot = e.slot; break; }
                        }

                        if (slot == NONE) {
                            slot = debug_array.values.size();          // dense, no gaps
                            bucket.push_back({ origin, slot });
                            debug_array.values.push_back(
                                LangAPI::DFADebug::createExpression(makeDebug(target)));
                        }

                        row.keys.push_back(LangAPI::Int { .value = static_cast<long long>(c) });
                        row.values.push_back(LangAPI::Int::createExpression(
                            LangAPI::Int { .value = static_cast<long long>(slot) }));
                    }
                }, states[s].transitions[c]);
            }

            if (!row.keys.empty()) {   // states without debug cells are not emitted at all
                debug_index.keys.push_back(LangAPI::Int { .value = static_cast<long long>(s) });
                debug_index.values.push_back(LangAPI::Map::createExpression(std::move(row)));
            }
        }

        if (debug_array.values.empty()) {
            // Action-only transitions have no provenance cells, but lookup
            // still needs a table argument. An unindexed placeholder keeps
            // the generated declaration valid.
            debug_array.values.push_back(LangAPI::DFADebug::createExpression(
                LangAPI::DFADebug{.position_in_token = 0}));
        }

        using LangAPI::Type;
        using LangAPI::ValueType;

        auto array_decl = LangAPI::Array::createDeclaration(LangAPI::Variable {
            .name = "debug_array",
            .type = Type {
                ValueType::FixedSizeArray,
                Type { LangAPI::IspaLibSymbol { .exports = LangAPI::StdlibExports::DFADebug } },
                LangAPI::RValue { LangAPI::Int {
                    .value = static_cast<long long>(debug_array.values.size()) } }
            },
            .value = LangAPI::Array::createExpression(std::move(debug_array)),
            .is_static = true
        });

        auto index_decl = LangAPI::Map::createDeclaration(LangAPI::Variable {
            .name = "debug_index",
            .type = Type {
                ValueType::Map,
                Type { ValueType::Int },
                Type { ValueType::Map, Type { ValueType::Int }, Type { ValueType::Int } }
            },
            .value = LangAPI::Map::createExpression(std::move(debug_index)),
            .is_static = true
        });

        return std::pair { std::move(array_decl), std::move(index_decl) };
    }
    // Emits the single unified transition table. Each entry is a plain
    // size_t, sentinel-encoded per the scheme above -- no wrapper type,
    // no per-transition metadata struct, since acceptance now lives
    // entirely in the numeric value rather than needing its own field.
    auto ConstructLexer::makeDfaTableDecl(
        const stdu::vector<DFA::State<DFA::ClassTransitions>> &states,
        std::size_t state_count,
        std::size_t class_count
    ) -> std::pair<std::shared_ptr<LangAPI::Declaration>, LangAPI::Visibility> {

        stdu::vector<LangAPI::Expression> rows;
        rows.reserve(state_count);
        const auto failures = lexerFailureSites(tree);
        for (const auto &state : states) {
            stdu::vector<LangAPI::Expression> row;
            row.reserve(class_count);
            const auto failure_index = failures.empty() ? NFA::NULL_STATE : failureAt(tree, state, failures);
            const auto dead_transition = [&]() -> LangAPI::RValue {
                if (failure_index == NFA::NULL_STATE)
                    return LangAPI::IspaLibSymbol{.exports = LangAPI::StdlibExports::DfaNullState};
                return LangAPI::Int{.value = static_cast<long long>(state_count +
                    lexer_builder.getActionTable().size() + lexer_builder.getSemanticTable().size() + failure_index)};
            };

            for (std::size_t cls = 0; cls < class_count; ++cls) {
                LangAPI::RValue encoded;

                if (cls < state.transitions.size()) {
                    const auto &t = state.transitions[cls];
                    if (std::holds_alternative<NFA::TNFA::DFATarget>(t)) {
                        if (std::get<NFA::TNFA::DFATarget>(t).id == NFA::NULL_STATE) {
                            encoded = dead_transition();
                        } else {
                            encoded = LangAPI::Int::createRValue(LangAPI::Int {.value = static_cast<long long>(std::get<NFA::TNFA::DFATarget>(t).id)});
                        }
                    } else if (std::holds_alternative<NFA::TNFA::ActionTarget>(t)) {
                        // t.next is final_st -- the DFA state to continue to
                        // AFTER this action/semantic chain resolves -- NOT the
                        // index of which lr_table entry to invoke. That real
                        // index lives in t.accept_index (populated from
                        // target.id in classify()). The runtime doesn't need
                        // final_st pre-baked into this sentinel at all: once
                        // it looks up lr_table[accept_index] and executes it,
                        // THAT entry's own next_state field (already correctly
                        // resolved/rebased by DFA.cpp's optimize/minimize
                        // passes) tells it what to do next -- continue to
                        // another action, a semantic reduction, or a DFA
                        // state -- chaining progressively at runtime rather
                        // than needing to be resolved in advance here.
                        // Embedding final_st here instead produced an
                        // arbitrary DFA-state-sized number in the slot the
                        // runtime treats as an action-table index, indexing
                        // clean off the end of the real (much smaller) table.
                        encoded = LangAPI::Int::createRValue(LangAPI::Int {.value = static_cast<long long>(std::get<NFA::TNFA::ActionTarget>(t).id + state_count)});
                    } else if (std::holds_alternative<NFA::TNFA::SemanticTarget>(t)) {
                        // Same fix, mirrored for semantic_table.
                        encoded = LangAPI::Int::createRValue(LangAPI::Int {.value = static_cast<long long>(std::get<NFA::TNFA::SemanticTarget>(t).id + state_count + lexer_builder.getActionTable().size())});
                    } else {
                        throw Error("Unknown table type {}", t.index());
                    }
                } else {
                    encoded = dead_transition();
                }
                row.push_back(LangAPI::Int::createExpression(encoded));
            }
            rows.push_back(LangAPI::Array::createExpression(LangAPI::Array {.values = row}));
        }

        LangAPI::IspaLibSymbol dfa_table_symbol {.exports = LangAPI::StdlibExports::DfaTable};
        dfa_table_symbol.template_parameters.push_back(
            std::make_shared<LangAPI::RValue>(LangAPI::Int {.value = static_cast<long long>(state_count)})
        );
        dfa_table_symbol.template_parameters.push_back(
            std::make_shared<LangAPI::RValue>(LangAPI::Int {.value = static_cast<long long>(class_count)})
        );
        LangAPI::Variable var {
            .name = "dfa_table",
            .type = LangAPI::Type {dfa_table_symbol},
            .value = LangAPI::FixedSizeArray::createExpression(LangAPI::FixedSizeArray {.values = rows}),
            .nested_array_type = true,
            .is_static = true,
        };
        return {
            std::make_shared<LangAPI::Declaration>(LangAPI::Variable::createDeclaration(var)),
            LangAPI::Visibility::Private
        };
    }

    auto ConstructLexer::makeActionTableDecl(
        const stdu::vector<NFA::TNFA::ActionState> &states,
        std::size_t state_count
    ) -> std::pair<std::shared_ptr<LangAPI::Declaration>, LangAPI::Visibility> {
        (void)state_count;

        constexpr std::size_t action_columns = 4;
        const auto action_state_count = states.size();
        stdu::vector<LangAPI::Expression> rows;
        rows.reserve(action_state_count);

        const auto push_operand = [](auto &row, std::size_t v) {
            if (v == NFA::NULL_STATE)   // API::null_state; a raw -1 would narrow inside the size_t array
                row.push_back(LangAPI::IspaLibSymbol::createExpression(
                    LangAPI::IspaLibSymbol {.exports = LangAPI::StdlibExports::DfaNullState}));
            else
                row.push_back(LangAPI::Int::createExpression(
                    LangAPI::Int {.value = static_cast<long long>(v)}));
        };

        for (const auto &state : states) {
            stdu::vector<LangAPI::Expression> row;
            row.reserve(action_columns);

            row.push_back(LangAPI::Int::createExpression(LangAPI::Int {
                .value = static_cast<long long>(state.action)
            }));
            push_operand(row, state.operand_a);
            push_operand(row, state.operand_b);
            std::visit([&](const auto &target) {
                using T = std::decay_t<decltype(target)>;
                if (std::is_same_v<T, NFA::DFATarget>) {
                    push_operand(row, target.id);
                } else if (std::is_same_v<T, NFA::ActionTarget>) {
                    row.push_back(LangAPI::Int::createExpression(LangAPI::Int {
                        .value = static_cast<long long>(target.id + state_count)
                    }));
                } else if (std::is_same_v<T, NFA::SemanticTarget>) {
                    row.push_back(LangAPI::Int::createExpression(LangAPI::Int {
                        .value = static_cast<long long>(target.id + state_count + lexer_builder.getActionTable().size())
                    }));
                } else {
                    throw Error("Unknown target type {}", (int) target.id);
                }

            }, state.next_state);


            rows.push_back(
                LangAPI::Array::createExpression(
                    LangAPI::Array {.values = row}
                )
            );
        }

        LangAPI::IspaLibSymbol action_table_symbol {
            .exports = LangAPI::StdlibExports::DfaTable
        };
        action_table_symbol.template_parameters.push_back(
            std::make_shared<LangAPI::RValue>(
                LangAPI::Int {.value = static_cast<long long>(action_state_count)}
            )
        );
        action_table_symbol.template_parameters.push_back(
            std::make_shared<LangAPI::RValue>(
                LangAPI::Int {.value = static_cast<long long>(action_columns)}
            )
        );

        LangAPI::Variable var {
            .name = "action_table",
            .type = LangAPI::Type {action_table_symbol},
            .value = LangAPI::FixedSizeArray::createExpression(
                LangAPI::FixedSizeArray {.values = rows}
            ),
            .nested_array_type = true,
            .is_static = true,
        };

        return {
            std::make_shared<LangAPI::Declaration>(
                LangAPI::Variable::createDeclaration(var)
            ),
            LangAPI::Visibility::Private
        };
    }
    auto ConstructLexer::makeSemanticSwitchFunction(const stdu::vector<NFA::TNFA::SemanticState> semantic_table) -> LangAPI::Function {
        stdu::vector<LangAPI::Statements> semantic_table_statements;
        // change semantic table to raw Statements
        for (const auto &semantic_state : semantic_table) {
            LangAPI::Statements statements = semantic_state.statements;
            if (semantic_state.dynamic_source) {
                const auto &source = *semantic_state.dynamic_source;
                const auto symbol = [](const std::string &name) {
                    return LangAPI::Symbol::createExpression(LangAPI::Symbol{name});
                };
                const auto integer = [](std::size_t value) {
                    return LangAPI::Int::createExpression(LangAPI::Int{.value = static_cast<long long>(value)});
                };
                const auto escaped = [](const std::string& value) {
                    std::string result;
                    for (char c : value) result += corelib::text::getEscapedAsStr(c, true);
                    return result;
                };
                std::set<std::string> declared;
                for (const auto &[name, id] : semantic_state.dynamic_captures) {
                    if (!declared.insert(name).second) continue;
                    LangAPI::StorageSymbol capture;
                    capture.what = symbol("captures");
                    capture.path.emplace_back(LangAPI::FunctionCall{
                        .name = std::make_shared<LangAPI::Symbol>(LangAPI::Symbol{"text"}),
                        .args = {integer(id)}});
                    statements.push_back(LangAPI::Variable::createStatement(LangAPI::Variable{
                        .name = name, .type = LangAPI::ValueType::String,
                        .value = LangAPI::StorageSymbol::createExpression(capture)}));
                }
                if (auto bindings = parameter_bindings.find(source.token_name); bindings != parameter_bindings.end())
                    for (const auto &binding : bindings->second) {
                        if (!declared.insert(binding.name).second) continue;
                        statements.push_back(LangAPI::Variable::createStatement(LangAPI::Variable{
                            .name = binding.name, .type = binding.field.type,
                            .value = symbol(binding.field.name)}));
                    }

                LangAPI::Array choices;
                std::set<stdu::vector<std::string>> visiting;
                const auto collect = [&](const auto &self, const AST::RuleMember &member) -> void {
                    for (const auto &feature : member.features)
                        if (const auto *dynamic = std::get_if<AST::DynamicPattern>(&feature)) {
                            if (!dynamic->name.empty()) choices.values.push_back(symbol(dynamic->name));
                            else if (const auto *expression = std::get_if<UAST::py::Expr>(&dynamic->expression.code))
                                choices.values.push_back(Parser::ParserBase::convertPython(*expression));
                            else throw Error("Dynamic lexer pattern has no expression");
                            return;
                        }
                    if (member.isString()) {
                        choices.values.push_back(LangAPI::String::createExpression(
                            LangAPI::String{.value = escaped(member.getString().value)}));
                    } else if (member.isCsequence()) {
                        const auto &set = member.getCsequence();
                        std::set<unsigned char> chars;
                        for (unsigned char c : set.characters) chars.insert(c);
                        for (char c : set.escaped)
                            chars.insert(static_cast<unsigned char>(corelib::text::getEscapedFromChar(c)));
                        for (const auto &[first, last] : set.diapasons)
                            for (int c = static_cast<unsigned char>(first); c <= static_cast<unsigned char>(last); ++c)
                                chars.insert(static_cast<unsigned char>(c));
                        for (int c = 1; c < 128; ++c)
                            if (set.negative ? !chars.contains(static_cast<unsigned char>(c)) :
                                chars.contains(static_cast<unsigned char>(c)))
                                choices.values.push_back(LangAPI::String::createExpression(
                                    LangAPI::String{.value = corelib::text::getEscapedAsStr(
                                        static_cast<char>(c), true)}));
                    } else if (member.isGroup()) {
                        if (member.getGroup().values.size() != 1)
                            throw Error("negated dynamic lexer group must match one character");
                        self(self, *member.getGroup().values.front());
                    } else if (member.isOp()) {
                        for (const auto &option : member.getOp().options) self(self, *option);
                    } else if (member.isName()) {
                        const auto &name = member.getName().name;
                        if (!visiting.insert(name).second || !tree.getTreeMap().contains(name))
                            throw Error("unresolved or recursive dynamic lexer exclusion");
                        const auto &rule = tree.getTreeMap().at(name);
                        if (rule.rule_members.size() != 1)
                            throw Error("negated dynamic lexer rule must match one character");
                        self(self, *rule.rule_members.front());
                        visiting.erase(name);
                    } else throw Error("unsupported dynamic lexer pattern");
                };
                collect(collect, source.member);
                LangAPI::Type strings{LangAPI::ValueType::Array};
                strings.template_parameters.push_back(LangAPI::ValueType::String);
                statements.push_back(LangAPI::Variable::createStatement(LangAPI::Variable{
                    .name = "ispa_dynamic_choices", .type = strings,
                    .value = LangAPI::Array::createExpression(std::move(choices))}));
                const auto result = [&](std::size_t target) -> LangAPI::Statements {
                    LangAPI::Expression next = target == NFA::NULL_STATE
                        ? LangAPI::IspaLibSymbol::createExpression(LangAPI::IspaLibSymbol{
                            .exports = LangAPI::StdlibExports::DfaNullState})
                        : integer(target);
                    return LangAPI::Return::createStatements(LangAPI::Return{
                        .value = LangAPI::MakeTuple::createExpression(LangAPI::MakeTuple{
                            .args = {next, LangAPI::Inheritance::createExpression(
                                LangAPI::Inheritance{.name = LangAPI::Symbol{"Token"}})}})});
                };
                const auto dynamic_fail = [&]() -> LangAPI::Statements {
                    return LangAPI::Return::createStatements(LangAPI::Return{
                        .value = LangAPI::MakeTuple::createExpression(LangAPI::MakeTuple{
                            .args = {
                                LangAPI::IspaLibSymbol::createExpression(LangAPI::IspaLibSymbol{
                                    .exports = LangAPI::StdlibExports::DfaDynamicFailState}),
                                LangAPI::Inheritance::createExpression(
                                    LangAPI::Inheritance{.name = LangAPI::Symbol{"Token"}}),
                            }})});
                };
                std::size_t next = NFA::NULL_STATE;
                std::visit([&](const auto &target) {
                    using T = std::decay_t<decltype(target)>;
                    if constexpr (std::is_same_v<T, NFA::DFATarget>) next = target.id;
                    else if constexpr (std::is_same_v<T, NFA::ActionTarget>)
                        next = target.id + lexer_builder.getDFA().states.size();
                    else next = target.id + lexer_builder.getDFA().states.size() + lexer_builder.getActionTable().size();
                }, semantic_state.next_state);
                auto match = LangAPI::IspaLibFunctionCall::createExpression(LangAPI::IspaLibFunctionCall{
                    .symbol = LangAPI::IspaLibSymbol{.exports = LangAPI::StdlibExports::DfaDynamicMatch},
                    .args = {symbol("cursor"), symbol("ispa_dynamic_choices"),
                        LangAPI::Bool::createExpression(LangAPI::Bool{.value = source.member.negative_match}),
                        LangAPI::String::createExpression(LangAPI::String{
                            .value = source.member.quantifier == '*' ? "*" : ""})}});
                statements.push_back(LangAPI::If::createStatement(LangAPI::If{
                    std::move(match), result(next), dynamic_fail()}));
                semantic_table_statements.push_back(std::move(statements));
                continue;
            }
            const auto &instance_symbol = std::get<LangAPI::Symbol>(semantic_state.instance_value.name);
            stdu::vector<std::string> token_name;
            for (const auto& part : instance_symbol.path) {
                if (!std::holds_alternative<std::string>(part)) { token_name.clear(); break; }
                token_name.push_back(std::get<std::string>(part));
            }
            bool has_token_actions = false;
            if (auto found = tree.getTreeMap().find(token_name); found != tree.getTreeMap().end()) {
                std::set<std::string> locals;
                std::set<std::string> objects;
                const auto collect_variable_captures = [&](auto&& self, const AST::RuleMember& member) -> void {
                    for (const auto& feature : member.features)
                        if (const auto* capture = std::get_if<AST::VariableCapture>(&feature))
                            locals.insert(capture->name);
                    if (member.isGroup())
                        for (const auto& child : member.getGroup().values) self(self, *child);
                    if (member.isOp())
                        for (const auto& child : member.getOp().options) self(self, *child);
                };
                for (const auto& member : found->second.rule_members)
                    collect_variable_captures(collect_variable_captures, *member);
                if (auto bindings = parameter_bindings.find(token_name); bindings != parameter_bindings.end())
                    for (const auto& binding : bindings->second) {
                        locals.insert(binding.name);
                        statements.push_back(LangAPI::Variable{.name = "ispa_action_" + binding.name,
                            .type = binding.field.type,
                            .value = LangAPI::Symbol::createExpression(LangAPI::Symbol{binding.field.name})});
                    }
                const auto classes = pythonClasses(tree.getPythonDeclarations());
                for (const auto& feature : found->second.features) if (auto action = std::get_if<AST::PythonAction>(&feature)) {
                    if (!has_token_actions) {
                        LangAPI::Expression end = LangAPI::Symbol::createExpression(LangAPI::Symbol{"start"});
                        end.emplace_back(LangAPI::ExpressionElement::Add);
                        auto length = LangAPI::Symbol::createExpression(LangAPI::Symbol{"length"});
                        end.insert(end.end(), length.begin(), length.end());
                        statements.push_back(LangAPI::Variable::createStatement(LangAPI::Variable{
                            .name = "ispa_token_text", .type = LangAPI::ValueType::String,
                            .value = LangAPI::Inheritance::createExpression(LangAPI::Inheritance{
                                .name = LangAPI::Symbol{"std", "string"},
                                .args = {LangAPI::Symbol::createExpression(LangAPI::Symbol{"start"}),
                                    end}})}));
                        statements.push_back(LangAPI::Variable::createStatement(LangAPI::Variable{
                            .name = "ispa_token_result",
                            .type = LangAPI::Type{instance_symbol},
                            .value = LangAPI::Inheritance::createExpression(semantic_state.instance_value)}));
                        has_token_actions = true;
                    }
                    auto code = Parser::ParserBase::convertPythonAction(*action, locals, false, classes, &objects,
                        corelib::text::join(token_name, "."));
                    statements.insert(statements.end(), code.begin(), code.end());
                }
            }
            LangAPI::RValue next_state;
            std::visit([&](const auto &target) {
                using T = std::decay_t<decltype(target)>;
                if (std::is_same_v<T, NFA::DFATarget>) {
                    if (target.id == NFA::NULL_STATE) {
                        next_state = LangAPI::RValue {LangAPI::IspaLibSymbol {.exports = LangAPI::StdlibExports::DfaNullState}};
                    } else {
                        next_state = LangAPI::Int::createRValue(LangAPI::Int {.value = static_cast<long long>(target.id)});
                    }
                } else if (std::is_same_v<T, NFA::ActionTarget>) {
                    next_state = LangAPI::Int::createRValue(LangAPI::Int {.value = static_cast<long long>(target.id + lexer_builder.getDFA().states.size())});
                } else if (std::is_same_v<T, NFA::SemanticTarget>) {
                    next_state = LangAPI::Int::createRValue(LangAPI::Int {.value = static_cast<long long>(target.id + lexer_builder.getDFA().states.size() + lexer_builder.getActionTable().size())});
                } else {
                    throw Error("Unknown target type {}", (int) target.id);
                }

            }, semantic_state.next_state);
            LangAPI::Symbol tokens_enum_value = {instance_symbol.path};
            tokens_enum_value.path.insert(tokens_enum_value.path.begin(), "Tokens");
            tokens_enum_value.path.insert(tokens_enum_value.path.end(), "id");
            statements.push_back(
                LangAPI::Return::createStatement(LangAPI::Return {.value = LangAPI::MakeTuple::createExpression(LangAPI::MakeTuple {.args = {
                    LangAPI::Int::createExpression(next_state),
                    LangAPI::IspaLibFunctionCall::createExpression(LangAPI::IspaLibFunctionCall {
                        .symbol = LangAPI::IspaLibSymbol {.exports = LangAPI::StdlibExports::RawTokenNodeConstruct, .template_parameters = {std::make_shared<LangAPI::Type>(std::get<LangAPI::Symbol>(semantic_state.instance_value.name))}},
                        .args = {
                            LangAPI::Symbol::createExpression(LangAPI::Symbol {"start_pos"}),
                            LangAPI::Symbol::createExpression(LangAPI::Symbol {"start"}),
                            LangAPI::Symbol::createExpression(LangAPI::Symbol {"length"}),
                            LangAPI::Symbol::createExpression(LangAPI::Symbol {"line"}),
                            LangAPI::Symbol::createExpression(tokens_enum_value),
                            has_token_actions ? LangAPI::Symbol::createExpression(LangAPI::Symbol{"ispa_token_result"}) :
                                LangAPI::Inheritance::createExpression(semantic_state.instance_value)
                        }
                    })
                }}
            )}));
            semantic_table_statements.push_back(std::move(statements));
        }
        const auto lexer_failures = lexerFailureSites(tree);
        for (const auto& site : lexer_failures) {
            LangAPI::Statements failure;
            auto current = LangAPI::Symbol::createExpression(LangAPI::Symbol{"start"});
            current.emplace_back(LangAPI::ExpressionElement::Add);
            const auto offset = LangAPI::Symbol::createExpression(LangAPI::Symbol{"length"});
            current.insert(current.end(), offset.begin(), offset.end());
            failure.push_back(LangAPI::Variable::createStatement(LangAPI::Variable{
                .name = "ctx", .type = LangAPI::ValueType::Undef,
                .value = LangAPI::FunctionCall::createExpression(LangAPI::FunctionCall{
                    .name = std::make_shared<LangAPI::Symbol>("lexer_failure_context"),
                    .args = {current,
                        LangAPI::String::createExpression(LangAPI::String{.value = corelib::text::join(site.token_name, ".")}),
                        LangAPI::String::createExpression(LangAPI::String{.value = corelib::text::join(site.token_name, ".")})}})}));
            const auto classes = pythonClasses(tree.getPythonDeclarations());
            for (const auto& call : site.calls) {
                if (!call.block) continue;
                std::set<std::string> locals, objects;
                for (std::size_t i = 0; i < call.arguments.size(); ++i) {
                    const auto bindings = parameter_bindings.find(site.token_name);
                    if (bindings == parameter_bindings.end())
                        throw Error("Unknown lexer fail argument {} in {}", call.arguments[i], site.token_name);
                    const auto binding = std::ranges::find_if(bindings->second, [&](const auto& value) {
                        return value.name == call.arguments[i];
                    });
                    if (binding == bindings->second.end())
                        throw Error("Unknown lexer fail argument {} in {}", call.arguments[i], site.token_name);
                    failure.push_back(LangAPI::Variable::createStatement(LangAPI::Variable{
                        .name = "ispa_action_" + call.block->parameters[i], .type = binding->field.type,
                        .value = LangAPI::Symbol::createExpression(LangAPI::Symbol{binding->field.name})}));
                    locals.insert(call.block->parameters[i]);
                }
                auto code = Parser::ParserBase::convertPythonAction(call.block->action, locals, true, classes, &objects,
                    corelib::text::join(site.token_name, "."));
                failure.insert(failure.end(), code.begin(), code.end());
            }
            LangAPI::Symbol node_type{site.token_name};
            node_type.path.insert(node_type.path.begin(), "Types");
            LangAPI::Symbol token_id{site.token_name};
            token_id.path.insert(token_id.path.begin(), "Tokens");
            token_id.path.emplace_back("id");
            auto inserted = LangAPI::IspaLibFunctionCall::createExpression(LangAPI::IspaLibFunctionCall{
                .symbol = LangAPI::IspaLibSymbol{
                    .exports = LangAPI::StdlibExports::RawTokenNodeConstruct,
                    .template_parameters = {std::make_shared<LangAPI::Type>(node_type)}},
                .args = {LangAPI::Symbol::createExpression(LangAPI::Symbol{"start_pos"}),
                    LangAPI::Symbol::createExpression(LangAPI::Symbol{"start"}),
                    LangAPI::Symbol::createExpression(LangAPI::Symbol{"length"}),
                    LangAPI::Symbol::createExpression(LangAPI::Symbol{"line"}),
                    LangAPI::Symbol::createExpression(token_id),
                    LangAPI::Inheritance::createExpression(LangAPI::Inheritance{.name = node_type})}});
            failure.push_back(LangAPI::Return::createStatement(LangAPI::Return{
                .value = LangAPI::FunctionCall::createExpression(LangAPI::FunctionCall{
                    .name = std::make_shared<LangAPI::Symbol>("lexer_failure_result"),
                    .args = {LangAPI::Symbol::createExpression(LangAPI::Symbol{"ctx"}), inserted}})}));
            semantic_table_statements.push_back(std::move(failure));
        }
        if (!lexer_failures.empty()) {
            LangAPI::Statements failure;
            failure.push_back(LangAPI::Expression::createStatement(
                LangAPI::FunctionCall::createExpression(LangAPI::FunctionCall{
                    .name = std::make_shared<LangAPI::Symbol>("ispa_token_error"),
                    .args = {LangAPI::String::createExpression(LangAPI::String{.value = "unmatched lexer input"})}})));
            semantic_table_statements.push_back(std::move(failure));
        }
        LangAPI::Function fun {
            .type = LangAPI::Type {LangAPI::ValueType::Tuple, LangAPI::Type {LangAPI::ValueType::Int}, LangAPI::Type {LangAPI::Symbol {"Token"}}},
            .name = "semantic_action_exec",
            .parameters = {
                std::make_pair(LangAPI::Type {LangAPI::ValueType::Int}, "state"),
                std::make_pair(LangAPI::Type {LangAPI::IspaLibSymbol {.exports = LangAPI::StdlibExports::DfaCaptures, .Const = true, .Reference = true}}, "captures"),
                std::make_pair(LangAPI::Type {LangAPI::ValueType::Int}, "start_pos"),
                std::make_pair(LangAPI::Type {LangAPI::ValueType::NonOwnedString}, "start"),
                std::make_pair(LangAPI::Type {LangAPI::ValueType::Int}, "length"),
                std::make_pair(LangAPI::Type {LangAPI::ValueType::Int}, "line"),
                std::make_pair(LangAPI::Type {LangAPI::ValueType::BorrowSemantic,
                    LangAPI::Type{LangAPI::ValueType::NonOwnedString}}, "cursor"),
            },
            .is_static = false
        };
        LangAPI::Switch switch_;
        switch_.expression = LangAPI::Symbol::createExpression(LangAPI::Symbol {"state"});
        long long state = 0;
        for (const auto &statements : semantic_table_statements) {
            switch_.cases.emplace_back(LangAPI::Int::createRValue(LangAPI::Int {.value = state++}), ensureTypesNs(statements));
        }
        fun.statements = LangAPI::Switch::createStatements(switch_);
        fun.statements.push_back(LangAPI::Throw::createStatement(LangAPI::Throw {.throw_value = LangAPI::IspaLibFunctionCall::createExpression(LangAPI::IspaLibFunctionCall {.symbol = LangAPI::IspaLibSymbol {.exports = LangAPI::StdlibExports::Error}, .args = {LangAPI::String::createExpression(LangAPI::String {.value = "Out of bound semantic action"})}})}));
        return fun;
    }
    auto ConstructLexer::constructLexer() -> void {
        holder.push(LangAPI::TypeAlias::createDeclaration(createTypeToken()));
        Tlog::Branch b(logger, "LangRepr/ConstructLexer.log");
        auto lexer = createLexerClass();
        parameter_bindings.clear();
        stdu::vector<LangAPI::Variable> parameter_fields;
        for (const auto& [name, rule] : tree.getTreeMap()) {
            if (!AST::RuleMemberName{.name = name}.isTerminal()) continue;
            for (const auto& feature : rule.features) if (const auto* syntax = std::get_if<AST::DeclarationSyntax>(&feature)) {
                std::set<std::size_t> used;
                LangAPI::Function setter;
                setter.name = "set" + Parser::ParserBase::fun_name(name) + "Parameters";
                setter.type = LangAPI::ValueType::Void;
                for (std::size_t i = 0; i < syntax->parameters.size(); ++i) {
                    const auto* annotation = i < syntax->parameterAnnotations.size()
                        ? std::get_if<UAST::py::Expr>(&syntax->parameterAnnotations[i].code) : nullptr;
                    if (!annotation) throw std::invalid_argument("lexer parameter needs a type annotation: " + syntax->parameters[i]);
                    auto type = Parser::ParserBase::convertPythonType(std::make_shared<UAST::py::Expr>(*annotation));
                    if (type == LangAPI::ValueType::Undef || type == LangAPI::ValueType::Void)
                        throw std::invalid_argument("lexer parameter needs a concrete type: " + syntax->parameters[i]);
                    std::size_t slot = 0;
                    while (slot < parameter_fields.size() && (used.contains(slot) || parameter_fields[slot].type != type)) ++slot;
                    if (slot == parameter_fields.size())
                        parameter_fields.push_back(LangAPI::Variable{.name = "ispa_parameter_" + std::to_string(slot),
                            .type = type, .value = LangAPI::EmptyInitializer::createExpression(LangAPI::EmptyInitializer{})});
                    used.insert(slot);
                    parameter_bindings[name].push_back(ParameterBinding{syntax->parameters[i], parameter_fields[slot]});
                    const auto argument = "argument_" + std::to_string(i);
                    setter.parameters.emplace_back(type, argument);
                    setter.statements.push_back(LangAPI::VariableAssignment::createStatement(LangAPI::VariableAssignment{
                        .name = LangAPI::Symbol{parameter_fields[slot].name},
                        .value = LangAPI::Symbol::createExpression(LangAPI::Symbol{argument})}));
                }
                if (!setter.parameters.empty())
                    lexer.data.emplace_back(std::make_shared<LangAPI::Declaration>(LangAPI::Function::createDeclaration(std::move(setter))),
                        LangAPI::Visibility::Public);
            }
        }
        for (auto& field : parameter_fields)
            lexer.data.emplace_back(std::make_shared<LangAPI::Declaration>(LangAPI::Variable::createDeclaration(std::move(field))),
                LangAPI::Visibility::Private);
        const auto memberCall = [](const std::string& object, const std::string& method,
                                   stdu::vector<LangAPI::Expression> args = {}) -> LangAPI::Expression {
            LangAPI::StorageSymbol storage;
            storage.what = LangAPI::Symbol::createExpression(LangAPI::Symbol{object});
            storage.path.emplace_back(LangAPI::FunctionCall{
                .name = std::make_shared<LangAPI::Symbol>(LangAPI::Symbol{method}), .args = std::move(args)});
            return LangAPI::StorageSymbol::createExpression(storage);
        };
        const auto callStatement = [](const LangAPI::Expression& expression) {
            return LangAPI::Expression::createStatement(expression);
        };
        LangAPI::Type pending_type{LangAPI::Symbol{"std", "vector"}};
        pending_type.template_parameters.push_back(LangAPI::Type{LangAPI::Symbol{"Token"}});
        lexer.data.emplace_back(std::make_shared<LangAPI::Declaration>(LangAPI::Variable::createDeclaration(
            LangAPI::Variable{.name = "ispa_pending_tokens", .type = pending_type})), LangAPI::Visibility::Private);
        namespace py = UAST::py;
        const auto sharedName = [](const py::ExprPtr& expression) -> std::string {
            auto field = expression ? expression->get<py::Attribute>() : nullptr;
            auto shared = field && field->value ? field->value->get<py::Attribute>() : nullptr;
            auto ctx = shared && shared->value ? shared->value->get<py::Name>() : nullptr;
            return ctx && ctx->id == "ctx" && shared->attr == "shared" ? field->attr : std::string{};
        };
        const auto literalType = [&](auto&& self, const py::ExprPtr& expression) -> LangAPI::Type {
            if (!expression) return LangAPI::ValueType::Undef;
            if (auto constant = expression->get<py::Constant>()) {
                switch (constant->kind) {
                    case py::ConstantKind::Bool: return LangAPI::ValueType::Bool;
                    case py::ConstantKind::Int: return LangAPI::ValueType::Int;
                    case py::ConstantKind::Float: return LangAPI::ValueType::Float;
                    case py::ConstantKind::Str: return LangAPI::ValueType::String;
                    default: return LangAPI::ValueType::Undef;
                }
            }
            if (auto list = expression->get<py::List>(); list && !list->elts.empty()) {
                auto element = self(self, list->elts.front());
                if (element == LangAPI::ValueType::Undef ||
                    !std::ranges::all_of(list->elts, [&](const auto& value) { return self(self, value) == element; }))
                    return LangAPI::ValueType::Undef;
                LangAPI::Type result{LangAPI::ValueType::Array};
                result.template_parameters.push_back(element);
                return result;
            }
            if (auto dict = expression->get<py::Dict>(); dict && !dict->keys.empty() && !dict->values.empty()) {
                auto key = self(self, dict->keys.front()), value = self(self, dict->values.front());
                if (key == LangAPI::ValueType::Undef || value == LangAPI::ValueType::Undef ||
                    !std::ranges::all_of(dict->keys, [&](const auto& item) { return self(self, item) == key; }) ||
                    !std::ranges::all_of(dict->values, [&](const auto& item) { return self(self, item) == value; }))
                    return LangAPI::ValueType::Undef;
                LangAPI::Type result{LangAPI::ValueType::Map};
                result.template_parameters.push_back(key);
                result.template_parameters.push_back(value);
                return result;
            }
            return LangAPI::ValueType::Undef;
        };
        std::set<std::string> shared_names;
        std::map<std::string, LangAPI::Type> declared_types;
        std::map<std::string, std::vector<LangAPI::Type>> inferred_types;
        const auto recordDeclaredType = [&](const std::string& name, const LangAPI::Type& type) {
            shared_names.insert(name);
            if (auto found = declared_types.find(name); found != declared_types.end() && found->second != type)
                throw std::invalid_argument("conflicting types for ctx.shared." + name);
            declared_types.insert_or_assign(name, type);
        };
        const auto recordInferredType = [&](const std::string& name, const LangAPI::Type& type) {
            shared_names.insert(name);
            if (type != LangAPI::ValueType::Undef) inferred_types[name].push_back(type);
        };
        const auto scanStatements = [&](auto&& self, const py::StmtList& statements) -> void {
            for (const auto& statement : statements) if (statement) std::visit([&](const auto& item) {
                using T = std::decay_t<decltype(item)>;
                if constexpr (std::is_same_v<T, py::Assign>) {
                    for (const auto& target : item.targets) if (auto name = sharedName(target); !name.empty()) {
                        recordInferredType(name, literalType(literalType, item.value));
                    }
                } else if constexpr (std::is_same_v<T, py::AnnAssign>) {
                    if (auto name = sharedName(item.target); !name.empty()) {
                        const auto declared = Parser::ParserBase::convertPythonType(item.annotation);
                        if (declared == LangAPI::ValueType::Undef || declared == LangAPI::ValueType::Void)
                            throw std::invalid_argument("ctx.shared." + name + " needs a concrete type");
                        recordDeclaredType(name, declared);
                        recordInferredType(name, literalType(literalType, item.value));
                    }
                } else if constexpr (std::is_same_v<T, py::If> || std::is_same_v<T, py::While>) {
                    self(self, item.body); self(self, item.orelse);
                }
            }, statement->kind);
        };
        for (const auto& [name, rule] : tree.getTreeMap())
            if (AST::RuleMemberName{.name = name}.isTerminal())
                for (const auto& feature : rule.features)
                    if (auto action = std::get_if<AST::PythonAction>(&feature))
                        if (auto module = std::get_if<py::Module>(&action->code))
                            scanStatements(scanStatements, module->body);
        for (const auto& name : shared_names) {
            auto declared = declared_types.find(name);
            auto inferred = inferred_types.find(name);
            if (declared == declared_types.end() && inferred == inferred_types.end())
                throw std::invalid_argument("ctx.shared." + name + " requires a type annotation or literal assignment");
            if (declared == declared_types.end() && !std::ranges::all_of(inferred->second,
                [&](const auto& type) { return type == inferred->second.front(); }))
                throw std::invalid_argument("conflicting inferred types for ctx.shared." + name);
            const auto& type = declared != declared_types.end() ? declared->second : inferred->second.front();
            LangAPI::Expression initial;
            if (type == LangAPI::ValueType::Int) initial = LangAPI::Int::createExpression(LangAPI::Int{.value = 0});
            else if (type == LangAPI::ValueType::Float) initial = LangAPI::Float::createExpression(LangAPI::Float{.value = 0});
            else if (type == LangAPI::ValueType::Bool) initial = LangAPI::Bool::createExpression(LangAPI::Bool{.value = false});
            else if (type == LangAPI::ValueType::String) initial = LangAPI::String::createExpression(LangAPI::String{.value = ""});
            lexer.data.emplace_back(std::make_shared<LangAPI::Declaration>(LangAPI::Variable::createDeclaration(
                LangAPI::Variable{.name = "ispa_shared_" + name, .type = type, .value = initial})),
                LangAPI::Visibility::Private);
        }
        lexer.data.emplace_back(std::make_shared<LangAPI::Declaration>(LangAPI::Variable::createDeclaration(
            LangAPI::Variable{.name = "ispa_skip_token", .type = LangAPI::ValueType::Bool,
                .value = LangAPI::Bool::createExpression(LangAPI::Bool{.value = false})})), LangAPI::Visibility::Private);
        LangAPI::Function skip_token;
        skip_token.name = "ispa_skip_current_token";
        skip_token.type = LangAPI::ValueType::Void;
        skip_token.statements = LangAPI::VariableAssignment::createStatements(LangAPI::VariableAssignment{
            .name = LangAPI::Symbol{"ispa_skip_token"},
            .value = LangAPI::Bool::createExpression(LangAPI::Bool{.value = true})});
        lexer.data.emplace_back(std::make_shared<LangAPI::Declaration>(LangAPI::Function::createDeclaration(skip_token)),
            LangAPI::Visibility::Private);
        LangAPI::Function token_error;
        token_error.name = "ispa_token_error";
        token_error.type = LangAPI::ValueType::Void;
        token_error.parameters = {{LangAPI::Type{LangAPI::ValueType::String}, "message"}};
        token_error.statements = LangAPI::Throw::createStatements(LangAPI::Throw{
            .throw_value = LangAPI::IspaLibFunctionCall::createExpression(LangAPI::IspaLibFunctionCall{
                .symbol = LangAPI::IspaLibSymbol{.exports = LangAPI::StdlibExports::Error},
                .args = {LangAPI::Symbol::createExpression(LangAPI::Symbol{"message"})}})});
        lexer.data.emplace_back(std::make_shared<LangAPI::Declaration>(LangAPI::Function::createDeclaration(token_error)),
            LangAPI::Visibility::Private);
        LangAPI::Statements flush;
        flush.push_back(callStatement(memberCall("tokens", "insert", {
            memberCall("tokens", "end"), memberCall("ispa_pending_tokens", "begin"),
            memberCall("ispa_pending_tokens", "end")})));
        flush.push_back(callStatement(memberCall("ispa_pending_tokens", "clear")));
        LangAPI::Function push_token;
        push_token.name = "on_token";
        push_token.type = LangAPI::ValueType::Void;
        push_token.parameters = {{LangAPI::Type{LangAPI::ValueType::MoveSemantic, LangAPI::Type{LangAPI::Symbol{"Token"}}}, "token"}};
        push_token.override = true;
        LangAPI::Expression skip_condition;
        skip_condition.push_back(LangAPI::Symbol::createExpression(LangAPI::Symbol{"ispa_skip_token"}));
        for (const auto& [name, rule] : tree.getTreeMap()) {
            if (name.empty() || !AST::RuleMemberName{.name = name}.isTerminal()) continue;
            const bool skip = std::ranges::any_of(rule.features, [](const auto& feature) {
                const auto* syntax = std::get_if<AST::DeclarationSyntax>(&feature);
                return syntax && std::ranges::find(syntax->modifiers, "skip") != syntax->modifiers.end();
            });
            if (!skip) continue;
            skip_condition.push_back(LangAPI::ExpressionElement::Or);
            auto kind = LangAPI::FunctionCall::createExpression(LangAPI::FunctionCall{
                .name = std::make_shared<LangAPI::Symbol>(LangAPI::Symbol{"token", "kind"})});
            stdu::vector<std::string> token_id{"Parser", "Tokens"};
            token_id.insert(token_id.end(), name.begin(), name.end());
            token_id.push_back("id");
            skip_condition.push_back(std::move(kind));
            skip_condition.push_back(LangAPI::ExpressionElement::Equal);
            skip_condition.push_back(LangAPI::Symbol::createExpression(LangAPI::Symbol{token_id}));
        }
        push_token.statements.push_back(LangAPI::If::createStatement(LangAPI::If{
            std::move(skip_condition), {},
            {callStatement(LangAPI::FunctionCall::createExpression(LangAPI::FunctionCall{
                .name = std::make_shared<LangAPI::Symbol>(LangAPI::Symbol{"append_token"}),
                .args = {LangAPI::Symbol::createExpression(LangAPI::Symbol{"token"})}}))}}));
        push_token.statements.push_back(LangAPI::VariableAssignment::createStatement(LangAPI::VariableAssignment{
            .name = LangAPI::Symbol{"ispa_skip_token"},
            .value = LangAPI::Bool::createExpression(LangAPI::Bool{.value = false})}));
        push_token.statements.insert(push_token.statements.end(), flush.begin(), flush.end());
        lexer.data.emplace_back(std::make_shared<LangAPI::Declaration>(LangAPI::Function::createDeclaration(push_token)),
            LangAPI::Visibility::Public);
        LangAPI::Function push_eof;
        push_eof.name = "on_eof";
        push_eof.type = LangAPI::ValueType::Void;
        push_eof.override = true;
        auto eof = tree.getTreeMap().end();
        for (auto it = tree.getTreeMap().begin(); it != tree.getTreeMap().end(); ++it)
            if (!it->first.empty() && it->first.back() == "END_INPUT") {
                if (eof != tree.getTreeMap().end())
                    throw std::invalid_argument("multiple END_INPUT token rules");
                eof = it;
            }
        if (eof != tree.getTreeMap().end()) {
            push_eof.statements.push_back(LangAPI::Variable::createStatement(LangAPI::Variable{
                .name = "start", .type = LangAPI::ValueType::NonOwnedString,
                .value = LangAPI::FunctionCall::createExpression(LangAPI::FunctionCall{
                    .name = std::make_shared<LangAPI::Symbol>(LangAPI::Symbol{"std", "strchr"}),
                    .args = {LangAPI::Symbol::createExpression(LangAPI::Symbol{"_in"}),
                        LangAPI::Int::createExpression(LangAPI::Int{.value = 0})}})}));
            push_eof.statements.push_back(LangAPI::Variable::createStatement(LangAPI::Variable{
                .name = "start_pos", .type = LangAPI::ValueType::Int,
                .value = LangAPI::FunctionCall::createExpression(LangAPI::FunctionCall{
                    .name = std::make_shared<LangAPI::Symbol>(LangAPI::Symbol{"std", "strlen"}),
                    .args = {LangAPI::Symbol::createExpression(LangAPI::Symbol{"_in"})}})}));
            push_eof.statements.push_back(LangAPI::Variable::createStatement(LangAPI::Variable{
                .name = "length", .type = LangAPI::ValueType::Int,
                .value = LangAPI::Int::createExpression(LangAPI::Int{.value = 0})}));
            push_eof.statements.push_back(LangAPI::Variable::createStatement(LangAPI::Variable{
                .name = "line", .type = LangAPI::ValueType::Int,
                .value = LangAPI::FunctionCall::createExpression(LangAPI::FunctionCall{
                    .name = std::make_shared<LangAPI::Symbol>(LangAPI::Symbol{"__line"}),
                    .args = {LangAPI::Symbol::createExpression(LangAPI::Symbol{"start"})}})}));
            push_eof.statements.push_back(LangAPI::Variable::createStatement(LangAPI::Variable{
                .name = "ispa_token_text", .type = LangAPI::ValueType::String,
                .value = LangAPI::String::createExpression(LangAPI::String{.value = ""})}));
            LangAPI::Symbol eof_type{"Types"};
            for (const auto& part : eof->first) eof_type.path.emplace_back(part);
            push_eof.statements.push_back(LangAPI::Variable::createStatement(LangAPI::Variable{
                .name = "ispa_token_result", .type = LangAPI::Type{eof_type}}));
            std::set<std::string> locals, objects;
            const auto classes = pythonClasses(tree.getPythonDeclarations());
            for (const auto& feature : eof->second.features)
                if (auto action = std::get_if<AST::PythonAction>(&feature)) {
                    auto code = Parser::ParserBase::convertPythonAction(*action, locals, false, classes, &objects,
                        corelib::text::join(eof->first, "."));
                    push_eof.statements.insert(push_eof.statements.end(), code.begin(), code.end());
                }
        }
        push_eof.statements.insert(push_eof.statements.end(), flush.begin(), flush.end());
        push_eof.statements.push_back(callStatement(memberCall("tokens", "emplace_back")));
        lexer.data.emplace_back(std::make_shared<LangAPI::Declaration>(LangAPI::Function::createDeclaration(push_eof)),
            LangAPI::Visibility::Public);
        if (std::ranges::any_of(tree.getTreeMap(), [](const auto& entry) { return entry.second.external.has_value(); })) {
            LangAPI::Function external;
            external.name = "hasExternalRules";
            external.type = LangAPI::Type{LangAPI::ValueType::Bool};
            external.is_const = true;
            external.override = true;
            external.statements = LangAPI::Return::createStatements(LangAPI::Return{
                .value = LangAPI::Bool::createExpression(LangAPI::Bool{.value = true})});
            lexer.data.emplace_back(std::make_shared<LangAPI::Declaration>(LangAPI::Function::createDeclaration(external)), LangAPI::Visibility::Public);
        }

        const auto &dfa = lexer_builder.getDFA();
        const auto &class_table = dfa.table;
        const auto &states = dfa.states.get();
        const std::size_t state_count = states.size();
        const std::size_t class_count = class_table.num_classes;

        lexer.data.push_back(makeCharClassTableDecl(class_table));

        lexer.data.push_back(makeDfaTableDecl(states, state_count, class_count));

        lexer.data.push_back(makeActionTableDecl(lexer_builder.getActionTable(), state_count));
        lexer.data.push_back(std::make_pair(std::make_shared<LangAPI::Declaration>(LangAPI::Function::createDeclaration(makeSemanticSwitchFunction(lexer_builder.getSemanticTable()))), LangAPI::Visibility::Private));
        if (lexer_builder.getDFA().states.size() > 0) {
            lexer.data.push_back(std::make_pair(
                std::make_shared<LangAPI::Declaration>(LangAPI::Variable::createDeclaration(LangAPI::Variable {
                    .name = "init_done",
                    .type = LangAPI::ValueType::Bool,
                    .value = LangAPI::Bool::createExpression(LangAPI::Bool {.value = false})
                })),
                LangAPI::Visibility::Private
            ));
            lexer.data.push_back(std::make_pair(
                std::make_shared<LangAPI::Declaration>(LangAPI::Function::createDeclaration(LangAPI::Function {
                    .type = LangAPI::ValueType::Void, .name = "init", .statements = LangAPI::Return::createStatements(LangAPI::Return {}), .override = true
                })),
                LangAPI::Visibility::Private
            ));
            lexer.data.push_back(std::make_pair(
                std::make_shared<LangAPI::Declaration>(LangAPI::Function::createDeclaration(LangAPI::Variable {
                    .name = "tdfa_registers",
                    .type = LangAPI::Type { LangAPI::IspaLibSymbol {.exports = LangAPI::StdlibExports::TdfaLayout, .template_parameters = {
                        std::make_shared<LangAPI::RValue>(LangAPI::Int {.value = (long long) lexer_builder.getRegistersCount()}),
                        std::make_shared<LangAPI::RValue>(LangAPI::Int {.value = (long long) lexer_builder.getOutputCount()})
                    }}}
                })),
                LangAPI::Visibility::Private
            ));
            auto debug = makeDebugTable(states, state_count, class_count);
            if (debug.has_value()) {
                auto [debug_array, debug_index] = debug.value();
                lexer.data.push_back(std::make_pair(
                    std::make_shared<LangAPI::Declaration>(debug_array),
                    LangAPI::Visibility::Private
                ));
                lexer.data.push_back(std::make_pair(
                    std::make_shared<LangAPI::Declaration>(debug_index),
                    LangAPI::Visibility::Private
                ));
            }
        }

        LangAPI::Lambda semantic_callback;
        semantic_callback.parameters = {
            {LangAPI::Type{LangAPI::ValueType::Int}, "state"},
            {LangAPI::Type{LangAPI::IspaLibSymbol{.exports = LangAPI::StdlibExports::DfaCaptures,
                .Const = true, .Reference = true}}, "captures"},
            {LangAPI::Type{LangAPI::ValueType::Int}, "start_pos"},
            {LangAPI::Type{LangAPI::ValueType::NonOwnedString}, "start"},
            {LangAPI::Type{LangAPI::ValueType::Int}, "length"},
            {LangAPI::Type{LangAPI::ValueType::Int}, "line"}
            ,{LangAPI::Type{LangAPI::ValueType::BorrowSemantic,
                LangAPI::Type{LangAPI::ValueType::NonOwnedString}}, "cursor"}
        };
        semantic_callback.statements = LangAPI::Return::createStatements(LangAPI::Return{
            .value = LangAPI::FunctionCall::createExpression(LangAPI::FunctionCall{
                .name = std::make_shared<LangAPI::Symbol>(LangAPI::Symbol{"semantic_action_exec"}),
                .args = {LangAPI::Symbol::createExpression(LangAPI::Symbol{"state"}),
                    LangAPI::Symbol::createExpression(LangAPI::Symbol{"captures"}),
                    LangAPI::Symbol::createExpression(LangAPI::Symbol{"start_pos"}),
                    LangAPI::Symbol::createExpression(LangAPI::Symbol{"start"}),
                    LangAPI::Symbol::createExpression(LangAPI::Symbol{"length"}),
                    LangAPI::Symbol::createExpression(LangAPI::Symbol{"line"}),
                    LangAPI::Symbol::createExpression(LangAPI::Symbol{"cursor"})}})});

        lexer.data.push_back(
            std::make_pair(
                std::make_shared<LangAPI::Declaration>(LangAPI::Function::createDeclaration(LangAPI::Function {
                    .type = LangAPI::Type {Token},
                    .name = "makeToken",
                    .parameters = {std::make_pair(
                        LangAPI::Type {LangAPI::IspaLibSymbol {
                            .exports = LangAPI::StdlibExports::LexerMakeTokenParameter,
                            .Const = true, .Reference = true
                        }},
                        std::string("pos")
                    )},
                    .statements = LangAPI::Return::createStatements(
                        LangAPI::Return {
                            .value = LangAPI::FunctionCall::createExpression(LangAPI::FunctionCall {
                                .name = std::make_shared<LangAPI::Symbol>(LangAPI::Symbol {"lookup"}),
                                .args =
                                {
                                    LangAPI::Symbol::createExpression(LangAPI::Symbol {"dfa_table"}),
                                    LangAPI::Symbol::createExpression(LangAPI::Symbol {"char_class_table"}),
                                    LangAPI::Symbol::createExpression(LangAPI::Symbol {"action_table"}),
                                    LangAPI::Symbol::createExpression(LangAPI::Symbol {"tdfa_registers"}),
                                    LangAPI::Lambda::createExpression(semantic_callback),
                                    LangAPI::Symbol::createExpression(LangAPI::Symbol {"debug_array"}),
                                    LangAPI::Symbol::createExpression(LangAPI::Symbol {"debug_index"}),
                                    LangAPI::Symbol::createExpression(LangAPI::Symbol {"pos"}),
                                }
                            })
                        }
                    ),
                    .override = true,
                    .is_wrap_function = true
                })),
                LangAPI::Visibility::Public
            )
        );

        holder.push(lexer);
    }
}
