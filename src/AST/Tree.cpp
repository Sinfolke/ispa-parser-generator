module AST.Tree;
import AST.Pass;
import logging;
import dstd;
import AST.API;
import LLIR.Builder;
import LLIR.Builder.DataWrapper;
import LLIR.Rule.MemberBuilder;
import LLIR.Builder.Data;
import cpuf.printf;
import cpuf.op;
import constants;
import std;

namespace {
    class AstInputGenerator {
        AST::InitialItemSet& itemSet;

        static auto isWhitespaceName(const stdu::vector<std::string>& name) -> bool {
            if (name.empty()) return false;
            return name == constants::whitespace ||
                   name == constants::whitespace_token ||
                   name == constants::whitespace_rule
            ;
        }

        static auto cartesianProduct(const std::vector<std::vector<std::string>>& choices, std::size_t maxLimit = 16) -> std::vector<std::string> {
            std::vector<std::string> result = {""};
            for (const auto& choice : choices) {
                if (choice.empty()) continue;
                std::vector<std::string> next_result;
                next_result.reserve(std::min(result.size() * choice.size(), maxLimit));

                for (const auto& res : result) {
                    for (const auto& c : choice) {
                        next_result.push_back(res + c);
                        if (next_result.size() >= maxLimit) break;
                    }
                    if (next_result.size() >= maxLimit) break;
                }
                result = std::move(next_result);
            }
            return result;
        }

        auto resolveRuleKey(const stdu::vector<std::string>& currentScope,
                            const stdu::vector<std::string>& refName) const -> std::optional<stdu::vector<std::string>> {
            // 1. Direct match
            if (itemSet.find(refName) != itemSet.end()) return refName;

            // Strip/add '#' from refName segments
            stdu::vector<std::string> cleanRef = refName;
            stdu::vector<std::string> hashedRef = refName;
            for (auto &s : cleanRef) {
                if (!s.empty() && s.front() == '#') s = s.substr(1);
            }
            if (!hashedRef.empty() && (hashedRef.back().empty() || hashedRef.back().front() != '#')) {
                hashedRef.back() = "#" + hashedRef.back();
            }

            if (itemSet.find(cleanRef) != itemSet.end()) return cleanRef;
            if (itemSet.find(hashedRef) != itemSet.end()) return hashedRef;

            // 2. Relative match appended to currentScope
            for (const auto &ref : {refName, cleanRef, hashedRef}) {
                stdu::vector<std::string> scoped = currentScope;
                scoped.insert(scoped.end(), ref.begin(), ref.end());
                if (itemSet.find(scoped) != itemSet.end()) return scoped;
            }

            // 3. Search backwards through parent scope prefixes
            for (std::size_t i = currentScope.size(); i > 0; --i) {
                stdu::vector<std::string> scopePrefix(currentScope.begin(), currentScope.begin() + i);

                for (const auto &ref : {refName, cleanRef, hashedRef}) {
                    stdu::vector<std::string> c = scopePrefix;
                    c.insert(c.end(), ref.begin(), ref.end());
                    if (itemSet.find(c) != itemSet.end()) return c;
                }
            }

            // 4. Fallback search by trailing segment name
            if (!cleanRef.empty()) {
                const auto &target = cleanRef.back();
                for (const auto &[key, _] : itemSet) {
                    if (!key.empty()) {
                        std::string kLast = key.back();
                        if (!kLast.empty() && kLast.front() == '#') kLast = kLast.substr(1);
                        if (kLast == target) return key;
                    }
                }
            }

            return std::nullopt;
        }

        auto expandCsequence(const AST::RuleMemberCsequence& cs) -> std::vector<std::string> {
            std::set<std::string> chars;
            for (char c : cs.characters) chars.insert(std::string(1, c));
            for (char c : cs.escaped) chars.insert(std::string(1, c));

            for (const auto& range : cs.diapasons) {
                chars.insert(std::string(1, range.first));
                chars.insert(std::string(1, range.second));
                if (range.second > range.first + 1) {
                    chars.insert(std::string(1, static_cast<char>((range.first + range.second) / 2)));
                }
            }
            if (chars.empty()) chars.insert("a");
            return {chars.begin(), chars.end()};
        }

