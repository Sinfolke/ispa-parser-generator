module;
#include <tsl/ordered_map.h>
module LangRepr.ConstructTypes;

import LangAPI;
import AST.API;
import corelib;
import std;
auto collectReferencedNames(const LangAPI::Type &type) -> std::pair<utype::unordered_map<Name, std::size_t>, utype::unordered_set<Name>> {
    utype::unordered_map<Name, std::size_t> out;
    utype::unordered_set<Name> to_forward_declare;
    std::function<void(const LangAPI::Type&)> walk = [&](const LangAPI::Type &t) {
        if (t.isSymbol()) {
            const auto &sym = t.getSymbol();
            Name n;
            n.reserve(sym.path.size());
            for (const auto &part : sym.path) {
                if (!std::holds_alternative<std::string>(part)) return; // skip function-based paths
                n.push_back(std::get<std::string>(part));
            }
            if (!n.empty()) ++out[n];
        }
        for (const auto &param : t.template_parameters) {
            if (std::holds_alternative<LangAPI::Type>(param)) {
                walk(std::get<LangAPI::Type>(param));
            }
        }
    };
    walk(type);
    return std::make_pair(out, to_forward_declare);
}
// Builds a Symbol path for one of our generated grammar types, qualified
// with the Types namespace so it resolves as the real nested class
// (e.g. {"A","B","C"} -> Types::A::B::C) instead of the old flattened
// FlatTypes name ("A_B_C").
auto qualifiedTypesSymbol(const Name &name) -> LangAPI::Symbol {
    stdu::vector<LangAPI::Symbol::PathPart> path;
    path.reserve(name.size() + 1);
    path.emplace_back(std::string{"Types"});
    for (const auto &part : name) path.emplace_back(part);
    return LangAPI::Symbol{std::move(path)};
}

// Human-readable "::"-joined form of a (possibly qualified) Class name,
// for the generated to_string()/printer output.
auto displaySymbolPath(const LangAPI::Symbol &sym) -> std::string {
    std::string out;
    bool first = true;
    for (const auto &part : sym.path) {
        if (!std::holds_alternative<std::string>(part)) continue;
        if (!first) out += "::";
        out += std::get<std::string>(part);
        first = false;
    }
    return out;
}
namespace LangRepr {
    // A member's name plus its declared LangAPI type.
    struct MemberInfo {
        std::string name;
        LangAPI::Type type;
    };

    // Returns true when the type represents a generated AST node.
    auto isNodeType(const LangAPI::Type& type) -> bool {
        if (!type.isValueType())
            return false;

        const auto value_type = type.getValueType();

        if (value_type == LangAPI::ValueType::Token ||
               value_type == LangAPI::ValueType::Rule) {
            return true;
        }
        if (value_type == LangAPI::ValueType::Box) {
            return isNodeType(std::get<LangAPI::Type>(type.template_parameters.front()));
        }
        return false;
    }

    // Returns the wrapped node type if `type` is a wrapper around a node.
    //
    // This helper deliberately only reasons about LangAPI semantics.
    // The backend is responsible for deciding how the wrapper is represented
    // in the target language.
    auto nodeInnerType(const LangAPI::Type& type) -> const LangAPI::Type* {
        if (!type.isValueType())
            return nullptr;

        if (type.getValueType() != LangAPI::ValueType::Box)
            return nullptr;

        if (type.template_parameters.empty())
            return nullptr;

        const auto& parameter = type.template_parameters.front();

        if (!std::holds_alternative<LangAPI::Type>(parameter))
            return nullptr;

        const auto& inner = std::get<LangAPI::Type>(parameter);

        return isNodeType(inner) ? &inner : nullptr;
    };
    // Returns the wrapped node type if `t` is a wrapper around a node.
    auto boxedNodeInnerType(const LangAPI::Type &t) -> const LangAPI::Type* {
        if (!t.isValueType() || t.getValueType() != LangAPI::ValueType::Box || t.template_parameters.empty())
            return nullptr;
        const auto &param = t.template_parameters.front();
        if (!std::holds_alternative<LangAPI::Type>(param))
            return nullptr;
        const auto &inner = std::get<LangAPI::Type>(param);
        return isNodeType(inner) ? &inner : nullptr;
    }
    // Create an expression referencing a member.
    auto memberExpression(const std::string& member) -> LangAPI::Expression {
        return LangAPI::Symbol::createExpression(
            LangAPI::Symbol{member}
        );
    }

