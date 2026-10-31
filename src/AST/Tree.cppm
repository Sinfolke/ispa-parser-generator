export module AST.Tree;
import AST.types;
import AST.API;
import corelib;
import hash;
import cpuf.printf;
import cpuf.op;
import dstd;
import std;
import LangAPI;
export namespace AST {
    /*
     * Tree class that holds features could be done on it. Contains tree_map map
     */
    using LLkToken = stdu::vector<std::string>;
    using LLkSeq = std::vector<LLkToken>;
    using LLkSet = std::set<LLkSeq>;
    using LLkTable = std::map<LLkToken, LLkSet>;
    class Tree {
        TreeMap tree_map;
        stdu::vector<FailBlock> fail_blocks;
        LangAPI::Declarations python_declarations;
        SpacemodeStates spacemode = SpacemodeStates::MIXED;
        std::string name;
        InitialItemSet initial_item_set;
        UsePlaceTable use_places;
        NullableMap nullable;
        First first;
        Follow follow;
        std::map<std::size_t, LLkTable> first_k_cache;
        std::map<std::size_t, LLkTable> follow_k_cache;
        NameToIndexMap name_to_index;
        // Rules removed by TreePass::autoInline: alias -> rule that now stands for it. Every reference in
        // the tree was already rewritten; the alias survives only as a type name (ConstructTypes) and as an
        // export (External::exports), both of which must keep resolving.
        std::map<stdu::vector<std::string>, stdu::vector<std::string>> aliases;

        static auto compute_group_length(const stdu::vector<std::shared_ptr<AST::RuleMember>> &group) -> std::size_t;
        void getUsePlacesTable(const stdu::vector<std::shared_ptr<AST::RuleMember>> &members, const stdu::vector<std::string> &name);
        void transform_helper(
            stdu::vector<std::shared_ptr<AST::RuleMember>>& members,
            const stdu::vector<std::string>& fullname,
            const stdu::vector<std::string>& original_fullname,
            bool whole_rule = false
        );
        void transform();
        void createInitialItemSet();
        auto createUsePlacesTable() -> UsePlaceTable&;
        bool isMemberNullable(const AST::RuleMember& member) const;
        void computeNullableSet();
        bool processMemberFirst(
            const AST::RuleMember& member,
            const stdu::vector<std::string>& nonterminal,
            bool& changed
        );
        void constructFirstSet(const stdu::vector<AST::Rule>& options, const stdu::vector<std::string> &nonterminal, bool &changed);
        void constructFirstSet();
        // Helper function to collect the FIRST set of an arbitrary RuleMember (Name or Group)
        void collectMemberFirst(const AST::RuleMember& member, std::set<stdu::vector<std::string>>& outFirst);
        // Tailored recursive function to handle updating FOLLOW relations inside groups/sequences cleanly
        void processFollowForSequence(
            const stdu::vector<std::string>& lhs_name,
            const stdu::vector<std::shared_ptr<AST::RuleMember>>& members,
            const stdu::vector<std::shared_ptr<AST::RuleMember>>& trailing,
            bool is_left_recursive,
            bool& hasChanges,
            stdu::vector<stdu::vector<std::string>>& prev_depend
        );
        void constructFollowSet();
        void formatFirstOrFollowSet(std::ostringstream &oss, AST::First &set);
        auto memberBaseFirstK(const AST::RuleMember&, std::size_t,
            const LLkTable&) const -> LLkSet;
        auto memberFirstK(const AST::RuleMember&, std::size_t,
            const LLkTable&) const -> LLkSet;
        auto sequenceFirstK(
            const stdu::vector<std::shared_ptr<AST::RuleMember>>&,
            std::size_t, const LLkTable&) const -> LLkSet;
        void propagateFollowSequenceK(
            const stdu::vector<std::shared_ptr<AST::RuleMember>>&,
            const LLkSet&, std::size_t, const LLkTable&, LLkTable&,
            bool&) const;
    public:

