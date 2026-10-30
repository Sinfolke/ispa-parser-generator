import Algorithm.API;
import AST.types;
import AST.BackendGuard;
import LLIR.Builder;
import LLIR.Serialize;
import ParserIR.API;
import ParserIR.IR;
import LangRepr.Construct;
import corelib;
import std;

namespace {
auto build(const Algorithm::BuildRequest& request, Algorithm::Metadata& metadata) -> Algorithm::Result {
    AST::requireCppSupport(request.tree, true);
    // FIRST/FOLLOW and inferred data types can depend on any other rule. Until
    // those dependencies are tracked, invalidate all productions on any change.
    std::ostringstream grammar;
    // Recovery sets include the complete reachable production suffix.
    grammar << "LL metadata lowering v14 rule parameters and templates\n";
    for (const auto& [name, rule] : request.tree.getTreeMap()) {
        grammar << corelib::text::join(name, ".") << '\n' << rule << '\n';
    }
    const auto source = grammar.str();
    // Action bodies and fail blocks are not represented by Rule's text dump.
    // Until the cache key includes their complete syntax, rebuild action grammars.
    const bool actions = !request.tree.getPythonDeclarations().empty() || !request.tree.getFailBlocks().empty() || std::ranges::any_of(request.tree.getTreeMap(),
        [](const auto& entry) { return !entry.second.features.empty(); });
    std::unordered_map<std::string, ParserIR::Production> cache;
    if (const auto stamp = metadata.cache.find("grammar");
        !actions && stamp != metadata.cache.end() && stamp->second == source) {
        if (const auto records = metadata.cache.find("productions"); records != metadata.cache.end()) {
            std::istringstream in(records->second);
            while (in.peek() != std::char_traits<char>::eof()) {
                auto production = LLIR::Serialize::readProductionRecord(in);
                if (!production) { cache.clear(); break; }
                cache.emplace(corelib::text::join(production->name, "."), std::move(*production));
            }
        }
    }
    AST::TreeMap unchanged;
    LLIR::Builder builder(request.tree, unchanged, cache, request.progress);
    auto ir = std::make_shared<ParserIR::IR>(builder.get());
    std::ostringstream records;
    for (const auto& production : ir->getData())
        LLIR::Serialize::writeProductionRecord(records, production);
    metadata.cache = {{"grammar", source}, {"productions", records.str()}};
    return {std::move(ir), [](void* output, Algorithm::ConversionRequest& request) {
        return LangRepr::Construct::construct(std::move(request.lexer),
            std::move(*static_cast<ParserIR::IR*>(output)), request.tree,
            request.language, request.namespace_name);
    }};
}
}

extern "C" auto ispa_algorithm_v1() -> const Algorithm::Descriptor* {
    static const Algorithm::Descriptor descriptor{Algorithm::interface_version, "LL",
        LLIR::Serialize::FORMAT_VERSION, build};
    return &descriptor;
}