    // Convert an arbitrary LangAPI value into its textual representation.
    //
    // This is deliberately a semantic LangAPI operation. The generator does
    // not decide whether the target language uses:
    //
    //     std::to_string(value)
    //     value.to_string()
    //     str(value)
    //     String.valueOf(value)
    //     ...
    //
    // That decision belongs to the target-language backend.
    auto memberToStringExpression(
        const std::string& member,
        const LangAPI::Type&
    ) -> LangAPI::Expression {
        return LangAPI::ToString::createExpression(
            LangAPI::ToString{
                .what = memberExpression(member)
            }
        );
    }

    auto generateToStringFunction(
        const LangAPI::Class& cls,
        const stdu::vector<MemberInfo>& members
    ) -> LangAPI::Function {
        LangAPI::Function function;

        function.name = "to_string";
        function.type = LangAPI::Type{ LangAPI::ValueType::String };
        function.is_const = true;

        auto stringLiteral = [](std::string value) -> LangAPI::Expression {
            return LangAPI::String::createExpression(LangAPI::String{.value = std::move(value)});
        };

        stdu::vector<LangAPI::Expression> parts;

        // ClassName {           <-- was `cls.name + " {"`, cls.name is no longer a string
        parts.push_back(stringLiteral(displaySymbolPath(cls.name) + " {"));

        for (std::size_t i = 0; i < members.size(); ++i) {
            const auto& member = members[i];
            parts.push_back(stringLiteral(member.name + " = "));
            if (isNodeType(member.type)) {
                parts.push_back(
                    LangAPI::StorageSymbol::createExpression(
                        LangAPI::StorageSymbol{
                            LangAPI::Symbol::createExpression(LangAPI::Symbol {member.name}),
                            stdu::vector<LangAPI::StorageSymbol::PathPart>{
                                LangAPI::FunctionCall{
                                    .name = std::make_shared<LangAPI::Symbol>(LangAPI::Symbol{"to_string"}),
                                }
                            }
                        }
                    )
                );
            } else {
                parts.push_back(LangAPI::Symbol::createExpression(LangAPI::Symbol {member.name}));
            }
            if (i + 1 < members.size()) parts.push_back(stringLiteral(", "));
        }
        parts.push_back(stringLiteral("}"));

        function.statements.push_back(
            LangAPI::Return::createStatement(
                LangAPI::Return{
                    .value = LangAPI::IspaLibFunctionCall::createExpression(
                        LangAPI::IspaLibFunctionCall{
                            .symbol = {.exports = LangAPI::StdlibExports::Concat},
                            .args = std::move(parts)
                        }
                    )
                }
            )
        );
        return function;
    }
    // Render into an existing printer so nested nodes retain their parent's depth.
    auto generateWriteToOutputFunction(
        const LangAPI::Class& cls,
        const std::vector<MemberInfo>& members
    ) -> LangAPI::Function {
        LangAPI::Function print_func;

        print_func.name = "write_to_output";
        print_func.type = LangAPI::ValueType::Void;
        print_func.is_const = true;
        print_func.parameters.emplace_back(
            LangAPI::Type{LangAPI::ValueType::BorrowSemantic, LangAPI::Type {LangAPI::IspaLibSymbol{.exports = LangAPI::StdlibExports::ASTPrinter}}},
            "printer"
        );

        auto symbolExpr = [](const std::string& name) {
            return LangAPI::Symbol::createExpression(
                LangAPI::Symbol{name}
            );
        };

        auto boolExpr = [](bool value) {
            return LangAPI::Bool::createExpression(
                LangAPI::Bool{.value = value}
            );
        };

        /*
         * printer.node(name, last)
         */
        auto printerNode = [&](const LangAPI::Symbol& name, bool last) {
            std::string path_str;
            for (const auto &sym_part : name.path) {
                path_str += std::get<std::string>(sym_part);
                path_str += '.';
            }
            if (!path_str.empty()) {
                path_str.pop_back(); // Remove the trailing '.'
            }
            LangAPI::StorageSymbol call{
                symbolExpr("printer"),
                std::vector<LangAPI::StorageSymbol::PathPart>{
                    LangAPI::FunctionCall{
                        .name = std::make_shared<LangAPI::Symbol>(
                            LangAPI::Symbol{"node"}
                        ),
                        .args = {
                            LangAPI::String::createExpression(
                                LangAPI::String{.value = path_str}
                            ),
                            boolExpr(last)
                        }
                    }
                }
            };

            return LangAPI::StorageSymbol::createStatement(
                std::move(call)
            );
        };

        /*
         * printer.node_with_value(name, value, last)
         */
        auto printerNodeValue = [&](
            const std::string& name,
            LangAPI::Expression value,
            bool subtree,
            bool last
        ) {
            LangAPI::StorageSymbol call{
                symbolExpr("printer"),
                std::vector<LangAPI::StorageSymbol::PathPart>{
                    LangAPI::FunctionCall{
                        .name = std::make_shared<LangAPI::Symbol>(
                            LangAPI::Symbol{subtree ? "subtree" : "node_with_value"}
                        ),
                        .args = {
                            LangAPI::String::createExpression(
                                LangAPI::String{.value = name}
                            ),
                            std::move(value),
                            boolExpr(last)
                        }
                    }
                }
            };

            return LangAPI::StorageSymbol::createStatement(
                std::move(call)
            );
        };

        /*
         * printer.up(has_more_siblings)
         */
        auto printerUp = [&](bool has_more_siblings) {
            LangAPI::StorageSymbol call{
                symbolExpr("printer"),
                std::vector<LangAPI::StorageSymbol::PathPart>{
                    LangAPI::FunctionCall{
                        .name = std::make_shared<LangAPI::Symbol>(
                            LangAPI::Symbol{"up"}
                        ),
                        .args = {
                            boolExpr(has_more_siblings)
                        }
                    }
                }
            };

            return LangAPI::StorageSymbol::createStatement(
                std::move(call)
            );
        };

        /*
         * printer.down()
         */
        auto printerDown = [&]() {
            LangAPI::StorageSymbol call{
                symbolExpr("printer"),
                std::vector<LangAPI::StorageSymbol::PathPart>{
                    LangAPI::FunctionCall{
                        .name = std::make_shared<LangAPI::Symbol>(
                            LangAPI::Symbol{"down"}
                        )
                    }
                }
            };

            return LangAPI::StorageSymbol::createStatement(
                std::move(call)
            );
        };

        // Root node
        print_func.statements.push_back(
            printerNode(cls.name, members.empty())
        );

        if (!members.empty()) {
            print_func.statements.push_back(
                printerUp(false)
            );

            for (std::size_t i = 0; i < members.size(); ++i) {
                const auto& member = members[i];
                const bool is_last = i == members.size() - 1;

                const bool direct_node = isNodeType(member.type);
                const bool boxed_node =
                    !direct_node &&
                    boxedNodeInnerType(member.type) != nullptr;

                if (direct_node || boxed_node) {
                    //
                    // Node:
                    //   printer.node("member", last);
                    //   printer.up(...);
                    //   <object>.data.<member>.print(printer);
                    //   printer.down();
                    //

                    print_func.statements.push_back(
                        printerNode(member.name, is_last)
                    );

                    print_func.statements.push_back(
                        printerUp(!is_last)
                    );

                    LangAPI::StorageSymbol member_print_call{
                        LangAPI::Symbol::createExpression(LangAPI::Symbol {member.name}),
                        std::vector<LangAPI::StorageSymbol::PathPart>{
                            LangAPI::FunctionCall{
                                .name = std::make_shared<LangAPI::Symbol>(
                                    LangAPI::Symbol{"print"}
                                ),
                                .args = { symbolExpr("printer") } // Pass printer instance
                            }
                        }
                    };

                    print_func.statements.push_back(
                        LangAPI::StorageSymbol::createStatement(
                            std::move(member_print_call)
                        )
                    );

                    print_func.statements.push_back(
                        printerDown()
                    );
                } else {
                    auto isStructuredType =
                    [&](const auto& self, const LangAPI::Type& type) -> bool {
                        if (!type.isValueType())
                            return false;

                        switch (type.getValueType()) {
                            case LangAPI::ValueType::Token:
                            case LangAPI::ValueType::Rule:
                                return true;

                            case LangAPI::ValueType::Array:
                                if (!type.template_parameters.empty()) {
                                    if (const auto* inner = std::get_if<LangAPI::Type>(
                                            &type.template_parameters.front())) {
                                        return self(self, *inner);
                                            }
                                }
                                return false;

                            default:
                                return false;
                        }
                    };
                    const bool is_subtree =
                        isStructuredType(isStructuredType, member.type);

                    print_func.statements.push_back(
                        printerNodeValue(
                            member.name,
                            symbolExpr(member.name),
                            is_subtree,
                            is_last
                        )
                    );
                }
            }

            print_func.statements.push_back(
                printerDown()
            );
        }

        return print_func;
    }

