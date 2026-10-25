module;
#include <ispa/parser.h>
module MemLower;
import AST.Builder;
import logging;
import corelib;
import constants;
import std;

namespace {
    std::set<std::string> pythonClasses(const LangAPI::Declarations& declarations, const std::string& prefix = "") {
        std::set<std::string> names;
        for (const auto& declaration : declarations) {
            if (declaration.isNamespace()) {
                const auto& ns = declaration.getNamespace();
                const auto nested = pythonClasses(ns.declarations, prefix + ns.name + ".");
                names.insert(nested.begin(), nested.end());
            } else if (declaration.isClass()) {
                const auto& cls = declaration.getClass();
                std::string name = prefix;
                for (const auto& part : cls.name.path) if (const auto* value = std::get_if<std::string>(&part)) {
                    if (name.size() > prefix.size()) name += ".";
                    name += *value;
                }
                names.insert(name);
            }
        }
        return names;
    }
    // A repeated production may itself own indentation/newline tokens. Do not
    // consume its separator before entering that production on the next pass.
    bool explicitWhitespaceBoundary(const AST::Tree& tree, const AST::RuleMember& member,
                                    std::set<AST::LLkToken>& visiting, bool leading = true) {
        if (member.isName()) {
            const auto& name = member.getName().name;
            if (name.empty()) return false;
            if (name.back() == "__WS" || name.back() == "NEWLINE") return true;
            if (tree.contains(name) && tree[name].external) return leading;
            if (!member.getName().isNonterminal() || !tree.contains(name) || !visiting.insert(name).second) return false;
            const auto& values = tree.getTreeMap().at(name).rule_members;
            const bool result = !values.empty() &&
                (explicitWhitespaceBoundary(tree, *values.front(), visiting, leading) ||
                 explicitWhitespaceBoundary(tree, *values.back(), visiting, false));
            visiting.erase(name);
            return result;
        }
        if (member.isGroup()) {
            const auto& values = member.getGroup().values;
            return !values.empty() &&
                (explicitWhitespaceBoundary(tree, *values.front(), visiting, leading) ||
                 explicitWhitespaceBoundary(tree, *values.back(), visiting, false));
        }
        if (member.isOp()) {
            // A mixed external/host choice still needs host whitespace between
            // iterations of its ordinary alternatives.
            return !member.getOp().options.empty() && std::ranges::all_of(member.getOp().options,
                [&](const auto& child) { return explicitWhitespaceBoundary(tree, *child, visiting, leading); });
        }
        return false;
    }
    bool explicitWhitespaceBoundary(const AST::Tree& tree, const AST::RuleMember& member) {
        std::set<AST::LLkToken> visiting;
        return explicitWhitespaceBoundary(tree, member, visiting);
    }
    // Redirects fun_ctx.statements into `target` for the lifetime of the scope.
    struct StatementsScope {
        FunctionCtx &ctx;
        LangAPI::Statements *saved;
        StatementsScope(FunctionCtx &ctx, LangAPI::Statements &target) : ctx(ctx), saved(ctx.statements) {
            ctx.statements = &target;
        }
        ~StatementsScope() { ctx.statements = saved; }
        StatementsScope(const StatementsScope &) = delete;
        StatementsScope &operator=(const StatementsScope &) = delete;
    };

    auto appendExpr(LangAPI::Expression &dst, const LangAPI::Expression &src) -> void {
        dst.insert(dst.end(), src.begin(), src.end());
    }

    auto op(LangAPI::ExpressionElement e) -> LangAPI::ExpressionValue {
        return LangAPI::ExpressionValue {e};
    }

    auto notExpr(const std::string &var) -> LangAPI::Expression {
        return LangAPI::Expression {
            op(LangAPI::ExpressionElement::Not),
            LangAPI::Symbol::createExpressionValue(LangAPI::Symbol {var})
        };
    }

    auto setBool(const std::string &var, bool value) -> LangAPI::Statement {
        return LangAPI::VariableAssignment::createStatement(LangAPI::VariableAssignment {
            .name = LangAPI::Symbol {var},
            .value = LangAPI::Bool::createExpression(LangAPI::Bool {.value = value})
        });
    }

    auto joinTokens(const std::vector<AST::LLkToken> &tokens) -> std::string {
        std::string out;
        for (const auto &token : tokens) {
            if (!out.empty()) out += ", ";
            out += corelib::text::join(token, ".");
        }
        return out;
    }

}

// ---------------------------------------------------------------- helpers ----

auto MemLower::push(const LangAPI::Statement &stmt) -> void {
    fun_ctx->statements->push_back(stmt);
}

auto MemLower::comment(const AST::RuleMember &mem, const std::string &meaning) -> void {
    std::ostringstream out;
    out << mem;
    push(LangAPI::Comment::createStatement(
        LangAPI::Comment {.value = "grammar `" + out.str() + "`: " + meaning}));
}

auto MemLower::createValueAssignment(const std::string &name, const LangAPI::Expression &expr) -> LangAPI::VariableAssignment {
    return LangAPI::VariableAssignment {
        .name = LangAPI::Symbol {name},
        .value = expr
    };
}

// Hoists a declaration to the top of the function. Returns false when the type
// is unknown (Undef): the caller must then declare it in place instead.
auto MemLower::declare(const std::string &name, const LangAPI::Type &type) -> bool {
    if (type == LangAPI::ValueType::Undef) return false;
    LangAPI::Variable var;
    var.name = name;
    var.type = type;
    fun_ctx->declarations.push_back(LangAPI::Variable::createStatement(var));
    return true;
}

// Declares `name` (hoisted when its type is known) and assigns `value` at the
// current emission point.
auto MemLower::bind(const std::string &name, const LangAPI::Type &type, const LangAPI::Expression &value) -> LangAPI::Variable {
    LangAPI::Variable var;
    var.name = name;
    var.type = type;
    if (declare(name, type)) {
        push(LangAPI::VariableAssignment::createStatement(createValueAssignment(name, value)));
    } else {
        var.value = value;
        push(LangAPI::Variable::createStatement(var));
    }
    return var;
}

auto MemLower::hasValue(const Parser::LowerMeta &meta) -> bool {
    return !meta.value.name.empty() || !meta.value_list.name.empty();
}

auto MemLower::isWhitespaceRef(const AST::RuleMember &mem) -> bool {
    return mem.isAutoGenerated && mem.isName() && mem.getName().name == constants::whitespace && mem.features.empty();
}

