module LangRepr.Construct;

import Dump;
import UAST.PyAST;

import ParserIR.API;
import NFA.TNFA.API;
import DFA.API;

import constants;
import corelib;
import cpuf.printf;
import cpuf.op;
import logging;
import hash;
import DFA.Base;
import DFA;

import dstd;
import std;

import LangRepr.ConstructTypes;
import LangRepr.ConstructLexer;
import LangRepr.ConstructParser;

namespace LangRepr {
    namespace {
        // Moves the wanted classes (qualified names such as python.ast.a_ast) out of `source` into `out`,
        // keeping their namespace nesting.
        void hoistClasses(LangAPI::Declarations& source, const std::set<std::string>& wanted,
                          LangAPI::Declarations& out, const std::string& prefix) {
            for (auto it = source.begin(); it != source.end();) {
                if (it->isNamespace()) {
                    auto& ns = it->getNamespace();
                    LangAPI::Declarations inner;
                    hoistClasses(ns.declarations, wanted, inner, prefix + ns.name + ".");
                    if (!inner.empty()) {
                        LangAPI::Namespace copy;
                        copy.name = ns.name;
                        copy.declarations = std::move(inner);
                        out.push_back(LangAPI::Declaration{std::move(copy)});
                    }
                    ++it;
                } else if (it->isClass() && !it->getClass().name.path.empty() &&
                           std::holds_alternative<std::string>(it->getClass().name.path.back()) &&
                           wanted.contains(prefix + std::get<std::string>(it->getClass().name.path.back()))) {
                    out.push_back(std::move(*it));
                    it = source.erase(it);
                } else ++it;
            }
        }

        // Types in the generated parser's namespace are written relative to it (`modules::p::ID`); an imported class
        // lives outside of it, so a typed rule's captured types are spelled `<namespace>::FlatTypes::modules_...`.
        void qualifyType(LangAPI::Type& type, const std::string& ns) {
            if (type.isSymbol()) {
                auto& path = type.getSymbol().path;
                if (!path.empty() && std::holds_alternative<std::string>(path.front()) &&
                    std::get<std::string>(path.front()) == "modules") {
                    std::string flat;
                    for (const auto& piece : path) if (const auto* text = std::get_if<std::string>(&piece)) flat += (flat.empty() ? "" : "_") + *text;
                    path = {LangAPI::Symbol::PathPart{ns}, LangAPI::Symbol::PathPart{std::string{"FlatTypes"}}, LangAPI::Symbol::PathPart{flat}};
                }
            }
            for (auto& parameter : type.template_parameters)
                if (auto* inner = std::get_if<LangAPI::Type>(&parameter)) qualifyType(*inner, ns);
        }
        void qualifyClass(LangAPI::Declarations& declarations, const std::string& ns) {
            for (auto& declaration : declarations) {
                if (declaration.isNamespace()) qualifyClass(declaration.getNamespace().declarations, ns);
                else if (declaration.isClass()) {
                    auto& cls = declaration.getClass();
                    if (cls.constructor) for (auto& parameter : cls.constructor->parameters) qualifyType(parameter.first, ns);
                    for (auto& [member, visibility] : cls.data) {
                        if (!member) continue;
                        if (member->isVariable()) qualifyType(member->getVariable().type, ns);
                        else if (member->isFunction()) for (auto& parameter : member->getFunction().parameters)
                            qualifyType(parameter.first, ns);
                    }
                }
            }
        }
        bool mentions(const LangAPI::Type& type, const std::vector<std::string>& path) {
            if (type.isSymbol()) {
                const auto& symbol = type.getSymbol().path;
                if (symbol.size() == path.size()) {
                    bool same = true;
                    for (std::size_t i = 0; same && i < path.size(); ++i) {
                        const auto* piece = std::get_if<std::string>(&symbol[i]);
                        same = piece && *piece == path[i];
                    }
                    if (same) return true;
                }
            }
            for (const auto& parameter : type.template_parameters)
                if (const auto* inner = std::get_if<LangAPI::Type>(&parameter); inner && mentions(*inner, path)) return true;
            return false;
        }
        bool mentions(const LangAPI::Declaration& declaration, const std::vector<std::string>& path) {
            if (declaration.isNamespace()) {
                for (const auto& inner : declaration.getNamespace().declarations) if (mentions(inner, path)) return true;
                return false;
            }
            if (declaration.isVariable()) return mentions(declaration.getVariable().type, path);
            if (!declaration.isClass()) return false;
            for (const auto& [member, visibility] : declaration.getClass().data)
                if (member && member->isVariable() && mentions(member->getVariable().type, path)) return true;
            return false;
        }
        // Moves the first declaration that mentions `path`, and everything after it, into `tail` (keeping the
        // namespace nesting), so the class can be emitted exactly between the types it needs and its first user.
        bool splitBefore(LangAPI::Declarations& declarations, const std::vector<std::string>& path,
                         LangAPI::Declarations& tail) {
            for (std::size_t i = 0; i < declarations.size(); ++i) {
                if (declarations[i].isNamespace()) {
                    auto& ns = declarations[i].getNamespace();
                    LangAPI::Declarations inner;
                    if (!splitBefore(ns.declarations, path, inner)) continue;
                    LangAPI::Namespace rest = ns;
                    rest.declarations = std::move(inner);
                    tail.push_back(LangAPI::Declaration{std::move(rest)});
                    tail.insert(tail.end(), std::make_move_iterator(declarations.begin() + i + 1),
                                std::make_move_iterator(declarations.end()));
                    declarations.erase(declarations.begin() + i + 1, declarations.end());
                    return true;
                }
                if (mentions(declarations[i], path)) {
                    tail.insert(tail.end(), std::make_move_iterator(declarations.begin() + i),
                                std::make_move_iterator(declarations.end()));
                    declarations.erase(declarations.begin() + i, declarations.end());
                    return true;
                }
            }
            return false;
        }
        std::vector<std::string> splitDots(const std::string& text) {
            std::vector<std::string> out;
            for (std::size_t begin = 0, end; begin <= text.size(); begin = end + 1) {
                end = text.find('.', begin);
                if (end == std::string::npos) end = text.size();
                out.push_back(text.substr(begin, end - begin));
            }
            return out;
        }

