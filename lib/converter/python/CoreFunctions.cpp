module Python.CoreFunctions;

import dstd;
import std;

namespace Python {
namespace {
    template<class... F> struct overloaded : F... { using F::operator()...; };
    template<class... F> overloaded(F...) -> overloaded<F...>;
    template<typename Items, typename Fn>
    std::string join(const Items& items, const std::string& sep, Fn&& render) {
        std::string out;
        for (const auto& item : items) {
            if (!out.empty()) out += sep;
            out += render(item);
        }
        return out;
    }
    [[noreturn]] void unsupported(const char* kind) {
        throw std::runtime_error(std::string("Python target: not lowered: ") + kind);
    }
    std::string member(const std::string& name) {
        if (name == "__ispa_python_self") return "self";
        return name;
    }
    // `A::B::C` -> `A.B.C`, escaping every segment (not just the whole string).
    std::string path_identifiers(const std::string& name) {
        const auto dotted = Core::symbol_path(name);
        std::string out;
        std::size_t begin = 0;
        while (begin <= dotted.size()) {
            const auto p = dotted.find('.', begin);
            const auto part = dotted.substr(begin, p == std::string::npos ? p : p - begin);
            if (begin) out += '.';
            out += Core::identifier(part);
            if (p == std::string::npos) break;
            begin = p + 1;
        }
        return out;
    }
    bool is_name(const std::string& text) {
        if (text.empty() || std::isdigit(static_cast<unsigned char>(text.front()))) return false;
        for (const char c : text)
            if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_')) return false;
        return true;
    }
    // C++ `x++` / `++x` / `x--` / `--x` used as a value, applied to the rendered operand text.
    //   local name     -> walrus: (x := x + 1), postfix: ((x := x + 1) - 1)
    //   obj.attr       -> ispa_std.step_attr(obj, "attr", delta, post)
    //   obj[key]       -> ispa_std.step_item(obj, key, delta, post)
    std::string step_value(const std::string& operand, int delta, bool post) {
        const std::string sign = delta > 0 ? " + 1" : " - 1";
        const std::string undo = delta > 0 ? " - 1" : " + 1";
        if (is_name(operand)) {
            const auto updated = "(" + operand + " := " + operand + sign + ")";
            return post ? "(" + updated + undo + ")" : updated;
        }
        const std::string tail = std::string(", ") + std::to_string(delta) + ", " + (post ? "True" : "False") + ")";
        if (operand.ends_with(']')) {
            int depth = 0;
            for (std::size_t i = operand.size(); i-- > 0;) {
                if (operand[i] == ']') ++depth;
                else if (operand[i] == '[' && --depth == 0) {
                    if (i == 0) break;
                    return "ispa_std.step_item(" + operand.substr(0, i) + ", " +
                           operand.substr(i + 1, operand.size() - i - 2) + tail;
                }
            }
        } else {
            const auto dot = operand.rfind('.');
            if (dot != std::string::npos && dot > 0 && is_name(operand.substr(dot + 1)))
                return "ispa_std.step_attr(" + operand.substr(0, dot) + ", \"" + operand.substr(dot + 1) + "\"" + tail;
        }
        unsupported("++/-- on an operand that is not a name, attribute or subscript");
    }
    // Shortest round-trip text, always a valid Python float literal.
    std::string float_literal(double v) {
        if (std::isnan(v)) return "float('nan')";
        if (std::isinf(v)) return v < 0 ? "float('-inf')" : "float('inf')";
        auto text = std::format("{}", v);
        if (text.find_first_of(".e") == std::string::npos) text += ".0";
        return text;
    }
    // C++ `/` on integers truncates; Python `/` always yields float. Without type
    // information we use true division only when the expression visibly involves a float.
    bool has_float(const LangAPI::Expression& e) {
        for (const auto& part : e) {
            if (part.type() != LangAPI::ExpressionValueType::RValue) continue;
            const auto& v = part.getRValue();
            if (v.type() == LangAPI::RValueType::Float) return true;
            if (v.type() == LangAPI::RValueType::Cast) {
                const auto& t = v.getCast().type;
                if (t && t->isValueType() && t->getValueType() == LangAPI::ValueType::Float) return true;
            }
        }
        return false;
    }
}
Core::Core(std::string name) : module_name_(std::move(name)) {}
void Core::set_statement_writer(StatementWriter value) {
    statement_writer_ = std::move(value);
}
void Core::enter_scope(const std::vector<std::string>& params) {
    scopes_.emplace_back(params.begin(), params.end());
    assigned_.emplace_back();
}
void Core::declare(const std::string& name) {
    if (!scopes_.empty()) scopes_.back().insert(name);
}
void Core::note_assigned(const std::string& name) {
    if (!assigned_.empty()) assigned_.back().insert(name);
}
std::vector<std::string> Core::leave_scope() {
    std::vector<std::string> result;
    if (scopes_.empty()) return result;
    // C++ `[&]` lambdas write through to captured locals; a bare assignment inside a Python
    // `def` would silently create a new local instead, so such names need `nonlocal`.
    for (const auto& name : assigned_.back()) {
        if (scopes_.back().contains(name)) continue;
        for (std::size_t i = scopes_.size() - 1; i-- > 0;) {
            if (scopes_[i].contains(name)) { result.push_back(name); break; }
        }
    }
    std::sort(result.begin(), result.end());
    scopes_.pop_back();
    assigned_.pop_back();
    return result;
}
std::vector<Core::LambdaDefinition> Core::take_pending_lambdas() {
    std::vector<LambdaDefinition> result;
    result.swap(pending_lambdas_);
    return result;
}
void Core::restore_pending_lambdas(std::vector<LambdaDefinition> saved) {
    // Entries queued before the nested rendering stay ahead of anything queued since.
    saved.insert(saved.end(), std::make_move_iterator(pending_lambdas_.begin()),
                 std::make_move_iterator(pending_lambdas_.end()));
    pending_lambdas_ = std::move(saved);
}
std::string Core::counter() const { return counters_.back(); }
std::string Core::identifier(const std::string& name) {
    // Names beginning with two underscores are rewritten by Python inside
    // class suites, including attribute access such as Types.__WS.
    if (name.starts_with("__") && !name.ends_with("__")) return "_ispa" + name;
    static const std::unordered_set<std::string> reserved {
        "False", "None", "True", "and", "as", "assert", "async", "await", "break", "class", "continue",
        "def", "del", "elif", "else", "except", "finally", "for", "from", "global", "if", "import",
        "in", "is", "lambda", "nonlocal", "not", "or", "pass", "raise", "return", "try", "while",
        "with", "yield", "match", "case", "type",
        // Names the generator itself emits: a C++ identifier must not shadow them.
        "self", "len", "str", "int", "float", "bool", "list", "dict", "tuple", "ispa_std",
        "typing", "enum"
    };
    if (reserved.contains(name)) return name + "_";
    return name;
}
std::string Core::symbol_path(const std::string& name) {
    std::string result;
    for (std::size_t i = 0; i < name.size(); ++i) {
        if (name[i] == ':' && i + 1 < name.size() && name[i + 1] == ':') {
            result += '.'; ++i;
        } else result += name[i];
    }
    if (result.starts_with('.')) result.erase(0, 1);
    return result;
}
std::string Core::literal(const std::string& value) const {
    static constexpr char hex[] = "0123456789abcdef";
    std::string out = "\"";
    for (const unsigned char c : value) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            case '\0': out += "\\x00"; break;
            default:
                if (c < 32 || c == 127) {
                    out += "\\x"; out += hex[c >> 4]; out += hex[c & 15];
                } else out += static_cast<char>(c);
        }
    }
    return out + "\"";
}
std::string Core::templates(const decltype(LangAPI::Type::template_parameters)& values) {
    return join(values, ", ", [this](const auto& item) {
        if (const auto* t = std::get_if<LangAPI::Type>(&item)) return type(*t);
        return rvalue(std::get<LangAPI::RValue>(item));
    });
}
std::string Core::templates(const stdu::vector<std::variant<std::shared_ptr<LangAPI::Type>, std::shared_ptr<LangAPI::RValue>>>& values) {
    return join(values, ", ", [this](const auto& item) {
        if (const auto* t = std::get_if<std::shared_ptr<LangAPI::Type>>(&item)) return type(**t);
        return rvalue(*std::get<std::shared_ptr<LangAPI::RValue>>(item));
    });
}
std::string Core::type(const LangAPI::Type& t) {
    if (t.isSymbol()) {
        auto name = symbol(t.getSymbol());
        if (name.find('&') != std::string::npos || name.find('*') != std::string::npos)
            return "typing.Any";
        return name; // Python erases user-defined C++ template instantiation.
    }
    if (t.isIspaLibSymbol()) return lib(t.getIspaLibSymbol());
    if (!t.isValueType()) unsupported("LangAPI::Type");
    using V = LangAPI::ValueType;
    const std::string args = templates(t.template_parameters);
    switch (t.getValueType()) {
        case V::Void: return "None";
        case V::ImmutableSemantic:
        case V::BorrowSemantic:
        case V::MoveSemantic: return args.empty() ? "typing.Any" : args;
        case V::Char:
        case V::String:
        case V::NonOwnedString: return "str";
        case V::Int:
        case V::LargeUInt: return "int";
        case V::Float: return "float";
        case V::Bool: return "bool";
        case V::Tuple: return "tuple[" + (args.empty() ? "typing.Any, ..." : args) + "]";
        case V::Array:
        case V::FixedSizeArray: return "list[" + (args.empty() ? "typing.Any" : args) + "]";
        case V::Map: return "dict[" + (args.empty() ? "typing.Any, typing.Any" : args) + "]";
        case V::Token:
        case V::Rule: return "ispa_std.Node";
        case V::RawToken:
            return args.empty() ? "ispa_std.RawToken" : "ispa_std.RawToken.specialize(" + args + ")";
        case V::TokenResult:
        case V::RuleResult: return "ispa_std.MatchResult";
        case V::Variant:
        case V::LargeVariant:
            return args.empty() ? "ispa_std.Variant" : "ispa_std.Variant.specialize(" + args + ")";
        case V::Box: return args.empty() ? "typing.Any" : args;
        case V::Span: return "list[" + (args.empty() ? "typing.Any" : args) + "]";
        case V::TypeTag: return "type";
        case V::Undef: return "typing.Any";
    }
    unsupported("ValueType");
}
std::string Core::symbol(const LangAPI::Symbol& s) {
    std::string result;
    bool first = true;
    for (const auto& piece : s.path) {
        std::string item;
        if (const auto* str = std::get_if<std::string>(&piece)) {
            item = (*str == "__ispa_parser__") ? module_name_ : member(path_identifiers(*str));
            if (first && method_members_.contains(*str)) item = "self." + item;
        } else if (const auto* special = std::get_if<LangAPI::IspaLibSymbol>(&piece)) {
            item = lib(*special);
        } else item = call(std::get<LangAPI::FunctionCall>(piece));
        if (!result.empty() && !item.empty()) result += '.';
        result += item;
        first = false;
    }
    return result;
}
std::string Core::storage(const LangAPI::StorageSymbol& s) {
    std::string result = expression(s.what);
    for (const auto& piece : s.path) {
        if (const auto* name = std::get_if<std::string>(&piece)) {
            result += "." + identifier(*name);
        } else if (const auto* offset = std::get_if<LangAPI::StorageOffset>(&piece)) {
            result += "[" + expression(offset->offset) + "]";
        } else if (const auto* property = std::get_if<LangAPI::Property>(&piece)) {
            result += "." + property->name + "()";
        } else if (const auto* method = std::get_if<LangAPI::ArrayMethodCall>(&piece)) {
            using M = LangAPI::ArrayMethods;
            std::string name;
            switch (method->method) {
                case M::Push:
                case M::PythonAppend: name = "append"; break;
                case M::Pop: name = "pop"; break;
                case M::Size: result = "len(" + result + ")"; continue;
                case M::PythonExtend: name = "extend"; break;
                case M::PythonInsert: name = "insert"; break;
                case M::PythonRemove: name = "remove"; break;
                case M::PythonPop: name = "pop"; break;
                case M::PythonClear: name = "clear"; break;
                case M::PythonCopy: name = "copy"; break;
                case M::PythonCount: name = "count"; break;
                case M::PythonIndex: name = "index"; break;
                case M::PythonReverse: name = "reverse"; break;
                case M::PythonSort: name = "sort"; break;
                case M::PythonGet: name = "get"; break;
                case M::PythonSetDefault: name = "setdefault"; break;
                case M::PythonUpdate: name = "update"; break;
                case M::PythonKeys: name = "keys"; break;
                case M::PythonValues: name = "values"; break;
                case M::PythonItems: name = "items"; break;
                case M::PythonFromKeys: name = "fromkeys"; break;
                default: unsupported("ArrayMethodCall");
            }
            result += "." + name + "(" + join(method->args, ", ", [this](const auto& a) { return expression(a); }) + ")";
        } else if (const auto* func = std::get_if<LangAPI::FunctionCall>(&piece)) {
            result += "." + call(*func);
        } else if (const auto* special = std::get_if<LangAPI::IspaLibSymbol>(&piece)) {
            result += "." + lib(*special);
        } else unsupported("StorageSymbol segment");
    }
    return result;
}
std::string Core::lib(const LangAPI::IspaLibSymbol& s) {
    using S = LangAPI::StdlibExports;
    switch (s.exports) {
        case S::Node: return "ispa_std.Node";
        case S::MatchResult: return "ispa_std.MatchResult";
        case S::Lexer: return "ispa_std.Lexer";
        case S::Parser: return "ispa_std.LLParser";
        case S::RawTokenNodeConstruct:
        case S::ParserNodeConstructor: return "Token.create";
        case S::TokenNodeConstruct: return "ispa_std.Node.create";
        case S::LexerMakeTokenParameter: return "ispa_std.Position";
        case S::ParserFunctionParameter:
        case S::LexerTokenIterator:
        case S::LexerLazyIterator: return "ispa_std.Cursor";
        case S::DfaState:
        case S::DfaTable:
        case S::DfaAcceptTable: return "list";
        case S::DfaClassTable: return "list[int]";
        case S::DfaNullState: return "ispa_std.NULL_STATE";
        case S::TdfaLayout: return "ispa_std.TdfaLayout(" + templates(s.template_parameters) + ")";
        case S::DfaCaptures: return "ispa_std.Captures";
        case S::DfaDynamicMatch: return "ispa_std.dynamic_match";
        case S::DFADebug: return "ispa_std.DFADebug";
        case S::Error: return "RuntimeError";
        case S::ASTPrinter: return "ispa_std.ASTPrinter";
        case S::Concat: return "ispa_std.concat";
        case S::MatchResultValue: return "node";
        case S::VariantIndex: return "index()";
        case S::ParserRequired: return "self.required";
        case S::ParserOptional: return "self.optional";
        case S::ParserRange: return "self.range";
        case S::ParserRangeOptional: return "self.range_optional";
        case S::ParserCounted: return "self.counted";
        case S::AssignCompatible: return "ispa_std.assign_compatible";
        // LR backend: runtime lives in ispa_std/lr.py (port of lr_machine.hpp / lr_conflicts.hpp).
        case S::LRParser: return "ispa_std.LRParser";
        case S::LRGeneralizedParser: return "ispa_std.LRGeneralizedParser";
        case S::LRParse: return "self.lr_parse";
        case S::LRParseGeneralized: return "self.lr_parse_generalized";
        case S::LRGet: return "ispa_std.lr_get";
        case S::LRToken: return "ispa_std.lr_token";
        case S::LRPack: return "ispa_std.lr_pack";
        case S::LRNode: return "ispa_std.lr_node";
        case S::LRCaptureAssign: return "ispa_std.lr_capture_assign";
    }
    unsupported("StdlibExports");
}
// Recovery::Tokens<A, B, ...> / EmptyTokens -> a Python tuple of token ids.
std::string Core::recovery_set(const LangAPI::Type& set) {
    if (!set.isSymbol()) throw std::runtime_error("Python target: invalid recovery token set");
    const auto name = symbol(set.getSymbol());
    if (name != "ISPA_STD.Recovery.Tokens" && name != "ISPA_STD.Recovery.EmptyTokens")
        throw std::runtime_error("Python target: unsupported recovery token set");
    std::string out = "(";
    std::size_t values = 0;
    for (const auto& item : set.template_parameters) {
        std::string text = std::holds_alternative<LangAPI::Type>(item)
            ? type(std::get<LangAPI::Type>(item))
            : rvalue(std::get<LangAPI::RValue>(item));
        if (text == "ISPA_STD.Recovery.EndOfInput") text = "ispa_std.EndOfInput";
        if (values++) out += ", ";
        out += text;
    }
    if (values == 1) out += ",";
    return out + ")";
}
// Recovery::Policy<AllowInsertion, AllowDeletion, AllowReplacement, MaxSkip> -> (bool, bool, bool, int)
std::string Core::recovery_policy(const LangAPI::Type& policy) {
    if (!policy.isSymbol() || symbol(policy.getSymbol()) != "ISPA_STD.Recovery.Policy")
        throw std::runtime_error("Python target: unsupported recovery policy");
    std::vector<std::string> values;
    for (const auto& item : policy.template_parameters)
        values.push_back(std::holds_alternative<LangAPI::Type>(item)
            ? type(std::get<LangAPI::Type>(item))
            : rvalue(std::get<LangAPI::RValue>(item)));
    if (values.size() > 4) throw std::runtime_error("Python target: too many recovery policy arguments");
    static const char* const defaults[] = {"True", "True", "True", "64"};
    for (std::size_t i = values.size(); i < 4; ++i) values.emplace_back(defaults[i]);
    return "(" + join(values, ", ", [](const std::string& v) { return v; }) + ")";
}
std::string Core::call(const LangAPI::FunctionCall& c) {
    std::string fn = std::visit(overloaded{
        [this](const std::shared_ptr<LangAPI::Symbol>& name) { return symbol(*name); },
        [this](const std::shared_ptr<LangAPI::IspaLibSymbol>& name) { return lib(*name); }
    }, c.name);
    const auto as_type = [](const auto& item) -> const LangAPI::Type& {
        const auto* t = std::get_if<std::shared_ptr<LangAPI::Type>>(&item);
        if (!t || !*t) throw std::runtime_error("Python target: invalid recovery template argument");
        return **t;
    };
    if (fn == "recover_choice") {
        if (c.args.size() != 3 || c.template_parameters.size() % 2 != 0)
            throw std::runtime_error("Python target: invalid recovery choice");
        std::string alternatives = "[";
        for (const auto& parameter : c.template_parameters) {
            if (alternatives.size() > 1) alternatives += ", ";
            alternatives += recovery_set(as_type(parameter));
        }
        alternatives += "]";
        return "self.recover_choice(" + expression(c.args[0]) + ", " + expression(c.args[1]) +
            ", " + expression(c.args[2]) + ", " + alternatives + ")";
    }
    // recovery_at<Sets...>(expecting, names...): the claim sets become a list of token-id tuples.
    if (fn == "recovery_at" || fn == "self.recovery_at") {
        if (c.args.size() != c.template_parameters.size() + 1)
            throw std::runtime_error("Python target: recovery_at needs an expectation and one name per claim set");
        std::string sets = "[";
        for (std::size_t i = 0; i < c.template_parameters.size(); ++i) {
            if (i) sets += ", ";
            sets += recovery_set(as_type(c.template_parameters[i]));
        }
        sets += "]";
        std::string out = "self.recovery_at(" + sets;
        for (const auto& arg : c.args) out += ", " + expression(arg);
        return out + ")";
    }
    // Recovery::handler<Expected, Continuation, Follow, Synchronization, Policy>(rule, expected_text)
    if (fn == "ISPA_STD.Recovery.handler") {
        if (c.template_parameters.size() != 5 || c.args.size() != 2)
            throw std::runtime_error("Python target: invalid recovery handler");
        return "ispa_std.Recovery.handler(" +
            recovery_set(as_type(c.template_parameters[0])) + ", " +
            recovery_set(as_type(c.template_parameters[1])) + ", " +
            recovery_set(as_type(c.template_parameters[2])) + ", " +
            recovery_set(as_type(c.template_parameters[3])) + ", " +
            recovery_policy(as_type(c.template_parameters[4])) + ", " +
            expression(c.args[0]) + ", " + expression(c.args[1]) + ")";
    }
    if (fn == "ISPA_STD.external_match") {
        if (c.template_parameters.size() != 1 || c.args.size() != 1)
            throw std::runtime_error("Python target: invalid external parser call");
        const auto* binding = std::get_if<std::shared_ptr<LangAPI::Type>>(&c.template_parameters.front());
        if (!binding || !*binding) throw std::runtime_error("Python target: invalid external parser binding");
        return "ispa_std.external_match(" + type(**binding)
            + ", " + expression(c.args.front()) + ")";
    }
    return fn + "(" + join(c.args, ", ", [this](const auto& p) { return expression(p); }) + ")";
}
std::string Core::lib_call(const LangAPI::IspaLibFunctionCall& c) {
    std::string fn = lib(c.symbol);
    using S = LangAPI::StdlibExports;
    const auto kind = c.symbol.exports;
    const bool parser_match = kind == S::ParserRequired || kind == S::ParserOptional ||
        kind == S::ParserRange || kind == S::ParserRangeOptional || kind == S::ParserCounted;
    std::vector<std::string> args;
    args.reserve(c.args.size() + (parser_match ? 1 : 0));
    for (std::size_t i = 0; i < c.args.size(); ++i) {
        const auto& arg = c.args[i];
        const bool callback = parser_match && i > 0 && arg.size() == 1 &&
            arg.front().isRvalue() && arg.front().getRValue().isSymbol();
        if (callback) {
            std::string name = expression(arg);
            const auto dot = name.rfind('.');
            if (dot != std::string::npos) name = name.substr(dot + 1);
            args.emplace_back("lambda _ispa_cursor: self." + name + "(_ispa_cursor)");
        } else args.push_back(expression(arg));
    }
    // C++ required<Node>(pos, fail) / required<Node>(pos, rec, fail)
    // C++ counted<Node>(pos, spec, [rec])
    // Insert the erased Node type at the position expected by the Python API.
    if (parser_match) {
        if (c.template_parameters.empty())
            throw std::runtime_error("Python target: parser primitive is missing NodeStruct template argument");
        const auto& first = c.template_parameters.front();
        const auto expected = std::holds_alternative<std::shared_ptr<LangAPI::Type>>(first)
            ? type(*std::get<std::shared_ptr<LangAPI::Type>>(first))
            : rvalue(*std::get<std::shared_ptr<LangAPI::RValue>>(first));
        const auto target_index = (kind == S::ParserCounted) ? 2u : 1u;
        args.insert(args.begin() + std::min((std::size_t)target_index, args.size()), expected);
    }
    // C++ lr_parse<Generated>(pos): the template argument is the generated parser class itself.
    if (kind == S::LRParse || kind == S::LRParseGeneralized) {
        if (c.template_parameters.empty())
            throw std::runtime_error("Python target: lr_parse is missing its generated parser template argument");
        const auto& first = c.template_parameters.front();
        args.insert(args.begin(), std::holds_alternative<std::shared_ptr<LangAPI::Type>>(first)
            ? type(*std::get<std::shared_ptr<LangAPI::Type>>(first))
            : rvalue(*std::get<std::shared_ptr<LangAPI::RValue>>(first)));
    }
    return fn + "(" + join(args, ", ", [](const std::string& arg) { return arg; }) + ")";
}

