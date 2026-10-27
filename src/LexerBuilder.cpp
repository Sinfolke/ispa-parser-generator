module;
#include <tsl/ordered_map.h>
module LexerBuilder;
import corelib;
import Parser.Base;
import NFA.IR;
import NFA.IR.Interpreter;
import NFA.TNFA;
import NFA.TNFA.Interpreter;
import NFA.TNFA.API;
import DFA.API;
import DFA;
import Dump;
import args;
import logging;
import constants;
import UAST.PyAST;
import cpuf.op;
import cpuf.printf;
import std;

void accumulateNestedNames(stdu::vector<std::shared_ptr<AST::RuleMember>> members, stdu::vector<stdu::vector<std::string>> &names) {
    for (const auto &mem_ptr : members) {
        auto &mem = *mem_ptr;
        if (mem.isGroup())
            accumulateNestedNames(mem.getGroup().values, names);
        if (mem.isOp())
            accumulateNestedNames(mem.getOp().options, names);
        if (mem.isName())
            names.push_back(mem.getName().name);
    }
}
bool LexerBuilder::isTopLevel(const stdu::vector<std::string> &name) {
    auto &use_places = ast.getUsePlacesTable();
    if (!use_places.contains(name))
        return true;
    bool used_in_non_terminal = false;
    for (const auto &place : use_places.at(name)) {
        if (corelib::text::isLower(place.back())) {
            used_in_non_terminal = true;
            break;
        }
    }
    return used_in_non_terminal;
}
void LexerBuilder::build() {
    NFA::InitialNFA initial_nfa(ast);
    initial_nfa.build();
    initial_nfa.unrollGroups();
    initial_nfa.unrollQuantifiers();
    initial_nfa.unrollNestedTokens();
    // 1. Generate exhaustive test cases using AstInputGenerator
    auto test_cases = ast.generateRandomTokenInputs(2);

    // 2. Instantiate interpreter

    // 3. Execute test inputs and print AST trees
    for (const auto &[rule_name, samples] : test_cases) {
        Tlog::Branch b(logger, "NFA-IR-TEST/" + corelib::text::join(rule_name, "/") + ".log");
        if (!initial_nfa.get().contains(rule_name)) {
            b.log("<optimized out of Top-List rules>");
            continue;
        }
        NFA::Interpreter::IRInterpreter interpreter(initial_nfa.get().at(rule_name));
        for (const auto &sample : samples) {
            logger.log("sample {}", sample);
            auto result = interpreter.parse(sample);
            logger.log("result: {}", result);
        }
    }
    if (dumper.shouldDump("NFA-IR")) {
        std::ofstream dumpNFAFile(dumper.makeDumpPath("NFA-IR"));
        if (!dumpNFAFile.is_open())
            throw Error("failed to open NFA-IR for dump");
        dumpNFAFile << initial_nfa;
    }

    // build actual NFA from NFA IR
    NFA::TNFA::TNFABuilder builder(initial_nfa, debug);
    builder.build();
    if (dumper.shouldDump("NFA")) {
        std::ofstream dumpNFAFile(dumper.makeDumpPath("NFA"));
        if (!dumpNFAFile.is_open())
            throw Error("failed to open NFA for dump");
        dumpNFAFile << builder;
    }
    // 1. Instantiate interpreter ONCE outside the loop to avoid copying state vectors repeatedly
    NFA::Interpreter::TNFAInterpreter interpreter(builder.getStates());

    for (const auto &[rule_name, samples]: test_cases) {
        auto entry = builder.getEntry(rule_name);
        if (!entry.has_value()) {
            continue;
        }

        // 3. Avoid nested subdirectories in filenames unless directories are created explicitly
        const std::string sanitize_rule = corelib::text::join(rule_name, "_");
        Tlog::Branch b(logger, "TNFA-TEST/" + sanitize_rule + ".log");

        for (const auto &sample: samples) {
            logger.log("sample {}", sample);

            // 4. Pass 'sample' directly: sample.c_str() forces an unnecessary O(N) strlen calculation
            auto result = interpreter.run(entry.value(), sample);
            logger.log("result: {}", result);
        }
    }
    DFA::DFA dfa(&builder);
    dfa.build();
    dfa.minimize();
    auto classified = dfa.classify();
    this->dfa = std::move(classified);
    action_table = dfa.getActionTable();
    semantic_table = dfa.getSemanticTable();
    registers_count = dfa.getRegisterCount();
    output_count = dfa.getOutputCount();

}
auto LexerBuilder::getDataBlocks() const -> ParserIR::DataBlockList {
    ParserIR::DataBlockList list;
    namespace py = UAST::py;
    const auto fieldName = [](const py::ExprPtr& expression) -> std::string {
        auto field = expression ? expression->get<py::Attribute>() : nullptr;
        auto ast = field && field->value ? field->value->get<py::Attribute>() : nullptr;
        auto ctx = ast && ast->value ? ast->value->get<py::Name>() : nullptr;
        return ctx && ctx->id == "ctx" && ast->attr == "ast" ? field->attr : std::string{};
    };
    for (const auto& [name, rule] : ast) {
        if (rule.external_token || corelib::text::isLower(name.back())) continue;
        auto data = ParserIR::dataBlock(rule.data_block);
        std::map<std::string, LangAPI::Type> locals;
        for (const auto& feature : rule.features)
            if (const auto* syntax = std::get_if<AST::DeclarationSyntax>(&feature))
                for (std::size_t i = 0; i < syntax->parameters.size() && i < syntax->parameterAnnotations.size(); ++i)
                    if (const auto* annotation = std::get_if<py::Expr>(&syntax->parameterAnnotations[i].code))
                        locals.emplace(syntax->parameters[i],
                            Parser::ParserBase::convertPythonType(std::make_shared<py::Expr>(*annotation)));
        const auto capture_locals = [&](auto&& self, const AST::RuleMember& member) -> void {
            for (const auto& feature : member.features)
                if (const auto* capture = std::get_if<AST::VariableCapture>(&feature))
                    locals.insert_or_assign(capture->name, LangAPI::ValueType::String);
            if (member.isGroup())
                for (const auto& child : member.getGroup().values) self(self, *child);
            if (member.isOp())
                for (const auto& child : member.getOp().options) self(self, *child);
        };
        for (const auto& member : rule.rule_members) capture_locals(capture_locals, *member);
        auto infer = [&](auto&& self, const py::ExprPtr& value) -> LangAPI::Type {
            if (!value) return LangAPI::ValueType::Undef;
            if (auto literal = value->get<py::Constant>()) {
                switch (literal->kind) {
                    case py::ConstantKind::Int: return LangAPI::ValueType::Int;
                    case py::ConstantKind::Float: return LangAPI::ValueType::Float;
                    case py::ConstantKind::Bool: return LangAPI::ValueType::Bool;
                    case py::ConstantKind::Str: return LangAPI::ValueType::String;
                    default: return LangAPI::ValueType::Undef;
                }
            }
            if (auto variable = value->get<py::Name>()) {
                if (auto found = locals.find(variable->id); found != locals.end()) return found->second;
            }
            if (auto unary = value->get<py::UnaryExpr>())
                return unary->op == py::UnaryOp::Not ? LangAPI::Type{LangAPI::ValueType::Bool} : self(self, unary->operand);
            if (auto binary = value->get<py::BinaryExpr>()) {
                auto left = self(self, binary->left), right = self(self, binary->right);
                return left == LangAPI::ValueType::Float || right == LangAPI::ValueType::Float ?
                    LangAPI::Type{LangAPI::ValueType::Float} : left;
            }
            if (value->get<py::Compare>() || value->get<py::BoolExpr>()) return LangAPI::ValueType::Bool;
            if (auto field = fieldName(value); !field.empty() && data.is_inclosed_map()) {
                auto& fields = data.getInclosedMap();
                if (auto found = fields.find(field); found != fields.end()) return found->second.second;
            }
            return LangAPI::ValueType::Undef;
        };
        const auto add = [&](const py::ExprPtr& target, const py::ExprPtr& value,
                             const py::ExprPtr& annotation) {
            auto type = infer(infer, value);
            const auto field = fieldName(target);
            const auto variable = target ? target->get<py::Name>() : nullptr;
            if (!field.empty() || variable) {
                if (annotation) {
                    auto named = annotation->get<py::Name>();
                    if (!named) throw std::invalid_argument("token field annotation must be int, float, bool or str");
                    if (named->id == "int") type = LangAPI::ValueType::Int;
                    else if (named->id == "float") type = LangAPI::ValueType::Float;
                    else if (named->id == "bool") type = LangAPI::ValueType::Bool;
                    else if (named->id == "str") type = LangAPI::ValueType::String;
                    else throw std::invalid_argument("token field annotation must be int, float, bool or str");
                }
            }
            if (!field.empty()) {
                if (type == LangAPI::ValueType::Undef)
                    throw std::invalid_argument("cannot infer token ctx.ast." + field + "; add a field annotation");
                if (data.is_regular_data_block())
                    throw std::invalid_argument("token action fields cannot extend a scalar token value");
                if (!data.is_inclosed_map()) data.value = ParserIR::inclosed_map{};
                auto& fields = data.getInclosedMap();
                if (auto found = fields.find(field); found != fields.end()) {
                    if (annotation && found->second.second != type)
                        throw std::invalid_argument("conflicting annotation for token ctx.ast." + field);
                } else fields.emplace(field, ParserIR::regular_data_block{{}, type});
            } else if (variable)
                locals.insert_or_assign(variable->id, std::move(type));
        };
        const auto walk = [&](auto&& self, const py::StmtList& statements) -> void {
            for (const auto& statement : statements) if (statement) std::visit([&](const auto& item) {
                using T = std::decay_t<decltype(item)>;
                if constexpr (std::is_same_v<T, py::Assign>) {
                    for (const auto& target : item.targets) add(target, item.value, {});
                } else if constexpr (std::is_same_v<T, py::AnnAssign>) add(item.target, item.value, item.annotation);
                else if constexpr (std::is_same_v<T, py::If> || std::is_same_v<T, py::While>) {
                    self(self, item.body); self(self, item.orelse);
                }
            }, statement->kind);
        };
        for (const auto& feature : rule.features) if (auto action = std::get_if<AST::PythonAction>(&feature)) {
            if (auto module = std::get_if<py::Module>(&action->code)) walk(walk, module->body);
            else if (auto expression = std::get_if<py::Expr>(&action->code); expression && !action->target.empty()) {
                const auto type = infer(infer, std::make_shared<py::Expr>(*expression));
                if (type == LangAPI::ValueType::Undef)
                    throw std::invalid_argument("cannot infer token AST field " + action->target);
                if (!data.is_inclosed_map()) data.value = ParserIR::inclosed_map{};
                data.getInclosedMap().insert_or_assign(action->target, ParserIR::regular_data_block{{}, type});
            }
        }
        list.emplace(name, std::move(data));
    }
    return list;
}