auto MemLower::tokenIdSymbol(const AST::LLkToken &token) -> LangAPI::Symbol {
    if (token.size() == 1 && token.front() == "$")
        return LangAPI::Symbol{"::ISPA_STD", "Recovery", "EndOfInput"};
    LangAPI::Symbol sym {token};
    sym.path.insert(sym.path.begin(), std::string {"Tokens"});
    sym.path.emplace_back(std::string {"id"});
    return sym;
}

// `(*pos).kind` of the current lookahead cursor
auto MemLower::kindOfCurrent() -> LangAPI::Expression {
    return LangAPI::FunctionCall::createExpression(LangAPI::FunctionCall{
        .name = std::make_shared<LangAPI::Symbol>(LangAPI::Symbol{"::ISPA_STD", "lookahead_kind"}),
        .args = {LangAPI::Pos::createExpression(LangAPI::Pos{.dereference = false})}});
}

auto MemLower::tokenKindIs(const AST::LLkToken &token) -> LangAPI::Expression {
    for (const auto& [name, rule] : tree->getTreeMap()) if (rule.external && rule.external->marker == token) {
        return LangAPI::FunctionCall::createExpression(LangAPI::FunctionCall{
            .name = std::make_shared<LangAPI::Symbol>(LangAPI::Symbol{"::ISPA_STD", "external_starts"}),
            .template_parameters = {std::make_shared<LangAPI::Type>(LangAPI::Type{LangAPI::Symbol{"::" + rule.external->cpp_binding}})},
            .args = {LangAPI::Pos::createExpression(LangAPI::Pos{.dereference = false})}});
    }
    auto expr = kindOfCurrent();
    expr.push_back(op(LangAPI::ExpressionElement::Equal));
    appendExpr(expr, LangAPI::Symbol::createExpression(tokenIdSymbol(token)));
    return expr;
}

auto MemLower::getMatchSymbol(char quantifier) -> LangAPI::StdlibExports {
    switch (quantifier) {
        case '\0': return LangAPI::StdlibExports::ParserRequired;
        case '?':  return LangAPI::StdlibExports::ParserOptional;
        case '+':  return LangAPI::StdlibExports::ParserRange;
        case '*':  return LangAPI::StdlibExports::ParserRangeOptional;
        default:
            throw Error("Undefined quantifier: {}", quantifier);
    }
}

auto MemLower::ismemberOptional(char quantifier) -> bool {
    return quantifier == '?' || quantifier == '*';
}

// Node type argument of the stdlib combinators.
auto MemLower::nodeType(const AST::RuleMemberName &name) -> LangAPI::Type {
    if (!name.templateParameter.empty())
        return LangAPI::Type{LangAPI::Symbol{name.templateParameter}};
    LangAPI::Type result{LangAPI::Symbol{"Types", name.name}};
    for (std::size_t i = 0; i < name.templateArguments.size(); ++i) {
        auto path = name.templateArguments[i];
        const bool parameter = i < name.templateArgumentIsParameter.size() && name.templateArgumentIsParameter[i];
        if (!parameter) path.insert(path.begin(), "Types");
        result.template_parameters.emplace_back(LangAPI::Type{LangAPI::Symbol{path}});
    }
    return result;
}

// ------------------------------------------------------------- entry point ----

auto MemLower::build(const AST::RuleMember &mem) -> void {
    // ParserBase keeps its own copy of Ctx, so the per-function name counters
    // are loaded from / stored back to the FunctionCtx around each member.
    ctx.variables_lowered = fun_ctx->variables_lowered;
    lower(mem);
    fun_ctx->variables_lowered = ctx.variables_lowered;
}

auto MemLower::lowerEmbeddedMatch(const UAST::py::IspaMatch& match, std::set<std::string>& locals)
    -> LangAPI::Statements {
    const std::string source = "embedded_action { " + match.source + " }";
    IspaParser parser;
    const auto& syntax = parser.parse(source.c_str());
    if (!parser.getErrors().empty())
        throw Error("Invalid embedded match: {}", parser.getErrors().front().message);
    AST::Builder builder(syntax, "<embedded match>");
    builder.build();
    auto embedded = builder.get();
    const auto& members = embedded.getTreeMap().at(stdu::vector<std::string>{"embedded_action"}).rule_members;
    LangAPI::Statements statements;
    StatementsScope scope(*fun_ctx, statements);
    fun_ctx->action_locals = locals;
    const auto exports_before = fun_ctx->exports.size();
    for (const auto& member : members) {
        const auto resolve = [&](auto&& self, AST::RuleMember& value) -> void {
            const auto resolve_name = [&](stdu::vector<std::string>& name) {
                if (tree->contains(name)) return;
                const auto& scope_name = fun_ctx->rule_name;
                for (std::size_t count = scope_name.size(); count != 0; --count) {
                    stdu::vector<std::string> candidate(scope_name.begin(), scope_name.begin() + count);
                    candidate.insert(candidate.end(), name.begin(), name.end());
                    if (tree->contains(candidate)) { name = std::move(candidate); return; }
                }
            };
            if (value.isName()) resolve_name(value.getName().name);
            if (value.isString()) {
                const auto literal = value.getString().value;
                for (const auto& [name, rule] : tree->getTreeMap()) {
                    if (name.empty() || !corelib::text::isUpper(name.back()) || rule.rule_members.size() != 1) continue;
                    const auto& token = *rule.rule_members.front();
                    if (token.isString() && token.getString().value == literal) {
                        value.value = AST::RuleMemberName{.name = name};
                        break;
                    }
                }
            }
            for (auto& feature : value.features)
                if (auto* stream = std::get_if<AST::StreamMatch>(&feature)) resolve_name(stream->rule);
            if (value.isGroup()) for (auto& child : value.getGroup().values) self(self, *child);
            if (value.isOp()) for (auto& child : value.getOp().options) self(self, *child);
        };
        resolve(resolve, *member);
        lower(*member);
    }
    fun_ctx->exports.resize(exports_before);
    locals = fun_ctx->action_locals;
    return statements;
}

