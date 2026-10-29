// ISPA_CORE::LLParser<MainNode, Token>: the ispastdlib parser interface whose combinators, error recovery and
// frames run in the lowered core (python::ispa_std::parser_ll). Typed iterators, tokens and nodes stay typed:
// they cross into the core through a cursor bridge and travel back inside typed node objects, not std::any.
#pragma once
#include <ispastdlib/parser_ll.hpp>
#include <ispa_core/namespace.hpp>
#include <ispa_core/lexer.hpp>
#include <memory>
#include <optional>

namespace ISPA_CORE {
namespace core_detail {
template<class T, class... A> std::shared_ptr<T> counted(A&&... a) { return ISPA_STD::make_object<T>(std::forward<A>(a)...); }

template<class T, class... Ts>
constexpr std::size_t alternative_index(const ISPA_STD::RawToken<Ts...>*) {
    constexpr bool same[] = {std::is_same_v<T, Ts>...};
    for (std::size_t i = 0; i < sizeof...(Ts); ++i) if (same[i]) return i;
    return static_cast<std::size_t>(-1);
}
template<class T, class Token>
constexpr long long token_index_of = static_cast<long long>(alternative_index<T>(static_cast<const Token*>(nullptr)));

// A core Node that carries the typed node: how a typed AST value passes through the core combinators.
// The core's contexts are generic over the AST they carry; the facade keeps its AST native.
struct NoAst {};

template<class T>
struct TypedNode : lo::node::Node {
    ISPA_STD::Node<T> value;
};

template<class T>
std::shared_ptr<TypedNode<T>> typed_node(ISPA_STD::Node<T> value, long long startpos, long long length, long long line,
                                         long long column, long long name) {
    auto node = counted<TypedNode<T>>();
    node->__init__(startpos, std::string(), false, length, line, column, name, false, std::any{});
    node->value = std::move(value);
    return node;
}
template<class T>
ISPA_STD::Node<T> take_node(const std::shared_ptr<lo::node::Node>& node) {
    if (auto typed = std::dynamic_pointer_cast<TypedNode<T>>(node)) return std::move(typed->value);
    return ISPA_STD::Node<T>{};
}

// Metadata whose text is read from the typed token's source pointers instead of a copied source string.
struct TypedMeta : lo::raw_token::RawTokenMeta {
    const char* start = nullptr;
    long long span = 0;
    auto has_source() -> bool override { return start != nullptr; }
    auto text() -> std::string override { return start ? std::string(start, static_cast<std::size_t>(span)) : std::string(); }
};

// The token at the cursor as the core's RawToken: kind, metadata, alternative index. Its to_node() makes the
// typed node of T from the typed token.
template<class Token, class T>
struct TypedRawToken : lo::raw_token::RawToken {
    Token token;
    TypedMeta meta_;  // part of the token: no separate object
    auto to_node(long long) -> std::shared_ptr<lo::node::Node> override {
        const auto& meta = token.meta();
        if constexpr (token_index_of<T, Token> == static_cast<long long>(-1)) {
            return lo::node::Node::blank();  // T is a rule, not a token alternative
        } else
        return typed_node<T>(token.template to_node<T>(), static_cast<long long>(meta.startpos()),
                             static_cast<long long>(meta.length()), static_cast<long long>(meta.line()),
                             static_cast<long long>(meta.column()), static_cast<long long>(meta.name()));
    }
};

template<class IT> using source_origin_op = decltype(std::declval<const IT&>().sourceOrigin());
template<class IT> using decrement_op = decltype(--std::declval<IT&>());
template<class IT> inline constexpr bool HasSource = ISPA_STD::detail::detected_v<source_origin_op, IT>;

// A core TokenCursor over a typed iterator.
template<class IT, class Token, class T>
struct TypedCursor : lo::lexer::TokenCursor {
    IT it;
    // current() is asked several times per position; the token object is rebuilt only after the cursor moves.
    std::shared_ptr<lo::raw_token::RawToken> cached_token;
    long long cached_distance = -1;
    explicit TypedCursor(IT value) : it(std::move(value)) {}

