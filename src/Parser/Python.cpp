module Parser.Base;
import UAST.PyAST;
import std;

namespace Parser {
namespace {
namespace py = UAST::py;
namespace la = LangAPI;
// Dots occur only in synthetic names bound by Python.Imports. Keep namespace
// paths in the language-neutral symbol rather than embedding C++ spelling.
la::Symbol pythonSymbol(const std::string& name) {
    la::Symbol result;
    std::size_t begin = 0;
    do {
        const auto end = name.find('.', begin);
        result.path.emplace_back(name.substr(begin, end - begin));
        if (end == std::string::npos) break;
        begin = end + 1;
    } while (true);
    return result;
}
[[noreturn]] void unsupported(const UAST::SourcePosition& p, std::string_view what) {
    throw std::invalid_argument(p.file + ":" + std::to_string(p.line) + ":" +
        std::to_string(p.column) + ": Python conversion: " + std::string(what));
}
// Decode source syntax at the source-language boundary. LangAPI stores values.
std::string decodedString(std::string_view source, const UAST::SourcePosition& position) {
    std::string result;
    const auto unicode = [&](unsigned value) {
        if (value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff))
            unsupported(position, "string contains a non-scalar Unicode escape");
        if (value < 0x80) result += static_cast<char>(value);
        else if (value < 0x800) {
            result += static_cast<char>(0xc0 | (value >> 6));
            result += static_cast<char>(0x80 | (value & 0x3f));
        } else if (value < 0x10000) {
            result += static_cast<char>(0xe0 | (value >> 12));
            result += static_cast<char>(0x80 | ((value >> 6) & 0x3f));
            result += static_cast<char>(0x80 | (value & 0x3f));
        } else {
            result += static_cast<char>(0xf0 | (value >> 18));
            result += static_cast<char>(0x80 | ((value >> 12) & 0x3f));
            result += static_cast<char>(0x80 | ((value >> 6) & 0x3f));
            result += static_cast<char>(0x80 | (value & 0x3f));
        }
    };
    for (std::size_t i = 0; i < source.size(); ++i) {
        if (source[i] != '\\') { result += source[i]; continue; }
        if (++i == source.size()) unsupported(position, "dangling string escape");
        const char escaped = source[i];
        switch (escaped) {
            case '\\': case '\'': case '"': result += escaped; break;
            case 'n': result += '\n'; break;
            case 'r': result += '\r'; break;
            case 't': result += '\t'; break;
            case 'a': result += '\a'; break;
            case 'b': result += '\b'; break;
            case 'f': result += '\f'; break;
            case 'v': result += '\v'; break;
            case '\n': break;
            case 'x': case 'u': case 'U': {
                const std::size_t width = escaped == 'x' ? 2 : escaped == 'u' ? 4 : 8;
                unsigned value = 0;
                for (std::size_t j = 0; j < width; ++j) {
                    if (++i == source.size()) unsupported(position, "truncated string escape");
                    const unsigned char c = source[i];
                    if (!std::isxdigit(c)) unsupported(position, "invalid hexadecimal string escape");
                    value = (value << 4) | (std::isdigit(c) ? c - '0' : std::tolower(c) - 'a' + 10);
                }
                unicode(value);
                break;
            }
            case 'N': unsupported(position, "named Unicode escapes need source-language decoding");
            default:
                if (escaped >= '0' && escaped <= '7') {
                    unsigned value = escaped - '0';
                    for (int j = 0; j < 2 && i + 1 < source.size() && source[i + 1] >= '0' && source[i + 1] <= '7'; ++j)
                        value = value * 8 + source[++i] - '0';
                    unicode(value);
                } else { result += '\\'; result += escaped; }
        }
    }
    return result;
}
struct PythonConverter {
    std::set<std::string> locals;
    bool action_context = false, failure_context = false;
    std::string self_name;
    std::string token_rule;
    std::set<std::string> class_names, objects;
    std::map<std::string, la::Type> local_types;
    std::set<std::string> debug_ready;
    bool debug_ready_initialized = false;
    std::size_t debug_line_cursor = 0;
    std::function<la::Statements(const py::IspaMatch&, std::set<std::string>&)> embedded_match;
    std::string localName(const std::string& name) const {
        return action_context && locals.contains(name) ? "ispa_action_" + name : name;
    }
    std::string tokenName(const py::ExprPtr& value) const {
        if (!value) return {};
        if (auto name = value->template get<py::Name>())
            return locals.contains(name->id) || objects.contains(name->id) ||
                class_names.contains(name->id) || name->id == "ctx" ? "" : name->id;
        if (auto attribute = value->template get<py::Attribute>()) {
            auto prefix = tokenName(attribute->value);
            return prefix.empty() ? "" : prefix + "." + attribute->attr;
        }
        return {};
    }
    std::size_t block_depth = 0, loop_depth = 0;
    // Python locals are function-scoped; the targets' are block-scoped. A first assignment inside a block
    // is declared there when the block alone uses the name, otherwise it is hoisted to the function top.
    const py::StmtList* function_body = nullptr;
    std::vector<const py::StmtList*> block_lists;
    std::vector<std::vector<std::string>> block_declared;
    // Names first assigned in a block (declared there or hoisted): not definitely assigned after it.
    std::vector<std::vector<std::string>> block_assigned;
    std::vector<std::pair<std::string, la::Type>> hoisted;
    template<class T> static void count_name(const T& value, const std::string& name, std::size_t& total) {
        if constexpr (std::is_same_v<T, py::Expr>) {
            if (auto n = value.template get<py::Name>(); n && n->id == name) ++total;
            count_name(value.kind, name, total);
        } else if constexpr (requires { std::visit([](const auto&) {}, value); }) {
            std::visit([&](const auto& item) { count_name(item, name, total); }, value);
        } else if constexpr (requires { value.get(); *value; }) {
            if (value) count_name(*value, name, total);
        } else if constexpr (requires { value.members(); }) {
            std::apply([&](const auto&... items) { (count_name(items, name, total), ...); }, value.members());
        } else if constexpr (std::ranges::range<T> && !std::is_same_v<T, std::string>) {
            for (const auto& item : value) count_name(item, name, total);
        }
    }
    bool escapes_block(const std::string& name) const {
        if (!function_body || block_lists.empty()) return false;
        std::size_t everywhere = 0, inside = 0;
        count_name(*function_body, name, everywhere);
        count_name(*block_lists.back(), name, inside);
        return everywhere > inside;
    }
    la::Statements nested(const py::StmtList& list, bool loop = false) {
        ++block_depth;
        loop_depth += loop;
        block_lists.push_back(&list);
        block_declared.emplace_back();
        block_assigned.emplace_back();
        auto out = body(list);
        for (const auto& name : block_assigned.back()) debug_ready.erase(name);
        block_assigned.pop_back();
        for (const auto& name : block_declared.back()) {
            locals.erase(name);
            local_types.erase(name);
            objects.erase(name);
        }
        block_declared.pop_back();
        block_lists.pop_back();
        loop_depth -= loop;
        --block_depth;
        return out;
    }
    la::Expression expr(const py::ExprPtr& p) {
        if (!p) throw std::invalid_argument("Python conversion: missing expression");
        return expr(*p);
    }
    la::Expression group(la::Expression e) {
        e.insert(e.begin(), la::ExpressionElement::GroupOpen);
        e.emplace_back(la::ExpressionElement::GroupClose);
        return e;
    }
    la::Expression binary(la::Expression left, la::ExpressionElement op, la::Expression right) {
        auto out = group(std::move(left));
        out.emplace_back(op);
        auto rhs = group(std::move(right));
        out.insert(out.end(), rhs.begin(), rhs.end());
        return group(std::move(out));
    }
    la::ExpressionElement binop(py::BinOp op, const UAST::SourcePosition& p) {
        switch (op) {
            case py::BinOp::Add: return la::ExpressionElement::Add;
            case py::BinOp::Sub: return la::ExpressionElement::Minus;
            case py::BinOp::Mult: return la::ExpressionElement::Multiply;
            // `//` lowers to the typed Divide: exact for the non-negative ints the runtime uses.
            case py::BinOp::Div: case py::BinOp::FloorDiv: return la::ExpressionElement::Divide;
            case py::BinOp::Mod: return la::ExpressionElement::Modulo;
            default: unsupported(p, "operator has no LangAPI representation");
        }
    }
    la::Type type(const py::ExprPtr& p) {
        if (!p) return la::Type{la::ValueType::Undef};
        if (auto n = p->template get<py::Name>()) {
            if (n->id == "int") return la::Type{la::ValueType::Int};
            if (n->id == "float") return la::Type{la::ValueType::Float};
            if (n->id == "bool") return la::Type{la::ValueType::Bool};
            if (n->id == "str") return la::Type{la::ValueType::String};
            return la::Type{pythonSymbol(n->id)};
        }
        if (auto sub = p->template get<py::Subscript>(); sub && sub->value) {
            auto name = sub->value->template get<py::Name>();
            if (name && name->id == "list") return la::Type{la::ValueType::Array, type(sub->slice)};
            if (name && name->id == "dict") {
                auto args = sub->slice ? sub->slice->template get<py::Tuple>() : nullptr;
                if (!args || args->elts.size() != 2) unsupported(p->pos, "dict requires key and value types");
                return la::Type{la::ValueType::Map, type(args->elts[0]), type(args->elts[1])};
            }
            if (name && (name->id == "Callable" || name->id == "typing.Callable")) {
                auto args = sub->slice ? sub->slice->template get<py::Tuple>() : nullptr;
                auto params = args && args->elts.size() == 2 && args->elts[0] ? args->elts[0]->template get<py::List>() : nullptr;
                if (!params) unsupported(p->pos, "Callable requires [[parameter types], return type]");
                la::Type result{la::ValueType::Callable};
                for (const auto& parameter : params->elts) result.template_parameters.emplace_back(type(parameter));
                result.template_parameters.emplace_back(type(args->elts[1]));
                return result;
            }
            if (name && (name->id.starts_with("ISPA_STD.") || name->id.starts_with("__ispa_parser__."))) {
                la::Type result{pythonSymbol(name->id)};
                if (auto args = sub->slice ? sub->slice->template get<py::Tuple>() : nullptr) {
                    for (const auto& arg : args->elts) result.template_parameters.emplace_back(type(arg));
                } else result.template_parameters.emplace_back(type(sub->slice));
                return result;
            }
        }
        if (auto c = p->template get<py::Constant>(); c && c->kind == py::ConstantKind::None)
            return la::Type{la::ValueType::Void};
        unsupported(p->pos, "unsupported type annotation");
    }
    bool object(const py::ExprPtr& value) const {
        if (!value) return false;
        if (auto n = value->template get<py::Name>()) return objects.contains(n->id) || class_names.contains(n->id) || (action_context && n->id == "ctx");
        if (auto c = value->template get<py::Call>(); c && c->func)
            if (auto n = c->func->template get<py::Name>()) return class_names.contains(n->id);
        return false;
    }
    la::Type inferred(const py::ExprPtr& value) {
        if (!value) return la::Type{la::ValueType::Undef};
        if (auto n = value->template get<py::Name>()) {
            if (auto found = local_types.find(n->id); found != local_types.end()) return found->second;
        }
        if (auto c = value->template get<py::Constant>()) {
            switch (c->kind) {
                case py::ConstantKind::Int: return la::Type{la::ValueType::Int};
                case py::ConstantKind::Float: return la::Type{la::ValueType::Float};
                case py::ConstantKind::Bool: return la::Type{la::ValueType::Bool};
                case py::ConstantKind::Str: return la::Type{la::ValueType::String};
                default: break;
            }
        }
        const auto is = [](const la::Type& t, la::ValueType v) { return t.isValueType() && t.getValueType() == v; };
        if (auto b = value->template get<py::BinaryExpr>()) {
            const auto l = inferred(b->left), r = inferred(b->right);
            if (is(l, la::ValueType::Int) && is(r, la::ValueType::Int)) return l;
            if ((is(l, la::ValueType::Float) || is(l, la::ValueType::Int)) &&
                (is(r, la::ValueType::Float) || is(r, la::ValueType::Int))) return la::Type{la::ValueType::Float};
            if (b->op == py::BinOp::Add && l == r && (is(l, la::ValueType::String) || is(l, la::ValueType::Array))) return l;
            return la::Type{la::ValueType::Undef};
        }
        if (auto u = value->template get<py::UnaryExpr>())
            return u->op == py::UnaryOp::Not ? la::Type{la::ValueType::Bool} : inferred(u->operand);
        if (value->template get<py::Compare>()) return la::Type{la::ValueType::Bool};
        if (auto b = value->template get<py::BoolExpr>()) {
            for (const auto& item : b->values) if (!is(inferred(item), la::ValueType::Bool)) return la::Type{la::ValueType::Undef};
            return la::Type{la::ValueType::Bool};
        }
        if (auto c = value->template get<py::IfExpr>()) {
            const auto l = inferred(c->body);
            return l == inferred(c->orelse) ? l : la::Type{la::ValueType::Undef};
        }
        if (auto sub = value->template get<py::Subscript>()) {
            const auto base = inferred(sub->value);
            if (is(base, la::ValueType::Array) && !base.template_parameters.empty())
                if (const auto* element = std::get_if<la::Type>(&base.template_parameters.front())) return *element;
        }
        if (auto list = value->template get<py::List>(); list && !list->elts.empty())
            return la::Type{la::ValueType::Array, inferred(list->elts.front())};
        if (auto dict = value->template get<py::Dict>(); dict && !dict->keys.empty())
            return la::Type{la::ValueType::Map, inferred(dict->keys.front()), inferred(dict->values.front())};
        return la::Type{la::ValueType::Undef};
    }
    la::Expression initializer(const py::ExprPtr& value, const la::Type& expected) {
        auto result = expr(value);
        if (result.size() == 1 && result.front().isRvalue()) {
            auto& literal = result.front().getRValue();
            if (literal.isArray() || literal.isMap()) {
                decltype(la::Array::template_parameters) params;
                for (const auto& item : expected.template_parameters) if (auto t = std::get_if<la::Type>(&item))
                    params.push_back(std::make_shared<la::Type>(*t));
                if (literal.isArray()) literal.getArray().template_parameters = params;
                else literal.getMap().template_parameters = params;
            }
        }
        return result;
    }
    std::variant<la::Symbol, la::StorageSymbol> target(const py::ExprPtr& p) {
        if (action_context) {
            const py::Expr* root = p.get();
            const py::Attribute* property = nullptr;
            while (root) {
                if (auto a = root->template get<py::Attribute>()) { property = a; root = a->value.get(); }
                else if (auto s = root->template get<py::Subscript>()) root = s->value.get();
                else break;
            }
            if (root) if (auto n = root->template get<py::Name>(); n && n->id == "ctx") {
                if (!property || (token_rule.empty() ? property->attr != "pos" && property->attr != "ast" :
                    property->attr != "ast" && property->attr != "shared"))
                    unsupported(p->pos, token_rule.empty() ?
                        "ctx and its metadata are read-only; only ctx.pos and ctx.ast are writable" :
                        "token ctx metadata are read-only; only ctx.ast and ctx.shared fields are writable");
            }
        }
        auto e = expr(p);
        if (e.size() == 1 && e.front().isRvalue()) {
            auto& r = e.front().getRValue();
            if (r.isSymbol()) return r.getSymbol();
            if (r.isStorageSymbol()) return r.getStorageSymbol();
        }
        unsupported(p->pos, "assignment target must be a name, attribute or subscript");
    }
    la::Expression expr(const py::Expr& e) {
        return std::visit([&](const auto& x) -> la::Expression {
            using T = std::decay_t<decltype(x)>;
            if constexpr (std::is_same_v<T, py::Name>) {
                if (!token_rule.empty() && !failure_context && x.id == "ctx") unsupported(e.pos, "token context needs a property");
                return la::Symbol::createExpression(pythonSymbol(x.id == self_name ? "__ispa_python_self" : localName(x.id)));
            }
            else if constexpr (std::is_same_v<T, py::Constant>) {
                switch (x.kind) {
                    case py::ConstantKind::Bool: return la::Bool::createExpression(la::Bool{.value = x.value == "True"});
                    case py::ConstantKind::Int: {
                        auto text = x.value;
                        std::erase(text, '_');
                        std::size_t end;
                        int base = 10;
                        if (text.size() > 2 && text[0] == '0') {
                            switch (text[1]) {
                                case 'x': case 'X': base = 16; break;
                                case 'o': case 'O': base = 8; break;
                                case 'b': case 'B': base = 2; break;
                            }
                            if (base != 10) text.erase(0, 2);
                        }
                        long long value;
                        try { value = std::stoll(text, &end, base); }
                        catch (const std::exception&) { unsupported(e.pos, "integer literal is invalid or exceeds LangAPI range"); }
                        if (end != text.size()) unsupported(e.pos, "invalid integer literal");
                        return la::Int::createExpression(la::Int{.value = value});
                    }
                    case py::ConstantKind::Float: {
                        auto text = x.value;
                        std::erase(text, '_');
                        std::size_t end;
                        double value;
                        try { value = std::stod(text, &end); }
                        catch (const std::exception&) { unsupported(e.pos, "float literal is invalid or exceeds LangAPI range"); }
                        if (end != text.size()) unsupported(e.pos, "invalid float literal");
                        return la::Float::createExpression(la::Float{.value = value});
                    }
                    case py::ConstantKind::Str: {
                        return la::String::createExpression(la::String{.value = decodedString(x.value, e.pos)});
                    }
                    default: unsupported(e.pos, "None has no LangAPI value representation");
                }
            } else if constexpr (std::is_same_v<T, py::Attribute> || std::is_same_v<T, py::Subscript>) {
                if constexpr (std::is_same_v<T, py::Attribute>) {
                    if (!token_rule.empty() && !failure_context && x.value)
                        if (auto shared = x.value->template get<py::Attribute>(); shared && shared->attr == "shared" && shared->value)
                            if (auto ctx = shared->value->template get<py::Name>(); ctx && ctx->id == "ctx")
                                return la::Symbol::createExpression(la::Symbol{"ispa_shared_" + x.attr});
                    if (!token_rule.empty() && !failure_context && x.value) if (auto n = x.value->template get<py::Name>(); n && n->id == "ctx") {
                        if (x.attr == "ast") return la::Symbol::createExpression(la::Symbol{"ispa_token_result"});
                        if (x.attr == "start_pos") return la::Symbol::createExpression(la::Symbol{"start_pos"});
                        if (x.attr == "rule") return la::String::createExpression(la::String{.value = token_rule});
                        if (x.attr == "failed_rule") return la::String::createExpression(la::String{.value = token_rule});
                        if (x.attr == "text") return la::Symbol::createExpression(la::Symbol{"ispa_token_text"});
                        if (x.attr == "line_prefix") return la::FunctionCall::createExpression(la::FunctionCall{
                            .name = std::make_shared<la::Symbol>(la::Symbol{"__line_prefix"}),
                            .args = {la::Symbol::createExpression(la::Symbol{"start"})}});
                        if (x.attr == "next_char") return la::FunctionCall::createExpression(la::FunctionCall{
                            .name = std::make_shared<la::Symbol>(la::Symbol{"__next_char"}),
                            .args = {binary(la::Symbol::createExpression(la::Symbol{"start"}),
                                la::ExpressionElement::Add, la::Symbol::createExpression(la::Symbol{"length"}))}});
                        unsupported(e.pos, "unknown token ctx property: " + x.attr);
                    }
                    if (action_context && x.value) if (auto n = x.value->template get<py::Name>(); n && n->id == "ctx") {
                        if (x.attr != "pos" && x.attr != "ast" && x.attr != "start_pos" && x.attr != "rule" &&
                            !(failure_context && x.attr == "failed_rule"))
                            unsupported(e.pos, "unknown ctx property: " + x.attr);
                    }
                }
                la::StorageSymbol s;
                s.what = group(expr(x.value));
                if constexpr (std::is_same_v<T, py::Attribute>) s.path.emplace_back(x.attr);
                else s.path.emplace_back(la::StorageOffset{.offset = expr(x.slice)});
                return la::StorageSymbol::createExpression(s);
            } else if constexpr (std::is_same_v<T, py::Call>) {
                if (const auto* intrinsic = x.func ? x.func->template get<py::Name>() : nullptr;
                    intrinsic && intrinsic->id.starts_with("__ispa_ir__.")) {
                    const auto name = intrinsic->id.substr(12);
                    if (!x.keywords.empty()) unsupported(e.pos, "IR operations require positional arguments");
                    const std::map<std::string, std::pair<la::ArrayMethods, std::size_t>> collections{
                        {"size", {la::ArrayMethods::Size, 1}}, {"push", {la::ArrayMethods::Push, 2}},
                        {"extend", {la::ArrayMethods::Extend, 2}}, {"clear", {la::ArrayMethods::Clear, 1}}};
                    if (const auto found = collections.find(name); found != collections.end()) {
                        if (x.args.size() != found->second.second) unsupported(e.pos, "wrong IR operation arity");
                        la::StorageSymbol access;
                        access.what = expr(x.args[0]);
                        la::ArrayMethodCall call{.method = found->second.first};
                        for (std::size_t i = 1; i < x.args.size(); ++i) call.args.push_back(expr(x.args[i]));
                        access.path.emplace_back(std::move(call));
                        return la::StorageSymbol::createExpression(access);
                    }
                    const std::map<std::string, std::pair<la::StdlibExports, std::size_t>> streams{
                        {"token_cursor", {la::StdlibExports::TokenCursor, 1}},
                        {"lazy_token_cursor", {la::StdlibExports::LazyTokenCursor, 2}},
                        {"reset_token_stream", {la::StdlibExports::ResetTokenStream, 1}},
                        {"fail", {la::StdlibExports::Fail, 2}}, {"attempt", {la::StdlibExports::Attempt, 1}},
                        {"failure_message", {la::StdlibExports::FailureMessage, 0}}};
                    if (const auto found = streams.find(name); found != streams.end()) {
                        if (x.args.size() != found->second.second) unsupported(e.pos, "wrong IR operation arity");
                        la::IspaLibFunctionCall call{.symbol = la::IspaLibSymbol{.exports = found->second.first}};
                        for (const auto& argument : x.args) call.args.push_back(expr(argument));
                        return la::IspaLibFunctionCall::createExpression(call);
                    }
                    unsupported(e.pos, "unknown IR operation " + name);
                }
                if (!token_rule.empty() && !failure_context) if (auto method = x.func ? x.func->template get<py::Attribute>() : nullptr)
                    if (auto owner = method->value ? method->value->template get<py::Name>() : nullptr;
                        owner && owner->id == "ctx") {
                        if (method->attr == "skip" && x.args.empty() && x.keywords.empty()) {
                            return la::FunctionCall::createExpression(la::FunctionCall{
                                .name = std::make_shared<la::Symbol>(la::Symbol{"ispa_skip_current_token"})});
                        }
                        if (method->attr == "error" && x.args.size() == 1 && x.keywords.empty()) {
                            return la::FunctionCall::createExpression(la::FunctionCall{
                                .name = std::make_shared<la::Symbol>(la::Symbol{"ispa_token_error"}),
                                .args = {expr(x.args.front())}});
                        }
                        if (method->attr == "emit" && x.args.size() == 1 && x.keywords.empty()) {
                            auto literal = x.args.front() ? x.args.front()->template get<py::Constant>() : nullptr;
                            if (!literal || literal->kind != py::ConstantKind::Str || literal->value.empty() ||
                                !std::ranges::all_of(literal->value, [](unsigned char c) {
                                    return std::isalnum(c) || c == '_' || c == '.';
                                })) unsupported(e.pos, "ctx.emit requires a token-name string literal");
                            const auto scope_end = token_rule.rfind('.');
                            const auto qualified = literal->value.find('.') != std::string::npos || scope_end == std::string::npos ?
                                literal->value : token_rule.substr(0, scope_end + 1) + literal->value;
                            auto end = binary(la::Symbol::createExpression(la::Symbol{"start"}),
                                la::ExpressionElement::Add, la::Symbol::createExpression(la::Symbol{"length"}));
                            la::FunctionCall create;
                            create.name = std::make_shared<la::Symbol>(la::Symbol{"Token", "create"});
                            create.args = {binary(la::Symbol::createExpression(la::Symbol{"start_pos"}),
                                    la::ExpressionElement::Add, la::Symbol::createExpression(la::Symbol{"length"})),
                                end,
                                la::Int::createExpression(la::Int{.value = 0}),
                                la::FunctionCall::createExpression(la::FunctionCall{
                                    .name = std::make_shared<la::Symbol>(la::Symbol{"__line"}), .args = {end}}),
                                la::Symbol::createExpression(pythonSymbol("Tokens." + qualified + ".id")),
                                la::Inheritance::createExpression(la::Inheritance{.name = pythonSymbol("Types." + qualified)})};
                            la::StorageSymbol queue;
                            queue.what = la::Symbol::createExpression(la::Symbol{"ispa_pending_tokens"});
                            queue.path.emplace_back(la::FunctionCall{
                                .name = std::make_shared<la::Symbol>(la::Symbol{"push_back"}),
                                .args = {la::FunctionCall::createExpression(create)}});
                            return la::StorageSymbol::createExpression(queue);
                        }
                        unsupported(e.pos, "unknown token context method: " + method->attr);
                    }
                if (auto method = x.func ? x.func->template get<py::Attribute>() : nullptr; method && !object(method->value)) {
                    if (method->attr == "fromkeys") {
                        auto owner = method->value ? method->value->template get<py::Name>() : nullptr;
                        if (owner && owner->id == "dict") {
                            if (x.args.size() != 2 || !x.keywords.empty()) unsupported(e.pos, "dict.fromkeys requires keys and an explicit default");
                            la::FunctionCall call;
                            call.name = std::make_shared<la::Symbol>(pythonSymbol("ISPA_STD.python_fromkeys"));
                            for (const auto& arg : x.args) call.args.push_back(expr(arg));
                            return la::FunctionCall::createExpression(call);
                        }
                    }
                    static const std::map<std::string, std::tuple<la::ArrayMethods, std::size_t, std::size_t>> methods{
                        {"append", {la::ArrayMethods::PythonAppend, 1, 1}},
                        {"extend", {la::ArrayMethods::PythonExtend, 1, 1}},
                        {"insert", {la::ArrayMethods::PythonInsert, 2, 2}},
                        {"remove", {la::ArrayMethods::PythonRemove, 1, 1}},
                        {"pop", {la::ArrayMethods::PythonPop, 0, 2}},
                        {"clear", {la::ArrayMethods::PythonClear, 0, 0}},
                        {"copy", {la::ArrayMethods::PythonCopy, 0, 0}},
                        {"count", {la::ArrayMethods::PythonCount, 1, 1}},
                        {"index", {la::ArrayMethods::PythonIndex, 1, 3}},
                        {"reverse", {la::ArrayMethods::PythonReverse, 0, 0}},
                        {"sort", {la::ArrayMethods::PythonSort, 0, 0}},
                        {"get", {la::ArrayMethods::PythonGet, 2, 2}},
                        {"setdefault", {la::ArrayMethods::PythonSetDefault, 2, 2}},
                        {"update", {la::ArrayMethods::PythonUpdate, 0, 1}},
                        {"keys", {la::ArrayMethods::PythonKeys, 0, 0}},
                        {"values", {la::ArrayMethods::PythonValues, 0, 0}},
                        {"items", {la::ArrayMethods::PythonItems, 0, 0}},
                        {"fromkeys", {la::ArrayMethods::PythonFromKeys, 2, 2}},
                    };
                    if (auto found = methods.find(method->attr); found != methods.end()) {
                        const auto& [kind, minimum, maximum] = found->second;
                        if (x.args.size() < minimum || x.args.size() > maximum)
                            unsupported(e.pos, "wrong argument count for collection method " + method->attr);
                        la::ArrayMethodCall call;
                        call.method = kind;
                        for (const auto& arg : x.args) call.args.push_back(expr(arg));
                        if (!x.keywords.empty()) {
                            if (method->attr == "update") {
                                std::set<std::string> names;
                                for (const auto& keyword : x.keywords) {
                                    if (!names.insert(keyword.arg).second) unsupported(e.pos, "duplicate update keyword");
                                    la::MakeTuple item;
                                    item.args = {la::String::createExpression(la::String{.value = keyword.arg}), expr(keyword.value)};
                                    call.args.push_back(la::MakeTuple::createExpression(item));
                                }
                            } else {
                                if (method->attr != "sort") unsupported(e.pos, "unsupported collection keyword arguments");
                                py::ExprPtr reverse, key;
                                for (const auto& keyword : x.keywords) {
                                    if (keyword.arg == "reverse" && !reverse) reverse = keyword.value;
                                    else if (keyword.arg == "key" && !key) key = keyword.value;
                                    else unsupported(e.pos, "sort accepts key and reverse once each");
                                }
                                call.args.push_back(reverse ? expr(reverse) : la::Bool::createExpression(la::Bool{.value = false}));
                                if (key) call.args.push_back(expr(key));
                            }
                        }
                        la::StorageSymbol storage;
                        storage.what = group(expr(method->value));
                        storage.path.emplace_back(call);
                        return la::StorageSymbol::createExpression(storage);
                    }
                    if (method->attr == "popitem")
                        unsupported(e.pos, "collection method requires insertion ordering or a concrete dictionary type");
                }
                if (!x.keywords.empty()) unsupported(e.pos, "keyword arguments are unsupported");
                if (action_context) if (auto method = x.func ? x.func->template get<py::Attribute>() : nullptr) {
                    if (auto owner = method->value ? method->value->template get<py::Name>() : nullptr; owner && owner->id == "ctx") {
                        const auto& name = method->attr;
                        const bool failure_method = name == "error" || name == "note" || name == "sync" || name == "sync_tokens" ||
                            name == "skip" || name == "delete" || name == "insert" || name == "replace" || name == "retry" || name == "abort";
                        const bool action_method = name == "at" || name == "peek" || name == "take" || name == "prev" ||
                            name == "mark" || name == "restore" || name == "commit" || name == "collect_braced";
                        if ((failure_method && !failure_context) || (!failure_method && !action_method))
                            unsupported(e.pos, "unknown context method: " + name);
                        if (failure_context && !token_rule.empty() &&
                            name != "at" && name != "error" && name != "delete" && name != "skip" &&
                            name != "insert" && name != "abort")
                            unsupported(e.pos, "unsupported lexer failure method: " + name);
                        if (name == "sync" || name == "sync_tokens") {
                            if (x.args.empty()) unsupported(e.pos, "ctx." + name + " requires at least one boundary");
                            for (const auto& arg : x.args) {
                                auto literal = arg ? arg->template get<py::Constant>() : nullptr;
                                if (literal && literal->kind == py::ConstantKind::Str && !literal->value.empty()) continue;
                                if (name == "sync_tokens" && !tokenName(arg).empty()) continue;
                                if (name == "sync_tokens") if (auto variable = arg ? arg->template get<py::Name>() : nullptr;
                                    variable && locals.contains(variable->id)) continue;
                                unsupported(e.pos, "ctx." + name + " boundaries must be nonempty strings or token identifiers");
                            }
                        } else if (name == "error") {
                            if (x.args.empty() || x.args.size() == 3 || x.args.size() > 4)
                                unsupported(e.pos, "ctx.error requires a message, optional code, and optional offset/length pair");
                            if (x.args.size() == 2 || x.args.size() == 4) {
                                auto code = x.args[1] ? x.args[1]->template get<py::Constant>() : nullptr;
                                if (!code || code->kind != py::ConstantKind::Str || code->value.empty())
                                    unsupported(e.pos, "ctx.error diagnostic code must be a nonempty string literal");
                            }
                        } else if (name == "note") {
                            if (x.args.size() != 1) unsupported(e.pos, "ctx.note requires one message");
                        } else if (name == "at") {
                            if (x.args.empty() || x.args.size() > 2) unsupported(e.pos, "ctx.at requires a token and optional lookahead offset");
                        } else if (name == "peek") {
                            if (x.args.size() > 1) unsupported(e.pos, "ctx.peek takes at most one lookahead offset");
                        } else if (name == "mark" || name == "restore" || name == "commit") {
                            if (x.args.size() != 1) unsupported(e.pos, "ctx." + name + " requires one checkpoint name");
                            auto label = x.args.front() ? x.args.front()->template get<py::Constant>() : nullptr;
                            if (!label || label->kind != py::ConstantKind::Str || label->value.empty())
                                unsupported(e.pos, "ctx." + name + " checkpoint name must be a nonempty string literal");
                        } else if (x.args.size() != 0) unsupported(e.pos, "ctx." + name + " takes no arguments");
                        if (name == "at") {
                            const auto token = tokenName(x.args.front());
                            if (!token.empty()) {
                                la::FunctionCall call;
                                call.name = std::make_shared<la::Symbol>("at_token");
                                call.args = {la::String::createExpression(la::String{.value = token})};
                                if (x.args.size() == 2) call.args.push_back(expr(x.args[1]));
                                la::StorageSymbol storage;
                                storage.what = group(expr(method->value));
                                storage.path.emplace_back(call);
                                return la::StorageSymbol::createExpression(storage);
                            }
                        }
                        if (name == "delete" && failure_context && !token_rule.empty()) {
                            la::StorageSymbol storage;
                            storage.what = group(expr(method->value));
                            storage.path.emplace_back(la::FunctionCall{
                                .name = std::make_shared<la::Symbol>("erase")});
                            return la::StorageSymbol::createExpression(storage);
                        }
                        if (name == "sync_tokens") {
                            la::FunctionCall call;
                            call.name = std::make_shared<la::Symbol>("sync_tokens");
                            for (const auto& arg : x.args) {
                                const auto token = tokenName(arg);
                                if (token.empty()) { call.args.push_back(expr(arg)); continue; }
                                la::FunctionCall boundary;
                                boundary.name = std::make_shared<la::Symbol>("token_boundary");
                                boundary.args.push_back(la::String::createExpression(la::String{.value = token}));
                                la::StorageSymbol tagged;
                                tagged.what = group(expr(method->value));
                                tagged.path.emplace_back(boundary);
                                call.args.push_back(la::StorageSymbol::createExpression(tagged));
                            }
                            la::StorageSymbol storage;
                            storage.what = group(expr(method->value));
                            storage.path.emplace_back(call);
                            return la::StorageSymbol::createExpression(storage);
                        }
                    }
                }
                la::FunctionCall call;
                for (const auto& a : x.args) call.args.push_back(expr(a));
                if (auto n = x.func ? x.func->template get<py::Name>() : nullptr) {
                    call.name = std::make_shared<la::Symbol>(pythonSymbol(localName(n->id)));
                    return la::FunctionCall::createExpression(call);
                }
                if (auto a = x.func ? x.func->template get<py::Attribute>() : nullptr) {
                    if (auto owner = a->value ? a->value->template get<py::Name>() : nullptr; owner && class_names.contains(owner->id)) {
                        call.name = std::make_shared<la::Symbol>(pythonSymbol(owner->id + "." + a->attr));
                        return la::FunctionCall::createExpression(call);
                    }
                    call.name = std::make_shared<la::Symbol>(a->attr);
                    la::StorageSymbol s;
                    s.what = group(expr(a->value));
                    s.path.emplace_back(call);
                    return la::StorageSymbol::createExpression(s);
                }
                unsupported(e.pos, "call target must be a name or method");
            } else if constexpr (std::is_same_v<T, py::BinaryExpr>) return binary(expr(x.left), binop(x.op, e.pos), expr(x.right));
            else if constexpr (std::is_same_v<T, py::UnaryExpr>) {
                la::Expression out;
                switch (x.op) {
                    case py::UnaryOp::Not: out.emplace_back(la::ExpressionElement::Not); break;
                    case py::UnaryOp::UAdd: out.emplace_back(la::ExpressionElement::Add); break;
                    case py::UnaryOp::USub: out.emplace_back(la::ExpressionElement::Minus); break;
                    default: unsupported(e.pos, "bitwise invert is unsupported");
                }
                auto operand = group(expr(x.operand));
                out.insert(out.end(), operand.begin(), operand.end());
                return group(out);
            } else if constexpr (std::is_same_v<T, py::BoolExpr>) {
                if (x.values.empty()) unsupported(e.pos, "empty boolean expression");
                auto out = expr(x.values.front());
                for (std::size_t i = 1; i < x.values.size(); ++i)
                    out = binary(out, x.op == py::BoolOp::And ? la::ExpressionElement::And : la::ExpressionElement::Or, expr(x.values[i]));
                return out;
            } else if constexpr (std::is_same_v<T, py::Compare>) {
                if (x.ops.size() != 1 || x.comparators.size() != 1)
                    unsupported(e.pos, "chained comparison requires single-evaluation lowering");
                la::ExpressionElement op;
                switch (x.ops[0]) {
                    case py::CmpOp::Eq: op = la::ExpressionElement::Equal; break;
                    case py::CmpOp::NotEq: op = la::ExpressionElement::NotEqual; break;
                    case py::CmpOp::Lt: op = la::ExpressionElement::Lower; break;
                    case py::CmpOp::LtE: op = la::ExpressionElement::LowerOrEqual; break;
                    case py::CmpOp::Gt: op = la::ExpressionElement::Higher; break;
                    case py::CmpOp::GtE: op = la::ExpressionElement::HigherOrEqual; break;
                    default: unsupported(e.pos, "identity and membership comparisons are unsupported");
                }
                if (action_context && (x.ops[0] == py::CmpOp::Eq || x.ops[0] == py::CmpOp::NotEq)) {
                    auto tokenCompare = [&](const py::ExprPtr& property, const py::ExprPtr& other) -> std::optional<la::Expression> {
                        auto attribute = property ? property->template get<py::Attribute>() : nullptr;
                        if (!attribute || attribute->attr != "name") return {};
                        if (const auto token = tokenName(other); !token.empty()) {
                            la::FunctionCall call;
                            call.name = std::make_shared<la::Symbol>("matches_name");
                            call.args.push_back(la::String::createExpression(la::String{.value = token}));
                            la::StorageSymbol storage;
                            storage.what = group(expr(attribute->value));
                            storage.path.emplace_back(call);
                            auto result = la::StorageSymbol::createExpression(storage);
                            if (x.ops[0] == py::CmpOp::NotEq) {
                                result = group(std::move(result));
                                result.insert(result.begin(), la::ExpressionElement::Not);
                            }
                            return result;
                        }
                        if (auto literal = other ? other->template get<py::Constant>() : nullptr;
                            literal && literal->kind == py::ConstantKind::Str) {
                            la::StorageSymbol text;
                            text.what = group(expr(attribute->value));
                            text.path.emplace_back("text");
                            return binary(la::StorageSymbol::createExpression(text), op, expr(other));
                        }
                        return {};
                    };
                    if (auto converted = tokenCompare(x.left, x.comparators[0])) return *converted;
                    if (auto converted = tokenCompare(x.comparators[0], x.left)) return *converted;
                }
                return binary(expr(x.left), op, expr(x.comparators[0]));
            } else if constexpr (std::is_same_v<T, py::Tuple>) {
                la::MakeTuple out;
                for (auto& item : x.elts) out.args.push_back(expr(item));
                return la::MakeTuple::createExpression(out);
            } else if constexpr (std::is_same_v<T, py::List>) {
                la::Array out;
                for (auto& item : x.elts) out.values.push_back(expr(item));
                for (const auto& item : inferred(std::make_shared<py::Expr>(e)).template_parameters)
                    if (auto t = std::get_if<la::Type>(&item)) out.template_parameters.push_back(std::make_shared<la::Type>(*t));
                return la::Array::createExpression(out);
            } else if constexpr (std::is_same_v<T, py::Dict>) {
                if (x.keys.size() != x.values.size()) unsupported(e.pos, "invalid dictionary");
                la::Map out;
                for (std::size_t i = 0; i < x.keys.size(); ++i) {
                    auto key = expr(x.keys[i]);
                    if (key.size() != 1 || !key.front().isRvalue()) unsupported(e.pos, "dictionary keys must be integer or string literals");
                    const auto& r = key.front().getRValue();
                    if (r.isInt()) out.keys.emplace_back(r.getInt());
                    else if (r.isString()) out.keys.emplace_back(r.getString());
                    else unsupported(e.pos, "dictionary keys must be integer or string literals");
                    out.values.push_back(expr(x.values[i]));
                }
                for (const auto& item : inferred(std::make_shared<py::Expr>(e)).template_parameters)
                    if (auto t = std::get_if<la::Type>(&item)) out.template_parameters.push_back(std::make_shared<la::Type>(*t));
                return la::Map::createExpression(out);
            } else if constexpr (std::is_same_v<T, py::Lambda>) {
                // Parameter types stay Undef: C++ (auto), Rust (closure inference) and Python infer them
                // from the expected Callable. The body sees the enclosing locals; its parameters shadow them.
                PythonConverter scope = *this;
                scope.block_depth = scope.loop_depth = 0;
                la::Lambda out;
                for (const auto& name : x.params) {
                    out.parameters.emplace_back(la::Type{la::ValueType::Undef}, name);
                    scope.locals.insert(name);
                    scope.local_types.insert_or_assign(name, la::Type{la::ValueType::Undef});
                    scope.objects.erase(name);
                }
                out.statements = la::Return::createStatements(la::Return{.value = scope.expr(x.body)});
                return la::Lambda::createExpression(std::move(out));
            } else unsupported(e.pos, "conditional expression is unsupported");
        }, e.kind);
    }
    la::Statements body(const py::StmtList& list) {
        if (!debug_ready_initialized) { debug_ready = locals; debug_ready_initialized = true; }
        la::Statements out;
        const auto checkpoint = [&](const py::Stmt& statement, const UAST::SourcePosition& position,
                                    bool after) -> std::optional<la::Statement> {
            if (position.file.empty() || position.line == 0 || statement.template get<py::Pass>())
                return std::nullopt;
            std::ostringstream marker;
            marker << "ISPA_DEBUG_LINE " << std::quoted(position.file) << ' '
                   << position.line << ' ' << position.column;
            if (after) marker << " @after";
            if (action_context && token_rule.empty()) marker << " ctx";
            for (const auto& name : debug_ready)
                if (name != "ctx") marker << ' ' << name << '=' << localName(name);
            return la::Comment::createStatement(la::Comment{.value = marker.str()});
        };
        for (const auto& p : list) {
            if (!p) throw std::invalid_argument("Python conversion: missing statement");
            auto position = p->pos;
            if (position.file.ends_with(".py") && position.line <= 1) {
                std::ifstream source(position.file);
                std::string line;
                std::size_t row = 0;
                while (std::getline(source, line)) {
                    ++row;
                    if (row < debug_line_cursor) continue;
                    const auto column = line.find_first_not_of(" \t\r");
                    if (column == std::string::npos || line[column] == '#' ||
                        line.compare(column, 5, "else:") == 0 ||
                        line.compare(column, 7, "except ") == 0 ||
                        line.compare(column, 8, "finally:") == 0) continue;
                    position.line = row;
                    position.column = column + 1;
                    debug_line_cursor = row + 1;
                    break;
                }
            }
            if (auto before = checkpoint(*p, position, false)) out.push_back(std::move(*before));
            auto part = stmt(*p);
            out.insert(out.end(), part.begin(), part.end());
            if (auto assignment = p->template get<py::AnnAssign>(); assignment && !assignment->value) {
                if (auto name = assignment->target ? assignment->target->template get<py::Name>() : nullptr)
                    debug_ready.erase(name->id);
            } else if (auto assignment = p->template get<py::Assign>()) {
                if (assignment->targets.size() == 1)
                    if (auto name = assignment->targets[0] ? assignment->targets[0]->template get<py::Name>() : nullptr)
                        debug_ready.insert(name->id);
            } else if (auto assignment = p->template get<py::AnnAssign>()) {
                if (auto name = assignment->target ? assignment->target->template get<py::Name>() : nullptr)
                    debug_ready.insert(name->id);
            }
            if (auto after = checkpoint(*p, position, true)) out.push_back(std::move(*after));
        }
        return out;
    }
    la::Statements stmt(const py::Stmt& s) {
        return std::visit([&](const auto& x) -> la::Statements {
            using T = std::decay_t<decltype(x)>;
            if constexpr (std::is_same_v<T, py::Pass>) return {};
            else if constexpr (std::is_same_v<T, py::IspaMatch>) {
                if (!embedded_match) unsupported(s.pos, "embedded ISPA match requires an advanced rule");
                return embedded_match(x, locals);
            }
            else if constexpr (std::is_same_v<T, py::ExprStmt>) return la::Statements{la::Statement{expr(x.value)}};
            else if constexpr (std::is_same_v<T, py::Return>) {
                if (!token_rule.empty()) unsupported(s.pos, "token actions cannot return; update ctx.ast instead");
                if (!action_context) return la::Return::createStatements(la::Return{.value = x.value ? expr(x.value) : la::Expression{}});
                if (failure_context && x.value) unsupported(s.pos, "fail blocks use a bare return");
                la::FunctionCall call;
                call.name = std::make_shared<la::Symbol>(failure_context ? "custom_failure_decision" : "action_result");
                if (failure_context) call.args = {la::Symbol::createExpression(la::Symbol{"before_failure"}), la::Symbol::createExpression(la::Symbol{"failed_pos"}), la::Symbol::createExpression(la::Symbol{"ctx"})};
                else {
                    la::StorageSymbol ast{"ast"}, pos{"pos"};
                    ast.what = pos.what = la::Symbol::createExpression(la::Symbol{"ctx"});
                    call.args = {x.value ? expr(x.value) : la::StorageSymbol::createExpression(ast), la::StorageSymbol::createExpression(pos)};
                }
                return la::Return::createStatements(la::Return{.value = la::FunctionCall::createExpression(call)});
            }
            else if constexpr (std::is_same_v<T, py::Break>) {
                if (!loop_depth) unsupported(s.pos, "break outside loop");
                return la::Break::createStatements(la::Break{});
            }
            else if constexpr (std::is_same_v<T, py::Continue>) {
                if (!loop_depth) unsupported(s.pos, "continue outside loop");
                return la::Continue::createStatements(la::Continue{});
            }
            else if constexpr (std::is_same_v<T, py::If>) return la::Statements{la::If{expr(x.test), nested(x.body), nested(x.orelse)}};
            else if constexpr (std::is_same_v<T, py::While>) {
                if (!x.orelse.empty()) unsupported(s.pos, "loop else is unsupported");
                return la::Statements{la::While{expr(x.test), nested(x.body, true)}};
            } else if constexpr (std::is_same_v<T, py::Assign>) {
                if (x.targets.size() != 1) unsupported(s.pos, "multiple assignment targets are unsupported");
                auto value = expr(x.value);
                if (auto n = x.targets[0] ? x.targets[0]->template get<py::Name>() : nullptr; n && !locals.contains(n->id)) {
                    if (action_context && n->id == "ctx") unsupported(s.pos, "ctx cannot be rebound");
                    const bool hoist = block_depth && escapes_block(n->id);
                    const auto value_type = inferred(x.value);
                    if (hoist && value_type.isValueType() && value_type.getValueType() == la::ValueType::Undef)
                        unsupported(s.pos, "cannot infer the type of '" + n->id + "', which is used outside this block; declare it with an annotation first");
                    locals.insert(n->id);
                    if (object(x.value)) objects.insert(n->id);
                    local_types.insert_or_assign(n->id, value_type);
                    if (block_depth) block_assigned.back().push_back(n->id);
                    if (hoist) {
                        hoisted.emplace_back(localName(n->id), value_type);
                        return la::VariableAssignment::createStatements(la::VariableAssignment{.name = target(x.targets[0]), .value = value});
                    }
                    if (block_depth) block_declared.back().push_back(n->id);
                    return la::Statements{la::Variable{.name = localName(n->id), .type = la::Type{la::ValueType::Undef}, .value = value}};
                }
                return la::VariableAssignment::createStatements(la::VariableAssignment{.name = target(x.targets[0]), .value = value});
            } else if constexpr (std::is_same_v<T, py::AnnAssign>) {
                auto n = x.target ? x.target->template get<py::Name>() : nullptr;
                if (!n && (action_context || !self_name.empty())) {
                    const auto destination = target(x.target);
                    if (!x.value) return {};
                    return la::VariableAssignment::createStatements(la::VariableAssignment{
                        .name = destination, .value = initializer(x.value, type(x.annotation))});
                }
                if (!n) unsupported(s.pos, "annotated target must be a name");
                if (action_context && n->id == "ctx") unsupported(s.pos, "ctx cannot be rebound");
                if (locals.contains(n->id)) unsupported(s.pos, "annotated declaration must introduce a new local");
                const bool hoist = block_depth && escapes_block(n->id);
                locals.insert(n->id);
                local_types.insert_or_assign(n->id, type(x.annotation));
                if (block_depth) block_assigned.back().push_back(n->id);
                if (hoist) {
                    hoisted.emplace_back(localName(n->id), type(x.annotation));
                    if (!x.value) return {};
                    return la::VariableAssignment::createStatements(la::VariableAssignment{
                        .name = target(x.target), .value = initializer(x.value, type(x.annotation))});
                }
                if (block_depth) block_declared.back().push_back(n->id);
                if (auto annotation = x.annotation ? x.annotation->template get<py::Name>() : nullptr;
                    annotation && class_names.contains(annotation->id)) objects.insert(n->id);
                return la::Statements{la::Variable{.name = localName(n->id), .type = type(x.annotation), .value = x.value ? initializer(x.value, type(x.annotation)) : la::Expression{}}};
            } else if constexpr (std::is_same_v<T, py::AugAssign>) {
                la::OperatorType op;
                switch (x.op) {
                    case py::BinOp::Add: op = la::OperatorType::Add; break;
                    case py::BinOp::Sub: op = la::OperatorType::Minus; break;
                    case py::BinOp::Mult: op = la::OperatorType::Multiply; break;
                    case py::BinOp::Div: case py::BinOp::FloorDiv: op = la::OperatorType::Divide; break;
                    case py::BinOp::Mod: op = la::OperatorType::Modulo; break;
                    default: unsupported(s.pos, "augmented operator is unsupported");
                }
                return la::VariableAssignment::createStatements(la::VariableAssignment{.name = target(x.target), .type = op, .value = expr(x.value)});
            } else if constexpr (std::is_same_v<T, py::Raise>) {
                if (x.cause) unsupported(s.pos, "raise from is unsupported");
                return la::Statements{la::Throw{.throw_value = x.exc ? expr(x.exc) : la::Expression{}}};
            } else unsupported(s.pos, "statement has no LangAPI representation (use declaration conversion for definitions)");
        }, s.kind);
    }
    la::Declaration declaration(const py::Stmt& s) {
        if (auto f = s.get<py::FunctionDef>()) {
            if (f->is_async || !f->decorators.empty()) unsupported(s.pos, "async functions and decorators are unsupported");
            PythonConverter scope;
            la::Function out;
            out.name = f->name;
            out.type = type(f->returns);
            scope.self_name = self_name;
            scope.class_names = class_names;
            if (s.pos.file.ends_with(".py")) {
                std::ifstream source(s.pos.file);
                std::string line;
                std::size_t row = 0;
                while (std::getline(source, line)) {
                    ++row;
                    if (line.find("def " + f->name + "(") != std::string::npos) {
                        scope.debug_line_cursor = row + 1;
                        break;
                    }
                }
            }
            if (!self_name.empty()) scope.objects.insert(self_name);
            for (auto& p : f->params) {
                if (!self_name.empty() && p.name == self_name && &p == &f->params.front()) continue;
                if (p.default_value) unsupported(p.pos, "default parameters are unsupported");
                out.parameters.emplace_back(type(p.annotation), p.name);
                scope.locals.insert(p.name);
                scope.local_types.insert_or_assign(p.name, type(p.annotation));
                if (auto annotation = p.annotation ? p.annotation->template get<py::Name>() : nullptr;
                    annotation && class_names.contains(annotation->id)) scope.objects.insert(p.name);
            }
            scope.function_body = &f->body;
            out.statements = scope.body(f->body);
            for (auto it = scope.hoisted.rbegin(); it != scope.hoisted.rend(); ++it)
                out.statements.insert(out.statements.begin(), la::Statement{la::Variable{.name = it->first, .type = it->second}});
            if (out.statements.empty() && (out.type == la::ValueType::Void || f->name == "__init__")) out.statements = la::Return::createStatements(la::Return{});
            return la::Declaration{out};
        }
        if (auto c = s.get<py::ClassDef>()) {
            if (!c->decorators.empty() || !c->keywords.empty()) unsupported(s.pos, "class decorators and keywords are unsupported");
            la::Class out;
            out.name = la::Symbol{c->name};
            for (auto& b : c->bases) {
                auto n = b ? b->template get<py::Name>() : nullptr;
                if (!n) unsupported(s.pos, "base class must be a name");
                out.inherit_members.emplace_back(la::Visibility::Public, pythonSymbol(n->id));
            }
            PythonConverter scope;
            scope.class_names = class_names;
            std::set<std::string> fields;
            for (const auto& member : c->body) if (auto a = member->template get<py::AnnAssign>(); a && a->target)
                if (auto n = a->target->template get<py::Name>()) fields.insert(n->id);
            // Discover fixed instance fields from __init__. Explicit annotations
            // win; otherwise a typed parameter or scalar literal supplies the type.
            for (const auto& member : c->body) if (auto init = member->template get<py::FunctionDef>();
                init && init->name == "__init__" && !init->params.empty()) {
                for (const auto& line : init->body) {
                    py::ExprPtr destination, value, annotation;
                    if (auto a = line->template get<py::Assign>(); a && a->targets.size() == 1) {
                        destination = a->targets[0]; value = a->value;
                    } else if (auto a = line->template get<py::AnnAssign>()) {
                        destination = a->target; value = a->value; annotation = a->annotation;
                    } else continue;
                    auto attr = destination ? destination->template get<py::Attribute>() : nullptr;
                    auto owner = attr && attr->value ? attr->value->template get<py::Name>() : nullptr;
                    if (!owner || owner->id != init->params.front().name || fields.contains(attr->attr)) continue;
                    la::Type field_type{la::ValueType::Undef};
                    if (annotation) field_type = type(annotation);
                    else if (auto name = value ? value->template get<py::Name>() : nullptr) {
                        for (const auto& p : init->params) if (p.name == name->id) field_type = type(p.annotation);
                    } else if (auto literal = value ? value->template get<py::Constant>() : nullptr) {
                        switch (literal->kind) {
                            case py::ConstantKind::Int: field_type = la::Type{la::ValueType::Int}; break;
                            case py::ConstantKind::Float: field_type = la::Type{la::ValueType::Float}; break;
                            case py::ConstantKind::Bool: field_type = la::Type{la::ValueType::Bool}; break;
                            case py::ConstantKind::Str: field_type = la::Type{la::ValueType::String}; break;
                            default: break;
                        }
                    }
                    if (field_type == la::ValueType::Undef) unsupported(line->pos, "instance field requires an annotation or a typed parameter initializer");
                    fields.insert(attr->attr);
                    out.data.emplace_back(std::make_shared<la::Declaration>(la::Variable{.name = attr->attr, .type = field_type}), la::Visibility::Public);
                }
            }
            for (auto& member : c->body) {
                if (!member) unsupported(s.pos, "missing class member");
                if (member->template get<py::Pass>()) continue;
                if (auto text = member->template get<py::ExprStmt>(); text && text->value && text->value->template get<py::Constant>() &&
                    text->value->template get<py::Constant>()->kind == py::ConstantKind::Str) continue;
                if (auto f = member->template get<py::FunctionDef>()) {
                    auto method = *f;
                    bool is_static = false;
                    if (method.decorators.size() == 1) {
                        auto name = method.decorators[0]->template get<py::Name>();
                        if (name && name->id == "staticmethod") { is_static = true; method.decorators.clear(); }
                    }
                    if (!is_static && method.params.empty()) unsupported(member->pos, "instance methods require a self parameter");
                    scope.self_name = is_static ? "" : method.params.front().name;
                    py::Stmt syntax{member->pos, method};
                    auto lowered = scope.declaration(syntax);
                    auto& function = lowered.getFunction();
                    function.is_static = is_static;
                    if (f->name == "__init__") {
                        if (is_static || (f->returns && function.type != la::ValueType::Void))
                            unsupported(member->pos, "__init__ must be an instance method returning None");
                        function.name = c->name;
                        if (out.constructor) unsupported(member->pos, "duplicate constructor");
                        out.constructor = std::move(function);
                        scope.self_name.clear();
                        continue;
                    }
                    out.data.emplace_back(std::make_shared<la::Declaration>(std::move(lowered)), la::Visibility::Public);
                    scope.self_name.clear();
                } else {
                    // `NAME: ClassVar[T] = value` is a class-level constant, not an instance field.
                    if (auto a = member->template get<py::AnnAssign>(); a && a->target && a->annotation) {
                        auto marker = a->annotation->template get<py::Subscript>();
                        auto marker_name = marker && marker->value ? marker->value->template get<py::Name>() : nullptr;
                        auto field = a->target->template get<py::Name>();
                        if (marker_name && (marker_name->id == "ClassVar" || marker_name->id == "typing.ClassVar") && field) {
                            if (!a->value) unsupported(member->pos, "ClassVar requires an initializer");
                            const auto field_type = scope.type(marker->slice);
                            la::Variable constant{.name = field->id, .type = field_type,
                                                  .value = scope.initializer(a->value, field_type), .is_static = true};
                            out.data.emplace_back(std::make_shared<la::Declaration>(la::Declaration{constant}), la::Visibility::Public);
                            continue;
                        }
                    }
                    auto lowered = scope.declaration(*member);
                    if (lowered.isVariable() && lowered.getVariable().type == la::ValueType::Undef)
                        unsupported(member->pos, "class fields require type annotations");
                    out.data.emplace_back(std::make_shared<la::Declaration>(std::move(lowered)), la::Visibility::Public);
                }
            }
            return la::Declaration{out};
        }
        auto statements = stmt(s);
        if (statements.size() == 1 && statements.front().isVariable()) return la::Declaration{statements.front().getVariable()};
        unsupported(s.pos, "expected a function, class or variable declaration");
    }
};
}
auto ParserBase::convertPython(const UAST::py::Expr& e) -> LangAPI::Expression { return PythonConverter{}.expr(e); }
auto ParserBase::convertPython(const UAST::py::Stmt& s, const std::set<std::string>& variables) -> LangAPI::Statements { return PythonConverter{variables}.stmt(s); }
auto ParserBase::convertPython(const UAST::py::Module& m, const std::set<std::string>& variables) -> LangAPI::Statements { return PythonConverter{variables}.body(m.body); }
auto ParserBase::convertPythonDeclaration(const UAST::py::Stmt& s, const std::set<std::string>& classes) -> LangAPI::Declaration {
    PythonConverter converter;
    converter.class_names = classes;
    return converter.declaration(s);
}
auto ParserBase::convertPythonDeclarations(const UAST::py::Module& m) -> LangAPI::Declarations {
    PythonConverter converter;
    for (const auto& s : m.body) if (s) if (auto cls = s->template get<py::ClassDef>()) converter.class_names.insert(cls->name);
    LangAPI::Declarations out;
    for (auto& s : m.body) {
        if (!s) unsupported(m.pos, "missing declaration");
        if (s->template get<UAST::py::Pass>()) continue;
        out.push_back(converter.declaration(*s));
    }
    return out;
}
auto ParserBase::convertPythonType(const UAST::py::ExprPtr& annotation) -> LangAPI::Type {
    return PythonConverter{}.type(annotation);
}
auto ParserBase::convertPythonAction(const AST::PythonAction& action, std::set<std::string>& variables, bool failure,
    const std::set<std::string>& classes, std::set<std::string>* objects, const std::string& token_rule,
    const std::function<LangAPI::Statements(const UAST::py::IspaMatch&, std::set<std::string>&)>& match) -> LangAPI::Statements {
    PythonConverter converter{variables, true, failure};
    converter.class_names = classes;
    converter.token_rule = token_rule;
    converter.embedded_match = match;
    if (objects) converter.objects = *objects;
    auto result = std::visit([&](const auto& code) -> LangAPI::Statements {
        using T = std::decay_t<decltype(code)>;
        if constexpr (std::is_same_v<T, UAST::py::Module>) return converter.body(code.body);
        else if constexpr (std::is_same_v<T, UAST::py::Expr>) {
            auto value = converter.expr(code);
            if (action.target.empty()) return {LangAPI::Statement{std::move(value)}};
            LangAPI::StorageSymbol destination{action.target};
            destination.what = LangAPI::Symbol::createExpression(LangAPI::Symbol{
                token_rule.empty() ? "result" : "ispa_token_result"});
            return LangAPI::IspaLibFunctionCall::createStatements(LangAPI::IspaLibFunctionCall{
                .symbol = LangAPI::IspaLibSymbol{.exports = LangAPI::StdlibExports::AssignCompatible},
                .args = {LangAPI::StorageSymbol::createExpression(std::move(destination)), std::move(value)}});
        }
        else throw std::invalid_argument("Python action has no executable syntax");
    }, action.code);
    variables = std::move(converter.locals);
    if (objects) *objects = std::move(converter.objects);
    return result;
}
}
