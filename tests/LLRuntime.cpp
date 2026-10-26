#include <ispastdlib/parser_ll.hpp>
#include <ispastdlib/external.hpp>
#include <gtest/gtest.h>

namespace {
struct Leaf {};
struct Other {};
struct Space {};
using Token = ISPA_STD::RawToken<Leaf, Other, Space>;
struct Iter {
    const Token* tokens;
    std::size_t index = 0;
    std::size_t distance() const { return index; }
    bool isEnd() const { return tokens[index].empty(); }
    const Token& operator*() const { return tokens[index]; }
    const Token* operator->() const { return &tokens[index]; }
    Iter& operator++() { ++index; return *this; }
    Iter& operator--() { if (index) --index; return *this; }
};
struct Parser : ISPA_STD::LLParser<Leaf, Token> {
    using LLParser::optional;
    using LLParser::required;
    using LLParser::range;
    using LLParser::range_optional;
    using LLParser::counted;
    using LLParser::counter_scope;
    using LLParser::bind_counter;
    int failures = 0;
    void parseFromTokens() override {}
    auto lazyParse() -> ISPA_STD::MatchResult<Leaf, ISPA_STD::Lexer_base<Token>::lazy_iterator> override { return {true, {}, {}}; }
    auto leaf(Iter pos) -> ISPA_STD::MatchResult<Leaf, Iter> {
        auto node = required<Leaf>(pos, [this](Iter&) { ++failures; });
        return {true, node, pos};
    }
    auto pair(Iter pos) -> ISPA_STD::MatchResult<Leaf, Iter> {
        leafToken(pos);
        auto node = leafToken(pos);
        return {true, node, pos};
    }
    auto leafToken(Iter& pos) -> ISPA_STD::Node<Leaf> {
        return required<Leaf>(pos, [this](Iter&) { ++failures; });
    }
};

struct RecoveryParser : Parser {
    const Token* input = nullptr;
    auto lazyParse() -> ISPA_STD::MatchResult<Leaf, ISPA_STD::Lexer_base<Token>::lazy_iterator> override {
        Iter pos{input};
        required<Leaf>(pos, [](Iter& cursor, ISPA_STD::Recovery::State& state) {
            const ISPA_STD::Recovery::Plan plan{
                .rule = "entry", .expected = {1},
                .allow_insertion = false, .allow_replacement = false
            };
            return ISPA_STD::Recovery::recover(cursor, state, plan, [](const Token& token) {
                return token.holds<Leaf>() ? 1u : 2u;
            });
        });
        return {true, {}, {}};
    }
    void run(const Token* tokens) {
        input = tokens;
        parse("test input");
    }
    auto skipped() const { return recovery_state_.total_skipped; }
    void setErrorBudget(std::size_t limit) { recovery_state_.max_errors = limit; }
    auto errorBudget() const { return recovery_state_.max_errors; }
};

struct AnchorParser : Parser {
    using LLParser::rule_context;
    using LLParser::recovery_frame;
    using LLParser::recovery_at;
    using LLParser::recovery_state_;
    using LLParser::fail_context;
    using LLParser::required_rule_recovering;
    using LLParser::custom_failure;
};

TEST(LLRuntime, ActionLookaheadReturnsOwnedTokenAndRestoresWithinRule) {
    const char source[] = "ab";
    const Token tokens[]{
        Token{{0, source, source + 1, 1, 1, 1, 1}, Leaf{}},
        Token{{1, source + 1, source + 2, 1, 1, 2, 2}, Other{}},
        Token{}
    };
    Iter pos{tokens};
    AnchorParser parser;
    Leaf ast;
    auto ctx = parser.rule_context(pos, ast, pos, "item",
        +[](std::size_t kind) -> const char* { return kind == 1 ? "A" : "B"; });
    const auto ahead = ctx.peek(1);
    EXPECT_EQ(ahead.text, "b");
    EXPECT_EQ(ahead.name, "B");
    EXPECT_EQ(ahead.offset, 1u);
    EXPECT_TRUE(ahead.raw.holds<Other>());
    EXPECT_EQ(pos.index, 0u);
    ctx.mark("choice");
    const auto first = ctx.take();
    EXPECT_EQ(first.text, "a");
    EXPECT_EQ(pos.index, 1u);
    const auto previous = ctx.prev();
    EXPECT_EQ(previous.text, "a");
    EXPECT_EQ(pos.index, 0u);
    EXPECT_TRUE(ctx.prev().empty());
    EXPECT_EQ(pos.index, 0u);
    ctx.take();
    ctx.restore("choice");
    EXPECT_EQ(pos.index, 0u);
    EXPECT_EQ(first.text, "a");
    ctx.commit("choice");
    EXPECT_TRUE(ctx.peek(2).empty());
    EXPECT_THROW(ctx.peek(65), std::out_of_range);
}
struct HandoffLexer : ISPA_STD::Lexer_base<Token> {
    std::size_t scanned = 0;
    Token makeToken(const char*& p) override {
        const char* start = p++;
        ++scanned;
        ISPA_STD::RawTokenMeta meta{getCurrentPos(start), start, p, 1, 1, 1, *start == 'a' ? 1u : 2u};
        if (*start == 'a') return Token{meta, Leaf{}};
        return Token{meta, Other{}};
    }
    auto clone() const -> std::shared_ptr<ISPA_STD::Lexer_base<Token>> override { return std::make_shared<HandoffLexer>(*this); }
    std::any checkpoint() const override { return scanned; }
    void restoreCheckpoint(const std::any& state) override { scanned = std::any_cast<std::size_t>(state); }
};
struct ExternalLeaf {
    using node_type = Leaf;
    static bool starts(const char*, const char* start) { return *start == 'a'; }
    static ISPA_STD::ExternalResult<Leaf> parse(const char* origin, const char* start) {
        const char* end = start;
        while (*end == 'a') ++end;
        ISPA_STD::Node<Leaf> node;
        node.set_data(Leaf{});
        return {true, ISPA_STD::external_node(origin, start, end, std::move(node)), end};
    }
};
}