    auto copy() -> std::shared_ptr<lo::lexer::TokenCursor> override {
        auto copied = core_detail::counted<TypedCursor>(it);
        copied->cached_token = cached_token;
        copied->cached_distance = cached_distance;
        return copied;
    }
    auto assign(std::shared_ptr<lo::lexer::TokenCursor> other) -> void override {
        it = static_cast<TypedCursor&>(*other).it;
    }
    auto has_source() -> bool override { return HasSource<IT>; }
    auto sourceOrigin() -> std::string override {
        if constexpr (HasSource<IT>) { const char* origin = it.sourceOrigin(); return origin ? origin : ""; }
        else return {};
    }
    auto getErrors() -> std::vector<std::string> override {
        if constexpr (HasSource<IT>) return it.getErrors();
        else return {};
    }
    auto sourcePosition() -> long long override {
        if constexpr (HasSource<IT>) {
            const char* at = it.sourcePosition();
            return at ? static_cast<long long>(at - it.sourceOrigin()) : -1;
        } else return -1;
    }
    auto consumeExternal(long long end) -> void override {
        if constexpr (HasSource<IT>) it.consumeExternal(it.sourceOrigin() + end);
        else (void)end;
    }
    auto isEnd() -> bool override { return it.isEnd(); }
    auto next() -> std::shared_ptr<lo::lexer::TokenCursor> override { if (!it.isEnd()) ++it; return shared_self(); }
    auto prev() -> std::shared_ptr<lo::lexer::TokenCursor> override {
        if constexpr (ISPA_STD::detail::detected_v<decrement_op, IT>) --it;
        return shared_self();
    }
    auto advance_by(long long count) -> std::shared_ptr<lo::lexer::TokenCursor> override {
        while (count-- > 0 && !it.isEnd()) ++it;
        return shared_self();
    }
    auto plus(long long count) -> std::shared_ptr<lo::lexer::TokenCursor> override {
        auto moved = core_detail::counted<TypedCursor>(it);
        moved->advance_by(count);
        return moved;
    }
    auto current() -> std::shared_ptr<lo::raw_token::RawToken> override {
        if (cached_token && cached_distance == static_cast<long long>(it.distance())) return cached_token;
        if (it.isEnd()) return lo::raw_token::RawToken::empty_token();
        const Token& token = *it;
        const auto& meta = token.meta();
        auto raw = core_detail::counted<TypedRawToken<Token, T>>();
        // The metadata is a member of the token; the core is handed a non-owning pointer to it.
        auto& core_meta_value = raw->meta_;
        core_meta_value.start = meta.start();
        core_meta_value.span = static_cast<long long>(meta.length());
        core_meta_value.__init__(static_cast<long long>(meta.startpos()), std::string(), false,
                            static_cast<long long>(meta.length()), static_cast<long long>(meta.line()),
                            static_cast<long long>(meta.column()), static_cast<long long>(meta.name()));
        std::shared_ptr<lo::raw_token::RawTokenMeta> core_meta(std::shared_ptr<void>(), &core_meta_value);
        raw->__init__(core_meta, static_cast<long long>(token.index()));
        raw->token = token;
        cached_token = raw;
        cached_distance = static_cast<long long>(it.distance());
        return raw;
    }
    auto minus(std::shared_ptr<lo::lexer::TokenCursor> other) -> long long override {
        return static_cast<long long>(it.distance()) - static_cast<long long>(static_cast<TypedCursor&>(*other).it.distance());
    }
    auto distance() -> long long override { return static_cast<long long>(it.distance()); }
    auto resumePosition() -> long long override { return -1; }
    auto advanceCharacters(long long) -> bool override { return false; }

private:
    std::shared_ptr<lo::lexer::TokenCursor> shared_self() { return std::static_pointer_cast<lo::lexer::TokenCursor>(this->shared_from_this()); }
};

// Recovery::handler<...>() result: a core handler.
struct CoreFail {
    std::shared_ptr<lo::recovery::FailHandler> handler;
};

// frame guard that closes with scope exit (C++ RAII in ispastdlib)
struct FrameScope {
    lo::parser_ll::LLParser* parser;
    explicit FrameScope(lo::parser_ll::LLParser* p, const std::string& rule) : parser(p) { parser->enter_frame(rule); }
    FrameScope(const FrameScope&) = delete;
    FrameScope(FrameScope&& other) noexcept : parser(std::exchange(other.parser, nullptr)) {}
    ~FrameScope() { if (parser) parser->leave_frame(); }
};

template<class Set>
std::shared_ptr<lo::recovery::TokenSet> token_set() {
    const auto values = Set::values();
    return ISPA_STD::construct<lo::recovery::TokenSet>(std::vector<long long>(values.begin(), values.end()));
}

// token ids are hashes, so the names come from the generated function instead of a table
struct FnTokenNames : lo::parser_ll::TokenNames {
    const char* (*fn)(std::size_t) = nullptr;
    explicit FnTokenNames(const char* (*f)(std::size_t)) : fn(f) {}
    auto lookup(long long id) -> std::string override {
        try {
            const char* name = fn(static_cast<std::size_t>(id));
            return name ? name : "";
        } catch (...) {  // the generated switch throws for an id that is not a token
            return "";
        }
    }
};

inline lo::recovery::Decision* unused_decision = nullptr;
// custom_failure(lambda): kept typed until a combinator supplies the iterator type.
template<class Action>
struct CustomFail {
    Action action;
};
}  // namespace core_detail

namespace Recovery {
using namespace ISPA_STD::Recovery;

template<class Expected, class Continuation, class Follow, class Synchronization, class RecoveryPolicy>
auto handler(std::string_view rule, std::string_view expected_text) {
    namespace lo = core_detail::lo;
    // One core handler per call site: the sets are template arguments and the texts are literals.
    struct Entry { const char* rule; const char* text; core_detail::CoreFail fail; };
    static thread_local std::vector<Entry> cache;
    for (const auto& entry : cache)
        if (entry.rule == rule.data() && entry.text == expected_text.data()) return entry.fail;
    ISPA_STD::HeapScope heap;  // cached for the life of the program
    auto policy = ISPA_STD::construct<lo::recovery::Policy>(RecoveryPolicy::allow_insertion, RecoveryPolicy::allow_deletion,
        RecoveryPolicy::allow_replacement, static_cast<long long>(RecoveryPolicy::max_skip));
    core_detail::CoreFail made{ISPA_STD::construct<lo::recovery::Handler>(std::string(rule), std::string(expected_text),
        core_detail::token_set<Expected>(), core_detail::token_set<Continuation>(), core_detail::token_set<Follow>(),
        core_detail::token_set<Synchronization>(), policy)};
    cache.push_back({rule.data(), expected_text.data(), made});
    return made;
}
}  // namespace Recovery

template<typename MainNode, typename Token>
class LLParser : public ISPA_STD::LLParser<MainNode, Token> {
    using Base = ISPA_STD::LLParser<MainNode, Token>;
public:
    using Base::parse;
    // The core owns recovery state while a parse runs; reading it (or starting a new parse) folds it into the typed state first.
    [[nodiscard]] const std::vector<ISPA_STD::Recovery::Diagnostic>& getErrors() const {
        const_cast<LLParser*>(this)->drain_core();
        return Base::getErrors();
    }
    [[nodiscard]] const std::vector<ISPA_STD::Recovery::ContextDiagnostic>& getContextErrors() const {
        const_cast<LLParser*>(this)->drain_core();
        return Base::getContextErrors();
    }
    void reset_recovery() {
        Base::reset_recovery();
        core()->reset_recovery();
    }
    auto& core() {
        if (!core_) { ISPA_STD::HeapScope heap; core_ = ISPA_STD::construct<core_detail::lo::parser_ll::LLParser>(); }
        return core_;
    }

