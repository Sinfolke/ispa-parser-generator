module Cpp.CoreFunctions;

import corelib;
import logging;
import AST.API;
import constants;
import dstd;
import std;

auto Core::isNestedArrayType(const LangAPI::Type &t) -> bool {
    if (t.isValueType() && (t.getValueType() == LangAPI::ValueType::Array || t.getValueType() == LangAPI::ValueType::FixedSizeArray)) {
        for (const auto &param : t.template_parameters) {
            if (std::holds_alternative<LangAPI::Type>(param)) {
                const auto &type = std::get<LangAPI::Type>(param);
                if (type.isValueType() && (type.getValueType() == LangAPI::ValueType::Array || type.getValueType() == LangAPI::ValueType::FixedSizeArray)) {
                    return true;
                }
            }
        }
    }
    return false;
}
template<LangAPI::ValueType ...VT>
auto isNestedT(const LangAPI::Type &t) -> bool {
    for (const auto &param : t.template_parameters) {
        if (std::holds_alternative<LangAPI::Type>(param)) {
            const auto &type = std::get<LangAPI::Type>(param);
            if (type.isValueType() && ((type.getValueType() == VT) || ...)) {
                return true;
            }
        }
    }
    return false;
}
// "::Name::" - or just "::" when no name is configured (used to give "::::Tokens")
static auto scopePrefix() -> std::string { return Core::name.empty() ? std::string("::") : "::" + Core::name + "::"; }
auto Core::convertType(const LangAPI::Type &type) -> std::string {
    if (type.isValueType()) {
        switch (type.getValueType()) {
            case LangAPI::ValueType::Void:
                return "void";
            case LangAPI::ValueType::ImmutableSemantic: {
                if (isNestedT<LangAPI::ValueType::NonOwnedString>(type)) {
                    return convertTemplates(type.template_parameters) + " const";
                } else {
                    return "const " + convertTemplates(type.template_parameters);
                }
            }
            case LangAPI::ValueType::BorrowSemantic:
                return convertTemplates(type.template_parameters) + "&";
            case LangAPI::ValueType::MoveSemantic:
                return convertTemplates(type.template_parameters) + "&&";
            case LangAPI::ValueType::Tuple:
                if (type.template_parameters.size() == 2) {
                    return std::string("std::pair<") + convertTemplates(type.template_parameters) + ">";
                } else {
                    return std::string("std::tuple<") + convertTemplates(type.template_parameters) + ">";
                }
            case LangAPI::ValueType::Char:
                return "char";
            case LangAPI::ValueType::Int:
                return "long long";
            case LangAPI::ValueType::LargeUInt:
                return "std::size_t";
            case LangAPI::ValueType::Float:
                return "double";
            case LangAPI::ValueType::Bool:
                return "bool";
            case LangAPI::ValueType::String:
                return "std::string";
            case LangAPI::ValueType::NonOwnedString:
                return "const char*";
            case LangAPI::ValueType::Array:
                return std::string("std::vector<") + convertTemplates(type.template_parameters) + ">";
            case LangAPI::ValueType::FixedSizeArray:
                return std::string("std::array<") + convertTemplates(type.template_parameters) + ">";
            case LangAPI::ValueType::Map:
                return std::string("std::unordered_map<") + convertTemplates(type.template_parameters) + ">";
            case LangAPI::ValueType::Token:
                return std::string("::ISPA_STD::Node<") + convertTemplates(type.template_parameters) + ">";
            case LangAPI::ValueType::Rule:
                return std::string("::ISPA_STD::Node<") + convertTemplates(type.template_parameters) + ">";
            case LangAPI::ValueType::RawToken:
                return std::string("::ISPA_STD::RawToken<") + convertTemplates(type.template_parameters) + ">";
            case LangAPI::ValueType::TokenResult:
                return std::string("::ISPA_STD::MatchResult<<") + convertTemplates(type.template_parameters) + ">";
            case LangAPI::ValueType::RuleResult:
                return std::string("::ISPA_STD::MatchResult<") + convertTemplates(type.template_parameters) + ">";
            case LangAPI::ValueType::Variant:
                return std::string("std::variant<std::monostate, ") + convertTemplates(type.template_parameters) + ">";
            case LangAPI::ValueType::LargeVariant:
                // large variant maps to regular variant yet
                return std::string("std::variant<std::monostate, ") + convertTemplates(type.template_parameters) + ">";
            case LangAPI::ValueType::Box:
                return std::string("std::shared_ptr<") + convertTemplates(type.template_parameters) + ">";
            case LangAPI::ValueType::Span:
                return "::ISPA_STD::Span<" + convertTemplates(type.template_parameters) + ">";
            case LangAPI::ValueType::TypeTag:
                return "std::type_identity<" + convertTemplates(type.template_parameters) + ">";
            case LangAPI::ValueType::Undef:
                return "auto";
            default:
                throw Error("Unknown type: {}", (int) type.getValueType());
        }
    } else if (type.isSymbol()) {
        auto name = convertSymbol(type.getSymbol());
        if (!type.template_parameters.empty())
            name += "<" + convertTemplates(type.template_parameters) + ">";
        return name;
    } else if (type.isIspaLibSymbol()) {
        return convertIspaLibSymbol(type.getIspaLibSymbol());
    } else throw Error("unknown variant type in Type::type");
}

auto Core::convertTemplates(const decltype(LangAPI::Type::template_parameters) &template_parameters) -> std::string {
    if (template_parameters.empty()) return "";
    std::string res;
    for (const auto &param : template_parameters) {
        if (std::holds_alternative<LangAPI::Type>(param)) {
            res += convertType(std::get<LangAPI::Type>(param));
        } else {
            res += convertRValue(std::get<LangAPI::RValue>(param));
        }
        res += ", ";
    }
    return res.substr(0, res.size() - 2);
}
auto Core::convertTemplates(const stdu::vector<std::variant<std::shared_ptr<LangAPI::Type>, std::shared_ptr<LangAPI::RValue>>> &template_parameters) -> std::string {
    if (template_parameters.empty()) return "";
    std::string res;
    for (const auto &param : template_parameters) {
        if (std::holds_alternative<std::shared_ptr<LangAPI::Type>>(param)) {
            res += convertType(*std::get<std::shared_ptr<LangAPI::Type>>(param));
        } else {
            res += convertRValue(*std::get<std::shared_ptr<LangAPI::RValue>>(param));
        }
        res += ", ";
    }
    return res.substr(0, res.size() - 2);
}

