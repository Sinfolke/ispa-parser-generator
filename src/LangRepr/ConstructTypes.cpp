module LangRepr.ConstructTypes;

import LangAPI;
import corelib;
import std;
auto collectReferencedNames(const LangAPI::Type &type) -> std::pair<utype::unordered_map<Name, std::size_t>, utype::unordered_set<Name>> {
    utype::unordered_map<Name, std::size_t> out;
    utype::unordered_set<Name> to_forward_declare;
    std::function<void(const LangAPI::Type&, bool)> walk = [&](const LangAPI::Type &t, bool inside_array = false) {
        if (t.isSymbol()) {
            const auto &sym = t.getSymbol();
            Name n;
            n.reserve(sym.path.size());
            for (const auto &part : sym.path) {
                if (!std::holds_alternative<std::string>(part)) return; // skip function-based paths
                n.push_back(std::get<std::string>(part));
            }
            if (!n.empty()) ++out[n];
        } else if (t.isValueType() && t.getValueType() == LangAPI::ValueType::Array) {
            for (const auto &param : t.template_parameters) {
                if (std::holds_alternative<LangAPI::Type>(param)) {
                    walk(std::get<LangAPI::Type>(param), true);
                }
            }
        } else {
            for (const auto &param : t.template_parameters) {
                if (std::holds_alternative<LangAPI::Type>(param)) {
                    walk(std::get<LangAPI::Type>(param), false);
                }
            }
        }
    };
    walk(type, false);
    return std::make_pair(out, to_forward_declare);
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

// Generate:
    //
    //     to_string() -> String
    //
    // producing:
    //
    //     ClassName {field1 = value1, field2 = value2, ...}
    //
    // The representation is constructed as a semantic concatenation expression,
    // rather than using target-language-specific mutable-string operations.
    auto generateToStringFunction(
        const LangAPI::Class& cls,
        const stdu::vector<MemberInfo>& members
    ) -> LangAPI::Function {
        LangAPI::Function function;

        function.name = "to_string";
        function.type = LangAPI::Type{
            LangAPI::ValueType::String
        };
        function.is_const = true;

        auto stringLiteral = [](std::string value)
            -> LangAPI::Expression {
            return LangAPI::String::createExpression(
                LangAPI::String{
                    .value = std::move(value)
                }
            );
        };

        stdu::vector<LangAPI::Expression> parts;

        // ClassName {
        parts.push_back(
            stringLiteral(cls.name + " {")
        );

        for (std::size_t i = 0; i < members.size(); ++i) {
            const auto& member = members[i];

            // field =
            parts.push_back(
                stringLiteral(member.name + " = ")
            );
            if (isNodeType(member.type)) {
                // value: call member.to_string()
                parts.push_back(
                    LangAPI::StorageSymbol::createExpression(
                        LangAPI::StorageSymbol{
                            LangAPI::Symbol::createExpression(LangAPI::Symbol {member.name}),
                            stdu::vector<LangAPI::StorageSymbol::PathPart>{
                                LangAPI::FunctionCall{
                                    .name = std::make_shared<LangAPI::Symbol>(
                                        LangAPI::Symbol{"to_string"}
                                    ),
                                }
                            }
                        }
                    )
                );
            } else {
                parts.push_back(LangAPI::Symbol::createExpression(LangAPI::Symbol {member.name}));
            }
            if (i + 1 < members.size()) {
                parts.push_back(
                    stringLiteral(", ")
                );
            }
        }

        // }
        parts.push_back(
            stringLiteral("}")
        );

        // return concat(...)
        function.statements.push_back(
            LangAPI::Return::createStatement(
                LangAPI::Return{
                    .value =
                        LangAPI::IspaLibFunctionCall::createExpression(
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
        LangAPI::Type{LangAPI::Symbol{"::ISPA_STD::ASTPrinter<std::ostream>&"}},
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
    auto printerNode = [&](const std::string& name, bool last) {
        LangAPI::StorageSymbol call{
            symbolExpr("printer"),
            std::vector<LangAPI::StorageSymbol::PathPart>{
                LangAPI::FunctionCall{
                    .name = std::make_shared<LangAPI::Symbol>(
                        LangAPI::Symbol{"node"}
                    ),
                    .args = {
                        LangAPI::String::createExpression(
                            LangAPI::String{.value = name}
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
        bool last
    ) {
        LangAPI::StorageSymbol call{
            symbolExpr("printer"),
            std::vector<LangAPI::StorageSymbol::PathPart>{
                LangAPI::FunctionCall{
                    .name = std::make_shared<LangAPI::Symbol>(
                        LangAPI::Symbol{"node_with_value"}
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
                //
                // Leaf:
                //   printer.node_with_value(
                //       "member",
                //       <object>.data.<member>,
                //       last
                //   );
                //

                print_func.statements.push_back(
                    printerNodeValue(
                        member.name,
                    symbolExpr(member.name),
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
    function.parameters.emplace_back(LangAPI::Type{LangAPI::Symbol{"std::ostream&"}}, "os");
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
    function.parameters.emplace_back(LangAPI::Type{LangAPI::Symbol{cls.name}}, cls.name);
    function.statements.push_back(LangAPI::StorageSymbol::createStatement(
        LangAPI::StorageSymbol{
        LangAPI::Symbol::createExpression(LangAPI::Symbol{cls.name}),
        stdu::vector<LangAPI::StorageSymbol::PathPart>{LangAPI::FunctionCall{
            .name = std::make_shared<LangAPI::Symbol>(LangAPI::Symbol{"print"}),
            .args = {LangAPI::Symbol::createExpression(LangAPI::Symbol{"os"})}
        }}
    }));
    function.statements.push_back(LangAPI::Return::createStatement(LangAPI::Return{
        .value = LangAPI::Symbol::createExpression(LangAPI::Symbol{"os"})
    }));
    return function;
}

    auto ConstructTypes::constructTokensAndRulesEnum() -> void {
        LangAPI::Enum tokens_enum("Tokens", {"NONE"});
        LangAPI::Enum rules_enum("Rules", {"NONE"});
        for (const auto &dtb : lexer_builder.getDataBlocks()) {
            tokens_enum.value.push_back(corelib::text::join(dtb.first, "_"));
        }
        for (const auto &dtb : ir.getDataBlocks()) {
            rules_enum.value.push_back(corelib::text::join(dtb.first, "_"));
        }
        holder.push(tokens_enum);
        holder.push(rules_enum);
    }
    auto ConstructTypes::constructTokensAndRulesEnumToString() -> void {
        auto constructSwitchFromDataBlockList = [](const LLIR::DataBlockList &list, const char* name) {
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
        // Tree node to represent namespaces/types
        Node root;
        stdu::vector<Name> order;
        utype::unordered_map<Name, std::size_t> name_to_index_in_order;
        utype::unordered_map<Name, std::size_t> reorder_count;

        // 1. Build the tree
        auto buildDtb = [&root, &order, &name_to_index_in_order](const LLIR::DataBlockList &dtb_list) {
            for (const auto& dtb : dtb_list) {
                const Name &nameParts = dtb.first;
                name_to_index_in_order.emplace(nameParts, order.size());
                order.push_back(nameParts);
                root.find_or_emplace(nameParts)->data = dtb.second;
            }

        };
        auto lexer_data_blocks = lexer_builder.getDataBlocks();
        auto parser_data_blocks = ir.getDataBlocks();
        buildDtb(lexer_data_blocks);
        buildDtb(parser_data_blocks);
        // 2. Sort
        // --- Collect deps ---
        utype::unordered_map<Name, utype::unordered_set<Name>> deps;
        utype::unordered_map<Name, std::size_t> usage_count;
        utype::unordered_set<Name> to_forward_declare;
        for (const Name &n : order) {
            const Node *node = root.find(n);
            if (!node) { deps[n] = {}; continue; }

            utype::unordered_set<Name> used;
            if (node->data.is_regular_data_block()) {
                auto dt = node->data.getRegularDataBlock().second;
                auto [s, _to_forward_declare] = collectReferencedNames(dt);
                for (const auto &[name, count] : s) {
                    usage_count[name] += count;
                    used.insert(name);
                }
                to_forward_declare.insert(_to_forward_declare.begin(), _to_forward_declare.end());
            } else if (node->data.is_inclosed_map()) {
                for (const auto &p : node->data.getInclosedMap()) {
                    auto [s, _to_forward_declare] = collectReferencedNames(p.second.second);
                    for (const auto &[name, count] : s) {
                        usage_count[name] += count;
                        used.insert(name);
                    }
                    to_forward_declare.insert(_to_forward_declare.begin(), _to_forward_declare.end());
                }
            }
            // remove self-dependency if any
            used.erase(n);
            deps[n] = std::move(used);
        }

        // --- Build reverse graph: dependency -> set of dependents
        utype::unordered_map<Name, utype::unordered_set<Name>> rev_deps;
        for (const Name &n : order) rev_deps[n]; // ensure key exists
        for (const auto &kv : deps) {
            const Name &node = kv.first;
            for (const Name &dep : kv.second) {
                // if `dep` is not one of our known names, skip it
                if (name_to_index_in_order.find(dep) == name_to_index_in_order.end()) continue;
                rev_deps[dep].insert(node); // dep -> node (dependent)
            }
        }

        // --- indegree = number of dependencies each node has
        utype::unordered_map<Name, std::size_t> indeg;
        for (const Name &n : order) indeg[n] = deps[n].size();

        // --- initialize queue with indeg == 0, preserving original order
        std::deque<Name> q;
        for (const Name &n : order) {
            if (indeg[n] == 0) q.push_back(n);
        }

        // --- Kahn: when we pop v, we iterate rev_deps[v] (dependents) and decrement them
        stdu::vector<Name> sorted;
        sorted.reserve(order.size());
        while (!q.empty()) {
            Name v = q.front(); q.pop_front();
            sorted.push_back(v);
            // for each dependent of v
            for (const Name &dependent : rev_deps[v]) {
                // decrement indegree of dependent (it had one less unresolved dependency)
                if (indeg[dependent] > 0) {
                    --indeg[dependent];
                    if (indeg[dependent] == 0) {
                        q.push_back(dependent);
                    }
                }
            }
        }

        // cycles: nodes with indeg > 0
        utype::unordered_set<Name> cycled;
        utype::unordered_set<Name> dependent;
        if (sorted.size() != order.size()) {
            cycled.reserve(order.size() - sorted.size());
            for (const Name &n : order) {
                if (indeg[n] > 0) cycled.insert(n);
            }
            // append remaining preserving original order (or compute SCCs and handle)
            utype::unordered_set<Name> independent;
            for (const Name &n : cycled) {
                if (usage_count[n] == 1) {
                    independent.insert(n);
                } else {
                    dependent.insert(n);
                }
            }
            stdu::vector<Name> sorted_final;

            // 3a: Add all acyclic nodes in original Kahn order
            for (const Name &n : sorted) {
                if (!cycled.contains(n)) {
                    sorted_final.push_back(n);
                }
            }

            // 3b: Topologically sort independents among themselves
            utype::unordered_map<Name, std::size_t> indeg2;
            for (const Name &n : independent) {
                indeg2[n] = 0;
            }

            // Count dependencies **only on other independent nodes** (acyclic already sorted)
            for (const auto& [u, adj] : deps) {
                if (!independent.contains(u)) continue;
                for (const Name &v : adj) {
                    if (independent.contains(v)) ++indeg2[u]; // increment **for u**, not v
                }
            }

            // Kahn for independents
            std::deque<Name> q2;
            for (const Name &n : independent) {
                if (indeg2[n] == 0) q2.push_back(n);
            }

            while (!q2.empty()) {
                Name v = q2.front(); q2.pop_front();
                sorted_final.push_back(v);

                // Decrement indegree for independent nodes that depend on v
                for (const Name &dep : rev_deps[v]) {
                    if (!independent.contains(dep)) continue;
                    --indeg2[dep];
                    if (indeg2[dep] == 0) q2.push_back(dep);
                }
            }

            // 3c: Append dependents in original input order
            for (const Name &n : order) {
                if (dependent.contains(n)) sorted_final.push_back(n);
            }

            // Replace original sorted
            sorted = std::move(sorted_final);
        }
        auto needs_box = dependent;
        needs_box.insert(to_forward_declare.begin(), to_forward_declare.end());
        // `sorted` now contains dependency-first order for acyclic parts,
        // followed by any cyclical group(s) (which you must handle specially).
        // Use `sorted` for emission.

        // 3. Wrap everything into `Types` and FlatTypes namespace
        LangAPI::Namespace flatTypesNamespace {.name = "FlatTypes"};
        LangAPI::Namespace typesNamespace { .name = "Types" };
        for (const auto &dep : dependent) {
            flatTypesNamespace.declarations.push_back(LangAPI::ForwardDeclaredClass::createDeclaration(LangAPI::ForwardDeclaredClass {.name = corelib::text::join(dep, "_")}));
        }
        for (const auto &dep : to_forward_declare) {
            flatTypesNamespace.declarations.push_back(LangAPI::ForwardDeclaredClass::createDeclaration(LangAPI::ForwardDeclaredClass {.name = corelib::text::join(dep, "_")}));
        }
        // output types to flatTypes namespace
        auto switchToFlatType = [&](LangAPI::Type &t) {
            if (t.isSymbol()) {
                std::string path;
                std::size_t count = 0;
                for (const auto &part : t.getSymbol().path) {
                    if (std::holds_alternative<std::string>(part)) {
                        const auto &part_str = std::get<std::string>(part);
                        path += part_str + "_";
                    }
                    count++;
                }
                path.pop_back();
                t.type = LangAPI::Symbol {path};
            }
        };
        std::function<void(LangAPI::Type &)> switchToFlatTypeRecursively = [&](LangAPI::Type &t) {
            if (auto vt = t.getValueType(); vt == LangAPI::ValueType::Token || vt == LangAPI::ValueType::Rule || vt == LangAPI::ValueType::TokenResult || vt == LangAPI::ValueType::RuleResult || vt == LangAPI::ValueType::Box) {
                switchToFlatType(std::get<LangAPI::Type>(t.template_parameters[0]));
            } else {
                for (auto &templ : t.template_parameters) {
                    if (std::holds_alternative<LangAPI::Type>(templ)) {
                        switchToFlatTypeRecursively(std::get<LangAPI::Type>(templ));
                    }
                }
            }
        };

        // Boxes a referenced (Symbol) type when it names something in `needs_box`
        // (i.e. it's part of a dependency cycle, or was flagged for forward
        // declaration) — this is what makes `Token<Identifier>` become
        // `Token<Box<Identifier>>` for types that can't be stored by value here.
        std::function<void(LangAPI::Type &)> makeDependentTypeBox = [&](LangAPI::Type &t) {
            if (t.isSymbol()) {
                Name actual_name;
                const auto &first = t.getSymbol().path[0];
                for (const auto &part : t.getSymbol().path) {
                    if (!std::holds_alternative<std::string>(part))
                        return;
                    actual_name.push_back(std::get<std::string>(part));
                }
                if (!needs_box.contains(actual_name))
                    return;
                t.template_parameters = {LangAPI::Type {LangAPI::Symbol {corelib::text::join(actual_name, "_")}}};
                t.type = LangAPI::ValueType::Box;
            } else if (t.isValueType()) {
                const auto &vtype = t.getValueType();
                if (vtype == LangAPI::ValueType::Box || vtype == LangAPI::ValueType::Token || vtype == LangAPI::ValueType::Rule || vtype == LangAPI::ValueType::RuleResult || vtype == LangAPI::ValueType::TokenResult) {
                    makeDependentTypeBox(std::get<LangAPI::Type>(t.template_parameters[0]));
                }
            }
        };
        // Walks a field's type looking for the Token/Rule/Box wrapper that
        // carries a referenced type, and hands that referenced type to
        // makeDependentTypeBox above.
        std::function<void(LangAPI::Type &)> makeDependentTypeBoxRecursively = [&](LangAPI::Type &t) {
            if (auto vt = t.getValueType(); vt == LangAPI::ValueType::Token || vt == LangAPI::ValueType::Rule || vt == LangAPI::ValueType::TokenResult || vt == LangAPI::ValueType::RuleResult || vt == LangAPI::ValueType::Box) {
                makeDependentTypeBox(std::get<LangAPI::Type>(t.template_parameters[0]));
            } else {
                for (auto &templ : t.template_parameters) {
                    if (std::holds_alternative<LangAPI::Type>(templ)) {
                        makeDependentTypeBoxRecursively(std::get<LangAPI::Type>(templ));
                    }
                }
            }
        };

        for (const auto &fullName : sorted) {
            const Node *old = root.find(fullName);
            if (!old) continue; // or throw
            LangAPI::Class c;
            c.name = corelib::text::join(fullName, "_");

            // Collected AFTER makeDependentTypeBoxRecursively/switchToFlatTypeRecursively
            // run on each field's type below, so `members` reflects each field's
            // actual, final declared type (Box-wrapping included) — exactly what
            // generateToStringFunction/generatePrintFunction need to see.
            stdu::vector<MemberInfo> members;

            if (old->data.is_regular_data_block()) {
                auto type = old->data.getRegularDataBlock().second;
                makeDependentTypeBoxRecursively(type);
                switchToFlatTypeRecursively(type);
                members.push_back(MemberInfo{"value", type});
                c.data.push_back(
                    std::make_pair(
                        std::make_shared<LangAPI::Declaration>(LangAPI::Variable::createDeclaration(LangAPI::Variable {.name = "value", .type = type})),
                        LangAPI::Visibility::Public
                    )
                );
            } else if (old->data.is_inclosed_map()) {
                for (const auto &[name, type] : old->data.getInclosedMap()) {
                    auto t = type.second;
                    makeDependentTypeBoxRecursively(t);
                    switchToFlatTypeRecursively(t);
                    members.push_back(MemberInfo{name, t});
                    c.data.push_back((std::make_pair(std::make_shared<LangAPI::Declaration>(LangAPI::Variable::createDeclaration(LangAPI::Variable {.name = name, .type = t})), LangAPI::Visibility::Public)));
                }
            }

            // Member rendering shares one printer through every nested node.
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

            flatTypesNamespace.declarations.push_back(LangAPI::Class::createDeclaration(std::move(c)));
        }

        // build index map (fast lookup of sorted order)
        utype::unordered_map<Name, std::size_t> sorted_index;
        sorted_index.reserve(sorted.size());
        for (std::size_t i = 0; i < sorted.size(); ++i) sorted_index[sorted[i]] = i;

        // recursive factory: takes old Node* and the full path for that node
        std::function<std::unique_ptr<SortedNode>(const Node*, const Name&)> make_sorted_from_node;
        make_sorted_from_node = [&](const Node* old_ptr, const Name &fullPath) -> std::unique_ptr<SortedNode> {
            if (!old_ptr) return nullptr;
            auto out = std::make_unique<SortedNode>();

            // copy data (or move if you intentionally want to steal)
            out->data = old_ptr->data;

            // collect child names and their indices (if known)
            std::vector<std::pair<std::size_t, std::string>> child_order;
            child_order.reserve(old_ptr->children.size());

            std::size_t unknown_base = sorted.size() + 1; // index for children not in 'sorted'
            for (const auto &child_pair : old_ptr->children) {
                Name childFull = fullPath;
                childFull.push_back(child_pair.first);
                auto it = sorted_index.find(childFull);
                std::size_t idx = (it != sorted_index.end() ? it->second : unknown_base++);
                child_order.emplace_back(idx, child_pair.first);
            }

            // stable sort children by their position in global 'sorted'
            std::stable_sort(child_order.begin(), child_order.end(),
                             [](const auto &a, const auto &b){ return a.first < b.first; });

            // build children in sorted order, recursively passing the child's full path
            for (const auto &p : child_order) {
                const std::string &childName = p.second;
                Name childFull = fullPath;
                childFull.push_back(childName);
                const Node *childOld = old_ptr->children.at(childName).get();
                auto childSorted = make_sorted_from_node(childOld, childFull);
                out->children.emplace(childName, std::move(childSorted));
            }

            return out;
        };

        // top-level loop: build new_tree following 'sorted' order
        SortedNode new_tree;
        for (const auto &fullName : sorted) {
            // walk or create path in new_tree, but use factory only for the leaf node
            SortedNode* cur = &new_tree;
            for (std::size_t i = 0; i < fullName.size(); ++i) {
                const auto &part = fullName[i];
                auto it = cur->children.find(part);
                if (it == cur->children.end()) {
                    cur->children.emplace(part, std::make_unique<SortedNode>());
                    it = cur->children.find(part);
                }
                cur = it->second.get();
            }

            // populate cur->data + its children using make_sorted_from_node
            const Node *old = root.find(fullName);
            if (!old) continue; // or throw
            // take the result subtree (already sorted among its children)
            auto subtree = make_sorted_from_node(old, fullName);
            if (subtree) {
                // move data and children into the placeholder node `cur`
                cur->data = std::move(subtree->data);
                cur->children = std::move(subtree->children);
            }
        }

        // 2. Recursive builder
        std::function<LangAPI::Declaration(const SortedNode&, const std::string&, stdu::vector<std::string>&)> build;
        build = [&](const SortedNode& node, const std::string& name, stdu::vector<std::string> &fullname) -> LangAPI::Declaration {
            // Build children first
            stdu::vector<std::pair<Name, const SortedNode*>> child_output_order;
            if (node.children.empty()) {
                return LangAPI::TypeAlias::createDeclaration(LangAPI::TypeAlias {.name = name, .type = LangAPI::Symbol {"FlatTypes", corelib::text::join(fullname, "_")}});
            }
            LangAPI::Class s;
            s.name = name;
            s.inherit_members = {std::make_pair(LangAPI::Visibility::Public, LangAPI::Symbol {"FlatTypes", corelib::text::join(fullname, "_")})};
            for (const auto& [childName, childNode] : node.children) {
                fullname.push_back(childName);
                auto nested = build(*childNode, childName, fullname);
                fullname.pop_back();
                s.data.push_back(std::make_pair(std::make_shared<LangAPI::Declaration>(std::move(nested)), LangAPI::Visibility::Public));
            }
            return LangAPI::Class::createDeclaration(std::move(s));
        };
        // Build top-level children of root
        for (const auto& [name, node] : new_tree.children) {
            Name fullname = {name};
            auto topDecls = build(*node, name, fullname);
            typesNamespace.declarations.push_back(std::move(topDecls));
        }
        holder.push(flatTypesNamespace);
        holder.push(typesNamespace);
    }
}