void MemLower::lowerStreamMatch(const AST::StreamMatch &match) {
    if (!fun_ctx->action_locals.contains(match.stream))
        throw Error("match({}) requires a previously declared token stream", match.stream);
    if (!tree->contains(match.rule) || !AST::RuleMemberName{.name = match.rule}.isNonterminal())
        throw Error("match({}) requires a parser rule", match.stream);
    for (const auto& feature : tree->getTreeMap().at(match.rule).features)
        if (const auto* syntax = std::get_if<AST::DeclarationSyntax>(&feature);
            syntax && (!syntax->parameters.empty() || !syntax->templateParameters.empty()))
            throw Error("match({}) cannot call a parameterized parser rule", match.stream);
    if (!match.capture.empty() && (match.capture == "ctx" || fun_ctx->action_locals.contains(match.capture)))
        throw Error("Duplicate or reserved action binding {}", match.capture);

    std::set<stdu::vector<std::string>> visited;
    const auto source_visit = [&](auto&& self, const stdu::vector<std::string>& name) -> bool {
        if (!tree->contains(name) || !visited.insert(name).second) return false;
        const auto& rule = tree->getTreeMap().at(name);
        if (rule.external) return true;
        const auto member_requires_source = [&](auto&& walk, const AST::RuleMember& member) -> bool {
            if (member.isName()) return self(self, member.getName().name);
            if (member.isGroup()) for (const auto& child : member.getGroup().values)
                if (walk(walk, *child)) return true;
            if (member.isOp()) for (const auto& child : member.getOp().options)
                if (walk(walk, *child)) return true;
            return false;
        };
        for (const auto& member : rule.rule_members)
            if (member_requires_source(member_requires_source, *member)) return true;
        return false;
    };
    const bool requires_source = source_visit(source_visit, match.rule);

    LangAPI::FunctionCall rule_call;
    rule_call.name = std::make_shared<LangAPI::Symbol>(fun_name(match.rule));
    rule_call.args = {LangAPI::Symbol::createExpression(LangAPI::Symbol{"stream_pos"})};
    LangAPI::Lambda callback;
    callback.parameters = {{LangAPI::Type{LangAPI::Symbol{"Lexer", requires_source ? "lazy_iterator" : "iterator"}}, "stream_pos"}};
    callback.statements = LangAPI::Return::createStatements(LangAPI::Return{
        .value = LangAPI::FunctionCall::createExpression(std::move(rule_call))});

    LangAPI::FunctionCall parse;
    parse.name = std::make_shared<LangAPI::Symbol>(LangAPI::Symbol{"ISPA_STD",
        requires_source ? "parse_source_span" : "parse_token_stream"});
    parse.template_parameters = {std::make_shared<LangAPI::Type>(LangAPI::Type{LangAPI::Symbol{"Lexer"}})};
    parse.args = {LangAPI::Symbol::createExpression(LangAPI::Symbol{"ispa_action_" + match.stream}),
        LangAPI::Lambda::createExpression(std::move(callback))};

    AST::RuleMember named;
    named.value = AST::RuleMemberName{.name = match.rule};
    const auto variable = match.capture.empty() ? "ispa_stream_match_" + std::to_string(fun_ctx->uid()) :
        "ispa_action_" + match.capture;
    bind(variable, deduceVarTypeByRuleMember(named), LangAPI::FunctionCall::createExpression(std::move(parse)));
    if (!match.capture.empty()) {
        fun_ctx->action_locals.insert(match.capture);
    } else {
        LangAPI::StorageSymbol empty;
        empty.what = LangAPI::Symbol::createExpression(LangAPI::Symbol{variable});
        empty.path.emplace_back(LangAPI::FunctionCall{.name = std::make_shared<LangAPI::Symbol>("empty")});
        LangAPI::FunctionCall error;
        error.name = std::make_shared<LangAPI::Symbol>(LangAPI::Symbol{"std", "runtime_error"});
        error.args = {LangAPI::String::createExpression(LangAPI::String{.value = "stream did not match parser rule"})};
        push(LangAPI::If::createStatement(LangAPI::If{
            LangAPI::StorageSymbol::createExpression(std::move(empty)),
            LangAPI::Throw::createStatements(LangAPI::Throw{
                .throw_value = LangAPI::FunctionCall::createExpression(std::move(error))})}));
    }
}

auto MemLower::lower(const AST::RuleMember &mem) -> Parser::LowerMeta {
    const auto capture_begin = fun_ctx->captures.size();
    Parser::LowerMeta meta;
    for (const auto& feature : mem.features) if (const auto* action = std::get_if<AST::PythonAction>(&feature)) {
        auto embedded = [this](const UAST::py::IspaMatch& match, std::set<std::string>& locals) {
            return lowerEmbeddedMatch(match, locals);
        };
        for (const auto& statement : convertPythonAction(*action, fun_ctx->action_locals, false,
                 pythonClasses(tree->getPythonDeclarations()), &fun_ctx->action_objects, {}, embedded)) push(statement);
    }
    for (const auto& feature : mem.features) if (const auto* stream = std::get_if<AST::StreamMatch>(&feature))
        lowerStreamMatch(*stream);
    if (isWhitespaceRef(mem)) {
        const auto& members = tree->getTreeMap().at(fun_ctx->rule_name).rule_members;
        if (mem_idx + 1 < members.size() && members[mem_idx].get() == &mem) {
            const auto heads = firstTokens(*members[mem_idx + 1]);
            const bool foreign = std::ranges::any_of(heads, [&](const auto& head) {
                return std::ranges::any_of(tree->getTreeMap(), [&](const auto& entry) {
                    return entry.second.external && entry.second.external->marker == head;
                });
            });
            if (foreign) {
                push(LangAPI::FunctionCall::createStatement(LangAPI::FunctionCall{
                    .name = std::make_shared<LangAPI::Symbol>("skip_external_spaces"),
                    .template_parameters = {std::make_shared<LangAPI::Type>(LangAPI::Type{LangAPI::Symbol{"Types", "__WS"}})},
                    .args = {LangAPI::Symbol::createExpression(LangAPI::Symbol{"pos"})}}));
                return meta;
            }
        }
        // Inserted by TreePass::addSpaceToken between members. Same effect as
        // optional<__WS>, without creating a value.
        push(LangAPI::SkipSpaces::createStatement(LangAPI::SkipSpaces {.isToken = false}));
        return meta;
    }
    if (mem.isName()) {
        meta = lowerName(mem);
    } else if (mem.isGroup()) {
        meta = lowerGroup(mem);
    } else if (mem.isOp()) {
        meta = lowerOp(mem);
    } else if (mem.empty()) {
        throw Error("Empty member in rule {}", fun_ctx->rule_name);
    } else {
        throw Error("Unsupported member in LL rule {}: literals and character sets must be lexer tokens", fun_ctx->rule_name);
    }
    if (hasValue(meta)) {
        for (const auto& feature : mem.features) if (const auto* capture = std::get_if<AST::VariableCapture>(&feature)) {
            if (capture->name == "ctx" || fun_ctx->action_locals.contains(capture->name))
                throw Error("Duplicate or reserved action binding {}", capture->name);
            const auto& value = meta.value.name.empty() ? meta.value_list : meta.value;
            const auto binding = "ispa_action_" + capture->name;
            const auto expression = LangAPI::Symbol::createExpression(LangAPI::Symbol{value.name});
            if (declare(binding, value.type)) push(LangAPI::VariableAssignment::createStatement(
                LangAPI::VariableAssignment{.name = LangAPI::Symbol{binding}, .value = expression}));
            else push(LangAPI::Variable::createStatement(LangAPI::Variable{.name = binding,
                .type = value.type, .value = expression}));
            fun_ctx->action_locals.insert(capture->name);
        }
        fun_ctx->exports.push_back(LoweredExport {
            .prefix = mem.prefix, .meta = meta, .quantifier = mem.quantifier,
            .member_index = mem_idx, .depth = depth
        });
        if (!mem.prefix.empty()) {
            // Capturing a whole member replaces its descendants' captures.
            fun_ctx->captures.resize(capture_begin);
            fun_ctx->captures.push_back(fun_ctx->exports.back());
        }
    }
    return meta;
}

