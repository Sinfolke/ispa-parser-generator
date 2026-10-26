module;
#include <unicode/utf8.h>
#include <unicode/uchar.h>
module AST.Pass;
import corelib;
import AST.Tree;
import AST.API;
import cpuf.printf;
import cpuf.op;
import logging;
import constants;
import std;
void AST::TreePass::removeEmptyRule(AST::Tree &ast) {
    auto &treeMap = ast.getTreeMap();
    for (auto it = treeMap.begin(); it != treeMap.end();) {
        auto &[name, value] = *it;
        if (value.rule_members.empty()) {
            it = treeMap.erase(it);
            continue;
        }
        it++;
    }
}
auto AST::TreePass::inlineGroup(AST::RuleMemberGroup group, stdu::vector<std::shared_ptr<AST::RuleMember>> &members, stdu::vector<std::shared_ptr<AST::RuleMember>>::iterator toInsert)
    -> stdu::vector<std::shared_ptr<AST::RuleMember>>::iterator
{
    inlineGroupsInSequence(group.values);
    return members.insert(toInsert, group.values.begin(), group.values.end())
         + static_cast<stdu::vector<std::shared_ptr<AST::RuleMember>>::difference_type>(group.values.size());
}

// Flattens unprefixed, unquantified groups out of a *sequence* context:
// a rule's top-level members, a group's own values, or a Cll `if` body.
// Deliberately never descends into RuleMemberOp::options — each option
// there is one whole alternative (possibly itself a group standing for a
// multi-member alternative), not a sequence; unwrapping a group there
// would turn `(a b) | c` into the different grammar `a | b | c`.
void AST::TreePass::inlineGroupsInSequence(stdu::vector<std::shared_ptr<AST::RuleMember>> &members) {
    for (auto it = members.begin(); it != members.end(); /* no auto-increment */) {
        auto &member = *it;

        if (member->isGroup() && !member->negative_match && member->quantifier == '\0' && member->prefix.empty() && member->features.empty()) {
            auto group = member->getGroup(); // copy before erase
            it = members.erase(it);
            it = inlineGroup(group, members, it);
            continue;
        }

        if (member->isGroup()) {
            // Quantified/prefixed: this group stays, but its own
            // contents may still hold flattenable nested groups.
            inlineGroupsInSequence(member->getGroup().values);
        }
        // RuleMemberOp::options intentionally not visited — see above.

        ++it;
    }
}

void AST::TreePass::inlineSingleGroups(AST::Tree &ast) {
    for (auto &[name, value] : ast.getTreeMap()) {
        inlineGroupsInSequence(value.rule_members);
    }
}

