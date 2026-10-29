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

    auto symbolKey(const LangAPI::Symbol& symbol) -> std::string {
        std::vector<std::string> parts;
        for (const auto& part : symbol.path) {
            const auto* name = std::get_if<std::string>(&part);
            if (!name) return {};
            parts.push_back(*name);
        }
        return joinStrings(parts, "::");
    }
    auto resolveFunction(const LangAPI::Symbol& symbol) -> const LangAPI::Function* {
        auto key = symbolKey(symbol);
        if (key.empty()) return nullptr;
        auto scope = RustCore::scope;
        while (true) {
            const auto qualified = (scope.empty() ? "" : joinStrings(scope, "::") + "::") + key;
            if (const auto it = RustCore::functions.find(qualified); it != RustCore::functions.end()) {
                if (it->second.size() != 1)
                    throw Error("Rust target: ambiguous call `{}`; IR needs a resolved callee identity", qualified);
                return it->second.front();
            }
            if (scope.empty()) break;
            scope.pop_back();
        }
        return nullptr;
    }
    // `receiver.name(args)`: the IR has no receiver type, but when every method of that name takes its parameters the same
    // way (borrowed / mutably borrowed / by value), the adaptation of the arguments is unambiguous.
    auto resolveMethodByName(const std::string& name, std::size_t argument_count) -> const LangAPI::Function* {
        const auto it = RustCore::methods.find(name);
        if (it == RustCore::methods.end()) return nullptr;
        const auto signature = [](const LangAPI::Function& function) {
            std::vector<int> result;
            for (const auto& parameter : function.parameters) {
                int kind = 0;
                if (isValueTypeOf(parameter.first, LangAPI::ValueType::BorrowSemantic)) {
                    kind = 1;
                    if (parameter.first.template_parameters.size() == 1 &&
                        std::holds_alternative<LangAPI::Type>(parameter.first.template_parameters[0]) &&
                        isValueTypeOf(std::get<LangAPI::Type>(parameter.first.template_parameters[0]), LangAPI::ValueType::ImmutableSemantic))
                        kind = 2;
                }
                result.push_back(kind);
            }
            return result;
        };
        const LangAPI::Function* first = nullptr;
        for (const auto* candidate : it->second) {
            if (candidate->is_static || candidate->parameters.size() != argument_count) continue;
            if (!first) first = candidate;
            else if (signature(*first) != signature(*candidate)) return nullptr;
        }
        return first;
    }
    auto argument(const LangAPI::Type& type, const LangAPI::Expression& source, const std::string& rendered) -> std::string {
        if (type.isValueType() && type.getValueType() == LangAPI::ValueType::BorrowSemantic && source.size() == 1 && source[0].isRvalue() &&
            source[0].getRValue().isSymbol() && source[0].getRValue().getSymbol().path.size() == 1 &&
            std::holds_alternative<std::string>(source[0].getRValue().getSymbol().path[0]) &&
            RustCore::reference_parameters.contains(std::get<std::string>(source[0].getRValue().getSymbol().path[0])))
            return "&mut *" + RustCore::ident(std::get<std::string>(source[0].getRValue().getSymbol().path[0]));   // lent again
        if (type.isValueType() && type.getValueType() == LangAPI::ValueType::BorrowSemantic) {
            if (type.template_parameters.size() != 1 || !std::holds_alternative<LangAPI::Type>(type.template_parameters[0]))
                throw Error("Rust target: BorrowSemantic requires one type parameter");
            const auto& inner = std::get<LangAPI::Type>(type.template_parameters[0]);
            if (!isValueTypeOf(inner, LangAPI::ValueType::ImmutableSemantic) && source.size() == 1 && source[0].isRvalue() &&
                source[0].getRValue().isSymbol() && source[0].getRValue().getSymbol().path.size() == 1 &&
                std::holds_alternative<std::string>(source[0].getRValue().getSymbol().path[0]) &&
                RustCore::const_borrowed_parameters.contains(std::get<std::string>(source[0].getRValue().getSymbol().path[0]))) {
                // the frontend only sees a container being handed on, not that the callee mutates it
                const auto name = RustCore::ident(std::get<std::string>(source[0].getRValue().getSymbol().path[0]));
                return "::ispa_std::pycoll::unshare(" + name + ")";
            }
            return std::string(isValueTypeOf(inner, LangAPI::ValueType::ImmutableSemantic) ? "&" : "&mut ") + asExpr(rendered);
        }
        if (isValueTypeOf(type, LangAPI::ValueType::MoveSemantic)) return rendered;
        return RustCore::convertInitializer(type, source);
    }
}

// `inline const int x = 5;` style scalars become a real `const`; anything else needs runtime construction
auto RustCore::isConstScalar(const LangAPI::Type &t) -> bool {
    if (!isValueTypeOf(t, LangAPI::ValueType::ImmutableSemantic) || t.template_parameters.size() != 1) return false;
    if (!std::holds_alternative<LangAPI::Type>(t.template_parameters.front())) return false;
    const auto &inner = std::get<LangAPI::Type>(t.template_parameters.front());
    if (!inner.isValueType()) return false;
    switch (inner.getValueType()) {
        case LangAPI::ValueType::Int: case LangAPI::ValueType::LargeUInt: case LangAPI::ValueType::Float:
        case LangAPI::ValueType::Bool: case LangAPI::ValueType::Char: return true;
        default: return false;
    }
}
auto RustCore::bind(const std::string& name) -> void {
    if (!locals.empty()) locals.back().insert(name);
    borrowed_parameters.erase(name);
    shared_locals.erase(ident(name));
    lent_locals.erase(ident(name));
}
auto RustCore::indexFunctions(const LangAPI::Declarations& declarations) -> void {
    functions.clear();
    methods.clear();
    globals.clear();
    shared_classes.clear();
    class_methods.clear();
    class_fields.clear();
    class_generics.clear();
    class_decls.clear();
    slot_functions.clear();
    overrides.clear();
    std::vector<const LangAPI::Class*> all_classes;
    std::vector<std::string> path;
    const auto add = [&](const LangAPI::Function& function) {
        functions[joinStrings(path, "::") + (path.empty() ? "" : "::") + function.name].push_back(&function);
    };
    const auto walk = [&](auto&& self, const LangAPI::Declarations& items) -> void {
        for (const auto& declaration : items) {
            if (declaration.isNamespace()) {
                path.push_back(declaration.getNamespace().name);
                self(self, declaration.getNamespace().declarations);
                path.pop_back();
            } else if (declaration.isFunction()) add(declaration.getFunction());
            else if (declaration.isVariable()) {
                const auto& variable = declaration.getVariable();
                if (!isConstScalar(variable.type) || variable.value.empty())
                    globals.insert(joinStrings(path, "::") + (path.empty() ? "" : "::") + variable.name);
            } else if (declaration.isClass()) {
                const auto& cls = declaration.getClass();
                class_decls[symbolKey(cls.name)] = &cls;
                all_classes.push_back(&cls);
                if (cls.shared_object) shared_classes.insert(symbolKey(cls.name));
                class_generics[symbolKey(cls.name)] = cls.template_parameters.size();
                path.push_back(symbolKey(cls.name));
                for (const auto& [member, visibility] : cls.data) {
                    if (member->isFunction()) {
                        add(member->getFunction());
                        methods[member->getFunction().name].push_back(&member->getFunction());
                        class_methods[symbolKey(cls.name) + "::" + member->getFunction().name].push_back(&member->getFunction());
                    } else if (member->isVariable())
                        class_fields[symbolKey(cls.name)][member->getVariable().name] = &member->getVariable().type;
                }
                if (cls.to_str_fun) add(*cls.to_str_fun);
                path.pop_back();
            }
        }
    };
    walk(walk, declarations);
    // classes the hand-written facade subclasses (its typed cursors, tokens and name tables): every method dispatches
    for (const auto* cls : all_classes) {
        static const std::unordered_set<std::string> open_classes{"TokenCursor", "RawToken", "RawTokenMeta", "TokenNames"};
        if (!open_classes.contains(symbolKey(cls->name))) continue;
        for (const auto& [member, visibility] : cls->data)
            if (member->isFunction() && !member->getFunction().is_static && member->getFunction().name != "__init__" &&
                member->getFunction().is_virtual)
                slot_functions.insert(&member->getFunction());
    }
    // dispatch slots: an overriding method registers in the first-base chain's topmost class that declares the method
    for (const auto* cls : all_classes) {
        for (const auto& [member, visibility] : cls->data) {
            if (!member->isFunction()) continue;
            const auto& function = member->getFunction();
            // every non-static method but `__init__` is virtual (Python); generated code marks its overrides of runtime methods
            if (function.is_static || function.name == "__init__" || (!function.is_virtual && !function.override)) continue;
            const LangAPI::Class* current = cls;
            const LangAPI::Function* introduced = nullptr;
            const LangAPI::Class* introducer = nullptr;
            std::size_t hops = 0, found_hops = 0;
            while (!current->inherit_members.empty()) {
                const auto* base = std::get_if<LangAPI::Symbol>(&current->inherit_members.front().second);
                if (!base) break;
                if (base->path.empty() || !std::holds_alternative<std::string>(base->path.back())) break;
                const auto it = class_decls.find(std::get<std::string>(base->path.back()));
                if (it == class_decls.end()) break;
                current = it->second;
                ++hops;
                for (const auto& [candidate, v] : current->data)
                    if (candidate->isFunction() && candidate->getFunction().name == function.name && !candidate->getFunction().is_static) {
                        introduced = &candidate->getFunction();
                        introducer = current;
                        found_hops = hops;
                    }
            }
            if (!introduced) {
                // the base is the runtime (a library type): its slot lives there, registered like any other
                if (function.override && !current->inherit_members.empty() &&
                    std::holds_alternative<LangAPI::IspaLibSymbol>(current->inherit_members.front().second))
                    overrides[&function] = {"", hops + 1, true};
                continue;
            }
            slot_functions.insert(introduced);
            overrides[&function] = {symbolKey(introducer->name), found_hops};
        }
    }
}

