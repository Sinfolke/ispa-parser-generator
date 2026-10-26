module AST.BackendGuard;
import std;

namespace {
[[noreturn]] void reject(const AST::SourcePosition& p, const char* feature) {
    throw std::runtime_error(p.file + ":" + std::to_string(p.line) + ":" +
        std::to_string(p.column) + ": feature " + feature +
        " is not yet supported by the C++ backend");
}

void check(const AST::FrontendFeature& feature) {
    std::visit([](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        const char* name;
        if constexpr (std::is_same_v<T, AST::PythonAction>) name = "Python action";
        else if constexpr (std::is_same_v<T, AST::FailBlock>) name = "fail block";
        else if constexpr (std::is_same_v<T, AST::FailBinding>) name = "rule fail binding";
        else if constexpr (std::is_same_v<T, AST::FailMarker>) name = "fail marker";
        else if constexpr (std::is_same_v<T, AST::RegionCapture>) name = "region capture";
        else if constexpr (std::is_same_v<T, AST::DynamicPattern>) name = "dynamic pattern";
        else if constexpr (std::is_same_v<T, AST::Counter>) name = "bounded counter";
        else if constexpr (std::is_same_v<T, AST::SharedCounterRegistration>) name = "shared counter registration";
        else if constexpr (std::is_same_v<T, AST::VariableCapture>) name = "variable capture";
        else if constexpr (std::is_same_v<T, AST::DeclarationSyntax>) name = "rule parameters";
        else if constexpr (std::is_same_v<T, AST::SpacePattern>) name = "explicit whitespace pattern";
        else if constexpr (std::is_same_v<T, AST::StreamMatch>) name = "token stream match";
        else name = "advanced rule";
        reject(node.position, name);
    }, feature);
}

void check(const AST::RuleMember& member, bool token, bool actions) {
    for (const auto& f : member.features) {
        if (actions && !token && member.isName() && member.getName().isNonterminal()) if (const auto* syntax = std::get_if<AST::DeclarationSyntax>(&f);
            syntax && syntax->modifiers.empty()) continue;
        if (actions && !token && (std::holds_alternative<AST::PythonAction>(f) ||
            std::holds_alternative<AST::VariableCapture>(f) || std::holds_alternative<AST::FailMarker>(f) ||
            std::holds_alternative<AST::AdvancedRule>(f) || std::holds_alternative<AST::StreamMatch>(f))) continue;
        if (actions && token && (std::holds_alternative<AST::VariableCapture>(f) ||
                                 std::holds_alternative<AST::FailMarker>(f))) continue;
        if (const auto* counter = std::get_if<AST::Counter>(&f); counter && !token) {
            if (!(member.isName() || member.isString()) ||
                (member.quantifier != '*' && member.quantifier != '+'))
                reject(counter->position, "counter on a group, choice, or optional matcher");
            if (counter->kind == "name") continue;
            if (counter->kind == "number" && !counter->bound.empty() &&
                std::ranges::all_of(counter->bound, [](char c) { return c >= '0' && c <= '9'; })) {
                try { (void)std::stoll(counter->bound); }
                catch (...) { reject(counter->position, "counter outside the supported integer range"); }
                continue;
            }
        }
        if (!token && std::holds_alternative<AST::SharedCounterRegistration>(f) &&
            (member.isName() || member.isString())) continue;
        check(f);
    }
    if (member.isGroup())
        for (const auto& child : member.getGroup().values) check(*child, token, actions);
    if (member.isOp())
        for (const auto& child : member.getOp().options) check(*child, token, actions);
}
}

void AST::requireCppSupport(const Tree& tree, bool ll_actions) {
    if (!ll_actions) for (const auto& fail : tree.getFailBlocks()) reject(fail.position, "fail block");
    for (const auto& [name, rule] : tree.getTreeMap()) {
        const bool token = AST::RuleMemberName{.name = name}.isTerminal();
        const bool advanced = std::ranges::any_of(rule.features, [](const auto& feature) {
            return std::holds_alternative<AST::AdvancedRule>(feature);
        });
        if (ll_actions && token && advanced) {
            const auto uses = [&](auto&& self, const AST::RuleMember& member) -> bool {
                if (member.isName() && member.getName().name == name) return true;
                if (member.isGroup()) for (const auto& child : member.getGroup().values)
                    if (self(self, *child)) return true;
                if (member.isOp()) for (const auto& child : member.getOp().options)
                    if (self(self, *child)) return true;
                return false;
            };
            for (const auto& [other_name, other] : tree.getTreeMap())
                if (other_name != name && AST::RuleMemberName{.name = other_name}.isTerminal())
                    for (const auto& member : other.rule_members)
                        if (uses(uses, *member)) reject(rule.position, "advanced token embedded in another token");
        }
        if (ll_actions && token && advanced && rule.rule_members.size() == 1 &&
            rule.rule_members.front()->isGroup() && rule.rule_members.front()->getGroup().values.empty())
            reject(rule.position, "advanced token without a match");
        for (const auto& feature : rule.features) {
            if (const auto* syntax = std::get_if<AST::DeclarationSyntax>(&feature);
                syntax && syntax->parameters.empty() && syntax->templateParameters.empty() &&
                std::ranges::all_of(syntax->modifiers, [](const auto& m) { return m == "inline" || m == "skip"; })) continue;
            if (const auto* syntax = std::get_if<AST::AdvancedRule>(&feature))
                for (const auto& child : syntax->syntax.children)
                    if (child.field == "dep" && !child.children.empty()) reject(syntax->position, "advanced rule dependencies");
            // A parser advanced rule made only of ordinary `match` clauses is
            // already an LR production after normalization.  Statements and
            // stream matches still need semantic callbacks and stay rejected.
            if (!ll_actions && !token && std::holds_alternative<AST::AdvancedRule>(feature)) continue;
            if (ll_actions && (std::holds_alternative<AST::AdvancedRule>(feature) ||
                std::holds_alternative<AST::PythonAction>(feature) ||
                std::holds_alternative<AST::DeclarationSyntax>(feature))) continue;
            if (ll_actions && std::holds_alternative<AST::FailBinding>(feature)) continue;
            check(feature);
        }
        for (const auto& member : rule.rule_members) check(*member, token, ll_actions);
    }
}
