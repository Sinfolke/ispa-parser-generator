export module AST.Tree;
import AST.types;
import AST.API;
import corelib;
import Parser;
import hash;
import cpuf.printf;
import dstd;
import std;
import LangAPI;
export namespace AST {
    /*
     * Tree class that holds features could be done on it. Contains tree_map map
     */
    class Tree {
        TreeMap tree_map;
        SpacemodeStates spacemode = SpacemodeStates::MIXED;
        Use use;
        std::string name;
        InitialItemSet initial_item_set;
        UsePlaceTable use_places;
        NullableMap nullable;
        First first;
        Follow follow;
        NameToIndexMap name_to_index;

        static auto compute_group_length(const stdu::vector<std::shared_ptr<AST::RuleMember>> &group) -> std::size_t;
        void getUsePlacesTable(const stdu::vector<std::shared_ptr<AST::RuleMember>> &members, const stdu::vector<std::string> &name);
        void transform_helper(
            stdu::vector<std::shared_ptr<AST::RuleMember>> &members,
            const stdu::vector<std::string> &fullname,
            const stdu::vector<std::string> &original_fullname,
            utype::unordered_map<stdu::vector<std::string>, std::pair<char, stdu::vector<std::string>>> &replacements
        );
        void transform();
        void createInitialItemSet();
        auto createUsePlacesTable() -> UsePlaceTable&;
        bool isMemberNullable(const AST::RuleMember& member) const;
        void computeNullableSet();
        void constructFirstSet(const stdu::vector<AST::Rule>& options, const stdu::vector<std::string> &nonterminal, bool &changed);
        void constructFirstSet();
        // Helper function to collect the FIRST set of an arbitrary RuleMember (Name or Group)
        void collectMemberFirst(const AST::RuleMember& member, std::set<stdu::vector<std::string>>& outFirst);
        // Tailored recursive function to handle updating FOLLOW relations inside groups/sequences cleanly
        void processFollowForSequence(
            const stdu::vector<std::string>& lhs_name,
            const stdu::vector<std::shared_ptr<AST::RuleMember>>& members,
            bool is_left_recursive,
            bool& hasChanges,
            stdu::vector<stdu::vector<std::string>>& prev_depend
        );
        void constructFollowSet();
        void formatFirstOrFollowSet(std::ostringstream &oss, AST::First &set);
    public:
        auto getTreeMap() const -> const TreeMap& { return tree_map; };
        auto getUse() const -> const Use& { return use; };
        auto getSpacemode() const -> const SpacemodeStates& { return spacemode; };
        auto getName() const -> const std::string& { return name; }
        auto getTreeMap() ->  TreeMap& { return tree_map; };
        auto getUse() ->  Use& { return use; };
        auto getSpacemode() -> SpacemodeStates& { return spacemode; };
        auto getName() -> std::string& { return name; }
        auto begin() const { return tree_map.begin(); };
        auto end() const { return tree_map.end(); };
        Tree(std::string&& name, SpacemodeStates&& spacemode, Use&& use, TreeMap&& map)
            : name(std::move(name)),
              spacemode(std::move(spacemode)),
              use(std::move(use)),
              tree_map(std::move(map)) {}
        Tree(std::string& name, SpacemodeStates& spacemode, Use& use, TreeMap& map)
            : name(name),
              spacemode(spacemode),
              use(use),
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