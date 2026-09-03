module ConstructBase;
import cpuf.printf;
import LLIR.Builder.Base;
namespace LangRepr {
    auto ConstructBase::createTypeToken() -> LangAPI::TypeAlias {
        LangAPI::Type variant_type = LangAPI::ValueType::Variant;
        for (const auto &[name, dtb] : lexer_builder.getDataBlocks()) {
            LangAPI::Symbol tn = name;
            tn.path.insert(tn.path.begin(), "Types");
            variant_type.template_parameters.push_back(LangAPI::Type {tn});
        }
        Token = LangAPI::Symbol { "Token" };
        return LangAPI::TypeAlias {.name = "Token", .type = variant_type};
    }
    auto ConstructBase::createLexerClass() -> LangAPI::Class {
        LangAPI::IspaLibSymbol lexer_s {.exports = LangAPI::StdlibExports::Lexer};
        lexer_s.template_parameters.push_back(std::make_shared<LangAPI::Type>(LangAPI::Symbol {"Token"}));
        return LangAPI::Class {
            .name = "Lexer",
            .inherit_members = {std::make_pair(LangAPI::Visibility::Public, lexer_s)},
            .default_visibility = LangAPI::Visibility::Private
        };
    }

    auto ConstructBase::makeIntRValue(int v) -> std::shared_ptr<LangAPI::RValue> {
        return std::make_shared<LangAPI::RValue>(
            LangAPI::Int::createRValue(LangAPI::Int{.value = v})
        );
    };
    auto ConstructBase::ensureTypesNs(LangAPI::Type t) -> LangAPI::Type {
        if (t.isSymbol()) {
            const auto &sym_path = t.getSymbol().path;
            stdu::vector<std::string> path;
            for (const auto &part : sym_path) {
                if (std::holds_alternative<std::string>(part)) {
                    path.push_back(std::get<std::string>(part));
                } else return t;
            }
            if (tree.contains(path) && std::get<std::string>(t.getSymbol().path.front()) != "Types") {
                t.getSymbol().path.insert(t.getSymbol().path.begin(), "Types");
            }
        }
        if (!t.template_parameters.empty()) {
            for (auto &p : t.template_parameters) p = ensureTypesNs(std::get<LangAPI::Type>(p));
        }
        return t;
    };
    // 1. Symbol Traversal
    auto ConstructBase::ensureTypesNs(LangAPI::Symbol s) -> LangAPI::Symbol {
        bool was_fun_call = false;
        stdu::vector<std::string> path;
        for (auto &part : s.path) {
            if (std::holds_alternative<LangAPI::FunctionCall>(part)) {
                part = ensureTypesNs(std::get<LangAPI::FunctionCall>(part));
                was_fun_call = true;
            } else {
                path.push_back(std::get<std::string>(part));
            }
        }
        if (tree.contains(path) && !was_fun_call && std::get<std::string>(s.path.front()) != "Types") {
            s.path.insert(s.path.begin(), "Types");
        }
        return s;
    }
    auto ConstructBase::ensureTypesNs(LangAPI::MakeTuple t) -> LangAPI::MakeTuple {
        for (auto &p : t.args) {
            p = ensureTypesNs(p);
        }
        return t;
    }
    // 2. StorageSymbol Traversal
    auto ConstructBase::ensureTypesNs(LangAPI::StorageSymbol s) -> LangAPI::StorageSymbol {
        s.what = ensureTypesNs(s.what);
        for (auto &part : s.path) {
            if (std::holds_alternative<LangAPI::FunctionCall>(part)) {
                part = ensureTypesNs(std::get<LangAPI::FunctionCall>(part));
            }
        }
        return s;
    }

    // 3. Inheritance Traversal
    auto ConstructBase::ensureTypesNs(LangAPI::Inheritance s) -> LangAPI::Inheritance {
        if (std::holds_alternative<LangAPI::Symbol>(s.name)) {
            s.name = ensureTypesNs(std::get<LangAPI::Symbol>(s.name));
        }
        for (auto &arg : s.args) {
            arg = ensureTypesNs(arg);
        }
        return s;
    }

    // 4. RValue Traversal
    auto ConstructBase::ensureTypesNs(LangAPI::RValue r) -> LangAPI::RValue {
        if (r.isStorageSymbol()) {
            r.set(ensureTypesNs(r.getStorageSymbol()));
        } else if (r.isInheritance()) {
            r.set(ensureTypesNs(r.getInheritance()));
        } else if (r.isArray()) {
            auto &arr = r.getArray();
            for (auto &val : arr.values) val = ensureTypesNs(val);
            for (auto &param : arr.template_parameters) {
                std::visit([&](auto &p) {
                    using T = std::decay_t<decltype(p)>;
                    if constexpr (std::is_same_v<T, std::shared_ptr<LangAPI::Type>>) {
                        if (p) *p = ensureTypesNs(*p);
                    } else if constexpr (std::is_same_v<T, LangAPI::RValue>) {
                        p = ensureTypesNs(p);
                    }
                }, param);
            }
        } else if (r.isMap()) {
            auto &m = r.getMap();
            for (auto &val : m.values) val = ensureTypesNs(val);
        } else if (r.isSpan()) {
            auto &span = r.getSpan();
            if (span.type) *span.type = ensureTypesNs(*span.type);
            span.sym = ensureTypesNs(span.sym);
        } else if (r.isMakeTuple()) {
            auto &mt = r.getMakeTuple();
            mt = ensureTypesNs(mt);
        }
        return r;
    }