auto Core::convertSymbol(const LangAPI::Symbol &symbol) -> std::string {
    if (symbol.path.empty()) return "";
    if (symbol.path.size() == 1 && std::holds_alternative<std::string>(symbol.path.front()) &&
        std::get<std::string>(symbol.path.front()) == "__ispa_python_self") return "(*this)";
    std::string res;
    bool first = true;
    for (const auto &part : symbol.path) {
        std::string piece;
        if (std::holds_alternative<std::string>(part)) {
            piece = std::get<std::string>(part);
            if (first && piece == "__ispa_parser__") piece = name;
        } else if (std::holds_alternative<LangAPI::IspaLibSymbol>(part)) {
            piece = convertIspaLibSymbol(std::get<LangAPI::IspaLibSymbol>(part));
        } else {
            // Append function call as a symbol path segment (do not overwrite the accumulated path)
            piece = convertFunctionCall(std::get<LangAPI::FunctionCall>(part));
        }
        // Avoid "::::": lib symbols already start with "::", and an empty segment is only
        // meaningful as the first one (leading "::" = global scope).
        if (!first && piece.starts_with("::")) piece.erase(0, 2);
        if (!first && piece.empty()) continue;
        if (!first) res += "::";
        res += piece;
        first = false;
    }
    return res;
}
auto Core::convertStorageSymbol(const LangAPI::StorageSymbol &symbol) -> std::string {
    std::string res = convertExpression(symbol.what);
    for (const auto &part : symbol.path) {
        if (auto method = std::get_if<LangAPI::ArrayMethodCall>(&part);
            method && method->method >= LangAPI::ArrayMethods::PythonAppend) {
            std::string name;
            switch (method->method) {
                case LangAPI::ArrayMethods::PythonAppend: name = "append"; break;
                case LangAPI::ArrayMethods::PythonExtend: name = "extend"; break;
                case LangAPI::ArrayMethods::PythonInsert: name = "insert"; break;
                case LangAPI::ArrayMethods::PythonRemove: name = "remove"; break;
                case LangAPI::ArrayMethods::PythonPop: name = "pop"; break;
                case LangAPI::ArrayMethods::PythonClear: name = "clear"; break;
                case LangAPI::ArrayMethods::PythonCopy: name = "copy"; break;
                case LangAPI::ArrayMethods::PythonCount: name = "count"; break;
                case LangAPI::ArrayMethods::PythonIndex: name = "index"; break;
                case LangAPI::ArrayMethods::PythonReverse: name = "reverse"; break;
                case LangAPI::ArrayMethods::PythonSort: name = "sort"; break;
                case LangAPI::ArrayMethods::PythonGet: name = "get"; break;
                case LangAPI::ArrayMethods::PythonSetDefault: name = "setdefault"; break;
                case LangAPI::ArrayMethods::PythonUpdate: name = "update"; break;
                case LangAPI::ArrayMethods::PythonKeys: name = "keys"; break;
                case LangAPI::ArrayMethods::PythonValues: name = "values"; break;
                case LangAPI::ArrayMethods::PythonItems: name = "items"; break;
                case LangAPI::ArrayMethods::PythonFromKeys: name = "fromkeys"; break;
                default: throw Error("Unknown Python collection method");
            }
            if (method->method == LangAPI::ArrayMethods::PythonAppend) {
                res += ".push_back(" + convertExpression(method->args.front()) + ")";
                continue;
            }
            if (method->method == LangAPI::ArrayMethods::PythonClear) {
                res += ".clear()";
                continue;
            }
            std::string call = "ISPA_STD::python_" + name + "(" + res;
            for (const auto& arg : method->args) call += ", " + convertExpression(arg);
            res = call + ")";
            continue;
        }
        if (!std::holds_alternative<LangAPI::StorageOffset>(part))
            res += ".";
        if (std::holds_alternative<std::string>(part)) {
            res += std::get<std::string>(part);
        } else if (std::holds_alternative<LangAPI::ArrayMethodCall>(part)) {
            const auto &method_call = std::get<LangAPI::ArrayMethodCall>(part);
            switch (method_call.method) {
                case LangAPI::ArrayMethods::Push:
                    res += "push_back";
                    break;
                case LangAPI::ArrayMethods::Pop:
                    res += "pop_back";
                    break;
                case LangAPI::ArrayMethods::Size:
                    res += "size";
                    break;
                default:
                    throw Error("Unknown array method");
            }
            res += "(";
            bool first = true;
            for (const auto &param : method_call.args) {
                if (!first) res += ", ";
                res += convertExpression(param);
                first = false;
            }
            res += ")";
        } else if (std::holds_alternative<LangAPI::StorageOffset>(part)) {
            res += "[" + convertExpression(std::get<LangAPI::StorageOffset>(part).offset) + "]";
        } else if (std::holds_alternative<LangAPI::Property>(part)) {
            res += std::get<LangAPI::Property>(part).name + "()";
        } else if (std::holds_alternative<LangAPI::IspaLibSymbol>(part)) {
            res += convertIspaLibSymbol(std::get<LangAPI::IspaLibSymbol>(part));
        } else {
            // Append function call to the current storage symbol chain
            res += convertFunctionCall(std::get<LangAPI::FunctionCall>(part), true);
        }
    }
    return res;
}
auto Core::convertIspaLibSymbol(const LangAPI::IspaLibSymbol &symbol) -> std::string {
    switch (symbol.exports) {
        case LangAPI::StdlibExports::Node:
            return std::string("::ISPA_STD::Node<") + convertTemplates(symbol.template_parameters) + ">";
        case LangAPI::StdlibExports::MatchResult:
            return std::string("::ISPA_STD::MatchResult<") + convertTemplates(symbol.template_parameters) + ">";
        case LangAPI::StdlibExports::Lexer:
            return std::string("::ISPA_STD::Lexer_base") + (symbol.template_parameters.empty() ? "" : ("<" + convertTemplates(symbol.template_parameters) + ">"));
        case LangAPI::StdlibExports::LRGeneralizedParser:
            return std::string("::ISPA_STD::LRGeneralizedParser<") + convertTemplates(symbol.template_parameters) + ">";
        case LangAPI::StdlibExports::LRParseGeneralized: return "lr_parse_generalized";
        case LangAPI::StdlibExports::LRParser:
            return std::string("::ISPA_STD::LRParser<") + convertTemplates(symbol.template_parameters) + ">";
        case LangAPI::StdlibExports::LRGet: return "lr_get";
        case LangAPI::StdlibExports::LRToken: return "lr_token";
        case LangAPI::StdlibExports::LRPack: return "lr_pack";
        case LangAPI::StdlibExports::LRCaptureAssign: return "lr_capture_assign";
        case LangAPI::StdlibExports::LRNode: return "lr_node";
        case LangAPI::StdlibExports::LRParse:
            return "lr_parse";
        case LangAPI::StdlibExports::Parser:
            return std::string("::ISPA_STD::LLParser<") + convertTemplates(symbol.template_parameters) + ">";
        case LangAPI::StdlibExports::LexerMakeTokenParameter:
            return (symbol.Const ? "const " : "") + std::string("char*") + (symbol.Reference ? "&" : "");
        case LangAPI::StdlibExports::ParserFunctionParameter:
            return "Iterator";
        case LangAPI::StdlibExports::DfaState:
            return "::ISPA_STD::DFA::API::State<" + convertTemplates(symbol.template_parameters) + ">";
        case LangAPI::StdlibExports::DfaTable:
            return "::ISPA_STD::DFA::API::Table<" + convertTemplates(symbol.template_parameters) + ">";
        case LangAPI::StdlibExports::DfaAcceptTable:
            return "::ISPA_STD::DFA::API::AcceptTable<" + convertTemplates(symbol.template_parameters) + ">";
        case LangAPI::StdlibExports::DfaClassTable:
            return "::ISPA_STD::DFA::API::CharToClass";
        case LangAPI::StdlibExports::DfaNullState:
            return "::ISPA_STD::DFA::API::null_state";
        case LangAPI::StdlibExports::Error:
            return "std::runtime_error";
        case LangAPI::StdlibExports::TokenNodeConstruct:
            return "::ISPA_STD::Node<" + convertTemplates(symbol.template_parameters) + ">::create";
        case LangAPI::StdlibExports::RawTokenNodeConstruct:
            return "Token::create";
        case LangAPI::StdlibExports::ParserNodeConstructor:
            return "Token::create";
        case LangAPI::StdlibExports::DFADebug:
            return "::ISPA_STD::DFA::API::DFADebug";
        case LangAPI::StdlibExports::TdfaLayout:
            return "::ISPA_STD::DFA::API::TdfaLayout<" + convertTemplates(symbol.template_parameters) + ">";
        case LangAPI::StdlibExports::DfaCaptures:
            return (symbol.Const ? "const " : "") + std::string("::ISPA_STD::DFA::API::Captures") + (symbol.Reference ? "&" : "");
        case LangAPI::StdlibExports::DfaDynamicMatch:
            return "::ISPA_STD::DFA::API::dynamic_match";
        case LangAPI::StdlibExports::ASTPrinter:
            return "::ISPA_STD::ASTPrinter<std::ostream>";
        case LangAPI::StdlibExports::Concat:
            return "::ISPA_STD::concat";
        case LangAPI::StdlibExports::MatchResultValue:
            return "node";
        case LangAPI::StdlibExports::VariantIndex:
            return "index()";
        case LangAPI::StdlibExports::ParserRequired: return "required";
        case LangAPI::StdlibExports::ParserOptional: return "optional";
        case LangAPI::StdlibExports::ParserRange: return "range";
        case LangAPI::StdlibExports::ParserRangeOptional: return "range_optional";
        case LangAPI::StdlibExports::ParserCounted: return "counted";
        case LangAPI::StdlibExports::AssignCompatible:
            return "::ISPA_STD::assign_compatible";
        case LangAPI::StdlibExports::LexerTokenIterator:
            return scopePrefix() + "Lexer::iterator";
        case LangAPI::StdlibExports::LexerLazyIterator:
            return scopePrefix() + "Lexer::lazy_iterator";
        default:
            throw Error("Unknown IspaLibSymbol exports: {}", (int) symbol.exports);
    }
}