        // Flat generated struct names (`modules_p_ID`) that a type refers to, whether written as the struct or as
        // its `Types::modules::p::ID` alias.
        void flatNames(const LangAPI::Type& type, std::set<std::string>& names) {
            if (type.isSymbol()) {
                std::vector<std::string> parts;
                for (const auto& piece : type.getSymbol().path) if (const auto* text = std::get_if<std::string>(&piece)) parts.push_back(*text);
                if (!parts.empty() && parts.back().starts_with("modules_")) names.insert(parts.back());
                if (const auto it = std::ranges::find(parts, "modules"); it != parts.end()) {
                    std::string flat;
                    for (auto at = it; at != parts.end(); ++at) flat += (flat.empty() ? "" : "_") + *at;
                    names.insert(flat);
                }
            }
            for (const auto& parameter : type.template_parameters)
                if (const auto* inner = std::get_if<LangAPI::Type>(&parameter)) flatNames(*inner, names);
        }
        void flatNames(const LangAPI::Declaration& declaration, std::set<std::string>& names) {
            if (declaration.isNamespace()) for (const auto& inner : declaration.getNamespace().declarations) flatNames(inner, names);
            else if (declaration.isVariable()) flatNames(declaration.getVariable().type, names);
            else if (declaration.isClass()) for (const auto& [member, visibility] : declaration.getClass().data)
                if (member && member->isVariable()) flatNames(member->getVariable().type, names);
        }
        // Cuts the class named `flat` out of `declarations`, wrapped in copies of its enclosing namespaces.
        bool extractClass(LangAPI::Declarations& declarations, const std::string& flat, LangAPI::Declarations& out) {
            for (std::size_t i = 0; i < declarations.size(); ++i) {
                if (declarations[i].isNamespace()) {
                    auto& ns = declarations[i].getNamespace();
                    LangAPI::Declarations inner;
                    if (!extractClass(ns.declarations, flat, inner)) continue;
                    LangAPI::Namespace copy = ns;
                    copy.declarations = std::move(inner);
                    out.push_back(LangAPI::Declaration{std::move(copy)});
                    return true;
                }
                if (declarations[i].isClass()) {
                    const auto& path = declarations[i].getClass().name.path;
                    const auto* leaf = path.empty() ? nullptr : std::get_if<std::string>(&path.back());
                    if (leaf && *leaf == flat) {
                        out.push_back(std::move(declarations[i]));
                        declarations.erase(declarations.begin() + i);
                        return true;
                    }
                }
            }
            return false;
        }
        // Moves the structs `names` (and what they need) out of `tail` into `before`, dependencies first.
        void pullDependencies(LangAPI::Declarations& tail, const std::set<std::string>& names,
                              LangAPI::Declarations& before, std::set<std::string>& done) {
            for (const auto& name : names) {
                if (!done.insert(name).second) continue;
                LangAPI::Declarations one;
                if (!extractClass(tail, name, one)) continue;
                std::set<std::string> needs;
                for (const auto& declaration : one) flatNames(declaration, needs);
                pullDependencies(tail, needs, before, done);
                before.insert(before.end(), std::make_move_iterator(one.begin()), std::make_move_iterator(one.end()));
            }
        }
        // Classes that a typed rule instantiates for its value: the rule's `value` action is a call of the class.
        std::set<std::string> typedRuleClasses(AST::Tree& tree) {
            std::set<std::string> classes;
            for (const auto& [name, rule] : tree) for (const auto& feature : rule.features)
                if (const auto* action = std::get_if<AST::PythonAction>(&feature); action && action->target == "value")
                    if (const auto* expression = std::get_if<UAST::py::Expr>(&action->code))
                        if (const auto* call = expression->get<UAST::py::Call>(); call && call->func)
                            if (const auto* callee = call->func->get<UAST::py::Name>(); callee && callee->id.starts_with("python."))
                                classes.insert(callee->id);
            return classes;
        }
    }
    Construct::Construct(LexerBuilder &&lexer_builder, LRBase &parser, AST::Tree &tree,
                         LangAPI::Language lang, const std::string &namespace_name)
        : lexer_builder(std::move(lexer_builder)), lr(&parser), tree(tree), lang(lang), namespace_name(namespace_name) {
        if (!parser.isSupported()) throw std::invalid_argument("LR(*) is not implemented");
        if (!parser.automaticConflictResolution()) parser.requireConflictFree();
        stdu::vector<ParserIR::Production> metadata;
        for (std::size_t i = 1; i < parser.getProductions().size(); ++i) {
            const auto &production = parser.getProductions()[i];
            ParserIR::Production entry;
            entry.name = production.lhs;
            entry.block = ParserIR::dataBlock(production.source.data_block);
            // ctx.ast.x assigned by a rule's statements are AST fields in every algorithm
            if (!production.source.normalization_source && tree.contains(production.lhs))
                entry.block = ParserIR::withActionFields(tree, production.lhs, tree[production.lhs], std::move(entry.block));
            metadata.push_back(std::move(entry));
        }
        stdu::vector<DFA::DFA> dfas;
        ir = ParserIR::IR(std::move(metadata), false, std::move(dfas));
    }
    auto Construct::construct(LexerBuilder &&lexer_builder, LRBase &parser, AST::Tree &tree,
                              LangAPI::Language lang, const std::string &namespace_name) -> Holder {
        Construct construct(std::move(lexer_builder), parser, tree, lang, namespace_name);
        return construct.construct();
    }
    auto Construct::constructTypes() -> void {
        ConstructTypes construct_types(holder, lexer_builder, ir, namespace_name, tree);
        construct_types.constructTokensAndRulesEnum();
        // construct_types.constructTokensAndRulesEnumToString();
        construct_types.constructTypesNamespace();
    }

