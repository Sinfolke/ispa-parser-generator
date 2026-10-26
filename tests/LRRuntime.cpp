#include <ispastdlib/lr_machine.hpp>
#include <gtest/gtest.h>
#include <cassert>
namespace {
struct Leaf { int value; };
struct Root { int value; };
struct Token {
    int value;
    std::size_t kind() const { return value < 0 ? 99 : 7; }
};
struct Iter {
    const std::vector<Token>* input;
    std::size_t index = 0;
    bool isEnd() const { return index == input->size(); }
    const Token& operator*() const { return input->at(index); }
    Iter& operator++() { ++index; return *this; }
};
struct Parser : ISPA_STD::LRParser<Root, Token> {
    void parseFromTokens() override {}
    auto lazyParse() -> ISPA_STD::MatchResult<Root, ISPA_STD::Lexer_base<Token>::lazy_iterator> override { return {}; }
    static std::size_t lr_k() { return 2; }
    static long long lr_action(std::size_t s, const std::vector<std::size_t>& w) {
        const auto eof = std::numeric_limits<std::size_t>::max();
        if (s == 0 && w == std::vector<std::size_t>{7, eof}) return 2;
        if (s == 1 && w == std::vector<std::size_t>{eof, eof}) return -1;
        if (s == 2 && w == std::vector<std::size_t>{eof, eof}) return 0;
        throw std::runtime_error("unexpected token");
    }
    static std::any lr_shift(Token token) { return lr_node(Leaf{token.value}); }
    static std::size_t lr_length(std::size_t) { return 1; }
    static std::size_t lr_lhs(std::size_t) { return 0; }
    static std::size_t lr_goto(std::size_t s, std::size_t) { assert(s == 0); return 2; }
    static std::any lr_reduce(std::size_t, const std::vector<std::any>& values) {
        return lr_node(Root{lr_get<Leaf>(values, 0).data().value});
    }
    auto run(const std::vector<Token>& tokens) { return lr_parse<Parser>(Iter{&tokens}); }
};
struct EmptyParser : Parser {
    static long long lr_action(std::size_t state, const std::vector<std::size_t>& window) {
        if (window != std::vector<std::size_t>(2, std::numeric_limits<std::size_t>::max()))
            throw std::runtime_error("unexpected token");
        return state == 0 ? -1 : 0;
    }
    static std::size_t lr_length(std::size_t) { return 0; }
    static std::size_t lr_goto(std::size_t, std::size_t) { return 1; }
    static std::any lr_reduce(std::size_t, const std::vector<std::any>& values) {
        if (!values.empty()) throw std::runtime_error("nonempty epsilon reduction");
        return lr_node(Root{0});
    }
    auto run(const std::vector<Token>& tokens) { return lr_parse<EmptyParser>(Iter{&tokens}); }
};

// S -> LEAF semantic LEAF, semantic -> epsilon. The action must execute
// between the two shifts, at the epsilon reduction selected by the LR table.
struct SemanticParser : Parser {
    bool ran = false;
    static long long lr_action(std::size_t state, const std::vector<std::size_t>& window) {
        const auto eof = std::numeric_limits<std::size_t>::max();
        if (state == 0 && window == std::vector<std::size_t>{7, 7}) return 2;
        if (state == 1 && window == std::vector<std::size_t>{7, eof}) return -1;
        if (state == 2 && window == std::vector<std::size_t>{7, eof}) return 4;
        if (state == 3 && window == std::vector<std::size_t>{eof, eof}) return -2;
        if (state == 4 && window == std::vector<std::size_t>{eof, eof}) return 0;
        throw std::runtime_error("unexpected token");
    }
    static std::size_t lr_length(std::size_t rule) { return rule == 0 ? 0 : 3; }
    static std::size_t lr_lhs(std::size_t rule) { return rule == 0 ? 1 : 0; }
    static std::size_t lr_goto(std::size_t state, std::size_t lhs) {
        if (state == 1 && lhs == 1) return 2;
        if (state == 0 && lhs == 0) return 4;
        throw std::runtime_error("invalid goto");
    }
    static std::any lr_reduce(std::size_t rule, const std::vector<std::any>& values) {
        if (rule != 1) throw std::runtime_error("semantic reduction was skipped");
        return lr_node(Root{lr_get<Leaf>(values, 0).data().value +
                            std::any_cast<int>(values.at(1)) +
                            lr_get<Leaf>(values, 2).data().value});
    }
    std::any lr_semantic_reduce(std::size_t rule, ISPA_STD::LRRegionFrame<Iter>& frame,
                               ISPA_STD::LRRuleSpan&, const std::vector<std::any>& values) {
        if (rule != 0) return lr_reduce(rule, values);
        if (frame.control.phase != ISPA_STD::LRControlPhase::Semantic ||
            frame.pos.index != 1 || frame.states.back() != 1 ||
            lr_get<Leaf>(frame.values, 0).data().value != 2)
            throw std::runtime_error("semantic action ran at the wrong LR state");
        ran = true;
        return 10;
    }
    auto run(const std::vector<Token>& tokens) { return lr_parse<SemanticParser>(Iter{&tokens}); }
};

struct SpaceParser : Parser {
    static bool lr_is_space(const Token& token) { return token.kind() == 99; }
    static bool lr_space(std::size_t, std::size_t gap) { return gap == 1; }
    static long long lr_action(std::size_t state, const std::vector<std::size_t>& window) {
        const auto eof = std::numeric_limits<std::size_t>::max();
        if (state == 0 && window == std::vector<std::size_t>{7, 7}) return 2;
        if (state == 1 && window == std::vector<std::size_t>{7, eof}) return 3;
        if (state == 2 && window == std::vector<std::size_t>{eof, eof}) return -1;
        if (state == 3 && window == std::vector<std::size_t>{eof, eof}) return 0;
        throw std::runtime_error("unexpected token");
    }
    static std::size_t lr_length(std::size_t) { return 2; }
    static std::size_t lr_goto(std::size_t, std::size_t) { return 3; }
    static std::any lr_reduce(std::size_t, const std::vector<std::any>& values) {
        return lr_node(Root{lr_get<Leaf>(values, 0).data().value + lr_get<Leaf>(values, 1).data().value});
    }
    auto run(const std::vector<Token>& tokens) { return lr_parse<SpaceParser>(Iter{&tokens}); }
};
struct NoSpaceParser : SpaceParser {
    static bool lr_space(std::size_t, std::size_t) { return false; }
    auto run(const std::vector<Token>& tokens) { return lr_parse<NoSpaceParser>(Iter{&tokens}); }
};

}
TEST(LRRuntime, ReducesCapturedValuesAndCanBeReused) {
    Parser parser;
    EXPECT_EQ(parser.run({{42}}).node.data().value, 42);
    EXPECT_EQ(parser.run({{11}}).node.data().value, 11);
}
TEST(LRRuntime, RunsSemanticReductionBetweenShifts) {
    SemanticParser parser;
    EXPECT_EQ(parser.run({{2}, {5}}).node.data().value, 17);
    EXPECT_TRUE(parser.ran);
}
TEST(LRRuntime, RegionFrameOwnsStackValuesAndContinuation) {
    const std::vector<Token> tokens{{3}};
    ISPA_STD::LRRegionFrame<Iter> outer(Iter{&tokens}, 7);
    outer.states.push_back(2);
    outer.values.push_back(Leaf{3});
    outer.capture_destinations.push_back(1);
    ISPA_STD::LRRegionFrame<Iter> inner(outer.pos, 9);
    inner.states.push_back(4);
    inner.values.push_back(Leaf{8});
    inner.control.phase = ISPA_STD::LRControlPhase::Reduce;
    inner.control.production = 5;
    EXPECT_EQ(outer.states.back(), 2u);
    EXPECT_EQ(std::any_cast<Leaf>(outer.values.back()).value, 3);
    EXPECT_EQ(outer.control.continuation, 7u);
    EXPECT_EQ(inner.states.back(), 4u);
    EXPECT_EQ(inner.control.continuation, 9u);
    EXPECT_EQ(inner.control.production, 5u);
}
TEST(LRRuntime, RejectsMissingAndTrailingTokens) {
    Parser parser;
    EXPECT_THROW(parser.run({}), std::runtime_error);
    EXPECT_THROW(parser.run({{1}, {2}}), std::runtime_error);
}