auto Core::convertExpression(const LangAPI::Expression &expression) -> std::string {
    std::ostringstream out;
    for (const auto &expr : expression) {
        switch (expr.type()) {
            case LangAPI::ExpressionValueType::Empty:
                break;
            case LangAPI::ExpressionValueType::RValue:
                out << convertRValue(expr.getRValue());
                break;
            case LangAPI::ExpressionValueType::EmptyInitializer:
                out << "{}";
                break;
            case LangAPI::ExpressionValueType::ExpressionElement:
                out << convertExpressionElement(expr.getExpressionElement());
                break;
            case LangAPI::ExpressionValueType::FunctionCall:
                out << convertFunctionCall(expr.getFunctionCall());
                break;
            case LangAPI::ExpressionValueType::StringCompare: {
                auto compare = expr.getStringCompare();
                out << "strncmp(" << counter.back() << ", " << (compare.is_string ? std::string("\"") + compare.str.value + "\""  : std::string("'") + compare.str.value + "'");
                break;
            }
            case LangAPI::ExpressionValueType::Return:
                out << "return " << convertExpression(expr.getReturn().value);
                break;
            case LangAPI::ExpressionValueType::Break:
                out << "break";
                break;
            case LangAPI::ExpressionValueType::Continue:
                out << "continue";
                break;
            case LangAPI::ExpressionValueType::VariableAssignment: {
                const auto &var = expr.getVariableAssignment();
                std::string type;
                switch (var.type) {
                    case LangAPI::OperatorType::Assign:
                        type = "=";
                        break;
                    case LangAPI::OperatorType::Add:
                        type = "+=";
                        break;
                    case LangAPI::OperatorType::Minus:
                        type = "-=";
                        break;
                    case LangAPI::OperatorType::Multiply:
                        type = "*=";
                        break;
                    case LangAPI::OperatorType::Divide:
                        type = "/=";
                        break;
                    case LangAPI::OperatorType::Modulo:
                        type = "%=";
                        break;
                }
                if (std::holds_alternative<LangAPI::Symbol>(var.name))
                    out << convertSymbol(std::get<LangAPI::Symbol>(var.name));
                else
                    out << convertStorageSymbol(std::get<LangAPI::StorageSymbol>(var.name));
                out << " " << type << " " << convertExpression(var.value);
                break;
            }
            case LangAPI::ExpressionValueType::CounterIncreament:
                out << std::string("++") + counter.back();
                break;
            case LangAPI::ExpressionValueType::CounterAssignment:
                out << counter.back() + " = " + convertExpression(*expr.getCounterAssignment().v);
                break;
            case LangAPI::ExpressionValueType::ResetPosCounter:
                counter.pop_back();
                break;
            case LangAPI::ExpressionValueType::PushPosCounter: {
                const auto &a = expr.getPushPosCounter();
                out << "auto " << a.name << " = " << counter.back();
                counter.push_back(a.name);;
                break;
            }
            case LangAPI::ExpressionValueType::PopPosCounter: {
                out << counter[counter.size() - 2] + " = " << counter.back();
                counter.pop_back();
                break;
            }
            case LangAPI::ExpressionValueType::SkipSpaces:
                out << "skip_spaces<Types::__WS>(" << counter.back() << ")";
                break;
            case LangAPI::ExpressionValueType::DfaLookup: {
                const auto &lookup = expr.getDfaLookup();
                out << lookup.output_name << " = ::ISPA_STD::DFA::decide(dfa_table_"  << std::to_string(lookup.dfa_count) << ", pos, &Parser::PANIC_MODE)";
                break;
            }
            case LangAPI::ExpressionValueType::ReportError:
                out << "raise_error(*pos, \"" << expr.getReportError().message << "\")";
                break;
            case LangAPI::ExpressionValueType::Lambda:
                out << convertLambda(expr.getLambda());
                break;
            case LangAPI::ExpressionValueType::IspaLibFunctionCall:
                out << convertIspaLibFunctionCall(expr.getIspaLibFunctionCall());
                break;
            case LangAPI::ExpressionValueType::DFADebug:
                out << convertDFADebug(expr.getDFADebug());
                break;
            default:
                throw Error("Unknown expression type");
        }
    }
    return out.str();
}
auto Core::convertExpressionElement(LangAPI::ExpressionElement element) -> const char * {
    switch (element) {
        case LangAPI::ExpressionElement::GroupOpen:
            return "(";
        case LangAPI::ExpressionElement::GroupClose:
            return ")";
        case LangAPI::ExpressionElement::SquareBraceOpen:
            return "[";
        case LangAPI::ExpressionElement::SquareBraceClose:
            return "]";
        case LangAPI::ExpressionElement::And:
            return " && ";
        case LangAPI::ExpressionElement::Or:
            return " || ";
        case LangAPI::ExpressionElement::Not:
            return "!";
        case LangAPI::ExpressionElement::Equal:
            return " == ";
        case LangAPI::ExpressionElement::NotEqual:
            return " != ";
        case LangAPI::ExpressionElement::Higher:
            return " > ";
        case LangAPI::ExpressionElement::Lower:
            return " < ";
        case LangAPI::ExpressionElement::HigherOrEqual:
            return " >= ";
        case LangAPI::ExpressionElement::LowerOrEqual:
            return " <= ";
        case LangAPI::ExpressionElement::Add:
            return " + ";
        case LangAPI::ExpressionElement::Minus:
            return " - ";
        case LangAPI::ExpressionElement::Multiply:
            return " * ";
        case LangAPI::ExpressionElement::Divide:
            return " / ";
        case LangAPI::ExpressionElement::Modulo:
            return " % ";
        case LangAPI::ExpressionElement::PlusPlus:
            return "++";
        case LangAPI::ExpressionElement::MinusMinus:
            return "--";
        default:
            throw Error("Unknown expression element");
    }
}
auto Core::convertLambda(const LangAPI::Lambda &lambda) -> std::string {
    std::string out = "[&](";
    for (std::size_t i = 0; i < lambda.parameters.size(); ++i) {
        if (i) out += ", ";
        out += convertType(lambda.parameters[i].first) + " " + lambda.parameters[i].second;
    }
    return out + ") {\n" + lambda_body(lambda.statements) + "}";
}
// auto Core::convertLambda(const LangAPI::Lambda &lambda) -> std::string {
//     // use lower level interaction to writer to output lambda in correct place with correct indentation
//     std::ostringstream out;
//     out << "[](";
//     bool first = true;
//     for (const auto &p : lambda.parameters) {
//         if (!first) {
//             out << ", ";
//         }
//         out << convertType(p.first) << ' ' << p.second;
//         first = false;
//     }
//     out << ") {\n";
//     auto prev_writer = stmts_converter->getWriter();
//     stmts_converter->getWriter().increaseIndentation();
//     buildStatements(lambda.statements);
//     Core::symbol_path.push_back("");
//     declarations_converter->closeFunction();
//     const auto text = stmts_converter->getWriter().get().erase(0, prev_writer.get().size());
//     out << text;
//     stmts_converter->getWriter() = prev_writer;
//     return out.str();
// }
auto Core::convertFunctionParams(const stdu::vector<LangAPI::Expression> &args) {
    std::string str;
    bool first = true;
    for (const auto &param : args) {
        if (!first) str += ", ";
        str += convertExpression(param);
        first = false;
    }
    return str;
}
auto Core::convertFunctionCall(const LangAPI::FunctionCall &call, bool need_template) -> std::string {
    std::string str ;
    if (need_template && !call.template_parameters.empty())
        str += "template ";
    if (std::holds_alternative<std::shared_ptr<LangAPI::Symbol>>(call.name)) {
        str += convertSymbol(*std::get<std::shared_ptr<LangAPI::Symbol>>(call.name));
    } else {
        str += convertIspaLibSymbol(*std::get<std::shared_ptr<LangAPI::IspaLibSymbol>>(call.name));
    }
    if (!call.template_parameters.empty()) {

        str += "<" + convertTemplates(call.template_parameters) + ">";
    }
    str += "(";
    str += convertFunctionParams(call.args);
    str += ")";
    return str;
}
auto Core::convertIspaLibFunctionCall(const LangAPI::IspaLibFunctionCall &call) -> std::string {
    std::ostringstream out;
    out << convertIspaLibSymbol(call.symbol);
    if (!call.template_parameters.empty())
        out << "<" << convertTemplates(call.template_parameters) << ">";
    out << "(";
    const auto kind = call.symbol.exports;
    const bool parser_match = kind == LangAPI::StdlibExports::ParserRequired ||
        kind == LangAPI::StdlibExports::ParserOptional ||
        kind == LangAPI::StdlibExports::ParserRange ||
        kind == LangAPI::StdlibExports::ParserRangeOptional;
    for (std::size_t i = 0; i < call.args.size(); ++i) {
        if (i) out << ", ";
        const auto& param = call.args[i];
        // Rule callbacks are member templates. Bind the parser instance and
        // let the iterator argument select the specialization at the call site.
        const bool counted_callback = kind == LangAPI::StdlibExports::ParserCounted && i == 2;
        if (((parser_match && i == 1 &&
            (call.args.size() == 3 || kind == LangAPI::StdlibExports::ParserOptional ||
             kind == LangAPI::StdlibExports::ParserRangeOptional)) || counted_callback) && param.size() == 1 &&
            param.front().isRvalue() && param.front().getRValue().isSymbol())
            out << "[this](auto cursor) { return this->"
                << convertExpression(param) << "(cursor); }";
        else
            out << convertExpression(param);
    }
    out << ")";
    return out.str();
}
auto Core::convertMakeTuple(const LangAPI::MakeTuple &make_tuple) -> std::string {
    std::string tuple = make_tuple.args.size() == 2 ? "std::make_pair(" : "std::make_tuple(";
    bool first = true;
    for (const auto &el : make_tuple.args) {
        tuple += convertExpression(el) + ", ";
    }
    tuple.pop_back();
    tuple.pop_back();
    tuple += ")";
    return tuple;
}
auto Core::convertGetVariant(const LangAPI::GetVariant &get_variant) -> std::string {
    if (get_variant.is_raw_token) {
        return "(" + convertExpression(get_variant.sym) + ").template to_node<" + convertType(*get_variant.type) + ">()";
    }
    return std::string("std::get<") + convertType(*get_variant.type) + ">(" + convertExpression(get_variant.sym) + ")";
}
auto Core::convertCheckVariant(const LangAPI::CheckVariant &check_variant) -> std::string {
    if (check_variant.is_raw_token) {
        return "(" + convertExpression(check_variant.sym) + ").template holds<" + convertType(*check_variant.type) + ">()";
    }
    return std::string("std::holds_alternative<") + convertType(*check_variant.type) + ">(" + convertExpression(check_variant.sym) + ")";
}
auto Core::convertDFADebug(const LangAPI::DFADebug& dfa_debug)
    -> std::string
{
    std::stringstream ss;

    // Escape arbitrary text for a generated C++ string literal.
    const auto escapeString = [](std::string_view value) {
        std::string result;
        result.reserve(value.size() * 2);

        constexpr char hex[] = "0123456789ABCDEF";

        for (unsigned char c : value) {
            switch (c) {
                case '\\': result += "\\\\"; break;
                case '"':  result += "\\\""; break;
                case '\n': result += "\\n";  break;
                case '\r': result += "\\r";  break;
                case '\t': result += "\\t";  break;
                case '\0': result += "\\000"; break;

                default:
                    if (c < 0x20 || c == 0x7F) {
                        result += "\\x";
                        result += hex[c >> 4];
                        result += hex[c & 0x0F];

                        // Prevent hexadecimal escape continuation.
                        result += "\"\"";
                    } else {
                        result += static_cast<char>(c);
                    }
                    break;
            }
        }

        return result;
    };

    // Escape a character for a generated C++ character literal.
    const auto escapeChar = [](char value) {
        switch (value) {
            case '\\': return std::string("\\\\");
            case '\'': return std::string("\\'");
            case '\n': return std::string("\\n");
            case '\r': return std::string("\\r");
            case '\t': return std::string("\\t");
            case '\0': return std::string("\\000");
            default: {
                const auto c = static_cast<unsigned char>(value);

                if (c < 0x20 || c == 0x7F) {
                    constexpr char hex[] = "0123456789ABCDEF";

                    std::string result = "\\x";
                    result += hex[c >> 4];
                    result += hex[c & 0x0F];
                    return result;
                }

                return std::string(1, value);
            }
        }
    };

    std::string member;

    if (dfa_debug.member.empty()) {
        member = "<Undefined>";
    } else if (dfa_debug.member.isString()) {
        member = dfa_debug.member.getString().value;
    } else if (dfa_debug.member.isCsequence()) {
        const auto& sequence =
            dfa_debug.member.getCsequence();

        member = "[";

        for (const auto c : sequence.characters) {
            member += c;
        }

        for (const auto c : sequence.escaped) {
            member += '\\';
            member += c;
        }

        for (const auto& [from, to] : sequence.diapasons) {
            member += from;
            member += '-';
            member += to;
        }

        member += "]";
    } else {
        std::stringstream member_stream;
        member_stream << dfa_debug.member;
        member = member_stream.str();
    }

    ss << "::ISPA_STD::DFA::API::DFADebug {"
       << '"' << escapeString(member) << '"'
       << ", \"" << escapeString(
            corelib::text::join(dfa_debug.token_name, ".")
          ) << '"'
       << ", " << (
            dfa_debug.position_in_token
          )
       << ", " << (
            dfa_debug.call == constants::NULL_STATE
                ? "-1"
                : std::to_string(dfa_debug.call)
          )
       << ", " << (
            dfa_debug.group == constants::NULL_STATE
                ? "-1"
                : std::to_string(dfa_debug.group)
          )
       << ", " << (
            dfa_debug.rule_run == constants::NULL_STATE
                ? "-1"
                : std::to_string(dfa_debug.rule_run)
          )
       << ", " << dfa_debug.offset
       << ", " << dfa_debug.length
       << ", '" << escapeChar(dfa_debug.ch) << "'"
       << "}";

    return ss.str();
}
auto Core::convertArrayValues(
    const auto& values,
    bool double_braces
) -> std::string {
    std::string res = double_braces ? "{{" : "{";

    bool first = true;
    for (const auto& el : values) {
        if (!first)
            res += ", ";

        res += convertExpression(el);
        first = false;
    }

    res += double_braces ? "}}" : "}";
    return res;
}
auto Core::convertRValue(const LangAPI::RValue &rvalue) -> std::string {
    switch (rvalue.type()) {
        case LangAPI::RValueType::Undef:
            throw Error("RValue type is undefined");
        case LangAPI::RValueType::Char:
            return "'" + (rvalue.getChar().escaped ? std::string("\\") + std::string(1, rvalue.getChar().value) : corelib::text::getEscapedAsStr(rvalue.getChar().value, false)) + "'";
        case LangAPI::RValueType::Int:
            return std::to_string(rvalue.getInt().value);
        case LangAPI::RValueType::Bool:
            return rvalue.getBool().value ? "true" : "false";
        case LangAPI::RValueType::Float:
            return std::to_string(rvalue.getFloat().value);
        case LangAPI::RValueType::String:
            return std::string("\"") + rvalue.getString().value + "\"";
        case LangAPI::RValueType::Array: {
            const auto& array = rvalue.getArray();

            std::string res;
            if (!array.template_parameters.empty()) {
                res += "std::vector<"
                    + convertTemplates(array.template_parameters)
                    + ">";
            }

            res += convertArrayValues(array.values, false);
            return res;
        }
        case LangAPI::RValueType::FixedSizeArray: {
            const auto &array = rvalue.getFixedSizeArray();

            std::string res;

            if (!array.template_parameters.empty()) {
                res += "std::array<"
                    + convertTemplates(array.template_parameters)
                    + ">";
            }
            inside_array++;
            res += convertArrayValues(array.values, inside_array == 1);
            inside_array--;
            return res;
        }
        case LangAPI::RValueType::Map: {
            const auto &map = rvalue.getMap();
            std::string res;

            // Explicit type prefix if templates are specified
            if (!map.template_parameters.empty()) {
                res += "std::unordered_map<" + convertTemplates(map.template_parameters) + "> ";
            }

            res += "{ ";

            for (std::size_t i = 0; i < map.keys.size(); ++i) {
                if (i > 0) {
                    res += ", ";
                }

                // Each map element pair MUST be enclosed in braces: { key, value }
                res += "{ ";

                // 1. Format Key
                if (std::holds_alternative<LangAPI::String>(map.keys[i])) {
                    res += "\"" + std::get<LangAPI::String>(map.keys[i]).value + "\"";
                } else if (std::holds_alternative<LangAPI::Int>(map.keys[i])) {
                    res += std::to_string(std::get<LangAPI::Int>(map.keys[i]).value);
                }

                res += ", ";

                // 2. Format Value (convertExpression already formats nested maps/types correctly)
                res += convertExpression(map.values[i]);

                res += " }";
            }

            res += " }";
            return res;
        }
        case LangAPI::RValueType::Pos: {
            const auto &counter_data = rvalue.getPos();
            std::string res;
            if (counter_data.dereference) res += "(*";
            if (counter_data.offset != 0) res += "(" + counter.back() + " + " + std::to_string(counter_data.offset) + ")";
            else                          res +=  counter.back();
            if (counter_data.dereference) res += ")";
            return res;
        }
        case LangAPI::RValueType::Symbol:
            return convertSymbol(rvalue.getSymbol());
        case LangAPI::RValueType::IspaLibSymbol:
            return convertIspaLibSymbol(rvalue.getIspaSymbol());
        case LangAPI::RValueType::StorageSymbol:
            return convertStorageSymbol(rvalue.getStorageSymbol());
        case LangAPI::RValueType::Inheritance: {
            const auto &inheritance = rvalue.getInheritance();
            std::string res;
            if (std::holds_alternative<LangAPI::Symbol>(inheritance.name)) {
                res += convertSymbol(std::get<LangAPI::Symbol>(inheritance.name));
            } else if (std::holds_alternative<LangAPI::IspaLibSymbol>(inheritance.name)) {
                res += convertIspaLibSymbol(std::get<LangAPI::IspaLibSymbol>(inheritance.name));
            } else {
                res += convertType(*std::get<std::shared_ptr<LangAPI::Type>>(inheritance.name));
            }
            res += '{';
            res += convertFunctionParams(inheritance.args);
            res += '}';
            return res;
        }
        case LangAPI::RValueType::IspaLibDfaTransition: {
            const auto &transition = rvalue.getIspaLibDfaTransition();
            auto number_or_null = [](std::size_t number) -> std::string {
                return number != std::numeric_limits<std::size_t>::max() ? std::to_string(number) : "ISPA_STD::DFA::API::null_state";
            };
            std::ostringstream out_content;
            out_content << convertIspaLibSymbol(transition.transition_type) << "{ ";
            if (std::holds_alternative<std::size_t>(transition.symbol)) {
                const auto sym = std::to_string(std::get<std::size_t>(transition.symbol));
                // When referring to another DFA table as a symbol, the expected key type depends on transition kind:
                // - CharTableTransition expects Span<const variant<SpanState<CharTransition>, CharEmptyState<TOKEN_T>>> → plain ::ISPA_STD::Span{...}
                // - MultiTableTransition expects DFA::API::SpanMultiTable<Tokens, Nodes...> → wrap span into SpanMultiTable
                if (transition.is_refferring_char_table) {
                    // Key type for CharTableTransition is SpanCharTable<Tokens, ReturnType>
                    out_content << "::ISPA_STD::DFA::API::SpanCharTable<Tokens, "
                                << convertTemplates(transition.transition_type.template_parameters)
                                << "> {dfa_table_" + sym + ".data(), dfa_table_" + sym + ".size()}";
                } else {
                    // Build SpanMultiTable type using transition.transition_type template parameters (the node types)
                    out_content << "::ISPA_STD::DFA::API::SpanMultiTable<Tokens, "
                                << convertTemplates(transition.transition_type.template_parameters)
                                << ">{ ::ISPA_STD::Span {dfa_table_" << sym << ".data(), dfa_table_" << sym << ".size()} }";
                }
                out_content << ", ";
            } else if (!std::holds_alternative<char>(transition.symbol)) {
                out_content << "Tokens::" << corelib::text::join(std::get<stdu::vector<std::string>>(transition.symbol), "_");
                out_content << ", ";
            }
            out_content << number_or_null(transition.next) << ", "
            << std::boolalpha << transition.new_cst_node << ", "
            << std::boolalpha << transition.new_member << ", "
            << std::boolalpha << transition.close_cst_node << ", "
            << number_or_null(transition.new_group) << ", "
            << number_or_null(transition.group_close) << ", "
            << number_or_null(transition.accept)
            << " }";
            return out_content.str();
        }
        case LangAPI::RValueType::IspaLibDfaSpanCharState: {
            const auto &state = rvalue.getIspaLibDfaSpanCharState();
            auto number_or_null = [](std::size_t number) -> std::string {
                return number != std::numeric_limits<std::size_t>::max() ? std::to_string(number) : "ISPA_STD::DFA::API::null_state";
            };
            std::ostringstream out_content;
            out_content << "::ISPA_STD::DFA::API::SpanCharState {" << number_or_null(state.else_goto) << ", " << number_or_null(state.else_goto_accept) << ", "
                        << "::ISPA_STD::Span<::ISPA_STD::DFA::API::CharTransition> {dfa_state_" << state.state_id << ".data(), dfa_state_" << state.state_id << ".size()}}";
            return out_content.str();
        }
        case LangAPI::RValueType::IspaLibDfaSpanMultiTableState: {
            const auto &state = rvalue.getIspaLibDfaSpanMultiTableState();
            auto number_or_null = [](std::size_t number) -> std::string {
                return number != std::numeric_limits<std::size_t>::max() ? std::to_string(number) : "ISPA_STD::DFA::API::null_state";
            };
            std::ostringstream out_content;
            // Build template argument list: <Tokens[, MultiTableTransition<Tokens, ...> ...]>
            std::string tmpl = "<Tokens";
            for (const auto &sym : state.mutli_table_transitions) {
                tmpl += ", " + convertIspaLibSymbol(sym);
            }
            tmpl += ">";
            out_content << "::ISPA_STD::DFA::API::SpanMultiTableState" << tmpl << " {" << number_or_null(state.else_goto) << ", " << number_or_null(state.else_goto_accept) << ", "
                        << "{dfa_state_" << state.state_id << ".data(), dfa_state_" << state.state_id << ".size()}}";
            return out_content.str();
        }
        // case LangAPI::RValueType::IspaLibDfaEmptyState: {
        //     const auto &state = rvalue.getIspaLibDfaEmptyState();
        //     return "{ Tokens::" + corelib::text::join(state.token_name, "_") + ", " + convertLambda(*state.construction_lambda) + " }";
        // }
        case LangAPI::RValueType::IspaLibDfaSpan: {
            std::ostringstream out_content;
            const auto &span = rvalue.getIspaLibDfaSpan();
            out_content << convertIspaLibSymbol(span.type) << "{" << convertSymbol(span.assing_name) << ".data(), " << convertSymbol(span.assing_name) << ".size()}";
            return out_content.str();
        }
        case LangAPI::RValueType::Reference:
            return "&" + convertRValue(*rvalue.getReference().value);
        case LangAPI::RValueType::Span:
            return "::ISPA_STD::Span<" + convertType(*rvalue.getSpan().type) + "> { " + convertSymbol(rvalue.getSpan().sym) + ".data(), " + convertSymbol(rvalue.getSpan().sym) + ".size() }";
        case LangAPI::RValueType::MakeTuple:
            return convertMakeTuple(rvalue.getMakeTuple());
        case LangAPI::RValueType::GetVariant:
            return convertGetVariant(rvalue.getVariantCast());
        case LangAPI::RValueType::CheckVariant:
            return convertCheckVariant(rvalue.CheckVariantCast());
        case LangAPI::RValueType::ToString:
            return "std::to_string(" + convertExpression(rvalue.getToString().what) + ")";
        case LangAPI::RValueType::CharToStringConstructor:
            return "std::string(1, " + convertExpression(rvalue.getCharToStringConstructor().what) + ")";
        case LangAPI::RValueType::Cast:
            return "static_cast<" + convertType(*rvalue.getCast().type) + ">(" + convertExpression(rvalue.getCast().what) + ")";
        case LangAPI::RValueType::GetFromBox:
            return "(*" + convertExpression(rvalue.getGetFromBox().what) + ")";
        default:
            throw Error("Unknown RValue type: {}", (int) rvalue.type());
    }
}
// ----------------------------------------------------------------------------
// 1. Symbol & StorageSymbol
// ----------------------------------------------------------------------------
auto Core::ensureNamespaced(const std::string &name, const LangAPI::Symbol &sym) -> LangAPI::Symbol {
    if (name.empty() || sym.path.empty()) {
        return sym;
    }

    LangAPI::Symbol result = sym;
    if (const auto *first_str = std::get_if<std::string>(&result.path.front())) {
        if (*first_str == name) {
            result.path.insert(result.path.begin(), Core::symbol_path->begin(), Core::symbol_path->end());
        }
    }
    return result;
}

