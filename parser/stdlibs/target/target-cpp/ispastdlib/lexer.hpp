/**
 * ispastdlib/lexer.hpp - Lexer_base
 * Part of the split ISPA std lib; include <ispastdlib.hpp> for everything.
 */
#pragma once
#include <stdexcept>
#include <any>
#include <cstring>
#include <ispastdlib/core.hpp>
#include <ispastdlib/dfa_run.hpp>
#include <ispastdlib/token_flow.hpp>
#include <ispastdlib/raw_token.hpp>
#include <ispastdlib/traits.hpp>

namespace ISPA_STD {

    template<class T, class = void> struct has_empty_method : std::false_type {};
    template<class T> struct has_empty_method<T, std::void_t<decltype(std::declval<const T&>().empty())>>
        : std::true_type {};
    template<class T> struct is_std_variant : std::false_type {};
    template<class... T> struct is_std_variant<std::variant<T...>> : std::true_type {};

    struct LexerFailureContext {
        enum class Decision { None, Delete, Insert, Abort };
        const char* cursor;
        std::vector<std::string>& diagnostics;
        std::string rule;
        std::string failed_rule;
        Decision decision = Decision::None;

        bool at(const std::string& value, std::size_t offset = 0) const {
            const char* current = cursor;
            while (offset-- && *current) ++current;
            return value == (*current ? std::string(1, *current) : std::string{});
        }
        void error(const std::string& message) { diagnostics.push_back(message); }
        void erase() { decision = Decision::Delete; }
        void skip() { erase(); }
        void insert() { decision = Decision::Insert; }
        void abort() { decision = Decision::Abort; }
    };

    template<typename Token>
    constexpr bool is_token_end(const Token& token) {
        if constexpr (is_raw_token_v<Token>) {
            return token.empty() && !token.isAmbiguous();
        } else if constexpr (has_empty_method<Token>::value) {
            return token.empty();
        } else if constexpr (is_std_variant<std::decay_t<Token>>::value) {
            return std::holds_alternative<std::monostate>(token);
        } else {
            return false;
        }
    }

    template<typename Token>
    class Lexer_base {
    protected:
        const char* _in = nullptr;
        std::string _owned_input;
        TokenFlow<Token> tokens;
        std::vector<std::string> errors;
        long long line = 1;
        bool external_boundary = false;

        LexerFailureContext lexer_failure_context(const char* cursor, std::string rule,
                                                   std::string failed_rule) {
            return {cursor, errors, std::move(rule), std::move(failed_rule)};
        }
        std::pair<std::size_t, Token> lexer_failure_result(LexerFailureContext& ctx, Token inserted) {
            if (ctx.decision == LexerFailureContext::Decision::Delete)
                return {DFA::API::lexer_delete_state, Token{}};
            if (ctx.decision == LexerFailureContext::Decision::Insert)
                return {DFA::API::null_state, std::move(inserted)};
            throw std::runtime_error(errors.empty() ? "unmatched lexer input" : errors.back());
        }

        std::size_t getCurrentPos(const char* pos) const {
            return pos - _in;
        }

        std::size_t skip_spaces(const char*& in) {
            auto prev = in;
            while (isspace(*in)) in++;
            return in - prev;
        }

        std::size_t __line(const char* pos) const {
            if (!_in || !pos) throw std::logic_error("Lexer line lookup has no source position");
            std::size_t count = 1;
            const char* in = _in;
            for (; in < pos && *in != '\0'; in++) {
                if (*in == '\n') count++;
            }
            if (in != pos) throw std::out_of_range("Lexer line lookup exceeds source input");
            return count;
        }

        std::size_t __column(const char* pos) const {
            if (!_in || !pos) throw std::logic_error("Lexer column lookup has no source position");
            std::size_t count = 1;
            const char* in = _in;
            for (; in < pos && *in != '\0'; in++) {
                count = (*in == '\n') ? 0 : count + 1;
            }
            if (in != pos) throw std::out_of_range("Lexer column lookup exceeds source input");
            return count;
        }

