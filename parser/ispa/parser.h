
#pragma once
#include <Parser.h>
#include <ispa/lexical.hpp>

#include <stdexcept>
#include <string>
#include <string_view>

// The modular generator qualifies token types under their source modules.
// Keep one custom lexer implementation for both generated parser layouts.
namespace IspaSyntax {
#ifdef ISPA_MODULAR_PARSER
#ifndef ISPA_EXTERNAL_PYTHON
using Python = ::Parser::Types::modules::common::PY_RAW;
inline const auto pythonToken = ::Parser::Tokens::modules::common::PY_RAW::id;
#endif
using String = ::Parser::Types::modules::rule::regular::value::STRING;
using CharacterGroup = ::Parser::Types::modules::rule::regular::value::CGROUP;
inline const auto stringToken = ::Parser::Tokens::modules::rule::regular::value::STRING::id;
inline const auto characterGroupToken = ::Parser::Tokens::modules::rule::regular::value::CGROUP::id;
#else
using Python = ::Parser::Types::PY_RAW;
using String = ::Parser::Types::rule::value::STRING;
using CharacterGroup = ::Parser::Types::rule::value::CGROUP;
inline const auto pythonToken = ::Parser::Tokens::PY_RAW::id;
inline const auto stringToken = ::Parser::Tokens::rule::value::STRING::id;
inline const auto characterGroupToken = ::Parser::Tokens::rule::value::CGROUP::id;
#endif
}

class IspaLexer : public Parser::Lexer {
public:
    using Token = Parser::Token;

private:
    // Scans a single-quoted ISPA grammar string.
    //
    // Examples:
    //   'hello'
    //   'it\\'s working'
    //   '\\\\'
    //   '\\n'
    //
    // Returns a pointer immediately after the closing quote.
    static const char* scanString(const char* begin) {
        if (!begin || *begin != '\'')
            throw std::runtime_error("Expected ISPA string");

        const char* p = begin + 1;

        while (*p != '\0') {
            if (*p == '\\') {
                ++p;

                if (*p == '\0')
                    throw std::runtime_error(
                        "Unterminated escape sequence in ISPA string"
                    );

                ++p;
                continue;
            }

            if (*p == '\'')
                return p + 1;

            if (*p == '\n' || *p == '\r')
                throw std::runtime_error(
                    "Unterminated ISPA string before newline"
                );

            ++p;
        }

        throw std::runtime_error("Unterminated ISPA string");
    }
    static const char* scanCharacterGroup(const char* begin) {
        if (!begin || *begin != '[')
            throw std::runtime_error("Expected character group");

        const char* p = begin + 1;

        while (*p) {
            if (*p == '\\') {
                if (!p[1])
                    throw std::runtime_error(
                        "Unterminated character group escape"
                    );

                p += 2;
                continue;
            }

            if (*p == ']')
                return p + 1;

            ++p;
        }

        throw std::runtime_error("Unterminated character group");
    }

public:
    Token makeToken(const char*& pos) override {
#ifdef ISPA_MODULAR_PARSER
        // These delimit the host statements in an otherwise Python body.
        if (IspaSyntax::isIspaMatch(pos)) {
            const char* begin = pos; pos += 5;
            return Token::create(getCurrentPos(begin), begin, 5, __line(begin),
                Parser::Tokens::modules::rule::advanced::advanced_rule::regular_match::MATCH::id,
                Parser::Types::modules::rule::advanced::advanced_rule::regular_match::MATCH{});
        }
#endif
#ifdef ISPA_EXTERNAL_PYTHON
        // Preserve the lexical priority of the Python block delimiter over
        // the host identifier "p". The external parser owns its payload.
        if (pos[0] == 'p' && pos[1] == '{') {
            const char* begin = pos;
            pos += 2;
            return Token::ambiguous(ISPA_STD::RawTokenMeta{getCurrentPos(begin), begin, pos,
                2, __line(begin), __column(begin), Token::ambiguous_id});
        }
#else
        // Python semantic code block: p{ ... }
        if (pos[0] == 'p' && pos[1] == '{') {
            const char* begin = pos;
            const char* end = IspaSyntax::scanPythonBlock(begin);

            auto token = Parser::Token::create(
                Lexer::getCurrentPos(pos),
                begin,
                static_cast<long long>(end - begin),
                0,
                IspaSyntax::pythonToken,
                IspaSyntax::Python{
                    .value = std::string(begin, end)
                }
            );

            pos = end;
            return token;
        }
#endif

        // Single-quoted ISPA grammar string.
        if (pos[0] == '\'') {
            const char* begin = pos;
            const char* end = scanString(begin);

            auto token = Parser::Token::create(
                Lexer::getCurrentPos(pos),
                begin,
                static_cast<long long>(end - begin),
                0,
                IspaSyntax::stringToken,
                IspaSyntax::String{
                    // Preserve escapes, remove surrounding quotes.
                    .value = std::string(begin + 1, end - 1)
                }
            );

            pos = end;
            return token;
        }
        if (pos[0] == '[') {
            const char* begin = pos;
            const char* end = scanCharacterGroup(begin);
            char negative = '\0';
            const char* v_begin = begin + 1;
            if (*(begin + 1) == '^') {
                negative = '^';
                v_begin += 1;
            }
            auto token = Parser::Token::create(
                Lexer::getCurrentPos(pos),
                begin,
                static_cast<long long>(end - begin),
                0,
                IspaSyntax::characterGroupToken,
                IspaSyntax::CharacterGroup{
                    .negative = negative,
                    .v = std::string(v_begin, end - 1)
                }
            );

            pos = end;
            return token;
        }
        return Parser::Lexer::makeToken(pos);
    }
    auto clone() const -> std::shared_ptr<ISPA_STD::Lexer_base<Token>> override {
        return std::make_shared<IspaLexer>(*this);
    }
};
class IspaParser : public Parser::Parser {
    IspaLexer grammar_lexer_;
    public:
        auto lazyParse() -> ISPA_STD::MatchResult<::Parser::Types::main, ::Parser::Lexer::lazy_iterator> override {
            grammar_lexer_.clearTokens();
            auto begin = IspaLexer::lazy_iterator(grammar_lexer_, text);
            skip_spaces<::Parser::Types::__WS>(begin);
            auto res = main(begin);
            auto end = res.it;
            skip_spaces<::Parser::Types::__WS>(end);
            if (!res.status || !end.isEnd()) {
                raise_error(*end, "unexpected " + describe_token(end) +
                    "; expected a grammar declaration or end of input");
                throw ISPA_STD::IspaAPIExcept("Could not parse ISPA grammar");
            }
            tree = res.node.data();
            return res;
        }
};