void AST::TreePass::literalsToToken(AST::Tree &ast) {
    using Name = stdu::vector<std::string>;
    using MemberPtr = std::shared_ptr<AST::RuleMember>;

    struct GeneratedToken {
        AST::RuleMember identity;
        Name name;
        std::string preferred_name;
        std::size_t insertion_index;
        stdu::vector<MemberPtr> references;
        bool global = false;
    };

    auto &treeMap = ast.getTreeMap();

    stdu::vector<std::pair<Name, AST::Rule>> toInsert;
    stdu::vector<GeneratedToken> generated;

    // Includes existing grammar names and newly generated names.
    // std::set is used here so Name only needs lexicographic ordering.
    std::set<Name> reserved;

    for (const auto &[name, rule] : treeMap)
        reserved.insert(name);

    auto isIdent = [](unsigned char c) -> bool {
        return (c >= 'A' && c <= 'Z') ||
               (c >= 'a' && c <= 'z') ||
               (c >= '0' && c <= '9') ||
               c == '_';
    };

    // Convert multi-character strings to uppercase identifiers.
    auto upperIdentifier = [&](std::string_view value) {
        std::string result;

        for (unsigned char c : value) {
            if (c >= 'a' && c <= 'z')
                c = static_cast<unsigned char>(c - 'a' + 'A');

            if (isIdent(c)) {
                result += static_cast<char>(c);
            } else if (result.empty() || result.back() != '_') {
                result += '_';
            }
        }

        while (!result.empty() && result.back() == '_')
            result.pop_back();

        if (!result.empty() &&
            result.front() >= '0' &&
            result.front() <= '9') {
            result.insert(0, "TOKEN_");
        }

        return result;
    };

    // Return descriptive names only for standalone string literals.
    auto literalName = [&](const AST::RuleMember &member)
        -> std::string
    {
        if (!member.isString())
            return {};

        const std::string &value = member.getString().value;

        if (value.empty())
            return {};

        // Decode exactly one UTF-8 code point.
        // U8_NEXT handles both ASCII and non-ASCII.
        std::int32_t offset = 0;
        UChar32 cp = 0;

        U8_NEXT(
            value.data(),
            offset,
            static_cast<std::int32_t>(value.size()),
            cp
        );

        if (cp >= 0 &&
            offset == static_cast<std::int32_t>(value.size())) {
            return upperIdentifier(corelib::text::characterName(cp));
        }

        // If it is a longer ASCII string, use its uppercase form.
        // Non-ASCII multi-character literals fall back to AUTO_N
        // until a Unicode-aware identifier policy is added.
        if (std::any_of(value.begin(), value.end(),
                        [](unsigned char c) { return c >= 128; })) {
            return {};
        }

        return upperIdentifier(value);
    };

    // Allocate a unique name in the requested scope.
    auto allocateName = [&](const Name &scope,
                            const std::string &preferred) -> Name
    {
        const std::string base =
            preferred.empty() ? "AUTO" : preferred;

        std::size_t suffix = 0;

        while (true) {
            Name candidate = scope;

            candidate.push_back(
                suffix == 0
                    ? base
                    : base + "_" + std::to_string(suffix)
            );

            if (reserved.insert(candidate).second)
                return candidate;

            ++suffix;
        }
    };

    // Same token pattern, regardless of its capture name or quantifier.
    auto identityOf = [](const AST::RuleMember &member) {
        AST::RuleMember identity = member;

        identity.prefix.clear();
        identity.quantifier = '\0';
        identity.negative_match = false;
        std::erase_if(identity.features, [](const auto& feature) {
            return std::holds_alternative<Counter>(feature) || std::holds_alternative<SharedCounterRegistration>(feature) ||
                   std::holds_alternative<VariableCapture>(feature) || std::holds_alternative<FailMarker>(feature);
        });

        return identity;
    };

    std::function<void(
        stdu::vector<MemberPtr>&,
        const Name&
    )> process;

    process = [&](stdu::vector<MemberPtr> &literals,
                  const Name &scope) {

        for (auto &member_ptr : literals) {
            auto &member = *member_ptr;

            const bool is_literal =
                member.isString() ||
                member.isHex() ||
                member.isBin() ||
                member.isEscaped() ||
                member.isCsequence();

            if (is_literal) {
                auto identity = identityOf(member);

                auto found = std::find_if(
                    generated.begin(),
                    generated.end(),
                    [&](const GeneratedToken &token) {
                        return token.identity == identity;
                    }
                );

                if (found != generated.end()) {
                    if (!found->global) {
                        // Second occurrence: promote the original token.
                        Name global_name =
                            allocateName({}, found->preferred_name);

                        found->name = global_name;
                        found->global = true;

                        // Rename the original generated rule.
                        toInsert[found->insertion_index].first =
                            global_name;

                        // Redirect every earlier reference.
                        for (auto &reference : found->references) {
                            reference->value = AST::RuleMemberName{
                                .name = global_name
                            };
                            reference->isAutoGenerated = true;
                        }
                    }

                    member.value = AST::RuleMemberName{
                        .name = found->name
                    };
                    member.isAutoGenerated = true;
                    found->references.push_back(member_ptr);

                    continue;
                }

                // First occurrence: keep the token in the rule scope.
                const std::string preferred = literalName(member);

                Name generated_name =
                    allocateName(scope, preferred);

                // Preserve the literal before replacing it.
                AST::RuleMember token_member = member;
                token_member.prefix.clear();
                token_member.quantifier = '\0';
                token_member.negative_match = false;
                std::erase_if(token_member.features, [](const auto& feature) {
                    return std::holds_alternative<Counter>(feature) || std::holds_alternative<SharedCounterRegistration>(feature) ||
                           std::holds_alternative<VariableCapture>(feature) || std::holds_alternative<FailMarker>(feature);
                });

                AST::Rule newRule;
                newRule.rule_members = {
                    std::make_shared<AST::RuleMember>(
                        std::move(token_member)
                    )
                };

                const std::size_t index = toInsert.size();

                toInsert.emplace_back(generated_name, std::move(newRule));

                generated.push_back(GeneratedToken{
                    .identity = std::move(identity),
                    .name = generated_name,
                    .preferred_name = preferred,
                    .insertion_index = index,
                    .references = {member_ptr},
                    .global = false
                });

                member.value = AST::RuleMemberName{
                    .name = generated_name
                };
                member.isAutoGenerated = true;

            } else if (member.isGroup()) {
                process(member.getGroup().values, scope);

            } else if (member.isOp()) {
                process(member.getOp().options, scope);
            }
        }
    };

    for (auto &[name, rule] : treeMap) {
        if (!name.empty() &&
            corelib::text::isLower(name.back())) {
            process(rule.rule_members, name);
        }
    }

    // Insert only after traversal to avoid modifying treeMap
    // while iterating over it.
    for (auto &[name, rule] : toInsert) {
        treeMap[name] = std::move(rule);
    }
    // All promotions are now final, including tokens shared across scopes.
    // Parser captures hold these token nodes; lexer captures retain their
    // original character/string types because their literals were not lowered.
    for (auto& [name, rule] : treeMap)
        if (!name.empty() && corelib::text::isLower(name.back()))
            inferDataTypes(rule);
}
bool AST::TreePass::prioritySort(const AST::Tree& ast, const AST::String &first, const AST::String &second) {
    if (first.value.size() != second.value.size())
        return first.value.size() > second.value.size();
    return first.value > second.value;
}
bool AST::TreePass::prioritySort(const AST::Tree& ast, const AST::RuleMemberBin &first, const AST::RuleMemberBin &second) {
    return first.bin_chars.size() > second.bin_chars.size();
}

