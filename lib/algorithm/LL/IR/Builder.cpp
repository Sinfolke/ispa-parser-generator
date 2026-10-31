module;
#include <tsl/ordered_map.h>
#include <ispa/parser.h>
module LLIR.Builder;
import AST.Builder;
import RuleLower;
import Parser.Base;
import UAST.PyAST;
import corelib;
import std;
namespace LLIR {
using namespace ParserIR;
Builder::Builder(AST::Tree& tree, int) : Builder(tree, AST::TreeMap{}, {}) {}

Builder::Builder(AST::Tree& tree, const AST::TreeMap& changed,
                 const std::unordered_map<std::string, Production>& cache,
                 const std::function<void(std::size_t, std::size_t)>& progress) {
    Parser::Ctx ctx{};
    RuleLower lower(ctx, tree);
    const auto total = std::ranges::count_if(tree.getTreeMap(), [](const auto& item) {
        return !item.second.isTemplatePlaceholder && !item.second.isTemplateDefinition && !corelib::text::isUpper(item.first.back());
    });
    std::size_t completed = 0;
    for (const auto& [name, rule] : tree) {
        if (progress) progress(completed, total);
        if (rule.isTemplatePlaceholder || rule.isTemplateDefinition) continue;
        if (corelib::text::isUpper(name.back())) continue;
        ++completed;
        if (rule.external) {
            Production production;
            production.name = name;
            data.push_back(std::move(production));
            continue;
        }
        const auto cached = cache.find(corelib::text::join(name, "."));
        if (!changed.contains(name) && cached != cache.end()) {
            data.push_back(cached->second);
            continue;
        }
        lower.build(name, rule);
        Production production;
        production.name = name;
        production.block = dataBlock(rule.data_block);
        const auto& lowered = lower.lowered_functions_meta().at(name);
        // The body is opaque to the Builder: all value bindings come from the
        // semantic exports, regardless of declaration placement or control flow.
        production.rule_members = lowered.function.statements;
        production.block = ParserIR::withActionFields(tree, name, rule, std::move(production.block));
        std::size_t capture = 0;
        for (const auto& exported : lowered.captures) {
            if (exported.prefix.empty()) continue;
            // Preserve capture metadata for IR consumers. ConstructParser uses
            // the live assignments emitted by RuleLower for these fields.
            const auto& value = exported.meta.value.name.empty() ? exported.meta.value_list : exported.meta.value;
            auto expression = LangAPI::Symbol::createExpression(LangAPI::Symbol{value.name});
            if (production.block.is_regular_data_block())
                production.block.getRegularDataBlock().first = std::move(expression);
            else if (production.block.is_inclosed_map()) {
                if (rule.data_block.type) production.block.getInclosedMap().find("value").value().first = std::move(expression);
                else if (capture < rule.data_block.fields.size()) {
                    const auto& field = rule.data_block.fields[capture].first;
                    production.block.getInclosedMap().find(field).value().first = std::move(expression);
                }
            }
            ++capture;
        }
        data.push_back(std::move(production));
    }
    if (progress) progress(completed, total);
}
auto Builder::get() -> IR { return IR{std::move(data), false, std::move(dfas)}; }
auto Builder::getRawData() -> stdu::vector<Production>& { return data; }
auto Builder::getRawData() const -> const stdu::vector<Production>& { return data; }
}
