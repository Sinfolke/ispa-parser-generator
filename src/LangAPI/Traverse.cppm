export module LangAPI.Traverse;
import LangAPI;
import std;

export namespace LangAPI::Traverse {

// Only AST nodes listed below are visited. Non-node data (strings, numbers,
// enum flags, vector/pair/variant wrappers) is traversed transparently.
template<class T> struct Children;

template<class T>
concept AstNode = requires { Children<std::remove_cvref_t<T>>::is_node; };

namespace detail {
    template<class T> struct is_variant : std::false_type {};
    template<class... Ts> struct is_variant<std::variant<Ts...>> : std::true_type {};
    template<class T> struct is_optional : std::false_type {};
    template<class U> struct is_optional<std::optional<U>> : std::true_type {};
    template<class T> struct is_shared : std::false_type {};
    template<class U> struct is_shared<std::shared_ptr<U>> : std::true_type {};
    template<class T> struct is_pair : std::false_type {};
    template<class A, class B> struct is_pair<std::pair<A,B>> : std::true_type {};

    template<class T, class F>
    void descend(T& value, F& visit) {
        using U = std::remove_cvref_t<T>;
        if constexpr (AstNode<U>) {
            visit(value);
        } else if constexpr (is_variant<U>::value) {
            std::visit([&](auto& v) { descend(v, visit); }, value);
        } else if constexpr (is_optional<U>::value || is_shared<U>::value) {
            if (value) descend(*value, visit);
        } else if constexpr (is_pair<U>::value) {
            descend(value.first, visit);
            descend(value.second, visit);
        } else if constexpr (std::ranges::input_range<T> &&
                             !std::same_as<U, std::string>) {
            for (auto& element : value) descend(element, visit);
        }
    }
}

// A Children specialization enumerates direct AST children, NOT itself.
// Calls into shared pointers are structural ownership assumptions; do not
// mutate container topology or switch variants while walking.
template<> struct Children<LangAPI::Declarations> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
        for (auto& child : node) detail::descend(child, f);
    }
};

template<> struct Children<LangAPI::Statements> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
        for (auto& child : node) detail::descend(child, f);
    }
};

template<> struct Children<LangAPI::Expression> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
        for (auto& child : node) detail::descend(child, f);
    }
};

template<> struct Children<LangAPI::Array> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.values, f);
        detail::descend(node.template_parameters, f);
    }
};

template<> struct Children<LangAPI::FixedSizeArray> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.values, f);
        detail::descend(node.template_parameters, f);
    }
};

template<> struct Children<LangAPI::Map> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.keys, f);
        detail::descend(node.values, f);
        detail::descend(node.template_parameters, f);
    }
};

template<> struct Children<LangAPI::ArrayMethodCall> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.args, f);
    }
};

template<> struct Children<LangAPI::FunctionCall> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.name, f);
        detail::descend(node.template_parameters, f);
        detail::descend(node.args, f);
    }
};

template<> struct Children<LangAPI::IspaLibSymbol> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.template_parameters, f);
    }
};

template<> struct Children<LangAPI::Symbol> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.path, f);
    }
};

template<> struct Children<LangAPI::StorageOffset> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.offset, f);
    }
};

template<> struct Children<LangAPI::StorageSymbol> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.what, f);
        detail::descend(node.path, f);
    }
};

template<> struct Children<LangAPI::IspaLibFunctionCall> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.symbol, f);
        detail::descend(node.args, f);
    }
};

template<> struct Children<LangAPI::Inheritance> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.name, f);
        detail::descend(node.args, f);
    }
};

template<> struct Children<LangAPI::IspaLibDfaTransition> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.transition_type, f);
    }
};

template<> struct Children<LangAPI::IspaLibDfaState> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.transitions, f);
    }
};

template<> struct Children<LangAPI::IspaLibDfaSpanMultiTableState> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.mutli_table_transitions, f);
    }
};

template<> struct Children<LangAPI::IspaLibDfaEmptyState> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.construction_lambda, f);
    }
};

template<> struct Children<LangAPI::IspaLibDfaSpan> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.type, f);
        detail::descend(node.assing_name, f);
    }
};

template<> struct Children<LangAPI::Reference> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.value, f);
    }
};

template<> struct Children<LangAPI::Span> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.type, f);
        detail::descend(node.sym, f);
    }
};

template<> struct Children<LangAPI::MakeTuple> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.args, f);
    }
};

template<> struct Children<LangAPI::GetVariant> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.type, f);
        detail::descend(node.sym, f);
    }
};

template<> struct Children<LangAPI::CheckVariant> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.type, f);
        detail::descend(node.sym, f);
    }
};

template<> struct Children<LangAPI::CharToStringConstructor> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.what, f);
    }
};

template<> struct Children<LangAPI::ToString> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.what, f);
    }
};

template<> struct Children<LangAPI::Cast> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.type, f);
        detail::descend(node.what, f);
    }
};

template<> struct Children<LangAPI::GetFromBox> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.what, f);
    }
};

template<> struct Children<LangAPI::Instance> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N&, F&) {}
};

template<> struct Children<LangAPI::RValue> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.get(), f);
    }
};

template<> struct Children<LangAPI::Type> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.type, f);
        detail::descend(node.template_parameters, f);
    }
};

template<> struct Children<LangAPI::Function> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.type, f);
        detail::descend(node.parameters, f);
        detail::descend(node.statements, f);
        detail::descend(node.template_parameters, f);
    }
};

template<> struct Children<LangAPI::Class> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.name, f);
        detail::descend(node.data, f);
        detail::descend(node.inherit_members, f);
        detail::descend(node.constructor, f);
        detail::descend(node.to_str_fun, f);
        detail::descend(node.output_fun, f);
        detail::descend(node.dependecies, f);
    }
};

