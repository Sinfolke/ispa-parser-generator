#pragma once
#include <DepParser.h>
#include <ispa/lexical.hpp>
#include <cstring>
#include <vector>

namespace IspaDependency {
using Syntax = ::DepParser::Types::modules::ispa_dep_scanner;
struct Span { std::size_t offset, length; };
}

// Only opaque Python blocks need a custom lexical boundary. All ISPA source
// structure, strings, character groups and comments come from grammar.isc.
class IspaDepLexer : public DepParser::Lexer {
    struct Delimiter { char value; std::size_t offset; };
    std::vector<Delimiter> delimiters;
    bool directive_braces = false;
    const char* opaque_end = nullptr;
public:
    using Token = DepParser::Token;
    std::vector<IspaDependency::Span> comments;
    std::size_t offset = 0;

    using DepParser::Lexer::push;
    void push(Token&& token) override {
        // Whitespace and comments are trivia in the dependency grammar. Keep
        // their byte spans in the source, without feeding trivia to the parser.
        if (token.template holds<DepParser::Types::__WS>()) return;
        const auto& meta = token.meta();
        if (meta.start() && meta.length() == 2 &&
            (meta.start()[0] == '&' || meta.start()[0] == '>') && meta.start()[1] == '(')
            delimiters.push_back({'(', meta.startpos() + 1});
        if (meta.start() && meta.length() == 1) {
            const char value = *meta.start();
            if (value == '(' || value == '{') delimiters.push_back({value, meta.startpos()});
            else if (value == ')' || value == '}') {
                const char opening = value == ')' ? '(' : '{';
                if (delimiters.empty() || delimiters.back().value != opening) {
                    offset = meta.startpos();
                    throw std::runtime_error(std::string("unexpected closing delimiter '") + value + "'");
                }
                delimiters.pop_back();
            }
        }
        DepParser::Lexer::push(std::move(token));
    }
    void push() override {
        if (!delimiters.empty()) {
            const auto opening = delimiters.back();
            offset = std::strlen(_in);
            const char closing = opening.value == '(' ? ')' : '}';
            throw std::runtime_error(std::string("expected '") + closing + "' to close '" +
                opening.value + "' opened at line " + std::to_string(__line(_in + opening.offset)));
        }
        DepParser::Lexer::push();
    }

    Token makeToken(const char*& pos) override {
        offset = getCurrentPos(pos);
        opaque_end = nullptr;
        if (delimiters.empty()) {
            if (IspaSyntax::keyword(pos, "import") || IspaSyntax::keyword(pos, "feature")) directive_braces = true;
            if (*pos == ';') directive_braces = false;
        }
        // Module directive selections remain structured. Every grammar body
        // brace is opaque to dependency discovery, including direct Python.
        if (*pos == '{' && !directive_braces) opaque_end = IspaSyntax::scanBracedPython(pos) - 1;
        auto token = DepParser::Lexer::makeToken(pos);
        // Feed the opening and closing braces to the grammar, while the shared
        // Python scanner owns everything between them (comments included).
        if (opaque_end && !token.empty()) pos = opaque_end;
        if (token.empty() || pos == _in + offset)
            throw std::runtime_error("unterminated or unrecognized token in ISPA source");
        // COMMENT may live in scanner_tokens or in the legacy scanner module.
        // Strings and Python blocks keep their own lexical boundaries.
        if (token.meta().length() >= 2 && token.meta().start()[0] == '/' &&
            (token.meta().start()[1] == '/' || token.meta().start()[1] == '*')) {
            const auto& meta = token.meta();
            comments.push_back({meta.startpos(), meta.length()});
            return Token::create(meta.startpos(), meta.start(), meta.length(), 0,
                DepParser::Tokens::__WS::id, DepParser::Types::__WS{});
        }
        return token;
    }
};

class IspaDepParser : public DepParser::Parser {
    template<class IT> auto parseSource(IT begin) {
        while (!begin.isEnd() && (*begin).template holds<DepParser::Types::__WS>()) ++begin;
        auto result = main(begin);
        auto end = result.it;
        while (!end.isEnd() && (*end).template holds<DepParser::Types::__WS>()) ++end;
        if (!result.status || !end.isEnd() || !getErrors().empty() || !getContextErrors().empty()) {
            offset = end.isEnd() ? (text ? std::strlen(text) : 0) : (*end).meta().startpos();
            throw std::runtime_error("invalid ISPA dependency syntax");
        }
        return std::move(result);
    }
public:
    std::vector<IspaDependency::Span> comments;
    std::size_t offset = 0;

    ISPA_STD::MatchResult<DepParser::Types::main, DepParser::Lexer::lazy_iterator> lazyParse() override {
        comments.clear();
        offset = 0;
        IspaDepLexer lexer;
        lexer.setInput(text);
        try {
            auto r = parseSource(IspaDepLexer::lazy_iterator(lexer, text));
            comments = std::move(lexer.comments);
            return r;
        } catch (...) {
            if (!offset) offset = lexer.offset;
            throw;
        }
    }
    void parseFromTokens() override {
        comments.clear();
        offset = 0;
        if (const auto* source = dynamic_cast<const IspaDepLexer*>(lexer)) comments = source->comments;
        tree = parseSource(DepParser::Lexer::iterator(lexer)).node.data();
    }
};
