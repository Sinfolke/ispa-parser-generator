#include <gtest/gtest.h>
#include <ispa/parser.h>
#include <ispa_dep_scanner/parser.h>
import Modules;
import InstalledLibraries;
import UAST;
import dstd;
import AST.Builder;
import AST.API;
import AST.Tree;
import AST.Pass;
import Algorithm.API;
import LexerBuilder;
import LangAPI;
import LangRepr.Converter;
import cpuf.dlib;
import Dump;
import args;
import std;

namespace {
struct ModuleFiles {
    std::filesystem::path root;
    ModuleFiles() {
        static std::atomic<unsigned> serial = 0;
        root = std::filesystem::temp_directory_path() / ("ispa-modules-" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-" + std::to_string(serial++));
        std::filesystem::create_directories(root);
    }
    ~ModuleFiles() { std::error_code error; std::filesystem::remove_all(root, error); }
    std::string write(const std::string& name, const std::string& text) {
        const auto path = root / name;
        std::filesystem::create_directories(path.parent_path());
        std::ofstream(path) << text;
        return path.string();
    }
    Modules::Project preprocess(const std::vector<std::string>& files = {}) const {
        return Modules::preprocess({root.string()}, files);
    }
};
AST::Tree build(const Modules::Project& project) {
    UAST::main input;
    std::vector<std::string> files;
    for (auto& source : project.sources) {
        IspaParser parser;
        const auto& syntax = parser.parse(source.grammar.c_str()).value.data();
        input.value.insert(input.value.end(), syntax.value.begin(), syntax.value.end());
        files.insert(files.end(), syntax.value.size(), source.file);
    }
    AST::Builder builder(input);
    builder.setSourceFiles(std::move(files));
    builder.setModules(project);
    builder.build();
    return builder.get();
}
auto name(const Modules::Path& path) { return stdu::vector<std::string>(path.begin(), path.end()); }
const Modules::Source& module(const Modules::Project& project, const Modules::Path& path) {
    for (const auto& source : project.sources) if (source.name == path) return source;
    throw std::runtime_error("test module not found");
}
void references(const AST::RuleMember& member, std::vector<Modules::Path>& out) {
    if (member.isName()) out.emplace_back(member.getName().name.begin(), member.getName().name.end());
    else if (member.isGroup()) for (auto& child : member.getGroup().values) references(*child, out);
    else if (member.isOp()) for (auto& child : member.getOp().options) references(*child, out);
}
std::vector<Modules::Path> references(const AST::Rule& rule) {
    std::vector<Modules::Path> out;
    for (auto& member : rule.rule_members) references(*member, out);
    return out;
}
const std::string cpp = R"(
cpp_constexpr: 'constexpr';
feature cpp_constexpr { cpp_constexpr };
declaration: cpp_constexpr? 'x';
required: cpp_constexpr 'y';
caller: required;
choice: required | declaration;
)";
}

TEST(Modules, InstalledGrammarIsDiscoveredWithoutBecomingTheEntry) {
    ModuleFiles files;
    const auto package_root = files.root / "libraries" / "std";
    const auto grammar_root = package_root / "ispalib" / "grammar";
    std::filesystem::create_directories(grammar_root);
    std::ofstream(grammar_root / "names.isc") << "module std.names;\nID: [a-z]+;\n";
    const auto main = files.write("project/main.isc",
        "module host -> main;\nimport {ID} from std.names;\nmain: ID;\n");
    const auto installed = InstalledLibraries::discover(files.root / "libraries");
    ASSERT_EQ(installed.source_directories.size(), 1);
    const auto project = Modules::preprocess({(files.root / "project").string()}, {main}, {},
        installed.source_directories);
    ASSERT_TRUE(project.root.has_value());
    EXPECT_EQ(Modules::spelling(*project.root), "host");
    ASSERT_EQ(project.sources.size(), 2);
}

TEST(Modules, InstalledDependencyManifestUsesPackageNameAndCanBeOverridden) {
    ModuleFiles files;
    const auto package = files.root / "libraries" / "foreign" / "ispatarget" / "cpp";
    std::filesystem::create_directories(package);
    std::ofstream(package / "Foreign.ispa-export") << "ISPA_EXTERN 1\n";
    std::ofstream(package / "Foreign.h") << "// generated header\n";
    const auto installed = InstalledLibraries::discover(files.root / "libraries");
    const auto expected = (package / "Foreign").string();
    EXPECT_EQ(installed.dependencies.at("foreign"), expected);
    EXPECT_EQ(installed.dependencies.at("Foreign"), expected);
}