TEST(LLRuntime, LazyFailContextPrevRewindsOneTokenWithinRule) {
    HandoffLexer lexer;
    const char* source = "ab";
    HandoffLexer::lazy_iterator pos(lexer, source);
    AnchorParser parser;
    Leaf ast;
    const auto start = pos;
    ++pos;
    auto ctx = parser.fail_context(pos, ast, start, "item", "B",
        +[](std::size_t kind) -> const char* { return kind == 1 ? "A" : "B"; });
    EXPECT_EQ(ctx.prev().text, "a");
    EXPECT_EQ(pos.distance(), start.distance());
    EXPECT_TRUE(ctx.prev().empty());
    EXPECT_EQ(pos.distance(), start.distance());
}

TEST(LLRuntime, PanicRetriesExpectedTerminalBeyondOneTokenLookahead) {
    const Token tokens[]{Token{{}, Other{}}, Token{{}, Other{}},
                         Token{{}, Leaf{}}, Token{}};
    Iter pos{tokens};
    Parser parser;
    ISPA_STD::Recovery::State state;
    const auto fail = [&](Iter& cursor, ISPA_STD::Recovery::State&) {
        return ISPA_STD::Recovery::recover(cursor, state, {
            .rule = "entry", .expected = {1},
            .allow_insertion = false, .allow_replacement = false
        }, [](const Token& token) { return token.holds<Leaf>() ? 1u : 2u; });
    };
    EXPECT_FALSE(parser.required<Leaf>(pos, fail).empty());
    EXPECT_EQ(pos.index, 3u);
    ASSERT_EQ(state.diagnostics.size(), 1u);
    EXPECT_EQ(state.diagnostics.front().decision.skipped, 2u);
}