    // --- frames and recovery points ------------------------------------------------------------------------
    core_detail::FrameScope recovery_frame(std::string_view rule) { return core_detail::FrameScope(core().get(), std::string(rule)); }
    bool unwinding() const noexcept { return const_cast<LLParser*>(this)->core()->unwinding(); }

    template<typename... Sets, typename... Names>
    void recovery_at(std::string_view expecting, Names... names) {
        static_assert(sizeof...(Sets) == sizeof...(Names), "recovery_at: one claim name per claim set");
        // The sets are template arguments and the labels literals: build the claims once per call site.
        struct Site { const char* expecting; const char* first_label; std::vector<std::shared_ptr<core_detail::lo::recovery::ContextClaim>> claims; };
        static thread_local std::vector<Site> sites;
        const std::string_view labels[] = {std::string_view(names)..., std::string_view{}};
        for (const auto& site : sites)
            if (site.expecting == expecting.data() && site.first_label == labels[0].data()) {
                core()->recovery_at(std::string(expecting), site.claims);
                return;
            }
        ISPA_STD::HeapScope heap;
        std::vector<std::shared_ptr<core_detail::lo::recovery::ContextClaim>> claims;
        std::size_t i = 0;
        (claims.push_back(ISPA_STD::construct<core_detail::lo::recovery::ContextClaim>(std::string(labels[i++]), core_detail::token_set<Sets>())), ...);
        sites.push_back({expecting.data(), labels[0].data(), claims});
        core()->recovery_at(std::string(expecting), std::move(claims));
    }