        std::string __line_prefix(const char* pos) const {
            if (!_in || !pos) throw std::logic_error("Lexer line prefix has no source position");
            const char* begin = pos;
            while (begin > _in && begin[-1] != '\n' && begin[-1] != '\r') --begin;
            return {begin, pos};
        }

        std::string __next_char(const char* pos) const {
            if (!pos) throw std::logic_error("Lexer next character has no source position");
            return *pos == '\0' ? std::string{} : std::string(1, *pos);
        }

        void panic_mode(const char*& pos) {
            if (*pos != '\0') ++pos;
        }

        template<
            typename SemanticFunc,
            std::size_t table_states,
            std::size_t table_classes,
            std::size_t action_table_states,
            typename Layout
        >
        Token lookup(
            const DFA::API::Table<table_states, table_classes> &table,
            const DFA::API::CharToClass &class_table,
            const DFA::API::LRTable<action_table_states> &action_table,
            Layout layout,
            SemanticFunc semantic,
            const char* &pos
        ) {
            // MSVC's zero-size std::array still stores a dummy DFADebug, whose
            // std::string members are not literal types in C++17.
            static const DFA::API::DFADebugTable<0> no_debug{};
            static const DFA::API::DebugIndex no_debug_index{};
            return lookup(table, class_table, action_table, layout, semantic,
                          no_debug, no_debug_index, pos);
        }

        template<
            typename SemanticFunc,
            std::size_t table_states,
            std::size_t table_classes,
            std::size_t action_table_states,
            std::size_t debug_size,
            typename Layout
        >
        Token lookup(
            const DFA::API::Table<table_states, table_classes> &table,
            const DFA::API::CharToClass &class_table,
            const DFA::API::LRTable<action_table_states> &action_table,
            Layout layout,
            SemanticFunc semantic,
            const DFA::API::DFADebugTable<debug_size> &debug_array,
            const DFA::API::DebugIndex &debug_index,
            const char* &pos
        ) {
            if (*pos == '\0') {
                return Token {};
            }
#ifdef ISPA_STDLIB_DFA_RUN
            // Customization point: another standard library (--stdlib) supplies the DFA interpreter.
            return ISPA_STDLIB_DFA_RUN<Token>(
                pos, getCurrentPos(pos), line, table, class_table, action_table,
                semantic, debug_array, debug_index, layout
            );
#else
            return DFA::run<Token>(
                pos, getCurrentPos(pos), line, table, class_table, action_table,
                semantic, debug_array, debug_index, layout
            );
#endif
        }

    public:

        class lazy_iterator {
            struct SharedState {
                const char* pos = nullptr;
                bool eof_processed = false;
                std::vector<std::any> checkpoints;

                explicit SharedState(const char* in) : pos(in) {}
            };

            Lexer_base* owner = nullptr;
            std::shared_ptr<SharedState> state;
            std::size_t token_index = 0;
            std::size_t distance_base = 0;
            std::shared_ptr<Lexer_base> owned_owner;

            void ensure_token() {
                if (!owner || !state)
                    return;

                // All iterator copies share the token cache and source
                // cursor, but retain independent logical token indices.
                while (owner->tokens.size() <= token_index) {

                    if (state->eof_processed)
                        break;

                    if (!state->pos) {
                        throw std::logic_error(
                            "Lazy lexer has no source input"
                        );
                    }

                    // EOF must be finalized exactly once.
                    if (*state->pos == '\0') {
                        state->eof_processed = true;

                        // Virtual dispatch to language-specific EOF
                        // handling (e.g. PyLexer::finish()).
                        owner->push();
                        break;
                    }

                    const char* before = state->pos;

                    const auto checkpoint = owner->checkpoint();
                    Token raw = owner->makeToken(state->pos);
                    if (raw.empty() && owner->external_boundary) {
                        state->pos = before;
                        state->eof_processed = true;
                        owner->push();
                        break;
                    }
                    if constexpr (is_raw_token_v<Token>) {
                        if (raw.empty() && !raw.isAmbiguous() && owner->hasExternalRules()) {
                            state->pos = before + 1;
                            raw = Token::ambiguous(RawTokenMeta{owner->getCurrentPos(before), before, state->pos,
                                1, owner->__line(before), owner->__column(before), Token::ambiguous_id});
                        }
                    }

                    if (state->pos == before) {
                        throw std::runtime_error(
                            "Lazy lexer did not advance source position"
                        );
                    }

                    // Must dispatch through the derived lexer.
                    if (!is_token_end(raw)) owner->push(std::move(raw));
                    while (state->checkpoints.size() < owner->tokens.size()) state->checkpoints.push_back(checkpoint);

                    // push() may emit zero, one, or multiple tokens.
                    // Keep lexing until the requested token exists.
                }
            }

