module LangRepr.Construct;

import Dump;

import LLIR.API;
import NFA;
import DFA.API;

import constants;
import corelib;
import cpuf.printf;
import logging;
import hash;
import DFA.Base;

import dstd;
import std;

import LangRepr.ConstructTypes;
import LangRepr.ConstructLexer;
import LangRepr.ConstructParser;

namespace LangRepr {
    auto Construct::constructTypes() -> void {
        ConstructTypes construct_types(holder, lexer_builder, ir);
        construct_types.constructTokensAndRulesEnum();
        // construct_types.constructTokensAndRulesEnumToString();
        construct_types.constructTypesNamespace();
    }

    auto Construct::constructLexer() -> void {
        ConstructLexer construct_lexer(holder, lexer_builder, ir, tree);
        construct_lexer.constructLexer();

    }
    auto Construct::constructParser() -> void {
        // ConstructParser construct_parser(holder, lexer_builder, ir);
        // construct_parser.constructParser();
    }

    auto Construct::construct() -> Holder& {
        constructTypes();
        constructLexer();
        constructParser();
        // constructParser();
        LangAPI::Namespace ns;
        ns.name = namespace_name;
        ns.declarations = std::move(holder.get());
        holder.get().clear();
        holder.push(ns);
        return holder;
    }
    auto Construct::construct(LexerBuilder &&lexer_builder, LLIR::IR &&ir, AST::Tree &tree, LangAPI::Language lang, const std::string &namespace_name) -> Holder {
        Construct construct(std::move(lexer_builder), std::move(ir), tree,  lang, namespace_name);
        return construct.construct();
    }
}
