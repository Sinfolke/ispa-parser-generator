#ifdef CORE
#include <ispa_core/parser_ll.hpp>
#include <ispa_core/external.hpp>
#define LLP ISPA_CORE::LLParser
namespace NS = ISPA_CORE;
#else
#include <ispastdlib/parser_ll.hpp>
#include <ispastdlib/external.hpp>
#define LLP ISPA_STD::LLParser
#endif
#include <iostream>
#include <functional>
using namespace std;
struct Leaf {}; struct Other {}; struct Space {};
using Token = ISPA_STD::RawToken<Leaf, Other, Space>;
struct Iter {
    const Token* tokens; std::size_t index = 0;
    std::size_t distance() const { return index; }
    bool isEnd() const { return tokens[index].empty(); }
    const Token& operator*() const { return tokens[index]; }
    const Token* operator->() const { return &tokens[index]; }
    Iter& operator++() { ++index; return *this; }
    Iter& operator--() { if (index) --index; return *this; }
};
struct LeafSet { static std::vector<std::size_t> values() { return {0}; } static bool matches(const Token& t) { return t.holds<Leaf>(); } };
struct NoSet { static std::vector<std::size_t> values() { return {}; } static bool matches(const Token&) { return false; } };
struct Policy { static constexpr bool allow_insertion = false, allow_deletion = false, allow_replacement = false; static constexpr std::size_t max_skip = 8; };
struct Parser : LLP<Leaf, Token> {
    using LLP<Leaf, Token>::optional; using LLP<Leaf, Token>::required; using LLP<Leaf, Token>::range;
    using LLP<Leaf, Token>::range_optional; using LLP<Leaf, Token>::counted;
    using LLP<Leaf, Token>::counter_scope; using LLP<Leaf, Token>::bind_counter;
    int failures = 0;
#ifdef CORE
    void merge() { merge_diagnostics(); }
#else
    void merge() {}
#endif
    void parseFromTokens() override {}
    auto lazyParse() -> ISPA_STD::MatchResult<Leaf, ISPA_STD::Lexer_base<Token>::lazy_iterator> override { return {true, {}, {}}; }
    auto fail() {
#ifdef CORE
        return ISPA_CORE::Recovery::handler<LeafSet, NoSet, NoSet, NoSet, Policy>("leaf", "Leaf");
#else
        return [this](Iter&) { ++failures; };
#endif
    }
    auto leaf(Iter pos) -> ISPA_STD::MatchResult<Leaf, Iter> { auto node = required<Leaf>(pos, fail()); return {true, node, pos}; }
    auto leafToken(Iter& pos) -> ISPA_STD::Node<Leaf> { return required<Leaf>(pos, fail()); }
    auto pair(Iter pos) -> ISPA_STD::MatchResult<Leaf, Iter> { leafToken(pos); auto node = leafToken(pos); return {true, node, pos}; }
};
#define TRY(expr) try { cout << #expr << " -> " << (expr) << "\n"; } catch (const std::exception& e) { cout << #expr << " threw " << typeid(e).name() << ": " << e.what() << "\n"; }
#define L Token{{}, Leaf{}}
#define O Token{{}, Other{}}
#define S Token{{}, Space{}}
int main() {
    try { ISPA_STD::CountSpec::fixed(-1); } catch (const std::invalid_argument&) { cout << "fixed(-1) threw\n"; }
    { const Token t[]{L, L, L, Token{}}; Parser p; Iter pos{t};
      TRY(p.counted<Leaf>(pos, ISPA_STD::CountSpec::fixed(2)).size()); cout << pos.index << "\n";
      TRY(p.counted<Leaf>(pos, ISPA_STD::CountSpec::fixed(0)).size()); cout << pos.index << "\n";
      TRY(p.counted<Leaf>(pos, ISPA_STD::CountSpec::fixed(2)).size()); cout << pos.index << "\n"; 
      p.merge(); cout << "diag " << p.getErrors().size() << "\n"; }
    { const Token t[]{L, L, O, L, L, L, Token{}}; Parser p; Iter pos{t};
      auto scope = p.counter_scope();
      TRY(p.counted<Leaf>(pos, ISPA_STD::CountSpec::named("x")).size()); ++pos;
      { auto child = p.counter_scope();
        TRY(p.counted<Leaf>(pos, ISPA_STD::CountSpec::named("x"), [&](Iter c) { return p.leaf(c); }).size()); }
      cout << pos.index << "\n";
      TRY(p.counted<Leaf>(pos, ISPA_STD::CountSpec::named("x")).size()); }
    { const Token t[]{L, L, L, L, L, Token{}}; Parser p; Iter pos{t};
      TRY(p.counted<Leaf>(pos, ISPA_STD::CountSpec::fixed(2), [&](Iter c) { return p.pair(c); }).size()); cout << pos.index << "\n"; }
    { const Token t[]{L, O, L, L, Token{}}; Parser p; Iter pos{t};
      try { auto scope = p.counter_scope(); TRY(p.counted<Leaf>(pos, ISPA_STD::CountSpec::named("x")).size()); throw std::runtime_error("leave rule"); } catch (const std::runtime_error&) {}
      ++pos; auto scope = p.counter_scope();
      TRY(p.counted<Leaf>(pos, ISPA_STD::CountSpec::named("x")).size()); cout << pos.index << "\n"; }
    { const Token t[]{L, S, L, S, O, Token{}}; Parser p; Iter pos{t};
      TRY((p.counted<Leaf, Space>(pos, ISPA_STD::CountSpec::named("x")).size())); cout << pos.index << "\n"; }
    { const Token t[]{O, Token{}}; Parser p; Iter pos{t};
      auto empty = [](Iter c) -> ISPA_STD::MatchResult<Leaf, Iter> { ISPA_STD::Node<Leaf> n; n.set_data(Leaf{}); return {true, n, c}; };
      TRY(p.counted<Leaf>(pos, ISPA_STD::CountSpec::named("x"), empty).size());
      TRY(p.counted<Leaf>(pos, ISPA_STD::CountSpec::fixed(3), empty).size()); cout << pos.index << "\n"; }
    { const Token t[]{L, L, O, S, S, S, Token{}}; Parser p; Iter pos{t};
      const auto nodes = p.range_optional<Leaf>(pos); p.bind_counter("x", nodes); ++pos;
      TRY(p.counted<Space>(pos, ISPA_STD::CountSpec::named("x")).size()); cout << pos.index << "\n"; }
    { const Token t[]{L, L, L, Token{}}; Parser p; Iter pos{t};   // minimum above the count
      TRY(p.counted<Leaf>(pos, ISPA_STD::CountSpec::named("y", 5)).size()); cout << pos.index << "\n"; }
}