TEST(LLRuntime, NestedRecoveryPreservesEnclosingProductionAnchor) {
    const Token tokens[]{Token{{0, nullptr, nullptr, 0, 0, 0, 2}, Other{}},
                         Token{{0, nullptr, nullptr, 0, 0, 0, 2}, Other{}},
                         Token{{0, nullptr, nullptr, 0, 0, 0, 3}, Space{}}, Token{}};
    Iter pos{tokens};
    AnchorParser parser;
    auto outer = parser.recovery_frame("outer");
    parser.recovery_at<ISPA_STD::Recovery::Tokens<3>>("nested", "later_nested_terminal");
    {
        auto inner = parser.recovery_frame("inner");
        parser.recovery_at<ISPA_STD::Recovery::Tokens<1>>("Leaf", "Leaf");
        auto fail = ISPA_STD::Recovery::handler<ISPA_STD::Recovery::Tokens<1>,
            ISPA_STD::Recovery::EmptyTokens, ISPA_STD::Recovery::EmptyTokens,
            ISPA_STD::Recovery::EmptyTokens, ISPA_STD::Recovery::Policy<false, false, false>>(
                "inner", "Leaf");
        EXPECT_TRUE(parser.required<Leaf>(pos, fail).empty());
        EXPECT_EQ(pos.index, 2u);
        ASSERT_EQ(parser.getErrors().size(), 1u);
        EXPECT_NE(parser.getErrors().front().message.find("expected Leaf"), std::string::npos);
    }
    EXPECT_EQ(parser.recovery_state_.anchors.size(), 1u);
}

TEST(LLRuntime, GetErrorsExposesRecoveredDiagnosticsThroughConstParser) {
    const Token tokens[]{Token{{}, Other{}}, Token{{}, Leaf{}}, Token{}};
    RecoveryParser parser;
    EXPECT_TRUE(parser.getErrors().empty());
    parser.run(tokens);
    const auto& errors = std::as_const(parser).getErrors();
    ASSERT_EQ(errors.size(), 1u);
    EXPECT_EQ(errors.front().rule, "entry");
    EXPECT_EQ(errors.front().unexpected, 2u);
    EXPECT_EQ(errors.front().expected, (std::vector<ISPA_STD::Recovery::TokenId>{1}));
    EXPECT_EQ(errors.front().decision.mode, ISPA_STD::Recovery::Mode::Delete);
    EXPECT_EQ(parser.skipped(), 1u);
}

TEST(LLRuntime, ErrorsSurviveExceptionsAndResetBeforeEachParse) {
    const Token bad[]{Token{{}, Other{}}, Token{}};
    const Token good[]{Token{{}, Leaf{}}, Token{}};
    RecoveryParser parser;
    parser.setErrorBudget(7);
    EXPECT_THROW(parser.run(bad), std::logic_error);
    ASSERT_EQ(parser.getErrors().size(), 1u);
    EXPECT_EQ(parser.skipped(), 1u);
    EXPECT_THROW(parser.run(bad), std::logic_error);
    EXPECT_EQ(parser.getErrors().size(), 1u);
    EXPECT_EQ(parser.skipped(), 1u);
    parser.run(good);
    EXPECT_TRUE(parser.getErrors().empty());
    EXPECT_EQ(parser.skipped(), 0u);
    EXPECT_EQ(parser.errorBudget(), 7u);
}

TEST(LLRuntime, AbsentOptionalDescendantDoesNotRecoverOrAdvance) {
    const Token tokens[]{Token{{}, Other{}}, Token{}};
    Iter pos{tokens};
    Parser parser;
    EXPECT_TRUE(parser.optional<Leaf>(pos, [&](Iter cursor) { return parser.leaf(cursor); }).empty());
    EXPECT_EQ(pos.index, 0);
    EXPECT_EQ(parser.failures, 0);
    EXPECT_THROW(parser.required<Leaf>(pos, [&](Iter&) { ++parser.failures; }), std::logic_error);
    EXPECT_EQ(parser.failures, 1);
}

TEST(LLRuntime, CommittedOptionalFailureStillRecoversAndRestoresProbeState) {
    const Token tokens[]{Token{{}, Leaf{}}, Token{{}, Other{}}, Token{}};
    Iter pos{tokens};
    Parser parser;
    EXPECT_THROW(parser.optional<Leaf>(pos, [&](Iter cursor) { return parser.pair(cursor); }), std::logic_error);
    EXPECT_EQ(parser.failures, 1);
    ++pos;
    EXPECT_THROW(parser.required<Leaf>(pos, [&](Iter&) { ++parser.failures; }), std::logic_error);
    EXPECT_EQ(parser.failures, 2);
}