        public:
            lazy_iterator() = default;

            lazy_iterator(Lexer_base* lexer, const char* in)
                : owner(lexer),
                  state(std::make_shared<SharedState>(in)),
                  token_index(0) {
                // Token offsets are relative to the source used by this lazy
                // stream, just as they are for makeTokens(input).
                owner->_in = in;
            }

            lazy_iterator(Lexer_base& lexer, const char* in)
                : lazy_iterator(&lexer, in) {}

            lazy_iterator(Lexer_base& lexer, const char* origin, const char* start)
                : lazy_iterator(&lexer, origin) {
                state->pos = start;
                owner->line = owner->__line(start);
            }

            const char* sourceOrigin() const { return owner ? owner->_in : nullptr; }
            const std::vector<std::string>& getErrors() const {
                static const std::vector<std::string> empty;
                return owner ? owner->getErrors() : empty;
            }
            const char* sourcePosition() const {
                if (!owner || !state) return nullptr;
                if (token_index < owner->tokens.size()) {
                    const auto& token = owner->tokens[token_index];
                    if (!is_token_end(token) && token.meta().start()) return token.meta().start();
                }
                return state->pos;
            }
            // A separate cache/lexer branch preserves all pre-call iterator
            // copies, including callers probing an optional external rule.
            void consumeExternal(const char* end) {
                const char* begin = sourcePosition();
                const char* origin = sourceOrigin();
                if (!origin || !begin || !end || end < begin || end > origin + std::strlen(origin))
                    throw std::logic_error("External parser returned an invalid source cursor");
                auto branch = owner->clone();
                if (token_index < state->checkpoints.size()) branch->restoreCheckpoint(state->checkpoints[token_index]);
                branch->tokens.clear();
                branch->_in = origin;
                branch->line = branch->__line(end);
                branch->externalAdvance(begin, end);
                distance_base = distance() + (end != begin ? 1 : 0);
                token_index = 0;
                owned_owner = std::move(branch);
                owner = owned_owner.get();
                state = std::make_shared<SharedState>(end);
            }

            bool isEnd() const {
                if (!owner || !state)
                    return true;

                const_cast<lazy_iterator*>(this)->ensure_token();

                if (token_index >= owner->tokens.size())
                    return true;

                return is_token_end(owner->tokens[token_index]);
            }

            lazy_iterator& operator++() {
                if (!isEnd()) {
                    ++token_index;
                }

                return *this;
            }

            lazy_iterator& operator--() {
                if (token_index > 0) --token_index;
                return *this;
            }

            lazy_iterator operator++(int) {
                auto previous = *this;
                ++(*this);
                return previous;
            }
            lazy_iterator& operator+=(std::size_t count) {
                while (count-- && !isEnd()) ++(*this);
                return *this;
            }
            lazy_iterator operator+(std::size_t count) const {
                auto next = *this;
                next += count;
                return next;
            }

            const Token& operator*() const {
                const_cast<lazy_iterator*>(this)->ensure_token();

                if (token_index >= owner->tokens.size()) {
                    throw std::out_of_range(
                        "lazy_iterator: dereference beyond EOF"
                    );
                }

                return owner->tokens[token_index];
            }

            const Token* operator->() const {
                return &operator*();
            }

            std::ptrdiff_t operator-(
                const lazy_iterator& other
            ) const {
                return static_cast<std::ptrdiff_t>(distance()) -
                       static_cast<std::ptrdiff_t>(other.distance());
            }