// -------------------------------------------------------------------- names ----

// Only '\0' and '+' can fail: for '?' and '*' "not present" is a valid result.
auto MemLower::failureHandler(const AST::RuleMember &mem) -> LangAPI::Expression {
    std::vector<std::pair<stdu::vector<std::string>, stdu::vector<std::string>>> handlers;
    bool marked = false;
    for (const auto& feature : mem.features) if (const auto* marker = std::get_if<AST::FailMarker>(&feature)) {
        marked = true;
        if (!marker->name.empty()) handlers.emplace_back(marker->name, marker->parameters);
    }
    if (handlers.empty()) for (const auto& feature : tree->getTreeMap().at(fun_ctx->rule_name).features)
        if (const auto* binding = std::get_if<AST::FailBinding>(&feature))
            for (const auto& name : binding->names) handlers.push_back({{name}, {}});
    if (marked && handlers.empty()) throw Error("Fail marker in {} has no bound fail block", fun_ctx->rule_name);
    if (!handlers.empty()) {
        auto symbol = [](const std::string& name) { return LangAPI::Symbol::createExpression(LangAPI::Symbol{name}); };
        auto call = [](const std::string& name, stdu::vector<LangAPI::Expression> args) {
            return LangAPI::FunctionCall::createExpression(LangAPI::FunctionCall{
                .name = std::make_shared<LangAPI::Symbol>(name), .args = std::move(args)});
        };
        LangAPI::Lambda callback;
        callback.parameters = {{LangAPI::Type{LangAPI::Symbol{"auto&"}}, "failed_pos"},
                               {LangAPI::Type{LangAPI::Symbol{"auto&"}}, "recovery_state"}};
        callback.statements.push_back(LangAPI::Variable{.name = "before_failure", .type = LangAPI::Type{LangAPI::ValueType::Undef}, .value = symbol("failed_pos")});
        callback.statements.push_back(LangAPI::Variable{.name = "ctx", .type = LangAPI::Type{LangAPI::ValueType::Undef},
            .value = call("fail_context", {symbol("failed_pos"), symbol("result"), symbol("start_pos"),
                LangAPI::String::createExpression(LangAPI::String{.value = corelib::text::join(fun_ctx->rule_name, ".")}),
                LangAPI::String::createExpression(LangAPI::String{.value = corelib::text::join(mem.getName().name, ".")}),
                symbol("TokensToString")})});
        for (const auto& [name, arguments] : handlers) {
            const AST::FailBlock* block = nullptr;
            // Resolve in the containing rule's lexical scopes, then globally.
            for (std::size_t size = fun_ctx->rule_name.size();; --size) {
                stdu::vector<std::string> candidate(fun_ctx->rule_name.begin(), fun_ctx->rule_name.begin() + size);
                candidate.insert(candidate.end(), name.begin(), name.end());
                for (const auto& fail : tree->getFailBlocks()) if (fail.name == candidate) { block = &fail; break; }
                if (block || size == 0) break;
            }
            if (!block) throw Error("Unknown fail block {} in {}", name, fun_ctx->rule_name);
            if (block->parameters.size() != arguments.size()) throw Error("Fail block {} expects {} arguments, got {}", name, block->parameters.size(), arguments.size());
            std::set<std::string> locals = fun_ctx->action_locals;
            LangAPI::Statements body;
            for (std::size_t i = 0; i < arguments.size(); ++i) {
                if (block->parameters[i] == "ctx") throw Error("ctx is reserved in fail block {}", name);
                const auto argument = fun_ctx->action_locals.contains(arguments[i]) ? "ispa_action_" + arguments[i] : arguments[i];
                const auto temporary = "ispa_failure_argument_" + std::to_string(fun_ctx->uid());
                callback.statements.push_back(LangAPI::Variable{.name = temporary, .type = LangAPI::Type{LangAPI::ValueType::Undef}, .value = symbol(argument)});
                body.push_back(LangAPI::Variable{.name = "ispa_action_" + block->parameters[i], .type = LangAPI::Type{LangAPI::ValueType::Undef}, .value = symbol(temporary)});
                locals.insert(block->parameters[i]);
            }
            auto objects = fun_ctx->action_objects;
            const auto code = convertPythonAction(block->action, locals, true, pythonClasses(tree->getPythonDeclarations()), &objects);
            body.insert(body.end(), code.begin(), code.end());
            callback.statements.push_back(LangAPI::If{LangAPI::Bool::createExpression(LangAPI::Bool{.value = true}), std::move(body)});
        }
        callback.statements.push_back(LangAPI::Return::createStatement(LangAPI::Return{
            .value = call("custom_failure_decision", {symbol("before_failure"), symbol("failed_pos"), symbol("ctx")})}));
        return call("custom_failure", {LangAPI::Lambda::createExpression(std::move(callback))});
    }
    return recoveryHandler(*tree, fun_ctx->rule_name, mem_idx, {
        .allow_insertion = true,
        .allow_deletion = true,
        .allow_replacement = true,
        .max_skip = 32
    });
}