TEST(LLRuntime, NullableRuleDoesNotRepeatWithoutProgress) {
    const Token tokens[]{Token{{}, Other{}}, Token{}};
    Iter pos{tokens};
    Parser parser;
    auto empty = [](Iter cursor) -> ISPA_STD::MatchResult<Leaf, Iter> {
        ISPA_STD::Node<Leaf> node;
        node.set_data(Leaf{});
        return {true, node, cursor};
    };
    EXPECT_EQ(parser.range<Leaf>(pos, empty, [](Iter&) {}).size(), 1);
    EXPECT_TRUE(parser.range_optional<Leaf>(pos, empty).empty());
    EXPECT_EQ(pos.index, 0);
}

TEST(LLRuntime, CommittedUnsuccessfulResultIsNotTreatedAsAbsent) {
    const Token tokens[]{Token{{}, Leaf{}}, Token{{}, Other{}}, Token{}};
    Iter pos{tokens};
    Parser parser;
    auto incomplete = [&](Iter cursor) -> ISPA_STD::MatchResult<Leaf, Iter> {
        parser.leafToken(cursor);
        return {false, {}, cursor};
    };
    EXPECT_THROW(parser.optional<Leaf>(pos, incomplete), std::logic_error);
}

TEST(LLRuntime, RepetitionsSkipSeparatorsOnlyForSuccessfulIterations) {
    const Token tokens[]{Token{{}, Leaf{}}, Token{{}, Space{}}, Token{{}, Leaf{}},
                         Token{{}, Space{}}, Token{{}, Other{}}, Token{}};
    for (const bool required : {false, true}) {
        for (const bool recursive : {false, true}) {
            Iter pos{tokens};
            Parser parser;
            auto rec = [&](Iter cursor) { return parser.leaf(cursor); };
            auto fail = [&](Iter&) { ++parser.failures; };
            const auto nodes = recursive
                ? (required ? parser.range<Leaf, Space>(pos, rec, fail)
                            : parser.range_optional<Leaf, Space>(pos, rec))
                : (required ? parser.range<Leaf, Space>(pos, fail)
                            : parser.range_optional<Leaf, Space>(pos));
            EXPECT_EQ(nodes.size(), 2u);
            EXPECT_EQ(pos.index, 3u); // trailing space still belongs to the parent
            EXPECT_EQ(parser.failures, 0);
        }
    }
}

TEST(LLRuntime, RepetitionPreservesInitialAndExplicitNoSpaceBoundaries) {
    const Token tokens[]{Token{{}, Space{}}, Token{{}, Leaf{}},
                         Token{{}, Space{}}, Token{{}, Leaf{}}, Token{}};
    Iter pos{tokens};
    Parser parser;
    auto initial = parser.range_optional<Leaf, Space>(pos);
    EXPECT_TRUE(initial.empty());
    EXPECT_EQ(pos.index, 0u);
    ++pos;
    auto adjacent = parser.range<Leaf>(pos, [](Iter&) {});
    EXPECT_EQ(adjacent.size(), 1u);
    EXPECT_EQ(pos.index, 2u);
}

TEST(LLRuntime, FixedCountsMeasureTokensAndLeaveTheNextToken) {
    EXPECT_THROW(ISPA_STD::CountSpec::fixed(-1), std::invalid_argument);
    const Token tokens[]{Token{{}, Leaf{}}, Token{{}, Leaf{}}, Token{{}, Leaf{}}, Token{}};
    Parser parser;
    Iter pos{tokens};
    EXPECT_EQ(parser.counted<Leaf>(pos, ISPA_STD::CountSpec::fixed(2)).size(), 2u);
    EXPECT_EQ(pos.index, 2u);
    EXPECT_TRUE(parser.counted<Leaf>(pos, ISPA_STD::CountSpec::fixed(0)).empty());
    EXPECT_EQ(pos.index, 2u);
    EXPECT_THROW(parser.counted<Leaf>(pos, ISPA_STD::CountSpec::fixed(2)), std::logic_error);
    EXPECT_EQ(pos.index, 2u);
}

