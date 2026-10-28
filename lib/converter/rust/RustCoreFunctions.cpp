module Rust.CoreFunctions;

import corelib;
import logging;
import AST.API;
import constants;
import dstd;
import std;

// ============================================================================
// helpers
// ============================================================================
namespace {
    auto joinStrings(const std::vector<std::string> &items, std::string_view sep) -> std::string {
        std::string out;
        for (std::size_t i = 0; i < items.size(); ++i) {
            if (i) out += sep;
            out += items[i];
        }
        return out;
    }
    auto isIdentChar(char c) -> bool {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
    }
    auto isValueTypeOf(const LangAPI::Type &t, LangAPI::ValueType vt) -> bool {
        return t.isValueType() && t.getValueType() == vt;
    }
    // `<A, B>` in type context, `::<A, B>` in expression context
    auto generic(const std::string &args, bool expr) -> std::string {
        if (args.empty()) return "";
        return std::string(expr ? "::<" : "<") + args + ">";
    }
    auto templateParts(const decltype(LangAPI::Type::template_parameters) &params) -> std::vector<std::string> {
        std::vector<std::string> parts;
        for (const auto &param : params) {
            if (std::holds_alternative<LangAPI::Type>(param)) parts.push_back(RustCore::convertType(std::get<LangAPI::Type>(param)));
            else parts.push_back(RustCore::convertRValue(std::get<LangAPI::RValue>(param)));
        }
        return parts;
    }
    auto hex2(unsigned char c) -> std::string {
        constexpr char digits[] = "0123456789abcdef";
        std::string out = "\\x";
        out += digits[c >> 4];
        out += digits[c & 0x0F];
        return out;
    }
    auto numberOrNull(std::size_t number) -> std::string {
        return number != std::numeric_limits<std::size_t>::max() ? std::to_string(number) : "::ispa_std::dfa::api::NULL_STATE";
    }
    auto asExpr(std::string_view text) -> std::string { return "(" + std::string(text) + ")"; }
}

auto RustCore::warn(std::string message) -> void {
    if (std::find(diagnostics.begin(), diagnostics.end(), message) == diagnostics.end())
        diagnostics.push_back(std::move(message));
}
auto RustCore::crateRoot() -> std::string {
    return name.empty() ? std::string("crate") : "crate::" + name;
}

// ============================================================================
// identifiers and literals
// ============================================================================
auto RustCore::ident(const std::string &id) -> std::string {
    static const std::unordered_map<std::string, std::string> operators{
        {"operator<<", "operator_shl"}, {"operator>>", "operator_shr"}, {"operator<", "operator_lt"},
        {"operator>", "operator_gt"},   {"operator==", "operator_eq"},  {"operator!=", "operator_ne"},
        {"operator<=", "operator_le"},  {"operator>=", "operator_ge"},  {"operator()", "operator_call"},
        {"operator[]", "operator_index"}, {"operator+", "operator_add"}, {"operator-", "operator_sub"},
        {"operator*", "operator_mul"},  {"operator/", "operator_div"},  {"operator%", "operator_rem"},
        {"operator=", "operator_assign"}};
    static const std::unordered_set<std::string> keywords{
        "as", "break", "const", "continue", "crate", "else", "enum", "extern", "false", "fn", "for", "if", "impl",
        "in", "let", "loop", "match", "mod", "move", "mut", "pub", "ref", "return", "self", "Self", "static",
        "struct", "super", "trait", "true", "type", "unsafe", "use", "where", "while", "async", "await", "dyn",
        "abstract", "become", "box", "do", "final", "macro", "override", "priv", "typeof", "unsized", "virtual",
        "yield", "try", "gen"};
    if (id.empty()) return id;
    if (const auto it = operators.find(id); it != operators.end()) return it->second;
    // The IR sometimes smuggles raw text through a Symbol (`std::ostream &`); anything that is not a plain identifier
    // is passed through untouched.
    bool plain = !(id[0] >= '0' && id[0] <= '9');
    for (const char c : id) plain = plain && isIdentChar(c);
    if (!plain) return id;
    if (keywords.contains(id)) {
        if (id == "crate" || id == "self" || id == "Self" || id == "super") return id + "_";   // cannot be raw identifiers
        return "r#" + id;
    }
    return id;
}

// The IR stores string literals as C++ source text (it is copied between quotes by the C++ backend), so the body is
// translated escape by escape. Rust rejects \a \b \f \v \? and octal escapes, and \x only goes up to 0x7F.
auto RustCore::rustStringBody(std::string_view raw) -> std::string {
    std::string out;
    for (std::size_t i = 0; i < raw.size(); ++i) {
        const char c = raw[i];
        if (c != '\\') {
            if (c == '"') out += "\\\"";
            else if (c == '\n') out += "\\n";
            else if (c == '\r') out += "\\r";
            else if (c == '\t') out += "\\t";
            else out += c;
            continue;
        }
        if (++i >= raw.size()) throw Error("Rust Converter: dangling backslash in string literal");
        const char e = raw[i];
        switch (e) {
            case 'n': case 'r': case 't': case '\\': case '"': case '\'': out += '\\'; out += e; break;
            case 'a': out += "\\x07"; break;
            case 'b': out += "\\x08"; break;
            case 'f': out += "\\x0c"; break;
            case 'v': out += "\\x0b"; break;
            case '?': out += '?'; break;
            case 'x': {
                unsigned value = 0;
                std::size_t digits = 0;
                while (i + 1 < raw.size() && std::isxdigit(static_cast<unsigned char>(raw[i + 1]))) {
                    value = value * 16 + (std::isdigit(static_cast<unsigned char>(raw[i + 1])) ? raw[i + 1] - '0' : (std::tolower(raw[i + 1]) - 'a' + 10));
                    ++i; ++digits;
                }
                if (!digits || value > 0x7F) throw Error("Rust Converter: \\x escape above 0x7F cannot be written in a Rust string literal");
                out += hex2(static_cast<unsigned char>(value));
                break;
            }
            case 'u': case 'U': {
                const std::size_t width = e == 'u' ? 4 : 8;
                if (i + width >= raw.size()) throw Error("Rust Converter: malformed \\u escape");
                out += "\\u{" + std::string(raw.substr(i + 1, width)) + "}";
                i += width;
                break;
            }
            default:
                if (e >= '0' && e <= '7') {
                    unsigned value = e - '0';
                    for (int k = 0; k < 2 && i + 1 < raw.size() && raw[i + 1] >= '0' && raw[i + 1] <= '7'; ++k)
                        value = value * 8 + (raw[++i] - '0');
                    if (value > 0x7F) throw Error("Rust Converter: octal escape above 0x7F cannot be written in a Rust string literal");
                    out += value == 0 ? std::string("\\0") : hex2(static_cast<unsigned char>(value));
                    break;
                }
                throw Error("Rust Converter: unsupported escape sequence \\{}", e);
        }
    }
    return out;
}

