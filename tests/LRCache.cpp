#include <gtest/gtest.h>
#include <ispa/parser.h>
import AST.Builder;
import UAST;
import Modules;
import AST.Pass;
import AST.Tree;
import LRParser;
import LALRParser;
import std;

namespace {
auto cacheGrammar(const char* source, const char* file = "cache.isc") -> AST::Tree {
    Parser::Parser parser;
    AST::Builder builder(parser.parse(source), file);
    builder.build();
    auto tree = builder.get();
    AST::TreePass pass(tree); pass.work();
    return tree;
}
void sameTables(const LRBase& restored, const LRBase& fresh) {
    EXPECT_EQ(restored.getCanonicalCollection(), fresh.getCanonicalCollection());
    EXPECT_EQ(restored.getKActionTable(), fresh.getKActionTable());
    EXPECT_EQ(restored.getGotoTable(), fresh.getGotoTable());
    EXPECT_EQ(restored.skipsAutomaticWhitespace(), fresh.skipsAutomaticWhitespace());
    EXPECT_EQ(restored.getDecisionLookaheadK(), fresh.getDecisionLookaheadK());
    EXPECT_EQ(restored.needsRuntimeConflictResolution(), fresh.needsRuntimeConflictResolution());
    const auto& actual = restored.getConflicts();
    const auto& expected = fresh.getConflicts();
    ASSERT_EQ(actual.size(), expected.size());
    for (std::size_t i = 0; i < actual.size(); ++i) {
        EXPECT_EQ(actual[i].state, expected[i].state);
        EXPECT_EQ(actual[i].lookahead_sequence, expected[i].lookahead_sequence);
        EXPECT_EQ(actual[i].actions, expected[i].actions);
    }
    const auto& resolutions = restored.getConflictResolutions();
    const auto& expected_resolutions = fresh.getConflictResolutions();
    ASSERT_EQ(resolutions.size(), expected_resolutions.size());
    for (std::size_t i = 0; i < resolutions.size(); ++i) {
        EXPECT_EQ(resolutions[i].conflict, expected_resolutions[i].conflict);
        EXPECT_EQ(resolutions[i].lookahead, expected_resolutions[i].lookahead);
        const auto& decisions = resolutions[i].decisions;
        const auto& wanted = expected_resolutions[i].decisions;
        ASSERT_EQ(decisions.size(), wanted.size());
        for (std::size_t j = 0; j < decisions.size(); ++j) {
            EXPECT_EQ(decisions[j].state, wanted[j].state);
            EXPECT_EQ(decisions[j].window, wanted[j].window);
            EXPECT_EQ(decisions[j].action, wanted[j].action);
        }
    }
}

template<class ParserType> void roundTrip() {
    const char* source = "A:'a'; B:'b'; C:'c'; D:'d'; x:A; main:(A B C)|(x B D);";
    auto tree = cacheGrammar(source);
    ParserType cold(&tree);
    cold.resolveConflicts(3);
    const auto snapshot = cold.saveCache();
    ParserType warm(&tree, 1, snapshot);
    warm.resolveConflicts(3);
    EXPECT_GT(warm.getCacheStats().closures_reused, 0u);
    EXPECT_EQ(warm.getCacheStats().closures_built, 0u);
    EXPECT_GT(warm.getCacheStats().rows_reused, 0u);
    EXPECT_EQ(warm.getCacheStats().rows_built, 0u);
    EXPECT_TRUE(warm.getCacheStats().resolutions_reused);
    sameTables(warm, cold);

    // Settings change invalidates decisions while retaining the structural work.
    ParserType settings(&tree, 1, snapshot), fresh_settings(&tree);
    settings.resolveConflicts(1); fresh_settings.resolveConflicts(1);
    EXPECT_FALSE(settings.getCacheStats().resolutions_reused);
    EXPECT_GT(settings.getCacheStats().rows_reused, 0u);
    sameTables(settings, fresh_settings);

    auto damaged = snapshot;
    damaged[damaged.size() / 2] ^= 1;
    for (const auto& bytes : {damaged, snapshot.substr(0, snapshot.size() / 2), std::string("garbage")}) {
        ParserType corrupt(&tree, 1, bytes);
        corrupt.resolveConflicts(3);
        EXPECT_EQ(corrupt.getCacheStats().closures_reused, 0u);
        sameTables(corrupt, cold);
    }
}

template<class ParserType> void structuralChanges() {
    const char* first = "A:'a'; B:'b'; C:'c'; left:A; right:B; main:left right;";
    auto original = cacheGrammar(first);
    ParserType before(&original);
    before.resolveConflicts(3);
    const auto snapshot = before.saveCache();
    // Insertion/removal shifts production and state numbering. Cached references
    // must resolve by identity; dependent FIRST/nullable/alternative edits rebuild.
    for (const char* source : {
        "A:'a'; B:'b'; C:'c'; aa:C; left:A; right:B; main:left right;",
        "A:'a'; B:'b'; C:'c'; left:A; right:C; main:left right;",
        "A:'a'; B:'b'; C:'c'; left:A; right:B?; main:left right;",
        "A:'a'; B:'b'; C:'c'; left:A|C; right:B; main:left right;",
        "A:'a'; B:'b'; C:'c'; main:A B;"
    }) {
        SCOPED_TRACE(source);
        auto changed = cacheGrammar(source);
        ParserType incremental(&changed, 1, snapshot), fresh(&changed);
        incremental.resolveConflicts(3); fresh.resolveConflicts(3);
        sameTables(incremental, fresh);
        EXPECT_GT(incremental.getCacheStats().closures_reused, 0u);
        EXPECT_GT(incremental.getCacheStats().rows_reused, 0u);
        EXPECT_FALSE(incremental.getCacheStats().resolutions_reused);
        if (std::string_view(source).find("aa:C") != std::string_view::npos) {
            EXPECT_EQ(incremental.getCacheStats().closures_built, 0u);
            EXPECT_EQ(incremental.getCacheStats().rows_built, 0u);
        }
    }
}
}