namespace {
struct DiagnosticParser : Parser {
    static long long lr_action(std::size_t state, const std::vector<std::size_t>& window) {
        const auto eof = std::numeric_limits<std::size_t>::max();
        if (state == 0 && window == std::vector<std::size_t>{7, eof}) return 2;
        if (state == 1 && window == std::vector<std::size_t>{eof, eof}) return -1;
        if (state == 2 && window == std::vector<std::size_t>{eof, eof}) return 0;
        return -std::numeric_limits<long long>::max();
    }
    static std::vector<std::size_t> lr_expected(std::size_t state) {
        return state == 0 ? std::vector<std::size_t>{7} : std::vector<std::size_t>{std::numeric_limits<std::size_t>::max()};
    }
    static std::string lr_context(std::size_t) { return "main"; }
    static std::string lr_token_name(std::size_t token) {
        return token == 7 ? "LEAF" : token == std::numeric_limits<std::size_t>::max() ? "end of input" : "OTHER";
    }
    auto run(const std::vector<Token>& tokens) { return lr_parse<DiagnosticParser>(Iter{&tokens}); }
};

struct RecoveryParser : DiagnosticParser {
    bool stall = false;
    bool lr_recover(ISPA_STD::LRRegionFrame<Iter>& region, ISPA_STD::Recovery::Diagnostic& error) {
        if (region.control.phase != ISPA_STD::LRControlPhase::Recover || stall || region.pos.isEnd())
            return stall;
        error.decision.mode = ISPA_STD::Recovery::Mode::Delete;
        error.decision.skipped = 1;
        ++region.pos;
        ++region.cursor;
        return true;
    }
    auto run(const std::vector<Token>& tokens) { return lr_parse<RecoveryParser>(Iter{&tokens}); }
};
}