    // Public entry point: only a top-level print owns a new printer.
    auto generatePrintFunction() -> LangAPI::Function {
        LangAPI::Function function;
        function.name = "print";
        function.type = LangAPI::ValueType::Void;
        function.is_const = true;
        function.parameters.emplace_back(LangAPI::Type{LangAPI::ValueType::BorrowSemantic, LangAPI::Type{LangAPI::IspaLibSymbol{.exports = LangAPI::StdlibExports::TextOutput}}}, "os");
        function.statements.push_back(LangAPI::Variable{
            .name = "printer",
            .type = LangAPI::Type{LangAPI::IspaLibSymbol{.exports = LangAPI::StdlibExports::ASTPrinter}},
            .value = LangAPI::Inheritance::createExpression(LangAPI::Inheritance{
                .name = LangAPI::IspaLibSymbol{.exports = LangAPI::StdlibExports::ASTPrinter},
                .args = {LangAPI::Symbol::createExpression(LangAPI::Symbol{"os"})}
            })
        });
        function.statements.push_back(LangAPI::Expression::createStatement(
            LangAPI::FunctionCall::createExpression(LangAPI::FunctionCall{
                .name = std::make_shared<LangAPI::Symbol>(LangAPI::Symbol{"write_to_output"}),
                .args = {LangAPI::Symbol::createExpression(LangAPI::Symbol{"printer"})}
            })
        ));
        return function;
    }

