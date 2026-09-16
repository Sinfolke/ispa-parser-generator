export module NFA.IR;
import NFA.IR.API;
import AST.Tree;
import AST.API;
import hash;
import cpuf.op;
import dstd;
import std;
using namespace NFA::InitialAPI;
export namespace NFA {
    class InitialNFA {
        AST::Tree *tree;
        utype::unordered_map<stdu::vector<std::string>, Token> tokens;
        stdu::vector<std::unique_ptr<AST::RuleMember>> synthesized_members;

        AST::RuleMember *synthesize(const AST::RuleMember &base,
                                     std::optional<AST::RulePrefix> prefix_override,
                                     std::optional<char> quantifier_override);
        stdu::vector<AST::RuleMember *> resolveTransparent(AST::RuleMember &member);
        struct Fragment {
            stdu::vector<TransitionValue> entries;
            stdu::vector<TokenID> tails;
        };

        Fragment expandMember(AST::RuleMember &member,
                                                       const stdu::vector<std::string> &token_name,
                                                       std::size_t position_in_token,
                                                       std::size_t &site_counter,
                                                       Token &token,
                                                       const stdu::vector<TransitionValue> &next);
    public:
        InitialNFA(AST::Tree &tree) : tree(&tree) {}
        auto &get() { return tokens; }
        auto &get() const { return tokens; }
        void build(bool addStoreActions = true);
        void unrollGroups();
        void unrollQuantifiers();
        void unrollNestedTokens();
    };
}
export auto operator<<(std::ostream &os, NFA::InitialNFA &infa) -> std::ostream& {
    for (const auto &token : infa.get()) {
        os << token.second << "\n";
    }
    return os;
}