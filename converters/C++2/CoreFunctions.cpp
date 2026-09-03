module Cpp.CoreFunctions;

import corelib;
import logging;
import Cpp.Statement;
import Cpp.Declarations;

import dstd;

auto Core::convertType(const LangAPI::Type &type) -> std::string {
    if (type.isValueType()) {
        switch (type.getValueType()) {
            case LangAPI::ValueType::Void:
                return "void";
            case LangAPI::ValueType::Const:
                return "const " + convertTemplates(type.template_parameters);
            case LangAPI::ValueType::Reference:
                return convertTemplates(type.template_parameters) + "&";
            case LangAPI::ValueType::Tuple:
                if (type.template_parameters.size() == 2) {
                    return std::string("std::pair<") + convertTemplates(type.template_parameters) + ">";
                } else {
                    return std::string("std::tuple<") + convertTemplates(type.template_parameters) + ">";
                }
            case LangAPI::ValueType::Char:
                return "char";
            case LangAPI::ValueType::Int:
                return "int";
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
                return std::string("::ISPA_STD::Node<Tokens, ") + convertTemplates(type.template_parameters) + ">";
            case LangAPI::ValueType::Rule:
                return std::string("::ISPA_STD::Node<Rules, ") + convertTemplates(type.template_parameters) + ">";
            case LangAPI::ValueType::TokenResult:
                return std::string("::ISPA_STD::MatchResult<Tokens, ") + convertTemplates(type.template_parameters) + ">";
            case LangAPI::ValueType::RuleResult:
                return std::string("::ISPA_STD::MatchResult<Rules, ") + convertTemplates(type.template_parameters) + ">";
            case LangAPI::ValueType::Variant:
                return std::string("std::variant<std::monostate, ") + convertTemplates(type.template_parameters) + ">";
            case LangAPI::ValueType::Box:
                return std::string("std::unique_ptr<") + convertTemplates(type.template_parameters) + ">";
            case LangAPI::ValueType::Span:
                return "::ISPA_STD::Span<" + convertTemplates(type.template_parameters) + ">";
            case LangAPI::ValueType::Undef:
                return "Undef";
                // throw Error("Type is undefined");
            default:
                throw Error("Unknown type");
        }
    } else if (type.isSymbol()) {
        auto sym = convertSymbol(type.getSymbol());
        if (sym == "std::any") return "Tokens";
        return sym;
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
    std::string res;
    for (const auto &part : symbol.path) {
        if (std::holds_alternative<std::string>(part)) {
            res += std::get<std::string>(part);
        } else if (std::holds_alternative<LangAPI::IspaLibSymbol>(part)) {
            res += convertIspaLibSymbol(std::get<LangAPI::IspaLibSymbol>(part));
        } else {
            // Append function call as a symbol path segment (do not overwrite the accumulated path)
            res += convertFunctionCall(std::get<LangAPI::FunctionCall>(part));
        }
        res += "::";
    }
    res.pop_back();
    res.pop_back();
    return res;
}
auto Core::convertStorageSymbol(const LangAPI::StorageSymbol &symbol) -> std::string {
    std::string res = convertExpression(symbol.what);
    for (const auto &part : symbol.path) {
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
        case LangAPI::StdlibExports::Lexer:
            return std::string("::ISPA_STD::Lexer_base") + (symbol.template_parameters.empty() ? "" : ("<Tokens, " + convertTemplates(symbol.template_parameters) + ">"));
        case LangAPI::StdlibExports::Parser:
            return std::string("::ISPA_STD::LLParser_base<Tokens, Rules") + (symbol.template_parameters.empty() ? "" : ", " + convertTemplates(symbol.template_parameters)) + ">";
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
        case LangAPI::StdlibExports::ActionUNDEF:
            return "::ISPA_STD::DFA::API::Action::UNDEF";
        case LangAPI::StdlibExports::ActionBEGIN:
            return "::ISPA_STD::DFA::API::Action::BEGIN";
        case LangAPI::StdlibExports::ActionEND:
            return "::ISPA_STD::DFA::API::Action::BEGIN";
        case LangAPI::StdlibExports::ActionPUSH:
            return "::ISPA_STD::DFA::API::Action::BEGIN";
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
            case LangAPI::ExpressionValueType::CounterIncreamentByLength:
                out << counter.back() + " += " + expr.getCounterIncreamentByLength().name + ".token.length()";
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
                out << "skip_spaces(" << counter.back() << ")";
                break;
            case LangAPI::ExpressionValueType::DfaLookup: {
                const auto &lookup = expr.getDfaLookup();
                out << lookup.output_name << " = ::ISPA_STD::DFA::decide(dfa_table_"  << std::to_string(lookup.dfa_count) << ", pos, &Parser::PANIC_MODE)";
                break;
            }
            case LangAPI::ExpressionValueType::ReportError:
                out << "[Error]";
                break;
            case LangAPI::ExpressionValueType::Lambda:
                out << convertLambda(expr.getLambda());
                break;
            case LangAPI::ExpressionValueType::IspaLibFunctionCall:
                out << convertIspaLibFunctionCall(expr.getIspaLibFunctionCall());
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
auto Core::buildStatement(const LangAPI::Statement &stmt) -> void {
    if (stmt.isIf()) {
        stmts_converter->createIf(stmt.getIf().expr);
        buildStatements(stmt.getIf().stmt);
        stmts_converter->closeIf();
    } else if (stmt.isWhile()) {
        stmts_converter->createWhile(stmt.getWhile().expr);
        buildStatements(stmt.getWhile().stmt);
        stmts_converter->closeWhile();
    } else if (stmt.isDoWhile()) {
        const auto dowhile = stmt.getDoWhile();
        stmts_converter->openDoWhile();
        buildStatements(dowhile.stmt);
        stmts_converter->closeDoWhile(dowhile.expr);
    } else if (stmt.isSwitch()) {
        const auto switch_statement = stmt.getSwitch();
        stmts_converter->createSwitch(switch_statement.expression);
        for (const auto &case_statement : switch_statement.cases) {
            stmts_converter->createCase(case_statement.first);
            buildStatements(case_statement.second);
            stmts_converter->closeCase();
        }
        stmts_converter->closeSwitch();
    } else if (stmt.isExpression()) {
        stmts_converter->createExpression(stmt.getExpression());
    } else if (stmt.isVariable()) {
        stmts_converter->createVariable(stmt.getVariable());
    }
}
auto Core::buildStatements(const LangAPI::Statements &statements) -> void {
    for (const auto &stmt : statements) {
        buildStatement(stmt);
    }
}
auto Core::convertLambda(const LangAPI::Lambda &lambda) -> std::string {
    // use lower level interaction to writer to output lambda in correct place with correct indentation
    std::ostringstream out;
    out << "[](";
    bool first = true;
    for (const auto &p : lambda.parameters) {
        if (!first) {
            out << ", ";
        }
        out << convertType(p.first) << ' ' << p.second;
        first = false;
    }
    out << ") {\n";
    auto prev_writer = stmts_converter->getWriter();
    stmts_converter->getWriter().increaseIndentation();
    buildStatements(lambda.statements);
    Core::symbol_path.push_back("");
    declarations_converter->closeFunction();
    const auto text = stmts_converter->getWriter().get().erase(0, prev_writer.get().size());
    out << text;
    stmts_converter->getWriter() = prev_writer;
    return out.str();
}
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
    out << convertIspaLibSymbol(call.symbol) << "(";
    bool first = true;
    for (const auto &param : call.args) {
        if (!first) out << ", ";
        out << convertExpression(param);
        first = false;
    }
    out << ")";
    return out.str();
}
auto Core::convertMakeTuple(const LangAPI::MakeTuple &make_tuple) -> std::string {
    std::string tuple = make_tuple.args.size() == 2 ? "std::make_pair(" : "std::make_tuple(";
    bool first = false;
    for (const auto &el : make_tuple.args) {
        tuple += convertExpression(el);
        if (!first) {
            tuple += ", ";
            first = true;
        }
    }
    tuple += ")";
    return tuple;
}
auto Core::convertGetVariant(const LangAPI::GetVariant &get_variant) -> std::string {
    std::cout << "ss: " << get_variant.sym << ", " << convertExpression(get_variant.sym) << std::endl;
    return std::string("std::get<") + convertType(*get_variant.type) + ">(" + convertExpression(get_variant.sym) + ")";
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
            const auto &array = rvalue.getArray();
            std::string res;
            if (!array.template_parameters.empty()) {
                res += "std::vector<" + convertTemplates(rvalue.getArray().template_parameters) + "> ";
            }
            if (array.values.empty()) {
                // For empty arrays (including std::array with N=0), emit a simple empty initializer
                res += "{}";
            } else {
                res += "{{";
                for (const auto &el : rvalue.getArray().values) {
                    res += convertExpression(el) + ", ";
                }
                // Remove the trailing comma+space and close braces
                res.pop_back();
                res.pop_back();
                res += "}}";
            }
            return res;
        }
        case LangAPI::RValueType::FixedSizeArray: {
            const auto &array = rvalue.getFixedSizeArray();
            std::string res;
            if (!array.template_parameters.empty()) {
                res += std::string("std::array<") + convertTemplates(array.template_parameters) + "> ";
            }
            if (array.values.empty()) {
                // For empty std::array initializers print just {}
                res += "{}";
            } else {
                res += ";";
                res += "{{";
                for (const auto &el : array.values) {
                    res += convertExpression(el) + ", ";
                }
                // Remove the trailing comma+space and close braces
                res.pop_back();
                res.pop_back();
                res += "}}";
            }
            return res;
        }
        case LangAPI::RValueType::Map: {
            const auto &map = rvalue.getMap();
            std::string res;
            if (!map.template_parameters.empty()) {
                res += "std::unordered_map<" + convertTemplates(rvalue.getMap().template_parameters) + "> ";
            }
            res +=  "{";
            for (std::size_t i = 0; i < rvalue.getMap().keys.size(); i++) {
                if (std::holds_alternative<LangAPI::String>(rvalue.getMap().keys[i])) {
                    res += std::string("\"") + std::get<LangAPI::String>(rvalue.getMap().keys[i]).value + "\", ";
                } else {
                    res += std::to_string(std::get<LangAPI::Int>(rvalue.getMap().keys[i]).value) + ", ";
                }
                res += "{ " + convertExpression(rvalue.getMap().values[i]) + "}, " ;
            }
            res.pop_back();
            res.pop_back();
            res += " }";
            return res;
        }
        case LangAPI::RValueType::Pos: {
            const auto &counter_data = rvalue.getPos();
            std::string res;
            if (counter_data.dereference) res += "*";
            if (counter_data.offset != 0) res += "(" + counter.back() + " + " + std::to_string(counter_data.offset) + ")";
            else                          res += counter.back();
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
            } else {
                res += convertIspaLibSymbol(std::get<LangAPI::IspaLibSymbol>(inheritance.name));
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
        case LangAPI::RValueType::IspaLibDfaEmptyState: {
            const auto &state = rvalue.getIspaLibDfaEmptyState();
            return "{ Tokens::" + corelib::text::join(state.token_name, "_") + ", " + convertLambda(*state.construction_lambda) + " }";
        }
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
            return "std::holds_alternative<" + convertType(*rvalue.CheckVariantCast().type) + ">(" + convertExpression(rvalue.CheckVariantCast().sym) + ")";
        case LangAPI::RValueType::CharToStringConstructor:
            return "std::string(1, " + convertExpression(rvalue.getCharToStringConstructor().what) + ")";
        default:
            throw Error("Unknown RValue type: {}", (int) rvalue.type());
    }
}
auto Core::flushInitContent() -> void {
    if (init_content.str().empty()) {
        return;
    }
    cpp_file.write("void {}::init() {\n{}\n}", corelib::text::join(symbol_path, "::"), init_content.str());
    init_content.clear();
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
            result.path.insert(result.path.begin(), Core::symbol_path.begin(), Core::symbol_path.end());
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