// Argument order: pos, [rec], [fail]
auto MemLower::ruleCallable(const AST::RuleMember &mem) -> LangAPI::Expression {
    const auto &name = mem.getName();
    if (!name.templateParameter.empty()) {
        LangAPI::Type tag{LangAPI::ValueType::TypeTag};
        tag.template_parameters.emplace_back(LangAPI::Type{LangAPI::Symbol{name.templateParameter}});
        LangAPI::FunctionCall call;
        call.name = std::make_shared<LangAPI::Symbol>("ispa_match_template");
        call.args = {
            LangAPI::Inheritance::createExpression(LangAPI::Inheritance{.name = std::make_shared<LangAPI::Type>(tag)}),
            LangAPI::Symbol::createExpression(LangAPI::Symbol{"rule_pos"})};
        LangAPI::Lambda lambda;
        lambda.parameters = {{LangAPI::Type{LangAPI::Symbol{"IT"}}, "rule_pos"}};
        lambda.statements = LangAPI::Return::createStatements(LangAPI::Return{
            .value = LangAPI::FunctionCall::createExpression(std::move(call))});
        return LangAPI::Lambda::createExpression(std::move(lambda));
    }
    const AST::DeclarationSyntax *call_syntax = nullptr;
    for (const auto &feature : mem.features)
        if (const auto *syntax = std::get_if<AST::DeclarationSyntax>(&feature)) call_syntax = syntax;
    const auto &rule = tree->getTreeMap().at(name.name);
    const AST::DeclarationSyntax *declaration = nullptr;
    for (const auto &feature : rule.features)
        if (const auto *syntax = std::get_if<AST::DeclarationSyntax>(&feature)) declaration = syntax;
    const auto argument_count = call_syntax ? call_syntax->parameters.size() : 0;
    const auto parameter_count = declaration ? declaration->parameters.size() : 0;
    const auto template_argument_count = name.templateArguments.size();
    const auto template_parameter_count = declaration ? declaration->templateParameters.size() : 0;
    if (argument_count != parameter_count)
        throw Error("Rule {} requires {} arguments, got {}", name.name, parameter_count, argument_count);
    if (template_argument_count != template_parameter_count)
        throw Error("Rule {} requires {} template arguments, got {}", name.name,
            template_parameter_count, template_argument_count);
    if (argument_count == 0 && template_argument_count == 0)
        return LangAPI::Symbol::createExpression(LangAPI::Symbol{fun_name(name.name)});
    LangAPI::FunctionCall call;
    call.name = std::make_shared<LangAPI::Symbol>(fun_name(name.name));
    for (std::size_t i = 0; i < template_argument_count; ++i) {
        auto path = name.templateArguments[i];
        const bool parameter = i < name.templateArgumentIsParameter.size() && name.templateArgumentIsParameter[i];
        if (!parameter) path.insert(path.begin(), "Types");
        call.template_parameters.push_back(std::make_shared<LangAPI::Type>(LangAPI::Type{LangAPI::Symbol{path}}));
    }
    call.args.push_back(LangAPI::Symbol::createExpression(LangAPI::Symbol{"rule_pos"}));
    for (const auto &argument : call_syntax->parameters) {
        const auto variable = fun_ctx->action_locals.contains(argument) ? "ispa_action_" + argument : argument;
        call.args.push_back(LangAPI::Symbol::createExpression(LangAPI::Symbol{variable}));
    }
    LangAPI::Lambda lambda;
    lambda.parameters = {{LangAPI::Type{LangAPI::Symbol{"IT"}}, "rule_pos"}};
    lambda.statements = LangAPI::Return::createStatements(LangAPI::Return{
        .value = LangAPI::FunctionCall::createExpression(std::move(call))});
    return LangAPI::Lambda::createExpression(std::move(lambda));
}

auto MemLower::combinatorArgs(const AST::RuleMember &mem) -> stdu::vector<LangAPI::Expression> {
    const auto &name = mem.getName();
    stdu::vector<LangAPI::Expression> args;
    args.push_back(LangAPI::Pos::createExpression(LangAPI::Pos {.dereference = false, .offset = 0}));
    if (name.isNonterminal())    // rule calls pass the rule function as `rec`
        args.push_back(ruleCallable(mem));
    if (!ismemberOptional(mem.quantifier))
        args.push_back(failureHandler(mem));
    return args;
}

auto MemLower::lowerName(const AST::RuleMember &mem) -> Parser::LowerMeta {
    const auto &name = mem.getName();
    const bool is_list = mem.quantifier == '*' || mem.quantifier == '+';

    Parser::LowerMeta meta;
    const auto vname = var_name(name.name, is_list);

    const AST::Counter* counter = nullptr;
    const AST::SharedCounterRegistration* registration = nullptr;
    for (const auto& feature : mem.features) {
        if (const auto* value = std::get_if<AST::Counter>(&feature)) counter = value;
        if (const auto* value = std::get_if<AST::SharedCounterRegistration>(&feature)) registration = value;
    }
    if (counter) {
        bool parameter_bound = false;
        for (const auto &feature : tree->getTreeMap().at(fun_ctx->rule_name).features)
            if (const auto *syntax = std::get_if<AST::DeclarationSyntax>(&feature))
                parameter_bound = std::ranges::find(syntax->parameters, counter->bound) != syntax->parameters.end();
        LangAPI::FunctionCall spec;
        spec.name = std::make_shared<LangAPI::Symbol>(LangAPI::Symbol{
            "ISPA_STD", "CountSpec", counter->kind == "number" || parameter_bound ? "fixed" : "named"});
        if (counter->kind == "number")
            spec.args.push_back(LangAPI::Int::createExpression(LangAPI::Int{.value = std::stoll(counter->bound)}));
        else if (parameter_bound)
            spec.args.push_back(LangAPI::Symbol::createExpression(LangAPI::Symbol{"ispa_action_" + counter->bound}));
        else {
            spec.args.push_back(LangAPI::String::createExpression(LangAPI::String{.value = counter->bound}));
        }
        spec.args.push_back(LangAPI::Int::createExpression(LangAPI::Int{.value = mem.quantifier == '+' ? 1 : 0}));
        LangAPI::IspaLibFunctionCall call;
        call.symbol = LangAPI::IspaLibSymbol{.exports = LangAPI::StdlibExports::ParserCounted};
        call.template_parameters.push_back(std::make_shared<LangAPI::Type>(nodeType(name)));
        if (tree->contains(constants::whitespace) && !explicitWhitespaceBoundary(*tree, mem))
            call.template_parameters.push_back(std::make_shared<LangAPI::Type>(
                LangAPI::Type{LangAPI::Symbol{"Types", constants::whitespace}}));
        call.args = {LangAPI::Pos::createExpression(LangAPI::Pos{.dereference = false}), LangAPI::FunctionCall::createExpression(spec)};
        if (name.isNonterminal())
            call.args.push_back(ruleCallable(mem));
        auto var = bind(vname, deduceVarTypeByRuleMember(mem), LangAPI::IspaLibFunctionCall::createExpression(call));
        meta.value_list = var;
        if (registration) push(LangAPI::FunctionCall::createStatement(LangAPI::FunctionCall{
            .name = std::make_shared<LangAPI::Symbol>("bind_counter"),
            .args = {LangAPI::String::createExpression(LangAPI::String{.value = registration->name}),
                     LangAPI::Symbol::createExpression(LangAPI::Symbol{var.name})}}));
        return meta;
    }

    LangAPI::IspaLibFunctionCall call = {
        .symbol = LangAPI::IspaLibSymbol {.exports = getMatchSymbol(mem.quantifier)},
        .args = combinatorArgs(mem),
        .template_parameters = {std::make_shared<LangAPI::Type>(nodeType(name))}
    };
    if (is_list && tree->getTreeMap().contains(constants::whitespace) && !explicitWhitespaceBoundary(*tree, mem))
        call.template_parameters.push_back(std::make_shared<LangAPI::Type>(
            LangAPI::Type{LangAPI::Symbol{"Types", constants::whitespace}}));
    auto var = bind(vname, deduceVarTypeByRuleMember(mem), LangAPI::IspaLibFunctionCall::createExpression(call));
    (is_list ? meta.value_list : meta.value) = var;
    if (registration) push(LangAPI::FunctionCall::createStatement(LangAPI::FunctionCall{
        .name = std::make_shared<LangAPI::Symbol>("bind_counter"),
        .args = {LangAPI::String::createExpression(LangAPI::String{.value = registration->name}),
                 LangAPI::Symbol::createExpression(LangAPI::Symbol{var.name})}}));
    // var_name already advances the per-name counter.
    return meta;
}