auto Core::ensureNamespaced(const std::string &ns, const LangAPI::StorageSymbol &ssym) -> LangAPI::StorageSymbol {
    LangAPI::StorageSymbol result = ssym;
    result.what = ensureNamespaced(ns, result.what);

    for (auto &part : result.path) {
        if (auto *fc = std::get_if<LangAPI::FunctionCall>(&part)) {
            *fc = ensureNamespaced(ns, *fc);
        } else if (auto *offset = std::get_if<LangAPI::StorageOffset>(&part)) {
            offset->offset = ensureNamespaced(ns, offset->offset);
        }
    }
    return result;
}
auto Core::ensureNamespaced(const std::string &ns, const LangAPI::IspaLibSymbol &sym) -> const LangAPI::IspaLibSymbol& {
    return sym;
}
// ----------------------------------------------------------------------------
// 2. Types & Function Calls
// ----------------------------------------------------------------------------
auto Core::ensureNamespaced(const std::string &ns, const LangAPI::Type &type) -> LangAPI::Type {
    LangAPI::Type result = type;

    // Namespace underlying symbol if this Type wraps a LangAPI::Symbol
    if (result.isSymbol()) {
        result.type = ensureNamespaced(ns, result.getSymbol());
    }

    // Recursively process template parameters
    for (auto &param : result.template_parameters) {
        std::visit([&ns](auto &arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, LangAPI::Type>) {
                arg = ensureNamespaced(ns, arg);
            } else if constexpr (std::is_same_v<T, LangAPI::RValue>) {
                arg = ensureNamespaced(ns, arg);
            }
        }, param);
    }

    return result;
}