        auto expandMemberBase(const AST::RuleMember& member,
                              const stdu::vector<std::string>& currentScope,
                              std::size_t depth,
                              std::size_t maxDepth) -> std::vector<std::string> {
            if (member.isNospace()) return {""};
            if (member.isString()) return {member.getString().value};
            if (member.isEscaped()) return {std::string(1, member.getEscaped().c)};
            if (member.isHex()) {
                const auto& hex = member.getHex().hex_chars;
                if (hex.empty()) return {"0x0"};
                return {std::string(1, hex.front()), std::string(1, hex.back())};
            }
            if (member.isBin()) {
                const auto& bin = member.getBin().bin_chars;
                if (bin.empty()) return {"0b0"};
                return {std::string(1, bin.front()), std::string(1, bin.back())};
            }
            if (member.isAny()) return {"a", "1", "_"};
            if (member.isCsequence()) return expandCsequence(member.getCsequence());

            if (member.isGroup()) {
                std::vector<std::vector<std::string>> groupChoices;
                for (const auto &sub : member.getGroup().values) {
                    groupChoices.push_back(expandMemberRecursive(*sub, currentScope, depth, maxDepth));
                }
                return cartesianProduct(groupChoices);
            }

            if (member.isOp()) {
                std::set<std::string> opResults;
                for (const auto &picked : member.getOp().options) {
                    auto res = expandMemberRecursive(*picked, currentScope, depth, maxDepth);
                    opResults.insert(res.begin(), res.end());
                    if (opResults.size() >= 8) break; // Limit op variant count
                }
                return {opResults.begin(), opResults.end()};
            }

            if (member.isName()) {
                const auto &name = member.getName().name;
                if (isWhitespaceName(name)) {
                    return {"", " "}; // Canonical whitespace samples
                }
                return expandRule(name, currentScope, depth + 1, maxDepth);
            }

            return {""};
        }

        auto expandMemberRecursive(const AST::RuleMember& member,
                                   const stdu::vector<std::string>& currentScope,
                                   std::size_t depth,
                                   std::size_t maxDepth) -> std::vector<std::string> {
            auto base = expandMemberBase(member, currentScope, depth, maxDepth);
            if (member.quantifier == '\0') return base;

            std::set<std::string> results;
            // 0 repetitions
            if (member.quantifier == '?' || member.quantifier == '*') {
                results.insert("");
            }
            // 1 repetition
            if (member.quantifier == '?' || member.quantifier == '*' || member.quantifier == '+') {
                for (const auto& b : base) {
                    results.insert(b);
                    if (results.size() >= 8) break;
                }
            }
            // 2 repetitions (bounded to avoid explosion)
            if (member.quantifier == '*' || member.quantifier == '+') {
                std::size_t count = 0;
                for (const auto& b1 : base) {
                    for (const auto& b2 : base) {
                        results.insert(b1 + b2);
                        if (++count >= 4) break;
                    }
                    if (count >= 4) break;
                }
            }
            return {results.begin(), results.end()};
        }

        auto expandRule(const stdu::vector<std::string>& ruleName,
                        const stdu::vector<std::string>& currentScope,
                        std::size_t depth,
                        std::size_t maxDepth) -> std::vector<std::string> {
            if (isWhitespaceName(ruleName)) {
                return {"", " "};
            }
            if (depth >= maxDepth) return {""};

            auto keyOpt = resolveRuleKey(currentScope, ruleName);
            if (!keyOpt.has_value()) return {"a"};

            const auto &key = *keyOpt;
            auto it = itemSet.find(key);
            if (it == itemSet.end() || it->second.empty()) return {"a"};

            const auto &rules = it->second;
            std::set<std::string> variants;

            for (const auto &rule : rules) {
                std::vector<std::vector<std::string>> memberChoices;
                for (const auto &member_ptr : rule.rule_members) {
                    memberChoices.push_back(expandMemberRecursive(*member_ptr, key, depth, maxDepth));
                }
                auto product = cartesianProduct(memberChoices);
                variants.insert(product.begin(), product.end());
                if (variants.size() >= 16) break;
            }

            return {variants.begin(), variants.end()};
        }

    public:
        explicit AstInputGenerator(AST::InitialItemSet& itemSetRef) : itemSet(itemSetRef) {}

        auto generateTokenSamples(std::size_t maxDepth = 2) -> utype::unordered_map<stdu::vector<std::string>, stdu::vector<std::string>> {
            utype::unordered_map<stdu::vector<std::string>, stdu::vector<std::string>> out;
            for (const auto &[name, rules] : itemSet) {
                if (name.empty() || !corelib::text::isUpper(name.back()) || isWhitespaceName(name)) continue;

                auto samples = expandRule(name, name, 0, maxDepth);
                out[name] = stdu::vector<std::string>{samples.begin(), samples.end()};
            }
            return out;
        }