auto RustCore::isSharedObjectInitializer(const LangAPI::Expression &value) -> bool {
    if (value.size() != 1 || !value[0].isFunctionCall()) return false;
    const auto &call = value[0].getFunctionCall();
    const auto *name = std::get_if<std::shared_ptr<LangAPI::Symbol>>(&call.name);
    if (!name || !*name || symbolKey(**name) != "ISPA_STD::construct" && symbolKey(**name) != "ISPA_STD::make_value") return false;
    if (call.template_parameters.size() != 1) return false;
    const auto *type = std::get_if<std::shared_ptr<LangAPI::Type>>(&call.template_parameters.front());
    return type && *type && isSharedClassType(**type);
}
auto RustCore::currentClass() -> std::string {
    return receiver.empty() || scope.empty() ? std::string() : scope.back();
}
auto RustCore::classOfType(const LangAPI::Type *t) -> std::string {
    while (t && t->isValueType() && t->template_parameters.size() == 1 && std::holds_alternative<LangAPI::Type>(t->template_parameters[0]) &&
           (t->getValueType() == LangAPI::ValueType::Box || t->getValueType() == LangAPI::ValueType::ImmutableSemantic ||
            t->getValueType() == LangAPI::ValueType::BorrowSemantic || t->getValueType() == LangAPI::ValueType::MoveSemantic))
        t = &std::get<LangAPI::Type>(t->template_parameters[0]);
    if (t && t->isSymbol() && !t->getSymbol().path.empty())
        if (const auto* name = std::get_if<std::string>(&t->getSymbol().path.back())) return *name;
    return {};
}
// the one value of an expression, looking through grouping parentheses
static auto soleRValue(const LangAPI::Expression &e) -> const LangAPI::RValue * {
    const LangAPI::RValue *found = nullptr;
    for (const auto &element : e) {
        if (element.isExpressionElement()) {
            if (element.getExpressionElement() == LangAPI::ExpressionElement::GroupOpen ||
                element.getExpressionElement() == LangAPI::ExpressionElement::GroupClose) continue;
            return nullptr;
        }
        if (!element.isRvalue() || found) return nullptr;
        found = &element.getRValue();
    }
    return found;
}
auto RustCore::typeOfExpression(const LangAPI::Expression &e) -> const LangAPI::Type * {
    const auto *sole = soleRValue(e);
    if (!sole) return nullptr;
    const auto &r = *sole;
    if (r.isSymbol()) {
        const auto &path = r.getSymbol().path;
        if (path.size() != 1 || !std::holds_alternative<std::string>(path[0])) return nullptr;
        const auto &name = std::get<std::string>(path[0]);
        if (const auto it = local_types.find(name); it != local_types.end()) return it->second;
        if (const auto cls = class_fields.find(currentClass()); cls != class_fields.end())
            if (const auto field = cls->second.find(name); field != cls->second.end()) return field->second;
        return nullptr;
    }
    if (r.type() == LangAPI::RValueType::GetFromBox) return typeOfExpression(r.getGetFromBox().what);
    if (r.isStorageSymbol()) {
        const auto &storage = r.getStorageSymbol();
        std::string cls;
        const auto *root = soleRValue(storage.what);
        if (root && root->type() == LangAPI::RValueType::Instance)
            cls = currentClass();
        else cls = classOfType(typeOfExpression(storage.what));
        const LangAPI::Type *type = nullptr;
        for (const auto &part : storage.path) {
            const auto *name = std::get_if<std::string>(&part);
            if (!name) return nullptr;
            const auto fields = class_fields.find(cls);
            if (fields == class_fields.end()) return nullptr;
            const auto field = fields->second.find(*name);
            if (field == fields->second.end()) return nullptr;
            type = field->second;
            cls = classOfType(type);
        }
        return type;
    }
    return nullptr;
}

auto RustCore::isSharedClassType(const LangAPI::Type &t) -> bool {
    if (!t.isSymbol() || t.getSymbol().path.empty() || !std::holds_alternative<std::string>(t.getSymbol().path.back())) return false;
    return shared_classes.contains(std::get<std::string>(t.getSymbol().path.back()));
}

auto RustCore::warn(std::string message) -> void {
    if (std::find(diagnostics.begin(), diagnostics.end(), message) == diagnostics.end())
        diagnostics.push_back(std::move(message));
}
auto RustCore::crateRoot() -> std::string {
    return name.empty() ? std::string("crate") : "crate::" + moduleIdent(name);
}

// ============================================================================
// identifiers and literals
// ============================================================================
auto RustCore::ident(const std::string &id) -> std::string {
    if (id.starts_with("__ispa_rust_") || id.starts_with("__ispa_python_"))
        throw Error("Rust target: language-specific pseudo-symbol `{}`; IR needs a semantic operation", id);
    static const std::unordered_set<std::string> keywords{
        "as", "break", "const", "continue", "crate", "else", "enum", "extern", "false", "fn", "for", "if", "impl",
        "in", "let", "loop", "match", "mod", "move", "mut", "pub", "ref", "return", "self", "Self", "static",
        "struct", "super", "trait", "true", "type", "unsafe", "use", "where", "while", "async", "await", "dyn",
        "abstract", "become", "box", "do", "final", "macro", "override", "priv", "typeof", "unsized", "virtual",
        "yield", "try", "gen"};
    if (id.empty()) return id;
    // Symbols contain identifiers, never source code in another language.
    bool plain = !(id[0] >= '0' && id[0] <= '9');
    for (const char c : id) plain = plain && isIdentChar(c);
    if (!plain) throw Error("Rust target: language-dependent Symbol `{}`; lower it to semantic IR before emission", id);
    if (id == "clone") return "clone_";   // a lowered method of that name is not Clone::clone
    if (keywords.contains(id)) {
        if (id == "crate" || id == "self" || id == "Self" || id == "super") return id + "_";   // cannot be raw identifiers
        return "r#" + id;
    }
    return id;
}

auto RustCore::moduleIdent(const std::string &raw) -> std::string {
    std::string result;
    for (const unsigned char c : raw)
        result += isIdentChar(static_cast<char>(c)) ? static_cast<char>(c) : '_';
    if (result.empty() || std::isdigit(static_cast<unsigned char>(result.front()))) result.insert(result.begin(), '_');
    return ident(result);
}

// LangAPI::String contains decoded UTF-8 data, not a source-language literal body.
auto RustCore::rustStringBody(std::string_view raw) -> std::string {
    std::string out;
    for (const unsigned char c : raw) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            case 0: out += "\\0"; break;
            default:
                if (c < 0x20 || c == 0x7f) out += hex2(c);
                else out += static_cast<char>(c);
        }
    }
    return out;
}