// ------------------------------------------------------------------- groups ----

auto MemLower::lowerGroup(const AST::RuleMember &mem) -> Parser::LowerMeta {
    return mem.quantifier == '\0' ? lowerSequence(mem) : lowerQuantified(mem);
}

auto MemLower::lowerOp(const AST::RuleMember &mem) -> Parser::LowerMeta {
    return mem.quantifier == '\0' ? lowerDispatch(mem) : lowerQuantified(mem);
}

auto MemLower::lowerBody(const AST::RuleMember &mem) -> Parser::LowerMeta {
    return mem.isGroup() ? lowerSequence(mem) : lowerDispatch(mem);
}

auto MemLower::lowerSequence(const AST::RuleMember &mem) -> Parser::LowerMeta {
    std::vector<Parser::LowerMeta> parts;
    ++depth;
    for (const auto &child : mem.getGroup().values) {
        auto meta = lower(*child);
        if (hasValue(meta)) parts.push_back(std::move(meta));
    }
    --depth;
    return composeValue(mem, parts);
}

// A sequence's value: nothing, the single child's value, or a tuple.
auto MemLower::composeValue(const AST::RuleMember &mem, const std::vector<Parser::LowerMeta> &parts) -> Parser::LowerMeta {
    if (parts.empty()) return {};
    if (parts.size() == 1) return parts.front();

    LangAPI::MakeTuple tuple;
    for (const auto &part : parts) {
        const auto &var = part.value.name.empty() ? part.value_list : part.value;
        tuple.args.push_back(LangAPI::Symbol::createExpression(LangAPI::Symbol {var.name}));
    }
    LangAPI::Type type{LangAPI::ValueType::Tuple};
    for (const auto& part : parts)
        type.template_parameters.emplace_back(
            (part.value.name.empty() ? part.value_list : part.value).type);
    Parser::LowerMeta meta;
    meta.value = bind("group" + std::to_string(fun_ctx->uid()), type,
                      LangAPI::MakeTuple::createExpression(std::move(tuple)));
    return meta;
}

// ---------------------------------------------- quantified group / choice ----

// FIRST_1 of the member's body (quantifier ignored). A body that can match the
// empty input cannot guard a loop: that is a grammar error, not an LL decision.
auto MemLower::firstTokens(const AST::RuleMember &mem) -> std::vector<AST::LLkToken> {
    AST::RuleMember base = mem;
    base.quantifier = '\0';

    std::set<AST::LLkToken> tokens;
    for (const auto &seq : tree->getAlternativeLookahead(base, fun_ctx->rule_name, 1)) {
        if (seq.empty())
            throw Error("Rule {}: quantified member can match empty input, it cannot be guarded by lookahead", fun_ctx->rule_name);
        tokens.insert(seq.front());
    }
    if (tokens.empty())
        throw Error("Rule {}: empty FIRST set for quantified member", fun_ctx->rule_name);
    return {tokens.begin(), tokens.end()};
}

auto MemLower::firstGuard(const AST::RuleMember &mem) -> LangAPI::Expression {
    LangAPI::Expression guard;
    guard.push_back(op(LangAPI::ExpressionElement::GroupOpen));
    bool first = true;
    for (const auto &token : firstTokens(mem)) {
        if (!first) guard.push_back(op(LangAPI::ExpressionElement::Or));
        appendExpr(guard, tokenKindIs(token));
        first = false;
    }
    guard.push_back(op(LangAPI::ExpressionElement::GroupClose));
    return guard;
}

