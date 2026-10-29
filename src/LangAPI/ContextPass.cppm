export module LangAPI.ContextPass;

import std;
import LangAPI;
import LangAPI.Context;
import LangAPI.Pass;
import LangAPI.Analysis;
import LangAPI.Traverse;

export namespace LangAPI {

// Facts borrow AST nodes and are valid only at the recorded context revision.
// Unknown is deliberately distinct from a proven absence of effects.
struct FunctionContextFacts {
    const Function* function = nullptr;
    std::uint64_t revision = 0;
    Analysis::ReferenceIndex references;
    std::map<std::string, Type> parameter_types;
    bool contains_throw = false;
    bool contains_call = false;
    bool effects_unknown = false;
    bool return_type_known = false;
    [[nodiscard]] bool validFor(const Context& context) const {
        return revision == context.revision();
    }
};

struct CallContextFacts {
    const FunctionCall* call = nullptr;
    std::uint64_t revision = 0;
    std::vector<const Function*> candidates;
    // Candidate discovery alone does not prove lexical or overload resolution.
    bool resolved = false;
};

class ContextPass final : public Passes::Pass {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "context-analysis"; }
    Passes::PassResult run(Context& context) override {
        context.clear<FunctionContextFacts>();
        context.clear<CallContextFacts>();
        std::map<std::string, std::vector<const Function*>> functions;
        std::size_t id = 0;
        Traverse::forEach<Function>(std::as_const(context.root()), [&](const Function& function) {
            functions[function.name].push_back(&function);
            auto& facts = context.emplace<FunctionContextFacts>(std::to_string(id++));
            facts.function = &function;
            facts.revision = context.revision();
            facts.references = Analysis::references(function.statements);
            facts.return_type_known = function.type != ValueType::Undef;
            for (const auto& [type, name] : function.parameters)
                facts.parameter_types.emplace(name, type);
            Traverse::forEach<Throw>(function.statements, [&](const Throw&) { facts.contains_throw = true; });
            Traverse::forEach<FunctionCall>(function.statements, [&](const FunctionCall&) { facts.contains_call = true; });
            Traverse::forEach<IspaLibFunctionCall>(function.statements, [&](const IspaLibFunctionCall&) { facts.contains_call = true; });
            facts.effects_unknown = facts.references.opaque || facts.contains_call;
        });
        id = 0;
        Traverse::forEach<FunctionCall>(std::as_const(context.root()), [&](const FunctionCall& call) {
            auto& facts = context.emplace<CallContextFacts>(std::to_string(id++));
            facts.call = &call;
            facts.revision = context.revision();
            if (const auto* symbol = std::get_if<std::shared_ptr<Symbol>>(&call.name); symbol && *symbol)
                if (const auto path = Analysis::symbolPath(**symbol)) {
                    auto found = functions.find(Analysis::leafName(*path));
                    if (found != functions.end()) facts.candidates = found->second;
                }
        });
        return {}; // Analysis updates metadata, never the AST revision.
    }
};
}