    auto generateStreamOutputFunction(const LangAPI::Class &cls) -> LangAPI::Function {
        LangAPI::Function function;
        function.name = "operator<<";
        // cls.name is already a Symbol now — no need to wrap it again.
        // Parameter can no longer reuse cls.name as its identifier (it may be
        // qualified, e.g. Types::A::B — not a legal bare identifier), so it's
        // just "value".
        function.parameters.emplace_back(LangAPI::Type{cls.name}, "value");
        function.statements.push_back(LangAPI::StorageSymbol::createStatement(
            LangAPI::StorageSymbol{
                LangAPI::Symbol::createExpression(LangAPI::Symbol{"value"}),
                stdu::vector<LangAPI::StorageSymbol::PathPart>{LangAPI::FunctionCall{
                    .name = std::make_shared<LangAPI::Symbol>(LangAPI::Symbol{"print"}),
                    .args = {LangAPI::Symbol::createExpression(LangAPI::Symbol{"os"})}
                }}
            }
        ));
        function.statements.push_back(LangAPI::Return::createStatement(LangAPI::Return{
            .value = LangAPI::Symbol::createExpression(LangAPI::Symbol{"os"})
        }));
        return function;
    }



    auto ConstructTypes::constructTokensAndRulesEnum() -> void {
        using IdMap = utype::unordered_map<Name, std::size_t>;
        auto tokenIdentity = [&](const Name& name) {
            std::set<Name> visiting;
            std::ostringstream pattern;
            auto member = [&](auto&& self, const AST::RuleMember& value) -> void {
                pattern << '(';
                if (value.isName() && tree.contains(value.getName().name) && visiting.insert(value.getName().name).second) {
                    for (const auto& child : tree[value.getName().name].rule_members) self(self, *child);
                    visiting.erase(value.getName().name);
                } else if (value.isGroup()) {
                    for (const auto& child : value.getGroup().values) self(self, *child);
                } else if (value.isOp()) {
                    for (const auto& child : value.getOp().options) { pattern << '|'; self(self, *child); }
                } else std::visit([&](const auto& atom) {
                    if constexpr (!std::is_same_v<std::decay_t<decltype(atom)>, std::monostate>) pattern << atom;
                }, value.value);
                pattern << ')' << value.quantifier;
            };
            if (tree.contains(name)) for (const auto& value : tree[name].rule_members) member(member, *value);
            std::uint64_t hash = 14695981039346656037ull;
            // The exported terminal name is its semantic identity; parser and
            // module names and payload layouts do not participate in this ID.
            for (unsigned char c : name.back() + '\0' + pattern.str()) { hash ^= c; hash *= 1099511628211ull; }
            return static_cast<std::size_t>(hash & 0x7fffffffffffffffull);
        };

        auto generate = [&](const ParserIR::DataBlockList& blocks,
                            const std::string& numeric_name,
                            const std::string& string_name) {
            Node root;
            IdMap ids;

            // Preserve the original data-block order for numeric IDs.
            for (const auto& [name, data] : blocks) {
                if (name.empty())
                    continue;

                root.find_or_emplace(name)->data = data;

                if (!ids.contains(name))
                    ids.emplace(name, numeric_name == "Tokens" ? tokenIdentity(name) : ids.size());
            }

            // Ensure that all intermediate nodes have IDs too.
            // Your grammar normally guarantees these paths already exist.
            root.visit([&](const Name& path, const Node&) {
                if (!ids.contains(path))
                    ids.emplace(path, ids.size());
                return false;
            });

            // Adapt this lambda to the initializer API of LangAPI::Variable.
            auto makeVariable = [&](const Name& path, bool as_string)
                -> LangAPI::Declaration
            {
                const auto type = LangAPI::Type {
                    LangAPI::ValueType::ImmutableSemantic,
                    LangAPI::Type{
                        as_string ? LangAPI::ValueType::NonOwnedString : LangAPI::ValueType::LargeUInt
                    }
                };

                LangAPI::Variable var{
                    .name = "id",
                    .type = type,
                    .value = as_string ? LangAPI::String::createExpression(LangAPI::String {.value = corelib::text::join(path, ".")}) : LangAPI::Int::createExpression(LangAPI::Int {.value = (long long) ids.at(path)}),
                };

                return LangAPI::Variable::createDeclaration(
                    std::move(var)
                );
            };
            // Recursively convert the Node hierarchy into LangAPI namespaces.
            std::function<LangAPI::Namespace(
                const Node&, const std::string&, Name&, bool
            )> build;

            build = [&](const Node& node,
                        const std::string& name,
                        Name& path,
                        bool as_string) -> LangAPI::Namespace
            {
                LangAPI::Namespace result;
                result.name = name;

                if (!path.empty()) {
                    result.declarations.push_back(
                        makeVariable(path, as_string)
                    );
                    if (numeric_name == "Tokens" && !as_string) result.declarations.push_back(
                        LangAPI::Variable::createDeclaration(LangAPI::Variable{
                            .name = "shared", .type = LangAPI::Type{LangAPI::ValueType::ImmutableSemantic, LangAPI::Type{LangAPI::ValueType::Bool}},
                            .value = LangAPI::Bool::createExpression(LangAPI::Bool{.value = true})}));
                }

                for (const auto& [child_name, child] : node.children) {
                    path.push_back(child_name);

                    result.declarations.push_back(
                        build(*child, child_name, path, as_string)
                    );

                    path.pop_back();
                }

                return result;
            };

            Name path;

            holder.push(build(root, numeric_name, path, false));
            holder.push(build(root, string_name, path, true));
        };
        auto makeToStrFunc = [&](const ParserIR::DataBlockList &dtb, std::string base_symbol) {
            LangAPI::Function func {
                .type = LangAPI::Type{LangAPI::ValueType::NonOwnedString},
                .name = base_symbol + "ToString",
                .parameters = { std::make_pair<LangAPI::Type>(LangAPI::Type{LangAPI::ValueType::LargeUInt}, "id") }
            };
            func.is_static = true;
            LangAPI::Switch switch_on_id {
                .expression = LangAPI::Symbol::createExpression(LangAPI::Symbol{"id"})
            };
            for (const auto &[name, _] : dtb) {
                switch_on_id.cases.emplace_back(
                    LangAPI::Symbol::createRValue(LangAPI::Symbol {base_symbol, name, "id"}),
                    LangAPI::Return::createExpression(LangAPI::Return {.value = LangAPI::Symbol::createExpression(LangAPI::Symbol {base_symbol + "Str", name, "id"})})
                );
            }
            func.statements = {
                switch_on_id,
                LangAPI::Throw::createStatement(LangAPI::Throw {.throw_value = LangAPI::String::createExpression(LangAPI::String {.value = "Invalid " + base_symbol + " ID"})})
            };
            holder.push(func);
        };
        auto tokens = lexer_builder.getDataBlocks();
        for (const auto& [name, rule] : tree) if (rule.external_token) tokens.emplace(name, ParserIR::DataBlock{});
        generate(
            tokens,
            "Tokens",
            "TokensStr"
        );
        generate(
            ir.getDataBlocks(),
            "Rules",
            "RulesStr"
        );
        makeToStrFunc(tokens, "Tokens");
        makeToStrFunc(ir.getDataBlocks(), "Rules");
    }