auto Core::ensureNamespaced(const std::string &ns, const LangAPI::FunctionCall &fc) -> LangAPI::FunctionCall {
    LangAPI::FunctionCall result = fc;
    std::visit([&](const auto &name) {
        result.name = std::make_shared<std::decay_t<decltype(*name)>>(ensureNamespaced(ns, *name));
    }, result.name);
    for (auto &arg : result.args) {
        arg = ensureNamespaced(ns, arg);
    }
    return result;
}

// ----------------------------------------------------------------------------
// 3. RValue
// ----------------------------------------------------------------------------
auto Core::ensureNamespaced(const std::string &ns, const LangAPI::RValue &rval) -> LangAPI::RValue {
    LangAPI::RValue result = rval;

    if (result.isSymbol()) {
        result.getSymbol() = ensureNamespaced(ns, result.getSymbol());
    } else if (result.isStorageSymbol()) {
        result.getStorageSymbol() = ensureNamespaced(ns, result.getStorageSymbol());
    } else if (result.isArray()) {
        for (auto &v : result.getArray().values) {
            v = ensureNamespaced(ns, v);
        }
    } else if (result.isMap()) {
        for (auto &v : result.getMap().values) {
            v = ensureNamespaced(ns, v);
        }
    } else if (result.isReference()) {
        if (result.getReference().value) {
            result.getReference().value = std::make_shared<LangAPI::RValue>(
                ensureNamespaced(ns, *result.getReference().value)
            );
        }
    } else if (result.isSpan()) {
        result.getSpan().sym = ensureNamespaced(ns, result.getSpan().sym);
    }

    return result;
}