// Characters are bytes: the parser walks a `const char*`, so `char` maps to u8 and 'x' to b'x'.
auto RustCore::rustCharLiteral(char value, bool escaped) -> std::string {
    if (escaped)
        throw Error("Rust target: Char contains an encoded escape; decode it at the source-language boundary");
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
                return "::ispa_std::Shared<" + joined + ">";
            case LangAPI::ValueType::Span:
                return "::ispa_std::Span<" + joined + ">";
            case LangAPI::ValueType::TypeTag:
                return "::std::marker::PhantomData<" + joined + ">";
            // Argument-position `impl FnMut`: callers pass plain closures (as lambdas lower), and
            // FnMut also admits Fn closures. Forwarding a callable parameter needs `&mut name`.
            case LangAPI::ValueType::Callable: {
                if (parts.empty()) throw Error("Rust Converter: Callable needs a return type");
                std::vector<std::string> arguments(parts.begin(), parts.end() - 1);
                const auto result = parts.back() == "()" ? std::string() : " -> " + parts.back();
                if (owned_callable) return "Box<dyn FnMut(" + joinStrings(arguments, ", ") + ")" + result + ">";
                return "impl FnMut(" + joinStrings(arguments, ", ") + ")" + result + (static_callable ? " + 'static" : "");
            }
            case LangAPI::ValueType::Any:
                return "::ispa_std::PyAny";
            case LangAPI::ValueType::Undef:
                return "_";
            default:
                throw Error("Unknown type: {}", (int) type.getValueType());
        }
    } else if (type.isSymbol()) {
        if (type.getSymbol().path.size() == 1 && std::holds_alternative<std::string>(type.getSymbol().path[0]) &&
            (std::get<std::string>(type.getSymbol().path[0]) == "auto" || std::get<std::string>(type.getSymbol().path[0]) == "auto&"))
            return "_";
        auto text = convertSymbol(type.getSymbol(), false);
        if (!type.template_parameters.empty())
            text += "<" + convertTemplates(type.template_parameters) + ">";
        return text;
    } else if (type.isIspaLibSymbol()) {
        return convertIspaLibSymbol(type.getIspaLibSymbol(), false);
    } else throw Error("unknown variant type in Type::type");
}

// Resolve receivers in emission state; Symbol paths themselves remain neutral.
auto RustCore::convertSymbol(const LangAPI::Symbol &symbol, bool value_context) -> std::string {
    if (symbol.path.empty()) return "";
    if (value_context && std::holds_alternative<std::string>(symbol.path.front())) {
        const auto& leaf = std::get<std::string>(symbol.path.front());
        if (symbol.path.size() == 1 && borrowed_parameters.contains(leaf)) return "(*" + ident(leaf) + ")";
        if (symbol.path.size() == 1 && callable_parameters.contains(leaf)) return "(&mut " + ident(leaf) + ")";
        const bool local = std::any_of(locals.rbegin(), locals.rend(), [&](const auto& frame) { return frame.contains(leaf); });
        if (!local && ((!receiver.empty() && members.contains(leaf)) || static_members.contains(leaf))) {
            const auto prefix = static_members.contains(leaf) ? std::string("Self::") : receiver + ".";
            LangAPI::Symbol tail = symbol;
            tail.path.erase(tail.path.begin());
            return prefix + ident(leaf) + (tail.path.empty() ? "" : "::" + convertSymbol(tail, false));
        }
    }
    if (symbol.path.size() >= 2 && std::holds_alternative<std::string>(symbol.path[0]) &&
        std::holds_alternative<std::string>(symbol.path[1]) &&
        std::get<std::string>(symbol.path[0]).empty() && std::get<std::string>(symbol.path[1]) == name) {
        LangAPI::Symbol relative = symbol;
        relative.path.erase(relative.path.begin(), relative.path.begin() + 2);
        const auto suffix = convertSymbol(relative, false);
        return crateRoot() + (suffix.empty() ? "" : "::" + suffix);
    }
    std::string res;
    bool first = true;
    for (const auto &part : symbol.path) {
        std::string piece;
        if (std::holds_alternative<std::string>(part)) {
            piece = std::get<std::string>(part);
            if (first && piece == "__ispa_parser__") piece = crateRoot();
            else if (first && piece == "ISPA_STD" && symbol.path.size() > 1 && std::holds_alternative<std::string>(symbol.path[1]) &&
                     std::get<std::string>(symbol.path[1]) == "CountSpec") piece = "::ispa_std";   // the repetition count of the typed stdlib
            else piece = ident(piece);
        } else if (std::holds_alternative<LangAPI::IspaLibSymbol>(part)) {
            piece = convertIspaLibSymbol(std::get<LangAPI::IspaLibSymbol>(part), true);
        } else {
            piece = convertFunctionCall(std::get<LangAPI::FunctionCall>(part));
        }
        // lib symbols already start with "::"; an empty segment only means something as the first one (leading "::")
        if (!first && piece.starts_with("::")) piece.erase(0, 2);
        if (!first && piece.empty()) continue;
        if (!first) res += "::";
        res += piece;
        first = false;
    }
    // Namespace-level variables live in a `Lazy`; `.get()` yields the (single-threaded, interior-mutable) place.
    if (value_context) {
        const auto key = symbolKey(symbol);
        const bool local = symbol.path.size() == 1 && std::any_of(locals.rbegin(), locals.rend(),
            [&](const auto& frame) { return frame.contains(key); });
        if (!key.empty() && !local) {
            auto where = scope;
            while (true) {
                if (globals.contains((where.empty() ? "" : joinStrings(where, "::") + "::") + key))
                    return "(*" + res + ".get())";
                if (where.empty()) break;
                where.pop_back();
            }
        }
    }
    return res;
}

auto RustCore::convertStorageSymbol(const LangAPI::StorageSymbol &symbol, bool place) -> std::string {
    std::string res = convertExpression(symbol.what);
    // a local holding an object handle: members are reached through the handle
    const auto bare = res.size() > 2 && res.front() == '(' && res.back() == ')' ? res.substr(1, res.size() - 2) : res;
    const auto saved_class = member_class;
    if (!receiver.empty() && bare == receiver) member_class = currentClass();
    else member_class = classOfType(typeOfExpression(symbol.what));
    struct Restore { std::string& slot; std::string value; ~Restore() { slot = value; } } restore{member_class, saved_class};
    if (!symbol.path.empty() && shared_locals.contains(bare) && !std::holds_alternative<LangAPI::StorageOffset>(symbol.path.front()))
        res = "(*" + res + ".borrow_mut())";
    std::vector<std::string> hoisted;
    const auto touches_self = [&](const std::string& text) {
        if (receiver.empty()) return false;
        if (text.find("(" + receiver + ")") != std::string::npos) return true;
        for (auto at = text.find(receiver + "."); at != std::string::npos; at = text.find(receiver + ".", at + 1))
            if (at == 0 || !(std::isalnum(static_cast<unsigned char>(text[at - 1])) || text[at - 1] == '_' || text[at - 1] == '.')) return true;
        return false;
    };
    const auto hoist = [&](std::string& text) {
        if (text.starts_with("|") || text.starts_with("move |")) return;   // a closure is typed by the callee
        const auto name = "__h" + std::to_string(++hoist_counter);
        hoisted.push_back("let " + name + " = " + text + ";");
        text = name;
    };
    for (std::size_t index = 0; index < symbol.path.size(); ++index) {
        const auto &part = symbol.path[index];
        if (std::holds_alternative<LangAPI::StorageOffset>(part)) {
            // a read that ends in an index goes through PyIndex (negative indices, str units, dict keys); anything that
            // is assigned to or continues into the element needs the native place
            if (!place && index + 1 == symbol.path.size()) {
                auto offset = convertExpression(std::get<LangAPI::StorageOffset>(part).offset);
                if (touches_self(res) && touches_self(offset)) hoist(offset);
                res = "::ispa_std::pycoll::PyIndex::py_index(&" + res + ", &(" + offset + "))";
                continue;
            }
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
                case LangAPI::ArrayMethods::Pop:
                case LangAPI::ArrayMethods::SequencePopBack: res += "pop"; break;
                case LangAPI::ArrayMethods::Size: res += "len"; break;
                case LangAPI::ArrayMethods::Extend: res += "extend"; break;
                case LangAPI::ArrayMethods::Clear: res += "clear"; break;
                case LangAPI::ArrayMethods::ShallowCopy: res += "clone"; break;
                // Python list/dict/`in` operations. Receiver-polymorphic ones go through ::ispa_std::pycoll traits
                case LangAPI::ArrayMethods::PythonAppend: res += "push"; break;
                case LangAPI::ArrayMethods::PythonExtend: res += "extend"; break;
                case LangAPI::ArrayMethods::PythonClear: res += "clear"; break;
                case LangAPI::ArrayMethods::PythonReverse: res += "reverse"; break;
                case LangAPI::ArrayMethods::PythonInsert: res += "insert"; break;
                case LangAPI::ArrayMethods::PythonContains: res += "py_contains"; break;
                case LangAPI::ArrayMethods::PythonPop:
                    res += method_call.args.empty() ? "pop" : "py_pop"; break;
                default:
                    throw Error("Rust target: source-language collection operation {}; lower it to language-neutral collection IR",
                                static_cast<int>(method_call.method));
            }
            std::vector<std::string> args;
            for (const auto &param : method_call.args)
                args.push_back(convertInitializerValue(LangAPI::Type{LangAPI::ValueType::Undef}, param));
            // a handle to `self` stored in a list of base-class handles
            if (method_call.method == LangAPI::ArrayMethods::PythonAppend || method_call.method == LangAPI::ArrayMethods::Push)
                for (auto& a : args) if (a.ends_with("instance has no shared owner\")")) a += ".upcast()";
            if (touches_self(res))
                for (auto& a : args) if (touches_self(a)) hoist(a);
            if (method_call.method == LangAPI::ArrayMethods::Push && args.empty())
                args.push_back("Default::default()");
            if ((method_call.method == LangAPI::ArrayMethods::Extend || method_call.method == LangAPI::ArrayMethods::PythonExtend) &&
                args.size() == 1)
                args[0] += ".into_iter()";   // the argument is an owned copy: the receiver may be reached through the same object
            if (method_call.method == LangAPI::ArrayMethods::PythonContains && args.size() == 1) args[0] = "&" + args[0];
            if (method_call.method == LangAPI::ArrayMethods::PythonInsert && args.size() == 2)
                args[0] = "(" + args[0] + ") as usize";
            res += "(" + joinStrings(args, ", ") + ")";
            if (method_call.method == LangAPI::ArrayMethods::PythonPop && method_call.args.empty())
                res += ".unwrap_or_else(|| ::ispa_std::fail(1, \"pop from empty list\"))";
            if (method_call.method == LangAPI::ArrayMethods::SequencePopBack)
                res += ".unwrap_or_else(|| ::ispa_std::fail(1, \"pop from empty sequence\"))";
        } else if (std::holds_alternative<LangAPI::Property>(part)) {
            res += ident(std::get<LangAPI::Property>(part).name) + "()";
        } else if (std::holds_alternative<LangAPI::IspaLibSymbol>(part)) {
            res += convertIspaLibSymbol(std::get<LangAPI::IspaLibSymbol>(part), true);
        } else {
            const auto& call = std::get<LangAPI::FunctionCall>(part);
            const auto saved_sink = hoist_sink;
            const auto saved_self = hoist_self;
            hoist_sink = &hoisted;
            hoist_self = touches_self(res);
            auto text = convertFunctionCall(call, true);
            hoist_sink = saved_sink;
            hoist_self = saved_self;
            // calling a Callable field: `(object.field)(args)`
            if (const auto* name = std::get_if<std::shared_ptr<LangAPI::Symbol>>(&call.name);
                name && (*name)->path.size() == 1 && std::holds_alternative<std::string>((*name)->path[0]) &&
                callable_fields.contains(std::get<std::string>((*name)->path[0]))) {
                const auto open = text.find('(');
                res.pop_back();   // the "." added above
                res = "(" + res + "." + text.substr(0, open) + ")" + text.substr(open);
                continue;
            }
            res += text;
        }
    }
    // collection sizes are the IR's `int` (i64), not Rust's usize
    if (!symbol.path.empty()) {
        if (const auto* last = std::get_if<LangAPI::ArrayMethodCall>(&symbol.path.back());
            last && last->method == LangAPI::ArrayMethods::Size)
            res = "(" + res + " as i64)";
    }
    if (!hoisted.empty()) res = "{ " + joinStrings(hoisted, " ") + " " + res + " }";
    return res;
}

