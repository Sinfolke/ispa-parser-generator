module External.Exports;
import AST.API;
import Parser.Base;
import corelib;
import std;

namespace External {
std::vector<std::pair<std::string, std::string>> exports(AST::Tree& tree,
    const Modules::Project& project, const std::string& name, bool python) {
    std::ostringstream header, definitions, manifest, python_exports;
    if (!python) {
        header << "\n#include <ispastdlib/external.hpp>\nnamespace " << name << "::Exports {\n";
        definitions << "\n#include " << std::quoted(name + ".h") << '\n';
    }
    manifest << "ISPA_EXTERN 1\nheader " << std::quoted(name + (python ? ".py" : ".h")) << '\n';
    if (python) python_exports << "\nclass Exports:\n";
    std::size_t index = 0;
    const auto& first = tree.getFirstSet(1);
    for (const auto& source : project.sources) {
        auto canonical = [&](const Modules::Path& relative) {
            AST::LLkToken path;
            if (project.modular) {
                path.push_back("modules");
                path.insert(path.end(), source.name.begin(), source.name.end());
            }
            path.insert(path.end(), relative.begin(), relative.end());
            return path;
        };
        auto entry = source.entry;
        if (!entry && source.declarations.contains({"main"})) entry = Modules::Path{"main"};
        if (entry && !tree.contains(canonical(*entry))) entry.reset();
        manifest << "module " << std::quoted(Modules::spelling(source.name)) << ' '
            << std::quoted(entry ? Modules::spelling(*entry) : "") << '\n';
        for (const auto& [relative, position] : source.declarations) {
            const auto declared = canonical(relative);
            // A rule removed by TreePass::autoInline stays exported under its own name; it parses as the rule it forwards to.
            const auto path = tree.resolveAlias(declared);
            if (!tree.contains(path)) continue;
            // An external parser has no arguments to supply to a parameterized rule.
            bool needs_arguments = false;
            for (const auto& feature : tree[path].features)
                if (const auto* syntax = std::get_if<AST::DeclarationSyntax>(&feature))
                    needs_arguments = !syntax->parameters.empty() || !syntax->templateParameters.empty();
            if (needs_arguments) continue;
            const bool token = AST::RuleMemberName{.name = path}.isTerminal();
            const auto qualified = corelib::text::join(declared, "::");
            const auto binding = "symbol_" + std::to_string(index++);
            if (python) python_exports << "    " << binding << " = ispa_std.ExternalRule(Parser, Lexer, "
                << std::quoted(Parser::ParserBase::fun_name(path)) << ", Types."
                << corelib::text::join(declared, ".") << ", "
                << (token ? "True" : "False") << ")\n";
            manifest << "symbol " << std::quoted(Modules::spelling(source.name)) << ' '
                << std::quoted(Modules::spelling(relative)) << ' '
                << std::quoted(name + "::Types::" + qualified) << ' '
                << std::quoted(name + "::Exports::" + binding) << '\n';
            if (python) continue;
            header << "struct " << binding << " : ::" << name << "::Parser {\n"
                << "using node_type = ::" << name << "::Types::" << qualified << ";\n"
                << "static bool starts(const char* origin, const char* start);\n"
                << "static ISPA_STD::ExternalResult<node_type> parse(const char* origin, const char* start);\n};\n";
            std::set<AST::LLkToken> heads;
            bool nullable = false;
            if (token) heads.insert(path);
            else if (auto found = first.find(path); found != first.end())
                for (const auto& sequence : found->second) {
                    if (sequence.empty()) nullable = true;
                    else heads.insert(sequence.front());
                }
            definitions << "\nbool " << name << "::Exports::" << binding << "::starts(const char* origin, const char* start) {\n"
                << "if (!start) return false;\n"
                << "if (!*start) return " << (nullable ? "true" : "false") << ";\n"
                << "Lexer lexer; lexer.setInput(origin); const char* p = start;\n"
                << "auto token = lexer.makeToken(p);\n";
            if (tree.contains({"__WS"}) && !heads.contains({"__WS"}))
                definitions << "while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);\n";
            definitions << "return (!token.empty() && (false";
            for (const auto& head : heads) {
                if (tree.contains(head) && tree[head].external_token) continue;
                definitions << " || token.kind() == Tokens::" << corelib::text::join(head, "::") << "::id";
            }
            definitions << "))";
            for (const auto& head : heads) {
                if (!tree.contains(head) || !tree[head].external_token) continue;
                for (const auto& [proxy, rule] : tree.getTreeMap())
                    if (rule.external && rule.external->marker == head)
                        definitions << " || ::" << rule.external->cpp_binding << "::starts(origin, start)";
            }
            if (nullable) definitions << " || true";
            definitions << ";\n}\nISPA_STD::ExternalResult<" << name << "::Exports::" << binding << "::node_type> "
                << name << "::Exports::" << binding << "::parse(const char* origin, const char* start) {\n"
                << binding << " parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);\n";
            if (tree.contains({"__WS"}) && !heads.contains({"__WS"}))
                definitions << "parser.template skip_spaces<Types::__WS>(it);\n";
            definitions << "auto node = parser.template optional<node_type>(it";
            if (!token) definitions << ", [&parser](auto cursor) { return parser."
                << Parser::ParserBase::fun_name(path) << "(cursor); }";
            definitions << ");\nif (node.empty()) return {};\n"
                << "if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);\n"
                << "const char* end = it.sourcePosition();\n"
                << "return {true, ISPA_STD::external_node(origin, start, end, std::move(node)), end};\n}\n";
        }
    }
    if (python && index == 0) python_exports << "    pass\n";
    if (python) return {{name + ".ispa-export", manifest.str()}, {name + ".py", python_exports.str()}};
    header << "}\n";
    std::vector<std::pair<std::string, std::string>> result{
        {name + ".h", header.str()}, {name + ".cpp", definitions.str()}, {name + ".ispa-export", manifest.str()}};
    return result;
}
}