// Characters are bytes: the parser walks a `const char*`, so `char` maps to u8 and 'x' to b'x'.
auto RustCore::rustCharLiteral(char value, bool escaped) -> std::string {
    if (escaped) {
        switch (value) {
            case 'n': return "b'\\n'";
            case 'r': return "b'\\r'";
            case 't': return "b'\\t'";
            case '0': return "b'\\0'";
            case '\\': return "b'\\\\'";
            case '\'': return "b'\\''";
            case '"': return "b'\"'";
            case 'a': return "b'\\x07'";
            case 'b': return "b'\\x08'";
            case 'f': return "b'\\x0c'";
            case 'v': return "b'\\x0b'";
            case '?': return "b'?'";
            default: throw Error("Rust Converter: unsupported escaped char \\{}", value);
        }
    }
    const auto c = static_cast<unsigned char>(value);
    if (value == '\\') return "b'\\\\'";
    if (value == '\'') return "b'\\''";
    if (value == '\n') return "b'\\n'";
    if (value == '\r') return "b'\\r'";
    if (value == '\t') return "b'\\t'";
    if (value == '\0') return "b'\\0'";
    if (c < 0x20 || c >= 0x7F) return "b'" + hex2(c) + "'";
    return std::string("b'") + value + "'";
}

auto RustCore::variantEnum(const std::vector<std::string> &alternatives) -> std::string {
    std::vector<std::string> unique;
    for (const auto &a : alternatives)
        if (std::find(unique.begin(), unique.end(), a) == unique.end()) unique.push_back(a);
    for (const auto &def : variants)
        if (def.alternatives == unique) return crateRoot() + "::" + def.name;
    VariantDef def{"Variant_" + std::to_string(variants.size() + 1), std::move(unique)};
    const auto path = crateRoot() + "::" + def.name;
    variants.push_back(std::move(def));
    return path;
}

// ============================================================================
// types
// ============================================================================
auto RustCore::convertTemplates(const decltype(LangAPI::Type::template_parameters) &template_parameters) -> std::string {
    return joinStrings(templateParts(template_parameters), ", ");
}
auto RustCore::convertTemplates(const stdu::vector<std::variant<std::shared_ptr<LangAPI::Type>, std::shared_ptr<LangAPI::RValue>>> &template_parameters) -> std::string {
    std::vector<std::string> parts;
    for (const auto &param : template_parameters) {
        if (std::holds_alternative<std::shared_ptr<LangAPI::Type>>(param)) parts.push_back(convertType(*std::get<std::shared_ptr<LangAPI::Type>>(param)));
        else parts.push_back(convertRValue(*std::get<std::shared_ptr<LangAPI::RValue>>(param)));
    }
    return joinStrings(parts, ", ");
}

auto RustCore::convertType(const LangAPI::Type &type) -> std::string {
    if (type.isValueType()) {
        const auto parts = templateParts(type.template_parameters);
        const auto joined = joinStrings(parts, ", ");
        switch (type.getValueType()) {
            case LangAPI::ValueType::Void:
                return "()";
            // `const T` is the default binding in Rust, the immutability lives on the binding, not the type.
            case LangAPI::ValueType::ImmutableSemantic:
                return joined;
            // `const T&` is Borrow(Immutable(T)) -> &T ; `T&` is Borrow(T) -> &mut T
            case LangAPI::ValueType::BorrowSemantic: {
                if (type.template_parameters.size() == 1 && std::holds_alternative<LangAPI::Type>(type.template_parameters.front())) {
                    const auto &inner = std::get<LangAPI::Type>(type.template_parameters.front());
                    if (isValueTypeOf(inner, LangAPI::ValueType::ImmutableSemantic))
                        return "&" + convertTemplates(inner.template_parameters);
                }
                return "&mut " + joined;
            }
            case LangAPI::ValueType::MoveSemantic:
                return joined;
            case LangAPI::ValueType::Tuple:
                if (parts.size() == 1) return "(" + joined + ",)";
                return "(" + joined + ")";
            case LangAPI::ValueType::Char:
                return "u8";
            case LangAPI::ValueType::Int:
                return "i64";
            case LangAPI::ValueType::LargeUInt:
                return "usize";
            case LangAPI::ValueType::Float:
                return "f64";
            case LangAPI::ValueType::Bool:
                return "bool";
            case LangAPI::ValueType::String:
                return "String";
            case LangAPI::ValueType::NonOwnedString:       // `const char*`: a Copy cursor into the input buffer
                return "::ispa_std::Ptr";
            case LangAPI::ValueType::Array:
                return "Vec<" + joined + ">";
            case LangAPI::ValueType::FixedSizeArray:
                if (parts.size() != 2) throw Error("Rust Converter: FixedSizeArray needs <T, N>");
                return "[" + parts[0] + "; " + parts[1] + "]";
            case LangAPI::ValueType::Map:
                return "::std::collections::HashMap<" + joined + ">";
            case LangAPI::ValueType::Token:
            case LangAPI::ValueType::Rule:
                return "::ispa_std::Node<" + joined + ">";
            case LangAPI::ValueType::RawToken:
                return "::ispa_std::RawToken<" + variantEnum(parts) + ">";
            case LangAPI::ValueType::TokenResult:          // the C++ backend prints `MatchResult<<` here, which looks like a typo
            case LangAPI::ValueType::RuleResult:
                return "::ispa_std::MatchResult<" + joined + ">";
            case LangAPI::ValueType::Variant:
            case LangAPI::ValueType::LargeVariant:
                return variantEnum(parts);
            case LangAPI::ValueType::Box:
                return "::std::rc::Rc<" + joined + ">";
            case LangAPI::ValueType::Span:
                return "::ispa_std::Span<" + joined + ">";
            case LangAPI::ValueType::TypeTag:
                return "::std::marker::PhantomData<" + joined + ">";
            case LangAPI::ValueType::Undef:
                return "_";
            default:
                throw Error("Unknown type: {}", (int) type.getValueType());
        }
    } else if (type.isSymbol()) {
        if (type.getSymbol().path.size() == 1 &&
            std::holds_alternative<std::string>(type.getSymbol().path.front()) &&
            (std::get<std::string>(type.getSymbol().path.front()) == "auto&" ||
             std::get<std::string>(type.getSymbol().path.front()) == "auto"))
            return "_";
        if (type.getSymbol().path.size() == 1 &&
            std::holds_alternative<std::string>(type.getSymbol().path.front()) &&
            std::get<std::string>(type.getSymbol().path.front()) == "std::ostream&")
            return "&mut dyn ::std::fmt::Write";
        auto text = convertSymbol(type.getSymbol());
        if (!type.template_parameters.empty())
            text += "<" + convertTemplates(type.template_parameters) + ">";
        return text;
    } else if (type.isIspaLibSymbol()) {
        return convertIspaLibSymbol(type.getIspaLibSymbol(), false);
    } else throw Error("unknown variant type in Type::type");
}