    template<typename SpaceToken, typename IT>
    void skip_spaces(IT& pos) {
        use_space_token<SpaceToken>();
        auto cursor = bridge<SpaceToken>(pos);
        core()->skip_spaces(cursor);
        pos = cursor->it;
    }

    template<class... Sets, class IT>
    int recover_choice(IT& pos, std::string_view rule, std::string_view expected) {
        static_assert(sizeof...(Sets) % 2 == 0, "recover_choice: a FIRST set and an interior set per alternative");
        std::vector<std::shared_ptr<core_detail::lo::recovery::TokenSet>> all{core_detail::token_set<Sets>()...};
        std::vector<std::shared_ptr<core_detail::lo::recovery::TokenSet>> first, inner;
        for (std::size_t i = 0; i < all.size(); ++i) (i % 2 == 0 ? first : inner).push_back(all[i]);
        auto cursor = bridge<Token>(pos);
        const auto chosen = core()->recover_choice(cursor, std::string(rule), std::string(expected), std::move(first), std::move(inner));
        pos = cursor->it;
        return static_cast<int>(chosen);
    }

    // --- required --------------------------------------------------------------------------------------------
    template<typename NodeStruct, typename IT>
    auto required(IT& pos, const core_detail::CoreFail& fail) -> ISPA_STD::Node<NodeStruct> {
        auto cursor = bridge<NodeStruct>(pos);
        auto node = core()->required_token(cursor, core_detail::token_index_of<NodeStruct, Token>, fail.handler);
        pos = cursor->it;
        return core_detail::take_node<NodeStruct>(node);
    }
    template<typename NodeStruct, typename IT, typename Recursive>
    auto required(IT& pos, Recursive&& rec, const core_detail::CoreFail& fail) -> ISPA_STD::Node<NodeStruct> {
        auto cursor = bridge<NodeStruct>(pos);
        auto node = core()->required_rule(cursor, std::string(), rule_callback<NodeStruct, IT>(rec), fail.handler);
        pos = cursor->it;
        return core_detail::take_node<NodeStruct>(node);
    }

    // --- optional --------------------------------------------------------------------------------------------
    template<typename NodeStruct, typename IT>
    auto optional(IT& pos) -> ISPA_STD::Node<NodeStruct> {
        auto cursor = bridge<NodeStruct>(pos);
        auto node = core()->optional_token(cursor, core_detail::token_index_of<NodeStruct, Token>);
        pos = cursor->it;
        return core_detail::take_node<NodeStruct>(node);
    }
    template<typename NodeStruct, typename IT, typename Recursive>
    auto optional(IT& pos, Recursive&& rec) -> ISPA_STD::Node<NodeStruct> {
        auto cursor = bridge<NodeStruct>(pos);
        auto node = core()->optional_rule(cursor, rule_callback<NodeStruct, IT>(rec));
        pos = cursor->it;
        return core_detail::take_node<NodeStruct>(node);
    }

    // --- ranges ----------------------------------------------------------------------------------------------
    template<typename NodeStruct, typename SpaceToken = void, typename IT>
    auto range(IT& pos, const core_detail::CoreFail& fail) -> std::vector<ISPA_STD::Node<NodeStruct>> {
        use_space_token<SpaceToken>();
        auto cursor = bridge<NodeStruct>(pos);
        auto nodes = core()->range_token(cursor, core_detail::token_index_of<NodeStruct, Token>, fail.handler);
        pos = cursor->it;
        return take_nodes<NodeStruct>(nodes);
    }
    template<typename NodeStruct, typename SpaceToken = void, typename IT, typename Recursive>
    auto range(IT& pos, Recursive&& rec, const core_detail::CoreFail& fail) -> std::vector<ISPA_STD::Node<NodeStruct>> {
        use_space_token<SpaceToken>();
        auto cursor = bridge<NodeStruct>(pos);
        auto nodes = core()->range_rule(cursor, std::string(), rule_callback<NodeStruct, IT>(rec), fail.handler);
        pos = cursor->it;
        return take_nodes<NodeStruct>(nodes);
    }
    template<typename NodeStruct, typename SpaceToken = void, typename IT>
    auto range_optional(IT& pos) -> std::vector<ISPA_STD::Node<NodeStruct>> {
        use_space_token<SpaceToken>();
        auto cursor = bridge<NodeStruct>(pos);
        auto nodes = core()->range_optional_token(cursor, core_detail::token_index_of<NodeStruct, Token>);
        pos = cursor->it;
        return take_nodes<NodeStruct>(nodes);
    }
    template<typename NodeStruct, typename SpaceToken = void, typename IT, typename Recursive>
    auto range_optional(IT& pos, Recursive&& rec) -> std::vector<ISPA_STD::Node<NodeStruct>> {
        use_space_token<SpaceToken>();
        auto cursor = bridge<NodeStruct>(pos);
        auto nodes = core()->range_optional_rule(cursor, rule_callback<NodeStruct, IT>(rec));
        pos = cursor->it;
        return take_nodes<NodeStruct>(nodes);
    }