std::string Core::lambda(const LangAPI::Lambda& fn) {
    std::vector<std::string> names;
    for (const auto& p : fn.parameters) names.push_back(identifier(p.second));  // same escaping as the body uses
    const std::string params = join(names, ", ", [](const auto& n) { return n; });
    if (fn.statements.empty()) return "lambda " + params + ": None";
    if (!statement_writer_) throw std::runtime_error("Python target: missing lambda statement writer");
    const auto name = "_ispa_lambda_" + std::to_string(++lambda_number_);
    // Delay emission until the enclosing statement is emitted: a Python `def`
    // can contain arbitrary control flow while `lambda` cannot.
    pending_lambdas_.push_back({name, params, statement_writer_(fn.statements, names)});
    return name;
}

std::string Core::element(LangAPI::ExpressionElement value, bool real_division) {
    using E = LangAPI::ExpressionElement;
    switch (value) {
        case E::GroupOpen: return "(";
        case E::GroupClose: return ")";
        case E::SquareBraceOpen: return "[";
        case E::SquareBraceClose: return "]";
        case E::And: return " and ";
        case E::Or: return " or ";
        case E::Not: return "not ";
        case E::Equal: return " == ";
        case E::NotEqual: return " != ";
        case E::Higher: return " > ";
        case E::Lower: return " < ";
        case E::HigherOrEqual: return " >= ";
        case E::LowerOrEqual: return " <= ";
        case E::Add: return " + ";
        case E::Minus: return " - ";
        case E::Multiply: return " * ";
        case E::Divide: return real_division ? " / " : " // ";
        case E::Modulo: return " % ";
        case E::PlusPlus:
        case E::MinusMinus: unsupported("prefix/postfix increment expression; use statement lowering");
    }
    unsupported("ExpressionElement");
}
std::string Core::rvalue(const LangAPI::RValue& v) {
    using R = LangAPI::RValueType;
    switch (v.type()) {
        case R::Undef: return "None";
        case R::Char: {
            const auto& c = v.getChar();
            if (c.escaped) {
                if (c.value == 'n') return "'\\n'";
                if (c.value == 't') return "'\\t'";
                if (c.value == 'r') return "'\\r'";
                if (c.value == '0') return "'\\x00'";
            }
            return literal(std::string(1, c.value));
        }
        case R::Int: return std::to_string(v.getInt().value);
        case R::Bool: return v.getBool().value ? "True" : "False";
        case R::Float: return float_literal(v.getFloat().value);
        case R::String: return literal(v.getString().value);
        case R::Array: return "[" + join(v.getArray().values, ", ", [this](const auto& e) { return expression(e); }) + "]";
        case R::FixedSizeArray: return "[" + join(v.getFixedSizeArray().values, ", ", [this](const auto& e) { return expression(e); }) + "]";
        case R::Map: {
            const auto& map = v.getMap();
            std::string out = "{";
            for (std::size_t i = 0; i < map.keys.size(); ++i) {
                if (i) out += ", ";
                if (const auto* name = std::get_if<LangAPI::String>(&map.keys[i])) out += literal(name->value);
                else out += std::to_string(std::get<LangAPI::Int>(map.keys[i]).value);
                out += ": " + expression(map.values[i]);
            }
            return out + "}";
        }
        case R::Pos: {
            const auto& p = v.getPos();
            auto out = counter();
            if (p.offset) out += " + (" + std::to_string(p.offset) + ")";
            return p.dereference ? ("(" + counter() + ").peek(" + std::to_string(p.offset) + ")") : out;
        }
        case R::Symbol: return symbol(v.getSymbol());
        case R::StorageSymbol: return storage(v.getStorageSymbol());
        case R::IspaLibSymbol: return lib(v.getIspaSymbol());
        case R::Inheritance: {
            const auto& inh = v.getInheritance();
            const auto ctor = std::visit(overloaded{
                [this](const LangAPI::Symbol& s) { return symbol(s); },
                [this](const LangAPI::IspaLibSymbol& s) { return lib(s); },
                [this](const std::shared_ptr<LangAPI::Type>& t) { return type(*t); }
            }, inh.name);
            const auto args = join(inh.args, ", ", [this](const auto& e) { return expression(e); });
            return (ctor == "ispa_std.Node" ? "ispa_std.Node.from_cpp" : ctor) + "(" + args + ")";
        }
        case R::Reference: return rvalue(*v.getReference().value);
        case R::Span: return symbol(v.getSpan().sym);
        case R::IspaLibDfaSpan: return symbol(v.getIspaLibDfaSpan().assing_name);
        case R::MakeTuple: {
            const auto& a = v.getMakeTuple().args;
            return "(" + join(a, ", ", [this](const auto& e) { return expression(e); }) + (a.size() == 1 ? "," : "") + ")";
        }
        case R::GetVariant: {
            const auto& cast = v.getVariantCast();
            return "ispa_std.variant_get(" + expression(cast.sym) + ", " + type(*cast.type) + ")";
        }
        case R::CheckVariant: {
            const auto& cast = v.CheckVariantCast();
            return "ispa_std.variant_holds(" + expression(cast.sym) + ", " + type(*cast.type) + ")";
        }
        case R::ToString: return "str(" + expression(v.getToString().what) + ")";
        case R::CharToStringConstructor: return "str(" + expression(v.getCharToStringConstructor().what) + ")";
        case R::Cast: return type(*v.getCast().type) + "(" + expression(v.getCast().what) + ")";
        case R::GetFromBox: return expression(v.getGetFromBox().what);
    }
    unsupported("RValueType");
}
std::string Core::expression(const LangAPI::Expression& e) {
    std::string out;
    const bool real_division = has_float(e);
    // Value-producing ++/--: a postfix operator rewrites the operand rendered just before it,
    // a prefix operator remembers its delta until the next operand has been rendered.
    std::size_t operand_begin = std::string::npos;
    int pending_prefix = 0;
    for (std::size_t index = 0; index < e.size(); ++index) {
        const auto& part = e[index];
        using E = LangAPI::ExpressionValueType;
        if (part.type() == E::ExpressionElement &&
            (part.getExpressionElement() == LangAPI::ExpressionElement::PlusPlus ||
             part.getExpressionElement() == LangAPI::ExpressionElement::MinusMinus)) {
            const int delta = part.getExpressionElement() == LangAPI::ExpressionElement::PlusPlus ? 1 : -1;
            if (operand_begin != std::string::npos) {
                const auto operand = out.substr(operand_begin);
                if (is_name(operand)) note_assigned(operand);
                out.erase(operand_begin);
                out += step_value(operand, delta, true);
                operand_begin = std::string::npos;
            } else pending_prefix = delta;
            continue;
        }
        const auto before = out.size();
        switch (part.type()) {
            case E::Empty: break;
            case E::RValue: out += rvalue(part.getRValue()); break;
            case E::EmptyInitializer: out += "None"; break;
            case E::ExpressionElement: out += element(part.getExpressionElement(), real_division); break;
            case E::FunctionCall: out += call(part.getFunctionCall()); break;
            case E::Return: unsupported("return inside expression");
            case E::Break: unsupported("break inside expression");
            case E::Continue: unsupported("continue inside expression");
            case E::VariableAssignment: {
                // `while ((c = next()) != x)` and friends: an assignment used as a value.
                const auto& var = part.getVariableAssignment();
                const auto lhs = std::holds_alternative<LangAPI::Symbol>(var.name)
                    ? symbol(std::get<LangAPI::Symbol>(var.name))
                    : storage(std::get<LangAPI::StorageSymbol>(var.name));
                if (!is_name(lhs)) unsupported("assignment used as a value on a non-local target");
                note_assigned(lhs);
                const auto rhs = expression(var.value);
                const char* op = nullptr;
                switch (var.type) {
                    case LangAPI::OperatorType::Assign: break;
                    case LangAPI::OperatorType::Add: op = " + "; break;
                    case LangAPI::OperatorType::Minus: op = " - "; break;
                    case LangAPI::OperatorType::Multiply: op = " * "; break;
                    case LangAPI::OperatorType::Divide: op = has_float(var.value) ? " / " : " // "; break;
                    case LangAPI::OperatorType::Modulo: op = " % "; break;
                }
                out += "(" + lhs + " := " + (op ? lhs + op + "(" + rhs + ")" : rhs) + ")";
                break;
            }
            case E::CounterIncreament: unsupported("counter increment inside expression");
            case E::CounterAssignment: unsupported("counter assignment inside expression");
            case E::ResetPosCounter: unsupported("counter stack reset inside expression");
            case E::PushPosCounter: unsupported("counter stack push inside expression");
            case E::PopPosCounter: unsupported("counter stack pop inside expression");
            case E::SkipSpaces: out += "self.skip_spaces(" + counter() + ")"; break;
            case E::DfaLookup: unsupported("DFA lookup requires class/action table lowering");
            case E::ReportError: out += "self.raise_error(" + literal(part.getReportError().message) + ")"; break;
            case E::Lambda: out += lambda(part.getLambda()); break;
            case E::IspaLibFunctionCall: out += lib_call(part.getIspaLibFunctionCall()); break;
            case E::DFADebug: std::cout << "Note: skipping DFADebug" << std::endl; break;
            case E::StringCompare: {
                const auto compare = part.getStringCompare();
                if (compare.is_string) {
                    const auto value = literal(compare.str.value);
                    out += "(" + counter() + ".source[" + counter() + ".index:" + counter() +
                           ".index + " + std::to_string(compare.str.value.size()) + "] == " + value + ")";
                } else {
                    out += "(" + counter() + ".peek() == " + literal(compare.str.value) + ")";
                }
                break;
            }
            default: unsupported("ExpressionValueType");
        }
        if (pending_prefix != 0) {
            if (part.type() != E::RValue) unsupported("prefix ++/-- without an assignable operand");
            const auto operand = out.substr(before);
            if (is_name(operand)) note_assigned(operand);
            out.erase(before);
            out += step_value(operand, pending_prefix, false);
            pending_prefix = 0;
            operand_begin = std::string::npos;
        } else operand_begin = part.type() == E::RValue ? before : std::string::npos;
    }
    if (pending_prefix != 0) unsupported("dangling prefix ++/--");
    return out;
}
std::string Core::statement_expression(const LangAPI::Expression& e) {
    if (e.size() == 1) {
        const auto& first = e.front();
        using E = LangAPI::ExpressionValueType;
        switch (first.type()) {
            case E::IspaLibFunctionCall: {
                const auto& call = first.getIspaLibFunctionCall();
                if (call.symbol.exports == LangAPI::StdlibExports::LRCaptureAssign && call.args.size() == 4) {
                    // C++ takes the destination by reference; Python returns the converted value.
                    const auto lhs = expression(call.args[0]);
                    if (is_name(lhs)) note_assigned(lhs);
                    return lhs + " = ispa_std.lr_capture_assign(" + lhs + ", " + expression(call.args[1]) + ", " +
                           expression(call.args[2]) + ", " + expression(call.args[3]) + ")";
                }
                if (call.symbol.exports == LangAPI::StdlibExports::AssignCompatible && call.args.size() == 2) {
                    const auto lhs = expression(call.args[0]);
                    return lhs + " = ispa_std.assign_compatible(" + lhs + ", " + expression(call.args[1]) + ")";
                }
                break;
            }
            case E::Return: return "return " + expression(first.getReturn().value);
            case E::Break: return "break";
            case E::Continue: return "continue";
            case E::VariableAssignment: {
                const auto& var = first.getVariableAssignment();
                const auto lhs = std::holds_alternative<LangAPI::Symbol>(var.name)
                    ? symbol(std::get<LangAPI::Symbol>(var.name))
                    : storage(std::get<LangAPI::StorageSymbol>(var.name));
                const char* op = "=";
                switch (var.type) {
                    case LangAPI::OperatorType::Assign: op = "="; break;
                    case LangAPI::OperatorType::Add: op = "+="; break;
                    case LangAPI::OperatorType::Minus: op = "-="; break;
                    case LangAPI::OperatorType::Multiply: op = "*="; break;
                    case LangAPI::OperatorType::Divide: op = has_float(var.value) ? "/=" : "//="; break;
                    case LangAPI::OperatorType::Modulo: op = "%="; break;
                }
                if (is_name(lhs)) note_assigned(lhs);
                return lhs + " " + op + " " + expression(var.value);
            }
            case E::CounterIncreament: note_assigned(counter()); return counter() + " += 1";
            case E::CounterAssignment: note_assigned(counter()); return counter() + " = " + expression(*first.getCounterAssignment().v);
            case E::PushPosCounter: {
                const auto& p = first.getPushPosCounter();
                const auto current = counter();
                counters_.push_back(p.name);
                declare(p.name);  // a fresh local of this def, never an outer binding
                return p.name + " = " + current + ".copy()";
            }
            case E::PopPosCounter: {
                if (counters_.size() < 2) unsupported("unbalanced counter pop");
                const auto previous = counters_[counters_.size() - 2];
                const auto child = counter();
                counters_.pop_back();
                return previous + ".index = " + child + ".index";
            }
            case E::ResetPosCounter: {
                if (counters_.size() < 2) unsupported("unbalanced counter reset");
                counters_.pop_back();
                return "pass";
            }
            case E::DfaLookup: {
                const auto& dfa = first.getDfaLookup();
                return dfa.output_name + " = self.lookup_dfa(" + std::to_string(dfa.dfa_count) + ", " + counter() + ")";
            }
            case E::ReportError: return "raise RuntimeError(" + literal(first.getReportError().message) + ")";
            default: break;
        }
    }
    return expression(e);
}
} // namespace Python
