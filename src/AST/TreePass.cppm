export module AST.Pass;
import corelib;
import LLIR.IR;
import AST.Tree;
import AST.API;
import dstd;
import std;

export namespace AST {
    class TreePass {
    public:
        // struct Conflict {
        //     stdu::vector<Parser::Rule>* lhs_rule;
        //     stdu::vector<Parser::Rule>* rhs_rule;
        //     stdu::vector<Parser::Rule>::iterator lhs_it;
        //     stdu::vector<Parser::Rule>::iterator rhs_it;
        // };
        // using ConflictsList = stdu::vector<Conflict>;

    private:
        Tree *ast;
        std::size_t token_count = 0;
        void literalsToToken(
            stdu::vector<std::shared_ptr<AST::RuleMember>> &literals,
            std::size_t &count,
            stdu::vector<std::pair<stdu::vector<std::string>, AST::Rule>> &toInsert,
            stdu::vector<std::pair<std::shared_ptr<AST::RuleMember>, std::shared_ptr<AST::RuleMember>>> &generated
        );

        static auto inlineGroup(AST::RuleMemberGroup group, stdu::vector<std::shared_ptr<AST::RuleMember>> &members, stdu::vector<std::shared_ptr<AST::RuleMember>>::iterator toInsert)
             -> stdu::vector<std::shared_ptr<AST::RuleMember>>::iterator;
        enum class Types {
            string, Rule_escaped, Rule_csequence, Rule_bin, Rule_hex, Rule_any, cll, name, group, nospace, op, empty
        };
        static Types getTypes(const AST::String&) { return Types::string; }
        static Types getTypes(const AST::RuleMemberEscaped &) { return Types::Rule_escaped; }
        static Types getTypes(const AST::RuleMemberCsequence &) { return Types::Rule_csequence; }
        static Types getTypes(const AST::RuleMemberBin&) { return Types::Rule_bin; }
        static Types getTypes(const AST::RuleMemberHex&) { return Types::Rule_hex; }
        static Types getTypes(const AST::RuleMemberAny&) { return Types::Rule_any; }
        static Types getTypes(const AST::Cll&) { return Types::cll; }
        // never meet types
        static Types getTypes(const AST::RuleMemberName&) { return Types::name; };
        static Types getTypes(const AST::RuleMemberGroup&) { return Types::group; }
        static Types getTypes(const AST::RuleMemberNospace&) {return Types::nospace; }
        static Types getTypes(const AST::RuleMemberOp&) { return Types::op; }
        static Types getTypes(const std::monostate&) { return Types::empty; }
    public:
        explicit TreePass(Tree &ast) : ast(&ast) {}
        // sort by priority functions
        static bool prioritySort(const AST::Tree& ast, const AST::String &first, const AST::String &second);
        static bool prioritySort(const AST::Tree& ast, const AST::RuleMemberBin &first, const AST::RuleMemberBin &second);
        static bool prioritySort(const AST::Tree& ast, const AST::RuleMemberHex &first, const AST::RuleMemberHex &second);
        static bool prioritySort(const AST::Tree& ast, const AST::RuleMemberName &first, const AST::RuleMemberName &second);
        static bool prioritySort(const AST::Tree& ast, const AST::RuleMemberCsequence &first, const AST::RuleMemberCsequence &second);
        static bool prioritySort(const AST::Tree& ast, const AST::RuleMemberGroup &first, const AST::RuleMemberGroup &second);
        static bool prioritySort(const AST::Tree& ast, const AST::RuleMemberOp &first, const AST::RuleMemberOp &second);
        static bool prioritySort(const AST::Tree& ast, const AST::RuleMember &first, const AST::RuleMember &second);

        static void removeEmptyRule(Tree &ast);
        static void inlineSingleGroups(Tree &ast);
        static void literalsToToken(Tree &ast);
        static void sortByPriority(Tree &ast);
        static void addSpaceToken(Tree &ast);
        static void sortByPriority(Tree &ast, AST::RuleMemberOp& options);
        static void sortByPriority(Tree &ast, stdu::vector<std::shared_ptr<AST::RuleMember>>& members);
        void removeEmptyRule();
        void inlineSingleGroups();
        void literalsToToken();
        void sortByPriority();
        void addSpaceToken();
        void work() {
            removeEmptyRule();
            inlineSingleGroups();
            sortByPriority();
            literalsToToken();
            addSpaceToken();
        }
    };
}