// Symbols: `a::b::c` stays a path, but a path that starts with self/this is a field / method chain and uses `.`.
auto RustCore::convertSymbol(const LangAPI::Symbol &symbol) -> std::string {
    if (symbol.path.empty()) return "";
    if (symbol.path.size() == 1 && std::holds_alternative<std::string>(symbol.path.front()) &&
        std::get<std::string>(symbol.path.front()) == "__ispa_python_self") return "self";
    std::string res;
    bool first = true;
    bool dotted = false;
    for (const auto &part : symbol.path) {
        std::string piece;
        if (std::holds_alternative<std::string>(part)) {
            piece = std::get<std::string>(part);
            if (first && (piece == "__ispa_parser__" || piece == "::" + name)) piece = crateRoot();
            else if (first && (piece == "self" || piece == "this" || piece == "Self")) dotted = piece != "Self";
            else piece = ident(piece);
        } else if (std::holds_alternative<LangAPI::IspaLibSymbol>(part)) {
            piece = convertIspaLibSymbol(std::get<LangAPI::IspaLibSymbol>(part), true);
        } else {
            piece = convertFunctionCall(std::get<LangAPI::FunctionCall>(part));
        }
        // lib symbols already start with "::"; an empty segment only means something as the first one (leading "::")
        if (!first && piece.starts_with("::")) piece.erase(0, 2);
        if (!first && piece.empty()) continue;
        if (!first) res += dotted ? "." : "::";
        res += piece;
        first = false;
    }
    return res;
}

auto RustCore::convertStorageSymbol(const LangAPI::StorageSymbol &symbol) -> std::string {
    std::string res = convertExpression(symbol.what);
    for (const auto &part : symbol.path) {
        if (auto method = std::get_if<LangAPI::ArrayMethodCall>(&part);
            method && method->method >= LangAPI::ArrayMethods::PythonAppend) {
            std::string method_name;
            switch (method->method) {
                case LangAPI::ArrayMethods::PythonAppend: method_name = "append"; break;
                case LangAPI::ArrayMethods::PythonExtend: method_name = "extend"; break;
                case LangAPI::ArrayMethods::PythonInsert: method_name = "insert"; break;
                case LangAPI::ArrayMethods::PythonRemove: method_name = "remove"; break;
                case LangAPI::ArrayMethods::PythonPop: method_name = "pop"; break;
                case LangAPI::ArrayMethods::PythonClear: method_name = "clear"; break;
                case LangAPI::ArrayMethods::PythonCopy: method_name = "copy"; break;
                case LangAPI::ArrayMethods::PythonCount: method_name = "count"; break;
                case LangAPI::ArrayMethods::PythonIndex: method_name = "index"; break;
                case LangAPI::ArrayMethods::PythonReverse: method_name = "reverse"; break;
                case LangAPI::ArrayMethods::PythonSort: method_name = "sort"; break;
                case LangAPI::ArrayMethods::PythonGet: method_name = "get"; break;
                case LangAPI::ArrayMethods::PythonSetDefault: method_name = "setdefault"; break;
                case LangAPI::ArrayMethods::PythonUpdate: method_name = "update"; break;
                case LangAPI::ArrayMethods::PythonKeys: method_name = "keys"; break;
                case LangAPI::ArrayMethods::PythonValues: method_name = "values"; break;
                case LangAPI::ArrayMethods::PythonItems: method_name = "items"; break;
                case LangAPI::ArrayMethods::PythonFromKeys: method_name = "fromkeys"; break;
                default: throw Error("Unknown Python collection method");
            }
            if (method->method == LangAPI::ArrayMethods::PythonAppend) {
                res += ".push(" + convertExpression(method->args.front()) + ")";
                continue;
            }
            if (method->method == LangAPI::ArrayMethods::PythonClear) {
                res += ".clear()";
                continue;
            }
            std::string call = "::ispa_std::python_" + method_name + "(&mut " + res;
            for (const auto &arg : method->args) call += ", " + convertExpression(arg);
            res = call + ")";
            continue;
        }
        if (std::holds_alternative<LangAPI::StorageOffset>(part)) {
            // Vec / array indexing needs usize, the IR has no type information to decide whether a cast is redundant
            res += "[(" + convertExpression(std::get<LangAPI::StorageOffset>(part).offset) + ") as usize]";
            continue;
        }
        res += ".";
        if (std::holds_alternative<std::string>(part)) {
            res += ident(std::get<std::string>(part));
        } else if (std::holds_alternative<LangAPI::ArrayMethodCall>(part)) {
            const auto &method_call = std::get<LangAPI::ArrayMethodCall>(part);
            switch (method_call.method) {
                case LangAPI::ArrayMethods::Push: res += "push"; break;
                case LangAPI::ArrayMethods::Pop: res += "pop"; break;
                case LangAPI::ArrayMethods::Size: res += "len"; break;
                default: throw Error("Unknown array method");
            }
            std::vector<std::string> args;
            for (const auto &param : method_call.args) args.push_back(convertExpression(param));
            res += "(" + joinStrings(args, ", ") + ")";
        } else if (std::holds_alternative<LangAPI::Property>(part)) {
            res += ident(std::get<LangAPI::Property>(part).name) + "()";
        } else if (std::holds_alternative<LangAPI::IspaLibSymbol>(part)) {
            res += convertIspaLibSymbol(std::get<LangAPI::IspaLibSymbol>(part), true);
        } else {
            res += convertFunctionCall(std::get<LangAPI::FunctionCall>(part));
        }
    }
    return res;
}

