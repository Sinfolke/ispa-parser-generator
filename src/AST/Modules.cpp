module AST.Builder;
import std;
import Modules;

namespace AST {
namespace {
using Name = stdu::vector<std::string>;
using Member = std::shared_ptr<RuleMember>;
Name canonical(const Modules::Path& path) {
    Name result{"modules"};
    result.insert(result.end(), path.begin(), path.end());
    return result;
}
bool begins(const Name& name, const Name& prefix) {
    return name.size() >= prefix.size() && std::equal(prefix.begin(), prefix.end(), name.begin());
}
Member copyMember(const Member& member) {
    auto copy = std::make_shared<RuleMember>(*member);
    if (copy->isGroup()) {
        for (auto& child : copy->getGroup().values) child = copyMember(child);
    } else if (copy->isOp()) {
        for (auto& child : copy->getOp().options) child = copyMember(child);
    }
    return copy;
}
Rule copyRule(const Rule& rule) {
    auto copy = rule;
    for (auto& member : copy.rule_members) member = copyMember(member);
    copy.normalization_source.reset();
    return copy;
}
template<class F> void names(RuleMember& member, const F& f) {
    for (auto& feature : member.features)
        if (auto* stream = std::get_if<StreamMatch>(&feature)) f(stream->rule);
    if (member.isName()) {
        auto& reference = member.getName();
        if (reference.templateParameter.empty()) f(reference.name);
        for (std::size_t i = 0; i < reference.templateArguments.size(); ++i)
            if (i >= reference.templateArgumentIsParameter.size() || !reference.templateArgumentIsParameter[i])
                f(reference.templateArguments[i]);
    }
    else if (member.isGroup()) for (auto& child : member.getGroup().values) names(*child, f);
    else if (member.isOp()) for (auto& child : member.getOp().options) names(*child, f);
}
template<class F> void names(Rule& rule, const F& f) {
    for (auto& member : rule.rule_members) names(*member, f);
}
bool optional(const RuleMember& member) { return member.quantifier == '?' || member.quantifier == '*'; }
bool possible(const RuleMember& member, const std::set<Name>& removed, bool quantifier = true) {
    if (quantifier && optional(member)) return true;
    if (member.isName()) return !removed.contains(member.getName().name);
    if (member.isGroup()) {
        for (auto& child : member.getGroup().values) if (!possible(*child, removed)) return false;
    } else if (member.isOp()) {
        for (auto& child : member.getOp().options) if (possible(*child, removed)) return true;
        return false;
    }
    return true;
}
bool productive(const RuleMember& member, const std::set<Name>& ready) {
    if (optional(member)) return true;
    if (member.isName()) return !member.getName().templateParameter.empty() || ready.contains(member.getName().name);
    if (member.isGroup()) {
        for (const auto& child : member.getGroup().values) if (!productive(*child, ready)) return false;
    } else if (member.isOp()) {
        for (const auto& child : member.getOp().options) if (productive(*child, ready)) return true;
        return false;
    }
    return true;
}
std::vector<bool> captureMask(const RuleMember& member, const std::set<Name>& removed) {
    if (!member.prefix.empty()) return {possible(member, removed, false)};
    std::vector<bool> mask;
    if (member.isGroup()) {
        for (const auto& child : member.getGroup().values) {
            auto part = captureMask(*child, removed);
            mask.insert(mask.end(), part.begin(), part.end());
        }
    } else if (member.isOp()) {
        std::optional<std::vector<bool>> choice;
        for (const auto& child : member.getOp().options) {
            if (!possible(*child, removed)) continue;
            auto part = captureMask(*child, removed);
            if (choice && *choice != part)
                throw std::runtime_error("Detach makes alternatives disagree on captured fields");
            choice = std::move(part);
        }
        if (choice) mask = std::move(*choice);
        else if (!member.getOp().options.empty()) mask = captureMask(*member.getOp().options.front(), {});
    }
    if (!possible(member, removed, false)) std::fill(mask.begin(), mask.end(), false);
    return mask;
}
// A null result means epsilon (an unavailable optional reference). A required
// unavailable member has already caused its enclosing rule to be removed.
Member prune(const Member& member, const std::set<Name>& removed) {
    if (!possible(*member, removed, false)) {
        return nullptr;
    }
    auto result = copyMember(member);
    if (result->isGroup()) {
        auto& children = result->getGroup().values;
        for (auto it = children.begin(); it != children.end();) {
            auto next = prune(*it, removed);
            if (next) { *it = std::move(next); ++it; }
            else it = children.erase(it);
        }
        if (children.empty()) result->quantifier = '\0';
    } else if (result->isOp()) {
        auto& options = result->getOp().options;
        for (auto it = options.begin(); it != options.end();) {
            if (!possible(**it, removed)) { it = options.erase(it); continue; }
            auto next = prune(*it, removed);
            if (!next) {
                next = std::make_shared<RuleMember>();
                next->value = RuleMemberGroup{};
            }
            *it++ = std::move(next);
        }
    }
    return result;
}
void pruneRule(Rule& rule, const std::set<Name>& removed) {
    std::vector<bool> captures;
    for (const auto& member : rule.rule_members) {
        auto part = captureMask(*member, removed);
        captures.insert(captures.end(), part.begin(), part.end());
    }
    if (rule.data_block.isTemplatedDataBlock()) {
        auto& fields = rule.data_block.getTemplatedDataBlock().names;
        if (fields.size() != captures.size()) throw std::runtime_error("Invalid capture metadata before detach");
        stdu::vector<std::string> remaining;
        for (std::size_t i = 0; i < captures.size(); ++i) if (captures[i]) remaining.push_back(fields[i]);
        fields = std::move(remaining);
        if (fields.empty()) rule.data_block = {};
    } else if (rule.data_block.isRegularDataBlock() && !rule.data_block.isWithKeys() &&
               captures.size() == 1 && !captures[0]) rule.data_block = {};

    for (auto it = rule.rule_members.begin(); it != rule.rule_members.end();) {
        auto next = prune(*it, removed);
        if (next) { *it = std::move(next); ++it; }
        else it = rule.rule_members.erase(it);
    }
    // Preserve epsilon productions through the initial removeEmptyRule pass.
    if (rule.rule_members.empty()) {
        auto epsilon = std::make_shared<RuleMember>();
        epsilon->value = RuleMemberGroup{};
        rule.rule_members.push_back(epsilon);
    }
}
}

void Builder::specializeModules() {
    const auto& project = *module_project;
    std::map<Modules::Path, const Modules::Source*> sources;
    std::map<Modules::Path, std::map<Name, Name>> visible;
    std::map<Name, Name> origins;
    std::set<Name> removed_globally;
    std::set<Modules::Path> done;
    for (const auto& source : project.sources) sources.emplace(source.name, &source);
    for (const auto& [name, rule] : tree_map) origins.emplace(name, name);
    auto process = [&](auto& self, const Modules::Source& source) -> void {
        if (!done.insert(source.name).second) return;
        for (const auto& imp : source.imports) {
            if (imp.python_import || !imp.external.empty())
                continue;

            const auto found = sources.find(imp.source);
            if (found == sources.end())
                throw std::runtime_error(
                    "ISPA module not found during specialization: " +
                    Modules::spelling(imp.source)
                );

            self(self, *found->second);
        }
        std::set<Name> own;
        for (const auto& [relative, position] : source.declarations) {
            auto name = source.name;
            name.insert(name.end(), relative.begin(), relative.end());
            own.insert(canonical(name));
        }
        auto& exports = visible[source.name];
        for (const auto& [local, target] : source.resolved_symbols) {
            Name spelling(local.begin(), local.end());
            exports.emplace(spelling, canonical(target));
        }
        if (source.detached_symbols.empty() && removed_globally.empty()) return;
        // Capture the imported dependency closure, including private helpers.
        // Those helpers retain the permissions of their defining module.
        std::set<Name> closure;
        auto collect = [&](auto& visit, const Name& name) -> void {
            if (!closure.insert(name).second || !tree_map.contains(name)) return;
            names(tree_map.at(name), [&](const Name& ref) { visit(visit, ref); });
        };
        for (const auto& name : own) collect(collect, name);
        for (const auto& [local, name] : exports) collect(collect, name);
        std::set<Name> removed = removed_globally;
        for (const auto& name : closure) {
            const auto original = origins.contains(name) ? origins.at(name) : name;
            for (const auto& detached : source.detached_symbols) {
                if (begins(original, canonical(detached))) { removed.insert(name); break; }
            }
        }
        // Propagate unavailable required dependencies to a fixed point. An
        // alternative survives if any branch survives; ? and * remain epsilon.
        bool changed;
        do {
            changed = false;
            for (const auto& name : closure) {
                if (removed.contains(name) || !tree_map.contains(name)) continue;
                for (auto& member : tree_map.at(name).rule_members) {
                    if (!possible(*member, removed)) {
                        removed.insert(name);
                        changed = true;
                        break;
                    }
                }
            }
        } while (changed);
        // Required recursive cycles can lose their last finite derivation when
        // a base alternative is detached. Compute productivity from literals
        // and epsilon upward rather than considering a cycle self-sustaining.
        std::set<Name> ready;
        do {
            changed = false;
            for (const auto& name : closure) {
                if (removed.contains(name) || ready.contains(name) || !tree_map.contains(name)) continue;
                bool finite = true;
                for (const auto& member : tree_map.at(name).rule_members)
                    if (!productive(*member, ready)) { finite = false; break; }
                if (finite) { ready.insert(name); changed = true; }
            }
        } while (changed);
        for (const auto& name : closure) if (!ready.contains(name)) removed.insert(name);
        // Only definitions depending on a removed symbol need a private copy.
        // Sharing the unaffected definitions also avoids duplicate lexer tokens.
        auto affected = removed;
        do {
            changed = false;
            for (const auto& name : closure) {
                if (affected.contains(name) || !tree_map.contains(name)) continue;
                bool dependent = false;
                names(tree_map.at(name), [&](const Name& ref) { dependent |= affected.contains(ref); });
                if (dependent) { affected.insert(name); changed = true; }
            }
        } while (changed);
        std::map<Name, Name> remap;
        Name copy_prefix = canonical(source.name);
        copy_prefix.push_back("ispa_imports");
        for (std::size_t suffix = 0;; ++suffix) {
            bool collision = false;
            for (const auto& old : closure) if (affected.contains(old) && !own.contains(old) && !removed.contains(old)) {
                Name copy = copy_prefix;
                copy.insert(copy.end(), old.begin(), old.end());
                if (tree_map.contains(copy)) { collision = true; break; }
            }
            if (!collision) break;
            copy_prefix.back() = "ispa_imports_" + std::to_string(suffix + 1);
        }
        for (const auto& old : closure) if (affected.contains(old) && !own.contains(old) && !removed.contains(old)) {
            Name copy = copy_prefix;
            copy.insert(copy.end(), old.begin(), old.end());
            remap.emplace(old, copy);
        }
        std::vector<std::pair<Name, AST::Rule>> updates;
        for (const auto& old : closure) {
            if (!affected.contains(old) || removed.contains(old) || !tree_map.contains(old)) continue;
            auto copy = copyRule(tree_map.at(old));
            pruneRule(copy, removed);
            names(copy, [&](Name& ref) { if (remap.contains(ref)) ref = remap.at(ref); });
            inferDataTypes(copy);
            const auto name = remap.contains(old) ? remap.at(old) : old;
            origins[name] = origins.at(old);
            updates.emplace_back(name, std::move(copy));
        }
        for (auto& [name, rule] : updates) tree_map[name] = std::move(rule);
        for (const auto& name : own) if (removed.contains(name)) {
            tree_map.erase(name);
            removed_globally.insert(name);
        }
        for (auto it = exports.begin(); it != exports.end();) {
            if (removed.contains(it->second) || !tree_map.contains(it->second)) it = exports.erase(it);
            else { if (remap.contains(it->second)) it->second = remap.at(it->second); ++it; }
        }
    };
    for (const auto& source : project.sources) process(process, source);
    // A back-edge can lead to a module visited before a peer removes one of
    // its declarations. Close those removals over the completed graph so the
    // result is independent of which module was visited first.
    if (!removed_globally.empty()) {
        bool changed;
        do {
            changed = false;
            for (const auto& [name, rule] : tree_map) {
                if (removed_globally.contains(name)) continue;
                for (const auto& member : rule.rule_members) if (!possible(*member, removed_globally)) {
                    removed_globally.insert(name);
                    changed = true;
                    break;
                }
            }
        } while (changed);
        auto affected = removed_globally;
        do {
            changed = false;
            for (auto& [name, rule] : tree_map) {
                if (affected.contains(name)) continue;
                bool dependent = false;
                names(rule, [&](const Name& ref) { dependent |= affected.contains(ref); });
                if (dependent) { affected.insert(name); changed = true; }
            }
        } while (changed);
        std::set<Name> ready;
        do {
            changed = false;
            for (const auto& [name, rule] : tree_map) {
                if (removed_globally.contains(name) || ready.contains(name)) continue;
                bool finite = true;
                for (const auto& member : rule.rule_members)
                    if (!productive(*member, ready)) { finite = false; break; }
                if (finite) { ready.insert(name); changed = true; }
            }
        } while (changed);
        for (const auto& name : affected) if (!ready.contains(name)) removed_globally.insert(name);
        for (auto it = tree_map.begin(); it != tree_map.end();) {
            if (removed_globally.contains(it->first)) it = tree_map.erase(it);
            else {
                if (affected.contains(it->first)) {
                    pruneRule(it->second, removed_globally);
                    inferDataTypes(it->second);
                }
                ++it;
            }
        }
    }
    if (project.root) {
        const auto& root = *sources.at(*project.root);
        std::set<Name> keep;
        auto collect = [&](auto& self, const Name& name) -> void {
            if (!tree_map.contains(name) || !keep.insert(name).second) return;
            names(tree_map.at(name), [&](const Name& ref) { self(self, ref); });
        };
        for (const auto& [relative, position] : root.declarations) {
            auto path = root.name;
            path.insert(path.end(), relative.begin(), relative.end());
            collect(collect, canonical(path));
        }
        for (const auto& [local, target] : visible.at(root.name)) collect(collect, target);
        if (project.entry) {
            collect(collect, canonical(*project.entry));
        }
        for (auto it = tree_map.begin(); it != tree_map.end();) {
            if (!keep.contains(it->first)) it = tree_map.erase(it);
            else ++it;
        }

    }
    if (project.entry) {
        const auto entry = canonical(*project.entry);
        if (!tree_map.contains(entry)) throw std::runtime_error("Module entry was removed by detach: " + Modules::spelling(*project.entry));
        AST::Rule wrapper;
        auto member = std::make_shared<RuleMember>();
        member->prefix.present = true;
        member->value = RuleMemberName{.name = entry};
        wrapper.rule_members.push_back(member);
        wrapper.data_block.value = RegularDataBlock{At{}};
        inferDataTypes(wrapper);
        tree_map[Name{"main"}] = std::move(wrapper);
    }
}
}
