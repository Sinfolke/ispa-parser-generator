module;
#include <tsl/ordered_map.h>
#include <ispa/parser.h>
module ParserIR.API;
import AST.Builder;
import AST.Tree;
import Parser.Base;
import UAST.PyAST;
import corelib;
import std;
namespace ParserIR {
namespace {
namespace py = UAST::py;
namespace la = LangAPI;
struct ActionFields {
    AST::Tree* tree = nullptr;
    std::map<std::string, la::Type> locals;
    std::map<std::string, la::Type> imported_types;
    inclosed_map fields;
    void imports(const la::Declarations& declarations, const std::string& scope = {}) {
        for (const auto& declaration : declarations) {
            if (declaration.isNamespace()) {
                const auto& ns = declaration.getNamespace();
                imports(ns.declarations, scope + ns.name + ".");
            } else if (declaration.isFunction()) {
                const auto& f = declaration.getFunction();
                imported_types.emplace(scope + f.name, f.type);
            } else if (declaration.isVariable()) {
                const auto& v = declaration.getVariable();
                imported_types.emplace(scope + v.name, v.type);
            }
        }
    }
    [[noreturn]] static void error(const UAST::SourcePosition& p, const std::string& message) {
        throw std::invalid_argument(p.file + ":" + std::to_string(p.line) + ":" + std::to_string(p.column) + ": " + message);
    }
    static std::string astField(const py::ExprPtr& expression) {
        if (!expression) return {};
        const auto* field = expression->get<py::Attribute>();
        const auto* ast = field && field->value ? field->value->get<py::Attribute>() : nullptr;
        const auto* ctx = ast && ast->value ? ast->value->get<py::Name>() : nullptr;
        return ctx && ctx->id == "ctx" && ast->attr == "ast" ? field->attr : std::string{};
    }
    la::Type annotation(const py::ExprPtr& expression) {
        if (expression) if (auto name = expression->get<py::Name>()) {
            if (name->id == "int") return la::ValueType::Int;
            if (name->id == "float") return la::ValueType::Float;
            if (name->id == "str") return la::ValueType::String;
            if (name->id == "bool") return la::ValueType::Bool;
        }
        error(expression ? expression->pos : UAST::SourcePosition{}, "ctx.ast field annotation must be int, float, str or bool");
    }
    la::Type infer(const py::ExprPtr& expression) {
        if (!expression) return la::ValueType::Undef;
        return std::visit([&](const auto& x) -> la::Type {
            using T = std::decay_t<decltype(x)>;
            if constexpr (std::is_same_v<T, py::Constant>) {
                switch (x.kind) {
                    case py::ConstantKind::Int: return la::ValueType::Int;
                    case py::ConstantKind::Float: return la::ValueType::Float;
                    case py::ConstantKind::Bool: return la::ValueType::Bool;
                    case py::ConstantKind::Str: return la::ValueType::String;
                    default: return la::ValueType::Undef;
                }
            } else if constexpr (std::is_same_v<T, py::Name>) {
                if (locals.contains(x.id)) return locals.find(x.id)->second;
                if (imported_types.contains(x.id)) return imported_types.find(x.id)->second;
            } else if constexpr (std::is_same_v<T, py::Call>) {
                if (auto name = x.func ? x.func->template get<py::Name>() : nullptr;
                    name && imported_types.contains(name->id)) return imported_types.find(name->id)->second;
            } else if constexpr (std::is_same_v<T, py::Compare> || std::is_same_v<T, py::BoolExpr>) return la::ValueType::Bool;
            else if constexpr (std::is_same_v<T, py::UnaryExpr>) return x.op == py::UnaryOp::Not ? la::Type{la::ValueType::Bool} : infer(x.operand);
            else if constexpr (std::is_same_v<T, py::BinaryExpr>) {
                auto left = infer(x.left), right = infer(x.right);
                if (left == la::ValueType::Float || right == la::ValueType::Float) return la::ValueType::Float;
                return left;
            } else if constexpr (std::is_same_v<T, py::List>) {
                if (!x.elts.empty()) {
                    auto element = infer(x.elts.front());
                    if (element != la::ValueType::Undef) return la::Type{la::ValueType::Array, element};
                }
            } else if constexpr (std::is_same_v<T, py::Attribute>) {
                const auto field = astField(expression);
                if (fields.contains(field)) return fields.find(field)->second.second;
                if (auto n = x.value ? x.value->template get<py::Name>() : nullptr; n && n->id == "ctx" &&
                    (x.attr == "rule" || x.attr == "failed_rule")) return la::ValueType::String;
            }
            return la::ValueType::Undef;
        }, expression->kind);
    }
    void assign(const py::ExprPtr& target, const py::ExprPtr& value, const py::ExprPtr& annotated = {}) {
        const auto field = astField(target);
        auto type = annotated ? annotation(annotated) : infer(value);
        if (type == la::ValueType::Void) error(target->pos, "a function returning None cannot initialize an AST field");
        if (!field.empty()) {
            if (fields.contains(field)) {
                if (annotated && !(fields.find(field)->second.second == type)) error(target->pos, "conflicting annotation for ctx.ast." + field);
                return;
            }
            if (type == la::ValueType::Undef)
                error(target->pos, "cannot infer ctx.ast." + field + "; add a field annotation");
            fields.emplace(field, regular_data_block{{}, std::move(type)});
        } else if (auto name = target ? target->get<py::Name>() : nullptr) locals.insert_or_assign(name->id, std::move(type));
    }
    void body(const py::StmtList& statements) {
        for (const auto& statement : statements) if (statement) std::visit([&](const auto& x) {
            using T = std::decay_t<decltype(x)>;
            if constexpr (std::is_same_v<T, py::Assign>) { for (const auto& target : x.targets) assign(target, x.value); }
            else if constexpr (std::is_same_v<T, py::AnnAssign>) assign(x.target, x.value, x.annotation);
            else if constexpr (std::is_same_v<T, py::IspaMatch>) {
                const std::string source = "embedded_action { " + x.source + " }";
                IspaParser parser;
                const auto& syntax = parser.parse(source.c_str());
                AST::Builder builder(syntax, "<embedded match>");
                builder.build();
                auto embedded = builder.get();
                const auto& members = embedded.getTreeMap().at(stdu::vector<std::string>{"embedded_action"}).rule_members;
                const auto resolve = [&](stdu::vector<std::string> name) {
                    if (tree && !tree->contains(name)) {
                        for (std::size_t count = current_rule.size(); count != 0; --count) {
                            stdu::vector<std::string> candidate(current_rule.begin(), current_rule.begin() + count);
                            candidate.insert(candidate.end(), name.begin(), name.end());
                            if (tree->contains(candidate)) return candidate;
                        }
                    }
                    return name;
                };
                const auto capture = [&](auto&& self, const AST::RuleMember& member) -> void {
                    for (const auto& feature : member.features) {
                        if (const auto* stream = std::get_if<AST::StreamMatch>(&feature); stream && !stream->capture.empty()) {
                            AST::RuleMember target;
                            target.value = AST::RuleMemberName{.name = resolve(stream->rule)};
                            locals.insert_or_assign(stream->capture, Parser::ParserBase::dataType(AST::captureType(target)));
                        }
                        if (const auto* variable = std::get_if<AST::VariableCapture>(&feature)) {
                            AST::RuleMember target = member;
                            if (target.isName()) target.getName().name = resolve(target.getName().name);
                            if (target.isString() && tree) {
                                const auto literal = target.getString().value;
                                for (const auto& [name, rule] : tree->getTreeMap()) {
                                    if (name.empty() || !corelib::text::isUpper(name.back()) || rule.rule_members.size() != 1) continue;
                                    const auto& token = *rule.rule_members.front();
                                    if (token.isString() && token.getString().value == literal) {
                                        target.value = AST::RuleMemberName{.name = name};
                                        break;
                                    }
                                }
                            }
                            locals.insert_or_assign(variable->name, Parser::ParserBase::dataType(AST::captureType(target)));
                        }
                    }
                    if (member.isGroup()) for (const auto& child : member.getGroup().values) self(self, *child);
                    if (member.isOp()) for (const auto& child : member.getOp().options) self(self, *child);
                };
                for (const auto& member : members) capture(capture, *member);
            }
            else if constexpr (std::is_same_v<T, py::If> || std::is_same_v<T, py::While>) { body(x.body); body(x.orelse); }
        }, statement->kind);
    }
    stdu::vector<std::string> current_rule;
    void action(const AST::PythonAction& action) {
        if (const auto* module = std::get_if<py::Module>(&action.code)) body(module->body);
        else if (const auto* expression = std::get_if<py::Expr>(&action.code); expression && !action.target.empty()) {
            auto type = infer(std::make_shared<py::Expr>(*expression));
            if (type == la::ValueType::Undef)
                error(expression->pos, "cannot infer AST field " + action.target);
            fields.insert_or_assign(action.target, regular_data_block{{}, std::move(type)});
        }
    }
    void member(const AST::RuleMember& member) {
        for (const auto& feature : member.features) {
            if (const auto* capture = std::get_if<AST::VariableCapture>(&feature))
                locals.insert_or_assign(capture->name, Parser::ParserBase::dataType(AST::captureType(member)));
            if (const auto* stream = std::get_if<AST::StreamMatch>(&feature); stream && !stream->capture.empty()) {
                AST::RuleMember target;
                target.value = AST::RuleMemberName{.name = stream->rule};
                locals.insert_or_assign(stream->capture, Parser::ParserBase::dataType(AST::captureType(target)));
            }
            if (const auto* code = std::get_if<AST::PythonAction>(&feature)) action(*code);
        }
        if (member.isGroup()) for (const auto& child : member.getGroup().values) this->member(*child);
        if (member.isOp()) for (const auto& child : member.getOp().options) this->member(*child);
    }
};
}
auto withActionFields(AST::Tree& tree, const stdu::vector<std::string>& name, const AST::Rule& rule, DataBlock block) -> DataBlock {
    struct { DataBlock block; } production{std::move(block)};
    ActionFields action_fields;
    action_fields.tree = &tree;
    action_fields.current_rule = name;
    action_fields.imports(tree.getPythonDeclarations());
    for (const auto& feature : rule.features)
        if (const auto* syntax = std::get_if<AST::DeclarationSyntax>(&feature))
            for (std::size_t i = 0; i < syntax->parameters.size() && i < syntax->parameterAnnotations.size(); ++i)
                if (const auto* annotation = std::get_if<py::Expr>(&syntax->parameterAnnotations[i].code))
                    action_fields.locals.emplace(syntax->parameters[i],
                        Parser::ParserBase::convertPythonType(std::make_shared<py::Expr>(*annotation)));
    if (production.block.is_inclosed_map()) action_fields.fields = production.block.getInclosedMap();
    for (const auto& member : rule.rule_members) action_fields.member(*member);
    for (const auto& feature : rule.features) if (const auto* action = std::get_if<AST::PythonAction>(&feature)) action_fields.action(*action);
    // A handler shares the AST shape of its owning rule. Include fields
    // introduced by handlers selected by markers or rule-level bindings.
    std::map<stdu::vector<std::string>, stdu::vector<std::string>> handler_names;
    for (const auto& feature : rule.features) if (const auto* binding = std::get_if<AST::FailBinding>(&feature))
        for (const auto& name : binding->names) handler_names.emplace(stdu::vector<std::string>{name}, stdu::vector<std::string>{});
    const auto gather = [&](auto&& self, const AST::RuleMember& member) -> void {
        for (const auto& feature : member.features) if (const auto* marker = std::get_if<AST::FailMarker>(&feature); marker && !marker->name.empty())
            handler_names.insert_or_assign(marker->name, marker->parameters);
        if (member.isGroup()) for (const auto& child : member.getGroup().values) self(self, *child);
        if (member.isOp()) for (const auto& child : member.getOp().options) self(self, *child);
    };
    for (const auto& member : rule.rule_members) gather(gather, *member);
    for (const auto& [handler, arguments] : handler_names) {
        const AST::FailBlock* resolved = nullptr;
        for (std::size_t size = name.size();; --size) {
            stdu::vector<std::string> candidate(name.begin(), name.begin() + size);
            candidate.insert(candidate.end(), handler.begin(), handler.end());
            for (const auto& block : tree.getFailBlocks()) if (block.name == candidate) { resolved = &block; break; }
            if (resolved || size == 0) break;
        }
        if (resolved) {
            for (std::size_t i = 0; i < arguments.size() && i < resolved->parameters.size(); ++i)
                if (action_fields.locals.contains(arguments[i])) action_fields.locals.insert_or_assign(resolved->parameters[i], action_fields.locals.find(arguments[i])->second);
            action_fields.action(resolved->action);
        }
    }
    if (!action_fields.fields.empty()) {
        if (production.block.is_regular_data_block() && !action_fields.fields.contains("value"))
            action_fields.fields.emplace("value", production.block.getRegularDataBlock());
        production.block.value = std::move(action_fields.fields);
    }
    return std::move(production.block);
}
}
