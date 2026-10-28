module Parser.Base;
import std;
import corelib;
import LangAPI;
import AST.Tree;
import AST.API;
namespace Parser {
    auto ParserBase::var_name(const std::vector<std::string> &name, bool list) -> std::string {
        auto &idx = ctx.variables_lowered[name];
        std::string vname;
        if (idx == 0) {
            vname = std::string("value_") + (list ? "list_" : "") + corelib::text::join(name, "_");
        } else {
            vname = std::string("value_") + (list ? "list_" : "") + corelib::text::join(name, "_") + "_" + std::to_string(idx);
        }
        ++idx;
        return vname;
    }

    auto ParserBase::fun_name(const std::vector<std::string> &name) -> std::string {
        std::string fun_name = name[0];
        for (std::size_t i = 1; i < name.size(); ++i) {
            fun_name += std::toupper(name[i][0]);
            for (std::size_t j = 1; j < name[i].size(); ++j) {
                fun_name += name[i][j];
            }
        }
        return fun_name;
    }

}


namespace Parser {
namespace {
    constexpr const char* kTokensRoot = "Tokens";

    struct TokenSet {
        std::set<AST::LLkToken> tokens;
        bool end = false;
    };

    struct RecoverySpec {
        std::string rule;
        std::string expected_text;
        TokenSet expected, continuation, follow, sync;
        bool allow_insertion = true;
        bool allow_deletion = true;
        bool allow_replacement = true;
        std::size_t max_skip = 64;
    };

    // ---------- LangAPI generation helpers ----------
    auto string_arg(std::string str) -> LangAPI::Expression {
        return LangAPI::RValue::createExpression(
            LangAPI::RValue{LangAPI::String{.value = std::move(str)}});
    }

    // {"Lexer", "SEMI"} -> Tokens::Lexer::SEMI (preserving qualified paths).
    auto tokenType(const AST::LLkToken& path) -> LangAPI::Type {
        LangAPI::Symbol symbol{kTokensRoot};
        for (const auto& part : path)
            symbol.path.emplace_back(part);
        symbol.path.emplace_back("id");
        return LangAPI::Type{std::move(symbol)};
    }

    // Keep recovery token sets semantic in the shared IR. Backends choose
    // their own representation for the set and its EOF marker.
    auto setType(const TokenSet& set) -> LangAPI::Type {
        auto type = LangAPI::Type{LangAPI::IspaLibSymbol{.exports = LangAPI::StdlibExports::RecoveryTokenSet}};
        for (const auto& path : set.tokens)
            type.getIspaLibSymbol().template_parameters.emplace_back(std::make_shared<LangAPI::Type>(tokenType(path)));
        if (set.end)
            type.getIspaLibSymbol().template_parameters.emplace_back(std::make_shared<LangAPI::Type>(LangAPI::Type{LangAPI::IspaLibSymbol{
                .exports = LangAPI::StdlibExports::RecoveryEndOfInput}}));
        return type;
    }

    // A target-neutral recovery policy; each backend renders its own form.
    auto policyType(const RecoverySpec& spec) -> LangAPI::Type {
        auto type = LangAPI::Type{LangAPI::IspaLibSymbol{.exports = LangAPI::StdlibExports::RecoveryPolicy}};
        for (const bool flag : {
                spec.allow_insertion, spec.allow_deletion, spec.allow_replacement}) {
            type.getIspaLibSymbol().template_parameters.emplace_back(
                std::make_shared<LangAPI::RValue>(LangAPI::RValue{LangAPI::Bool{.value = flag}}));
        }
        type.getIspaLibSymbol().template_parameters.emplace_back(std::make_shared<LangAPI::RValue>(LangAPI::RValue{
            LangAPI::Int{.value = static_cast<long long>(spec.max_skip)}}));
        return type;
    }

    auto pathText(const AST::LLkToken& path) -> std::string {
        return corelib::text::join(path, "::");
    }