TEST(Modules, PluginLibraryIsResolvedBesideExecutable) {
    ModuleFiles files;
#ifdef _WIN32
    const auto filename = "ispa-algorithm-LL.dll";
#elif defined(__APPLE__)
    const auto filename = "libispa-algorithm-LL.dylib";
#else
    const auto filename = "libispa-algorithm-LL.so";
#endif
    std::ofstream(files.root / filename) << "placeholder";
    const auto expected = (files.root / filename).replace_extension().string();
    EXPECT_EQ(InstalledLibraries::pluginPrefix(files.root, "algorithm", "LL"), expected);
}

TEST(Modules, LocalGrammarOverridesInstalledModuleWithSameName) {
    ModuleFiles files;
    const auto library = files.root / "libraries" / "std" / "ispalib" / "grammar";
    std::filesystem::create_directories(library);
    std::ofstream(library / "names.isc") << "module std.names;\nID: 'installed';\n";
    const auto local = files.write("project/names.isc", "module std.names;\nID: 'local';\n");
    const auto main = files.write("project/main.isc",
        "module host -> main;\nimport {ID} from std.names;\nmain: ID;\n");
    const auto project = Modules::preprocess({(files.root / "project").string()}, {main}, {},
        {library.string()});
    ASSERT_EQ(project.sources.size(), 2);
    const auto chosen = std::ranges::find_if(project.sources, [](const Modules::Source& source) {
        return source.name == Modules::Path{"std", "names"};
    });
    ASSERT_NE(chosen, project.sources.end());
    EXPECT_EQ(chosen->file, std::filesystem::canonical(local).string());
}

TEST(Modules, InstalledModulesDoNotTurnLegacyGrammarIntoModularInput) {
    ModuleFiles files;
    const auto library = files.root / "libraries" / "std" / "ispalib" / "grammar";
    std::filesystem::create_directories(library);
    std::ofstream(library / "names.isc") << "module std.names;\nID: [a-z]+;\n";
    const auto main = files.write("project/main.isc", "main: 'x';\n");
    const auto project = Modules::preprocess({(files.root / "project").string()}, {main}, {},
        {library.string()});
    EXPECT_FALSE(project.modular);
    ASSERT_EQ(project.sources.size(), 1);
    EXPECT_EQ(project.sources.front().file, std::filesystem::canonical(main).string());
}

TEST(Modules, RecursiveCaptureDeduplicatesAndRestrictsImports) {
    ModuleFiles files;
    files.write("lib/expr.isc", "expr: NUMBER; NUMBER: [0-9]+; unused: 'u';");
    const auto main = files.write("main.isc", "import {expr} from lib.expr;\nmain: expr;");
    auto project = Modules::preprocess({files.root.string(), (files.root / "lib").string()}, {main});
    ASSERT_TRUE(project.modular);
    ASSERT_EQ(project.sources.size(), 2u);
    const auto& scope = module(project, {"main"});
    EXPECT_EQ(scope.allowed_symbols.at({"expr"}), (Modules::Path{"lib", "expr", "expr"}));
    EXPECT_FALSE(scope.allowed_symbols.contains({"NUMBER"}));
    auto tree = build(project);
    EXPECT_EQ(references(tree.getTreeMap().at(name({"modules", "main", "main"}))).front(),
              (Modules::Path{"modules", "lib", "expr", "expr"}));
    EXPECT_TRUE(tree.getTreeMap().contains(name({"modules", "lib", "expr", "NUMBER"})));
    EXPECT_FALSE(tree.getTreeMap().contains(name({"modules", "lib", "expr", "unused"})));
}

TEST(Modules, TypedParameterHeadersAndCallsSurvivePreprocessing) {
    ModuleFiles files;
    const auto source = files.write("main.isc",
        "module main; TOKEN&(value: str): 'x'; item&(count: int): TOKEN=count; main: item &(amount);");
    auto project = files.preprocess();
    const auto& module = project.source(source);
    EXPECT_TRUE(module.declarations.contains({"TOKEN"}));
    EXPECT_TRUE(module.declarations.contains({"item"}));
    auto tree = build(project);
    const auto& rule = tree.getTreeMap().at(name({"modules", "main", "item"}));
    const auto& declaration = std::get<AST::DeclarationSyntax>(rule.features.front());
    EXPECT_EQ(declaration.parameters, (std::vector<std::string>{"count"}));
    EXPECT_EQ(declaration.parameterAnnotations.size(), 1u);
}