            std::size_t distance() const {
                return distance_base + token_index;
            }
            auto &current_state() const {
                return *state;
            }
            auto &current_state() {
                return *state;
            }
            /**
             * Returns the source position immediately after the current token.
             *
             * The returned pointer is suitable for resuming lexing with another
             * lexer, even when this lazy iterator has buffered lookahead tokens.
             */
            const char* resumePosition() const {
                if (!owner || !state)
                    return nullptr;

                auto* self = const_cast<lazy_iterator*>(this);
                self->ensure_token();

                if (token_index >= owner->tokens.size())
                    return state->pos;

                const Token& token = owner->tokens[token_index];

                if (is_token_end(token))
                    return state->pos;

                return token.meta().end();
            }
            /**
             * @brief Resume lexing N bytes after the current token's start.
             *
             * @warning Intended for external parser integration.
             *
             * Invalidates cached tokens starting at the current position,
             * then resumes lexing from the requested source address.
             *
             * All iterator copies share the lexer token cache, so previously
             * created iterators pointing into the invalidated region become
             * logically invalid.
             *
             * @param count Number of source bytes to advance.
             */
            auto advanceCharacters(std::size_t count) -> bool {
                if (!owner || !state)
                    return false;

                ensure_token();

                if (token_index >= owner->tokens.size())
                    return false;

                const Token& current = owner->tokens[token_index];

                if (is_token_end(current))
                    return false;

                const char* start = current.meta().start();

                if (!start)
                    return false;

                const char* target = start;

                // Avoid stepping past the null terminator.
                for (std::size_t i = 0; i < count; ++i) {
                    if (*target == '\0')
                        return false;
                    ++target;
                }

                // Invalidate tokens that were lexed from the old position.
                owner->tokens.erase(
                    owner->tokens.begin() + token_index,
                    owner->tokens.end()
                );

                state->pos = target;
                state->eof_processed = false;

                ensure_token();
                return true;
            }
        };


        // Iterates already-accumulated tokens; run makeTokens() before using this.
        class iterator {
            Lexer_base* owner = nullptr;
            typename TokenFlow<Token>::iterator pos;

        public:
            iterator() = default;
            iterator(Lexer_base& owner) : owner(&owner), pos(owner.tokens.begin()) {}
            iterator(Lexer_base* owner) : owner(owner), pos(owner->tokens.begin()) {}

            iterator& operator=(const iterator& other) = default;
            void operator+=(std::size_t count) { pos += count; }
            iterator& operator++() { pos += 1; return *this; }
            iterator& operator--() { if (pos != owner->tokens.begin()) pos -= 1; return *this; }
            iterator operator++(int) { auto tmp = *this; pos += 1; return tmp; }
            std::size_t operator-(const iterator& other) const { return pos - other.pos; }
            iterator operator+(std::size_t count) const { auto tmp = *this; tmp += count; return tmp; }

            bool isEnd() const { return is_token_end(*pos); }
            Token& operator*() const { return *pos; }
            Token* operator->() const { return &(*pos); }
            std::size_t distance() const { return pos - owner->tokens.begin(); }
            /**
             * @brief Returns the source position immediately after the current token.
             *
             * @warning Compatibility API for external parser integration.
             * Not recommended for ordinary parser code.
             *
             * Uses token source metadata to reconstruct a character pointer.
             * The source buffer must remain alive, and token metadata must refer
             * to that original buffer.
             *
             * This function does not advance the iterator.
             *
             * @return Pointer immediately after the current token, or nullptr
             *         if its source position cannot be determined.
             *
             * @note Synthetic tokens and EOF tokens may not have meaningful
             *       source spans.
             */
            [[nodiscard]]
            auto resumePosition() const -> const char* {
                if (!owner)
                    return nullptr;

                if (pos == owner->tokens.end())
                    return nullptr;

                const Token& token = *pos;

                if (is_token_end(token))
                    return nullptr;

                const auto& meta = token.meta();

                if (!meta.pos)
                    return nullptr;

                return meta._end;
            }
            auto advanceCharacters(std::size_t count) -> bool {
                throw std::runtime_error("Lexer_base::iterator::advanceCharacters() is not implemented. Use lazy_iterator for external parser integration.");
            }
        };