    auto Construct::constructLexer() -> void {
        ConstructLexer construct_lexer(holder, lexer_builder, ir, tree);
        construct_lexer.constructLexer();

    }
    auto Construct::constructParser() -> void {
        if (lr) {
            ConstructParser construct_parser(holder, lexer_builder, ir, *lr, tree);
            construct_parser.constructParser();
        } else {
            ConstructParser construct_parser(holder, lexer_builder, ir, tree);
            construct_parser.constructParser();
        }
    }

    auto Construct::construct() -> Holder& {
        if (lang == LangAPI::Language::Cpp) holder.include("ispastdlib/ispa_debug.hpp");
        for (const auto& [name, rule] : tree) if (rule.external) {
            holder.include(rule.external->header);
            holder.include("ispastdlib/external.hpp");
        }
        constructTypes();
        constructLexer();
        constructParser();
        // constructParser();
        LangAPI::Namespace ns;
        ns.name = namespace_name;
        ns.declarations = std::move(holder.get());
        holder.get().clear();
        // A typed rule stores its class by value in the generated types.
        auto python = tree.getPythonDeclarations();
        qualifyClass(python, namespace_name);
        LangAPI::Declarations hoisted;
        const auto wanted = typedRuleClasses(tree);
        if (lang == LangAPI::Language::Cpp && !wanted.empty()) {
            // C++ needs every class complete before the first struct that stores it, and after the capture structs it
            // holds. A struct can store one class while another class holds that struct, so the generated structs and the
            // classes are placed one by one, dependencies first.
            std::map<std::string, LangAPI::Declarations> classes;
            for (const auto& name : wanted) hoistClasses(python, {name}, classes[name], {});
            LangAPI::Declarations tail;
            for (const auto& name : wanted) {
                LangAPI::Declarations cut;
                if (splitBefore(ns.declarations, splitDots(name), cut)) tail.insert(tail.begin(), std::make_move_iterator(cut.begin()), std::make_move_iterator(cut.end()));
            }
            holder.push(ns);
            const auto wrap = [&](LangAPI::Declarations units) {
                LangAPI::Namespace wrapper;
                wrapper.name = namespace_name;
                wrapper.declarations = std::move(units);
                holder.push(wrapper);
            };
            std::map<std::string, int> state;   // 1: being placed, 2: placed
            std::function<void(const std::string&)> placeStruct, placeClass;
            const auto placeNeeds = [&](const LangAPI::Declarations& units, const std::string& what) {
                std::set<std::string> flats;
                for (const auto& unit : units) flatNames(unit, flats);
                for (const auto& flat : flats) {
                    if (state[flat] == 1) throw std::invalid_argument("Typed rule " + what + " is recursive: a Python class cannot hold a rule that stores it");
                    placeStruct(flat);
                }
                for (const auto& name : wanted) {
                    if (name == what) continue;
                    for (const auto& unit : units) if (mentions(unit, splitDots(name))) {
                        if (state["class " + name] == 1) throw std::invalid_argument("Typed rule " + what + " is recursive: a Python class cannot hold a rule that stores it");
                        placeClass(name);
                        break;
                    }
                }
            };
            placeStruct = [&](const std::string& flat) {
                if (state[flat]) return;
                LangAPI::Declarations unit;
                if (!extractClass(tail, flat, unit)) return;
                state[flat] = 1;
                placeNeeds(unit, flat);
                wrap(std::move(unit));
                state[flat] = 2;
            };
            placeClass = [&](const std::string& name) {
                auto& key = state["class " + name];
                if (key) return;
                key = 1;
                placeNeeds(classes.at(name), name);
                for (auto& unit : classes.at(name)) holder.push(unit);
                state["class " + name] = 2;
            };
            for (const auto& name : wanted) placeClass(name);
            if (!tail.empty()) wrap(std::move(tail));
        } else {
            if (!wanted.empty()) hoistClasses(python, wanted, hoisted, {});
            holder.get().insert(holder.get().end(), hoisted.begin(), hoisted.end());
            holder.push(ns);
        }
        // Imported helpers live outside the generated parser's namespace and
        // may refer to its Token, Types, Lexer, and other generated names.
        auto& declarations = holder.get();
        declarations.insert(declarations.end(), python.begin(), python.end());
        return holder;
    }
    auto Construct::construct(LexerBuilder &&lexer_builder, ParserIR::IR &&ir, AST::Tree &tree, LangAPI::Language lang, const std::string &namespace_name) -> Holder {
        Construct construct(std::move(lexer_builder), std::move(ir), tree,  lang, namespace_name);
        return construct.construct();
    }
}