TEST(LLRuntime, NamedCountsBindOnceAndAreInheritedByProductions) {
    const Token tokens[]{Token{{}, Leaf{}}, Token{{}, Leaf{}}, Token{{}, Other{}},
                         Token{{}, Leaf{}}, Token{{}, Leaf{}}, Token{{}, Leaf{}}, Token{}};
    Parser parser;
    Iter pos{tokens};
    auto scope = parser.counter_scope();
    EXPECT_EQ(parser.counted<Leaf>(pos, ISPA_STD::CountSpec::named("x")).size(), 2u);
    ++pos;
    {
        auto child = parser.counter_scope();
        EXPECT_EQ(parser.counted<Leaf>(pos, ISPA_STD::CountSpec::named("x"),
            [&](Iter cursor) { return parser.leaf(cursor); }).size(), 2u);
    }
    EXPECT_EQ(pos.index, 5u);
    EXPECT_THROW(parser.counted<Leaf>(pos, ISPA_STD::CountSpec::named("x")), std::logic_error);
}

TEST(LLRuntime, ProductionCountsMeasureInvocationsRatherThanConsumedTokens) {
    const Token tokens[]{Token{{}, Leaf{}}, Token{{}, Leaf{}}, Token{{}, Leaf{}},
                         Token{{}, Leaf{}}, Token{{}, Leaf{}}, Token{}};
    Parser parser;
    Iter pos{tokens};
    EXPECT_EQ(parser.counted<Leaf>(pos, ISPA_STD::CountSpec::fixed(2),
        [&](Iter cursor) { return parser.pair(cursor); }).size(), 2u);
    EXPECT_EQ(pos.index, 4u);
}

TEST(LLRuntime, CounterBindingsRestoreOnRuleExitAndExceptions) {
    const Token tokens[]{Token{{}, Leaf{}}, Token{{}, Other{}},
                         Token{{}, Leaf{}}, Token{{}, Leaf{}}, Token{}};
    Parser parser;
    Iter pos{tokens};
    try {
        auto scope = parser.counter_scope();
        EXPECT_EQ(parser.counted<Leaf>(pos, ISPA_STD::CountSpec::named("x")).size(), 1u);
        throw std::runtime_error("leave rule");
    } catch (const std::runtime_error&) {}
    ++pos;
    auto scope = parser.counter_scope();
    EXPECT_EQ(parser.counted<Leaf>(pos, ISPA_STD::CountSpec::named("x")).size(), 2u);
    EXPECT_EQ(pos.index, 4u);
}

TEST(LLRuntime, CounterSeparatorsBelongOnlyToSuccessfulIterations) {
    const Token tokens[]{Token{{}, Leaf{}}, Token{{}, Space{}}, Token{{}, Leaf{}},
                         Token{{}, Space{}}, Token{{}, Other{}}, Token{}};
    Parser parser;
    Iter pos{tokens};
    EXPECT_EQ((parser.counted<Leaf, Space>(pos, ISPA_STD::CountSpec::named("x")).size()), 2u);
    EXPECT_EQ(pos.index, 3u);
}

TEST(LLRuntime, UnboundCountsRejectNullableLoopsAndKnownCountsAllowThem) {
    const Token tokens[]{Token{{}, Other{}}, Token{}};
    Parser parser;
    Iter pos{tokens};
    auto empty = [](Iter cursor) -> ISPA_STD::MatchResult<Leaf, Iter> {
        ISPA_STD::Node<Leaf> node;
        node.set_data(Leaf{});
        return {true, node, cursor};
    };
    EXPECT_THROW(parser.counted<Leaf>(pos, ISPA_STD::CountSpec::named("x"), empty), std::logic_error);
    EXPECT_EQ(parser.counted<Leaf>(pos, ISPA_STD::CountSpec::fixed(3), empty).size(), 3u);
    EXPECT_EQ(pos.index, 0u);
}