TEST(Modules, TemplateArgumentsKeepTheirDeclaringScopes) {
    ModuleFiles files;
    files.write("lib.isc", "wrap<T>: @ T {@};");
    const auto main = files.write("main.isc", "import {wrap} from lib; TOKEN:'x'; main: @ wrap<TOKEN> {@};");
    auto tree = build(files.preprocess({main}));
    const auto& reference = tree.getTreeMap().at(name({"modules", "main", "main"})).rule_members.front()->getName();
    EXPECT_EQ(reference.name, name({"modules", "lib", "wrap"}));
    ASSERT_EQ(reference.templateArguments.size(), 1u);
    EXPECT_EQ(reference.templateArguments.front(), name({"modules", "main", "TOKEN"}));
    EXPECT_TRUE(tree.contains(name({"modules", "main", "TOKEN"})));
}
TEST(Modules, AliasesAndNestedRuleScopes) {
    ModuleFiles files;
    files.write("lib.isc", "main: inner #inner: 'x'; ;");
    files.write("main.isc", "import lib as syntax; main: syntax;");
    auto project = files.preprocess();
    auto tree = build(project);
    EXPECT_EQ(references(tree.getTreeMap().at(name({"modules", "main", "main"}))).front(),
              (Modules::Path{"modules", "lib", "main"}));
    EXPECT_EQ(references(tree.getTreeMap().at(name({"modules", "lib", "main"}))).front(),
              (Modules::Path{"modules", "lib", "main", "inner"}));
}
TEST(Modules, UnusedDiscoveredModulesDoNotRequireExternalDependencies) {
    ModuleFiles files;
    const auto main = files.write("main.isc", "module main; main: 'x';");
    const auto optional = files.write("optional.isc",
        "import extern foreign expression; main: expression;");
    auto project = files.preprocess({main});
    ASSERT_EQ(project.sources.size(), 1u);
    EXPECT_NO_THROW(build(project));
    EXPECT_THROW(files.preprocess({main, optional}), std::runtime_error);

    files.write("main.isc", "import optional; main: optional;");
    EXPECT_THROW(files.preprocess({main}), std::runtime_error);
    files.write("middle.isc", "import optional; main: optional;");
    files.write("main.isc", "import middle; main: middle;");
    EXPECT_THROW(files.preprocess({main}), std::runtime_error);
}
TEST(Modules, ImportErrorsAreReportedAtTheirSource) {
    ModuleFiles files;
    files.write("lib.isc", "rule: 'x';");
    const auto main = files.write("main.isc", "\nimport {missing} from lib; main: 'm';");
    try { files.preprocess(); FAIL() << "expected missing import"; }
    catch (const std::exception& e) { EXPECT_NE(std::string(e.what()).find(main + ":2:1:"), std::string::npos); }
    files.write("main.isc", "import absent; main: 'x';");
    EXPECT_THROW(files.preprocess(), std::runtime_error);
    files.write("main.isc", "import {rule} from lib; rule: 'y'; main: rule;");
    EXPECT_THROW(files.preprocess(), std::runtime_error);
    files.write("main.isc", "main: lib.rule;");
    files.write("lib.isc", "module lib; rule: 'x';");
    EXPECT_THROW(build(files.preprocess()), std::runtime_error);
}
TEST(Modules, MissingDelimiterReportsExpectedCloserAndOpeningLine) {
    ModuleFiles files;
    const auto file = files.write("main.isc", "A: 'a';\nmain: ( A ;\n");
    try { files.preprocess(); FAIL() << "expected missing delimiter"; }
    catch (const std::exception& error) {
        const std::string message = error.what();
        EXPECT_NE(message.find(file + ":3:1:"), std::string::npos) << message;
        EXPECT_NE(message.find("expected ')'"), std::string::npos) << message;
        EXPECT_NE(message.find("opened at line 2"), std::string::npos) << message;
    }
}
TEST(Modules, CyclicImportsResolveRulesAndFeatures) {
    ModuleFiles files;
    files.write("a.isc", "import {two} from b; one: 'a' two?; feature recursion {two};");
    files.write("b.isc", "import {one, recursion} from a; two: 'b' one?;");
    files.write("main.isc", "import {one} from a; main: one;");
    auto project = files.preprocess();
    EXPECT_EQ(module(project, {"b"}).allowed_features.at({"recursion"}),
              (std::vector<Modules::Path>{{"b", "two"}}));
    auto tree = build(project);
    EXPECT_EQ(references(tree.getTreeMap().at(name({"modules", "a", "one"}))).front(),
              (Modules::Path{"modules", "b", "two"}));
    EXPECT_EQ(references(tree.getTreeMap().at(name({"modules", "b", "two"}))).front(),
              (Modules::Path{"modules", "a", "one"}));
}
TEST(Modules, CyclicImportsSupportLocalFeatureDetach) {
    ModuleFiles files;
    files.write("a.isc", "import {two} from b; one: 'a' two?; feature recursion {two};");
    files.write("b.isc", "import {one} from a; two: 'b' one?;");
    files.write("main.isc", "import * from a; detach feature recursion; main: one;");
    auto tree = build(files.preprocess());
    auto target = references(tree.getTreeMap().at(name({"modules", "main", "main"}))).front();
    EXPECT_TRUE(references(tree.getTreeMap().at(name(target))).empty());
}
TEST(Modules, DetachPropagatesThroughRequiredCallersAndPrunesAlternatives) {
    ModuleFiles files;
    files.write("cpp.isc", cpp);
    files.write("main.isc", "import * from cpp; detach cpp_constexpr; main: declaration;");
    auto project = files.preprocess();
    EXPECT_FALSE(module(project, {"main"}).allowed_symbols.contains({"cpp_constexpr"}));
    auto tree = build(project);
    const auto target = references(tree.getTreeMap().at(name({"modules", "main", "main"}))).front();
    const auto& declaration = tree.getTreeMap().at(name(target));
    EXPECT_TRUE(references(declaration).empty());
    ASSERT_EQ(declaration.rule_members.size(), 1u);
    EXPECT_EQ(declaration.rule_members.front()->getString().value, "x");
    for (const auto& [path, rule] : tree.getTreeMap()) {
        EXPECT_NE(path.back(), "cpp_constexpr");
        EXPECT_NE(path.back(), "required");
        EXPECT_NE(path.back(), "caller");
        if (path.back() == "choice") EXPECT_EQ(references(rule).size(), 1u);
    }
}
TEST(Modules, DetachFeaturesAreLocalToEachImporter) {
    ModuleFiles files;
    files.write("cpp.isc", cpp);
    files.write("full.isc", "import {declaration} from cpp; main: declaration;");
    files.write("lean.isc", "import * from cpp; detach feature cpp_constexpr; main: declaration;");
    files.write("main.isc", "import full as full; import lean as lean; main: full '|' lean;");
    auto tree = build(files.preprocess());
    const auto full = references(tree.getTreeMap().at(name({"modules", "full", "main"}))).front();
    const auto lean = references(tree.getTreeMap().at(name({"modules", "lean", "main"}))).front();
    EXPECT_NE(full, lean);
    EXPECT_FALSE(references(tree.getTreeMap().at(name(full))).empty());
    EXPECT_TRUE(references(tree.getTreeMap().at(name(lean))).empty());
    EXPECT_TRUE(tree.getTreeMap().contains(name({"modules", "cpp", "cpp_constexpr"})));
}
TEST(Modules, RemovingEntryReportsAnErrorAndOptionalOnlyRulesRemainEpsilon) {
    ModuleFiles files;
    files.write("cpp.isc", "feature optional { optional }; optional: 'x'; empty: optional?;");
    files.write("main.isc", "import * from cpp; detach feature optional; main: empty;");
    auto tree = build(files.preprocess());
    AST::TreePass pass(tree);
    pass.work();
    bool empty = false;
    for (const auto& [path, rule] : tree.getTreeMap()) if (path.back() == "empty") {
        EXPECT_TRUE(rule.rule_members.empty()); empty = true;
    }
    EXPECT_TRUE(empty);
    files.write("main.isc", "import * from cpp; detach optional; main: optional;");
    EXPECT_THROW(build(files.preprocess()), std::runtime_error);
}
TEST(Modules, StringsCommentsAndPythonDoNotCreateDependencies) {
    ModuleFiles files;
    files.write("main.isc", R"(module example -> main;
// import missing;
/* detach main; */
main: 'import absent; detach main;' [import]*;
action { text = "import missing;" }
)");
    auto project = files.preprocess();
    const auto& source = module(project, {"example"});
    EXPECT_TRUE(source.imports.empty());
    EXPECT_TRUE(source.detach.empty());
    EXPECT_EQ(source.declarations.at({"main"}).line, 4u);
    EXPECT_EQ(source.grammar.find("main:"), std::string("module example -> main;\n// import missing;\n/* detach main; */\n").size());
}
TEST(Modules, SourcesWithoutDirectivesKeepSharedLegacyScope) {
    ModuleFiles files;
    files.write("a.isc", "main: TOKEN;");
    files.write("b.isc", "TOKEN: 'x';");
    auto project = files.preprocess();
    EXPECT_FALSE(project.modular);
    auto tree = build(project);
    EXPECT_TRUE(tree.getTreeMap().contains(name({"main"})));
    EXPECT_EQ(references(tree.getTreeMap().at(name({"main"}))).front(), (Modules::Path{"TOKEN"}));
}