    // --- custom fail blocks (ctx in `fail` actions) ---------------------------------------------------------------
    template<class Action>
    static auto custom_failure(Action action) { return core_detail::CustomFail<Action>{std::move(action)}; }

    template<class IT>
    static auto custom_failure_decision(const IT& before, const IT& after) { return Base::custom_failure_decision(before, after); }

    // `ctx` of a fail block: reads are typed, repairs and diagnostics run in the core's FailContext.
    template<class IT, class AST>
    struct FailContext {
        IT& pos;
        AST& ast;
        const IT start_pos;
        const std::string rule;
        const std::string failed_rule;
        const char* (*token_name)(std::size_t) = nullptr;
        std::shared_ptr<core_detail::lo::parser_ll::FailContext<core_detail::NoAst>> core_context;
        std::shared_ptr<core_detail::TypedCursor<IT, Token, void>> cursor;
        std::vector<std::pair<std::string, IT>> checkpoints;

        void push() { cursor->it = pos; }
        void pull() { pos = cursor->it; }

        bool at(std::string_view spelling, std::size_t lookahead = 0) const {
            return LLParser::template context_at<IT>(pos, spelling, lookahead, token_name);
        }
        bool at_token(std::string_view name, std::size_t lookahead = 0) const {
            return LLParser::template context_at<IT>(pos, name, lookahead, token_name, true);
        }
        auto peek(std::size_t lookahead = 0) const { return LLParser::template context_peek<IT>(pos, lookahead, token_name); }
        auto take() { auto token = peek(); if (!token.empty()) ++pos; return token; }
        auto prev() { return LLParser::template context_prev<IT>(pos, start_pos, token_name); }

        void mark(const std::string& name) { checkpoints.emplace_back(name, pos); }
        void restore(const std::string& name) {
            const auto found = std::find_if(checkpoints.rbegin(), checkpoints.rend(), [&](const auto& c) { return c.first == name; });
            if (found == checkpoints.rend()) throw std::logic_error("unknown action checkpoint '" + name + "'");
            const auto index = checkpoints.size() - 1 - static_cast<std::size_t>(found - checkpoints.rbegin());
            pos = checkpoints[index].second;
            checkpoints.erase(checkpoints.begin() + static_cast<std::ptrdiff_t>(index + 1), checkpoints.end());
        }
        void commit(const std::string& name) {
            const auto found = std::find_if(checkpoints.rbegin(), checkpoints.rend(), [&](const auto& c) { return c.first == name; });
            if (found == checkpoints.rend()) throw std::logic_error("unknown action checkpoint '" + name + "'");
            checkpoints.erase(checkpoints.begin() + static_cast<std::ptrdiff_t>(checkpoints.size() - 1 - static_cast<std::size_t>(found - checkpoints.rbegin())), checkpoints.end());
        }

        void error(const std::string& message, const std::string& code = {}, std::optional<std::size_t> source_offset = {},
                   std::optional<std::size_t> source_length = {}) {
            push();
            core_context->error(message, code, source_offset ? static_cast<long long>(*source_offset) : -1,
                                source_length ? static_cast<long long>(*source_length) : -1);
            pull();
        }
        void note(const std::string& message) { core_context->note(message); }
        template<class... Boundaries>
        void sync(Boundaries... boundaries) {
            push();
            core_context->sync(std::vector<std::string>{std::string(boundaries)...});
            pull();
        }
        auto token_boundary(std::string_view name) const {
            return core_detail::lo::parser_ll::token_boundary(std::string(name));
        }
        template<class... Boundaries>
        void sync_tokens(Boundaries... boundaries) {
            push();
            auto make = [](auto&& b) {
                if constexpr (std::is_convertible_v<decltype(b), std::string_view>)
                    return core_detail::lo::parser_ll::spelling(std::string(std::string_view(b)));
                else
                    return b;
            };
            core_context->sync_tokens(std::vector<std::shared_ptr<core_detail::lo::parser_ll::TokenBoundary>>{make(boundaries)...});
            pull();
        }
        void skip() { push(); core_context->skip(); pull(); }
        void insert() { core_context->insert(); }
        void replace() { push(); core_context->replace(); pull(); }
        void retry() { core_context->retry(); }
        void abort() { core_context->abort(); }