TEST(LLRuntime, CounterRegistrationUsesSuccessfulMatches) {
    const Token tokens[]{Token{{}, Leaf{}}, Token{{}, Leaf{}}, Token{{}, Other{}},
                         Token{{}, Space{}}, Token{{}, Space{}}, Token{{}, Space{}}, Token{}};
    Parser parser;
    Iter pos{tokens};
    const auto nodes = parser.range_optional<Leaf>(pos);
    parser.bind_counter("x", nodes);
    ++pos;
    EXPECT_EQ(parser.counted<Space>(pos, ISPA_STD::CountSpec::named("x")).size(), 2u);
    EXPECT_EQ(pos.index, 5u);
}

TEST(LLRuntime, ExternalHandoffPreservesPrefetchedAndFailedProbeCursors) {
    HandoffLexer lexer;
    const char* source = "aa!";
    HandoffLexer::lazy_iterator original(lexer, source);
    auto lookahead = original;
    ++lookahead; ++lookahead;
    EXPECT_EQ(lookahead->kind(), 2u);
    const auto result = ISPA_STD::external_match<ExternalLeaf>(original);
    ASSERT_TRUE(result.status);
    EXPECT_EQ(result.it.sourcePosition(), source + 2);
    EXPECT_EQ(result.it->kind(), 2u);
    EXPECT_EQ(original.sourcePosition(), source);
    EXPECT_EQ(original->kind(), 1u);
    EXPECT_GT(result.it.distance(), original.distance());
    EXPECT_EQ(lookahead.sourcePosition(), source + 2);
}

TEST(LLRuntime, FailContextMatchesTokenNamesAndLiteralSpellings) {
    const char source[] = "abc";
    const Token tokens[]{Token{{0, source, source + 3, 3, 1, 1, 1}, Leaf{}}, Token{}};
    Iter pos{tokens};
    AnchorParser parser;
    Leaf ast;
    auto ctx = parser.fail_context(pos, ast, pos, "owner", "expected",
        +[](std::size_t) -> const char* { return "modules.common.ID"; });
    EXPECT_TRUE(ctx.at("abc"));
    EXPECT_TRUE(ctx.at_token("ID"));
    EXPECT_TRUE(ctx.at_token("common.ID"));
    EXPECT_TRUE(ctx.at_token("modules::common::ID"));
    EXPECT_FALSE(ctx.at("ID"));
    EXPECT_FALSE(ctx.at("OTHER"));
    EXPECT_FALSE(ctx.at("D"));
    EXPECT_EQ(ctx.peek().text, "abc");
    EXPECT_TRUE(ctx.peek().matches_name("ID"));
    EXPECT_TRUE(ctx.peek().matches_name("common.ID"));
    EXPECT_FALSE(ctx.peek().matches_name("D"));
    ctx.mark("retry");
    EXPECT_EQ(ctx.take().name, "modules.common.ID");
    EXPECT_FALSE(ctx.at("ID"));
    EXPECT_EQ(ctx.prev().text, "abc");
    EXPECT_TRUE(ctx.prev().empty());
    EXPECT_TRUE(ctx.at_token("ID"));
    ctx.restore("retry");
    ctx.commit("retry");
    EXPECT_TRUE(ctx.at_token("ID"));
    ++pos;
    EXPECT_FALSE(ctx.at_token("ID"));
}

TEST(LLRuntime, FailSyncSkipsPythonContainersStringsAndCommentsWithoutLexing) {
    for (const char* source : {"f({'k': '};'}, +)}tail", "f(\"\"\"};\"\"\", +)}tail",
                               "f(1, # };\n +)}tail", "f([1, 2], +)}tail"}) {
        SCOPED_TRACE(source);
        HandoffLexer lexer;
        HandoffLexer::lazy_iterator pos(lexer, source);
        AnchorParser parser;
        Leaf ast;
        const auto before = pos;
        auto ctx = parser.fail_context(pos, ast, pos, "owner", "expr");
        ctx.error("broken expression");
        ctx.sync("}", ";");
        ctx.insert();
        const auto decision = ctx.finish(before);
        EXPECT_EQ(std::string_view(pos.sourcePosition()), "}tail");
        EXPECT_EQ(decision.mode, ISPA_STD::Recovery::Mode::Replace);
        EXPECT_EQ(decision.skipped, static_cast<std::size_t>(pos.sourcePosition() - source));
        EXPECT_EQ(parser.recovery_state_.total_skipped, decision.skipped);
        ASSERT_EQ(parser.getErrors().size(), 1u);
        EXPECT_EQ(parser.getErrors()[0].offset, 0u);
        EXPECT_EQ(lexer.scanned, 0u);
        EXPECT_EQ(before.sourcePosition(), source);
    }
}