TEST(Modules, PythonImportsAreErasedWithoutChangingSourcePositions) {
    ModuleFiles files;
    const std::string text = "import py package.helpers as h;\nmain: 'x';";
    files.write("main.isc", text);
    const auto project = files.preprocess();
    const auto& source = module(project, {"main"});
    std::vector<Modules::Import> py_imports;
    for (const auto &i : source.imports) {
        if (i.python_import)
            py_imports.push_back(i);
    }
    ASSERT_EQ(py_imports.size(), 1u);
    EXPECT_EQ(py_imports.front().source, (Modules::Path{"package", "helpers"}));
    EXPECT_EQ(py_imports.front().alias, "h");
    EXPECT_EQ(source.imports.size(), py_imports.size());
    EXPECT_EQ(source.grammar.size(), text.size());
    EXPECT_EQ(source.grammar.find("main:"), text.find("main:"));
    EXPECT_EQ(source.declarations.at({"main"}).line, 2u);
}

TEST(Modules, PythonImportsRejectUnsupportedModulesAtTheirSource) {
    ModuleFiles files;
    files.write("main.isc", "import py helpers; main: 'x';");
    const auto python = files.write("helpers.py", "class Box:\n    pass\n");
    for (const auto* code : {
        "def helper() -> int:\n    class Box:\n        pass\n    return 1\n",
        "import math\n",
        "def helper(value):\n    return value\n",
        "def helper(value: int = 1) -> int:\n    return value\n",
        "value = [1, 2]\n"}) {
        files.write("helpers.py", code);
        try { build(files.preprocess()); FAIL() << code; }
        catch (const std::exception& error) {
            EXPECT_NE(std::string(error.what()).find(python + ":"), std::string::npos) << error.what();
        }
    }
}