    // ASSUMPTION: EOF is represented as an empty path or a special $/EOF
    // suffix, not as a distinct user-defined terminal called EOF.
    bool isEndMarker(const AST::LLkToken& path) {
        return path.empty() || path.back() == "$" || path.back() == "EOF";
    }

    // FIRST(1)/FOLLOW(1) sets are sequences of qualified token paths.
    // FIRST epsilon is omitted. For FOLLOW/continuation, empty sequences are
    // provisionally treated as EOF per the existing Tree representation.
    void addLookahead(const AST::LLkSet& sequences,
                      TokenSet& output,
                      std::string_view purpose,
                      bool allow_end = false) {
        for (const auto& sequence : sequences) {
            if (sequence.empty()) {
                if (allow_end)
                    output.end = true;
                continue;
            }
            const auto& path = sequence.front();
            if (isEndMarker(path)) {
                if (allow_end)
                    output.end = true;
                else if (!path.empty())
                    throw std::invalid_argument(
                        "ISPA recovery: EOF is not valid in " + std::string(purpose));
                continue;
            }
            if (!corelib::text::isUpper(path.back())) {
                throw std::invalid_argument(
                    "ISPA recovery: " + pathText(path) +
                    " is not a terminal (in " + std::string(purpose) + ")");
            }
            output.tokens.insert(path);
        }
    }

    void addToken(const AST::LLkToken& path,
                  TokenSet& output,
                  std::string_view purpose,
                  bool allow_end) {
        if (isEndMarker(path)) {
            if (allow_end)
                output.end = true;
            else
                throw std::invalid_argument(
                    "ISPA recovery: EOF is not valid in " + std::string(purpose));
            return;
        }
        AST::LLkSet one;
        one.insert(AST::LLkSeq{path});
        addLookahead(one, output, purpose, allow_end);
    }

    bool isDirectTerminal(const AST::RuleMember& member) {
        return member.isName() && !member.getName().name.empty() &&
               member.getName().isTerminal();
    }

    // Recovery anchors are not lookahead: a useful terminal can occur after
    // several required members, or inside a referenced production. Walk the
    // complete reachable grammar, stopping cycles rather than limiting depth.
    void collectRecoveryTerminals(AST::Tree& tree, const AST::RuleMember& member,
                                  TokenSet& output,
                                  std::set<AST::LLkToken>& visited) {
        if (member.isName()) {
            const auto& name = member.getName().name;
            if (isDirectTerminal(member)) {
                // Whitespace is never a useful place to resume a construct.
                if (name != AST::LLkToken{"__WS"})
                    addToken(name, output, "recovery anchor", false);
            } else if (visited.insert(name).second && tree.contains(name)) {
                for (const auto& child : tree[name].rule_members)
                    if (child) collectRecoveryTerminals(tree, *child, output, visited);
            }
        } else if (member.isGroup()) {
            for (const auto& child : member.getGroup().values)
                if (child) collectRecoveryTerminals(tree, *child, output, visited);
        } else if (member.isOp()) {
            for (const auto& child : member.getOp().options)
                if (child) collectRecoveryTerminals(tree, *child, output, visited);
        }
    }