// ----------------------------------------------------------------------------
// 4. ExpressionValue & Expression
// ----------------------------------------------------------------------------
auto Core::ensureNamespaced(const std::string &ns, const LangAPI::ExpressionValue &val) -> LangAPI::ExpressionValue {
    LangAPI::ExpressionValue result = val;

    if (result.isRvalue()) {
        result.getRValue() = ensureNamespaced(ns, result.getRValue());
    } else if (result.isFunctionCall()) {
        result.getFunctionCall() = ensureNamespaced(ns, result.getFunctionCall());
    } else if (result.isReturn()) {
        result.getReturn().value = ensureNamespaced(ns, result.getReturn().value);
    } else if (result.isVariableAssignment()) {
        auto &assign = result.getVariableAssignment();
        std::visit([&ns](auto &target) {
            target = ensureNamespaced(ns, target);
        }, assign.name);
        assign.value = ensureNamespaced(ns, assign.value);
    } else if (result.isLambda()) {
        auto &lam = result.getLambda();
        lam.statements = ensureNamespaced(ns, lam.statements);
    }

    return result;
}

auto Core::ensureNamespaced(const std::string &ns, const LangAPI::Expression &expr) -> LangAPI::Expression {
    LangAPI::Expression result;
    result.reserve(expr.size());

    for (const auto &val : expr) {
        result.push_back(ensureNamespaced(ns, val));
    }

    return result;
}

