export module LangAPI.Analysis;

import std;
import LangAPI;
import LangAPI.Traverse;
import LangAPI.Context;

// Conservative *syntactic* reference analysis, NOT a language-complete name
// resolver. Names are over-approximated: ambiguous references keep all possible
// declarations; unknown references can prevent removal entirely.
export namespace LangAPI::Analysis {

inline constexpr std::size_t no_parent = std::numeric_limits<std::size_t>::max();

struct ReferenceIndex {
    // Counts syntactic Symbol mentions, not resolved references.
    std::unordered_map<std::string, std::size_t> mentions;
    std::unordered_map<std::string, std::size_t> writes;
    bool opaque = false;

    [[nodiscard]] bool mentionsName(std::string_view name) const {
        auto it = mentions.find(std::string(name));
        return it != mentions.end() && it->second != 0;
    }
    [[nodiscard]] std::size_t count(std::string_view name) const {
        auto it = mentions.find(std::string(name));
        return it == mentions.end() ? 0 : it->second;
    }
};

// Return a qualified name only for paths composed entirely of literal names.
// A Symbol may embed a FunctionCall or IspaLibSymbol; those aren't resolvable
// as a static qualified name without more semantic analysis.
[[nodiscard]] inline std::optional<std::string> symbolPath(const Symbol& symbol) {
    std::string path;
    for (const auto& part : symbol.path) {
        const auto* s = std::get_if<std::string>(&part);
        if (!s || s->empty()) return std::nullopt;
        if (!path.empty()) path += "::";
        path += *s;
    }
    if (path.empty()) return std::nullopt;
    return path;
}

[[nodiscard]] inline std::string leafName(std::string_view path) {
    const auto i = path.rfind("::");
    return std::string(path.substr(i == std::string_view::npos ? 0 : i + 2));
}

namespace detail {
    // Walk a subtree without descending into nested declarations. A parent
    // class/namespace must not absorb dependencies owned by its child nodes.
    template<class Root, class Callback>
    void localWalk(const Root& root, Callback&& callback) {
        auto& cb = callback;
        const auto* top_decl = [&]() -> const Declaration* {
            if constexpr (std::same_as<std::remove_cvref_t<Root>, Declaration>)
                return &root;
            else return nullptr;
        }();
        auto visit = [&](auto&& self, const auto& value) -> void {
            using T = std::remove_cvref_t<decltype(value)>;
            if constexpr (std::same_as<T, Declaration>) {
                if (&value != top_decl) return;
            }
            cb(value);
            Traverse::forEachChild(value, [&](const auto& child) {
                self(self, child);
            });
        };
        visit(visit, root);
    }

    template<class Callback>
    void symbolMentions(const Symbol& symbol, Callback& cb, bool& opaque) {
        if (auto path = symbolPath(symbol))
            cb(*path);
        else
            opaque = true;
    }

    template<class Root, class Callback>
    void referenceWalk(const Root& root, Callback&& callback, bool& opaque,
                       ReferenceIndex* index = nullptr) {
        auto& cb = callback;
        localWalk(root, [&](const auto& node) {
            using N = std::remove_cvref_t<decltype(node)>;
            if constexpr (std::same_as<N, Symbol>) {
                symbolMentions(node, cb, opaque);
            } else if constexpr (std::same_as<N, StorageSymbol>) {
                // String-based member access and path components may refer to
                // generated names; keep those candidates conservatively.
                for (const auto& part : node.path) {
                    if (const auto* s = std::get_if<std::string>(&part)) cb(*s);
                    if (const auto* p = std::get_if<Property>(&part)) cb(p->name);
                }
            } else if constexpr (std::same_as<N, DfaLookup>) {
                if (!node.output_name.empty()) cb(node.output_name);
                // Older Traverse deliberately treats DfaLookup as a leaf.
                // Its return_type is a genuine type reference, so inspect it.
                Traverse::forEach<Symbol>(node.return_type, [&](const Symbol& sym) {
                    symbolMentions(sym, cb, opaque);
                });
            } else if constexpr (std::same_as<N, VariableAssignment>) {
                if (index) {
                    if (const auto* sym = std::get_if<Symbol>(&node.name)) {
                        if (auto path = symbolPath(*sym))
                            ++index->writes[leafName(*path)];
                    }
                }
            } else if constexpr (std::same_as<N, Lambda>) {
                // Lambda::parameters aren't enumerated by existing Traverse.
                for (const auto& [type, name] : node.parameters)
                    Traverse::forEach<Symbol>(type, [&](const Symbol& sym) {
                        symbolMentions(sym, cb, opaque);
                    });
            }
        });
    }
}

// More than one candidate may share a name (overloads, nested scopes). This
// index deliberately does not claim that a given name resolves to one node.
template<Traverse::AstNode Root>
[[nodiscard]] ReferenceIndex references(const Root& root) {
    ReferenceIndex index;
    detail::referenceWalk(root, [&](const std::string& path) {
        if (!path.empty()) ++index.mentions[leafName(path)];
    }, index.opaque, &index);
    return index;
}

struct DependencyNode {
    const Declaration* declaration = nullptr; // invalid after structural rewrites
    std::string name;
    std::string qualified_name;
    std::size_t parent = no_parent;
    bool is_namespace = false;
    bool is_class = false;
    bool opaque = false;
    bool effectful_initializer = false;
    std::vector<std::size_t> children;
    std::vector<std::size_t> depends_on;
};

class DependencyGraph {
    std::vector<DependencyNode> nodes_;
    std::unordered_multimap<std::string, std::size_t> by_leaf_;
    std::unordered_multimap<std::string, std::size_t> by_full_;