    auto makeRecoverySpec(AST::Tree& tree,
                          const AST::LLkToken& rule_name,
                          std::size_t member_index,
                          const RecoveryOptions& options) -> RecoverySpec {
        if (!tree.contains(rule_name))
            throw std::invalid_argument("ISPA recovery: rule not in AST::Tree");

        const auto& rule = tree[rule_name];
        if (member_index >= rule.rule_members.size() ||
            !rule.rule_members[member_index])
            throw std::out_of_range("ISPA recovery: bad member index");
        const auto& member = *rule.rule_members[member_index];

        RecoverySpec spec;
        spec.rule = pathText(rule_name);
        spec.allow_insertion = options.allow_insertion;
        spec.allow_deletion = options.allow_deletion;
        spec.allow_replacement = options.allow_replacement;
        spec.max_skip = options.max_skip;
        if (!spec.max_skip)
            throw std::invalid_argument("RecoveryOptions.max_skip must be > 0");
        if (spec.max_skip > static_cast<std::size_t>(std::numeric_limits<long long>::max()))
            throw std::out_of_range("RecoveryOptions.max_skip cannot fit LangAPI::Int");

        // Single-token repair for now, so use FIRST(1) and FOLLOW(1).
        const auto& first = tree.getFirstSet(1);
        const auto& follow = tree.getFollowSet(1);
        if (auto it = follow.find(rule_name); it != follow.end())
            addLookahead(it->second, spec.follow, "FOLLOW(rule)", true);

        if (isDirectTerminal(member)) {
            addToken(member.getName().name, spec.expected,
                     "required terminal", false);
            spec.expected_text = pathText(member.getName().name);
        } else if (member.isName()) {
            const auto& name = member.getName().name;
            auto it = first.find(name);
            if (it == first.end())
                throw std::invalid_argument(
                    "ISPA recovery: missing FIRST(1) for " + pathText(name));
            addLookahead(it->second, spec.expected, "FIRST(nonterminal)");
            spec.expected_text = pathText(name);
        } else {
            addLookahead(tree.getMemberFirstSet(member, 1),
                         spec.expected, "FIRST(member)");
            spec.expected_text = "<group>";
        }

        // The suffix, not just the next member, determines continuation.
        if (member_index + 1 < rule.rule_members.size()) {
            AST::RuleMember suffix;
            AST::RuleMemberGroup group;
            for (std::size_t i = member_index + 1;
                 i < rule.rule_members.size(); ++i) {
                if (!rule.rule_members[i])
                    throw std::invalid_argument("ISPA recovery: null suffix member");
                group.values.push_back(rule.rule_members[i]);
            }
            suffix.value = std::move(group);
            addLookahead(tree.getAlternativeLookahead(suffix, rule_name, 1),
                         spec.continuation, "FIRST(suffix)", true);
        } else {
            spec.continuation = spec.follow;
        }

        for (const auto& path : options.sync)
            addToken(path, spec.sync, "explicit sync", true);

        std::set<AST::LLkToken> visited;
        for (std::size_t i = member_index; i < rule.rule_members.size(); ++i)
            if (rule.rule_members[i])
                collectRecoveryTerminals(tree, *rule.rule_members[i], spec.sync, visited);

        // Only a committed, required *single terminal* may be synthesized.
        if (member.quantifier == '?' || member.quantifier == '*' ||
            !isDirectTerminal(member)) {
            spec.allow_insertion = false;
            spec.allow_replacement = false;
        }
        if (spec.continuation.tokens.empty() && !spec.continuation.end &&
            spec.follow.tokens.empty() && !spec.follow.end) {
            spec.allow_insertion = false;
            spec.allow_replacement = false;
        }
        return spec;
    }

    // ---------- Hierarchical (context) recovery: per-call-site claims ----------
    struct ClaimSpec {
        std::string name;
        TokenSet first;
    };
    struct SiteSpec {
        std::string rule;
        std::string expected;            // what the member at member_index is
        std::vector<ClaimSpec> claims;   // what this rule can still accept here
    };