        ISPA_STD::Recovery::Decision finish(const IT& before) {
            push();
            auto marker = core_detail::counted<core_detail::TypedCursor<IT, Token, void>>(before);
            auto decision = core_context->finish(marker);
            pull();
            ISPA_STD::Recovery::Decision out;
            out.mode = static_cast<ISPA_STD::Recovery::Mode>(decision->mode);
            out.synthesized = static_cast<ISPA_STD::Recovery::TokenId>(decision->synthesized);
            out.unexpected = static_cast<ISPA_STD::Recovery::TokenId>(decision->unexpected);
            out.skipped = static_cast<std::size_t>(decision->skipped);
            return out;
        }
    };

    template<class IT, class AST>
    auto fail_context(IT& pos, AST& ast, IT start, std::string rule, std::string failed_rule,
                      const char* (*token_name)(std::size_t) = nullptr) {
        FailContext<IT, AST> ctx{pos, ast, start, rule, failed_rule, token_name, nullptr, nullptr, {}};
        if (token_name) {
            static thread_local std::vector<std::pair<const char* (*)(std::size_t), std::shared_ptr<core_detail::FnTokenNames>>> names;
            auto found = std::find_if(names.begin(), names.end(), [&](const auto& n) { return n.first == token_name; });
            if (found == names.end()) {
                ISPA_STD::HeapScope heap;
                names.emplace_back(token_name, core_detail::counted<core_detail::FnTokenNames>(token_name));
                found = names.end() - 1;
            }
            long long ambiguous = -1;
            if constexpr (ISPA_STD::detail::detected_v<ISPA_STD::detail::ambiguous_id, Token>) ambiguous = static_cast<long long>(Token::ambiguous_id);
            core()->set_token_names(found->second, ambiguous);
        }
        ctx.cursor = core_detail::counted<core_detail::TypedCursor<IT, Token, void>>(pos);
        auto start_cursor = core_detail::counted<core_detail::TypedCursor<IT, Token, void>>(start);
        ctx.core_context = core()->fail_context(ctx.cursor, core_detail::NoAst{}, start_cursor, rule, failed_rule);
        return ctx;
    }
    template<class IT, class AST>
    static auto custom_failure_decision(const IT& before, const IT&, FailContext<IT, AST>& ctx) { return ctx.finish(before); }

    template<typename NodeStruct, typename IT, typename Action>
    auto required(IT& pos, const core_detail::CustomFail<Action>& fail) -> ISPA_STD::Node<NodeStruct> {
        return required<NodeStruct>(pos, core_detail::CoreFail{custom_handler<IT>(fail.action)});
    }
    template<typename NodeStruct, typename IT, typename Recursive, typename Action>
    auto required(IT& pos, Recursive&& rec, const core_detail::CustomFail<Action>& fail) -> ISPA_STD::Node<NodeStruct> {
        return required<NodeStruct>(pos, std::forward<Recursive>(rec), core_detail::CoreFail{custom_handler<IT>(fail.action)});
    }

    template<typename NodeStruct, typename SpaceToken = void, typename IT, typename Action>
    auto range(IT& pos, const core_detail::CustomFail<Action>& fail) -> std::vector<ISPA_STD::Node<NodeStruct>> {
        return range<NodeStruct, SpaceToken>(pos, core_detail::CoreFail{custom_handler<IT>(fail.action)});
    }
    template<typename NodeStruct, typename SpaceToken = void, typename IT, typename Recursive, typename Action>
    auto range(IT& pos, Recursive&& rec, const core_detail::CustomFail<Action>& fail) -> std::vector<ISPA_STD::Node<NodeStruct>> {
        return range<NodeStruct, SpaceToken>(pos, std::forward<Recursive>(rec), core_detail::CoreFail{custom_handler<IT>(fail.action)});
    }

    // --- rule actions (ctx in advanced rule bodies): pure cursor queries, no recovery state, so the native context is used as is ---
    using Base::rule_context;