TEST(LRRuntime, ReportsExpectedTokenAndRuleOnInvalidAction) {
    DiagnosticParser parser;
    try { parser.run({}); FAIL(); }
    catch (const std::runtime_error& error) {
        EXPECT_NE(std::string(error.what()).find("expected LEAF"), std::string::npos);
        EXPECT_NE(std::string(error.what()).find("in rule main"), std::string::npos);
    }
    ASSERT_EQ(parser.getErrors().size(), 1u);
    EXPECT_EQ(parser.getErrors().front().code, "lr-unexpected-token");
    EXPECT_EQ(parser.getErrors().front().unexpected, std::numeric_limits<std::size_t>::max());
    EXPECT_EQ(parser.getErrors().front().expected, (std::vector<std::size_t>{7}));
    EXPECT_EQ(parser.run({{42}}).node.data().value, 42);
    EXPECT_TRUE(parser.getErrors().empty());
}

TEST(LRRuntime, RecoveryHookRetriesAfterChangingTheRegionCursor) {
    RecoveryParser parser;
    EXPECT_EQ(parser.run({{-1}, {42}}).node.data().value, 42);
    ASSERT_EQ(parser.getErrors().size(), 1u);
    EXPECT_EQ(parser.getErrors().front().decision.mode, ISPA_STD::Recovery::Mode::Delete);
    EXPECT_EQ(parser.getErrors().front().decision.skipped, 1u);
    parser.stall = true;
    try {
        (void)parser.run({{-1}, {42}});
        FAIL() << "stalled recovery was accepted";
    } catch (const std::runtime_error& error) {
        EXPECT_NE(std::string(error.what()).find("made no progress"), std::string::npos);
    }
}