    auto makeSiteSpec(AST::Tree& tree,
                      const AST::LLkToken& rule_name,
                      std::size_t member_index) -> SiteSpec {
        if (!tree.contains(rule_name))
            throw std::invalid_argument("ISPA recovery: rule not in AST::Tree");
        const auto& rule = tree[rule_name];
        if (member_index >= rule.rule_members.size() ||
            !rule.rule_members[member_index])
            throw std::out_of_range("ISPA recovery: bad member index");

        const auto& first = tree.getFirstSet(1);
        auto claimFor = [&](const AST::RuleMember& m) {
            ClaimSpec claim;
            if (isDirectTerminal(m)) {
                claim.name = pathText(m.getName().name);
                if (m.getName().name != AST::LLkToken{"__WS"})
                    addToken(m.getName().name, claim.first, "claim terminal", false);
            } else if (m.isName()) {
                const auto& name = m.getName().name;
                auto it = first.find(name);
                if (it == first.end())
                    throw std::invalid_argument(
                        "ISPA recovery: missing FIRST(1) for " + pathText(name));
                claim.name = pathText(name);
                std::set<AST::LLkToken> visited;
                collectRecoveryTerminals(tree, m, claim.first, visited);
            } else {
                claim.name = "<group>";
                std::set<AST::LLkToken> visited;
                collectRecoveryTerminals(tree, m, claim.first, visited);
            }
            return claim;
        };

        SiteSpec site;
        site.rule = pathText(rule_name);
        const auto& self = *rule.rule_members[member_index];
        auto selfClaim = claimFor(self);
        site.expected = selfClaim.name;
        // The current member can resume at an interior terminal too.
        site.claims.push_back(std::move(selfClaim));
        // Include the complete suffix, including nested required productions.
        // Tokens that end the rule belong to the parent frame's claims.
        for (std::size_t i = member_index + 1; i < rule.rule_members.size(); ++i) {
            if (!rule.rule_members[i])
                throw std::invalid_argument("ISPA recovery: null suffix member");
            const auto& m = *rule.rule_members[i];
            site.claims.push_back(claimFor(m));
        }
        return site;
    }
} // anonymous namespace

bool ParserBase::hasCustomFailure(const AST::RuleMember& member) {
    if (!member.failure_handler.name.empty())
        return true;
    return std::any_of(member.features.begin(), member.features.end(),
        [](const AST::FrontendFeature& f) {
            return std::holds_alternative<AST::FailBlock>(f) ||
                   std::holds_alternative<AST::FailBinding>(f) ||
                   std::holds_alternative<AST::FailMarker>(f);
        });
}

// Creates an *expression* returning a callable recovery handler.
// Call it on a committed failure with (pos, recovery_state).
auto ParserBase::recoveryHandler(AST::Tree& tree,
                                 const AST::LLkToken& rule,
                                 std::size_t member_index,
                                 const RecoveryOptions& options)
    -> LangAPI::Expression {
    const auto spec = makeRecoverySpec(tree, rule, member_index, options);

    LangAPI::FunctionCall call;
    call.name = std::make_shared<LangAPI::Symbol>(
        "ISPA_STD", "Recovery", "handler");
    for (const auto* set : {
            &spec.expected, &spec.continuation, &spec.follow, &spec.sync}) {
        call.template_parameters.emplace_back(
            std::make_shared<LangAPI::Type>(setType(*set)));
    }
    call.template_parameters.emplace_back(
        std::make_shared<LangAPI::Type>(policyType(spec)));
    call.args = {string_arg(spec.rule), string_arg(spec.expected_text)};
    return LangAPI::Expression{
        LangAPI::ExpressionValue{std::move(call)}
    };
}

// Expression: recovery_frame("rule")  -- emit once at the top of a rule
// function as `auto frame = <expr>;` (RAII guard; do not copy).
auto ParserBase::recoveryFrame(const AST::LLkToken& rule) -> LangAPI::Expression {
    LangAPI::FunctionCall call;
    call.name = std::make_shared<LangAPI::Symbol>("recovery_frame");
    call.args = {string_arg(pathText(rule))};
    return LangAPI::Expression{
        LangAPI::ExpressionValue{std::move(call)}
    };
}

// Expression: recovery_at<Set...>("expected", "claim0", "claim1", ...)
// Emit before each member (terminal, nonterminal or group) of the rule body.
auto ParserBase::recoverySite(AST::Tree& tree,
                              const AST::LLkToken& rule,
                              std::size_t member_index)
    -> LangAPI::Expression {
    const auto site = makeSiteSpec(tree, rule, member_index);
    LangAPI::FunctionCall call;
    call.name = std::make_shared<LangAPI::Symbol>("recovery_at");
    call.args.push_back(string_arg(site.expected));
    for (const auto& claim : site.claims) {
        call.template_parameters.emplace_back(
            std::make_shared<LangAPI::Type>(setType(claim.first)));
        call.args.push_back(string_arg(claim.name));
    }
    return LangAPI::Expression{
        LangAPI::ExpressionValue{std::move(call)}
    };
}

auto ParserBase::recoveryChoice(AST::Tree& tree, const AST::LLkToken& rule,
                                const AST::RuleMember& choice) -> LangAPI::Expression {
    LangAPI::FunctionCall call;
    call.name = std::make_shared<LangAPI::Symbol>("recover_choice");
    std::string expected;
    for (const auto& alternative : choice.getOp().options) {
        TokenSet first, reachable;
        addLookahead(tree.getMemberFirstSet(*alternative, 1), first, "choice FIRST");
        std::set<AST::LLkToken> visited;
        collectRecoveryTerminals(tree, *alternative, reachable, visited);
        call.template_parameters.emplace_back(std::make_shared<LangAPI::Type>(setType(first)));
        call.template_parameters.emplace_back(std::make_shared<LangAPI::Type>(setType(reachable)));
        for (const auto& token : first.tokens) {
            if (!expected.empty()) expected += ", ";
            expected += pathText(token);
        }
    }
    call.args = {LangAPI::Symbol::createExpression(LangAPI::Symbol{"pos"}),
                 string_arg(pathText(rule)), string_arg(expected)};
    return LangAPI::FunctionCall::createExpression(std::move(call));
}
} // namespace Parser