    // --- counted repetitions: the counters live in the core, so every count is shared with its recovery state ----
    struct CounterScope {
        std::shared_ptr<core_detail::lo::parser_ll::CounterScope> scope;
        explicit CounterScope(std::shared_ptr<core_detail::lo::parser_ll::CounterScope> scope) : scope(std::move(scope)) {}
        CounterScope(const CounterScope&) = delete;
        CounterScope& operator=(const CounterScope&) = delete;
        ~CounterScope() { scope->restore(); }
    };
    CounterScope counter_scope() { return CounterScope(core()->counter_scope()); }

    template<class Value>
    void bind_counter(std::string_view name, const Value& value) {
        const auto count = [&]() -> std::size_t {
            if constexpr (ISPA_STD::detail::detected_v<ISPA_STD::detail::size, Value>) return value.size();
            else return value.empty() ? 0 : 1;
        }();
        core()->bind_counter_count(std::string(name), static_cast<long long>(count));
    }

    static auto core_spec(const ISPA_STD::CountSpec& spec) {
        auto out = ISPA_STD::construct<core_detail::lo::counter::CountSpec>();
        out->__init__(spec.name, spec.count.has_value(), static_cast<long long>(spec.count.value_or(0)),
                      static_cast<long long>(spec.minimum));
        return out;
    }
    template<typename NodeStruct, typename SpaceToken = void, typename IT>
    auto counted(IT& pos, ISPA_STD::CountSpec spec) -> std::vector<ISPA_STD::Node<NodeStruct>> {
        use_space_token<SpaceToken>();
        auto cursor = bridge<NodeStruct>(pos);
        auto nodes = core()->counted_token(cursor, core_detail::token_index_of<NodeStruct, Token>, core_spec(spec));
        pos = cursor->it;
        return take_nodes<NodeStruct>(nodes);
    }
    template<typename NodeStruct, typename SpaceToken = void, typename IT, typename Recursive>
    auto counted(IT& pos, ISPA_STD::CountSpec spec, Recursive&& rec) -> std::vector<ISPA_STD::Node<NodeStruct>> {
        use_space_token<SpaceToken>();
        auto cursor = bridge<NodeStruct>(pos);
        auto nodes = core()->counted_rule(cursor, rule_callback<NodeStruct, IT>(rec), core_spec(spec));
        pos = cursor->it;
        return take_nodes<NodeStruct>(nodes);
    }

    // --- parse entry points: run, then merge the core's diagnostics into getErrors() ---------------------------------
    MainNode& parse() override {
        ISPA_STD::ArenaScope arena;
        core()->reset_recovery();
        struct Merge { LLParser& parser; ~Merge() { parser.merge_diagnostics(); parser.core()->reset_recovery(); } } merge{*this};
        return Base::parse();
    }
    MainNode& parseLazy() override {
        ISPA_STD::ArenaScope arena;
        core()->reset_recovery();
        struct Merge { LLParser& parser; ~Merge() { parser.merge_diagnostics(); parser.core()->reset_recovery(); } } merge{*this};
        return Base::parseLazy();
    }

protected:
    std::shared_ptr<core_detail::lo::parser_ll::LLParser> core_;

    // A typed fail action as the core's ActionFailure.
    template<typename IT, typename Action>
    std::shared_ptr<core_detail::lo::recovery::FailHandler> custom_handler(Action& action) {
        namespace lo = core_detail::lo;
        std::function<std::shared_ptr<lo::recovery::Decision>(std::shared_ptr<lo::lexer::TokenCursor>, std::shared_ptr<lo::recovery::State>)> run =
            [&action](std::shared_ptr<lo::lexer::TokenCursor> base, std::shared_ptr<lo::recovery::State>) {
                auto& cursor = static_cast<core_detail::TypedCursor<IT, Token, void>&>(*base);
                static ISPA_STD::Recovery::State unused;  // the typed actions do not read the state they are handed
                auto decision = action(cursor.it, unused);
                return ISPA_STD::construct<lo::recovery::Decision>(static_cast<long long>(decision.mode),
                    static_cast<long long>(decision.synthesized), static_cast<long long>(decision.unexpected),
                    static_cast<long long>(decision.skipped));
            };
        return core()->custom_failure(run);
    }

    template<typename T, typename IT>
    auto bridge(const IT& pos) {
        return core_detail::counted<core_detail::TypedCursor<IT, Token, T>>(pos);
    }