    // 5. FunctionCall Traversal
    auto ConstructBase::ensureTypesNs(LangAPI::FunctionCall s) -> LangAPI::FunctionCall {
        if (std::holds_alternative<std::shared_ptr<LangAPI::Symbol>>(s.name)) {
            s.name = std::make_shared<LangAPI::Symbol>(ensureTypesNs(*std::get<std::shared_ptr<LangAPI::Symbol>>(s.name)));
        }
        for (auto &p : s.template_parameters) {
            std::visit([&](auto &param) {
                using T = std::decay_t<decltype(param)>;
                if constexpr (std::is_same_v<T, LangAPI::Type>) {
                    param = ensureTypesNs(param);
                } else if constexpr (std::is_same_v<T, LangAPI::RValue>) {
                    param = ensureTypesNs(param);
                }
            }, p);
        }
        for (auto &arg : s.args) {
            arg = ensureTypesNs(arg);
        }
        return s;
    }

    // 6. Lambda Traversal
    auto ConstructBase::ensureTypesNs(LangAPI::Lambda l) -> LangAPI::Lambda {
        for (auto &[type, param_name] : l.parameters) {
            type = ensureTypesNs(type);
        }
        l.statements = ensureTypesNs(l.statements);
        return l;
    }

    // 7. ExpressionValue Traversal
    auto ConstructBase::ensureTypesNs(LangAPI::ExpressionValue ev) -> LangAPI::ExpressionValue {
        std::visit([&](auto &val) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, LangAPI::RValue>) {
                val = ensureTypesNs(val);
            } else if constexpr (std::is_same_v<T, LangAPI::FunctionCall>) {
                val = ensureTypesNs(val);
            } else if constexpr (std::is_same_v<T, LangAPI::Return>) {
                val.value = ensureTypesNs(val.value);
            } else if constexpr (std::is_same_v<T, LangAPI::VariableAssignment>) {
                val.value = ensureTypesNs(val.value);
            } else if constexpr (std::is_same_v<T, LangAPI::Lambda>) {
                val = ensureTypesNs(val);
            } else if constexpr (std::is_same_v<T, LangAPI::DfaLookup>) {
                val.return_type = ensureTypesNs(val.return_type);
            }
        }, ev.value);
        return ev;
    }

    // 8. Expression Traversal
    auto ConstructBase::ensureTypesNs(LangAPI::Expression expr) -> LangAPI::Expression {
        for (auto &val : expr) {
            val = ensureTypesNs(val);
        }
        return expr;
    }

    // 9. Variable Traversal
    auto ConstructBase::ensureTypesNs(LangAPI::Variable v) -> LangAPI::Variable {
        v.type = ensureTypesNs(v.type);
        v.value = ensureTypesNs(v.value);
        return v;
    }

    // 10. Control Flow Traversals (If, While, DoWhile, Switch)
    auto ConstructBase::ensureTypesNs(LangAPI::If s) -> LangAPI::If {
        s.expr = ensureTypesNs(s.expr);
        s.stmt = ensureTypesNs(s.stmt);
        s.else_stmt = ensureTypesNs(s.else_stmt);
        return s;
    }

    auto ConstructBase::ensureTypesNs(LangAPI::While s) -> LangAPI::While {
        s.expr = ensureTypesNs(s.expr);
        s.stmt = ensureTypesNs(s.stmt);
        return s;
    }

    auto ConstructBase::ensureTypesNs(LangAPI::DoWhile s) -> LangAPI::DoWhile {
        s.expr = ensureTypesNs(s.expr);
        s.stmt = ensureTypesNs(s.stmt);
        return s;
    }

    auto ConstructBase::ensureTypesNs(LangAPI::Switch s) -> LangAPI::Switch {
        s.expression = ensureTypesNs(s.expression);
        for (auto &[case_val, case_stmts] : s.cases) {
            case_val = ensureTypesNs(case_val);
            case_stmts = ensureTypesNs(case_stmts);
        }
        return s;
    }
    auto ConstructBase::ensureTypesNs(LangAPI::Throw s) -> LangAPI::Throw {
        s.throw_value = ensureTypesNs(s.throw_value);
        return s;
    }

    // 11. Statement Traversal
    auto ConstructBase::ensureTypesNs(const LangAPI::Statement &s) -> LangAPI::Statement {
        LangAPI::Statement new_statement;
        std::visit([&](const auto &unpacked) {
            using T = std::decay_t<decltype(unpacked)>;
            if constexpr (!std::is_same_v<T, std::monostate>) {
                new_statement.value = ensureTypesNs(unpacked);
            }
        }, s.value);
        return new_statement;
    }

    // 12. Statements Vector Traversal
    auto ConstructBase::ensureTypesNs(LangAPI::Statements stmts) -> LangAPI::Statements {
        for (auto &stmt : stmts) {
            stmt = ensureTypesNs(stmt);
        }
        return stmts;
    }
    auto ConstructBase::extractRawSymbol(const LangAPI::Type &t) -> stdu::vector<LangAPI::Type> {
        if (t.isSymbol())
            return {t};
        if (!t.template_parameters.empty()) {
            std::vector<LangAPI::Type> types;
            for (auto &p : t.template_parameters) {
                auto r = extractRawSymbol(std::get<LangAPI::Type>(p));
                types.insert(types.end(), r.begin(), r.end());
            }
            return types;
        }
        return {t};
    };
}