// ----------------------------------------------------------------------------
// 5. Statements & Control Flow
// ----------------------------------------------------------------------------
auto Core::ensureNamespaced(const std::string &ns, const LangAPI::Statement &stmt) -> LangAPI::Statement {
    LangAPI::Statement result = stmt;

    if (result.isExpression()) {
        result.getExpression() = ensureNamespaced(ns, result.getExpression());
    } else if (result.isIf()) {
        auto &if_stmt = result.getIf();
        if_stmt.expr = ensureNamespaced(ns, if_stmt.expr);
        if_stmt.stmt = ensureNamespaced(ns, if_stmt.stmt);
        if_stmt.else_stmt = ensureNamespaced(ns, if_stmt.else_stmt);
    } else if (result.isWhile() || result.isDoWhile()) {
        auto &cond = result.getWhileOrDoWhile();
        cond.expr = ensureNamespaced(ns, cond.expr);
        cond.stmt = ensureNamespaced(ns, cond.stmt);
    } else if (result.isSwitch()) {
        auto &sw = result.getSwitch();
        sw.expression = ensureNamespaced(ns, sw.expression);
        for (auto &[rval, stmts] : sw.cases) {
            rval = ensureNamespaced(ns, rval);
            stmts = ensureNamespaced(ns, stmts);
        }
    }

    return result;
}

auto Core::ensureNamespaced(const std::string &ns, const LangAPI::Statements &stmts) -> LangAPI::Statements {
    LangAPI::Statements result;
    result.reserve(stmts.size());

    for (const auto &s : stmts) {
        result.push_back(ensureNamespaced(ns, s));
    }

    return result;
}