TEST(Modules, PythonImportsValidateFilesExportsAndCallArguments) {
    ModuleFiles files;
    files.write("helpers.py", "def scale(value: int) -> int:\n    return value * 2\n");
    files.write("other.py", "OTHER = 1\n");
    for (const auto* code : {
        "import py absent; main: 'x';",
        "import py helpers as h; main { ctx.ast.value = h.absent(1) }",
        "import py helpers as h; main { ctx.ast.value = h.scale() }",
        "import py helpers as h; import py other as h; main: 'x';",
        "import py helpers as ctx; main: 'x';",
        "import py helpers as h; main { h = 1 }"}) {
        files.write("main.isc", code);
        EXPECT_THROW(build(files.preprocess()), std::invalid_argument) << code;
    }
}

TEST(Modules, PythonImportsResolveDottedPathsRelativeToTheirGrammarAndDeduplicateAliases) {
    ModuleFiles files;
    files.write("nested/main.isc", "import py package.helpers; import py package.helpers as h; main: 'x';");
    files.write("nested/package/helpers.py", "def scale(value: int) -> int:\n    return value * 2\n");
    auto tree = build(files.preprocess());
    ASSERT_EQ(tree.getPythonDeclarations().size(), 1u);
    const auto& python = tree.getPythonDeclarations().front().getNamespace();
    ASSERT_EQ(python.declarations.size(), 1u);
    const auto& package = python.declarations.front().getNamespace();
    EXPECT_EQ(package.name, "package");
    ASSERT_EQ(package.declarations.size(), 1u);
    EXPECT_EQ(package.declarations.front().getNamespace().name, "helpers");
}

TEST(Modules, PythonStdlibInternalsBindNativeParserNames) {
    ModuleFiles files;
    std::ifstream manifest(std::string(ROOT_DIR) + "/parser/stdlibs/python/internals.py");
    ASSERT_TRUE(manifest.good());
    files.write("internals.py", std::string(std::istreambuf_iterator<char>{manifest}, {}));
    files.write("helpers.py", R"py(from .internals import Token, RawToken, Tokens, TokensToString as token_name
def spelling(value: int) -> str:
    return token_name(value)
def identity(value: Token) -> Token:
    return value
def symbolic() -> int:
    return Tokens.EXAMPLE
def raw_identity(value: RawToken[int]) -> RawToken[int]:
    return value
)py");
    files.write("main.isc", "import py helpers; main: 'x';");
    auto tree = build(files.preprocess());
    ASSERT_EQ(tree.getPythonDeclarations().size(), 1u);
    const auto& python = tree.getPythonDeclarations().front().getNamespace();
    ASSERT_EQ(python.declarations.size(), 1u);
    const auto& helpers = python.declarations.front().getNamespace();
    ASSERT_EQ(helpers.declarations.size(), 4u);
    const auto& identity = helpers.declarations[1].getFunction();
    ASSERT_TRUE(identity.type.isSymbol());
    EXPECT_EQ(identity.type.getSymbol(), (LangAPI::Symbol{std::string("__ispa_parser__"), std::string("Token")}));
    const auto& raw = helpers.declarations[3].getFunction();
    ASSERT_TRUE(raw.type.isSymbol());
    EXPECT_EQ(raw.type.getSymbol(), (LangAPI::Symbol{std::string("ISPA_STD"), std::string("RawToken")}));
    ASSERT_EQ(raw.type.template_parameters.size(), 1u);
}