auto MemLower::lowerQuantified(const AST::RuleMember &mem) -> Parser::LowerMeta {
    const char q = mem.quantifier;
    const bool repeated = q == '*' || q == '+';

    auto guard = firstGuard(mem);

    LangAPI::Statements body;
    const auto capture_begin = fun_ctx->captures.size();
    Parser::LowerMeta item;
    {
        StatementsScope scope(*fun_ctx, body);
        ++depth;
        item = lowerBody(mem);
        --depth;
    }

    Parser::LowerMeta meta;
    if (repeated && hasValue(item)) {
        // Collect one value per iteration.
        const auto &element = item.value.name.empty() ? item.value_list : item.value;
        meta.value_list.name = "group_list" + std::to_string(fun_ctx->uid());
        meta.value_list.type = LangAPI::Type {LangAPI::ValueType::Array, element.type};
        declare(meta.value_list.name, meta.value_list.type);

        body.push_back(LangAPI::Statement {LangAPI::StorageSymbol::createExpression(LangAPI::StorageSymbol {
            LangAPI::Symbol::createExpression(LangAPI::Symbol {meta.value_list.name}),
            stdu::vector<LangAPI::StorageSymbol::PathPart> {LangAPI::ArrayMethodCall {
                .method = LangAPI::ArrayMethods::Push,
                .args = {LangAPI::Symbol::createExpression(LangAPI::Symbol {element.name})}
            }}
        })});
    } else if (q == '?') {
        meta = item;
    }

    if (repeated && mem.prefix.empty()) {
        // Nested captures belong to the repetition, so publish one collected
        // value per iteration rather than the final iteration's temporary.
        for (std::size_t i = capture_begin; i < fun_ctx->captures.size(); ++i) {
            auto& capture = fun_ctx->captures[i];
            const auto element = capture.meta.value.name.empty()
                ? capture.meta.value_list : capture.meta.value;
            LangAPI::Variable collection;
            collection.name = "capture_list" + std::to_string(fun_ctx->uid());
            collection.type = LangAPI::Type{LangAPI::ValueType::Array, element.type};
            declare(collection.name, collection.type);
            body.push_back(LangAPI::StorageSymbol::createStatement(LangAPI::StorageSymbol{
                LangAPI::Symbol::createExpression(LangAPI::Symbol{collection.name}),
                stdu::vector<LangAPI::StorageSymbol::PathPart>{LangAPI::ArrayMethodCall{
                    .method = LangAPI::ArrayMethods::Push,
                    .args = {LangAPI::Symbol::createExpression(LangAPI::Symbol{element.name})}
                }}
            }));
            capture.meta = Parser::LowerMeta{.value_list = std::move(collection)};
            capture.quantifier = mem.quantifier;
        }
    }

    // addSpaceToken only inserts ONE `__WS?` before the whole quantified member;
    // later iterations need their own whitespace skip before the guard is tested.
    if (repeated && !explicitWhitespaceBoundary(*tree, mem))
        body.push_back(LangAPI::SkipSpaces::createStatement(LangAPI::SkipSpaces {.isToken = false}));

    switch (q) {
        case '?':
            comment(mem, "optional; entered only when lookahead is in FIRST");
            push(LangAPI::If::createStatement(LangAPI::If {guard, body}));
            break;
        case '*':
            comment(mem, "repeat while lookahead is in FIRST");
            push(LangAPI::While::createStatement(LangAPI::While {guard, body}));
            break;
        case '+':
            comment(mem, "one required iteration, then repeat while lookahead is in FIRST");
            push(LangAPI::DoWhile::createStatement(LangAPI::DoWhile {guard, body}));
            break;
        default:
            throw Error("Undefined quantifier: {}", q);
    }
    return meta;
}

// ------------------------------------------------------------------ choice ----

static auto overlaps(const AST::LLkSeq &a, const AST::LLkSeq &b) -> bool {
    return std::equal(a.begin(), a.begin() + std::min(a.size(), b.size()), b.begin());
}

auto MemLower::resolveLookahead(const stdu::vector<std::shared_ptr<AST::RuleMember>> &alts) -> Resolution {
    std::string last_problem;
    for (std::size_t k = 1; k <= max_lookahead; ++k) {
        Resolution res;
        res.k = k;
        bool valid = !alts.empty();
        last_problem.clear();

        for (std::size_t i = 0; i < alts.size(); ++i) {
            auto la = tree->getAlternativeLookahead(*alts[i], fun_ctx->rule_name, k);
            std::set<AST::LLkSeq> seqs(la.begin(), la.end());
            if (seqs.empty()) {
                valid = false;
                last_problem = std::format("alternative {} has an empty lookahead set", i);
            }
            for (const auto &seq : seqs) {
                if (seq.empty()) {
                    valid = false;   // FOLLOW not resolved at this k
                    last_problem = std::format("alternative {} can match empty input", i);
                }
            }
            res.perAlt.push_back(std::move(seqs));
        }

        bool conflict = false;
        for (std::size_t i = 0; i < res.perAlt.size() && !conflict; ++i)
            for (std::size_t j = i + 1; j < res.perAlt.size() && !conflict; ++j)
                for (const auto &a : res.perAlt[i]) {
                    for (const auto &b : res.perAlt[j])
                        if (overlaps(a, b)) {
                            conflict = true;
                            last_problem = std::format("alternatives {} and {} share the lookahead prefix [{}]", i, j,
                                                       joinTokens(a.size() <= b.size() ? a : b));
                            break;
                        }
                    if (conflict) break;
                }

        if (valid && !conflict) return res;
    }
    throw Error("LL(k) conflict in rule {}: not resolved within k = {} ({}). Check common prefixes / left-factor the grammar.",
                fun_ctx->rule_name, max_lookahead, last_problem);
}

