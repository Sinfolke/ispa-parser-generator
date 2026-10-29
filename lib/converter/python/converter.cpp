module Python.Init;

import std;

namespace Python {
namespace {
    std::string trim(const std::string& text) {
        const auto b = text.find_first_not_of(" \t\r\n");
        if (b == std::string::npos) return {};
        const auto e = text.find_last_not_of(" \t\r\n");
        return text.substr(b, e - b + 1);
    }
    std::vector<std::string> split_path(const std::string& value) {
        std::vector<std::string> out;
        std::size_t begin = 0;
        while (begin <= value.size()) {
            const auto p = value.find('.', begin);
            out.push_back(value.substr(begin, p == std::string::npos ? p : p - begin));
            if (p == std::string::npos) break;
            begin = p + 1;
        }
        return out;
    }
    bool is_break(const LangAPI::Statement& s) {
        if (!s.isExpression()) return false;
        const auto& e = s.getExpression();
        return e.size() == 1 && e.front().type() == LangAPI::ExpressionValueType::Break;
    }
    std::string indent(const std::string& source, unsigned levels) {
        std::string out;
        std::istringstream in(source);
        std::string row;
        while (std::getline(in, row)) out += std::string(levels * 4, ' ') + row + '\n';
        return out;
    }
    void replace_all(std::string& source, std::string_view from, std::string_view to) {
        std::size_t offset = 0;
        while ((offset = source.find(from, offset)) != std::string::npos) {
            source.replace(offset, from.size(), to);
            offset += to.size();
        }
    }
    std::string last_segment(const std::string& name) {
        const auto p = name.rfind("::");
        return p == std::string::npos ? name : name.substr(p + 2);
    }
    std::string symbol_last_name(const LangAPI::Symbol& symbol) {
        std::string out;
        for (const auto& piece : symbol.path)
            if (const auto* text = std::get_if<std::string>(&piece)) out = last_segment(*text);
        return out;
    }
    // A Python class statement needs its bases to exist already, while C++ may define a derived
    // class before its base once the base is forward-declared. Stable order: every class keeps
    // its position unless a base declared later in the same scope has to be pulled in front of it.
    std::vector<const LangAPI::Declaration*> order_by_inheritance(const LangAPI::Declarations& decls) {
        std::unordered_map<std::string, std::size_t> class_index;
        for (std::size_t i = 0; i < decls.size(); ++i)
            if (decls[i].isClass()) class_index.emplace(symbol_last_name(decls[i].getClass().name), i);
        std::vector<const LangAPI::Declaration*> result;
        result.reserve(decls.size());
        std::vector<char> state(decls.size(), 0);  // 0 new, 1 visiting, 2 emitted
        std::function<void(std::size_t)> visit = [&](std::size_t i) {
            if (state[i]) return;  // also breaks inheritance cycles
            state[i] = 1;
            if (decls[i].isClass()) {
                for (const auto& inherit : decls[i].getClass().inherit_members) {
                    const auto* base = std::get_if<LangAPI::Symbol>(&inherit.second);
                    if (!base) continue;
                    const auto found = class_index.find(symbol_last_name(*base));
                    if (found != class_index.end() && found->second != i) visit(found->second);
                }
            }
            state[i] = 2;
            result.push_back(&decls[i]);
        };
        for (std::size_t i = 0; i < decls.size(); ++i) visit(i);
        return result;
    }
    // `auto frame = recovery_frame("rule");` owns the rule's recovery frame in C++ (RAII). Python has
    // no destructors, so the body that follows it is wrapped in try/finally and the frame is closed.
    // Returns the statement index, or npos.
    std::size_t recovery_frame_statement(const LangAPI::Statements& s) {
        std::size_t index = 0;
        for (const auto& st : s) {
            if (st.isVariable()) {
                const auto& value = st.getVariable().value;
                if (value.size() == 1 && value.front().type() == LangAPI::ExpressionValueType::FunctionCall) {
                    const auto* name = std::get_if<std::shared_ptr<LangAPI::Symbol>>(&value.front().getFunctionCall().name);
                    if (name && *name && (symbol_last_name(**name) == "recovery_frame" ||
                        symbol_last_name(**name) == "counter_scope")) return index;
                }
            }
            ++index;
        }
        return std::string::npos;
    }
    std::string bootstrap_newline_action(const std::string& source) {
        const auto action = source.find("ispa_token_result = Types.modules.tokens.NEWLINE()");
        if (action == std::string::npos) return {};
        const auto selected = source.rfind("== ", action);
        if (selected == std::string::npos) return {};
        const auto begin = selected + 3;
        const auto end = source.find_first_not_of("0123456789", begin);
        if (end == begin) return {};
        return source.substr(begin, end - begin);
    }
}
Construct::Construct(const LangAPI::Declarations* src, const char* name, bool incremental,
                     bool debug_instrumentation)
    : ConstructBase(src, name, incremental), core_(name), debug_instrumentation_(debug_instrumentation) {
    core_.set_statement_writer([this](const LangAPI::Statements& s, const std::vector<std::string>& params) {
        return statements_inline(s, params);
    });
}
void Construct::line(const std::string& text) {
    output_ += std::string(indentation_ * 4, ' ') + text + '\n';
}
void Construct::emit_pending_lambdas() {
    // Hoisted functions stay in the lexical suite which owns their captures.
    // Drain recursively because a lambda body may itself contain a lambda.
    for (const auto& lambda : core_.take_pending_lambdas()) {
        line("def " + lambda.name + "(" + lambda.params + "):");
        ++indentation_;
        bool wrote = false;
        std::istringstream lines(lambda.body);
        std::string row;
        while (std::getline(lines, row)) {
            // lambda.body already contains indentation relative to the body.
            line(row);
            wrote = true;
        }
        if (!wrote) line("pass");
        --indentation_;
    }
}
void Construct::comment(const std::string& value) {
    std::istringstream in(value);
    std::string part;
    while (std::getline(in, part)) line("# " + part);
}
std::string Construct::statements_inline(const LangAPI::Statements& s, const std::vector<std::string>& params) {
    auto saved_output = std::move(output_);
    const auto saved_indentation = indentation_;
    const bool saved_inside_function = inside_function_;
    // Lambdas queued by the enclosing expression must not be flushed into this body.
    auto saved_lambdas = core_.take_pending_lambdas();
    output_.clear();
    indentation_ = 0;
    inside_function_ = true;  // a lambda body is a function body even inside a class-field initializer
    core_.enter_scope(params);
    statements(s);
    const auto captured = core_.leave_scope();
    auto result = std::move(output_);
    if (!captured.empty()) {
        // [&]-captured locals that the body assigns to: bind them to the enclosing def.
        std::string names;
        for (const auto& name : captured) names += (names.empty() ? "" : ", ") + name;
        result = "nonlocal " + names + "\n" + result;
    }
    output_ = std::move(saved_output);
    indentation_ = saved_indentation;
    inside_function_ = saved_inside_function;
    core_.restore_pending_lambdas(std::move(saved_lambdas));
    return result;
}
void Construct::statements(const LangAPI::Statements& s) {
    const auto guard = recovery_frame_statement(s);
    std::size_t index = 0;
    for (const auto& statement_ : s) {
        statement(statement_);
        if (index++ == guard) {
            line("try:");
            ++indentation_;
            const LangAPI::Statements tail{std::next(s.begin(), index), s.end()};
            body_or_pass(tail);
            --indentation_;
            line("finally:");
            ++indentation_;
            line(Core::identifier(statement_.getVariable().name) + ".close()");
            --indentation_;
            return;
        }
    }
}
void Construct::body_or_pass(const LangAPI::Statements& s) {
    if (s.empty()) line("pass");
    else statements(s);
}
void Construct::if_stmt(const LangAPI::If& value) { if_chain(value, false); }
void Construct::if_chain(const LangAPI::If& value, bool elif) {
    // `else: if` towers hit Python's 100-level indentation limit on long chains: flatten to elif.
    auto expression = core_.expression(value.expr);
    bool nested = false;
    if (elif && core_.has_pending_lambdas()) {
        // A hoisted def cannot sit between `if` and `elif`; fall back to a nested if here.
        line("else:");
        ++indentation_;
        nested = true;
        elif = false;
    }
    if (!elif) emit_pending_lambdas();
    line(std::string(elif ? "elif " : "if ") + expression + ":");
    ++indentation_;
    body_or_pass(value.stmt);
    --indentation_;
    const auto& other = value.else_stmt;
    if (!other.empty()) {
        auto it = other.begin();
        if (std::next(it) == other.end() && it->isIf()) {
            if_chain(it->getIf(), true);
        } else {
            line("else:");
            ++indentation_;
            statements(other);
            --indentation_;
        }
    }
    if (nested) --indentation_;
}
void Construct::while_stmt(const LangAPI::While& value) {
    auto expression = core_.expression(value.expr);
    emit_pending_lambdas();
    line("while " + expression + ":");
    ++indentation_;
    body_or_pass(value.stmt);
    --indentation_;
}
void Construct::do_while_stmt(const LangAPI::DoWhile& value) {
    const auto first = "_ispa_first_" + std::to_string(++switch_counter_);
    line(first + " = True");
    auto expression = core_.expression(value.expr);
    emit_pending_lambdas();
    line("while " + first + " or (" + expression + "):");
    ++indentation_;
    line(first + " = False");
    statements(value.stmt);
    --indentation_;
}
void Construct::switch_body(const LangAPI::Statements& s) {
    // A trailing C++ `break` ends the case; in an if/elif chain it would be a SyntaxError
    // (or would exit an enclosing loop), so it is dropped.
    std::vector<const LangAPI::Statement*> items;
    for (const auto& st : s) items.push_back(&st);
    if (!items.empty() && is_break(*items.back())) items.pop_back();
    if (items.empty()) line("pass");
    for (const auto* st : items) statement(*st);
}
void Construct::switch_stmt(const LangAPI::Switch& value) {
    // if/elif chains preserve the semantics of C++ switch in early Python 3.10.
    // The scrutinee is evaluated once, not once for each case.
    const auto temp = "_ispa_switch_" + std::to_string(++switch_counter_);
    auto scrutinee = core_.expression(value.expression);
    emit_pending_lambdas();
    line(temp + " = " + scrutinee);
    bool first = true;
    for (const auto& entry : value.cases) {
        auto test_value = core_.value(entry.first);
        emit_pending_lambdas();
        line(std::string(first ? "if " : "elif ") + temp + " == " + test_value + ":");
        ++indentation_;
        switch_body(entry.second);
        --indentation_;
        first = false;
    }
}
void Construct::statement(const LangAPI::Statement& stmt) {
    if (stmt.isExpression()) {
        const std::string expr = trim(core_.statement_expression(stmt.getExpression()));
        emit_pending_lambdas();
        if (!expr.empty()) {
            line(expr);
            if (debug_instrumentation_ && expr.find("ispa_std.assign_compatible(result.") != std::string::npos)
                line("ispa_debug.value(\"result\", result)");
        }
    } else if (stmt.isIf()) if_stmt(stmt.getIf());
    else if (stmt.isWhile()) while_stmt(stmt.getWhile());
    else if (stmt.isDoWhile()) do_while_stmt(stmt.getDoWhile());
    else if (stmt.isSwitch()) switch_stmt(stmt.getSwitch());
    else if (stmt.isThrow()) {
        const auto expression = core_.expression(stmt.getThrow().throw_value);
        emit_pending_lambdas();
        line("raise " + expression);
    }
    else if (stmt.isComment()) {
        const auto& value = stmt.getComment().value;
        if (value.starts_with("ISPA_DEBUG_LINE ")) {
            std::istringstream input(value.substr(16));
            std::string file, local, phase = "before";
            int row = 0, column = 0;
            input >> std::quoted(file) >> row >> column;
            if (!input || row <= 0) throw std::runtime_error("Malformed ISPA debug line marker");
            std::ostringstream args;
            bool first = true;
            while (input >> local) {
                if (local == "@after") { phase = "after"; continue; }
                const auto equal = local.find('=');
                const auto label = equal == std::string::npos ? local : local.substr(0, equal);
                const auto symbol = equal == std::string::npos ? local : local.substr(equal + 1);
                if (!first) args << ", ";
                first = false;
                args << std::quoted(label) << ": " << Core::identifier(symbol);
            }
            std::ostringstream call;
            call << "ispa_debug.line(" << std::quoted(file) << ", " << row << ", " << column
                 << ", {" << args.str() << "}, " << std::quoted(phase) << ")";
            line("if ispa_debug.enabled():");
            ++indentation_;
            line(call.str());
            --indentation_;
        } else if (!value.starts_with("ISPA_DEBUG_SCOPE ")) comment(value);
    }
    else if (stmt.isVariable()) variable(stmt.getVariable());
    else throw std::runtime_error("Python target: unsupported statement kind");
}
void Construct::variable(const LangAPI::Variable& var) {
    // Non-static C++ class fields are instance-specific. Class emission initializes
    // them from __init__ instead of creating shared class attributes here.
    if (class_context_ && !inside_function_ && !var.is_static) return;
    if (inside_function_) core_.declare(Core::identifier(var.name));  // local of the current def
    const bool default_init = var.value.empty() ||
        (var.value.size() == 1 && var.value.front().type() == LangAPI::ExpressionValueType::EmptyInitializer);
    std::string rhs;
    if (!default_init) rhs = core_.expression(var.value);
    else if (var.name == "match_result") rhs = "ispa_std.MatchResult()";
    else if (var.type.isSymbol() || var.type.isIspaLibSymbol()) rhs = core_.type(var.type) + "()";
    else if (var.type.isValueType()) {
        using V = LangAPI::ValueType;
        switch (var.type.getValueType()) {
            case V::Array: case V::FixedSizeArray: case V::Span: rhs = "[]"; break;
            case V::Map: rhs = "{}"; break;
            case V::Bool: rhs = "False"; break;
            // C++ default-initialises these; lr_capture_assign infers the destination type from them.
            case V::Char: case V::String: case V::NonOwnedString: rhs = "\"\""; break;
            case V::Float: rhs = "0.0"; break;
            case V::Int: case V::LargeUInt: rhs = "0"; break;
            default: rhs = "None"; break;
        }
    } else rhs = "None";
    if (class_context_ && !inside_function_ && var.is_static &&
        rhs.starts_with(Core::identifier(namespace_name) + ".FlatTypes.")) {
        deferred_aliases_.emplace_back(current_scope_ + "." + Core::identifier(var.name), rhs);
        return;
    }
    // A namespace is a class body: an initializer naming the namespace's own classes by their qualified
    // path runs before that path exists, so it is assigned once the outermost namespace is complete.
    if (namespace_context_ && !class_context_ && !inside_function_ && !current_scope_.empty() && !rhs.empty() &&
        rhs.find(current_scope_.substr(0, current_scope_.find('.')) + ".") != std::string::npos) {
        deferred_aliases_.emplace_back(current_scope_ + "." + Core::identifier(var.name), rhs);
        return;
    }
    emit_pending_lambdas();
    line(Core::identifier(var.name) + " = " + rhs);
    if (debug_instrumentation_ && rhs.find("captures.") != std::string::npos)
        line("ispa_debug.capture(" + std::string("\"") + Core::identifier(var.name) +
             "\", " + Core::identifier(var.name) + ")");
}
void Construct::alias(const LangAPI::TypeAlias& value) {
    auto rhs = core_.type(value.type);
    const bool flat_alias = rhs.starts_with(Core::identifier(namespace_name) + ".FlatTypes.");
    if (!value.template_parameters.empty()) {
        std::string params;
        for (const auto& p : value.template_parameters) params += (params.empty() ? "" : ", ") + Core::identifier(p);
        rhs = "lambda " + params + ": " + rhs;
    }
    if ((class_context_ || current_scope_ == "FlatTypes") &&
        flat_alias) {
        deferred_aliases_.emplace_back(current_scope_ + "." + Core::identifier(value.name), rhs);
        return;
    }
    line(Core::identifier(value.name) + " = " + rhs);
}
void Construct::enumeration(const LangAPI::Enum& value) {
    line("class " + Core::identifier(value.name) + "(enum.IntEnum):");
    ++indentation_;
    if (value.value.empty()) line("pass");
    else {
        std::size_t i = 0;
        for (const auto& el : value.value) {
            line(Core::identifier(el) + " = " + std::to_string(i++));
        }
    }
    --indentation_;
}
void Construct::function(const LangAPI::Function& fn, bool ctor) {
    std::string name = ctor ? "__init__" : Core::identifier(fn.name);
    if (name == "operator<<") return; // C++ output operator maps to __str__ separately.
    // Namespace functions are static; class methods are instance-bound unless declared static.
    if (!ctor) {
        if (class_context_ && fn.is_static) line("@staticmethod");
        else if (namespace_context_ && !class_context_) line("@staticmethod");
        if (!overloaded_scopes_.empty() && overloaded_scopes_.back().contains(fn.name))
            line("@ispa_std.overload");  // dispatch cost is only paid where a name repeats
    }
    std::string args;
    const bool is_method = class_context_ && !fn.is_static;
    if (is_method || ctor) args = "self";
    for (const auto& [parameter, constraints] : fn.template_parameters) {
        if (!constraints.empty() || parameter.starts_with("ISPAParam")) continue;
        if (!args.empty()) args += ", ";
        args += Core::identifier(parameter);
    }
    for (const auto& param : fn.parameters) {
        if (!args.empty()) args += ", ";
        args += Core::identifier(param.second) + ": " + core_.type(param.first);
    }
    const auto result = ctor ? "" : " -> " + core_.type(fn.type);
    line("def " + name + "(" + args + ")" + result + ":");
    ++indentation_;
    const bool saved_inside_function = inside_function_;
    inside_function_ = true;
    if (is_method || ctor) core_.set_method_members(class_members_);
    {
        std::vector<std::string> names;
        for (const auto& param : fn.parameters) names.push_back(Core::identifier(param.second));
        core_.enter_scope(names);
    }
    LangAPI::Statements body = fn.statements;
    unsigned debug_scopes = 0;
    while (!body.empty() && body.front().isComment() &&
           body.front().getComment().value.starts_with("ISPA_DEBUG_SCOPE ")) {
        std::istringstream input(body.front().getComment().value.substr(17));
        std::string kind, scope_name, file;
        int row = 0, column = 0;
        input >> kind >> std::quoted(scope_name) >> std::quoted(file) >> row >> column;
        if (!input || (kind != "rule" && kind != "production"))
            throw std::runtime_error("Malformed ISPA debug scope marker");
        std::ostringstream call;
        call << "with ispa_debug.scope(" << std::quoted(kind) << ", " << std::quoted(scope_name)
             << ", " << std::quoted(file) << ", " << row << ", " << column << "):";
        line(call.str());
        ++indentation_;
        ++debug_scopes;
        body.erase(body.begin());
    }
    bool emitted = false;
    if (ctor) {
        // Initialize every non-static member before custom constructor statements.
        // __init__ never shares mutable default arrays or dictionaries between instances.
        // This prelude is emitted by cls(), not here.
    }
    if (!body.empty()) {
        statements(body);
        emitted = true;
    }
    if (!emitted) line("pass");
    while (debug_scopes > 0) { --indentation_; --debug_scopes; }
    core_.leave_scope();
    inside_function_ = saved_inside_function;
    core_.set_method_members({});
    --indentation_;
}
void Construct::cls(const LangAPI::Class& value) {
    const auto full_name = Core::symbol_path(core_.type(LangAPI::Type{value.name}));
    const auto path = split_path(full_name);
    auto name = path.empty() ? full_name : path.back();
    const bool qualified = path.size() > 1 && !class_context_;
    if (qualified) name = "_ispa_generated_class_" + std::to_string(++hidden_class_counter_);
    const bool generic = !value.template_parameters.empty();
    if (generic) {
        std::string params;
        for (const auto& parameter : value.template_parameters)
            params += (params.empty() ? "" : ", ") + Core::identifier(parameter);
        line("@ispa_std.template");
        line("def " + name + "(" + params + "):");
        ++indentation_;
    }

    std::string bases;
    bool has_lib_base = false;  // Lexer/Parser subclass: its __init__ must chain to the runtime
    for (const auto& inherit : value.inherit_members) {
        if (!bases.empty()) bases += ", ";
        if (const auto* sym = std::get_if<LangAPI::Symbol>(&inherit.second)) {
            auto base = Core::symbol_path(core_.type(LangAPI::Type{*sym}));
            // The class body runs before its qualified path exists: a sibling is named directly.
            if (!current_scope_.empty() && base.starts_with(current_scope_ + ".") &&
                base.find('.', current_scope_.size() + 1) == std::string::npos)
                base = base.substr(current_scope_.size() + 1);
            bases += base;
        } else {
            // ISPA lexer/parser inheritance is a real Python subclass.
            has_lib_base = true;
            bases += core_.type(LangAPI::Type{std::get<LangAPI::IspaLibSymbol>(inherit.second)});
        }
    }
    if (bases.empty()) bases = "object";
    line("class " + name + "(" + bases + "):");
    ++indentation_;
    const bool old_class = class_context_;
    const bool old_ns = namespace_context_;
    class_context_ = true;
    namespace_context_ = false;
    const auto saved_scope = current_scope_;
    current_scope_ = saved_scope.empty() ? name : saved_scope + "." + name;
    const auto saved_members = class_members_;
    class_members_.clear();
    for (const auto& [decl, access] : value.data) {
        if (decl->isVariable()) class_members_.insert(decl->getVariable().name);
        if (decl->isFunction()) class_members_.insert(decl->getFunction().name);
    }
    if (name == "Lexer") {
        class_members_.insert({"tokens", "lookup", "append_token", "source", "set_input"});
    } else if (name == "Parser") {
        class_members_.insert({"lr_parse", "lr_parse_generalized", "getErrors"});
        class_members_.insert({"lexer", "tree", "errors", "required", "optional", "range", "counted",
            "range_optional", "skip_spaces", "recovery_frame", "recovery_at", "unwinding", "action_result",
            "counter_scope", "bind_counter"});
        // Recovery runtime (ispa_std/parser.py). recover_choice stays bare: Core::call lowers it itself.
        class_members_.insert({"getContextErrors", "fail_context", "rule_context", "custom_failure_decision",
            "required_recovering", "sync_loop", "note_match", "reject_absent_rule", "recover_at_context",
            "recovery_state"});
    }

    // Keep instance attributes independent even when initializer is a mutable value.
    std::vector<const LangAPI::Variable*> fields;
    for (const auto& [decl, access] : value.data) {
        if (decl->isVariable() && !decl->getVariable().is_static)
            fields.push_back(&decl->getVariable());
    }
    if (value.constructor || !fields.empty()) {
        if (value.constructor) {
            const auto& ctor = *value.constructor;
            std::string args = "self";
            for (const auto& param : ctor.parameters)
                args += ", " + Core::identifier(param.second) + ": " + core_.type(param.first);
            line("def __init__(" + args + "):");
            ++indentation_;
            // Locals declared in the constructor body are real variables, not class fields.
            const bool saved_inside = inside_function_;
            inside_function_ = true;
            core_.set_method_members(class_members_);
            {
                std::vector<std::string> names;
                for (const auto& param : ctor.parameters) names.push_back(Core::identifier(param.second));
                core_.enter_scope(names);
            }
            if (has_lib_base) line("super().__init__()");
            for (const auto* f : fields) {
                const auto init = f->value.empty() ? std::string("None") : core_.expression(f->value);
                emit_pending_lambdas();  // def must precede the line that uses it
                line("self." + Core::identifier(f->name) + " = " + init);
            }
            if (ctor.statements.empty() && fields.empty() && !has_lib_base) line("pass");
            else statements(ctor.statements);
            core_.leave_scope();
            inside_function_ = saved_inside;
            core_.set_method_members({});
            --indentation_;
        } else {
            // Python has no C++ aggregate initialization: provide both positional
            // and keyword field constructors without sharing mutable defaults.
            std::string params = "self";
            if (has_lib_base) params += ", *args";
            for (const auto* f : fields) params += ", " + Core::identifier(f->name) + " = _ISPA_UNSET";
            if (has_lib_base) params += ", **kwargs";
            line("def __init__(" + params + "):");
            ++indentation_;
            if (has_lib_base) line("super().__init__(*args, **kwargs)");
            core_.set_method_members(class_members_);
            for (const auto* f : fields) {
                const auto field = Core::identifier(f->name);
                const bool empty = f->value.empty() ||
                    (f->value.size() == 1 && f->value.front().type() == LangAPI::ExpressionValueType::EmptyInitializer);
                std::string def = empty ? std::string("None") : core_.expression(f->value);
                if (empty && f->type.isValueType()) {
                    switch (f->type.getValueType()) {
                        case LangAPI::ValueType::Array:
                        case LangAPI::ValueType::FixedSizeArray: def = "[]"; break;
                        case LangAPI::ValueType::Map: def = "{}"; break;
                        default: break;
                    }
                }
                emit_pending_lambdas();
                line("self." + field + " = " + def + " if " + field + " is _ISPA_UNSET else " + field);
            }
            core_.set_method_members({});
            --indentation_;
        }
    }

    bool any = value.constructor.has_value() || !fields.empty();
    {
        std::unordered_set<std::string> seen, repeated;
        for (const auto& [decl, access] : value.data)
            if (decl->isFunction() && !seen.insert(decl->getFunction().name).second)
                repeated.insert(decl->getFunction().name);
        overloaded_scopes_.push_back(std::move(repeated));
    }
    for (const auto& [decl, access] : value.data) {
        if (decl->isVariable() && !decl->getVariable().is_static) continue;
        declaration(*decl);
        any = true;
    }
    if (value.to_str_fun) {
        function(*value.to_str_fun);
        any = true;
    }
    overloaded_scopes_.pop_back();
    if (!any) line("pass");
    namespace_context_ = old_ns;
    class_context_ = old_class;
    class_members_ = saved_members;
    current_scope_ = saved_scope;
    --indentation_;
    if (generic) {
        line("return " + name);
        --indentation_;
    }
    if (namespace_context_ && !class_context_ && !current_scope_.empty() && !qualified)
        line(current_scope_ + "." + name + " = " + name);
    if (qualified) {
        std::string target;
        for (std::size_t i = 0; i < path.size(); ++i) {
            if (i) target += ".";
            target += Core::identifier(path[i]);
        }
        deferred_aliases_.emplace_back(target, name);
    }
}
void Construct::ns(const LangAPI::Namespace& value) {
    // The outer C++ namespace is the Python module itself. A class with the
    // same name would hide the module alias and strand its nested declarations.
    if (!namespace_context_ && value.name == namespace_name) {
        declarations(value.declarations);
        return;
    }
    const auto saved_scope = current_scope_;
    const auto path = saved_scope.empty() ? Core::identifier(value.name) : saved_scope + "." + Core::identifier(value.name);
    // Base classes are evaluated immediately, before enclosing class bodies
    // finish. Publish a namespace shell so imported modules can refer to one
    // another while their definitions are being evaluated.
    line(path + " = type('_ispa_namespace', (), {})");
    line("class " + Core::identifier(value.name) + ":");
    ++indentation_;
    current_scope_ = path;
    const bool saved_ns = namespace_context_;
    const bool saved_class = class_context_;
    namespace_context_ = true;
    class_context_ = false;
    if (value.declarations.empty()) line("pass");
    declarations(value.declarations);
    namespace_context_ = saved_ns;
    class_context_ = saved_class;
    current_scope_ = saved_scope;
    --indentation_;
    if (!saved_scope.empty()) line(path + " = " + Core::identifier(value.name));
    if (saved_scope.empty()) {
        for (const auto& [target, rhs] : deferred_aliases_) line(target + " = " + rhs);
        deferred_aliases_.clear();
    }
}
void Construct::declaration(const LangAPI::Declaration& d) {
    if (d.isNamespace()) ns(d.getNamespace());
    else if (d.isFunction()) function(d.getFunction());
    else if (d.isClass()) cls(d.getClass());
    else if (d.isEnum()) enumeration(d.getEnum());
    else if (d.isTypeAlias()) alias(d.getTypeAlias());
    else if (d.isVariable()) variable(d.getVariable());
    else if (d.isForwardDeclaredClass()) {
        // A Python name must exist, but its full definition may follow later.
        // Python's first-class classes cannot be forward-declared like C++.
    }
    else throw std::runtime_error("Python target: unsupported declaration");
}
void Construct::declarations(const LangAPI::Declarations& decls) {
    std::unordered_set<std::string> seen, repeated;
    for (const auto& d : decls)
        if (d.isFunction() && !seen.insert(d.getFunction().name).second)
            repeated.insert(d.getFunction().name);
    overloaded_scopes_.push_back(std::move(repeated));
    std::unordered_set<std::string> emitted_namespaces;
    for (const auto* ordered : order_by_inheritance(decls)) {
        const auto& d = *ordered;
        if (!d.isNamespace()) {
            declaration(d);
            continue;
        }
        const auto& name = d.getNamespace().name;
        if (!emitted_namespaces.insert(name).second) continue;
        LangAPI::Namespace combined = d.getNamespace();
        for (const auto& other : decls) {
            if (&other == &d || !other.isNamespace() || other.getNamespace().name != name) continue;
            const auto& members = other.getNamespace().declarations;
            combined.declarations.insert(combined.declarations.end(), members.begin(), members.end());
        }
        ns(combined);
    }
    overloaded_scopes_.pop_back();
}
void Construct::construct() {
    output_.clear();
    files_.clear();
    indentation_ = 0;
    deferred_aliases_.clear();
    overloaded_scopes_.clear();
    hidden_class_counter_ = switch_counter_ = 0;
    class_context_ = namespace_context_ = inside_function_ = false;
    line("# Generated by ISPA Python target. Changes will be overwritten.");
    line("from __future__ import annotations");
    line("import enum");
    line("import sys as _sys");
    line("import typing");
    // Before `import ispa_std`: the Python bootstrap rewrites that line to add its own custom_failure import.
    line("from ispa_std import custom_failure, custom_failure_decision");
    line("import ispa_std");
    if (debug_instrumentation_) line("import ispa_debug");
    line("_ISPA_UNSET = object()");
    line();
    line(Core::identifier(namespace_name) + " = _sys.modules[__name__]");
    line();
    rewritten_source_ = rewriter_.run(*source);
    std::unordered_set<std::string> generic_functions;
    const auto collect = [&](auto&& self, const LangAPI::Declarations& decls) -> void {
        for (const auto& d : decls) {
            if (d.isFunction() && std::ranges::any_of(d.getFunction().template_parameters,
                [](const auto& parameter) { return parameter.second.empty() && !parameter.first.starts_with("ISPAParam"); }))
                generic_functions.insert(Core::identifier(d.getFunction().name));
            else if (d.isNamespace()) self(self, d.getNamespace().declarations);
            else if (d.isClass()) {
                LangAPI::Declarations members;
                for (const auto& [member, visibility] : d.getClass().data) members.push_back(*member);
                self(self, members);
            }
        }
    };
    collect(collect, rewritten_source_);
    core_.set_generic_functions(std::move(generic_functions));
    declarations(rewritten_source_);
    // C++ cursor snapshots are value copies. Python cursors are mutable, so
    // generated fail blocks must snapshot them explicitly before sync/skip.
    replace_all(output_, "start_pos = pos\n", "start_pos = pos.copy()\n");
    replace_all(output_, "before_failure = failed_pos\n", "before_failure = failed_pos.copy()\n");
    line();
    // Only wire what this grammar actually defines (a lexer-only or parser-less module has neither).
    const bool has_lexer = output_.find("\nclass Lexer(") != std::string::npos;
    const bool has_parser = output_.find("\nclass Parser(") != std::string::npos;
    if (has_parser && has_lexer) line("Parser.lexer_type = Lexer");
    if (has_parser && output_.find("\nclass Types:") != std::string::npos &&
        output_.find("_ispa__WS") != std::string::npos)
        line("Parser.parser_ws_type = Types._ispa__WS");
    if (!deferred_aliases_.empty()) {
        line();
        line("# Qualified classes (e.g. Types.rule.STRING) defined after their parents");
        for (const auto& [target, alias_] : deferred_aliases_)
            line(target + " = " + alias_);
    }
    // The Python grammar's advanced layout tokens are lowered through the C++
    // lexer API. Emit runnable Python operations and connect the generated DFA
    // to the Python standard runtime's EOF/comment/string handling.
    if (output_.find("ispa_shared_indents") != std::string::npos &&
        output_.find("Types.modules.tokens.NEWLINE()") != std::string::npos) {
        const auto newline_action = bootstrap_newline_action(output_);
        if (newline_action.empty())
            throw std::runtime_error("Python bootstrap NEWLINE action was not generated");
        replace_all(output_, "std.string(start, start + length)",
                    "start[start_pos:start_pos + length]");
        replace_all(output_, "str(start, start + length)",
                    "start[start_pos:start_pos + length]");
        replace_all(output_, "_ispa__next_char(((start) + (length)))",
                    "self._next_char_at(start_pos + length)");
        replace_all(output_, "_ispa__line(((start) + (length)))",
                    "self._line(start_pos + length)");
        replace_all(output_, "((start) + (length))", "start");
        replace_all(output_, "self.ispa_pending_tokens.push_back(",
                    "self.ispa_pending_tokens.append(");
        replace_all(output_, "(self.ispa_shared_indents).size()",
                    "len(self.ispa_shared_indents)");
        replace_all(output_, "(ispa_token_text).size()", "len(ispa_token_text)");
        replace_all(output_, "(ispa_token_text).substr(ispa_action_i, 1)",
                    "ispa_token_text[ispa_action_i:ispa_action_i + 1]");
        replace_all(output_, "== (\"\\\\n\")", "== (\"\\n\")");
        replace_all(output_, "== (\"\\\\t\")", "== (\"\\t\")");
        replace_all(output_, "Parser.lexer_type = Lexer",
                    "Parser.lexer_type = ispa_std.make_python_lexer(_sys.modules[__name__], " + newline_action + ")");
    }
    // Python is already module-incremental: avoid generating 800 import fragments.
    // The incremental flag is accepted so the host may use the same C ABI.
    files_.emplace_back(std::string(namespace_name) + ".py", std::move(output_));
}
}

extern "C" auto python_construct(const LangAPI::Declarations* tree, const char* module_name,
                                 bool incremental, bool debug_instrumentation)
    -> Python::Construct* {
    return new Python::Construct(tree, module_name, incremental, debug_instrumentation);
}
extern "C" auto python_construct_finish(Python::Construct* ptr) -> void {
    delete ptr;
}