        auto generateOneStepRuleSamples() -> std::unordered_map<std::string, stdu::vector<std::string>> {
            std::unordered_map<std::string, stdu::vector<std::string>> out;
            for (const auto &[name, rules] : itemSet) {
                if (name.empty() || !corelib::text::isLower(name.back()) || rules.empty() || isWhitespaceName(name)) continue;
                auto key = corelib::text::join(name, "_");

                std::set<std::string> ruleVariants;
                for (const auto &rule : rules) {
                    std::vector<std::vector<std::string>> memberChoices;
                    for (const auto &member_ptr : rule.rule_members) {
                        const auto &member = *member_ptr;
                        if (member.isName()) {
                            const auto &ref = member.getName().name;
                            memberChoices.push_back(expandRule(ref, name, 0, 1));
                        } else {
                            memberChoices.push_back(expandMemberRecursive(member, name, 0, 1));
                        }
                    }
                    auto product = cartesianProduct(memberChoices);
                    ruleVariants.insert(product.begin(), product.end());
                    if (ruleVariants.size() >= 16) break;
                }

                out[key] = stdu::vector<std::string>{ruleVariants.begin(), ruleVariants.end()};
            }
            return out;
        }
    };
}

auto AST::Tree::getTerminals() const -> stdu::vector<stdu::vector<std::string>> {
    stdu::vector<stdu::vector<std::string>> set;
    for (const auto &[name, value] : tree_map) {
        if (corelib::text::isUpper(name.back()))
            set.push_back(name);
    }
    return set;
}
auto AST::Tree::getNonTerminals() const -> stdu::vector<stdu::vector<std::string>> {
    stdu::vector<stdu::vector<std::string>> set;
    for (const auto &[name, value] : tree_map) {
        if (corelib::text::isLower(name.back()))
            set.push_back(name);
    }
    return set;
}
void AST::Tree::getUsePlacesTable(const stdu::vector<std::shared_ptr<AST::RuleMember>> &members, const stdu::vector<std::string> &name) {
    for (const auto &member_ptr : members) {
        auto &member = *member_ptr;
        if (member.isGroup()) {
            getUsePlacesTable(member.getGroup().values, name);
        } else if (member.isOp()) {
            getUsePlacesTable(member.getOp().options, name);
        } else if (member.isName()) {
            use_places[member.getName().name].push_back(name);
        }
    }
}
auto AST::Tree::createUsePlacesTable() -> UsePlaceTable& {
    for (const auto &[name, value] : tree_map) {
        getUsePlacesTable(value.rule_members, name);
    }
    return use_places;
}
auto AST::Tree::compute_group_length(const stdu::vector<std::shared_ptr<AST::RuleMember>> &group) -> std::size_t {
    std::size_t count = 0;
    for (auto &rule : group) {
        if (rule->isGroup() && rule->quantifier == '\0') {
            count += compute_group_length(rule->getGroup().values);
        } else count++;
    }
    return count;
};
void AST::Tree::transform_helper(
    stdu::vector<std::shared_ptr<AST::RuleMember>> &members,
    const stdu::vector<std::string> &fullname,
    const stdu::vector<std::string> &original_fullname,
    utype::unordered_map<stdu::vector<std::string>, std::pair<char, stdu::vector<std::string>>> &replacements
) {
    logger.increaseIndentLevel();
    auto size = members.size();
    for (std::size_t i = 0; i < size; ++i) {
        auto &member_ptr = members[i];
        auto &member = *member_ptr;
        if (member.isGroup()) {
            logger.increaseIndentLevel();
            logger.log("IsGroup, data: ");
            logger.increaseIndentLevel();
            logger.log("{}", member.getGroup().values);
            logger.decreaseIndentLevel();
            auto data = member.getGroup(); // should be copy!!!
            if (member.quantifier == '\0') {
                transform_helper(data.values, fullname, original_fullname, replacements);
                logger.log("Unrolling group with empty quantifier: {}", members);

                // Remove the group at position i
                auto it = members.erase(members.begin() + i);

                // Insert inlined values at position i
                members.insert(it, data.values.begin(), data.values.end());

                logger.log("Unrolled to {}, inserted {} elements at {}", members, data.values.size(), i);

                // Move i past inserted values
                i += data.values.size();  // Already erased 1, inserted N — so we skip N
                i--; // Correct for loop increment

                continue;
            }
            std::string quant_rule_name = "__grp" + std::to_string(i);
            auto quant_fullname = fullname;
            quant_fullname.push_back(quant_rule_name);
            // Recursively process the group
            logger.log("running transform_helper on group");
            transform_helper(data.values, quant_fullname, original_fullname, replacements);

            stdu::vector<stdu::vector<std::shared_ptr<AST::RuleMember>>> new_alternatives;
            logger.log("quant_fullname: {}", quant_fullname);
            // Replace current member with reference to new rule
            member_ptr = std::make_shared<AST::RuleMember>(AST::RuleMember { .value = AST::RuleMemberName {.name = quant_fullname} });

            stdu::vector<stdu::vector<std::shared_ptr<AST::RuleMember>>> new_alts;
            logger.log("dealing with quantifier {}", member.quantifier);
            switch (member.quantifier) {
                case '?':
                    new_alts.push_back({}); // empty
                    new_alts.push_back(data.values);
                    break;

                case '+': {
                    // + = A A*
                    std::string tail_name = quant_rule_name + "_tail";
                    auto tail_fullname = quant_fullname;
                    tail_fullname.push_back(tail_name);

                    // A → data tail
                    auto base = data.values;
                    base.push_back(std::make_shared<AST::RuleMember>(AST::RuleMember { .value = AST::RuleMemberName {.name = tail_fullname} }));
                    new_alts.push_back(base);

                    // A* tail: ε | data tail
                    stdu::vector<stdu::vector<std::shared_ptr<AST::RuleMember>>> tail_alts;
                    tail_alts.push_back({});
                    auto recur = data.values;
                    recur.push_back(std::make_shared<AST::RuleMember>(AST::RuleMember { .value = AST::RuleMemberName {.name = tail_fullname} }));
                    tail_alts.push_back(recur);

                    for (auto &alt : tail_alts)
                        initial_item_set[tail_fullname].push_back(AST::Rule {.rule_members = alt, .original_rules = original_fullname});
                    break;
                }

                case '*': {
                    // * = ε | data A*
                    new_alts.push_back({});
                    auto recur = data.values;
                    recur.push_back(std::make_shared<AST::RuleMember>(AST::RuleMember { .value = AST::RuleMemberName {.name = quant_fullname} }));
                    new_alts.push_back(recur);
                    break;
                }
                default:
                    initial_item_set[quant_fullname].push_back(AST::Rule {
                        .rule_members = data.values,
                        .original_rules = original_fullname
                    });
            }

            for (auto &alt : new_alts) {
                logger.log("alt: ", alt);
                logger.increaseIndentLevel();
                logger.log("{}", alt);
                logger.decreaseIndentLevel();
                initial_item_set[quant_fullname].push_back(AST::Rule {.rule_members = alt, .original_rules = original_fullname});
            }
            logger.log("<leaving group>");
            logger.decreaseIndentLevel();
            continue;
        }
        if (member.isOp()) {
            logger.log("in Op: {}", member.getOp());
            auto data = member.getOp(); // should be copy !!!
            stdu::vector<std::string> push_name;
            stdu::vector<std::shared_ptr<AST::RuleMember>>::iterator val_it;
            stdu::vector<std::pair<std::size_t, std::size_t>> group_pos = {};
            std::size_t _count = 0;
            if (members.size() == 1) {
                logger.log("[branch] members.size() == 1");
                std::size_t count = 0;
                bool is_first_going_group = false;

                for (const auto &rule : data.options) {
                    if (rule->isGroup()) {
                        if (count == 0)
                            is_first_going_group = true;

                        auto len = compute_group_length(rule->getGroup().values);
                        group_pos.push_back({count, len});
                        count += len;
                        continue;
                    }
                    count++;
                }
                logger.log("group_pos: {}", group_pos);
                logger.log("running transform_helper on op");
                transform_helper(data.options, fullname, original_fullname, replacements); // process internal ops/groups
                if (is_first_going_group) {
                    logger.log("[branch] is_first_going_group == true");
                    logger.log("group_pos[0]: {}", group_pos[0]);
                    auto [pos, len] = group_pos[0];
                    auto insert_pos = members.begin() + i;
                    members.erase(insert_pos);  // erase original group placeholder

                    // Reset insert_pos, because erase invalidates it
                    insert_pos = members.begin() + i;

                    std::size_t j = 0;
                    val_it = data.options.begin();
                    logger.increaseIndentLevel();
                    for (; j < len; ++j, ++val_it, ++_count) {
                        logger.log("j: {}, _count: {}, val_it: {}", j, _count, &(*val_it));
                        insert_pos = members.insert(insert_pos, *val_it);
                        ++insert_pos; // move insert position forward for next item
                    }
                    logger.decreaseIndentLevel();

                    // val_it now correctly points to the remaining options, if any
                    val_it = data.options.begin() + j;
                } else {
                    logger.log("[branch] is_first_going_group == false");
                    logger.log("data.options[0]: {}, data.options.begin(): {}, data.options.begin() + 1: {}, _count: {}, _count + 1: {}",
                        *data.options[0], &(*data.options.begin()), &(*data.options.begin()) + 1, _count, _count + 1
                    );
                    logger.log("remain element {} in set", *data.options[0]);
                    members[i] = data.options[0];
                    val_it = data.options.begin() + 1;
                    _count++;
                }

                push_name = fullname;
            } else {
                logger.log("[branch] members.size() == {} (!= 1)", members.size());
                std::string name ="__rop" + std::to_string(i);
                auto new_fullname = fullname;
                new_fullname.push_back(name);
                members[i] = std::make_shared<AST::RuleMember>(AST::RuleMember {.value = AST::RuleMemberName {.name = new_fullname}});

                transform_helper(data.options, fullname, original_fullname, replacements); // process internal ops/groups
                val_it = data.options.begin();
                push_name = new_fullname;
                logger.log("new_fullname: {}", new_fullname);
            }
            for (; val_it != data.options.end(); val_it++, _count++) {
                auto group = std::find_if(group_pos.begin(), group_pos.end(), [&_count](const std::pair<std::size_t, std::size_t> &unit) {return unit.first == _count;});
                stdu::vector<std::shared_ptr<AST::RuleMember>> values;
                logger.log("found group: {}", group != group_pos.end());
                if (group != group_pos.end()) {
                    logger.log("iterating through group");
                    for (std::size_t i = 0; i < group->second && val_it != data.options.end(); i++, _count++, val_it++) {
                        logger.log("i: {}, _count: {}, val_it: {}", i, _count, &(*val_it));
                        values.push_back(*val_it);
                    }
                    val_it--;
                    _count--;
                    logger.log("final _count: {}, val_it: {}", _count, &(*val_it));
                } else {
                    logger.log("inserting raw element {}", **val_it);
                    values.push_back(*val_it);
                }
                initial_item_set[push_name].push_back(AST::Rule {.rule_members = values, .original_rules = original_fullname});
            }
            logger.log("<Leaving Op>");
            continue;
        }
        // Handle standalone quantifier case (e.g., id?, id+, id*)
        if (member.quantifier == '\0')
            continue;

        auto find = replacements.find(fullname);
        if (find != replacements.end() && find->second.first == member.quantifier) {
            member_ptr = std::make_shared<AST::RuleMember>(AST::RuleMember {
                .quantifier = '\0',
                .value = AST::RuleMemberName {.name = find->second.second}
            });
            continue;
        }

        std::string quant_rule_name = "__q" + std::to_string(i);

        auto quant_fullname = fullname;
        quant_fullname.push_back(quant_rule_name);

        AST::RuleMember replaced = member;
        replaced.quantifier = '\0';

        stdu::vector<stdu::vector<std::shared_ptr<AST::RuleMember>>> new_alts;
        logger.log("dealing with quantifier {} for rule {}", member.quantifier, member);
        switch (member.quantifier) {
            case '?':
                new_alts.push_back({});
                new_alts.push_back({std::make_shared<AST::RuleMember>(replaced)});
                break;
            case '+': {
                std::string tail_name = quant_rule_name + "_tail";
                auto tail_fullname = quant_fullname;
                tail_fullname.push_back(tail_name);

                new_alts.push_back({
                    std::make_shared<AST::RuleMember>(replaced),
                    std::make_shared<AST::RuleMember>(AST::RuleMember {
                        .value = AST::RuleMemberName {.name = tail_fullname}
                    })
                });

                stdu::vector<stdu::vector<std::shared_ptr<AST::RuleMember>>> tail_alts = {
                    {},
                    {
                        std::make_shared<AST::RuleMember>(replaced),
                        std::make_shared<AST::RuleMember>(AST::RuleMember {
                            .value = AST::RuleMemberName {.name = tail_fullname}
                        })
                    }
                };
                for (auto &alt : tail_alts) {
                    initial_item_set[tail_fullname].push_back(AST::Rule {.rule_members = alt, .original_rules = original_fullname});
                }
                break;
            }
            case '*':
                new_alts.push_back({});
                new_alts.push_back({
                    std::make_shared<AST::RuleMember>(replaced),
                    std::make_shared<AST::RuleMember>(AST::RuleMember {
                        .value = AST::RuleMemberName {.name = quant_fullname}
                    })
                });
                break;
        }

        for (auto &alt : new_alts) {
            initial_item_set[quant_fullname].push_back(AST::Rule {.rule_members = alt, .original_rules = original_fullname});
        }
    }
    logger.decreaseIndentLevel();
}
void AST::Tree::transform() {
    stdu::vector<stdu::vector<std::string>> keys;
    for (const auto &pair : initial_item_set) {
        if (corelib::text::isLower(pair.first.back()))
            keys.push_back(pair.first);
    }

    utype::unordered_map<stdu::vector<std::string>, std::pair<char, stdu::vector<std::string>>> replacement;
    Tlog::Branch lb(logger, "AST/transform.log");
    for (const auto &name : keys) {
        auto it = initial_item_set.find(name);
        if (it == initial_item_set.end())
            throw Error("Previous key disappeared");
        if (it->second.empty())
            throw Error("Rule {} empty", name);
        logger.log("transforming {}: {}", name, it->second[0].rule_members);
        transform_helper(it->second[0].rule_members, name, name, replacement);
        logger.log("new rule {}: {}", name, it->second[0].rule_members);
    }
}
void AST::Tree::createInitialItemSet() {
    if (!initial_item_set.empty())
        return;
    for (auto [name, value] : tree_map) {
        value.original_rules = {name};
        initial_item_set[name] = {value};
    }
    transform();
}
auto AST::Tree::getInitialItemSet() -> InitialItemSet & {
    if (initial_item_set.empty())
        createInitialItemSet();
    return initial_item_set;
}

