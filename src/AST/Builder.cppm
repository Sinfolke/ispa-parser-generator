module;
#include <output/Parser.h>
export module AST.Builder;
import AST.types;
import AST.API;
import AST.Tree;
import dstd;
import hash;
import UAST;
import UAST.Parser;
import Modules;
import LangAPI;

export namespace AST {
class Builder {
    TreeMap tree_map;
    stdu::vector<FailBlock> fail_blocks;
    LangAPI::Declarations python_declarations;
    stdu::vector<FrontendFeature> rule_features;
    SpacemodeStates spacemode = SpacemodeStates::MIXED;
    std::string name;
    UAST::main input;
    std::string file;
    std::vector<std::string> source_files;
    const Modules::Project* module_project = nullptr;
    std::size_t module_scope_size = 0;
    void specializeModules();
    stdu::vector<std::string> fullname;
    using Members = stdu::vector<std::shared_ptr<AST::RuleMember>>;
    using Value = UAST::rule_value;
    using Pattern = UAST::rule_value_pattern;
    using Grammar = UAST::grammar;
    using Rule = UAST::rule::regular_rule;

    AST::RuleMember convertPattern(const Pattern& pattern);
    AST::RuleMember convertValue(const Value& value);
    AST::RuleMember convertCapture(const UAST::rule_value_one_capture& value);
    AST::DataBlock convertData(const UAST::Node<UAST::rule_ast>& data);
    void convertGrammar(const Grammar& grammar);
    void convertRule(const Rule& rule);
    // `name | PythonClass:` rules: the rule's value is built by calling the imported class.
    struct TypedRule {
        std::string type;
        UAST::SourcePosition at;
        AST::SourcePosition position;
    };
    std::optional<TypedRule> typed_rule;
    void convertTypedData(const UAST::Node<UAST::rule_ast>& data, Members& members, const TypedRule& typed);
    void convertMain(const UAST::main& main);

public:
    // UAST::main is the grammar module entry. `file` only labels positions.
    explicit Builder(const UAST::main& mod, std::string file = {}) : input(mod), file(std::move(file)) {}
    explicit Builder(const Parser::Types::main& mod, std::string file = {})
        : Builder(mod.value.data(), std::move(file)) {}
    explicit Builder(const ::ISPA_STD::Node<Parser::Types::main>& mod, std::string file = {})
        : Builder(mod.data(), std::move(file)) {}
    // One source label per top-level input entry, for multi-file CLI inputs.
    void setSourceFiles(std::vector<std::string> files) {
        if (files.size() != input.value.size()) throw std::invalid_argument("AST source file count does not match input entries");
        source_files = std::move(files);
    }
    // Project/source storage must outlive build(). Parser nodes retain source slices.
    void setModules(const Modules::Project& project) { module_project = &project; }
    void build();
    auto get() -> Tree {
        Tree tree{std::move(name), std::move(spacemode), std::move(tree_map)};
        tree.getFailBlocks() = std::move(fail_blocks);
        tree.getPythonDeclarations() = std::move(python_declarations);
        return tree;
    }
};
}
