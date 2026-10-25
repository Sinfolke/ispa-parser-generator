module LangRepr.Construct;

import Dump;

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
        holder.push(ns);
        // Imported helpers live outside the generated parser's namespace and
        // may refer to its Token, Types, Lexer, and other generated names.
        auto& declarations = holder.get();
        declarations.insert(declarations.end(), tree.getPythonDeclarations().begin(), tree.getPythonDeclarations().end());
        return holder;
    }
    auto Construct::construct(LexerBuilder &&lexer_builder, ParserIR::IR &&ir, AST::Tree &tree, LangAPI::Language lang, const std::string &namespace_name) -> Holder {
        Construct construct(std::move(lexer_builder), std::move(ir), tree,  lang, namespace_name);
        return construct.construct();
    }
}