TEST(LRCache, LRRestoresTablesAndConflictDecisions) { roundTrip<LRParser>(); }
TEST(LRCache, LALRRestoresTablesAndConflictDecisions) { roundTrip<LALRParser>(); }
TEST(LRCache, LRTracksStructuralDependenciesAndRemapsReferences) { structuralChanges<LRParser>(); }
TEST(LRCache, LALRTracksStructuralDependenciesAndRemapsReferences) { structuralChanges<LALRParser>(); }

TEST(LRCache, CurrentSourcesAndActionsReplaceCachedSemanticMetadata) {
    auto before = cacheGrammar("A:'a'; main:@ A { @ };", "old.isc");
    LRParser cold(&before);
    cold.resolveConflicts(3);
    auto changed = cacheGrammar("A:'a';\n\nmain:@ A { @ };", "new.isc");
    LRParser warm(&changed, 1, cold.saveCache()), fresh(&changed);
    warm.resolveConflicts(3); fresh.resolveConflicts(3);
    sameTables(warm, fresh);
    EXPECT_EQ(warm.getCacheStats().closures_built, 0u);
    for (std::size_t i = 1; i < warm.getProductions().size(); ++i)
        EXPECT_EQ(warm.getProductions()[i].source.position.file, "new.isc");
}

TEST(LRCache, FamiliesAndLookaheadWidthsHaveSeparateSnapshots) {
    auto tree = cacheGrammar("A:'a'; B:'b'; left:A; main:left B;");
    LRParser lr(&tree);
    LALRParser lalr(&tree, 1, lr.saveCache()), fresh_lalr(&tree);
    EXPECT_EQ(lalr.getCacheStats().closures_reused, 0u);
    sameTables(lalr, fresh_lalr);
    LRParser wider(&tree, 2, lr.saveCache()), fresh_wider(&tree, 2);
    EXPECT_EQ(wider.getCacheStats().closures_reused, 0u);
    sameTables(wider, fresh_wider);
    wider.resolveConflicts(3);
    LRParser warm_wider(&tree, 2, wider.saveCache());
    warm_wider.resolveConflicts(3);
    EXPECT_EQ(warm_wider.getCacheStats().closures_built, 0u);
    EXPECT_EQ(warm_wider.getCacheStats().rows_built, 0u);
    sameTables(warm_wider, wider);
}