// Helper function to determine if a single rule member is nullable
bool AST::Tree::isMemberNullable(const AST::RuleMember& member) const {
    if (member.isNospace()) {
        return true;
    }

    if (member.isName()) {
        return nullable.count(member.getName().name) > 0;
    }

    if (member.isGroup()) {
        // A group (A | B | C) is nullable if ANY of its alternative productions are completely nullable
        for (const auto& alt_rule : member.getGroup().values) {
            bool alt_all_nullable = true;
            if (!isMemberNullable(*alt_rule)) {
                alt_all_nullable = false;
                break;
            }
            if (alt_all_nullable) {
                return true; // Found a completely nullable path through this group
            }
        }
        return false;
    }

    return false;
}

void AST::Tree::computeNullableSet() {
    bool changed;
    do {
        changed = false;
        for (const auto &[nonterminal, productions] : initial_item_set) {
            for (const auto &prod : productions) {
                bool allNullable = true;
                for (const auto &sym : prod.rule_members) {
                    if (!isMemberNullable(*sym)) {
                        allNullable = false;
                        break;
                    }
                }
                if (allNullable && nullable.insert(nonterminal).second) {
                    changed = true;
                }
            }
        }
    } while (changed);
}
void AST::Tree::constructFirstSet(const stdu::vector<AST::Rule>& options, const stdu::vector<std::string>& nonterminal, bool& changed) {
    logger.increaseIndentLevel();
    for (const auto& option : options) {
        bool nullable_prefix = true;
        for (const auto& m : option.rule_members) {
            const auto &member = *m;
            if (member.isNospace())
                continue;

            // Handle Groups native to your structural API
            if (member.isGroup()) {
                logger.log("Encountered inline group inside rule {}", nonterminal);
                const auto& groupValues = member.getGroup().values;

                // Synthesize a temporary single-rule container matching your signature
                AST::Rule tempRule;
                tempRule.rule_members = groupValues;
                constructFirstSet(stdu::vector<AST::Rule>{tempRule}, nonterminal, changed);

                if (!isMemberNullable(member)) {
                    nullable_prefix = false;
                    break;
                }
                continue;
            }

            if (member.isOp()) {
                logger.log("Encountered operator options block inside rule {}", nonterminal);
                AST::Rule tempRule;
                tempRule.rule_members = member.getOp().options;
                constructFirstSet(stdu::vector<AST::Rule>{tempRule}, nonterminal, changed);

                if (!isMemberNullable(member)) {
                    nullable_prefix = false;
                    break;
                }
                continue;
            }

            if (!member.isName()) {
                // If it is a string asset, terminal representation, or hex block
                if (member.isString() || member.isCsequence() || member.isHex() || member.isBin()) {
                    nullable_prefix = false;
                    break;
                }
                throw Error("Unhandled RuleMember variant, rule {} but index {}", nonterminal, member.value.index());
            }

            const auto& rule_name = member.getName();
            auto& currentFirst = first[nonterminal];

            if (rule_name.name == nonterminal) {
                logger.log("rule.name == non-terminal -> continue");
                continue;
            }

            if (rule_name.isNonterminal()) {
                const auto& otherFirst = first[rule_name.name];
                logger.log("inserting non-terminal's[{}] first set: to {}", rule_name.name, nonterminal);
                for (const auto& el : otherFirst) {
                    if (corelib::text::isUpper(el.back()) && el != stdu::vector<std::string>{"ε"} ) continue;
                    if (currentFirst.insert(el).second)
                        changed = true;
                }

                if (nullable.find(rule_name.name) == nullable.end()) {
                    nullable_prefix = false;
                    break;
                }
            } else {
                logger.log("inserting terminal {} to {}", rule_name.name, nonterminal);
                if (currentFirst.insert(rule_name.name).second) {
                    changed = true;
                }
                nullable_prefix = false;
                break;
            }
        }

        if (nullable_prefix) {
            if (first[nonterminal].insert({"ε"}).second) {
                changed = true;
            }
        }
    }
    logger.decreaseIndentLevel();
}
void AST::Tree::constructFirstSet() {
    if (!first.empty())
        return;
    createInitialItemSet();
    computeNullableSet();
    bool changed;
    Tlog::Branch lb(logger, "AST/constructFirstSet.log");
    do {
        changed = false;
        logger.log("------------enter first set------------------");
        for (const auto &[nonterminal, productions] : initial_item_set) {
            logger.dlog("constructing first set for {} -> ", nonterminal);
            if (corelib::text::isUpper(nonterminal.back())) {
                logger.log("skipped\n");
                continue;
            }
            logger.dlog("\n");
            constructFirstSet(productions, productions[0].original_rules, changed);
        }
    } while (changed);
}