template<> struct Children<LangAPI::Namespace> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.declarations, f);
    }
};

template<> struct Children<LangAPI::TypeAlias> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.type, f);
    }
};

template<> struct Children<LangAPI::Variable> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.type, f);
        detail::descend(node.value, f);
        detail::descend(node.set, f);
    }
};

template<> struct Children<LangAPI::Declaration> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.value, f);
    }
};

template<> struct Children<LangAPI::Return> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.value, f);
    }
};

template<> struct Children<LangAPI::StringCompare> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.str, f);
    }
};

template<> struct Children<LangAPI::VariableAssignment> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.name, f);
        detail::descend(node.value, f);
    }
};

template<> struct Children<LangAPI::AssignCounter> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.v, f);
    }
};

template<> struct Children<LangAPI::Lambda> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.statements, f);
    }
};

template<> struct Children<LangAPI::ExpressionValue> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.value, f);
    }
};

template<> struct Children<LangAPI::ConditionalElement> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.expr, f);
        detail::descend(node.stmt, f);
    }
};

template<> struct Children<LangAPI::If> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.expr, f);
        detail::descend(node.stmt, f);
        detail::descend(node.else_stmt, f);
    }
};

template<> struct Children<LangAPI::While> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.expr, f);
        detail::descend(node.stmt, f);
    }
};

template<> struct Children<LangAPI::DoWhile> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.expr, f);
        detail::descend(node.stmt, f);
    }
};

template<> struct Children<LangAPI::Switch> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.expression, f);
        detail::descend(node.cases, f);
    }
};

template<> struct Children<LangAPI::Throw> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.throw_value, f);
    }
};

template<> struct Children<LangAPI::Statement> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N& node, F& f) {
                detail::descend(node.value, f);
    }
};

template<> struct Children<LangAPI::Char> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N&, F&) {}
};

template<> struct Children<LangAPI::Int> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N&, F&) {}
};

template<> struct Children<LangAPI::Bool> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N&, F&) {}
};

template<> struct Children<LangAPI::Float> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N&, F&) {}
};

template<> struct Children<LangAPI::String> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N&, F&) {}
};

template<> struct Children<LangAPI::Pos> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N&, F&) {}
};

template<> struct Children<LangAPI::Property> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N&, F&) {}
};

template<> struct Children<LangAPI::Token> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N&, F&) {}
};

template<> struct Children<LangAPI::Rule> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N&, F&) {}
};

template<> struct Children<LangAPI::TokenResult> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N&, F&) {}
};

template<> struct Children<LangAPI::RuleResult> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N&, F&) {}
};

template<> struct Children<LangAPI::IspaLibDfaSpanCharState> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N&, F&) {}
};

template<> struct Children<LangAPI::EmptyInitializer> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N&, F&) {}
};

template<> struct Children<LangAPI::ForwardDeclaredClass> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N&, F&) {}
};

template<> struct Children<LangAPI::Enum> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N&, F&) {}
};

template<> struct Children<LangAPI::Break> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N&, F&) {}
};

template<> struct Children<LangAPI::Continue> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N&, F&) {}
};

template<> struct Children<LangAPI::CounterIncreament> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N&, F&) {}
};

template<> struct Children<LangAPI::ResetPosCounter> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N&, F&) {}
};

template<> struct Children<LangAPI::PushPosCounter> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N&, F&) {}
};

template<> struct Children<LangAPI::PopPosCounter> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N&, F&) {}
};

template<> struct Children<LangAPI::SkipSpaces> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N&, F&) {}
};

template<> struct Children<LangAPI::DfaLookup> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N&, F&) {}
};

template<> struct Children<LangAPI::ReportError> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N&, F&) {}
};

template<> struct Children<LangAPI::DFADebug> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N&, F&) {}
};

template<> struct Children<LangAPI::Comment> {
    static constexpr bool is_node = true;
    template<class N, class F> static void each(N&, F&) {}
};

// Visit direct AST children (not the root).
template<AstNode T, class F>
void forEachChild(T& node, F&& f) {
    auto&& fn = f;
    Children<std::remove_cvref_t<T>>::each(node, fn);
}

// Preorder traversal. Callback must accept all visited AST node types.
template<AstNode Root, class F>
void walk(Root& root, F&& f) {
    auto&& callback = f;
    auto recurse = [&](auto&& self, auto& node) -> void {
        std::invoke(callback, node);
        forEachChild(node, [&](auto& child) { self(self, child); });
    };
    recurse(recurse, root);
}

template<class Target, AstNode Root, class F>
void forEach(Root& root, F&& f) {
    auto&& callback = f;
    walk(root, [&](auto& node) {
        if constexpr (std::same_as<std::remove_cvref_t<decltype(node)>, Target>)
            std::invoke(callback, node);
    });
}

template<class Target, AstNode Root>
auto collect(Root& root) {
    using P = std::conditional_t<std::is_const_v<std::remove_reference_t<Root>>,
                                 const Target*, Target*>;
    std::vector<P> output;
    forEach<Target>(root, [&](auto& value) { output.push_back(&value); });
    return output;
}

template<class Target, AstNode Root, class Pred>
auto find(Root& root, Pred&& pred) {
    using P = std::conditional_t<std::is_const_v<std::remove_reference_t<Root>>,
                                 const Target*, Target*>;
    P result = nullptr;
    forEach<Target>(root, [&](auto& value) {
        if (!result && std::invoke(pred, value)) result = &value;
    });
    return result;
}
} // namespace LangAPI::Traverse