namespace {
template<class ParserType> void editMatrix() {
    const std::vector<std::string> bodies{
        "left:A; right:B; main:left right;",
        "left:A|B; right:B; main:left right;",
        "left:B|A; right:B; main:left right;",
        "left:A|A; right:B; main:left right;",
        "left:A?; right:B*; main:left right;",
        "left:A; right:B; main:main A|left right;",
        "left:A left|A; right:B|C; main:left right;",
        "left:(A B)*; right:C?; main:left right;"
    };
    for (const auto& original : bodies) {
        auto before = cacheGrammar(("A:'a'; B:'b'; C:'c';" + original).c_str());
        ParserType built(&before);
        built.resolveConflicts(3);
        auto snapshot = built.saveCache();
        for (const auto& edited : bodies) {
            SCOPED_TRACE(original + " -> " + edited);
            auto tree = cacheGrammar(("A:'a'; B:'b'; C:'c';" + edited).c_str());
            ParserType incremental(&tree, 1, snapshot), fresh(&tree);
            incremental.resolveConflicts(3); fresh.resolveConflicts(3);
            sameTables(incremental, fresh);
            snapshot = incremental.saveCache();
        }
    }
}
}
TEST(LRCache, LREditSequencesMatchFreshBuilds) { editMatrix<LRParser>(); }
TEST(LRCache, LALREditSequencesMatchFreshBuilds) { editMatrix<LALRParser>(); }

TEST(LRCache, CachedConflictOriginsUseCurrentSourcePositions) {
    auto first = cacheGrammar("A:'a'; main:main main|A;", "old.isc");
    LRParser original(&first);
    original.resolveConflicts(3);
    auto current = cacheGrammar("A:'a';\n\nmain:main main|A;", "current.isc");
    LRParser warm(&current, 1, original.saveCache()), fresh(&current);
    warm.resolveConflicts(3); fresh.resolveConflicts(3);
    sameTables(warm, fresh);
    ASSERT_FALSE(warm.getConflicts().empty());
    EXPECT_EQ(warm.getCacheStats().rows_built, 0u);
    EXPECT_TRUE(warm.getCacheStats().resolutions_reused);
    for (const auto& conflict : warm.getConflicts())
        for (const auto& origin : conflict.origins) EXPECT_EQ(origin.position.file, "current.isc");
}

TEST(LRCache, BootstrapGrammarRestoresWithoutClosureOrRowReconstruction) {
    const auto project = Modules::preprocess({std::string(ROOT_DIR) + "/parser/ispa/grammar"}, {std::string(ROOT_DIR) + "/parser/ispa/entry.isc"});
    UAST::main syntax;
    std::vector<std::string> files;
    for (const auto& source : project.sources) {
        SCOPED_TRACE(source.file);
        IspaParser parser;
        const auto& parsed = parser.parse(source.grammar.c_str()).value.data();
        ASSERT_TRUE(parser.getErrors().empty()) << source.file;
        syntax.value.insert(syntax.value.end(), parsed.value.begin(), parsed.value.end());
        files.insert(files.end(), parsed.value.size(), source.file);
    }
    AST::Builder builder(syntax);
    builder.setSourceFiles(std::move(files));
    builder.setModules(project);
    builder.build();
    auto tree = builder.get();
    AST::TreePass pass(tree); pass.work();
    const auto begin = std::chrono::steady_clock::now();
    LRParser cold(&tree);
    cold.resolveConflicts(3);
    const auto snapshot = cold.saveCache();
    const auto middle = std::chrono::steady_clock::now();
    LRParser warm(&tree, 1, snapshot);
    warm.resolveConflicts(3);
    const auto warm_snapshot = warm.saveCache();
    const auto end = std::chrono::steady_clock::now();
    sameTables(warm, cold);
    EXPECT_EQ(warm_snapshot, snapshot);
    EXPECT_EQ(warm.getCacheStats().closures_built, 0u);
    EXPECT_EQ(warm.getCacheStats().rows_built, 0u);
    EXPECT_TRUE(warm.getCacheStats().resolutions_reused);
    std::cout << "LR bootstrap cache: cold "
        << std::chrono::duration_cast<std::chrono::milliseconds>(middle - begin).count()
        << " ms, warm " << std::chrono::duration_cast<std::chrono::milliseconds>(end - middle).count()
        << " ms, " << snapshot.size() << " bytes\n";
}
