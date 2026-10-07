module;
#include <boost/container/flat_map.hpp>
export module LangRepr.ConstructTypes;
import LangRepr.Holder;
import LLIR.IR;
import LLIR.API;
import LexerBuilder;
import LangAPI;
import hash;
import dstd;
import std;
using Name = stdu::vector<std::string>;
class Node {
public:
    using ChildT = boost::container::flat_map<std::string, std::unique_ptr<Node>>;
private:
    template<typename Func>
    auto visit(Func func, Name &fullname) const -> void {
        for (const auto &[name, child] : children) {
            fullname.push_back(name);
            if (func(fullname, *child)) return;
            child->visit(func, fullname);
            fullname.pop_back();
        }
    }
    template<typename Func>
    auto visit(Func func, Name &fullname) -> void {
        for (auto &[name, child] : children) {
            fullname.push_back(name);
            if (func(fullname, *child)) return;
            child->visit(func, fullname);
            fullname.pop_back();
        }
    }
public:
    ChildT children;
    LLIR::DataBlock data;
    auto find(const Name &name) const -> const Node* {
        const Node* current = this;
        for (const auto& part : name) {
            if (!current->children.contains(part)) return nullptr;
            auto& child = current->children.at(part);
            current = child.get();
        }
        return current;
    }
    auto find_or_emplace(const Name &name) -> Node* {
        Node* current = this;
        for (const auto& part : name) {
            auto& child = current->children[part];
            if (!child) child = std::make_unique<Node>();
            current = child.get();
        }
        return current;
    }
    template<typename Func>
    auto visit(Func func) const -> void {
        Name fullname;
        for (const auto &[name, child] : children) {
            fullname.push_back(name);
            if (func(fullname, *child)) return;
            child->visit(func, fullname);
            fullname.pop_back();
        }
    }
    template<typename Func>
    auto visit(Func func) -> void {
        Name fullname;
        for (auto &[name, child] : children) {
            fullname.push_back(name);
            if (func(fullname, *child)) return;
            child->visit(func, fullname);
            fullname.pop_back();
        }
    }
};

class SortedNode {
public:
    using ChildT = boost::container::flat_map<std::string, std::unique_ptr<SortedNode>>;
private:
    template<typename Func>
    auto visit(Func func, Name &fullname) const -> void {
        for (const auto &[name, child] : children) {
            fullname.push_back(name);
            if (func(fullname, *child)) return;
            child->visit(func, fullname);
            fullname.pop_back();
        }
    }
    template<typename Func>
    auto visit(Func func, Name &fullname) -> void {
        for (auto &[name, child] : children) {
            fullname.push_back(name);
            if (func(fullname, *child)) return;
            child->visit(func, fullname);
            fullname.pop_back();
        }
    }
public:
    ChildT children;
    LLIR::DataBlock data;
};
auto collectReferencedNames(const LangAPI::Type &type) -> std::pair<utype::unordered_map<Name, std::size_t>, utype::unordered_set<Name>>;
export namespace LangRepr {
    class ConstructTypes {
        Holder &holder;
        LexerBuilder &lexer_builder;
        LLIR::IR &ir;
        const std::string &namespace_name;
    public:
        auto constructTokensAndRulesEnum() -> void;
        auto constructTokensAndRulesEnumToString() -> void;
        auto constructTypesNamespace() -> void;
        ConstructTypes(Holder &holder, LexerBuilder &lexer_builder, LLIR::IR &ir, const std::string &namespace_name) : holder(holder), lexer_builder(lexer_builder), ir(ir), namespace_name(namespace_name) {};

        ~ConstructTypes() {}
    };
}