auto RustCore::convertIspaLibSymbol(const LangAPI::IspaLibSymbol &symbol, bool expr) -> std::string {
    const auto args = convertTemplates(symbol.template_parameters);
    const auto g = [&] { return generic(args, expr); };
    // scope of the generated Lexer type; its `iterator` typedef becomes an associated type of a runtime trait
    const auto lexer_scope = "<" + crateRoot() + "::Lexer as ::ispa_std::LexerTypes>";
    switch (symbol.exports) {
        case LangAPI::StdlibExports::Node: return "::ispa_std::Node" + g();
        case LangAPI::StdlibExports::MatchResult: return "::ispa_std::MatchResult" + g();
        case LangAPI::StdlibExports::Lexer: return "::ispa_std::LexerBase" + g();
        case LangAPI::StdlibExports::LRGeneralizedParser: return "::ispa_std::LRGeneralizedParser" + g();
        case LangAPI::StdlibExports::LRParseGeneralized: return "::ispa_std::lr_parse_generalized";
        case LangAPI::StdlibExports::LRParser: return "::ispa_std::LRParser" + g();
        case LangAPI::StdlibExports::LRGet: return "::ispa_std::lr_get";
        case LangAPI::StdlibExports::LRToken: return "::ispa_std::lr_token";
        case LangAPI::StdlibExports::LRPack: return "::ispa_std::lr_pack";
        case LangAPI::StdlibExports::LRCaptureAssign: return "::ispa_std::lr_capture_assign";
        case LangAPI::StdlibExports::LRNode: return "::ispa_std::lr_node";
        case LangAPI::StdlibExports::LRParse: return "::ispa_std::lr_parse";
        case LangAPI::StdlibExports::Parser: return "::ispa_std::LLParser" + g();
        case LangAPI::StdlibExports::LexerMakeTokenParameter:
            return symbol.Reference ? (symbol.Const ? "&::ispa_std::Ptr" : "&mut ::ispa_std::Ptr") : "::ispa_std::Ptr";
        case LangAPI::StdlibExports::ParserFunctionParameter:
            return "<Self as ::ispa_std::LLParserApi>::Iterator";
        case LangAPI::StdlibExports::DfaState: return "::ispa_std::dfa::api::State" + g();
        case LangAPI::StdlibExports::DfaTable: return "::ispa_std::dfa::api::Table" + g();
        case LangAPI::StdlibExports::DfaAcceptTable: return "::ispa_std::dfa::api::AcceptTable" + g();
        case LangAPI::StdlibExports::DfaClassTable: return "::ispa_std::dfa::api::CharToClass";
        case LangAPI::StdlibExports::DfaNullState: return "::ispa_std::dfa::api::NULL_STATE";
        case LangAPI::StdlibExports::DfaDynamicFailState: return "::ispa_std::dfa::api::DYNAMIC_FAIL_STATE";
        case LangAPI::StdlibExports::Error: return "::ispa_std::Error";
        case LangAPI::StdlibExports::TokenNodeConstruct: return "::ispa_std::Node" + g() + "::create";
        case LangAPI::StdlibExports::RawTokenNodeConstruct:
        case LangAPI::StdlibExports::ParserNodeConstructor: return "Token::create";
        case LangAPI::StdlibExports::DFADebug: return "::ispa_std::dfa::api::DfaDebug";
        case LangAPI::StdlibExports::TdfaLayout: return "::ispa_std::dfa::api::TdfaLayout" + g();
        case LangAPI::StdlibExports::DfaCaptures:
            return symbol.Reference ? (symbol.Const ? "&::ispa_std::dfa::api::Captures" : "&mut ::ispa_std::dfa::api::Captures")
                                    : "::ispa_std::dfa::api::Captures";
        case LangAPI::StdlibExports::DfaDynamicMatch: return "::ispa_std::dfa::api::dynamic_match";
        case LangAPI::StdlibExports::ASTPrinter: return "::ispa_std::ASTPrinter";
        case LangAPI::StdlibExports::Concat: return "::ispa_std::concat";
        case LangAPI::StdlibExports::MatchResultValue: return "node";
        case LangAPI::StdlibExports::VariantIndex: return "index()";
        case LangAPI::StdlibExports::ParserRequired: return "required";
        case LangAPI::StdlibExports::ParserOptional: return "optional";
        case LangAPI::StdlibExports::ParserRange: return "range";
        case LangAPI::StdlibExports::ParserRangeOptional: return "range_optional";
        case LangAPI::StdlibExports::ParserCounted: return "counted";
        case LangAPI::StdlibExports::AssignCompatible: return "::ispa_std::assign_compatible";
        case LangAPI::StdlibExports::LexerTokenIterator: return lexer_scope + "::Iterator";
        case LangAPI::StdlibExports::LexerLazyIterator: return lexer_scope + "::LazyIterator";
        default:
            throw Error("Unknown IspaLibSymbol exports: {}", (int) symbol.exports);
    }
}