    auto ConstructTypes::constructTokensAndRulesEnumToString() -> void {
        auto constructSwitchFromDataBlockList = [](const ParserIR::DataBlockList &list, const char* name) {
            LangAPI::Switch ss;
            ss.expression = LangAPI::Symbol::createExpression(LangAPI::Symbol {name});
            for (const auto &[name, block] : list) {
                ss.cases.emplace_back();
                ss.cases.back().first = LangAPI::Symbol::createRValue(LangAPI::Symbol {corelib::text::join(name, "_")});
                ss.cases.back().second = LangAPI::Return::createStatements(LangAPI::Return {.value = LangAPI::String::createExpression(corelib::text::join(name, "_"))});
            }
            return ss;
        };
        LangAPI::Function tokens_fun {
            .name = "tokensToString",
            .parameters = { std::make_pair<LangAPI::Type>(LangAPI::Symbol {"Tokens"}, "token") },
            .statements = LangAPI::Switch::createStatements(constructSwitchFromDataBlockList(lexer_builder.getDataBlocks(), "tokens"))
        };
        LangAPI::Function rules_fun {
            .name = "rulesToString",
            .parameters = { std::make_pair<LangAPI::Type>(LangAPI::Symbol {"Rules"}, "rules") },
            .statements = LangAPI::Switch::createStatements(constructSwitchFromDataBlockList(ir.getDataBlocks(), "rules"))
        };
        holder.push(tokens_fun);
        holder.push(rules_fun);
    }
    auto ConstructTypes::constructTypesNamespace() -> void {
        using NameSet = utype::unordered_set<Name>;
        using DependencyMap = utype::unordered_map<Name, NameSet>;

        Node root;
        stdu::vector<Name> names;
        NameSet known;

        auto addName = [&](const Name& n) {
            if (known.insert(n).second) {
                names.push_back(n);
                root.find_or_emplace(n);
            }
        };

        auto addData = [&](const ParserIR::DataBlockList& blocks) {
            for (const auto& [name, data] : blocks) {
                addName(name);
                root.find_or_emplace(name)->data = data;
                for (std::size_t len = 1; len < name.size(); ++len)
                    addName(Name(name.begin(), name.begin() + len));
            }
        };
        addData(lexer_builder.getDataBlocks());
        addData(ir.getDataBlocks());

        // Your original generator uses path segments as identifiers. If it
        // sanitizes or escapes identifiers elsewhere, reuse that function here.
        auto flattenName = [](const Name& n) -> std::string {
            std::string result;
            for (const auto& part : n) {
                if (!result.empty()) result += "_";
                result += part;
            }
            return result;
        };

        auto flatSymbol = [&](const Name& n) -> LangAPI::Symbol {
            return LangAPI::Symbol{"", namespace_name, "FlatTypes", flattenName(n)};
        };

        auto templateNames = [&](const Name& n) -> stdu::vector<std::string> {
            if (!tree.contains(n)) return {};
            for (const auto& feature : tree[n].features)
                if (const auto* syntax = std::get_if<AST::DeclarationSyntax>(&feature))
                    return syntax->templateParameters;
            return {};
        };
        auto flatType = [&](const Name& n) -> LangAPI::Type {
            LangAPI::Type type{flatSymbol(n)};
            for (const auto& parameter : templateNames(n))
                type.template_parameters.emplace_back(LangAPI::Type{LangAPI::Symbol{parameter}});
            return type;
        };
        auto instanceType = [&](const AST::RuleMemberName& origin) -> LangAPI::Type {
            LangAPI::Type type{flatSymbol(origin.name)};
            for (const auto& argument : origin.templateArguments)
                type.template_parameters.emplace_back(LangAPI::Type{flatSymbol(argument)});
            return type;
        };

        // Requires your LangAPI's real alias declaration API; see note below.
        auto emitTypeAlias = [&](LangAPI::Class& owner,
                                 const std::string& alias,
                                 const Name& target) {
            owner.data.push_back({
                std::make_shared<LangAPI::Declaration>(
                    LangAPI::TypeAlias::createDeclaration(
                        LangAPI::TypeAlias{
                            .name = alias,
                            .type = flatType(target),
                            .template_parameters = templateNames(target)
                        })),
                LangAPI::Visibility::Public
            });
        };

        // Every name has a canonical flat type, including pure grouping nodes.
        // Nested hierarchy is *aliases*, never inheritance or new nested structs.
        utype::unordered_map<Name, stdu::vector<Name>> children;
        for (const Name& n : names) {
            if (n.size() == 1) continue;
            const Name parent(n.begin(), n.end() - 1);
            children[parent].push_back(n);
        }

        // Dependency graph contains ONLY field-reference dependencies.
        // Do NOT add parent -> child structural edges. Aliases do not require
        // the target to be complete, and all flat targets are forward-declared.
        DependencyMap deps;
        NameSet explicitly_boxed;
        for (const Name& n : names) {
            deps[n];
            if (tree.contains(n) && tree[n].templateOrigin) {
                deps[n].insert(tree[n].templateOrigin->name);
                for (const auto& argument : tree[n].templateOrigin->templateArguments) deps[n].insert(argument);
            }
            const Node* node = root.find(n);
            if (!node) continue;
            auto absorb = [&](const LangAPI::Type& field_type) {
                auto [refs, forward] = collectReferencedNames(field_type);
                for (const auto& [ref, count] : refs) {
                    (void)count;
                    if (known.contains(ref)) deps[n].insert(ref);
                }
                for (const auto& ref : forward)
                    if (known.contains(ref)) explicitly_boxed.insert(ref);
            };
            if (node->data.is_regular_data_block()) {
                absorb(node->data.getRegularDataBlock().second);
            } else if (node->data.is_inclosed_map()) {
                for (const auto& [field_name, field] : node->data.getInclosedMap()) {
                    (void)field_name;
                    absorb(field.second);
                }
            }
        }

        // Tarjan SCC: identify genuine mutual/self recursion (not reference count).
        utype::unordered_map<Name, int> index, lowlink, component;
        NameSet onStack;
        stdu::vector<Name> stack;
        stdu::vector<stdu::vector<Name>> components;
        int nextIndex = 0;

        std::function<void(const Name&)> visit = [&](const Name& n) {
            index[n] = lowlink[n] = nextIndex++;
            stack.push_back(n);
            onStack.insert(n);
            for (const Name& dep : deps[n]) {
                if (!index.contains(dep)) {
                    visit(dep);
                    lowlink[n] = std::min(lowlink[n], lowlink[dep]);
                } else if (onStack.contains(dep)) {
                    lowlink[n] = std::min(lowlink[n], index[dep]);
                }
            }
            if (lowlink[n] == index[n]) {
                const int id = static_cast<int>(components.size());
                components.emplace_back();
                while (true) {
                    Name top = stack.back();
                    stack.pop_back();
                    onStack.erase(top);
                    component[top] = id;
                    components.back().push_back(top);
                    if (top == n) break;
                }
            }
        };
        for (const Name& n : names)
            if (!index.contains(n)) visit(n);

        // Box references that stay inside a cyclic component. Keep the decision
        // edge-specific, so unrelated fields do not get boxed merely because
        // their target appears somewhere in a cycle.
        auto isCyclicEdge = [&](const Name& source, const Name& target) -> bool {
            if (component[source] != component[target]) return false;
            const auto& scc = components[component[source]];
            return scc.size() > 1 || source == target;
        };

        // Topological sort after cycle-breaking. Edges requiring boxing no longer
        // demand a complete referenced type at definition time.
        DependencyMap strongDeps;
        utype::unordered_map<Name, stdu::vector<Name>> reverseDeps;
        utype::unordered_map<Name, std::size_t> indegree;
        for (const Name& n : names) {
            for (const Name& dep : deps[n]) {
                if (isCyclicEdge(n, dep) || explicitly_boxed.contains(dep))
                    continue;
                strongDeps[n].insert(dep);
                reverseDeps[dep].push_back(n);
            }
            indegree[n] = strongDeps[n].size();
        }
        std::deque<Name> queue;
        for (const Name& n : names)
            if (indegree[n] == 0) queue.push_back(n);

        stdu::vector<Name> sorted;
        sorted.reserve(names.size());
        while (!queue.empty()) {
            Name n = queue.front();
            queue.pop_front();
            sorted.push_back(n);
            for (const Name& user : reverseDeps[n])
                if (--indegree[user] == 0) queue.push_back(user);
        }
        if (sorted.size() != names.size())
            throw std::runtime_error("ISPA: internal error: unbroken type dependency cycle");

        auto getReferencedName = [&](const LangAPI::Type& t) -> Name {
            Name result;
            if (!t.isSymbol()) return result;
            for (const auto& part : t.getSymbol().path) {
                if (!std::holds_alternative<std::string>(part)) return {};
                result.push_back(std::get<std::string>(part));
            }
            return result;
        };

        // Adapt only grammar-symbol leaves, recursively through all wrappers.
        // A direct symbol in a recursive edge becomes Box<FlatType>.
        // If your AST uses Node<T> for recursive storage, ensure that wrapping
        // the inner T with Box has the semantics expected by ISPA_STD::Node.
        std::function<void(LangAPI::Type&, const Name&)> rewriteType;
        rewriteType = [&](LangAPI::Type& t, const Name& owner) {
            if (t.isSymbol()) {
                Name target = getReferencedName(t);
                if (!known.contains(target)) return;
                const bool box = isCyclicEdge(owner, target) ||
                                 explicitly_boxed.contains(target);
                auto canonical = LangAPI::Type{flatSymbol(target)};
                canonical.template_parameters = std::move(t.template_parameters);
                if (canonical.template_parameters.empty() && target == owner)
                    canonical = flatType(target);
                for (auto& parameter : canonical.template_parameters)
                    if (auto* inner = std::get_if<LangAPI::Type>(&parameter)) rewriteType(*inner, owner);
                if (box) {
                    t.type = LangAPI::ValueType::Box;
                    t.template_parameters = {std::move(canonical)};
                } else {
                    t = std::move(canonical);
                }
                return;
            }
            for (auto& param : t.template_parameters) {
                if (auto* inner = std::get_if<LangAPI::Type>(&param))
                    rewriteType(*inner, owner);
            }
        };

        // Emit all canonical forward declarations before any fields/aliases.
        LangAPI::Declarations flatForwards;
        for (const Name& n : names) {
            if (tree.contains(n) && tree[n].templateOrigin) continue;
            if (tree.contains(n) && tree[n].external) {
                flatForwards.push_back(LangAPI::TypeAlias::createDeclaration(LangAPI::TypeAlias{
                    .name = flattenName(n), .type = LangAPI::Type{LangAPI::Symbol{"::" + tree[n].external->cpp_type}}}));
                continue;
            }
            flatForwards.push_back(LangAPI::ForwardDeclaredClass::createDeclaration(
                LangAPI::ForwardDeclaredClass{.name = flattenName(n), .isStruct = true,
                    .template_parameters = templateNames(n)}));
        }
        for (const Name& n : names) if (tree.contains(n) && tree[n].templateOrigin)
            flatForwards.push_back(LangAPI::TypeAlias::createDeclaration(LangAPI::TypeAlias{
                .name = flattenName(n), .type = instanceType(*tree[n].templateOrigin)}));
        holder.push(LangAPI::Namespace{
            .name = "FlatTypes", .declarations = std::move(flatForwards)
        });

        // LangAPI class names in this namespace are SIMPLE flat identifiers.
        // The types' generated printer methods stay on the canonical classes.
        for (const Name& fullName : sorted) {
            if (tree.contains(fullName) && (tree[fullName].external || tree[fullName].templateOrigin)) continue;
            const Node* old = root.find(fullName);
            if (!old) throw std::runtime_error("ISPA: missing type tree node");

            LangAPI::Class c;
            c.name = LangAPI::Symbol{flattenName(fullName)};
            c.template_parameters = templateNames(fullName);

            // Keep the class's direct referenced-type dependencies in LangAPI,
            // not only in the temporary graph used to order definitions.
            // Include boxed/recursive references too: they are still semantic
            // dependencies even if they do not require a complete type here.
            // Iterate in the original name order for deterministic output.
            for (const Name& dependency : names) {
                if (deps[fullName].contains(dependency))
                    c.dependecies.push_back(flatSymbol(dependency));
            }

            stdu::vector<MemberInfo> members;

            if (old->data.is_regular_data_block()) {
                auto type = old->data.getRegularDataBlock().second;
                rewriteType(type, fullName);
                members.push_back(MemberInfo{"value", type});
                c.data.push_back({
                    std::make_shared<LangAPI::Declaration>(
                        LangAPI::Variable::createDeclaration(
                            LangAPI::Variable{.name = "value", .type = type})),
                    LangAPI::Visibility::Public
                });
            } else if (old->data.is_inclosed_map()) {
                for (const auto& [name, type] : old->data.getInclosedMap()) {
                    auto fieldType = type.second;
                    rewriteType(fieldType, fullName);
                    members.push_back(MemberInfo{name, fieldType});
                    c.data.push_back({
                        std::make_shared<LangAPI::Declaration>(
                            LangAPI::Variable::createDeclaration(
                                LangAPI::Variable{.name = name, .type = fieldType})),
                        LangAPI::Visibility::Public
                    });
                }
            }

            // Flat child types are already declared. Aliasing them introduces no
            // new type identity, and no parent-before-child definition constraint.
            if (auto it = children.find(fullName); it != children.end()) {
                for (const Name& child : it->second)
                    emitTypeAlias(c, child.back(), child);
            }

            c.data.push_back({
                std::make_shared<LangAPI::Declaration>(LangAPI::Function::createDeclaration(
                    generateWriteToOutputFunction(c, members))),
                LangAPI::Visibility::Public
            });
            c.data.push_back({
                std::make_shared<LangAPI::Declaration>(LangAPI::Function::createDeclaration(
                    generatePrintFunction())),
                LangAPI::Visibility::Public
            });
            c.to_str_fun = generateToStringFunction(c, members);
            c.output_fun = generateStreamOutputFunction(c);
            LangAPI::Declarations definition;
            definition.push_back(LangAPI::Class::createDeclaration(std::move(c)));
            holder.push(LangAPI::Namespace{
                .name = "FlatTypes", .declarations = std::move(definition)
            });
        }

        // Public hierarchical interface. This exposes the *same* canonical types:
        // Types::rule          == FlatTypes::rule
        // Types::rule::value   == FlatTypes::rule_value
        // Types::rule::value::STRING == FlatTypes::rule_value_STRING
        // This requires a namespace-scope TypeAlias API (not nested Class.data).
        LangAPI::Declarations publicAliases;
        for (const Name& n : names) {
            if (n.size() != 1) continue;
            publicAliases.push_back(LangAPI::TypeAlias::createDeclaration(
                LangAPI::TypeAlias{
                    .name = n[0],
                    .type = flatType(n),
                    .template_parameters = templateNames(n)
                }));
        }
        holder.push(LangAPI::Namespace{
            .name = "Types", .declarations = std::move(publicAliases)
        });
    }

}