        auto getFailBlocks() const -> const stdu::vector<FailBlock>& { return fail_blocks; }
        auto getFailBlocks() -> stdu::vector<FailBlock>& { return fail_blocks; }
        auto getPythonDeclarations() const -> const LangAPI::Declarations& { return python_declarations; }
        auto getPythonDeclarations() -> LangAPI::Declarations& { return python_declarations; }
        void constructFirstSet(std::size_t k);
        void constructFollowSet(std::size_t k);
        auto getFirstSet(std::size_t k) -> const LLkTable&;
        // FIRST of a member, retaining epsilon and without appending FOLLOW.
        auto getMemberFirstSet(const AST::RuleMember& member, std::size_t k) -> LLkSet;
        auto getFollowSet(std::size_t k) -> const LLkTable&;
        auto getAlternativeLookahead(const AST::RuleMember& alt, const stdu::vector<std::string>& ruleName, std::size_t k) -> LLkSet;
        auto getAliases() const -> const std::map<stdu::vector<std::string>, stdu::vector<std::string>>& { return aliases; }
        void addAlias(stdu::vector<std::string> alias, stdu::vector<std::string> target) {
            aliases[std::move(alias)] = std::move(target);
        }
        // The rule that implements `name`: follows auto-inline aliases, returns `name` itself otherwise.
        auto resolveAlias(const stdu::vector<std::string>& name) const -> stdu::vector<std::string> {
            auto current = name;
            for (std::size_t hops = 0; hops <= aliases.size(); ++hops) {
                const auto found = aliases.find(current);
                if (found == aliases.end()) return current;
                current = found->second;
            }
            return current;
        }
        auto getTreeMap() const -> const TreeMap& { return tree_map; };
        auto getSpacemode() const -> const SpacemodeStates& { return spacemode; };
        auto getName() const -> const std::string& { return name; }
        auto getTreeMap() ->  TreeMap& { return tree_map; };
        auto getSpacemode() -> SpacemodeStates& { return spacemode; };
        auto getName() -> std::string& { return name; }
        auto begin() const { return tree_map.begin(); };
        auto end() const { return tree_map.end(); };
        Tree(std::string&& name, SpacemodeStates&& spacemode, TreeMap&& map)
            : name(std::move(name)),
              spacemode(std::move(spacemode)),
              tree_map(std::move(map)) {}
        Tree(std::string& name, SpacemodeStates& spacemode, TreeMap& map)
            : name(name),
              spacemode(spacemode),
              tree_map(map) {}
        ~Tree() = default;
        auto getUsePlacesTable() -> UsePlaceTable& {
            if (use_places.empty())
                createUsePlacesTable();
            return use_places;
        };
        auto getFirstSet()-> First& {
            if (first.empty())
                constructFirstSet();
            return first;
        };
        auto getFollowSet() -> Follow& {
            if (follow.empty())
                constructFollowSet();
            return follow;
        };
        auto getRawFirstSet()-> First& { return first; };
        auto getRawFollowSet() -> Follow& { return follow; };
        auto getCodeForLexer() -> std::pair<LangAPI::Statements, LangAPI::Variable>;
        auto getInitialItemSet() -> InitialItemSet&;
        auto getTerminals() const -> stdu::vector<stdu::vector<std::string>>;
        auto getNonTerminals() const -> stdu::vector<stdu::vector<std::string>>;
        auto generateRandomTokenInputs(std::size_t maxDepth) -> utype::unordered_map<stdu::vector<std::string>, stdu::vector<std::string>>;
        void printFirstSet(const std::string &fileName);
        void printFollowSet(const std::string &fileName);
        void buildNameToIndexMap();
        auto getNameToIndexMap();
        auto getNameToIndexMap() const;
        auto operator[](const stdu::vector<std::string> &name) const -> const Rule& {
            return tree_map.at(name);
        }
        auto contains(const stdu::vector<std::string> &name) const -> bool {
            return tree_map.contains(name);
        }
    };
};