// Lookahead cursors: level L pushes a child cursor `la<op>_<L>`. Level > 1
// first advances it past one significant token. The chosen alternative is
// parsed with the cursor reset to the original position, so lookahead never
// consumes input by itself.
auto MemLower::decisionTree(
    const Resolution &res,
    const std::vector<LangAPI::Statements> &bodies,
    const std::vector<std::size_t> &candidates,
    std::size_t level,
    std::size_t op_id,
    const std::vector<AST::LLkToken> &path,
    const LangAPI::Variable &matched
) -> LangAPI::Statements {
    const auto cursor = [&](std::size_t l) { return "la" + std::to_string(op_id) + "_" + std::to_string(l); };
    const std::size_t index = level - 1;   // which lookahead token this level tests

    std::map<AST::LLkToken, std::set<std::size_t>> buckets;
    for (const auto alt : candidates) {
        for (const auto &seq : res.perAlt[alt]) {
            if (index >= seq.size()) continue;
            bool on_path = true;
            for (std::size_t i = 0; i < index; ++i)
                if (seq[i] != path[i]) { on_path = false; break; }
            if (on_path) buckets[seq[index]].insert(alt);
        }
    }

    LangAPI::Switch sw {.expression = kindOfCurrent()};
    const bool external = std::ranges::any_of(buckets, [&](const auto& entry) {
        return std::ranges::any_of(tree->getTreeMap(), [&](const auto& rule) {
            return rule.second.external && rule.second.external->marker == entry.first;
        });
    });
    LangAPI::Statements select;
    const auto selected = "external_choice_" + std::to_string(op_id) + "_" + std::to_string(level);
    if (external) {
        select.push_back(LangAPI::Variable::createStatement(LangAPI::Variable{
            .name = selected, .type = LangAPI::Type{LangAPI::ValueType::LargeUInt},
            .value = LangAPI::Int::createExpression(LangAPI::Int{.value = 0})}));
        sw.expression = LangAPI::Symbol::createExpression(LangAPI::Symbol{selected});
    }
    long long bucket = 0;
    for (const auto &[token, alts] : buckets) {
        sw.cases.emplace_back();
        auto &cs = sw.cases.back();
        cs.first = LangAPI::Symbol::createRValue(tokenIdSymbol(token));
        if (external) {
            cs.first = LangAPI::Int::createRValue(LangAPI::Int{.value = ++bucket});
            select.push_back(LangAPI::If::createStatement(LangAPI::If{tokenKindIs(token), {
                LangAPI::If::createStatement(LangAPI::If{
                    LangAPI::Symbol::createExpression(LangAPI::Symbol{selected}),
                    LangAPI::Throw::createStatements(LangAPI::Throw{.throw_value = LangAPI::FunctionCall::createExpression(LangAPI::FunctionCall{
                        .name = std::make_shared<LangAPI::Symbol>(LangAPI::Symbol{"std", "runtime_error"}),
                        .args = {LangAPI::String::createExpression(LangAPI::String{.value = "Ambiguous external parser alternatives"})}})})}),
                LangAPI::VariableAssignment::createStatement(LangAPI::VariableAssignment{
                    .name = LangAPI::Symbol{selected}, .value = LangAPI::Int::createExpression(LangAPI::Int{.value = bucket})})}}));
        }

        if (alts.size() == 1) {
            if (level > 1)
                cs.second.push_back(LangAPI::VariableAssignment::createStatement(LangAPI::VariableAssignment {
                    .name = LangAPI::Symbol {cursor(level)},
                    .value = LangAPI::Symbol::createExpression(LangAPI::Symbol {cursor(1)})
                }));
            const auto &body = bodies[*alts.begin()];
            cs.second.insert(cs.second.end(), body.begin(), body.end());
            cs.second.push_back(setBool(matched.name, true));
        } else {
            if (level >= res.k)
                throw Error("Unresolved LL(k) decision at depth {} in rule {}", index, fun_ctx->rule_name);
            auto next_path = path;
            next_path.push_back(token);
            auto nested = decisionTree(res, bodies, {alts.begin(), alts.end()}, level + 1, op_id, next_path, matched);
            cs.second.insert(cs.second.end(), nested.begin(), nested.end());
        }
    }

    LangAPI::Statements out;
    out.push_back(LangAPI::PushPosCounter::createStatement(LangAPI::PushPosCounter {.name = cursor(level)}));
    if (level > 1) {
        const AST::ExternalRule* preceding = nullptr;
        for (const auto& [name, rule] : tree->getTreeMap())
            if (rule.external && rule.external->marker == path.back()) preceding = &*rule.external;
        if (preceding) {
            auto call = LangAPI::FunctionCall::createExpression(LangAPI::FunctionCall{
                .name = std::make_shared<LangAPI::Symbol>(LangAPI::Symbol{"::ISPA_STD", "external_match"}),
                .template_parameters = {std::make_shared<LangAPI::Type>(LangAPI::Type{LangAPI::Symbol{"::" + preceding->cpp_binding}})},
                .args = {LangAPI::Pos::createExpression(LangAPI::Pos{.dereference = false})}});
            LangAPI::StorageSymbol next;
            next.what = std::move(call);
            next.path.emplace_back(std::string{"it"});
            out.push_back(LangAPI::VariableAssignment::createStatement(LangAPI::VariableAssignment{
                .name = LangAPI::Symbol{cursor(level)}, .value = LangAPI::StorageSymbol::createExpression(next)}));
        } else {
            out.push_back(LangAPI::CounterIncreament::createStatement(LangAPI::CounterIncreament {}));
            out.push_back(LangAPI::SkipSpaces::createStatement(LangAPI::SkipSpaces {.isToken = false}));
        }
    }
    out.insert(out.end(), select.begin(), select.end());
    out.push_back(LangAPI::Switch::createStatement(std::move(sw)));
    if (level > 1) {
        // A nested level that selected nothing must not leave the cursor advanced.
        out.push_back(LangAPI::If::createStatement(LangAPI::If {
            notExpr(matched.name),
            LangAPI::VariableAssignment::createStatements(LangAPI::VariableAssignment {
                .name = LangAPI::Symbol {cursor(level)},
                .value = LangAPI::Symbol::createExpression(LangAPI::Symbol {cursor(level - 1)})
            })
        }));
    }
    out.push_back(LangAPI::PopPosCounter::createStatement(LangAPI::PopPosCounter {}));
    return out;
}

auto MemLower::lowerDispatch(const AST::RuleMember &mem) -> Parser::LowerMeta {
    const auto &alts = mem.getOp().options;
    if (alts.empty()) throw Error("Empty choice in rule {}", fun_ctx->rule_name);

    AST::RuleMember base = mem;
    base.quantifier = '\0';
    const auto op_id = fun_ctx->uid();

    // Result variable (variant of the alternatives' values) and success flag.
    Parser::LowerMeta meta;
    const auto result_type = deduceVarTypeByRuleMember(base);
    const std::string result_name = "choice" + std::to_string(op_id);
    const bool has_result = declare(result_name, result_type);
    if (has_result) {
        meta.value.name = result_name;
        meta.value.type = result_type;
    }
    LangAPI::Variable matched;
    matched.name = "matched" + std::to_string(op_id);
    matched.type = LangAPI::Type {LangAPI::ValueType::Bool};
    declare(matched.name, matched.type);

    // Each alternative is lowered exactly once; decision-tree leaves copy the
    // statements, so alternatives with several FIRST tokens share variables.
    std::vector<LangAPI::Statements> bodies(alts.size());
    for (std::size_t i = 0; i < alts.size(); ++i) {
        StatementsScope scope(*fun_ctx, bodies[i]);
        ++depth;
        auto alt = lower(*alts[i]);
        --depth;
        if (has_result && hasValue(alt)) {
            const auto &src = alt.value.name.empty() ? alt.value_list : alt.value;
            push(LangAPI::VariableAssignment::createStatement(
                createValueAssignment(result_name, LangAPI::Symbol::createExpression(LangAPI::Symbol {src.name}))));
        }
    }

    const auto res = resolveLookahead(alts);

    comment(mem, "choice; alternative selected by LL(" + std::to_string(res.k) + ") lookahead");
    push(setBool(matched.name, false));
    std::vector<std::size_t> candidates(alts.size());
    std::iota(candidates.begin(), candidates.end(), 0);
    for (auto &stmt : decisionTree(res, bodies, candidates, 1, op_id, {}, matched))
        push(stmt);

    LangAPI::Switch recovery_switch{
        .expression = recoveryChoice(*tree, fun_ctx->rule_name, mem)};
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        auto body = bodies[i];
        body.push_back(setBool(matched.name, true));
        recovery_switch.cases.emplace_back(
            LangAPI::Int::createRValue(LangAPI::Int{.value = static_cast<long long>(i)}),
            std::move(body));
    }
    push(LangAPI::If::createStatement(LangAPI::If{
        notExpr(matched.name),
        LangAPI::Statements{LangAPI::Switch::createStatement(std::move(recovery_switch))}
    }));
    push(LangAPI::If::createStatement(LangAPI::If {
        notExpr(matched.name),
        LangAPI::Statements {
            LangAPI::Return::createStatements(LangAPI::Return {}).front()
        }
    }));
    return meta;
}