// ============================================================================
// expressions
// ============================================================================
auto RustCore::convertExpression(const LangAPI::Expression &expression) -> std::string {
    std::ostringstream out;
    for (std::size_t i = 0; i < expression.size(); ++i) {
        const auto &expr = expression[i];
        switch (expr.type()) {
            case LangAPI::ExpressionValueType::Empty:
                break;
            case LangAPI::ExpressionValueType::RValue:
                out << convertRValue(expr.getRValue());
                break;
            case LangAPI::ExpressionValueType::EmptyInitializer:
                out << "Default::default()";
                break;
            case LangAPI::ExpressionValueType::ExpressionElement: {
                const auto element = expr.getExpressionElement();
                // Rust has no ++/--. As a statement `x++` / `++x` is `x += 1` (the value of the expression is lost).
                if (element == LangAPI::ExpressionElement::PlusPlus || element == LangAPI::ExpressionElement::MinusMinus) {
                    const char *op = element == LangAPI::ExpressionElement::PlusPlus ? " += 1" : " -= 1";
                    if (i > 0 && expression[i - 1].type() == LangAPI::ExpressionValueType::RValue) {
                        out << op;                                           // postfix
                    } else if (i + 1 < expression.size()) {                  // prefix
                        LangAPI::Expression operand;
                        operand.push_back(expression[i + 1]);
                        out << convertExpression(operand) << op;
                        ++i;
                    } else throw Error("Rust Converter: dangling ++/--");
                    warn("++/-- converted to `+= 1` / `-= 1`: only valid in statement position");
                    break;
                }
                out << convertExpressionElement(element);
                break;
            }
            case LangAPI::ExpressionValueType::FunctionCall:
                out << convertFunctionCall(expr.getFunctionCall());
                break;
            case LangAPI::ExpressionValueType::StringCompare: {
                // The C++ backend prints an unterminated `strncmp(pos, "..."`, so that path cannot have been working;
                // this is the complete version.
                const auto compare = expr.getStringCompare();
                if (compare.is_string)
                    out << "::ispa_std::starts_with(" << counter.back() << ", \"" << rustStringBody(compare.str.value) << "\")";
                else
                    out << "(" << counter.back() << ".get() == " << rustCharLiteral(compare.str.value[0], false) << ")";
                break;
            }
            case LangAPI::ExpressionValueType::Return: {
                const auto value = convertExpression(expr.getReturn().value);
                out << "return" << (value.empty() ? "" : " " + value);
                break;
            }
            case LangAPI::ExpressionValueType::Break:
                out << (loop_frames.empty() ? std::string("break") : loop_frames.back().break_stmt);
                break;
            case LangAPI::ExpressionValueType::Continue: {
                std::string target = "continue";
                for (auto it = loop_frames.rbegin(); it != loop_frames.rend(); ++it)
                    if (!it->continue_stmt.empty()) { target = it->continue_stmt; break; }   // switch frames are transparent
                out << target;
                break;
            }
            case LangAPI::ExpressionValueType::VariableAssignment: {
                const auto &var = expr.getVariableAssignment();
                std::string op;
                switch (var.type) {
                    case LangAPI::OperatorType::Assign: op = "="; break;
                    case LangAPI::OperatorType::Add: op = "+="; break;
                    case LangAPI::OperatorType::Minus: op = "-="; break;
                    case LangAPI::OperatorType::Multiply: op = "*="; break;
                    case LangAPI::OperatorType::Divide: op = "/="; break;
                    case LangAPI::OperatorType::Modulo: op = "%="; break;
                }
                if (std::holds_alternative<LangAPI::Symbol>(var.name))
                    out << convertSymbol(std::get<LangAPI::Symbol>(var.name));
                else
                    out << convertStorageSymbol(std::get<LangAPI::StorageSymbol>(var.name));
                out << " " << op << " " << convertExpression(var.value);
                break;
            }
            // `pos` is a Copy cursor (::ispa_std::Ptr); inc() advances it and returns the new value, so it also works
            // inside larger expressions where `+= 1` would not.
            case LangAPI::ExpressionValueType::CounterIncreament:
                out << counter.back() << ".inc()";
                break;
            case LangAPI::ExpressionValueType::CounterAssignment:
                out << counter.back() << " = " << convertExpression(*expr.getCounterAssignment().v);
                break;
            case LangAPI::ExpressionValueType::ResetPosCounter:
                counter.pop_back();
                break;
            case LangAPI::ExpressionValueType::PushPosCounter: {
                const auto &a = expr.getPushPosCounter();
                out << "let mut " << ident(a.name) << " = " << counter.back();
                counter.push_back(ident(a.name));
                break;
            }
            case LangAPI::ExpressionValueType::PopPosCounter: {
                out << counter[counter.size() - 2] << " = " << counter.back();
                counter.pop_back();
                break;
            }
            case LangAPI::ExpressionValueType::SkipSpaces:
                out << "::ispa_std::skip_spaces::<" << crateRoot() << "::Types::__WS>(&mut " << counter.back() << ")";
                break;
            case LangAPI::ExpressionValueType::DfaLookup: {
                const auto &lookup = expr.getDfaLookup();
                out << ident(lookup.output_name) << " = ::ispa_std::dfa::decide(&dfa_table_" << std::to_string(lookup.dfa_count)
                    << ", pos, &Self::PANIC_MODE)";
                break;
            }
            case LangAPI::ExpressionValueType::ReportError:
                out << "self.raise_error(pos.get(), \"" << rustStringBody(expr.getReportError().message) << "\")";
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

auto RustCore::convertExpressionElement(LangAPI::ExpressionElement element) -> const char * {
    switch (element) {
        case LangAPI::ExpressionElement::GroupOpen: return "(";
        case LangAPI::ExpressionElement::GroupClose: return ")";
        case LangAPI::ExpressionElement::SquareBraceOpen: return "[";
        case LangAPI::ExpressionElement::SquareBraceClose: return "]";
        case LangAPI::ExpressionElement::And: return " && ";
        case LangAPI::ExpressionElement::Or: return " || ";
        case LangAPI::ExpressionElement::Not: return "!";
        case LangAPI::ExpressionElement::Equal: return " == ";
        case LangAPI::ExpressionElement::NotEqual: return " != ";
        case LangAPI::ExpressionElement::Higher: return " > ";
        case LangAPI::ExpressionElement::Lower: return " < ";
        case LangAPI::ExpressionElement::HigherOrEqual: return " >= ";
        case LangAPI::ExpressionElement::LowerOrEqual: return " <= ";
        case LangAPI::ExpressionElement::Add: return " + ";
        case LangAPI::ExpressionElement::Minus: return " - ";
        case LangAPI::ExpressionElement::Multiply: return " * ";
        case LangAPI::ExpressionElement::Divide: return " / ";
        case LangAPI::ExpressionElement::Modulo: return " % ";
        default:
            throw Error("Unknown expression element");
    }
}

auto RustCore::convertLambda(const LangAPI::Lambda &lambda) -> std::string {
    std::vector<std::string> params;
    for (const auto &p : lambda.parameters) {
        const auto type = convertType(p.first);
        params.push_back(ident(p.second) + (type == "_" ? "" : ": " + type));
    }
    return "|" + joinStrings(params, ", ") + "| {\n" + lambda_body(lambda.statements) + "}";
}

auto RustCore::convertFunctionCall(const LangAPI::FunctionCall &call) -> std::string {
    std::vector<std::string> args;
    for (const auto &param : call.args) args.push_back(convertExpression(param));

    if (std::holds_alternative<std::shared_ptr<LangAPI::Symbol>>(call.name)) {
        const auto &symbol = *std::get<std::shared_ptr<LangAPI::Symbol>>(call.name);
        const auto symbol_name = convertSymbol(symbol);
        const auto recovery_set = [](const auto& parameter) {
            const auto* type = std::get_if<std::shared_ptr<LangAPI::Type>>(&parameter);
            if (!type || !*type || !(**type).isIspaLibSymbol() ||
                (**type).getIspaLibSymbol().exports != LangAPI::StdlibExports::RecoveryTokenSet)
                throw Error("Rust Converter: expected a recovery token set");
            std::vector<std::string> values;
            for (const auto& value : (**type).getIspaLibSymbol().template_parameters) {
                if (const auto* item = std::get_if<std::shared_ptr<LangAPI::Type>>(&value)) {
                    if ((**item).isIspaLibSymbol() && (**item).getIspaLibSymbol().exports == LangAPI::StdlibExports::RecoveryEndOfInput)
                        values.push_back("::ispa_std::recovery::END_OF_INPUT");
                    else values.push_back(RustCore::convertType(**item));
                } else values.push_back(RustCore::convertRValue(*std::get<std::shared_ptr<LangAPI::RValue>>(value)));
            }
            return "&[" + joinStrings(values, ", ") + "]";
        };
        if (symbol_name == "recovery_at" || symbol_name == "recover_choice") {
            std::vector<std::string> sets;
            for (const auto& parameter : call.template_parameters) sets.push_back(recovery_set(parameter));
            return "self." + symbol_name + "(&[" + joinStrings(sets, ", ") + "]" +
                   (args.empty() ? "" : ", " + joinStrings(args, ", ")) + ")";
        }
        if (symbol_name == "ISPA_STD::Recovery::handler" || symbol_name == "::ISPA_STD::Recovery::handler") {
            if (call.template_parameters.size() != 5 || args.size() != 2)
                throw Error("Rust Converter: invalid recovery handler");
            std::vector<std::string> sets;
            for (std::size_t i = 0; i < 4; ++i) sets.push_back(recovery_set(call.template_parameters[i]));
            const auto* policy_type = std::get_if<std::shared_ptr<LangAPI::Type>>(&call.template_parameters[4]);
            if (!policy_type || !*policy_type || !(**policy_type).isIspaLibSymbol() ||
                (**policy_type).getIspaLibSymbol().exports != LangAPI::StdlibExports::RecoveryPolicy)
                throw Error("Rust Converter: invalid recovery policy");
            std::vector<std::string> policy;
            for (const auto& value : (**policy_type).getIspaLibSymbol().template_parameters) {
                if (const auto* item = std::get_if<std::shared_ptr<LangAPI::RValue>>(&value))
                    policy.push_back(convertRValue(**item));
                else policy.push_back(convertType(**std::get_if<std::shared_ptr<LangAPI::Type>>(&value)));
            }
            return "::ispa_std::recovery::handler(" + joinStrings(sets, ", ") + ", " +
                   joinStrings(policy, ", ") + ", " + joinStrings(args, ", ") + ")";
        }
        // call-site adaptation inserted by Rust.Rewriter
        if (symbol.path.size() == 1 && std::holds_alternative<std::string>(symbol.path.front()) && args.size() == 1) {
            const auto &n = std::get<std::string>(symbol.path.front());
            if (n == kRefWrapper) return "&" + asExpr(args[0]);
            if (n == kMutWrapper) return "&mut " + asExpr(args[0]);
            if (n == kCloneWrapper) return asExpr(args[0]) + ".clone()";
        }
        return convertSymbol(symbol) + generic(convertTemplates(call.template_parameters), true) + "(" + joinStrings(args, ", ") + ")";
    }
    const auto &lib = *std::get<std::shared_ptr<LangAPI::IspaLibSymbol>>(call.name);
    std::string callee = convertIspaLibSymbol(lib, true);
    if (lib.exports == LangAPI::StdlibExports::Error) callee += "::new";           // std::runtime_error(msg) -> Error::new(msg)
    return callee + generic(convertTemplates(call.template_parameters), true) + "(" + joinStrings(args, ", ") + ")";
}

auto RustCore::convertIspaLibFunctionCall(const LangAPI::IspaLibFunctionCall &call) -> std::string {
    const auto kind = call.symbol.exports;
    const bool parser_match = kind == LangAPI::StdlibExports::ParserRequired ||
        kind == LangAPI::StdlibExports::ParserOptional ||
        kind == LangAPI::StdlibExports::ParserRange ||
        kind == LangAPI::StdlibExports::ParserRangeOptional;
    const bool parser_call = parser_match || kind == LangAPI::StdlibExports::ParserCounted;

    std::vector<std::string> args;
    for (std::size_t i = 0; i < call.args.size(); ++i) {
        const auto &param = call.args[i];
        const bool counted_callback = kind == LangAPI::StdlibExports::ParserCounted && i == 2;
        const bool callback_slot = (parser_match && i == 1 &&
            (call.args.size() == 3 || kind == LangAPI::StdlibExports::ParserOptional ||
             kind == LangAPI::StdlibExports::ParserRangeOptional)) || counted_callback;
        if (callback_slot && param.size() == 1 && param.front().isRvalue() && param.front().getRValue().isSymbol()) {
            // Rule callbacks are member functions. `self.required(pos, |c| self.rule(c))` borrows self twice, so the runtime
            // hands the parser back to the callback as its first argument instead.
            const auto &sym = param.front().getRValue().getSymbol();
            std::vector<std::string> comps;
            bool simple = true;
            for (const auto &part : sym.path) {
                if (std::holds_alternative<std::string>(part)) comps.push_back(std::get<std::string>(part));
                else simple = false;
            }
            if (simple && !comps.empty() && (comps.front() == "self" || comps.front() == "this")) comps.erase(comps.begin());
            if (simple && comps.size() == 1)
                args.push_back("|__p, cursor| __p." + ident(comps.front()) + "(cursor)");
            else
                args.push_back("|__p, cursor| " + convertExpression(param) + "(cursor)");
        } else {
            args.push_back(convertExpression(param));
        }
    }
    const auto turbofish = generic(convertTemplates(call.template_parameters), true);
    if (parser_call) {
        // inherited LLParser members: reached through the LLParserApi trait implemented by the generated parser
        std::string joined = args.empty() ? "" : ", " + joinStrings(args, ", ");
        return "::ispa_std::LLParserApi::" + convertIspaLibSymbol(call.symbol, true) + turbofish + "(self" + joined + ")";
    }
    std::string callee = convertIspaLibSymbol(call.symbol, true);
    if (kind == LangAPI::StdlibExports::Error) callee += "::new";
    return callee + turbofish + "(" + joinStrings(args, ", ") + ")";
}

auto RustCore::convertMakeTuple(const LangAPI::MakeTuple &make_tuple) -> std::string {
    std::vector<std::string> parts;
    for (const auto &el : make_tuple.args) parts.push_back(convertExpression(el));
    return "(" + joinStrings(parts, ", ") + (parts.size() == 1 ? ",)" : ")");
}

// std::get<T>(v) / holds_alternative<T>(v) go through the generated variant enums' VariantGet<T> impls.
// std::get returns a reference to the alternative, so `.field` works but passing it by value needs a clone.
auto RustCore::convertGetVariant(const LangAPI::GetVariant &get_variant) -> std::string {
    if (get_variant.is_raw_token)
        return "(" + convertExpression(get_variant.sym) + ").to_node::<" + convertType(*get_variant.type) + ">()";
    return "::ispa_std::VariantGet::<" + convertType(*get_variant.type) + ">::get(&" + asExpr(convertExpression(get_variant.sym)) + ")";
}
auto RustCore::convertCheckVariant(const LangAPI::CheckVariant &check_variant) -> std::string {
    if (check_variant.is_raw_token)
        return "(" + convertExpression(check_variant.sym) + ").holds::<" + convertType(*check_variant.type) + ">()";
    return "::ispa_std::VariantGet::<" + convertType(*check_variant.type) + ">::holds(&" + asExpr(convertExpression(check_variant.sym)) + ")";
}

auto RustCore::convertDFADebug(const LangAPI::DFADebug &dfa_debug) -> std::string {
    std::string member;
    if (dfa_debug.member.empty()) {
        member = "<Undefined>";
    } else if (dfa_debug.member.isString()) {
        member = dfa_debug.member.getString().value;
    } else if (dfa_debug.member.isCsequence()) {
        const auto &sequence = dfa_debug.member.getCsequence();
        member = "[";
        for (const auto c : sequence.characters) member += c;
        for (const auto c : sequence.escaped) { member += '\\'; member += c; }
        for (const auto &[from, to] : sequence.diapasons) { member += from; member += '-'; member += to; }
        member += "]";
    } else {
        std::stringstream member_stream;
        member_stream << dfa_debug.member;
        member = member_stream.str();
    }
    // `member` is display text, not source: backslashes must be doubled here (unlike rustStringBody, which receives source text)
    const auto literal = [](std::string_view text) {
        std::string out;
        for (const unsigned char c : text) {
            if (c == '\\') out += "\\\\";
            else if (c == '"') out += "\\\"";
            else if (c == '\n') out += "\\n";
            else if (c == '\r') out += "\\r";
            else if (c == '\t') out += "\\t";
            else if (c == 0) out += "\\0";
            else if (c < 0x20 || c == 0x7F) out += hex2(c);
            else out += static_cast<char>(c);
        }
        return "\"" + out + "\"";
    };
    const auto state = [](auto value) {
        return value == constants::NULL_STATE ? std::string("-1") : std::to_string(value);
    };
    std::ostringstream ss;
    ss << "::ispa_std::dfa::api::DfaDebug::new(" << literal(member)
       << ", " << literal(corelib::text::join(dfa_debug.token_name, "."))
       << ", " << dfa_debug.position_in_token
       << ", " << state(dfa_debug.call)
       << ", " << state(dfa_debug.group)
       << ", " << state(dfa_debug.rule_run)
       << ", " << dfa_debug.offset
       << ", " << dfa_debug.length
       << ", " << rustCharLiteral(dfa_debug.ch, false) << ")";
    return ss.str();
}

// ============================================================================
// rvalues
// ============================================================================
auto RustCore::convertRValue(const LangAPI::RValue &rvalue) -> std::string {
    const auto values = [](const auto &list, bool brackets) {
        std::vector<std::string> parts;
        for (const auto &el : list) parts.push_back(convertExpression(el));
        return (brackets ? "[" : "") + joinStrings(parts, ", ") + (brackets ? "]" : "");
    };
    switch (rvalue.type()) {
        case LangAPI::RValueType::Undef:
            throw Error("RValue type is undefined");
        case LangAPI::RValueType::Char:
            return rustCharLiteral(rvalue.getChar().value, rvalue.getChar().escaped);
        case LangAPI::RValueType::Int:
            return std::to_string(rvalue.getInt().value);
        case LangAPI::RValueType::Bool:
            return rvalue.getBool().value ? "true" : "false";
        case LangAPI::RValueType::Float:
            return std::to_string(rvalue.getFloat().value);
        case LangAPI::RValueType::String:
            return "\"" + rustStringBody(rvalue.getString().value) + "\"";
        case LangAPI::RValueType::Array: {
            const auto &array = rvalue.getArray();
            if (array.values.empty())
                return array.template_parameters.empty() ? std::string("Vec::new()")
                                                         : "Vec::<" + convertTemplates(array.template_parameters) + ">::new()";
            return "vec![" + values(array.values, false) + "]";
        }
        case LangAPI::RValueType::FixedSizeArray:
            return values(rvalue.getFixedSizeArray().values, true);
        case LangAPI::RValueType::Map: {
            const auto &map = rvalue.getMap();
            std::vector<std::string> pairs;
            for (std::size_t i = 0; i < map.keys.size(); ++i) {
                std::string key;
                if (std::holds_alternative<LangAPI::String>(map.keys[i]))
                    key = "String::from(\"" + rustStringBody(std::get<LangAPI::String>(map.keys[i]).value) + "\")";
                else if (std::holds_alternative<LangAPI::Int>(map.keys[i]))
                    key = std::to_string(std::get<LangAPI::Int>(map.keys[i]).value);
                else throw Error("Rust Converter: unsupported map key kind");
                pairs.push_back("(" + key + ", " + convertExpression(map.values[i]) + ")");
            }
            const auto head = map.template_parameters.empty() ? std::string("::std::collections::HashMap")
                                                              : "::std::collections::HashMap::<" + convertTemplates(map.template_parameters) + ">";
            return head + "::from([" + joinStrings(pairs, ", ") + "])";
        }
        case LangAPI::RValueType::Pos: {
            // pos is a Copy cursor: offset(n) moves it, get() reads the byte under it
            const auto &counter_data = rvalue.getPos();
            std::string res = counter.back();
            if (counter_data.offset != 0) res += ".offset(" + std::to_string(counter_data.offset) + ")";
            if (counter_data.dereference) res += ".get()";
            return res;
        }
        case LangAPI::RValueType::Symbol:
            return convertSymbol(rvalue.getSymbol());
        case LangAPI::RValueType::IspaLibSymbol:
            return convertIspaLibSymbol(rvalue.getIspaSymbol(), true);
        case LangAPI::RValueType::StorageSymbol:
            return convertStorageSymbol(rvalue.getStorageSymbol());
        case LangAPI::RValueType::Inheritance: {
            // `Base{args}` in a constructor initialiser list. Rust has no struct-literal-by-position, so the base type
            // is expected to provide `new(args)`.
            const auto &inheritance = rvalue.getInheritance();
            std::string base;
            if (std::holds_alternative<LangAPI::Symbol>(inheritance.name))
                base = convertSymbol(std::get<LangAPI::Symbol>(inheritance.name));
            else if (std::holds_alternative<LangAPI::IspaLibSymbol>(inheritance.name))
                base = convertIspaLibSymbol(std::get<LangAPI::IspaLibSymbol>(inheritance.name), false);
            else
                base = convertType(*std::get<std::shared_ptr<LangAPI::Type>>(inheritance.name));
            std::vector<std::string> args;
            for (const auto &a : inheritance.args) args.push_back(convertExpression(a));
            return "<" + base + ">::new(" + joinStrings(args, ", ") + ")";
        }
        case LangAPI::RValueType::IspaLibDfaTransition: {
            const auto &transition = rvalue.getIspaLibDfaTransition();
            const auto type_args = convertTemplates(transition.transition_type.template_parameters);
            std::vector<std::string> args;
            if (std::holds_alternative<std::size_t>(transition.symbol)) {
                const auto sym = std::to_string(std::get<std::size_t>(transition.symbol));
                const auto slice = "::ispa_std::Span::from_slice(&dfa_table_" + sym + "[..])";
                if (transition.is_refferring_char_table)
                    args.push_back("::ispa_std::dfa::api::SpanCharTable::<Tokens, " + type_args + ">::from_slice(&dfa_table_" + sym + "[..])");
                else
                    args.push_back("::ispa_std::dfa::api::SpanMultiTable::<Tokens, " + type_args + ">::new(" + slice + ")");
            } else if (!std::holds_alternative<char>(transition.symbol)) {
                args.push_back("Tokens::" + corelib::text::join(std::get<stdu::vector<std::string>>(transition.symbol), "_"));
            }
            args.push_back(numberOrNull(transition.next));
            args.push_back(transition.new_cst_node ? "true" : "false");
            args.push_back(transition.new_member ? "true" : "false");
            args.push_back(transition.close_cst_node ? "true" : "false");
            args.push_back(numberOrNull(transition.new_group));
            args.push_back(numberOrNull(transition.group_close));
            args.push_back(numberOrNull(transition.accept));
            return convertIspaLibSymbol(transition.transition_type, true) + "::new(" + joinStrings(args, ", ") + ")";
        }
        case LangAPI::RValueType::IspaLibDfaSpanCharState: {
            const auto &state = rvalue.getIspaLibDfaSpanCharState();
            return "::ispa_std::dfa::api::SpanCharState::new(" + numberOrNull(state.else_goto) + ", " + numberOrNull(state.else_goto_accept) +
                   ", ::ispa_std::Span::from_slice(&dfa_state_" + std::to_string(state.state_id) + "[..]))";
        }
        case LangAPI::RValueType::IspaLibDfaSpanMultiTableState: {
            const auto &state = rvalue.getIspaLibDfaSpanMultiTableState();
            std::string tmpl = "Tokens";
            for (const auto &sym : state.mutli_table_transitions) tmpl += ", " + convertIspaLibSymbol(sym, false);
            return "::ispa_std::dfa::api::SpanMultiTableState::<" + tmpl + ">::new(" + numberOrNull(state.else_goto) + ", " +
                   numberOrNull(state.else_goto_accept) + ", ::ispa_std::Span::from_slice(&dfa_state_" + std::to_string(state.state_id) + "[..]))";
        }
        case LangAPI::RValueType::IspaLibDfaSpan: {
            const auto &span = rvalue.getIspaLibDfaSpan();
            return convertIspaLibSymbol(span.type, true) + "::from_slice(&" + convertSymbol(span.assing_name) + "[..])";
        }
        case LangAPI::RValueType::Reference:
            return "&" + convertRValue(*rvalue.getReference().value);
        case LangAPI::RValueType::Span:
            return "::ispa_std::Span::<" + convertType(*rvalue.getSpan().type) + ">::from_slice(&" + convertSymbol(rvalue.getSpan().sym) + "[..])";
        case LangAPI::RValueType::MakeTuple:
            return convertMakeTuple(rvalue.getMakeTuple());
        case LangAPI::RValueType::GetVariant:
            return convertGetVariant(rvalue.getVariantCast());
        case LangAPI::RValueType::CheckVariant:
            return convertCheckVariant(rvalue.CheckVariantCast());
        case LangAPI::RValueType::ToString:
            return asExpr(convertExpression(rvalue.getToString().what)) + ".to_string()";
        case LangAPI::RValueType::CharToStringConstructor:
            return "String::from((" + convertExpression(rvalue.getCharToStringConstructor().what) + ") as char)";
        case LangAPI::RValueType::Cast:
            return "((" + convertExpression(rvalue.getCast().what) + ") as " + convertType(*rvalue.getCast().type) + ")";
        case LangAPI::RValueType::GetFromBox:
            return "(*" + asExpr(convertExpression(rvalue.getGetFromBox().what)) + ")";
        default:
            throw Error("Unknown RValue type: {}", (int) rvalue.type());
    }
}