        virtual Token makeToken(const char*& pos) = 0;
        virtual void init() {}
        virtual std::shared_ptr<Lexer_base> clone() const {
            throw std::logic_error("External parser handoff requires a cloneable lexer");
        }
        virtual std::any checkpoint() const { return {}; }
        virtual void restoreCheckpoint(const std::any&) {}
        virtual void externalAdvance(const char*, const char*) {}
        virtual bool hasExternalRules() const { return false; }
        void setExternalBoundary(bool enabled = true) { external_boundary = enabled; }

        Lexer_base() { init(); }
        explicit Lexer_base(const std::string& in) : _owned_input(in), _in(_owned_input.c_str()) { init(); }
        explicit Lexer_base(const char* in) : _in(in) { init(); }
        explicit Lexer_base(const TokenFlow<Token>& tokens) : tokens(tokens) { init(); }
        virtual ~Lexer_base() = default;

        bool hasInput() const { return _in != nullptr; }
        bool hasTokens() const { return !tokens.empty(); }

        Lexer_base& setInput(const std::string& in) {
            errors.clear();
            _owned_input = in;
            _in = _owned_input.c_str();
            return *this;
        }

        Lexer_base& setInput(const char* in) {
            errors.clear();
            _owned_input.clear();
            _in = in;
            return *this;
        }

        const TokenFlow<Token>& getTokens() const { return tokens; }
        TokenFlow<Token>& getTokensReference() { return tokens; }
        void clearTokens() { tokens.clear(); }

        virtual void on_token(Token&& t) { tokens.push_back(std::move(t)); }
        virtual void on_eof() { tokens.emplace_back(); }
        virtual void push(Token&& t) { on_token(std::move(t)); }
        virtual void push(const Token &t) { on_token(Token(t)); }
        virtual void push() { on_eof(); }

        virtual TokenFlow<Token>& makeTokens() {
            if (hasExternalRules()) throw std::logic_error("AMBIGUOUS eager token stream: use lazy tokenization with external parsers");
            if (_in == nullptr) {
                throw IspaAPIExcept("can't lex because no input provided. ");
            }
            const char* pos = _in;
            // Tokens are a few bytes each: reserving avoids moving the whole stream on every doubling (capped, untouched pages stay virtual).
            tokens.reserve(std::min<std::size_t>(std::strlen(_in) / 3 + 16, std::size_t{1} << 24));
            while (*pos != '\0') {
                const char* before = pos;
                Token t = makeToken(pos);
                if (pos == before) {
                    throw IspaInternalExcept(
                        "no matches at position {}, on character {}, and no exception can handle it",
                        "Lexer TDFA generation", true, getCurrentPos(pos), *pos
                    );
                }
                if (!is_token_end(t)) push(std::move(t));
            }
            push();

            return tokens;
        }
        virtual bool has_buffered_tokens() const { return !tokens.empty(); }
        virtual Token pop_buffered_token() {
            Token t = std::move(tokens.front());
            tokens.erase(tokens.begin());
            return t;
        }
        TokenFlow<Token>& makeTokens(const std::string& in) {
            setInput(in);
            return makeTokens();
        }

        TokenFlow<Token>& makeTokens(const char* in) {
            setInput(in);
            return makeTokens();
        }

        virtual TokenFlow<Token>& makeTokensFromFile(const char* path) {
            std::ifstream file(path, std::ios::in | std::ios::binary);
            if (!file) {
                throw IspaAPIExcept("Failed to open file '{}'", path);
            }

            std::string str;
            str.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
            return makeTokens(str);
        }

        virtual auto getErrors() -> const std::vector<std::string>& {
            return errors;
        }

        virtual void append_token(const Token& token) {
            tokens.push_back(token);
        }

        Lexer_base& operator=(const Lexer_base& other) {
            if (this != &other) {
                tokens = other.tokens;
                errors = other.errors;
                _owned_input = other._owned_input;
                _in = _owned_input.empty() ? other._in : _owned_input.c_str();
            }
            return *this;
        }

        bool operator==(const Lexer_base& other) const { return tokens == other.tokens; }
        bool operator!=(const Lexer_base& other) const { return tokens != other.tokens; }
    };

} // namespace ISPA_STD
