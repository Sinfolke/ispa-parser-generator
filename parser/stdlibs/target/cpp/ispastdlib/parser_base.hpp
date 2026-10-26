/**
 * ispastdlib/parser.hpp - LLParser_base
 * Part of the split ISPA std lib; include <ispastdlib.hpp> for everything.
 */
#pragma once
#include <ispastdlib/core.hpp>
#include <ispastdlib/node.hpp>
#include <ispastdlib/lexer.hpp>
#include <ispastdlib/raw_token.hpp>
#include <ispastdlib/token_flow.hpp>
#include <ispastdlib/traits.hpp>

namespace ISPA_STD {
    /* PARSER */
    template<typename MainNode, typename Token>
    class Parser_base {
    protected:
        Lexer_base<Token>* lexer = nullptr;
        const char* text = nullptr;
        MainNode tree;
    public:
        // Constructors
        Parser_base() {}
        Parser_base(Lexer_base<Token>& lexer) {
            this->lexer = &lexer;
        }
        Parser_base(const char* text) : text(text) {}
        virtual ~Parser_base() {}
        // Parsing methods
        MainNode& parse(Lexer_base<Token>& lex) {
            lexer = &lex;
            return parse();
        }
        MainNode& parse(const char* txt) {
            text = txt;
            return parseLazy();
        }
        void setInput(Lexer_base<Token> &lex) {
            if (!lex.hasTokens())
                lexer = &lex;
        }
        void setInput(const char* txt) {
            text = txt;
        }
        void clearInput() {
            lexer = nullptr;
            text = nullptr;
        }
    protected:
        /**
         * Parse NON-LAZILY input
         */
        virtual MainNode& parse() {
            if (lexer == nullptr)
                throw IspaAPIExcept("Can't parse because no input lexer is provided");
            if (!lexer->hasInput())
                lexer->makeTokens();
            parseFromTokens();
            return tree;
        }
        /*
         * Parse LAZILY input
         */
        virtual MainNode& parseLazy() {
            if (text == nullptr)
                throw IspaAPIExcept("Can't parse because no input text is provided");
            auto result = lazyParse();
            if (result.status) tree = std::move(result.node.data());
            return tree;
        }
        /*
         * DO NOT OVERRIDE
         * This function is used internally as bridge between generator and std lib
         * you should override Lexer::parse() instead
         */
        virtual void parseFromTokens() = 0;
        /*
         * DO NOT OVERRIDE
         * This function is used internally as bridge between generator and std lib
         * you should override Lexer::parseLazy() instead
         */
        virtual MatchResult<MainNode, typename Lexer_base<Token>::lazy_iterator> lazyParse() = 0;
    };
} // namespace ISPA_STD