    template<typename SpaceToken>
    void use_space_token() {
        if constexpr (!std::is_void_v<SpaceToken>) core()->set_space_token(core_detail::token_index_of<SpaceToken, Token>);
    }

    template<typename NodeStruct>
    static std::vector<ISPA_STD::Node<NodeStruct>> take_nodes(const std::vector<std::shared_ptr<core_detail::lo::node::Node>>& nodes) {
        std::vector<ISPA_STD::Node<NodeStruct>> out;
        out.reserve(nodes.size());
        for (const auto& node : nodes) out.push_back(core_detail::take_node<NodeStruct>(node));
        return out;
    }

    // A typed rule as the core's callback. The typed match travels back inside a TypedNode.
    template<typename NodeStruct, typename IT, typename Recursive>
    static std::function<std::shared_ptr<core_detail::lo::match_result::MatchResult>(std::shared_ptr<core_detail::lo::lexer::TokenCursor>)>
    rule_callback(Recursive& rec) {
        namespace lo = core_detail::lo;
        return [&rec](std::shared_ptr<lo::lexer::TokenCursor> base) -> std::shared_ptr<lo::match_result::MatchResult> {
            auto& cursor = static_cast<core_detail::TypedCursor<IT, Token, NodeStruct>&>(*base);
            try {
                auto match = rec(cursor.it);
                auto out_cursor = core_detail::counted<core_detail::TypedCursor<IT, Token, NodeStruct>>(match.it);
                auto node = match.status ? core_detail::typed_node<NodeStruct>(std::move(match.node), 0, 0, 0, 0, 0)
                                         : std::shared_ptr<core_detail::TypedNode<NodeStruct>>();
                auto result = core_detail::counted<lo::match_result::MatchResult>();
                result->__init__(match.status, node ? std::static_pointer_cast<lo::node::Node>(node) : lo::node::Node::blank(), out_cursor);
                return result;
            } catch (const typename Base::NoMatch&) {
                ISPA_STD::fail(ISPA_STD::EK_NoMatch, "no match");
            }
        };
    }

    void drain_core() {
        if (core()->getErrors().empty() && core()->getContextErrors().empty()) return;
        merge_diagnostics();
        core()->reset_recovery();
    }

    void merge_diagnostics() {
        for (const auto& d : core()->getContextErrors()) {
            ISPA_STD::Recovery::ContextDiagnostic out;
            out.kind = static_cast<ISPA_STD::Recovery::ContextKind>(d->kind);
            out.message = d->message;
            out.position = static_cast<std::size_t>(d->position);
            out.offset = d->offset < 0 ? std::numeric_limits<std::size_t>::max() : static_cast<std::size_t>(d->offset);
            out.length = static_cast<std::size_t>(d->length);
            out.skipped = static_cast<std::size_t>(d->skipped);
            out.rule = d->rule;
            out.resume_rule = d->resume_rule;
            out.claim = d->claim;
            this->context_errors_.push_back(std::move(out));
        }
        auto& diagnostics = this->recovery_state_.diagnostics;
        std::vector<ISPA_STD::Recovery::Diagnostic> merged;
        for (const auto& d : core()->getErrors()) {
            ISPA_STD::Recovery::Diagnostic out;
            out.rule = d->rule;
            out.unexpected = static_cast<ISPA_STD::Recovery::TokenId>(d->unexpected);
            for (auto id : d->expected) out.expected.push_back(static_cast<ISPA_STD::Recovery::TokenId>(id));
            out.decision.mode = static_cast<ISPA_STD::Recovery::Mode>(d->decision->mode);
            out.decision.synthesized = static_cast<ISPA_STD::Recovery::TokenId>(d->decision->synthesized);
            out.decision.unexpected = static_cast<ISPA_STD::Recovery::TokenId>(d->decision->unexpected);
            out.decision.skipped = static_cast<std::size_t>(d->decision->skipped);
            out.expected_text = d->expected_text;
            out.offset = d->offset < 0 ? std::numeric_limits<std::size_t>::max() : static_cast<std::size_t>(d->offset);
            out.length = static_cast<std::size_t>(d->length);
            out.message = d->message;
            out.code = d->code;
            out.notes = d->notes;
            merged.push_back(std::move(out));
        }
        merged.insert(merged.end(), std::make_move_iterator(diagnostics.begin()), std::make_move_iterator(diagnostics.end()));
        diagnostics = std::move(merged);
    }
};
}