void AST::Tree::collectMemberFirst(const AST::RuleMember& member, std::set<stdu::vector<std::string>>& outFirst) {
    if (member.isNospace()) return;

    if (member.isName()) {
        const auto& nameInfo = member.getName();
        if (nameInfo.isTerminal()) {
            outFirst.insert(nameInfo.name);
        } else {
            const auto& f = first[nameInfo.name];
            outFirst.insert(f.begin(), f.end());
        }
        return;
    }

    if (member.isGroup()) {
        for (const auto& sub : member.getGroup().values) {
            collectMemberFirst(*sub, outFirst);
            if (!isMemberNullable(*sub)) break;
        }
    }

    if (member.isOp()) {
        for (const auto& opt : member.getOp().options) {
            collectMemberFirst(*opt, outFirst);
        }
    }
}

void AST::Tree::processFollowForSequence(
    const stdu::vector<std::string>& lhs_name,
    const stdu::vector<std::shared_ptr<AST::RuleMember>>& members,
    bool is_left_recursive,
    bool& hasChanges,
    stdu::vector<stdu::vector<std::string>>& prev_depend
) {
    for (std::size_t i = 0; i < members.size(); ++i) {
        auto &member = *members[i];
        if (member.isNospace()) continue;

        if (member.isName()) {
            const auto& nameInfo = member.getName();
            auto current_n = nameInfo.name;
            if (nameInfo.isTerminal()) continue;

            if (lhs_name == current_n) {
                auto f = first[lhs_name];
                for (auto &e : f) {
                    if (e == stdu::vector<std::string>{"ε"}) continue;
                    if (follow[lhs_name].insert(e).second) hasChanges = true;
                }
                prev_depend.push_back(lhs_name);
                continue;
            }

            if (is_left_recursive) {
                auto prev_size = follow[current_n].size();
                follow[current_n].insert(follow[lhs_name].begin(), follow[lhs_name].end());
                if (prev_size != follow[current_n].size()) hasChanges = true;
                prev_depend.push_back(current_n);
            }

            std::size_t next_idx = i + 1;
            bool reached_end_or_nullable = true;

            while (next_idx < members.size()) {
                if (members[next_idx]->isNospace()) { next_idx++; continue; }

                std::set<stdu::vector<std::string>> next_first;
                collectMemberFirst(*members[next_idx], next_first);

                for (const auto& e : next_first) {
                    if (e == stdu::vector<std::string>{"ε"}) continue;
                    if (follow[current_n].insert(e).second) hasChanges = true;
                }

                if (!isMemberNullable(*members[next_idx])) {
                    reached_end_or_nullable = false;
                    if (members[next_idx]->isName()) {
                        prev_depend.push_back(members[next_idx]->getName().name);
                    }
                    break;
                }
                next_idx++;
            }

            if (reached_end_or_nullable) {
                auto &f_lhs = follow[lhs_name];
                for (auto &sym : f_lhs) {
                    if (follow[current_n].insert(sym).second) hasChanges = true;
                }
            }
        }
        else if (member.isGroup()) {
            // Process the internal vector of structural members recursively
            processFollowForSequence(lhs_name, member.getGroup().values, is_left_recursive, hasChanges, prev_depend);
        }
        else if (member.isOp()) {
            processFollowForSequence(lhs_name, member.getOp().options, is_left_recursive, hasChanges, prev_depend);
        }
    }
}

