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

bool AST::Tree::isMemberNullable(const AST::RuleMember& member) const {
    // '?' (zero-or-one) and '*' (zero-or-more) make the occurrence itself
    // skippable regardless of what it contains. '+' does NOT — it still
    // requires at least one occurrence.
    if (member.quantifier == '?' || member.quantifier == '*')
        return true;

    if (member.isNospace())
        return true;

    if (member.isName())
        return nullable.count(member.getName().name) > 0;

    if (member.isGroup()) {
        // (a b c): nullable only if EVERY member is nullable
        for (const auto& sub : member.getGroup().values) {
            if (!isMemberNullable(*sub))
                return false;
        }
        return true;
    }

    if (member.isOp()) {
        // a | b | c: nullable if ANY option is nullable
        for (const auto& opt : member.getOp().options) {
            if (isMemberNullable(*opt))
                return true;
        }
        return false;
    }

    // String / Csequence / Hex / Bin / Any / Escaped / Cll: never nullable on their own
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
// Processes one RuleMember as part of nonterminal's production: adds its
// FIRST contribution to first[nonterminal] and returns whether the
// occurrence itself is nullable (quantifier included). Only the caller
// that owns a complete rule (constructFirstSet) is allowed to insert ε
// into first[nonterminal] — this never does it directly, so a nullable
// sub-expression in the middle of a longer rule can't wrongly mark the
// whole rule nullable.
bool AST::Tree::processMemberFirst(
    const AST::RuleMember& member,
    const stdu::vector<std::string>& nonterminal,
    bool& changed
) {
    if (member.isNospace())
        return true;

    bool force_nullable = (member.quantifier == '?' || member.quantifier == '*');

    if (member.isName()) {
        const auto& rule_name = member.getName();

        if (rule_name.name == nonterminal)
            return force_nullable || nullable.count(rule_name.name) > 0;

        auto& currentFirst = first[nonterminal];

        if (rule_name.isNonterminal()) {
            const auto& otherFirst = first[rule_name.name];
            for (const auto& el : otherFirst) {
                if (corelib::text::isUpper(el.back()) && el != stdu::vector<std::string>{"ε"})
                    continue;
                if (currentFirst.insert(el).second)
                    changed = true;
            }
            return force_nullable || nullable.count(rule_name.name) > 0;
        } else {
            if (currentFirst.insert(rule_name.name).second)
                changed = true;
            return force_nullable; // a terminal is never nullable by itself, '?'/'*' makes the occurrence skippable
        }
    }

    if (member.isGroup()) {
        bool all_nullable = true;
        for (const auto& sub : member.getGroup().values) {
            if (!processMemberFirst(*sub, nonterminal, changed)) {
                all_nullable = false;
                break;
            }
        }
        return force_nullable || all_nullable;
    }

    if (member.isOp()) {
        bool any_nullable = false;
        for (const auto& opt : member.getOp().options) {
            if (processMemberFirst(*opt, nonterminal, changed))
                any_nullable = true;
        }
        return force_nullable || any_nullable;
    }

    if (member.isString() || member.isCsequence() || member.isHex() || member.isBin())
        return force_nullable;

    throw Error("Unhandled RuleMember variant, rule {} but index {}", nonterminal, member.value.index());
}

void AST::Tree::constructFirstSet(const stdu::vector<AST::Rule>& options, const stdu::vector<std::string>& nonterminal, bool& changed) {
    logger.increaseIndentLevel();
    for (const auto& option : options) {
        bool nullable_prefix = true;
        for (const auto& m : option.rule_members) {
            if (!processMemberFirst(*m, nonterminal, changed)) {
                nullable_prefix = false;
                break;
            }
        }
        if (nullable_prefix) {
            if (first[nonterminal].insert({"ε"}).second)
                changed = true;
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
    const stdu::vector<std::shared_ptr<AST::RuleMember>>& trailing,
    bool is_left_recursive,
    bool& hasChanges,
    stdu::vector<stdu::vector<std::string>>& prev_depend
) {
    for (std::size_t i = 0; i < members.size(); ++i) {
        auto &member = *members[i];
        if (member.isNospace()) continue;

        bool can_repeat = (member.quantifier == '*' || member.quantifier == '+');

        // Everything after this member, plus whatever follows the whole
        // sequence we're inside of (members[i+1..] ++ trailing).
        stdu::vector<std::shared_ptr<AST::RuleMember>> rest(members.begin() + i + 1, members.end());
        rest.insert(rest.end(), trailing.begin(), trailing.end());

        if (member.isGroup()) {
            // (a b c): descend into the sequence with the same trailing continuation
            processFollowForSequence(lhs_name, member.getGroup().values, rest, is_left_recursive, hasChanges, prev_depend);
            continue;
        }

        if (member.isOp()) {
            // a | b | c: EACH alternative gets the same trailing continuation —
            // they must not see each other as "what comes next".
            for (const auto& opt : member.getOp().options) {
                stdu::vector<std::shared_ptr<AST::RuleMember>> one{opt};
                processFollowForSequence(lhs_name, one, rest, is_left_recursive, hasChanges, prev_depend);
            }
            continue;
        }

        if (!member.isName())
            continue; // terminal-like leaf: nothing more to propagate

        const auto& nameInfo = member.getName();
        auto current_n = nameInfo.name;
        if (nameInfo.isTerminal()) continue;

        std::size_t j = 0;
        bool reached_end_or_nullable = true;

        while (j < rest.size()) {
            if (rest[j]->isNospace()) { j++; continue; }

            std::set<stdu::vector<std::string>> next_first;
            collectMemberFirst(*rest[j], next_first);

            for (const auto& e : next_first) {
                if (e == stdu::vector<std::string>{"ε"}) continue;
                if (follow[current_n].insert(e).second) hasChanges = true;
            }

            if (!isMemberNullable(*rest[j])) {
                reached_end_or_nullable = false;
                if (rest[j]->isName())
                    prev_depend.push_back(rest[j]->getName().name);
                break;
            }
            j++;
        }

        if (reached_end_or_nullable) {
            for (auto &sym : follow[lhs_name]) {
                if (follow[current_n].insert(sym).second) hasChanges = true;
            }
        }

        if (can_repeat) {
            // a* / a+ : another occurrence of `a` can immediately follow
            // this one, so FIRST(a) is also part of FOLLOW(a).
            for (auto &e : first[current_n]) {
                if (e == stdu::vector<std::string>{"ε"}) continue;
                if (follow[current_n].insert(e).second) hasChanges = true;
            }
        }

        if (is_left_recursive) {
            auto prev_size = follow[current_n].size();
            follow[current_n].insert(follow[lhs_name].begin(), follow[lhs_name].end());
            if (prev_size != follow[current_n].size()) hasChanges = true;
        }

        prev_depend.push_back(current_n);
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

                bool is_left_recursive = false;
                auto it = rules.rule_members.begin();
                while (it != rules.rule_members.end() && (*it)->isNospace())
                    it++;

                if (it != rules.rule_members.end() &&
                    (*it)->isName() &&
                    name == (*it)->getName().name) {
                    is_left_recursive = true;
                }

                logger.dlog("Processing {} -> ", name);
                processFollowForSequence(name, rules.rule_members, {}, is_left_recursive, hasChanges, prev_depend);
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

// AST::Tree FIRST_k / FOLLOW_k implementation (C++17).
// Add the declarations shown in AST_Tree_LLk_INTEGRATION.md to AST::Tree.
// Include this file in the same translation unit/module as AST::Tree implementation.
// Each LookaheadSeq is a sequence of token identifiers; each token identifier
// is stdu::vector<std::string> (the existing AST name representation).

namespace {
using Token = stdu::vector<std::string>;
using Seq = std::vector<Token>;
using Set = std::set<Seq>;

// Concatenate and truncate to k tokens. Empty Seq is epsilon.
Set llk_concat(const Set& left, const Set& right, std::size_t k) {
    Set out;
    for (const auto& a : left) {
        for (const auto& b : right) {
            Seq joined = a;
            for (const auto& t : b) {
                if (joined.size() == k) break;
                joined.push_back(t);
            }
            out.insert(std::move(joined));
        }
    }
    return out;
}

// Zero or more repetitions, truncated at k. Handles nullable operands.
Set llk_star(const Set& operand, std::size_t k) {
    Set result{Seq{}};
    Set frontier{Seq{}};
    while (true) {
        Set added = llk_concat(frontier, operand, k);
        Set next;
        for (const auto& s : added) {
            if (result.insert(s).second) next.insert(s);
        }
        if (next.empty()) break;
        frontier = std::move(next);
    }
    return result;
}

bool llk_merge(Set& into, const Set& from) {
    bool changed = false;
    for (const auto& seq : from)
        changed |= into.insert(seq).second;
    return changed;
}
} // namespace

// Base member FIRST_k, ignoring the member's own quantifier.
auto AST::Tree::memberBaseFirstK(const AST::RuleMember& member,
    std::size_t k, const LLkTable& table) const -> LLkSet {
    if (member.isNospace()) return {LLkSeq{}};
    if (member.isName()) {
        const auto& name = member.getName();
        if (name.isTerminal()) return {LLkSeq{name.name}};
        const auto it = table.find(name.name);
        return it == table.end() ? LLkSet{} : it->second;
    }
    if (member.isGroup()) {
        LLkSet result{LLkSeq{}};
        for (const auto& sub : member.getGroup().values)
            result = llk_concat(result, memberFirstK(*sub, k, table), k);
        return result;
    }
    if (member.isOp()) {
        LLkSet result;
        for (const auto& option : member.getOp().options)
            llk_merge(result, memberFirstK(*option, k, table));
        return result;
    }
    // These members do not consume a named token in the existing grammar
    // analysis. Never silently interpret them as epsilon: that would produce
    // false LL(k) decisions. Add tokenization/terminal expansion here if needed.
    throw Error("FIRST_k: unsupported consuming RuleMember kind (index {})",
                member.value.index());
}

auto AST::Tree::memberFirstK(const AST::RuleMember& member,
    std::size_t k, const LLkTable& table) const -> LLkSet {
    LLkSet base = memberBaseFirstK(member, k, table);
    switch (member.quantifier) {
        case '?': {
            base.insert(LLkSeq{});
            return base;
        }
        case '*': return llk_star(base, k);
        case '+': return llk_concat(base, llk_star(base, k), k);
        default: return base;
    }
}

auto AST::Tree::sequenceFirstK(
    const stdu::vector<std::shared_ptr<AST::RuleMember>>& members,
    std::size_t k, const LLkTable& table) const -> LLkSet {
    LLkSet result{LLkSeq{}};
    for (const auto& member : members)
        result = llk_concat(result, memberFirstK(*member, k, table), k);
    return result;
}

// Calculate a complete fixed point at one depth. The previous depth is
// intentionally retained in first_k_cache: FIRST_{k-1} alone is not a
// sufficient seed to derive FIRST_k, so a new depth must reach its own fixed
// point. Results for old depths are still immediately reusable.
void AST::Tree::constructFirstSet(std::size_t k) {
    if (!k) throw Error("FIRST_k requires k >= 1");
    createInitialItemSet();
    computeNullableSet();
    for (std::size_t depth = 1; depth <= k; ++depth) {
        if (first_k_cache.count(depth)) continue;
        LLkTable working;
        for (const auto& entry : initial_item_set) {
            if (corelib::text::isUpper(entry.first.back()))
                continue;
            working[entry.first];
        }
        bool changed;
        do {
            changed = false;
            for (const auto& entry : initial_item_set) {
                const auto& name = entry.first;
                if (corelib::text::isUpper(name.back()))
                    continue;
                for (const auto& production : entry.second)
                    changed |= llk_merge(working[name],
                        sequenceFirstK(production.rule_members, depth, working));
            }
        } while (changed);
        first_k_cache.emplace(depth, std::move(working));
    }
}

// Visit nonterminal occurrences in a sequence. 'after' is FIRST_k of what
// can occur after the entire sequence (including enclosing context).
void AST::Tree::propagateFollowSequenceK(
    const stdu::vector<std::shared_ptr<AST::RuleMember>>& members,
    const LLkSet& after, std::size_t k, const LLkTable& firstTable,
    LLkTable& followTable, bool& changed) const {
    LLkSet suffix = after;
    for (auto it = members.rbegin(); it != members.rend(); ++it) {
        const auto& member = **it;
        const auto base = memberBaseFirstK(member, k, firstTable);
        LLkSet innerAfter = suffix;
        if (member.quantifier == '*' || member.quantifier == '+')
            innerAfter = llk_concat(llk_star(base, k), suffix, k);

        if (member.isName()) {
            const auto& name = member.getName();
            if (name.isNonterminal())
                changed |= llk_merge(followTable[name.name], innerAfter);
        } else if (member.isGroup()) {
            propagateFollowSequenceK(member.getGroup().values, innerAfter,
                                     k, firstTable, followTable, changed);
        } else if (member.isOp()) {
            for (const auto& option : member.getOp().options) {
                stdu::vector<std::shared_ptr<AST::RuleMember>> single{option};
                propagateFollowSequenceK(single, innerAfter,
                                         k, firstTable, followTable, changed);
            }
        }
        suffix = llk_concat(memberFirstK(member, k, firstTable), suffix, k);
    }
}

void AST::Tree::constructFollowSet(std::size_t k) {
    if (!k) throw Error("FOLLOW_k requires k >= 1");
    constructFirstSet(k);
    for (std::size_t depth = 1; depth <= k; ++depth) {
        if (follow_k_cache.count(depth)) continue;
        const auto& firstTable = first_k_cache.at(depth);
        LLkTable working;
        for (const auto& entry : initial_item_set) {
            if (corelib::text::isUpper(entry.first.back()))
                continue;
            working[entry.first];
        }
        // '$' is an end-of-input marker. It is a token identifier, not epsilon.
        working[Token{"__start"}].insert(LLkSeq{Token{"$"}});
        bool changed;
        do {
            changed = false;
            for (const auto& entry : initial_item_set) {
                const auto& lhs = entry.first;
                if (corelib::text::isUpper(lhs.back()))
                    continue;
                for (const auto& production : entry.second)
                    propagateFollowSequenceK(production.rule_members,
                        working[lhs], depth, firstTable, working, changed);
            }
        } while (changed);
        follow_k_cache.emplace(depth, std::move(working));
    }
}

auto AST::Tree::getFirstSet(std::size_t k) -> const LLkTable& {
    constructFirstSet(k);
    return first_k_cache.at(k);
}

auto AST::Tree::getFollowSet(std::size_t k) -> const LLkTable& {
    constructFollowSet(k);
    return follow_k_cache.at(k);
}

// FIRST_k(alternative followed by the current rule's continuation).
// This replaces OpBuilder::firstK + withFollow for alternatives that are
// represented as one RuleMember each.
auto AST::Tree::getAlternativeLookahead(
    const AST::RuleMember& alt,
    const stdu::vector<std::string>& ruleName, std::size_t k) -> LLkSet {
    const auto& firstTable = getFirstSet(k);
    const auto& followTable = getFollowSet(k);
    const auto firstAlt = memberFirstK(alt, k, firstTable);
    const auto it = followTable.find(ruleName);
    if (it == followTable.end() || it->second.empty()) return firstAlt;
    return llk_concat(firstAlt, it->second, k);
}

