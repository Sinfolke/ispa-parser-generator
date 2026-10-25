module Python.Rewriter;

import std;
import dstd;

namespace Python {
namespace {
    std::string at(const std::string& context, const char* segment) {
        return context + "/" + segment;
    }
    [[noreturn]] void cannot_lower(const std::string& context, const std::string& message) {
        throw std::runtime_error("Python rewriter at " + context + ": " + message);
    }
    bool is_increment(const LangAPI::ExpressionValue& value) {
        return value.isExpressionElement() &&
            (value.getExpressionElement() == LangAPI::ExpressionElement::PlusPlus ||
             value.getExpressionElement() == LangAPI::ExpressionElement::MinusMinus);
    }
    bool assignable(const LangAPI::Symbol& sym) {
        if (sym.path.empty()) return false;
        // A symbol containing calls is an expression, not a safe assignable path.
        for (const auto& part : sym.path)
            if (!std::holds_alternative<std::string>(part)) return false;
        return true;
    }
    bool assignable(const LangAPI::StorageSymbol& sym) {
        if (sym.path.empty()) return false;
        // The final segment must denote a location (not a method call or a property getter).
        const auto& last = sym.path.back();
        return std::holds_alternative<std::string>(last) ||
               std::holds_alternative<LangAPI::StorageOffset>(last);
    }
}

LangAPI::Declarations Rewriter::run(const LangAPI::Declarations& input) {
    stats_ = {};
    LangAPI::Declarations result = input;
    declarations(result, "<module>");
    return result;
}

void Rewriter::declarations(LangAPI::Declarations& decls, const std::string& context) {
    for (auto& decl : decls) declaration(decl, context);
}

void Rewriter::declaration(LangAPI::Declaration& decl, const std::string& context) {
    if (decl.isNamespace()) {
        auto& ns = decl.getNamespace();
        declarations(ns.declarations, context + "::" + ns.name);
    } else if (decl.isFunction()) {
        auto& fn = decl.getFunction();
        const auto location = context + "::" + fn.name;
        type(fn.type, location);
        for (auto& [t, name] : fn.parameters) type(t, location + "/param:" + name);
        for (auto& [name, constraints] : fn.template_parameters)
            for (auto& t : constraints) type(t, location + "/template:" + name);
        statements(fn.statements, location);
    } else if (decl.isClass()) {
        auto& cls = decl.getClass();
        std::string label = "class";
        for (const auto& part : cls.name.path)
            if (const auto* text = std::get_if<std::string>(&part)) label = *text;
        const auto location = at(context, label.c_str());
        symbol(cls.name, location);
        for (auto& base : cls.inherit_members) {
            if (auto* sym = std::get_if<LangAPI::Symbol>(&base.second)) symbol(*sym, location);
            // IspaLibSymbol template parameters have the shared pointer form.
            else template_args(std::get<LangAPI::IspaLibSymbol>(base.second).template_parameters, location);
        }
        for (auto& [owned, visibility] : cls.data) {
            if (!owned) cannot_lower(location, "null class declaration");
            // Deep-copy *before* touching members; a copied Class otherwise aliases input.
            owned = std::make_shared<LangAPI::Declaration>(*owned);
            declaration(*owned, location);
        }
        if (cls.constructor) {
            auto& fn = *cls.constructor;
            type(fn.type, location + "/constructor");
            for (auto& [t, name] : fn.parameters) type(t, location + "/constructor/param:" + name);
            statements(fn.statements, location + "/constructor");
        }
        if (cls.to_str_fun) {
            type(cls.to_str_fun->type, location + "/to_str");
            statements(cls.to_str_fun->statements, location + "/to_str");
        }
        if (cls.output_fun) {
            type(cls.output_fun->type, location + "/output");
            statements(cls.output_fun->statements, location + "/output");
        }
    } else if (decl.isVariable()) variable(decl.getVariable(), context);
    else if (decl.isTypeAlias()) type(decl.getTypeAlias().type, context + "/alias:" + decl.getTypeAlias().name);
}

void Rewriter::variable(LangAPI::Variable& value, const std::string& context) {
    const auto location = context + "/var:" + value.name;
    type(value.type, location);
    expression(value.value, location + "/initializer");
    for (auto& e : value.set) expression(e, location + "/set");
}

void Rewriter::statements(LangAPI::Statements& stmts, const std::string& context) {
    for (auto& stmt : stmts) statement(stmt, context);
}

void Rewriter::statement(LangAPI::Statement& stmt, const std::string& context) {
    if (stmt.isExpression()) {
        LangAPI::Expression lowered;
        if (increment_as_assignment(stmt.getExpression(), lowered, context)) {
            stmt.value = std::move(lowered);
        }
        expression(stmt.getExpression(), context);
    } else if (stmt.isVariable()) variable(stmt.getVariable(), context);
    else if (stmt.isIf()) {
        auto& s = stmt.getIf();
        expression(s.expr, context + "/if-condition");
        statements(s.stmt, context + "/if-then");
        statements(s.else_stmt, context + "/if-else");
    } else if (stmt.isWhile() || stmt.isDoWhile()) {
        auto& s = stmt.getWhileOrDoWhile();
        expression(s.expr, context + "/loop-condition");
        statements(s.stmt, context + "/loop-body");
    } else if (stmt.isSwitch()) {
        auto& s = stmt.getSwitch();
        expression(s.expression, context + "/switch");
        for (auto& [label, body] : s.cases) {
            rvalue(label, context + "/case-label");
            statements(body, context + "/case-body");
        }
    } else if (stmt.isThrow()) expression(stmt.getThrow().throw_value, context + "/throw");
}

bool Rewriter::increment_as_assignment(const LangAPI::Expression& input,
                                       LangAPI::Expression& result, const std::string& context) {
    // Outer grouping is syntactic only for a stand-alone statement.
    std::size_t first = 0, last = input.size();
    using E = LangAPI::ExpressionElement;
    auto is = [&](std::size_t i, E value) {
        return input[i].isExpressionElement() && input[i].getExpressionElement() == value;
    };
    while (last >= first + 4 && is(first, E::GroupOpen) && is(last - 1, E::GroupClose)) {
        ++first;
        --last;
    }
    if (last - first != 2) return false;
    const bool prefix = is_increment(input[first]);
    const bool postfix = is_increment(input[last - 1]);
    if (prefix == postfix) return false;

    const auto& oper = input[prefix ? first : last - 1];
    const auto& operand = input[prefix ? first + 1 : first];
    if (!operand.isRvalue())
        cannot_lower(context, "++/-- requires an assignable symbol or storage location");
    const auto& value = operand.getRValue();

    LangAPI::VariableAssignment assignment{};
    if (value.isSymbol()) {
        if (!assignable(value.getSymbol()))
            cannot_lower(context, "++/-- on a non-assignable symbol");
        assignment.name = value.getSymbol();
    } else if (value.isStorageSymbol()) {
        if (!assignable(value.getStorageSymbol()))
            cannot_lower(context, "++/-- on a non-assignable storage location");
        assignment.name = value.getStorageSymbol();
    } else {
        cannot_lower(context, "++/-- requires Symbol or StorageSymbol (not a literal, call, or cursor)");
    }

    const bool plus = oper.getExpressionElement() == E::PlusPlus;
    assignment.type = plus ? LangAPI::OperatorType::Add : LangAPI::OperatorType::Minus;
    LangAPI::Int one{};
    one.value = 1;
    assignment.value = LangAPI::Expression{LangAPI::ExpressionValue{LangAPI::RValue{one}}};
    result = LangAPI::Expression{LangAPI::ExpressionValue{std::move(assignment)}};
    if (plus) ++stats_.increments; else ++stats_.decrements;
    return true;
}

void Rewriter::expression(LangAPI::Expression& expr, const std::string& context) {
    // Standalone `x++;` was already turned into `x += 1` by statement(). A ++/-- that remains
    // here produces a value, so it is left in place: Python.Core lowers it (walrus for a local
    // name, ispa_std.step_attr / step_item for attributes and subscripts) and reports operands
    // it cannot express.
    for (auto& part : expr) expr_value(part, context);
}

void Rewriter::expr_value(LangAPI::ExpressionValue& part, const std::string& context) {
    if (part.isRvalue()) rvalue(part.getRValue(), context);
    else if (part.isFunctionCall()) function_call(part.getFunctionCall(), context);
    else if (part.isIspaLibFunctionCall()) lib_call(part.getIspaLibFunctionCall(), context);
    else if (part.isVariableAssignment()) {
        auto& v = part.getVariableAssignment();
        if (auto* sym = std::get_if<LangAPI::Symbol>(&v.name)) symbol(*sym, context);
        else storage(std::get<LangAPI::StorageSymbol>(v.name), context);
        expression(v.value, context + "/assignment-rhs");
    } else if (part.isReturn()) expression(part.getReturn().value, context + "/return");
    else if (part.isCounterAssignment()) {
        auto& p = part.getAssignCounter().v;
        if (p) {
            p = std::make_shared<LangAPI::Expression>(*p);
            expression(*p, context + "/counter-assignment");
        }
    } else if (part.isLambda()) lambda(part.getLambda(), context + "/lambda");
}

void Rewriter::lambda(LangAPI::Lambda& value, const std::string& context) {
    for (auto& [t, name] : value.parameters) type(t, context + "/param:" + name);
    statements(value.statements, context);
}

void Rewriter::function_call(LangAPI::FunctionCall& call, const std::string& context) {
    if (auto* name = std::get_if<std::shared_ptr<LangAPI::Symbol>>(&call.name)) {
        if (*name) {
            *name = std::make_shared<LangAPI::Symbol>(**name);
            symbol(**name, context);
        }
    } else {
        auto& p = std::get<std::shared_ptr<LangAPI::IspaLibSymbol>>(call.name);
        if (p) {
            p = std::make_shared<LangAPI::IspaLibSymbol>(*p);
            template_args(p->template_parameters, context);
        }
    }
    template_args(call.template_parameters, context);
    for (auto& arg : call.args) expression(arg, context + "/arg");
}

void Rewriter::lib_call(LangAPI::IspaLibFunctionCall& call, const std::string& context) {
    template_args(call.symbol.template_parameters, context);
    template_args(call.template_parameters, context);
    for (auto& arg : call.args) expression(arg, context + "/arg");
}

void Rewriter::symbol(LangAPI::Symbol& sym, const std::string& context) {
    for (auto& part : sym.path) {
        if (auto* fn = std::get_if<LangAPI::FunctionCall>(&part)) function_call(*fn, context);
        else if (auto* lib = std::get_if<LangAPI::IspaLibSymbol>(&part))
            template_args(lib->template_parameters, context);
    }
}

void Rewriter::storage(LangAPI::StorageSymbol& sym, const std::string& context) {
    expression(sym.what, context + "/base");
    for (auto& part : sym.path) {
        if (auto* index = std::get_if<LangAPI::StorageOffset>(&part))
            expression(index->offset, context + "/offset");
        else if (auto* fn = std::get_if<LangAPI::FunctionCall>(&part)) function_call(*fn, context);
        else if (auto* method = std::get_if<LangAPI::ArrayMethodCall>(&part))
            for (auto& arg : method->args) expression(arg, context + "/method-arg");
        else if (auto* lib = std::get_if<LangAPI::IspaLibSymbol>(&part))
            template_args(lib->template_parameters, context);
    }
}

void Rewriter::type(LangAPI::Type& t, const std::string& context) {
    if (t.isValueType() && t.getValueType() == LangAPI::ValueType::Any) {
        t.type = LangAPI::ValueType::Undef;  // both types erase to typing.Any in the Python target
        ++stats_.erased_any_types;
    } else if (t.isSymbol()) symbol(t.getSymbol(), context);
    else if (t.isIspaLibSymbol()) template_args(t.getIspaLibSymbol().template_parameters, context);
    for (auto& arg : t.template_parameters) {
        if (auto* inner = std::get_if<LangAPI::Type>(&arg)) type(*inner, context);
        else rvalue(std::get<LangAPI::RValue>(arg), context);
    }
}

void Rewriter::template_args(
    stdu::vector<std::variant<std::shared_ptr<LangAPI::Type>, std::shared_ptr<LangAPI::RValue>>>& args,
    const std::string& context) {
    for (auto& arg : args) {
        if (auto* t = std::get_if<std::shared_ptr<LangAPI::Type>>(&arg)) {
            if (*t) {
                *t = std::make_shared<LangAPI::Type>(**t);
                type(**t, context);
            }
        } else {
            auto& v = std::get<std::shared_ptr<LangAPI::RValue>>(arg);
            if (v) {
                v = std::make_shared<LangAPI::RValue>(*v);
                rvalue(*v, context);
            }
        }
    }
}

void Rewriter::rvalue(LangAPI::RValue& value, const std::string& context) {
    using R = LangAPI::RValueType;
    switch (value.type()) {
        case R::Array: {
            auto& a = value.getArray();
            for (auto& e : a.values) expression(e, context + "/array");
            template_args(a.template_parameters, context);
            break;
        }
        case R::FixedSizeArray: {
            auto& a = value.getFixedSizeArray();
            for (auto& e : a.values) expression(e, context + "/array");
            template_args(a.template_parameters, context);
            break;
        }
        case R::Map: {
            auto& m = value.getMap();
            for (auto& e : m.values) expression(e, context + "/map");
            template_args(m.template_parameters, context);
            break;
        }
        case R::Symbol: symbol(value.getSymbol(), context); break;
        case R::IspaLibSymbol: template_args(value.getIspaSymbol().template_parameters, context); break;
        case R::StorageSymbol: storage(value.getStorageSymbol(), context); break;
        case R::Inheritance: {
            auto& inh = value.getInheritance();
            if (auto* sym = std::get_if<LangAPI::Symbol>(&inh.name)) symbol(*sym, context);
            else if (auto* lib = std::get_if<LangAPI::IspaLibSymbol>(&inh.name))
                template_args(lib->template_parameters, context);
            else {
                auto& ptr = std::get<std::shared_ptr<LangAPI::Type>>(inh.name);
                if (ptr) { ptr = std::make_shared<LangAPI::Type>(*ptr); type(*ptr, context); }
            }
            for (auto& arg : inh.args) expression(arg, context + "/constructor-arg");
            break;
        }
        case R::IspaLibDfaEmptyState: {
            auto& ptr = value.getIspaLibDfaEmptyState().construction_lambda;
            if (ptr) { ptr = std::make_shared<LangAPI::Lambda>(*ptr); lambda(*ptr, context); }
            break;
        }
        case R::IspaLibDfaSpan: {
            auto& s = value.getIspaLibDfaSpan();
            template_args(s.type.template_parameters, context);
            symbol(s.assing_name, context);
            break;
        }
        case R::Reference: {
            auto& ptr = value.getReference().value;
            if (ptr) { ptr = std::make_shared<LangAPI::RValue>(*ptr); rvalue(*ptr, context); }
            break;
        }
        case R::Span: {
            auto& s = value.getSpan();
            symbol(s.sym, context);
            if (s.type) { s.type = std::make_shared<LangAPI::Type>(*s.type); type(*s.type, context); }
            break;
        }
        case R::MakeTuple:
            for (auto& arg : value.getMakeTuple().args) expression(arg, context + "/tuple");
            break;
        case R::GetVariant: {
            auto& v = value.getVariantCast();
            expression(v.sym, context + "/variant");
            if (v.type) { v.type = std::make_shared<LangAPI::Type>(*v.type); type(*v.type, context); }
            break;
        }
        case R::CheckVariant: {
            auto& v = value.CheckVariantCast();
            expression(v.sym, context + "/variant");
            if (v.type) { v.type = std::make_shared<LangAPI::Type>(*v.type); type(*v.type, context); }
            break;
        }
        case R::Cast: {
            auto& v = value.getCast();
            expression(v.what, context + "/cast");
            if (v.type) { v.type = std::make_shared<LangAPI::Type>(*v.type); type(*v.type, context); }
            break;
        }
        case R::GetFromBox: expression(value.getGetFromBox().what, context); break;
        case R::ToString: expression(value.getToString().what, context); break;
        case R::CharToStringConstructor: expression(value.getCharToStringConstructor().what, context); break;
        default: break;
    }
}
} // namespace Python