void AST::Tree::constructFollowSet() {
    if (!follow.empty())
        return;
    createInitialItemSet();
    follow[{"__start"}] = {{"$"}};
    bool hasChanges;
    bool prevDependedChanged;
    stdu::vector<stdu::vector<std::string>> prev_depend;
    stdu::vector<stdu::vector<std::string>> changed;
    Tlog::Branch lb(logger, "AST/constructFollowSet.log");

    do {
        hasChanges = false;
        prevDependedChanged = false;
        prev_depend.clear();
        changed.clear();

        for (const auto &[name, options] : initial_item_set) {
            if (corelib::text::isUpper(name.back()))
                continue;

            for (const auto &rules : options) {
                if (rules.rule_members.empty())
                    continue;

                // Determine left-recursion properties exactly like original layout
                bool is_left_recursive = false;
                auto rules_members_it = rules.rule_members.begin();
                while (rules_members_it != rules.rule_members.end() && (*rules_members_it)->isNospace())
                    rules_members_it++;

                if (rules_members_it != rules.rule_members.end() &&
                (*rules_members_it)->isName() &&
                    name == (*rules_members_it)->getName().name) {
                    is_left_recursive = true;
                }

                logger.dlog("Processing {} -> ", name);

                // Hand over full execution loop down to the group safe scanner
                processFollowForSequence(name, rules.rule_members, is_left_recursive, hasChanges, prev_depend);
            }

            if (hasChanges) {
                changed.push_back(name);
            }
        }

        if (!hasChanges) {
            for (auto &change_symbol : changed) {
                if (std::find(prev_depend.begin(), prev_depend.end(), change_symbol) != prev_depend.end()) {
                    prevDependedChanged = true;
                    break;
                }
            }
        }
        logger.log("");
    } while(hasChanges || prevDependedChanged);
}