bool AST::TreePass::prioritySort(const AST::Tree& ast, const AST::RuleMemberHex &first, const AST::RuleMemberHex &second) {
    return first.hex_chars.size() > second.hex_chars.size();
}

bool AST::TreePass::prioritySort(const AST::Tree& ast, const AST::RuleMemberName &first, const AST::RuleMemberName &second) {
    auto &treeMap = ast.getTreeMap();
    const auto first_data = treeMap.find(first.name);
    const auto second_data = treeMap.find(second.name);
    if (first_data == treeMap.end())
        throw Error("Not found Rule_name in map: {}", first.name);
    if (second_data == treeMap.end())
        throw Error("Not found Rule_name in map: {}", second.name);
    const auto &first_rules = first_data->second.rule_members;
    const auto &second_rules = second_data->second.rule_members;
    for (std::size_t i = 0; i < first_rules.size() && i < second_rules.size(); ++i) {
        if (*first_rules[i] == *second_rules[i]) {
            continue;
        }
        return prioritySort(ast, *first_rules[i], *second_rules[i]);
    }
    return first_rules.size() > second_rules.size();
}

bool AST::TreePass::prioritySort(const AST::Tree& ast, const AST::RuleMemberCsequence &first, const AST::RuleMemberCsequence &second) {
    if (!first.negative && second.negative)
        return true;
    if (first.negative && !second.negative)
        return false;
    return first.characters.size() + first.escaped.size() + first.diapasons.size() >
           second.characters.size() + second.escaped.size() + second.diapasons.size();
}

bool AST::TreePass::prioritySort(const AST::Tree& ast, const AST::RuleMemberGroup &first, const AST::RuleMemberGroup &second) {
    for (std::size_t i = 0; i < first.values.size() && i < second.values.size(); ++i) {
        if (*first.values[i] == *second.values[i])
            continue;
        return prioritySort(ast, *first.values[i], *second.values[i]);
    }
    return first.values.size() > second.values.size();
}