TEST(LLRuntime, FailSyncPreservesHardBoundariesAndBoundsBrokenStrings) {
    for (const char* source : {"f( + ;next: 'n';", "f( + }next", "f( + )]next"}) {
        HandoffLexer lexer;
        HandoffLexer::lazy_iterator pos(lexer, source);
        AnchorParser parser;
        Leaf ast;
        auto ctx = parser.fail_context(pos, ast, pos, "owner", "expr");
        ctx.sync("}", ";");
        EXPECT_TRUE(*pos.sourcePosition() == ';' || *pos.sourcePosition() == '}' || *pos.sourcePosition() == ']');
    }
    for (const char* source : {"'unterminated\nnext", "\"\"\"unterminated", "long_payload"}) {
        HandoffLexer lexer;
        HandoffLexer::lazy_iterator pos(lexer, source);
        AnchorParser parser;
        parser.recovery_state_.max_total_skipped = 4;
        Leaf ast;
        const auto before = pos;
        auto ctx = parser.fail_context(pos, ast, pos, "owner", "expr");
        ctx.sync("}");
        ctx.insert();
        EXPECT_EQ(ctx.finish(before).mode, ISPA_STD::Recovery::Mode::Abort);
        EXPECT_LE(parser.recovery_state_.total_skipped, 4u);
    }
}

TEST(LLRuntime, FailSyncTokensPreservesBoundaryAndStopsAtEof) {
    const char source[] = "xx;";
    const Token tokens[]{Token{{0, source, source + 1, 1, 1, 1, 1}, Leaf{}},
                         Token{{1, source + 1, source + 2, 1, 1, 2, 1}, Leaf{}},
                         Token{{2, source + 2, source + 3, 1, 1, 3, 2}, Other{}}, Token{}};
    for (const bool named : {false, true}) {
        Iter pos{tokens};
        AnchorParser parser;
        Leaf ast;
        const auto before = pos;
        auto ctx = parser.fail_context(pos, ast, pos, "owner", "expression",
            +[](std::size_t id) -> const char* { return id == 2 ? "python.DEDENT" : "python.ID"; });
        ctx.error("missing expression");
        if (named) ctx.sync_tokens(ctx.token_boundary("NEWLINE"), ctx.token_boundary("DEDENT"));
        else ctx.sync_tokens(";");
        EXPECT_EQ(pos.index, 2u);
        ctx.insert();
        EXPECT_EQ(ctx.finish(before).mode, ISPA_STD::Recovery::Mode::Replace);
        EXPECT_EQ(parser.recovery_state_.total_skipped, 2u);
        ctx.sync_tokens("absent");
        EXPECT_TRUE(pos.isEnd());
    }
}

TEST(LLRuntime, FailSyncTokensHonorsSkipBudgetAndAbort) {
    const Token tokens[]{Token{ISPA_STD::RawTokenMeta{}, Leaf{}},
                         Token{ISPA_STD::RawTokenMeta{}, Other{}}, Token{}};
    Iter pos{tokens};
    AnchorParser parser;
    parser.recovery_state_.max_total_skipped = 1;
    Leaf ast;
    const auto before = pos;
    auto ctx = parser.fail_context(pos, ast, pos, "owner", "expression");
    EXPECT_THROW(ctx.sync_tokens(""), std::invalid_argument);
    ctx.sync_tokens("missing");
    ctx.insert();
    EXPECT_EQ(ctx.finish(before).mode, ISPA_STD::Recovery::Mode::Abort);
    EXPECT_EQ(pos.index, 1u);
    EXPECT_EQ(parser.recovery_state_.total_skipped, 1u);
    ctx.sync_tokens("missing");
    EXPECT_EQ(pos.index, 1u);
}