TEST(LRRuntime, ReducesEmptyProductionWithoutPoppingInitialState) {
    EmptyParser parser;
    EXPECT_EQ(parser.run({}).node.data().value, 0);
}

TEST(LRRuntime, SkipsTriviaInLookaheadWithoutChangingCaptureIndexes) {
    SpaceParser parser;
    EXPECT_EQ(parser.run({{2}, {3}}).node.data().value, 5);
    EXPECT_EQ(parser.run({{2}, {-1}, {-1}, {3}}).node.data().value, 5);
    EXPECT_THROW(parser.run({{-1}, {2}, {3}}), std::runtime_error);
    EXPECT_THROW(parser.run({{2}, {3}, {-1}}), std::runtime_error);
}
TEST(LRRuntime, PreservesNoSpaceBoundaries) {
    NoSpaceParser parser;
    EXPECT_EQ(parser.run({{2}, {3}}).node.data().value, 5);
    EXPECT_THROW(parser.run({{2}, {-1}, {3}}), std::runtime_error);
}

namespace {
struct CaptureParser : Parser {
    using Parser::lr_pack;
    using Parser::lr_capture_assign;
    using Parser::lr_node;
    using Parser::lr_token;
};
}
TEST(LRRuntime, SyntheticOptionalPreservesPresentAndAbsentNodes) {
    auto present = CaptureParser::lr_pack({CaptureParser::lr_node(Leaf{42})}, {0}, {0}, {0}, 1);
    auto absent = CaptureParser::lr_pack({}, {}, {}, {}, 1);
    ISPA_STD::Node<Leaf> value;
    CaptureParser::lr_capture_assign(value, {present}, 0, std::size_t(-1));
    EXPECT_EQ(value.data().value, 42);
    CaptureParser::lr_capture_assign(value, {absent}, 0, std::size_t(-1));
    EXPECT_TRUE(value.empty());
    std::tuple<ISPA_STD::Node<Leaf>, ISPA_STD::Node<Root>> tuple;
    EXPECT_NO_THROW(CaptureParser::lr_capture_assign(tuple, {absent}, 0, std::size_t(-1)));
    EXPECT_TRUE(std::get<0>(tuple).empty());
    EXPECT_TRUE(std::get<1>(tuple).empty());
}
TEST(LRRuntime, SyntheticRepetitionPreservesColumnsAndOrder) {
    auto tail = CaptureParser::lr_pack({}, {}, {}, {}, 2);
    for (int i = 3; i > 0; --i)
        tail = CaptureParser::lr_pack({CaptureParser::lr_node(Leaf{i}), CaptureParser::lr_node(Root{i * 10}), tail},
                                      {0, 1, 2}, {0, 1, 0}, {0, 0, 1}, 2);
    std::vector<ISPA_STD::Node<Leaf>> first;
    std::vector<ISPA_STD::Node<Root>> second;
    CaptureParser::lr_capture_assign(first, {tail}, 0, 0);
    CaptureParser::lr_capture_assign(second, {tail}, 0, 1);
    ASSERT_EQ(first.size(), 3);
    ASSERT_EQ(second.size(), 3);
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(first[i].data().value, i + 1);
        EXPECT_EQ(second[i].data().value, (i + 1) * 10);
    }
}
TEST(LRRuntime, SyntheticAlternativesSupportVariantsAndBoxedNodes) {
    auto payload = CaptureParser::lr_pack({CaptureParser::lr_node(Leaf{7})}, {0}, {0}, {0}, 1);
    std::variant<ISPA_STD::Node<Root>, ISPA_STD::Node<Leaf>> choice;
    CaptureParser::lr_capture_assign(choice, {payload}, 0, std::size_t(-1));
    EXPECT_EQ(std::get<ISPA_STD::Node<Leaf>>(choice).data().value, 7);
    ISPA_STD::Node<std::shared_ptr<Leaf>> boxed;
    CaptureParser::lr_capture_assign(boxed, {payload}, 0, std::size_t(-1));
    ASSERT_TRUE(boxed.data());
    EXPECT_EQ(boxed.data()->value, 7);
    ISPA_STD::Node<Root> wrong;
    EXPECT_THROW(CaptureParser::lr_capture_assign(wrong, {payload}, 0, std::size_t(-1)), std::runtime_error);
}