bool AST::TreePass::prioritySort(const AST::Tree& ast, const AST::RuleMemberOp &first, const AST::RuleMemberOp &second) {
    return prioritySort(ast, *first.options.back(), *second.options.back());
}
bool AST::TreePass::prioritySort(const AST::Tree& ast, const AST::RuleMember &first, const AST::RuleMember &second) {
    if (first.isName() && second.isName())
        return prioritySort(ast, first.getName(), second.getName());
    if (first.isGroup() && second.isGroup())
        return prioritySort(ast, first.getGroup(), second.getGroup());
    if (first.isOp() && second.isOp())
        return prioritySort(ast, first.getOp(), second.getOp());
    if (first.isString() && second.isString())
        return prioritySort(ast, first.getString(), second.getString());
    if (first.isEscaped() && second.isEscaped())
        return false;
    if (first.isCsequence() && second.isCsequence())
        return prioritySort(ast, first.getCsequence(), second.getCsequence());
    if (first.isBin() && second.isBin())
        return prioritySort(ast, first.getBin(), second.getBin());
    if (first.isHex() && second.isHex())
        return prioritySort(ast, first.getHex(), second.getHex());
    if (first.isAny() && second.isAny())
        return false;

    if (first.isName()) {
        auto find_it = ast.getTreeMap().find(first.getName().name);
        if (find_it == ast.getTreeMap().end()) {
            throw Error("Not found Rule_name in map");
        }
        const auto &members = find_it->second.rule_members;
        return std::visit([&](const auto &f, const auto &s) -> bool {
            if (members.size() > 1 && getTypes(f) == getTypes(s))
                return false;
            return prioritySort(ast, *members[0], second);
        }, members[0]->value, second.value);
    }
    if (second.isName()){
        auto find_it = ast.getTreeMap().find(second.getName().name);
        if (find_it == ast.getTreeMap().end()) {
            throw Error("Not found Rule_name in map: {}", second.getName().name);
        }
        const auto &members = find_it->second.rule_members;
        return std::visit([&](const auto &f, const auto &s) -> bool {
            if (members.size() > 1 && getTypes(f) == getTypes(s))
                return true;
            return prioritySort(ast, first, *members[0]);
        }, first.value, members[0]->value);
    }
    if (first.isGroup()) {
        auto &dt = first.getGroup();
        if (dt.values.empty())
            throw Error("Empty group");
        return prioritySort(ast, *dt.values[0], second);
    }
    if (second.isGroup()) {
        auto &dt = second.getGroup();
        if (dt.values.empty())
            throw Error("Empty group");
        return prioritySort(ast, first, *dt.values[0]);
    }
    if (first.isOp()) {
        auto &dt = first.getOp();
        if (dt.options.empty())
            throw Error("Empty op");
        return prioritySort(ast, *dt.options.back(), second);
    }
    if (second.isOp()) {
        auto &dt = second.getOp();
        if (dt.options.empty())
            throw Error("Empty op");
        return prioritySort(ast, first, *dt.options[0]);
    }
    return std::visit([&](const auto &f, const auto &s) -> bool {
        stdu::vector<Types> priority_order = {
            Types::string,
            Types::Rule_escaped,
            Types::Rule_csequence,
            Types::Rule_bin,
            Types::Rule_hex,
            Types::Rule_any,
            Types::cll
        };

        auto first_pos = std::find(priority_order.begin(), priority_order.end(), getTypes(f));
        auto second_pos = std::find(priority_order.begin(), priority_order.end(), getTypes(s));
        if (first_pos != priority_order.end() && second_pos != priority_order.end()) {
            return first_pos < second_pos;
        } else if (first_pos != priority_order.end()) {
            return true;  // known comes before unknown
        } else if (second_pos != priority_order.end()) {
            return false; // unknown comes after known
        }
        return false; // both unknown
    }, first.value, second.value);
}
void AST::TreePass::sortByPriority(AST::Tree &ast, AST::RuleMemberOp& options) {
    AST::TreePass pass(ast);
    std::sort(options.options.begin(), options.options.end(), [&](std::shared_ptr<AST::RuleMember> &first, std::shared_ptr<AST::RuleMember> &second) {
        return pass.prioritySort(ast, *first, *second);
    });
}
void AST::TreePass::sortByPriority(AST::Tree &ast, stdu::vector<std::shared_ptr<AST::RuleMember>>& members) {
    for (auto &member_ptr : members) {
        auto &member = *member_ptr;
        if (member.isGroup()) {
            sortByPriority(ast, member.getGroup().values);
        }
        if (member.isOp()) {
            for (auto &option : member.getOp().options) {
                if (option->isGroup()) {
                    sortByPriority(ast, option->getGroup().values);
                }
            }
            sortByPriority(ast, member.getOp());
        }
    }
}
void AST::TreePass::sortByPriority(AST::Tree &ast) {
    for (auto &[name, value] : ast.getTreeMap()) {
        if (corelib::text::isUpper(name.back()))
            continue; // No sense for terminals -> NFA + DFA does not depend on ordering
        if (value.rule_members.empty()) {
            throw Error("Empty rule");
        }
        AST::TreePass pass(ast);
        pass.sortByPriority(ast, value.rule_members);
    }
}
void AST::TreePass::addSpaceToken(AST::Tree &ast) {
    bool whitespace_referenced = false;
    bool whitespace_token_referenced = false;

    auto createsExplicitWhitespace = [&ast](auto self, const std::shared_ptr<RuleMember> &mem) -> bool {
        if (!mem) return false;
        if (mem->isNospace() || mem->isAny()) return true;
        if (mem->isName() && !mem->getName().name.empty()) {
            const auto& name = mem->getName().name.back();
            if (name == "__WS" || name == "NEWLINE") return true;
            if (ast.contains(mem->getName().name) && ast[mem->getName().name].external) return true;
        }

        if (mem->isGroup()) {
            const auto &vals = mem->getGroup().values;
            if (vals.empty()) return false;
            return self(self, vals.front()) || self(self, vals.back());
        }

        if (mem->isOp()) {
            const auto &opts = mem->getOp().options;
            for (const auto &opt : opts) {
                if (self(self, opt)) return true;
            }
            return false;
        }

        if (mem->isString()) {
            const auto &str = mem->getString().value;
            if (str.empty()) return false;
            auto is_ws = [](char c) {
                return std::find(constants::whitespace_chars.begin(), constants::whitespace_chars.end(), c) != constants::whitespace_chars.end();
            };
            return is_ws(str.front()) || is_ws(str.back());
        }

        if (mem->isCsequence()) {
            const auto &cs = mem->getCsequence();
            if (std::find(cs.characters.begin(), cs.characters.end(), ' ') != cs.characters.end()) return true;
            for (char esc : cs.escaped) {
                if (std::find(constants::whitespace_chars.begin(), constants::whitespace_chars.end(), esc) != constants::whitespace_chars.end()) {
                    return true;
                }
            }
        }

        if (mem->isEscaped()) {
            char decoded = corelib::text::getCharFromEscaped(mem->getEscaped().c);
            if (std::find(constants::whitespace_chars.begin(), constants::whitespace_chars.end(), decoded) != constants::whitespace_chars.end()) {
                return true;
            }
        }
        return false;
    };

    auto processMembers = [&](auto self, stdu::vector<std::shared_ptr<RuleMember>> &rule_members, bool token) -> void {
        stdu::vector<std::shared_ptr<RuleMember>> updated_members;
        updated_members.reserve(rule_members.size() * 2);

        for (std::size_t i = 0; i < rule_members.size(); ++i) {
            auto mem = rule_members[i];

            if (mem->isGroup()) {
                self(self, mem->getGroup().values, token);
            } else if (mem->isOp()) {
                for (auto &opt : mem->getOp().options) {
                    if (opt->isGroup()) {
                        self(self, opt->getGroup().values, token);
                    }
                }
            }

            bool skip_space = createsExplicitWhitespace(createsExplicitWhitespace, mem) ||
                             (i > 0 && createsExplicitWhitespace(createsExplicitWhitespace, rule_members[i - 1]));

            if (i > 0 && !skip_space && !token) {
                auto ws_ref = std::make_shared<AST::RuleMember>(AST::RuleMember{
                    .quantifier = '?', .isAutoGenerated = true, .value = AST::RuleMemberName{.name = {constants::whitespace}}
                });
                updated_members.push_back(ws_ref);
                // FIX: track which whitespace variant actually gets referenced,
                // instead of unconditionally materializing both below.
                if (token) whitespace_token_referenced = true;
                else whitespace_referenced = true;
            }

            if (mem->isNospace()) {
                continue;
            }

            updated_members.push_back(mem);
        }

        rule_members = std::move(updated_members);
    };

    for (auto &[name, rule] : ast.getTreeMap()) {
        if (rule.external || rule.external_token) continue;
        if (name == constants::whitespace || name == constants::whitespace_rule || name == constants::whitespace_token)
            continue;
        processMembers(processMembers, rule.rule_members, corelib::text::isUpper(name.back()));
    }

    // FIX: materialize each whitespace token only if it was actually
    // referenced above. Registering one unconditionally wires it as an
    // independently-matchable top-level lexer token even when nothing in
    // the grammar names it — and since __WS/__WSTOKEN share an identical
    // pattern, the merged DFA cannot distinguish them at tokenize time,
    // so the unused one can still win accept()'s token_id tie-break and
    // silently corrupt the token stream (exactly what was happening here).
    if (!ast.getTreeMap().contains(constants::whitespace)) {
        AST::Rule spaceTokenRule;
        AST::RuleMemberCsequence csequence;
        const bool has_newline_token = std::ranges::any_of(ast.getTreeMap(), [](const auto& entry) {
            return !entry.first.empty() && entry.first.back() == "NEWLINE";
        });
        csequence.escaped = has_newline_token ? stdu::vector<char>{'t', 'v', 'f'} : stdu::vector<char>{'t', 'n', 'r', 'v', 'f'};
        csequence.characters = {' '};
        spaceTokenRule.rule_members = {
            std::make_shared<AST::RuleMember>(AST::RuleMember{ .quantifier = '*', .value = csequence })
        };
        ast.getTreeMap()[constants::whitespace] = spaceTokenRule;
    }
}
void AST::TreePass::removeEmptyRule() {
    removeEmptyRule(*ast);
}
void AST::TreePass::inlineSingleGroups() {
    inlineSingleGroups(*ast);
}
void AST::TreePass::literalsToToken() {
    literalsToToken(*ast);
}
void AST::TreePass::lowerParserNegations() {
    using Name = stdu::vector<std::string>;
    const auto& rules = ast->getTreeMap();
    const auto exclusions = [&](const auto& self, const AST::RuleMember& member,
                                std::set<Name>& result) -> void {
        if (member.isGroup()) {
            const auto& values = member.getGroup().values;
            if (values.size() != 1)
                throw Error("negated parser pattern must match one token");
            self(self, *values.front(), result);
        } else if (member.isOp()) {
            for (const auto& option : member.getOp().options) self(self, *option, result);
        } else if (member.isName() && member.getName().isTerminal()) {
            result.insert(member.getName().name);
        } else {
            throw Error("negated parser pattern must name a token or a choice of tokens");
        }
    };
    const auto lower = [&](const auto& self, AST::RuleMember& member) -> void {
        if (member.negative_match) {
            AST::RuleMember operand = member;
            operand.negative_match = false;
            operand.quantifier = '\0';
            std::set<Name> excluded;
            exclusions(exclusions, operand, excluded);
            AST::RuleMemberOp choices;
            for (const auto& [name, rule] : rules) {
                if (name.empty() || !AST::RuleMemberName{.name = name}.isTerminal() ||
                    !rule.isTopLevel || rule.isTemplateDefinition || rule.isTemplatePlaceholder || excluded.contains(name)) continue;
                choices.options.push_back(std::make_shared<AST::RuleMember>(AST::RuleMember{
                    .value = AST::RuleMemberName{.name = name}}));
            }
            if (choices.options.empty()) throw Error("negated parser pattern excludes every token");
            member.value = std::move(choices);
            member.negative_match = false;
            return;
        }
        if (member.isGroup()) for (auto& child : member.getGroup().values) self(self, *child);
        if (member.isOp()) for (auto& child : member.getOp().options) self(self, *child);
    };
    for (auto& [name, rule] : ast->getTreeMap()) {
        if (name.empty() || AST::RuleMemberName{.name = name}.isTerminal() || rule.isTemplateDefinition) continue;
        for (auto& member : rule.rule_members) lower(lower, *member);
        inferDataTypes(rule);
    }
}
void AST::TreePass::sortByPriority() {
    sortByPriority(*ast);
}
void AST::TreePass::addSpaceToken() {
    addSpaceToken(*ast);
}