auto RustCore::convertIspaLibSymbol(const LangAPI::IspaLibSymbol &symbol, bool expr) -> std::string {
    const auto args = convertTemplates(symbol.template_parameters);
    const auto g = [&] { return generic(args, expr); };
    // scope of the generated Lexer type; its `iterator` typedef becomes an associated type of a runtime trait
    const auto lexer_scope = "<" + crateRoot() + "::Lexer as ::ispa_std::LexerTypes>";
    switch (symbol.exports) {
        case LangAPI::StdlibExports::TextOutput: return "dyn ::std::fmt::Write";
        case LangAPI::StdlibExports::RecoveryHandler: return "::ispa_std::recovery::handler";
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
        case LangAPI::StdlibExports::LRValue: return "::std::boxed::Box<dyn ::std::any::Any>";
        case LangAPI::StdlibExports::LRParse: return "::ispa_std::lr_parse";
        case LangAPI::StdlibExports::Parser: return "::ispa_std::LLParser" + g();
        case LangAPI::StdlibExports::LexerMakeTokenParameter:
            // `const char*&`: the cursor is advanced through the reference (the const is the pointee's)
            return symbol.Reference ? "&mut ::ispa_std::Ptr" : "::ispa_std::Ptr";
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
        case LangAPI::StdlibExports::CloneSelfBox: return "::std::rc::Rc::new";
        case LangAPI::StdlibExports::LookaheadKind: return "::ispa_std::lookahead_kind";
        case LangAPI::StdlibExports::RecoveryEndOfInput: return "::ispa_std::recovery::END_OF_INPUT";
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
                if (i + 1 < expression.size() && expression[i + 1].isExpressionElement() &&
                    (expression[i + 1].getExpressionElement() == LangAPI::ExpressionElement::PlusPlus ||
                     expression[i + 1].getExpressionElement() == LangAPI::ExpressionElement::MinusMinus)) {
                    const auto step = expression[++i].getExpressionElement();
                    out << "{ let __slot = &mut (" << convertRValue(expr.getRValue()) << "); let __old = *__slot; *__slot "
                        << (step == LangAPI::ExpressionElement::PlusPlus ? "+=" : "-=") << " 1; __old }";
                } else if ([&] {   // the left operand of `+`, possibly closing groups first
                               auto next = i + 1;
                               while (next < expression.size() && expression[next].isExpressionElement() &&
                                      expression[next].getExpressionElement() == LangAPI::ExpressionElement::GroupClose) ++next;
                               return next < expression.size() && expression[next].isExpressionElement() &&
                                      expression[next].getExpressionElement() == LangAPI::ExpressionElement::Add;
                           }() && (expr.getRValue().isSymbol() || expr.getRValue().isStorageSymbol()))
                    out << asExpr(convertRValue(expr.getRValue())) << ".clone()";   // `+` consumes its left operand (String)
                else out << convertRValue(expr.getRValue());
                break;
            case LangAPI::ExpressionValueType::EmptyInitializer:
                out << "Default::default()";
                break;
            case LangAPI::ExpressionValueType::ExpressionElement: {
                const auto element = expr.getExpressionElement();
                if (element == LangAPI::ExpressionElement::PlusPlus || element == LangAPI::ExpressionElement::MinusMinus) {
                    if (i + 1 == expression.size() || !expression[i + 1].isRvalue())
                        throw Error("Rust target: increment/decrement requires a single assignable operand in IR");
                    out << "{ let __slot = &mut (" << convertRValue(expression[++i].getRValue()) << "); *__slot "
                        << (element == LangAPI::ExpressionElement::PlusPlus ? "+=" : "-=") << " 1; *__slot }";
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
                // a function returning `&T` hands out a borrow, not a clone
                const bool borrowed_result = return_type && isValueTypeOf(*return_type, LangAPI::ValueType::BorrowSemantic) &&
                                             !expr.getReturn().value.empty();
                const auto value = borrowed_result ? argument(*return_type, expr.getReturn().value, convertExpression(expr.getReturn().value))
                                   : return_type ? convertInitializer(*return_type, expr.getReturn().value)
                                                 : convertExpression(expr.getReturn().value);
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
                if (var.type == LangAPI::OperatorType::Assign && std::holds_alternative<LangAPI::StorageSymbol>(var.name) &&
                    !std::get<LangAPI::StorageSymbol>(var.name).path.empty() &&
                    std::holds_alternative<LangAPI::StorageOffset>(std::get<LangAPI::StorageSymbol>(var.name).path.back())) {
                    // `container[key] = value`: Python semantics (negative index, dict insert) in the runtime
                    auto base = std::get<LangAPI::StorageSymbol>(var.name);
                    const auto key = convertExpression(std::get<LangAPI::StorageOffset>(base.path.back()).offset);
                    base.path.pop_back();
                    out << "{ let __k = " << key << "; let __v = "
                        << convertInitializerValue(LangAPI::Type{LangAPI::ValueType::Undef}, var.value) << "; ::ispa_std::pycoll::py_set_index(&mut "
                        << convertStorageSymbol(base, true) << ", __k, __v) }";
                    break;
                }
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
                    out << convertStorageSymbol(std::get<LangAPI::StorageSymbol>(var.name), true);
                {
                    auto value = var.type == LangAPI::OperatorType::Assign ? convertInitializerValue(LangAPI::Type{LangAPI::ValueType::Undef}, var.value)
                                                                         : convertExpression(var.value);
                    if (var.type == LangAPI::OperatorType::Assign && var.value.size() == 1 &&
                        var.value[0].type() == LangAPI::ExpressionValueType::RValue &&
                        var.value[0].getRValue().type() == LangAPI::RValueType::Symbol) {
                        const auto& source = var.value[0].getRValue().getSymbol();
                        if (source.path.size() == 1 && std::holds_alternative<std::string>(source.path[0]) &&
                            callable_parameters.contains(std::get<std::string>(source.path[0])))
                            value = "Box::new(" + ident(std::get<std::string>(source.path[0])) + ")";
                    }
                    std::string target_name;
                    if (const auto* symbol_name = std::get_if<LangAPI::Symbol>(&var.name)) {
                        if (!symbol_name->path.empty())
                            if (const auto* last = std::get_if<std::string>(&symbol_name->path.back())) target_name = *last;
                    } else if (const auto& storage = std::get<LangAPI::StorageSymbol>(var.name); !storage.path.empty()) {
                        if (const auto* last = std::get_if<std::string>(&storage.path.back())) target_name = *last;
                    }
                    // a value stored into a variant-typed local becomes one of its alternatives
                    if (var.type == LangAPI::OperatorType::Assign && std::holds_alternative<LangAPI::Symbol>(var.name) &&
                        std::get<LangAPI::Symbol>(var.name).path.size() == 1 && !target_name.empty()) {
                        if (const auto it = local_types.find(target_name); it != local_types.end() &&
                            (isValueTypeOf(*it->second, LangAPI::ValueType::Variant) || isValueTypeOf(*it->second, LangAPI::ValueType::LargeVariant)))
                            value = asExpr(value) + ".into()";
                    }
                    if (var.type == LangAPI::OperatorType::Assign) {
                        if (!target_name.empty() && any_fields.contains(ident(target_name))) {
                            const bool none = var.value.size() == 1 && var.value[0].isRvalue() &&
                                              var.value[0].getRValue().type() == LangAPI::RValueType::Int && var.value[0].getRValue().getInt().value == 0;
                            value = none ? "Default::default()" : "::ispa_std::pycoll::to_any(" + value + ")";
                        }
                    }
                    out << " " << op << " " << value;
                }
                break;
            }
            // `pos` is a Copy cursor (::ispa_std::Ptr); inc() advances it and returns the new value, so it also works
            // inside larger expressions where `+= 1` would not.
            case LangAPI::ExpressionValueType::CounterIncreament:
                out << counter.back() << ".inc()";
                break;
            case LangAPI::ExpressionValueType::CounterAssignment:
                out << counter.back() << " = " << convertInitializerValue(LangAPI::Type{LangAPI::ValueType::Undef}, *expr.getCounterAssignment().v);
                break;
            case LangAPI::ExpressionValueType::ResetPosCounter:
                counter.pop_back();
                break;
            case LangAPI::ExpressionValueType::PushPosCounter: {
                const auto &a = expr.getPushPosCounter();
                out << "let mut " << ident(a.name) << " = " << counter.back() << ".clone()";
                counter.push_back(ident(a.name));
                break;
            }
            case LangAPI::ExpressionValueType::PopPosCounter: {
                out << counter[counter.size() - 2] << " = " << counter.back() << ".clone()";
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

auto RustCore::convertInitializer(const LangAPI::Type& type, const LangAPI::Expression& value) -> std::string {
    if (convertType(type).starts_with("::ispa_std::Shared<") && value.size() == 1 && value[0].isRvalue() &&
        value[0].getRValue().type() == LangAPI::RValueType::Int && value[0].getRValue().getInt().value == 0)
        return "Default::default()";   // None
    auto result = convertInitializerValue(type, value);
    // collection sizes and indices are the IR's `int` (i64); an unsigned target takes them by a numeric cast
    if (!result.empty() && convertType(type) == "usize" && result.find(" as i64)") != std::string::npos) return "((" + result + ") as usize)";
    // a handle to an object is accepted wherever a handle to one of its base classes is expected
    if (!result.empty() && convertType(type).starts_with("::ispa_std::Shared<")) return "(" + result + ").upcast()";
    return result;
}

static auto soleRValue(const LangAPI::Expression &e) -> const LangAPI::RValue *;
auto RustCore::convertInitializerValue(const LangAPI::Type& type, const LangAPI::Expression& value) -> std::string {
    if (value.empty()) return "";
    if (value.size() > 1) {
        // `(x)`: grouping around a single value does not change what it is
        if (const auto* sole = soleRValue(value)) {
            for (std::size_t i = 0; i < value.size(); ++i)
                if (value[i].isRvalue() && &value[i].getRValue() == sole) {
                    LangAPI::Expression inner;
                    inner.push_back(value[i]);
                    return convertInitializerValue(type, inner);
                }
        }
    }
    if (isValueTypeOf(type, LangAPI::ValueType::Any)) {
        if (value.size() == 1 && value[0].isRvalue() && value[0].getRValue().type() == LangAPI::RValueType::Int &&
            value[0].getRValue().getInt().value == 0)
            return "Default::default()";   // None
        return "::ispa_std::pycoll::to_any(" + convertInitializerValue(LangAPI::Type{LangAPI::ValueType::Undef}, value) + ")";
    }
    if (isValueTypeOf(type, LangAPI::ValueType::ImmutableSemantic) && type.template_parameters.size() == 1 &&
        std::holds_alternative<LangAPI::Type>(type.template_parameters[0]))
        return convertInitializerValue(std::get<LangAPI::Type>(type.template_parameters[0]), value);
    if (value.size() == 1 && value[0].isRvalue()) {
        const auto& literal = value[0].getRValue();
        if (literal.isString()) {
            const auto raw = "\"" + rustStringBody(literal.getString().value) + "\"";
            if (isValueTypeOf(type, LangAPI::ValueType::String))
                return "String::from(" + raw + ")";
            if (isValueTypeOf(type, LangAPI::ValueType::NonOwnedString))
                return "::ispa_std::Ptr::from_static(" + raw + ")";
        }
        if (isValueTypeOf(type, LangAPI::ValueType::FixedSizeArray) &&
            (literal.type() == LangAPI::RValueType::Array || literal.type() == LangAPI::RValueType::FixedSizeArray)) {
            if (type.template_parameters.size() != 2 || !std::holds_alternative<LangAPI::Type>(type.template_parameters[0]))
                throw Error("Rust target: FixedSizeArray requires an element type and length");
            const auto& element = std::get<LangAPI::Type>(type.template_parameters[0]);
            const auto& elements = literal.type() == LangAPI::RValueType::Array ? literal.getArray().values : literal.getFixedSizeArray().values;
            std::vector<std::string> rendered;
            for (const auto& item : elements) rendered.push_back(convertInitializer(element, item));
            return "[" + joinStrings(rendered, ", ") + "]";
        }
    }
    // `T& x = object.field;` / `= list[i]` binds the place itself
    if (isValueTypeOf(type, LangAPI::ValueType::BorrowSemantic) && type.template_parameters.size() == 1 &&
        std::holds_alternative<LangAPI::Type>(type.template_parameters[0]) && value.size() == 1 && value[0].isRvalue()) {
        const auto& inner = std::get<LangAPI::Type>(type.template_parameters[0]);
        const auto prefix = std::string(isValueTypeOf(inner, LangAPI::ValueType::ImmutableSemantic) ? "&" : "&mut ");
        const auto& source = value[0].getRValue();
        if (source.isStorageSymbol()) return prefix + convertStorageSymbol(source.getStorageSymbol(), true);
        if (source.isSymbol() && source.getSymbol().path.size() > 1 && std::holds_alternative<std::string>(source.getSymbol().path.back()))
            return prefix + convertExpression(value);
    }
    if (value.size() == 1 && value[0].isRvalue() && value[0].getRValue().isSymbol()) {
        const auto& source = value[0].getRValue().getSymbol();
        if (source.path.size() == 1 && std::holds_alternative<std::string>(source.path[0]) &&
            callable_parameters.contains(std::get<std::string>(source.path[0])))
            return convertExpression(value);   // `(&mut f)`: a reborrow, closures are not Clone
    }
    if (value.size() == 1 && value[0].isRvalue() &&
        (value[0].getRValue().isSymbol() || value[0].getRValue().isStorageSymbol() ||
         (value[0].getRValue().type() == LangAPI::RValueType::Pos && !value[0].getRValue().getPos().dereference)) &&
        !isValueTypeOf(type, LangAPI::ValueType::BorrowSemantic) && !isValueTypeOf(type, LangAPI::ValueType::MoveSemantic))
        return asExpr(convertExpression(value)) + ".clone()";
    return convertExpression(value);
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
        case LangAPI::ExpressionElement::Add: return " + &";   // String + &String and i64 + &i64 both exist
        case LangAPI::ExpressionElement::Minus: return " - ";
        case LangAPI::ExpressionElement::Multiply: return " * ";
        case LangAPI::ExpressionElement::Divide: return " / ";
        case LangAPI::ExpressionElement::Modulo: return " % ";
        default:
            throw Error("Unknown expression element");
    }
}

auto RustCore::convertLambda(const LangAPI::Lambda &lambda) -> std::string {
    const auto saved_frames = loop_frames;
    const auto saved_return_type = return_type;
    const auto saved_borrowed = borrowed_parameters;
    locals.emplace_back();
    loop_frames.clear();
    return_type = nullptr;
    std::vector<std::string> params;
    for (const auto &p : lambda.parameters) {
        bind(p.second);
        if (isValueTypeOf(p.first, LangAPI::ValueType::BorrowSemantic)) borrowed_parameters.insert(p.second);
        if (p.first.isSymbol() && p.first.getSymbol().path.size() == 1 && std::holds_alternative<std::string>(p.first.getSymbol().path[0]) &&
            std::get<std::string>(p.first.getSymbol().path[0]) == "auto&") reference_parameters.insert(p.second);   // `auto&`: a lent argument
        const auto type = convertType(p.first);
        params.push_back(ident(p.second) + (type == "_" ? "" : ": " + type));
    }
    // the closure may be handed to a method of the object it uses: it reaches the object through a raw pointer
    // taken outside, so it does not hold `self` borrowed while the callee runs
    const auto saved_receiver = receiver;
    std::string entry;
    if (!receiver.empty()) {
        entry = "let __this = unsafe { &mut *__p };\n";   // `__p` is the object's pointer, taken once at the top of the method
        receiver = "__this";
    }
    const auto body = lambda_body(lambda.statements);
    receiver = saved_receiver;
    locals.pop_back();
    loop_frames = saved_frames;
    return_type = saved_return_type;
    borrowed_parameters = saved_borrowed;
    return "|" + joinStrings(params, ", ") + "| {\n" + entry + body + "}";
}

// An argument for a runtime helper (which takes values): a copy of a place, except a reference parameter or the cursor of
// a semantic action, which are lent.
auto RustCore::runtimeArgument(const LangAPI::Expression &expression, const std::string &rendered) -> std::string {
    if (expression.size() == 1 && expression[0].isRvalue()) {
        const auto &rvalue = expression[0].getRValue();
        if (rvalue.isSymbol() && rvalue.getSymbol().path.size() == 1 && std::holds_alternative<std::string>(rvalue.getSymbol().path[0])) {
            const auto &name = std::get<std::string>(rvalue.getSymbol().path[0]);
            if (reference_parameters.contains(name) || (borrowed_parameters.contains(name) && !const_borrowed_parameters.contains(name)))
                return "&mut *" + ident(name);
            if (lent_locals.contains(ident(name))) return "&mut " + ident(name);
        }
        if (rvalue.type() == LangAPI::RValueType::Pos && !rvalue.getPos().dereference && rendered.find("(*") != std::string::npos &&
            rendered.find(".offset(") == std::string::npos)
            return "&mut " + rendered;
    }
    return convertInitializerValue(LangAPI::Type{LangAPI::ValueType::Undef}, expression);
}

auto RustCore::convertFunctionCall(const LangAPI::FunctionCall &call, bool member) -> std::string {
    auto* const sink = hoist_sink;
    const bool hoist_self_call = hoist_self;
    hoist_sink = nullptr;
    hoist_self = false;
    std::vector<std::string> args;
    for (const auto &param : call.args) args.push_back(convertExpression(param));
    const auto raw_args = args;

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
    if (const auto* library = std::get_if<std::shared_ptr<LangAPI::IspaLibSymbol>>(&call.name);
        library && (*library)->exports == LangAPI::StdlibExports::RecoveryHandler) {
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
        static constexpr const char* defaults[] = {"true", "true", "true", "64"};
        if (policy.size() > 4) throw Error("Rust Converter: invalid recovery policy length");
        for (std::size_t i = policy.size(); i < 4; ++i) policy.push_back(defaults[i]);
        return "::ispa_std::recovery::handler(" + joinStrings(sets, ", ") + ", " +
               joinStrings(policy, ", ") + ", " + joinStrings(args, ", ") + ")";
    }
    if (std::holds_alternative<std::shared_ptr<LangAPI::Symbol>>(call.name)) {
        const auto &symbol = *std::get<std::shared_ptr<LangAPI::Symbol>>(call.name);
        // a method name after `.` never refers to a local or member of the same spelling
        const auto symbol_name = convertSymbol(symbol, !member);
        if (symbol_name == "custom_failure" && !member && !counter.empty()) {
            // the cursor is named so the block's parameters have a known type
            const auto object = receiver.empty() ? std::string("self") : receiver;
            return object + ".custom_failure(&" + counter.back() + (args.empty() ? "" : ", " + joinStrings(args, ", ")) + ")";
        }
        if (symbol_name == "custom_failure_decision" && !member && args.size() == 3) {
            // the context holds the failing position: the decision is taken from it, not from a second borrow of the cursor
            const auto object = receiver.empty() ? std::string("self") : receiver;
            return object + ".custom_failure_decision(" + runtimeArgument(call.args[0], args[0]) + ", " + runtimeArgument(call.args[2], args[2]) + ")";
        }
        if (symbol_name == "recovery_at" || symbol_name == "recover_choice") {
            std::vector<std::string> sets;
            for (const auto& parameter : call.template_parameters) sets.push_back(recovery_set(parameter));
            // the receiver is this object's parser base, whichever name it is reached by
            const auto object = receiver.empty() ? std::string("self") : receiver;
            if (symbol_name == "recovery_at") {
                // (expected text, claim labels...) -> (expected text, [labels])
                std::vector<std::string> labels(args.begin() + (args.empty() ? 0 : 1), args.end());
                return object + ".recovery_at(&[" + joinStrings(sets, ", ") + "], " + (args.empty() ? "String::new()" : args.front()) +
                       ", &[" + joinStrings(labels, ", ") + "])";
            }
            // recover_choice(pos, rule, expected): the cursor is advanced in place
            if (!args.empty()) args[0] = "&mut " + args[0];
            return object + ".recover_choice(&[" + joinStrings(sets, ", ") + "]" +
                   (args.empty() ? "" : ", " + joinStrings(args, ", ")) + ")";
        }
        // Python-frontend object helpers: ISPA_STD.construct[T](args) allocates a shared instance and runs `__init__`,
        // make_value[T](args) builds an unshared one, borrow(x) passes x by mutable reference.
        if (symbol_name == "ISPA_STD::construct" || symbol_name == "ISPA_STD::make_value") {
            if (call.template_parameters.size() != 1)
                throw Error("Rust Converter: `{}` needs exactly one class argument", symbol_name);
            auto cls = convertTemplates(call.template_parameters);
            if (const auto* type = std::get_if<std::shared_ptr<LangAPI::Type>>(&call.template_parameters.front());
                type && *type && (**type).isSymbol() && (**type).template_parameters.empty() && !(**type).getSymbol().path.empty())
                if (const auto* last = std::get_if<std::string>(&(**type).getSymbol().path.back()))
                    if (const auto it = class_generics.find(*last); it != class_generics.end() && it->second > 0) {
                        cls += "<";   // inferred from the use
                        for (std::size_t i = 0; i < it->second; ++i) cls += i ? ", _" : "_";
                        cls += ">";
                    }
            if (const auto* type = std::get_if<std::shared_ptr<LangAPI::Type>>(&call.template_parameters.front());
                type && *type && (**type).isSymbol()) {
                auto init = (**type).getSymbol();
                init.path.emplace_back(std::string("__init__"));
                if (const auto* function = resolveFunction(init)) {
                    if (function->parameters.size() != call.args.size())
                        throw Error("Rust target: wrong argument count constructing `{}`", cls);
                    for (std::size_t i = 0; i < args.size(); ++i) {
                        args[i] = argument(function->parameters[i].first, call.args[i], args[i]);
                        // a callable handed to a constructor is owned by the new object: moved, not reborrowed
                        if (isValueTypeOf(function->parameters[i].first, LangAPI::ValueType::Callable) && args[i].starts_with("(&mut ") &&
                            args[i].ends_with(")"))
                        { args[i] = args[i].substr(6, args[i].size() - 7); callable_moved = true; }
                    }
                }
            }
            const auto init = "__init__(" + joinStrings(args, ", ") + ")";
            // a reference-semantic class has no unshared form: a "value" of it is its own object
            const bool shared_class = call.template_parameters.size() == 1 &&
                std::holds_alternative<std::shared_ptr<LangAPI::Type>>(call.template_parameters.front()) &&
                isSharedClassType(*std::get<std::shared_ptr<LangAPI::Type>>(call.template_parameters.front()));
            if (symbol_name == "ISPA_STD::construct" || shared_class)
                return "{ let __object = <" + cls + ">::shared_default(); __object.borrow_mut()." + init + "; __object }";
            return "{ let mut __value = <" + cls + ">::default(); __value." + init + "; __value }";
        }
        if (symbol_name.starts_with("ISPA_STD::python_"))
            return "::ispa_std::pycoll::" + symbol_name.substr(std::string("ISPA_STD::python_").size()) + "(" +
                   [&] { for (auto& a : args) a = "&" + a; return joinStrings(args, ", "); }() + ")";
        if (symbol_name == "ISPA_STD::borrow") {
            if (args.size() != 1) throw Error("Rust Converter: `ISPA_STD::borrow` takes one argument");
            if (call.args.size() == 1 && call.args[0].size() == 1 && call.args[0][0].isRvalue() && call.args[0][0].getRValue().isSymbol() &&
                shared_locals.contains(args[0]))
                return args[0] + ".clone()";   // a handle is already a reference
            return "&mut " + args[0];
        }
        if (!member && symbol.path.size() == 1 && std::holds_alternative<std::string>(symbol.path[0])) {
            if (const auto it = callable_types.find(std::get<std::string>(symbol.path[0])); it != callable_types.end()) {
                const auto& parameters = it->second->template_parameters;   // arguments..., result
                if (parameters.size() == call.args.size() + 1)
                    for (std::size_t i = 0; i < args.size(); ++i)
                        if (const auto* declared = std::get_if<LangAPI::Type>(&parameters[i]))
                            args[i] = convertInitializer(*declared, call.args[i]);
            }
        }
        if (member && !member_class.empty() && symbol.path.size() == 1 && std::holds_alternative<std::string>(symbol.path[0]) &&
            class_methods.contains(member_class + "::" + std::get<std::string>(symbol.path[0]))) {
            // `receiver.method(args)` with a known receiver class
            const auto it = class_methods.find(member_class + "::" + std::get<std::string>(symbol.path[0]));
            if (it->second.size() == 1 && it->second.front()->parameters.size() == call.args.size())
                for (std::size_t i = 0; i < args.size(); ++i)
                    args[i] = argument(it->second.front()->parameters[i].first, call.args[i], args[i]);
        } else if (member && symbol.path.size() == 1 && std::holds_alternative<std::string>(symbol.path[0])) {
            if (const auto* method = resolveMethodByName(std::get<std::string>(symbol.path[0]), call.args.size()))
                for (std::size_t i = 0; i < args.size(); ++i)
                    args[i] = argument(method->parameters[i].first, call.args[i], args[i]);
        } else if (const auto* function = resolveFunction(symbol)) {
            // `Base.method(self, ...)`: explicit receiver; UFCS, and DerefMut coerces `&mut Derived` to `&mut Base`
            const std::size_t receiver_args = !function->is_static && call.args.size() == function->parameters.size() + 1 ? 1 : 0;
            if (function->parameters.size() + receiver_args != call.args.size())
                throw Error("Rust target: wrong argument count for `{}`", symbol_name);
            for (std::size_t i = receiver_args; i < args.size(); ++i)
                args[i] = argument(function->parameters[i - receiver_args].first, call.args[i], args[i]);
            // the receiver is `self` itself, not a handle to it: the caller is already borrowed
            if (receiver_args && call.args[0].size() == 1 && call.args[0][0].isRvalue() &&
                call.args[0][0].getRValue().type() == LangAPI::RValueType::Instance && call.args[0][0].getRValue().getInstance().handle)
                args[0] = receiver;
        }
        // arguments passed by value out of a place are copies
        for (std::size_t i = 0; i < args.size(); ++i)
            if (args[i] == raw_args[i]) {
                // a namespace-level table handed to a runtime method is lent, not copied
                if (args[i].starts_with("(*") && args[i].ends_with(".get())")) { args[i] = "&" + args[i]; continue; }
                args[i] = runtimeArgument(call.args[i], args[i]);   // a copy, or a lent reference parameter / fail context
            }
        auto callee = convertSymbol(symbol, !member);
        // a bare call that names nothing in scope is an inherited runtime method of the base class
        if (!member && !receiver.empty() && symbol.path.size() == 1 && std::holds_alternative<std::string>(symbol.path[0]) &&
            !resolveFunction(symbol) && !callable_parameters.contains(std::get<std::string>(symbol.path[0])) &&
            !members.contains(std::get<std::string>(symbol.path[0])) &&
            !std::any_of(locals.rbegin(), locals.rend(), [&](const auto& frame) { return frame.contains(std::get<std::string>(symbol.path[0])); }))
            callee = receiver + "." + callee;
        // the DFA entry point with debugger tables (debug instrumentation on) is a separate method here
        if (!member && args.size() == 8 && callee.ends_with(".lookup")) callee += "_debug";
        if (!member)   // `Base.method(self, ...)` on a dispatching method runs the base implementation, not the dispatcher
            if (const auto* function = resolveFunction(symbol); function && (slot_functions.contains(function) || overrides.contains(function))) {
                const auto at = callee.rfind("::");
                const auto method = at == std::string::npos ? callee : callee.substr(at + 2);
                callee = (at == std::string::npos ? std::string() : callee.substr(0, at + 2)) +
                         (overrides.contains(function) ? "__impl_" : "__base_") + method;
            }
        // arguments are evaluated before the call borrows its receiver / `&mut` operands: bind the ones that would clash
        std::vector<std::string> pending;
        const auto bind_arg = [&](std::string& a) {
            if (a.starts_with("|") || a.starts_with("move |")) return;
            const auto name = "__h" + std::to_string(++hoist_counter);
            pending.push_back("let " + name + " = " + a + ";");
            a = name;
        };
        const auto mentions = [](const std::string& text, const std::string& name) {
            for (std::size_t at = text.find(name); at != std::string::npos; at = text.find(name, at + 1)) {
                const auto word = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; };
                if ((at == 0 || !word(text[at - 1])) && (at + name.size() >= text.size() || !word(text[at + name.size()]))) return true;
            }
            return false;
        };
        const bool self_call = !receiver.empty() && callee.starts_with(receiver + ".");
        const auto touches_receiver = [&](const std::string& text) {
            if (text.find("(" + receiver + ")") != std::string::npos) return true;
            for (auto at = text.find(receiver + "."); at != std::string::npos; at = text.find(receiver + ".", at + 1))
                if (at == 0 || !(std::isalnum(static_cast<unsigned char>(text[at - 1])) || text[at - 1] == '_' || text[at - 1] == '.')) return true;
            return false;
        };
        for (std::size_t i = 0; i < args.size(); ++i) {
            if (((sink && hoist_self_call) || self_call) && !receiver.empty() && touches_receiver(args[i])) { bind_arg(args[i]); continue; }
            if (args[i].starts_with("&")) continue;
            for (std::size_t other = 0; other < args.size(); ++other) {
                if (other == i || !args[other].starts_with("&mut ")) continue;
                auto target = args[other].substr(5);
                while (!target.empty() && (target.front() == '(' || target.front() == '*')) target.erase(0, 1);
                std::string name;
                for (const char c : target) { if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') name += c; else break; }
                if (!name.empty() && mentions(args[i], name)) { bind_arg(args[i]); break; }
            }
        }
        if (!pending.empty()) {
            if (sink) sink->insert(sink->end(), pending.begin(), pending.end());
            else {
                return "{ " + joinStrings(pending, " ") + " " + callee + generic(convertTemplates(call.template_parameters), true) + "(" + joinStrings(args, ", ") + ") }";
            }
        }
        return callee + generic(convertTemplates(call.template_parameters), true) + "(" + joinStrings(args, ", ") + ")";
    }
    const auto &lib = *std::get<std::shared_ptr<LangAPI::IspaLibSymbol>>(call.name);
    std::string callee = convertIspaLibSymbol(lib, true);
    if (lib.exports == LangAPI::StdlibExports::Error) callee += "::new";           // std::runtime_error(msg) -> Error::new(msg)
    for (std::size_t i = 0; i < args.size(); ++i)   // runtime helpers take their arguments by value
        if (args[i] == raw_args[i]) args[i] = runtimeArgument(call.args[i], args[i]);
    return callee + generic(convertTemplates(call.template_parameters), true) + "(" + joinStrings(args, ", ") + ")";
}

auto RustCore::convertIspaLibFunctionCall(const LangAPI::IspaLibFunctionCall &call) -> std::string {
    const auto kind = call.symbol.exports;
    if (kind == LangAPI::StdlibExports::TokenCursor || kind == LangAPI::StdlibExports::LazyTokenCursor ||
        kind == LangAPI::StdlibExports::ResetTokenStream) {
        const auto expected = kind == LangAPI::StdlibExports::LazyTokenCursor ? 2u : 1u;
        if (call.args.size() != expected) throw Error("invalid token cursor operation arguments");
        const auto method = kind == LangAPI::StdlibExports::TokenCursor ? "token_cursor" :
            kind == LangAPI::StdlibExports::LazyTokenCursor ? "lazy_token_cursor" : "reset_token_stream";
        // the second operand is read before the first is lent out
        if (expected == 2)
            return "{ let __input = " + convertInitializerValue(LangAPI::Type{LangAPI::ValueType::Undef}, call.args[1]) +
                   "; ispa_std::TokenStream::" + method + "(&mut " + convertExpression(call.args[0]) + ", __input) }";
        return std::string("ispa_std::TokenStream::") + method + "(&mut " + convertExpression(call.args[0]) + ")";
    }
    if (kind == LangAPI::StdlibExports::Fail || kind == LangAPI::StdlibExports::Attempt ||
        kind == LangAPI::StdlibExports::FailureMessage) {
        const auto name = kind == LangAPI::StdlibExports::Fail ? "fail" :
            kind == LangAPI::StdlibExports::Attempt ? "attempt" : "failure_message";
        std::vector<std::string> arguments;
        for (const auto &argument : call.args) arguments.push_back(convertInitializerValue(LangAPI::Type{LangAPI::ValueType::Undef}, argument));
        return std::string("::ispa_std::") + name + "(" + joinStrings(arguments, ", ") + ")";
    }
    if (kind == LangAPI::StdlibExports::CloneSelfBox)
        return "::ispa_std::Shared::new(::std::clone::Clone::clone(self))";   // a handle to a copy of the whole object
    const bool parser_match = kind == LangAPI::StdlibExports::ParserRequired ||
        kind == LangAPI::StdlibExports::ParserOptional ||
        kind == LangAPI::StdlibExports::ParserRange ||
        kind == LangAPI::StdlibExports::ParserRangeOptional;
    const bool parser_call = parser_match || kind == LangAPI::StdlibExports::ParserCounted;

    std::vector<std::string> args;
    bool has_callback = false;
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
            has_callback = true;
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
        if (!args.empty()) args[0] = "&mut " + args[0];   // the cursor is advanced in place
        // an argument that reads the cursor (a fail block naming it) is evaluated before the cursor is lent
        std::vector<std::string> pending;
        if (!args.empty()) {
            const auto cursor = args[0].substr(5);
            for (std::size_t i = 1; i < args.size(); ++i) {
                bool mentions = false;
                for (auto at = args[i].find(cursor); at != std::string::npos && !mentions; at = args[i].find(cursor, at + 1)) {
                    const auto word = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; };
                    mentions = (at == 0 || !word(args[i][at - 1])) && (at + cursor.size() >= args[i].size() || !word(args[i][at + cursor.size()]));
                }
                if (!mentions || args[i].starts_with("|")) continue;
                const auto name = "__h" + std::to_string(++hoist_counter);
                pending.push_back("let " + name + " = " + args[i] + ";");
                args[i] = name;
            }
        }
        if (!pending.empty()) {
            std::string joined = args.empty() ? "" : ", " + joinStrings(args, ", ");
            return "{ " + joinStrings(pending, " ") + " ::ispa_std::LLParserApi::" + convertIspaLibSymbol(call.symbol, true) + (has_callback ? "_rule" : "") +
                   turbofish + "(self" + joined + ") }";
        }
        // inherited LLParser members: reached through the LLParserApi trait implemented by the generated parser
        std::string joined = args.empty() ? "" : ", " + joinStrings(args, ", ");
        return std::string(call.template_parameters.size() > 1 ? "::ispa_std::LLParserSpaced::" : "::ispa_std::LLParserApi::") +
               convertIspaLibSymbol(call.symbol, true) + (has_callback ? "_rule" : "") +
               (call.template_parameters.size() > 1 ? "_spaced" : "") + turbofish +
               "(self" + joined + ")";
    }
    std::string callee = convertIspaLibSymbol(call.symbol, true);
    if (kind == LangAPI::StdlibExports::Concat) {
        // variadic: every part by reference as something that can be turned into text
        std::vector<std::string> parts;
        for (const auto &argument : call.args) parts.push_back("&" + asExpr(convertExpression(argument)) + " as &dyn ::ispa_std::ToText");
        return "::ispa_std::concat(&[" + joinStrings(parts, ", ") + "])";
    }
    if (kind == LangAPI::StdlibExports::Error) callee += "::new";
    if (kind == LangAPI::StdlibExports::AssignCompatible && call.args.size() == 2) {
        // assign_compatible(target, source): the target is written through, the source is read
        const auto* target = call.args[0].size() == 1 && call.args[0][0].isRvalue() ? &call.args[0][0].getRValue() : nullptr;
        std::string place = target && target->isStorageSymbol() ? convertStorageSymbol(target->getStorageSymbol(), true) : convertExpression(call.args[0]);
        // lists of nodes convert element by element
        const auto* to = target ? typeOfExpression(call.args[0]) : nullptr;
        const auto* from = typeOfExpression(call.args[1]);
        const bool lists = to && from && isValueTypeOf(*to, LangAPI::ValueType::Array) && isValueTypeOf(*from, LangAPI::ValueType::Array) &&
                           convertType(*to) != convertType(*from);
        std::string converter;
        if (lists) {
            // a list of groups (tuples): converted member by member
            const auto elements = [](const LangAPI::Type& list) -> const LangAPI::Type* {
                return list.template_parameters.size() == 1 && std::holds_alternative<LangAPI::Type>(list.template_parameters[0])
                           ? &std::get<LangAPI::Type>(list.template_parameters[0]) : nullptr; };
            const auto* to_element = elements(*to);
            const auto* from_element = elements(*from);
            if (to_element && from_element && isValueTypeOf(*to_element, LangAPI::ValueType::Tuple) && isValueTypeOf(*from_element, LangAPI::ValueType::Tuple) &&
                to_element->template_parameters.size() == from_element->template_parameters.size()) {
                std::string assigns;
                for (std::size_t i = 0; i < to_element->template_parameters.size(); ++i)
                    assigns += "::ispa_std::AssignFrom::assign_from(&mut converted." + std::to_string(i) + ", item." + std::to_string(i) + "); ";
                return "::ispa_std::assign_list_with(&mut " + place + ", " +
                       convertInitializerValue(LangAPI::Type{LangAPI::ValueType::Undef}, call.args[1]) +
                       ", |item| { let mut converted: " + convertType(*to_element) + " = Default::default(); " + assigns + "converted })";
            }
        }
        return (lists ? std::string("::ispa_std::assign_list") : callee) + "(&mut " + place + ", " +
               convertInitializerValue(LangAPI::Type{LangAPI::ValueType::Undef}, call.args[1]) + ")";
    }
    for (std::size_t i = 0; i < args.size() && i < call.args.size(); ++i)   // runtime helpers take their arguments by value
        args[i] = runtimeArgument(call.args[i], args[i]);
    return callee + turbofish + "(" + joinStrings(args, ", ") + ")";
}

auto RustCore::convertMakeTuple(const LangAPI::MakeTuple &make_tuple) -> std::string {
    std::vector<std::string> parts;
    for (const auto &el : make_tuple.args) parts.push_back(convertInitializerValue(LangAPI::Type{LangAPI::ValueType::Undef}, el));
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
            return "String::from(\"" + rustStringBody(rvalue.getString().value) + "\")";
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
            for (const auto &a : inheritance.args) {
                auto text = convertExpression(a);
                // a borrowed parameter handed on to a constructor is passed as the borrow it is
                if (a.size() == 1 && a[0].isRvalue() && a[0].getRValue().isSymbol() && a[0].getRValue().getSymbol().path.size() == 1 &&
                    std::holds_alternative<std::string>(a[0].getRValue().getSymbol().path[0]) &&
                    borrowed_parameters.contains(std::get<std::string>(a[0].getRValue().getSymbol().path[0])))
                    text = ident(std::get<std::string>(a[0].getRValue().getSymbol().path[0]));
                args.push_back(text);
            }
            if (base == "String") {
                if (args.empty()) return "String::new()";
                if (args.size() == 1) return "String::from(" + args[0] + ")";
                if (args.size() == 2) return "::ispa_std::string_range(" + args[0] + ", " + args[1] + ")";
            }
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
        case LangAPI::RValueType::Reference: {
            const auto& inner = *rvalue.getReference().value;
            // a reference parameter taken a reference of is lent again
            if (inner.isSymbol() && inner.getSymbol().path.size() == 1 && std::holds_alternative<std::string>(inner.getSymbol().path[0]) &&
                reference_parameters.contains(std::get<std::string>(inner.getSymbol().path[0])))
                return "&mut *" + ident(std::get<std::string>(inner.getSymbol().path[0]));
            return "&" + convertRValue(inner);
        }
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
            return "::ispa_std::pycoll::CharLike::char_string(&(" + convertExpression(rvalue.getCharToStringConstructor().what) + "))";
        case LangAPI::RValueType::Cast:
        {
            const auto target = convertType(*rvalue.getCast().type);
            if (target.starts_with("::ispa_std::Shared<"))   // out of a type-erased value
                return "::ispa_std::pycoll::py_cast::<_, " + target + ">(" + convertExpression(rvalue.getCast().what) + ")";
            return "((" + convertExpression(rvalue.getCast().what) + ") as " + target + ")";
        }
        case LangAPI::RValueType::Instance:
            if (receiver.empty()) throw Error("Rust target: instance access outside an instance method");
            if (rvalue.getInstance().handle) {
                if (!shared_receiver)
                    throw Error("Rust target: instance handle requires a shared-object class");
                return receiver + ".__ispa_owner.upgrade().expect(\"instance has no shared owner\")";
            }
            return receiver;
        case LangAPI::RValueType::GetFromBox:
            return "(*" + asExpr(convertExpression(rvalue.getGetFromBox().what)) + ".borrow_mut())";
        default:
            throw Error("Unknown RValue type: {}", (int) rvalue.type());
    }
}