TEST(Modules, GeneratedParserCallsImportedPythonFunctionsAndVariables) {
    ModuleFiles files;
    std::ifstream manifest(std::string(ROOT_DIR) + "/parser/stdlibs/python/internals.py");
    ASSERT_TRUE(manifest.good());
    files.write("internals.py", std::string(std::istreambuf_iterator<char>{manifest}, {}));
    files.write("helpers.py", R"py("""Helpers compiled into the generated parser."""
from .internals import Token, RawToken, TokensToString
FACTOR = 2
OFFSET: int = 1
def add(value: int) -> int:
    return value + OFFSET
def scale(value: int) -> int:
    return add(value) * FACTOR
def local(value: int) -> int:
    FACTOR = 3
    return value * FACTOR
def noop() -> None:
    pass
def negate(value: int) -> int:
    return -value
def token_name(value: int) -> str:
    return TokensToString(value)
def token_identity(value: Token) -> Token:
    return value
def raw_identity(value: RawToken[int]) -> RawToken[int]:
    return value
class Box:
    def __init__(self, value: int):
        self.value: int = add(value) - OFFSET
    def get(self) -> int:
        return self.value
    def add(self, amount: int) -> int:
        self.value += amount
        return self.value
    @staticmethod
    def twice(value: int) -> int:
        return value * 2
class Child(Box):
    pass
def collections() -> int:
    box = Box(10)
    xs: list[int] = [3, 1, 2]
    xs.append(box.add(4))
    xs.extend(xs)
    xs.insert(0, 5)
    xs.remove(5)
    xs.sort(reverse=True)
    removed = xs.pop()
    ignored = box.get()
    count = xs.count(14)
    where = xs.index(14)
    xs.reverse()
    copied = xs.copy()
    copied.sort(key=negate, reverse=True)
    copied.clear()
    d: dict[str, int] = {'a': 1}
    d.update({'a': 3}, b=4)
    other = d.fromkeys(['unused'], 9)
    other.clear()
    from_type = dict.fromkeys(['unused'], 9)
    from_type.clear()
    existing = d.setdefault('a', 9)
    inserted = d.setdefault('c', 5)
    missing = d.get('missing', 7)
    popped = d.pop('b')
    fallback = d.pop('absent', 8)
    keys = d.keys()
    values = d.values()
    items = d.items()
    duplicate = d.copy()
    duplicate.clear()
    empty: list[int] = []
    empty.append(9)
    return removed + count + where + existing + inserted + missing + popped + fallback + empty.pop()
)py");
    files.write("main.isc", R"isc(import py helpers as h;
import py helpers;
main {
    match 'x';
    box = h.Box(3)
    match 'y';
    box.get()
    h.noop()
    ctx.ast.result = h.scale(20)
    ctx.ast.again = helpers.scale(20)
    ctx.ast.local = h.local(4)
    ctx.ast.factor = h.FACTOR
}
)isc");
    auto tree = build(files.preprocess());
    ASSERT_EQ(tree.getPythonDeclarations().size(), 1u);
    AST::TreePass pass(tree);
    pass.work();
    static Args settings{};
    dumper.setArgsPointer(&settings);
    LexerBuilder lexer(tree);
    lexer.build();
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    Algorithm::Metadata metadata;
    auto result = descriptor->build({tree, {}}, metadata);
    Algorithm::ConversionRequest request{lexer, tree, LangAPI::Language::Cpp, "PythonRuntime"};
    auto holder = result.to_lang_repr(result.output.get(), request);
    LangRepr::Converter converter(holder, "cpp", "PythonRuntime", false);
    converter.init();
    converter.construct();
    for (const auto& [name, text] : converter.get()) files.write("generated/" + name, text);
    files.write("generated/run.cpp", R"cpp(
#include "PythonRuntime.h"
#include <ispastdlib/python_collections.hpp>
#include <cassert>
int main() {
    PythonRuntime::Parser parser;
    const auto& ast = parser.parse("xy").value.data();
    if (ast.result != 42 || ast.again != 42 || ast.local != 12 || ast.factor != 2) return 1;
    if (python::helpers::scale(20) != 42) return 2;
    if (!parser.getErrors().empty()) return 3;
    python::helpers::Box box(3);
    if (box.add(4) != 7 || box.get() != 7 || python::helpers::Box::twice(5) != 10) return 4;
    if (python::helpers::collections() != 39) return 5;
    std::vector<int> values{3, 1, 2};
    ISPA_STD::python_extend(values, values);
    assert(values.size() == 6);
    ISPA_STD::python_insert(values, -100, 9);
    assert(ISPA_STD::python_pop(values, 0) == 9);
    assert(ISPA_STD::python_pop(values, -1) == 2);
    bool failed = false;
    try { ISPA_STD::python_index(values, 3, 3, 1); } catch (const std::invalid_argument&) { failed = true; }
    assert(failed);
    failed = false;
    try { ISPA_STD::python_pop(values, 100); } catch (const std::out_of_range&) { failed = true; }
    assert(failed);
    int calls = 0;
    std::vector<int> stable{12, 11, 22, 21};
    ISPA_STD::python_sort(stable, true, [&](int value) { ++calls; return value / 10; });
    assert(calls == 4 && stable == (std::vector<int>{22, 21, 12, 11}));
    auto dict = ISPA_STD::python_fromkeys(std::vector<std::string>{"x", "y"}, "value");
    assert(dict.at("x") == "value");
}
)cpp");
    const auto generated = files.root / "generated";
    const auto command = std::string("clang++ -std=c++23 -I\"") + ROOT_DIR +
        "/parser/stdlibs/target/target-cpp\" -I\"" + generated.string() + "\" \"" +
        (generated / "PythonRuntime.cpp").string() + "\" \"" +
        (generated / "run.cpp").string() + "\" \"" + ROOT_DIR +
        "/parser/stdlibs/target/target-cpp/src/core.cpp\" -o \"" + (generated / "run").string() + "\"";
    ASSERT_EQ(std::system(command.c_str()), 0);
    EXPECT_EQ(std::system(('"' + (generated / "run").string() + '"').c_str()), 0);
}

TEST(Modules, DetachingOptionalCapturesUpdatesDataFields) {
    ModuleFiles files;
    files.write("cpp.isc", "keyword: 'constexpr'; WORD: [a-z]+; declaration: @ keyword? @ WORD @{keyword, name};");
    files.write("main.isc", "import * from cpp; detach keyword; main: declaration;");
    auto tree = build(files.preprocess());
    auto target = references(tree.getTreeMap().at(name({"modules", "main", "main"}))).front();
    const auto& rule = tree.getTreeMap().at(name(target));
    ASSERT_EQ(rule.data_block.fields.size(), 1u);
    EXPECT_EQ(rule.data_block.fields.front().first, "name");
}

TEST(Modules, GeneratedParserKeepsFullAndDetachedVariants) {
    ModuleFiles files;
    files.write("cpp.isc", "import full; keyword: 'c'; feature constexpr {keyword}; declaration: keyword? 'x' full?;");
    files.write("full.isc", "import {declaration} from cpp; main: declaration;");
    files.write("lean.isc", "import * from cpp; detach feature constexpr; main: declaration;");
    files.write("main.isc", "import full; import lean; main: full '|' lean;");
    auto tree = build(files.preprocess());
    AST::TreePass pass(tree);
    pass.work();
    static Args settings{};
    dumper.setArgsPointer(&settings);
    LexerBuilder lexer(tree);
    lexer.build();
    dlib library("libispa-algorithm-LL");
    const auto* descriptor = library.load<Algorithm::EntryFn*>("ispa_algorithm_v1")();
    Algorithm::Metadata metadata;
    auto result = descriptor->build({tree, {}}, metadata);
    Algorithm::ConversionRequest request{lexer, tree, LangAPI::Language::Cpp, "ModuleRuntime"};
    auto holder = result.to_lang_repr(result.output.get(), request);
    LangRepr::Converter converter(holder, "cpp", "ModuleRuntime", false);
    converter.init();
    converter.construct();
    for (const auto& [name, text] : converter.get()) files.write("generated/" + name, text);
    files.write("generated/run.cpp", R"driver(
#include "ModuleRuntime.h"
int main() {
    ModuleRuntime::Parser parser;
    for (const char* text : {"cx|x", "x|x", "cxcx|xx"}) {
        try { parser.parse(text); if (!parser.getErrors().empty()) return 1; }
        catch (...) { return 2; }
    }
    try { parser.parse("cx|cx"); if (parser.getErrors().empty()) return 3; }
    catch (...) { return 0; }
    return 0;
}
)driver");
    const auto generated = files.root / "generated";
    const auto command = std::string("clang++ -std=c++23 -I\"") + ROOT_DIR +
        "/parser/stdlibs/target/target-cpp\" -I\"" + generated.string() + "\" \"" +
        (generated / "ModuleRuntime.cpp").string() + "\" \"" +
        (generated / "run.cpp").string() + "\" \"" + ROOT_DIR +
        "/parser/stdlibs/target/target-cpp/src/core.cpp\" -o \"" + (generated / "run").string() + "\"";
    ASSERT_EQ(std::system(command.c_str()), 0);
    EXPECT_EQ(std::system(('"' + (generated / "run").string() + '"').c_str()), 0);
}

TEST(Modules, DetachRemovesRecursiveCyclesThatLoseTheirBaseAlternative) {
    ModuleFiles files;
    files.write("lib.isc", "base: 'b'; recursive: recursive | base; caller: recursive; safe: 's';");
    files.write("main.isc", "import * from lib; detach base; main: safe;");
    auto tree = build(files.preprocess());
    for (const auto& [path, rule] : tree.getTreeMap()) {
        EXPECT_NE(path.back(), "recursive");
        EXPECT_NE(path.back(), "caller");
        EXPECT_NE(path.back(), "base");
    }
    EXPECT_TRUE(tree.getTreeMap().contains(name({"modules", "lib", "safe"})));
}

TEST(Modules, CyclicDetachPropagatesToAnAlreadyVisitedRoot) {
    ModuleFiles files;
    files.write("a.isc", "import {caller} from b; base: 'x'; required: base; detach base;");
    const auto root = files.write("b.isc", "import {required} from a; caller: required; main: 'b' caller?;");
    auto project = files.preprocess({root});
    auto tree = build(project);
    EXPECT_TRUE(references(tree.getTreeMap().at(name({"modules", "b", "main"}))).empty());
    for (const auto& [path, rule] : tree.getTreeMap()) {
        EXPECT_NE(path.back(), "caller");
        EXPECT_NE(path.back(), "required");
        EXPECT_NE(path.back(), "base");
        for (const auto& ref : references(rule)) EXPECT_TRUE(tree.getTreeMap().contains(name(ref)));
    }
}

TEST(Modules, GeneratedDependencyParserCollectsNestedDeclarationsAndSpans) {
    ModuleFiles files;
    const std::string text = R"isc(module /* header */ app.front -> main;
feature optional { main.nested };
// import missing;
#[inline] main&(arg): (';' | [;#{}]) @{arg}
    #nested: 'x';
;
action { value = {"text": "import absent; #nested: ';';"}
# A Python comment with unmatched delimiters: } )
value = """ } import absent; """
}
)isc";
    files.write("main.isc", text);
    auto project = files.preprocess();
    const auto& source = module(project, {"app", "front"});
    EXPECT_EQ(source.entry, (std::optional<Modules::Path>{{"main"}}));
    EXPECT_TRUE(source.declarations.contains({"main"}));
    EXPECT_TRUE(source.declarations.contains({"main", "nested"}));
    EXPECT_TRUE(source.declarations.contains({"action"}));
    EXPECT_EQ(source.features.at({"optional"}), (std::vector<Modules::Path>{{"main", "nested"}}));
    EXPECT_TRUE(source.imports.empty());
    EXPECT_EQ(source.declarations.at({"main", "nested"}).line, 5u);
    EXPECT_EQ(source.grammar.size(), text.size());
    for (std::size_t i = 0; i < text.size(); ++i)
        if (text[i] == '\n') EXPECT_EQ(source.grammar[i], '\n');
    EXPECT_EQ(source.grammar.find("#[inline]"), text.find("#[inline]"));
    EXPECT_EQ(source.grammar.find("#nested:"), text.find("#nested:"));
    EXPECT_EQ(source.grammar.find("p{"), text.find("p{"));
}

TEST(Modules, GeneratedDependencyParserRejectsMalformedOrTruncatedSyntax) {
    ModuleFiles files;
    for (const auto* text : {"module app main;", "import {x from lib;", "feature x {main}; main: ('x';",
                             "main: 'x'; trailing", "main: 'unterminated;", "main: [unterminated;",
                             "main: 'x'; /* unterminated", "main: 'x'; 'unterminated",
                             "main: 'x'; [unterminated", "action { p{ value = { }"}) {
        files.write("main.isc", text);
        EXPECT_THROW(files.preprocess(), std::runtime_error) << text;
    }
}

TEST(Modules, DependencyLexerAndGeneratedParserSupportTokenizedInput) {
    const char* text = "module app.front -> main; /* comment */ main: 'x';";
    IspaDepLexer lexer;
    lexer.setInput(text);
    lexer.makeTokens();
    IspaDepParser parser;
    const auto& tree = parser.parse(lexer);
    EXPECT_EQ(tree.value.data().value.size(), 2u);
    EXPECT_EQ(lexer.comments.size(), 1u);
}

TEST(Modules, GeneratedDependencyParserDistinguishesDirectivesFromRuleNames) {
    ModuleFiles files;
    files.write("lib.isc", "feature_rule: 'x';");
    files.write("main.isc", R"(module app.front -> main;
import {feature_rule} from lib;
module_rule: 'm';
import_rule: 'i';
detach_rule: 'd';
feature optional {feature_rule};
detach feature optional;
main: module_rule import_rule detach_rule;
)");
    const auto project = files.preprocess();
    const auto& source = module(project, {"app", "front"});
    EXPECT_TRUE(source.declarations.contains({"module_rule"}));
    EXPECT_TRUE(source.declarations.contains({"import_rule"}));
    EXPECT_TRUE(source.declarations.contains({"detach_rule"}));
    EXPECT_EQ(source.imports.front().symbols, (std::vector<Modules::Path>{{"feature_rule"}}));
    EXPECT_TRUE(source.detach.front().feature);
    EXPECT_TRUE(source.detached_symbols.contains({"lib", "feature_rule"}));
}

TEST(Modules, GrammarParserRetainsLazyLexerAcrossResultInspection) {
    IspaParser parser;
    EXPECT_NO_THROW(parser.parse("main: 'x';\n"));
    EXPECT_NO_THROW(parser.parse("main: 'y';\n"));
}