namespace Parser {
auto ParserBase::dataType(const AST::DataType& type) -> LangAPI::Type {
    using Kind = AST::DataType::Kind;
    LangAPI::ValueType value;
    switch (type.kind) {
        case Kind::Character: value = LangAPI::ValueType::Char; break;
        case Kind::String: value = LangAPI::ValueType::String; break;
        case Kind::Token: value = LangAPI::ValueType::Token; break;
        case Kind::Rule: value = LangAPI::ValueType::Rule; break;
        case Kind::Tuple: value = LangAPI::ValueType::Tuple; break;
        case Kind::Array: value = LangAPI::ValueType::Array; break;
        case Kind::Variant: value = LangAPI::ValueType::Variant; break;
        default: throw std::invalid_argument("Unresolved AST data type");
    }
    LangAPI::Type result{value};
    if (!type.name.empty()) {
        LangAPI::Type node_type{LangAPI::Symbol{type.name}};
        for (std::size_t i = 0; i < type.templateArguments.size(); ++i) {
            auto path = type.templateArguments[i];
            node_type.template_parameters.emplace_back(LangAPI::Type{LangAPI::Symbol{path}});
        }
        result.template_parameters.emplace_back(std::move(node_type));
    }
    for (const auto& parameter : type.parameters)
        result.template_parameters.emplace_back(dataType(parameter));
    return result;
}
auto ParserBase::deduceVarTypeByRuleMemberBase(const AST::RuleMember& member) -> LangAPI::Type {
    auto base = member;
    base.quantifier = '\0';
    return dataType(AST::captureType(base));
}
auto ParserBase::deduceVarTypeByRuleMember(const AST::RuleMember& member) -> LangAPI::Type {
    return dataType(AST::captureType(member));
}
auto ParserBase::deduceUvarType(const LangAPI::Variable& var, const LangAPI::Variable& shadow) -> LangAPI::Type {
    if (var.type == LangAPI::ValueType::TokenResult || var.type == LangAPI::ValueType::RuleResult)
        return {var.type == LangAPI::ValueType::TokenResult ? LangAPI::ValueType::Token : LangAPI::ValueType::Rule,
                var.type.template_parameters};
    return shadow.name.empty() ? var.type : shadow.type;
}
}
