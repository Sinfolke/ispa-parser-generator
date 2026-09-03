export module LangRepr.Construct;

import LangRepr.Holder;

import LexerBuilder;
import LangAPI;
import LLIR.API;
import LLIR.IR;
import AST.Tree;
import hash;
import cpuf.printf;

import dstd;
import std;
export namespace LangRepr {
    class Construct {
        Holder holder;
        LexerBuilder lexer_builder;
        LLIR::IR ir;
        AST::Tree &tree;
        LangAPI::Language lang;
        const std::string &namespace_name;
        auto constructTypes() -> void;
        auto constructLexer() -> void;
        auto constructParser() -> void;
    public:
        Construct(LexerBuilder &&lexer_builder, LLIR::IR &&ir, AST::Tree &tree, LangAPI::Language lang, const std::string &namespace_name)
        : lexer_builder(std::move(lexer_builder)), ir(std::move(ir)), tree(tree), lang(lang), namespace_name(namespace_name) {}

        auto construct() -> Holder&;
        static auto construct(LexerBuilder &&lexer_builder, LLIR::IR &&ir, AST::Tree &tree, LangAPI::Language lang, const std::string &namespace_name) -> Holder;
    };

}