TEST(LRRuntime, LiteralCaptureUsesTokenTextWithoutLosingTypedNode) {
    static constexpr char text[] = "@";
    ISPA_STD::Node<Leaf> leaf(0, text, text + 1, 1, 1, 1, 7, Leaf{9});
    auto token = CaptureParser::lr_token(leaf);
    auto payload = CaptureParser::lr_pack({token}, {0}, {0}, {0}, 1);
    char character = 0;
    CaptureParser::lr_capture_assign(character, {payload}, 0, std::size_t(-1));
    EXPECT_EQ(character, '@');
    ISPA_STD::Node<Leaf> node;
    CaptureParser::lr_capture_assign(node, {payload}, 0, std::size_t(-1));
    EXPECT_EQ(node.data().value, 9);
}
TEST(LRRuntime, RepeatedTupleCaptureKeepsMembersInTheSameIteration) {
    auto payload = CaptureParser::lr_pack({}, {}, {}, {}, 2, true);
    for (int i = 2; i > 0; --i)
        payload = CaptureParser::lr_pack({CaptureParser::lr_node(Leaf{i}), CaptureParser::lr_node(Root{i * 10}), payload},
                                         {0, 1, 2}, {0, 1, 0}, {0, 0, 1}, 2, true);
    std::vector<std::tuple<ISPA_STD::Node<Leaf>, ISPA_STD::Node<Root>>> tuples;
    CaptureParser::lr_capture_assign(tuples, {payload}, 0, std::size_t(-1));
    ASSERT_EQ(tuples.size(), 2);
    EXPECT_EQ(std::get<0>(tuples[0]).data().value, 1);
    EXPECT_EQ(std::get<1>(tuples[0]).data().value, 10);
    EXPECT_EQ(std::get<0>(tuples[1]).data().value, 2);
    EXPECT_EQ(std::get<1>(tuples[1]).data().value, 20);
}

TEST(LRRuntime, LiteralVariantRetainsTextUntilAnAlternativeIsSelected) {
    static constexpr char text[] = "@";
    ISPA_STD::Node<Leaf> leaf(0, text, text + 1, 1, 1, 1, 7, Leaf{9});
    auto literal = CaptureParser::lr_token(leaf);
    auto payload = CaptureParser::lr_pack({literal}, {0}, {0}, {0}, 1);
    std::variant<std::monostate, ISPA_STD::Node<Root>, char> choice;
    CaptureParser::lr_capture_assign(choice, {payload}, 0, std::size_t(-1));
    ASSERT_TRUE(std::holds_alternative<char>(choice));
    EXPECT_EQ(std::get<char>(choice), '@');
    auto expression = CaptureParser::lr_pack({CaptureParser::lr_node(Root{42})}, {0}, {0}, {0}, 1);
    CaptureParser::lr_capture_assign(choice, {expression}, 0, std::size_t(-1));
    ASSERT_TRUE(std::holds_alternative<ISPA_STD::Node<Root>>(choice));
    EXPECT_EQ(std::get<ISPA_STD::Node<Root>>(choice).data().value, 42);
    std::variant<std::monostate, ISPA_STD::Node<Root>, std::string> string_choice;
    CaptureParser::lr_capture_assign(string_choice, {literal}, 0, std::size_t(-1));
    EXPECT_EQ(std::get<std::string>(string_choice), "@");
}