void AST::Tree::buildNameToIndexMap() {
    std::size_t index = 0;
    for (const auto &[name, value] : tree_map) {
        name_to_index[name] = index++;
    }
}
auto AST::Tree::getNameToIndexMap() {
    if (name_to_index.empty()) {
        buildNameToIndexMap();
    }
    return name_to_index;
}
auto AST::Tree::getNameToIndexMap() const {
    return name_to_index;
}

void AST::Tree::formatFirstOrFollowSet(std::ostringstream &oss, AST::First &set) {
    for (auto &el : set) {
        oss << corelib::text::join(el.first, "_") << ": " << '{';
        for (auto name : el.second) {
            oss << corelib::text::join(name, "_") << ", ";
        }
        oss << "}\n";
    }
}

void AST::Tree::printFirstSet(const std::string &fileName) {
    // Step 1: Print to std::ostringstream
    std::ostringstream oss;
    formatFirstOrFollowSet(oss, getFirstSet());

    // Step 2: Output the stringstream content to a file
    std::ofstream outFile(fileName);
    if (outFile.is_open()) {
        outFile << oss.str();
        outFile.close();
    } else {
        std::cerr << "Failed to open the file for writing: " << fileName << "\n";
    }
}
void AST::Tree::printFollowSet(const std::string &fileName) {
    // Step 1: Print to std::ostringstream
    std::ostringstream oss;
    formatFirstOrFollowSet(oss, getFollowSet());

    // Step 2: Output the stringstream content to a file
    std::ofstream outFile(fileName);
    if (outFile.is_open()) {
        outFile << oss.str();
        outFile.close();
    } else {
        std::cerr << "Failed to open the file for writing: " << fileName << "\n";
    }
}
auto AST::Tree::getCodeForLexer() -> std::pair<LangAPI::Statements, LangAPI::Variable> {
    return {};
}

auto AST::Tree::generateRandomTokenInputs(std::size_t maxDepth) -> utype::unordered_map<stdu::vector<std::string>, stdu::vector<std::string>> {
    createInitialItemSet();
    AstInputGenerator generator(initial_item_set);
    return generator.generateTokenSamples(maxDepth);
}