    static std::string declarationName(const Declaration& decl) {
        return std::visit([](const auto& value) -> std::string {
            using T = std::remove_cvref_t<decltype(value)>;
            if constexpr (std::same_as<T, Class> || std::same_as<T, Interface>) {
                if (auto path = symbolPath(value.name)) return leafName(*path);
                return {};
            } else if constexpr (std::same_as<T, Implementation>) {
                if (auto path = symbolPath(value.target)) return leafName(*path);
                return {};
            } else return value.name;
        }, decl.value);
    }

    void addDeclarations(const Declarations& decls, std::size_t parent,
                         std::string_view prefix) {
        for (const auto& decl : decls) addDeclaration(decl, parent, prefix);
    }

    void addDeclaration(const Declaration& decl, std::size_t parent,
                        std::string_view prefix) {
        const auto name = declarationName(decl);
        const auto full = prefix.empty() ? name :
            (name.empty() ? std::string(prefix) : std::string(prefix) + "::" + name);
        const auto id = nodes_.size();
        DependencyNode node;
        node.declaration = &decl;
        node.name = name;
        node.qualified_name = full;
        node.parent = parent;
        node.is_namespace = std::holds_alternative<Namespace>(decl.value);
        node.is_class = std::holds_alternative<Class>(decl.value);
        nodes_.push_back(std::move(node));
        if (parent != no_parent) nodes_[parent].children.push_back(id);
        if (!name.empty()) by_leaf_.emplace(name, id);
        if (!full.empty()) by_full_.emplace(full, id);
        if (const auto* ns = std::get_if<Namespace>(&decl.value)) {
            addDeclarations(ns->declarations, id, full);
        } else if (const auto* cls = std::get_if<Class>(&decl.value)) {
            for (const auto& entry : cls->data)
                if (entry.first) addDeclaration(*entry.first, id, full);
        }
    }

public:
    explicit DependencyGraph(const Declarations& declarations) {
        addDeclarations(declarations, no_parent, {});
        for (std::size_t i = 0; i < nodes_.size(); ++i) {
            auto& node = nodes_[i];
            std::unordered_set<std::size_t> candidates;
            detail::referenceWalk(*node.declaration, [&](const std::string& path) {
                if (path.empty()) return;
                auto full = by_full_.equal_range(path);
                bool exact = full.first != full.second;
                // Even if a qualified path matches, maintain a conservative
                // fallback for ambiguous/dynamically scoped names.
                auto leaf = by_leaf_.equal_range(leafName(path));
                for (auto it = leaf.first; it != leaf.second; ++it)
                    candidates.insert(it->second);
                if (exact) for (auto it = full.first; it != full.second; ++it)
                    candidates.insert(it->second);
            }, node.opaque);
            node.depends_on.assign(candidates.begin(), candidates.end());
            std::sort(node.depends_on.begin(), node.depends_on.end());
        }
    }

    [[nodiscard]] const std::vector<DependencyNode>& nodes() const noexcept {
        return nodes_;
    }

    [[nodiscard]] std::unordered_set<std::size_t>
    reachable(const std::vector<std::string>& roots,
              bool preserve_class_members = true) const {
        std::unordered_set<std::size_t> live;
        std::vector<std::size_t> work;
        auto enqueue = [&](std::size_t id) {
            if (live.insert(id).second) work.push_back(id);
        };
        for (const auto& root : roots) {
            const auto full = by_full_.equal_range(root);
            for (auto it = full.first; it != full.second; ++it) enqueue(it->second);
            const auto leaf = by_leaf_.equal_range(root);
            for (auto it = leaf.first; it != leaf.second; ++it) enqueue(it->second);
        }
        while (!work.empty()) {
            const auto id = work.back();
            work.pop_back();
            const auto& node = nodes_[id];
            for (auto dep : node.depends_on) enqueue(dep);
            // Retain the containing class/namespace for any used member.
            if (node.parent != no_parent) enqueue(node.parent);
            if (preserve_class_members && node.is_class)
                for (auto child : node.children) enqueue(child);
        }
        return live;
    }

    [[nodiscard]] bool matchesRoot(std::string_view root) const {
        return by_full_.contains(std::string(root)) ||
               by_leaf_.contains(std::string(root));
    }
};

} // namespace LangAPI::Analysis
