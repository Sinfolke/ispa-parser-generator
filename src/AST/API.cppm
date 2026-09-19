export module AST.API;

import logging;
import corelib;
import hash;
import dstd;
import std;

export namespace AST {
    // Forward declarations
    struct CllType;
    struct CllFunctionArguments;
    struct CllFunctionParameters;
    struct CllExpr;
    struct CllExprGroup;
    struct CllExprCompare;
    struct CllExprAddition;
    struct CllExprTerm;
    struct CllExprValue;
    struct CllExprLogical;
    struct CllExprLogicalPart;
    struct CllVariableMention;
    struct rvalue;
    struct RuleMember;

    using CllFunctionBodyCall = CllFunctionArguments;
    using CllFunctionBodyDecl = CllFunctionParameters;

    struct String {
        std::string value;

        static std::size_t count_strlen(const std::string &str);
        static std::string format_str(std::string str);

        std::size_t count_strlen() const;
        std::string format_str() const;

    private:
        friend struct ::uhash;

        auto members() const {
            return std::tie(value);
        }
    };

    struct Number {
        char sign = '\0';
        std::string main;
        std::string dec;

        std::string getFull() const;
        bool hasDec() const;
        bool hasSign() const;
        double getFullNumber() const;
        unsigned getMain() const;
        unsigned getDecimal() const;

    private:
        friend struct ::uhash;

        auto members() const {
            return std::tie(sign, main, dec);
        }
    };

    struct Boolean {
        std::string value;

        bool getBoolean() const;

    private:
        friend struct ::uhash;

        auto members() const {
            return std::tie(value);
        }
    };

    struct Array {
        // Recursive CllExpr is indirect because CllExpr itself eventually
        // contains structures that can contain CllFunctionArguments.
        stdu::vector<std::shared_ptr<CllExpr>> value;

    private:
        friend struct ::uhash;

        auto members() const {
            return std::tie(value);
        }
    };

    struct Object {
        // Same recursive AST relationship as Array.
        std::unordered_map<std::string, std::shared_ptr<CllExpr>> value;

    private:
        friend struct ::uhash;

        auto members() const {
            return std::tie(value);
        }
    };

    struct At : EmptyHashable {};

    struct ID {
        std::string value;

    private:
        friend struct ::uhash;

        auto members() const {
            return std::tie(value);
        }
    };

    struct rvalue {
        std::variant<String, Number, Boolean, Array, Object, At, ID> value;

        bool isString() const;
        bool isNumber() const;
        bool isBoolean() const;
        bool isArray() const;
        bool isObject() const;
        bool isAt() const;
        bool isID() const;

        String &getString();
        Number &getNumber();
        Boolean &getBoolean();
        Array &getArray();
        Object &getObject();
        ID &getID();

        const String &getString() const;
        const Number &getNumber() const;
        const Boolean &getBoolean() const;
        const Array &getArray() const;
        const Object &getObject() const;
        const ID &getID() const;

    private:
        auto members() const {
            return std::tie(value);
        }

        friend struct ::uhash;
    };

    struct CllCompareOp {
        std::string op;

    private:
        auto members() const {
            return std::tie(op);
        }

        friend struct ::uhash;
    };

    struct CllLogicalOp {
        bool isAnd;

    private:
        auto members() const {
            return std::tie(isAnd);
        }

        friend struct ::uhash;
    };

    struct CllType {
        std::string type;

        // Self-recursive type is fine here.
        stdu::vector<CllType> templ;

    private:
        auto members() const {
            return std::tie(type, templ);
        }

        friend struct ::uhash;
    };

    struct CllFunctionArguments {
        // CllExpr is incomplete here and is also mutually recursive with
        // CllFunctionArguments, so an owning pointer breaks the cycle.
        stdu::vector<std::shared_ptr<CllExpr>> expr;

    private:
        auto members() const {
            return std::tie(expr);
        }

        friend struct ::uhash;
    };

    struct CllFunctionParameters {
        stdu::vector<std::string> names;

    private:
        auto members() const {
            return std::tie(names);
        }

        friend struct ::uhash;
    };

    struct CllFunctionCall {
        std::string name;
        CllFunctionBodyCall body;

    private:
        auto members() const {
            return std::tie(name, body);
        }

        friend struct ::uhash;
    };

    struct CllFunctionDecl {
        std::string name;
        CllFunctionBodyDecl body;

    private:
        auto members() const {
            return std::tie(name, body);
        }

        friend struct ::uhash;
    };

    struct CllMethodCall {
        std::string name;
        CllFunctionCall body;

    private:
        auto members() const {
            return std::tie(name, body);
        }

        friend struct ::uhash;
    };

    struct CllExprValue {
        std::variant<
            std::shared_ptr<CllExprGroup>,
            std::shared_ptr<CllVariableMention>,
            CllFunctionCall,
            CllMethodCall,
            rvalue
        > value;

        bool isGroup() const;
        bool isVariable() const;
        bool isFunctionCall() const;
        bool isMethodCall() const;
        bool isrvalue() const;

        CllExprGroup &getGroup();
        CllVariableMention &getVariable();
        CllFunctionCall &getFunctionCall();
        CllMethodCall &getMethodCall();
        rvalue &getrvalue();

        const CllExprGroup &getGroup() const;
        const CllVariableMention &getVariable() const;
        const CllFunctionCall &getFunctionCall() const;
        const CllMethodCall &getMethodCall() const;
        const rvalue &getrvalue() const;

    private:
        auto members() const {
            return std::tie(value);
        }

        friend struct ::uhash;
    };

    struct CllExprTerm {
        CllExprValue value;
        stdu::vector<std::pair<char, CllExprValue>> rights;

    private:
        auto members() const {
            return std::tie(value, rights);
        }

        friend struct ::uhash;
    };

    struct CllExprAddition {
        CllExprTerm value;
        stdu::vector<std::pair<char, CllExprTerm>> rights;

    private:
        auto members() const {
            return std::tie(value, rights);
        }

        friend struct ::uhash;
    };

    struct CllExprCompare {
        CllExprAddition value;
        stdu::vector<std::pair<CllCompareOp, CllExprAddition>> rights;

    private:
        auto members() const {
            return std::tie(value, rights);
        }

        friend struct ::uhash;
    };

    struct CllExprLogical {
        CllExprCompare value;
        stdu::vector<std::pair<CllLogicalOp, CllExprCompare>> rights;

    private:
        auto members() const {
            return std::tie(value);
        }

        friend struct ::uhash;
    };

    struct CllExpr {
        CllExprLogical value;

    private:
        auto members() const {
            return std::tie(value);
        }

        friend struct ::uhash;
    };

    struct CllExprGroup {
        CllExpr expr;

    private:
        auto members() const {
            return std::tie(expr);
        }

        friend struct ::uhash;
    };

    struct CllVariableMention {
        std::optional<char> pre_increament;
        std::optional<char> post_increament;
        std::string name;
        std::optional<CllExpr> braceExpression;

    private:
        auto members() const {
            return std::tie(
                pre_increament,
                post_increament,
                name,
                braceExpression
            );
        }

        friend struct ::uhash;
    };

    struct CllIf {
        CllExpr expr;

        // RuleMember is incomplete at this point and RuleMember itself
        // contains Cll, creating a mutual recursion.
        stdu::vector<std::shared_ptr<RuleMember>> stmt;

    private:
        auto members() const {
            return std::tie(expr, stmt);
        }

        friend struct ::uhash;
    };

    struct CllVar {
        CllType type;
        std::string name;
        char op = '\0';
        std::optional<CllExpr> value;

    private:
        auto members() const {
            return std::tie(type, name, op, value);
        }

        friend struct ::uhash;
    };

    struct CllLoopWhile {
        CllExpr expr;

        // Same RuleMember recursion as CllIf.
        stdu::vector<std::shared_ptr<RuleMember>> stmt;

    private:
        auto members() const {
            return std::tie(expr, stmt);
        }

        friend struct ::uhash;
    };

    struct CllLoopFor : EmptyHashable {};

    struct Cll {
        std::variant<CllVar, CllIf, CllExpr> value;

        bool isVar() const;
        bool isIf() const;
        bool isExpr() const;

        CllVar &getVar();
        CllIf &getIf();
        CllExpr &getExpr();

        const CllVar &getVar() const;
        const CllIf &getIf() const;
        const CllExpr &getExpr() const;

    private:
        auto members() const {
            return std::tie(value);
        }

        friend struct ::uhash;
    };

    struct RulePrefix {
        bool is_key_value = false;
        std::string name;

        void clear();
        bool empty() const;

    private:
        auto members() const {
            return std::tie(is_key_value, name);
        }

        friend struct ::uhash;
    };

    struct RuleMemberName {
        stdu::vector<std::string> name;
        bool isAutoGenerated = false;

        [[nodiscard]]
        bool isTerminal() const {
            return corelib::text::isUpper(name.back());
        }

        [[nodiscard]]
        bool isNonterminal() const {
            return corelib::text::isLower(name.back());
        }

    private:
        auto members() const {
            return std::tie(name, isAutoGenerated);
        }

        friend struct ::uhash;
    };

    struct RuleMemberGroup {
        // RuleMember is mutually recursive with Cll -> CllIf/CllLoopWhile.
        stdu::vector<std::shared_ptr<RuleMember>> values;

    private:
        auto members() const {
            return std::tie(values);
        }

        friend struct ::uhash;
    };

    struct RuleMemberOp {
        // Same recursive relationship as RuleMemberGroup.
        stdu::vector<std::shared_ptr<RuleMember>> options;

    private:
        auto members() const {
            return std::tie(options);
        }

        friend struct ::uhash;
    };

    struct RuleMemberCsequence {
        bool negative = false;
        stdu::vector<char> characters;
        stdu::vector<char> escaped;
        stdu::vector<std::pair<char, char>> diapasons;

    private:
        auto members() const {
            return std::tie(
                negative,
                characters,
                escaped,
                diapasons
            );
        }

        friend struct ::uhash;
    };

    struct RuleMemberAny : EmptyHashable {};

    struct RuleMemberNospace : EmptyHashable {};

    struct RuleMemberEscaped {
        char c;

    private:
        auto members() const {
            return std::tie(c);
        }

        friend struct ::uhash;
    };

    // These structures are hashable by begin and end methods.
    struct RuleMemberHex {
        std::string hex_chars;

        std::string::iterator begin();
        std::string::iterator end();

        std::string::const_iterator begin() const;
        std::string::const_iterator end() const;
    };

    struct RuleMemberBin {
        std::string bin_chars;

        std::string::iterator begin();
        std::string::iterator end();

        std::string::const_iterator begin() const;
        std::string::const_iterator end() const;
    };

    struct RuleMember {
        RulePrefix prefix;
        char quantifier = '\0';
        bool isAutoGenerated = false;
        bool isInline = false;
        std::variant<
            std::monostate,
            String,
            RuleMemberName,
            RuleMemberGroup,
            RuleMemberOp,
            RuleMemberCsequence,
            RuleMemberAny,
            RuleMemberNospace,
            RuleMemberEscaped,
            RuleMemberHex,
            RuleMemberBin,
            Cll
        > value;

        bool isString() const;
        bool isName() const;
        bool isGroup() const;
        bool isOp() const;
        bool isCsequence() const;
        bool isAny() const;
        bool isNospace() const;
        bool isEscaped() const;
        bool isHex() const;
        bool isBin() const;
        bool isCll() const;
        bool emptyQuantifier() const;
        bool empty() const;

        // Non-const overloads
        String &getString();
        RuleMemberName &getName();
        RuleMemberGroup &getGroup();
        RuleMemberOp &getOp();
        RuleMemberCsequence &getCsequence();
        RuleMemberEscaped &getEscaped();
        RuleMemberHex &getHex();
        RuleMemberBin &getBin();
        Cll &getCll();

        // Const overloads
        const String &getString() const;
        const RuleMemberName &getName() const;
        const RuleMemberGroup &getGroup() const;
        const RuleMemberOp &getOp() const;
        const RuleMemberCsequence &getCsequence() const;
        const RuleMemberEscaped &getEscaped() const;
        const RuleMemberHex &getHex() const;
        const RuleMemberBin &getBin() const;
        const Cll &getCll() const;

        bool fullCompare(const RuleMember &second);

    private:
        auto members() const {
            return std::tie(value);
        }

        friend struct ::uhash;
    };

    struct RuleMemberKey {
        RuleMember base;

    private:
        auto members() const {
            return std::tie(base);
        }

        friend struct ::uhash;
    };

    struct RegularDataBlockWKeys {
        std::unordered_map<std::string, CllExpr> value;

        bool empty() const;

        std::unordered_map<std::string, CllExpr>::iterator begin();
        std::unordered_map<std::string, CllExpr>::iterator end();

        std::unordered_map<std::string, CllExpr>::const_iterator begin() const;
        std::unordered_map<std::string, CllExpr>::const_iterator end() const;

        CllExpr &operator[](const std::string &name);

    private:
        auto members() const {
            return std::tie(value);
        }

        friend struct ::uhash;
    };

    using RegularDataBlock = CllExpr;

    struct TemplatedDataBlock {
        stdu::vector<std::string> names;

        bool empty() const;

        stdu::vector<std::string>::iterator begin();
        stdu::vector<std::string>::iterator end();

        stdu::vector<std::string>::const_iterator begin() const;
        stdu::vector<std::string>::const_iterator end() const;
    };

    struct DataBlock {
        std::variant<
            std::monostate,
            RegularDataBlock,
            RegularDataBlockWKeys,
            TemplatedDataBlock
        > value;

        bool isRegularDataBlock() const;
        bool isWithKeys() const;
        bool isTemplatedDataBlock() const;
        bool empty() const;

        RegularDataBlock &getRegDataBlock();
        RegularDataBlockWKeys &getRegDataBlockWKeys();
        TemplatedDataBlock &getTemplatedDataBlock();

        const RegularDataBlock &getRegDataBlock() const;
        const RegularDataBlockWKeys &getRegDataBlockWKeys() const;
        const TemplatedDataBlock &getTemplatedDataBlock() const;

    private:
        friend struct ::uhash;

        auto members() const {
            return std::tie(value);
        }
    };

    struct Rule {
        stdu::vector<std::shared_ptr<AST::RuleMember>> rule_members;
        AST::DataBlock data_block;
        stdu::vector<std::string> original_rules;
        bool isTopLevel = true;

    private:
        friend struct ::uhash;

        auto members() const {
            return std::tie(rule_members, data_block);
        }
    };
    // Helper functions
    CllExpr make_expr_from_value(const AST::CllExprValue &val);

    // Define operator== outside structures
    bool operator==(const String &lhs, const String &rhs);
    bool operator==(const Number &lhs, const Number &rhs);
    bool operator==(const Boolean &lhs, const Boolean &rhs);
    bool operator==(const Array &lhs, const Array &rhs);
    bool operator==(const Object &lhs, const Object &rhs);
    bool operator==(const ID &lhs, const ID &rhs);
    bool operator==(const rvalue &lhs, const rvalue &rhs);
    bool operator==(const CllCompareOp &lhs, const CllCompareOp &rhs);
    bool operator==(const CllLogicalOp &lhs, const CllLogicalOp &rhs);
    bool operator==(const CllType &lhs, const CllType &rhs);
    bool operator==(const CllFunctionArguments &lhs, const CllFunctionArguments &rhs);
    bool operator==(const CllFunctionParameters &lhs, const CllFunctionParameters &rhs);
    bool operator==(const CllFunctionCall &lhs, const CllFunctionCall &rhs);
    bool operator==(const CllFunctionDecl &lhs, const CllFunctionDecl &rhs);
    bool operator==(const CllMethodCall &lhs, const CllMethodCall &rhs);
    bool operator==(const CllExprValue &lhs, const CllExprValue &rhs);
    bool operator==(const CllExprTerm &lhs, const CllExprTerm &rhs);
    bool operator==(const CllExprAddition &lhs, const CllExprAddition &rhs);
    bool operator==(const CllExprCompare &lhs, const CllExprCompare &rhs);
    bool operator==(const CllExprLogical &lhs, const CllExprLogical &rhs);
    bool operator==(const CllExpr &lhs, const CllExpr &rhs);
    bool operator==(const CllVariableMention &lhs, const CllVariableMention &rhs);
    bool operator==(const CllIf &lhs, const CllIf &rhs);
    bool operator==(const CllVar &lhs, const CllVar &rhs);
    bool operator==(const CllLoopWhile &lhs, const CllLoopWhile &rhs);
    bool operator==(const Cll &first, const Cll &second);
    bool operator==(const RulePrefix &lhs, const RulePrefix &rhs);
    bool operator==(const RuleMemberName &lhs, const RuleMemberName &rhs);
    bool operator==(const RuleMemberGroup &lhs, const RuleMemberGroup &rhs);
    bool operator==(const RuleMemberOp &lhs, const RuleMemberOp &rhs);
    bool operator==(const RuleMemberCsequence &lhs, const RuleMemberCsequence &rhs);
    bool operator==(const RuleMemberAny &lhs, const RuleMemberAny &rhs);
    bool operator==(const RuleMemberNospace &lhs, const RuleMemberNospace &rhs);
    bool operator==(const RuleMemberEscaped &lhs, const RuleMemberEscaped &rhs);
    bool operator==(const RuleMemberHex &lhs, const RuleMemberHex &rhs);
    bool operator==(const RuleMemberBin &lhs, const RuleMemberBin &rhs);
    bool operator==(const RuleMember &lhs, const RuleMember &rhs);
    bool operator==(const RuleMemberKey &lhs, const RuleMemberKey &rhs);
    bool operator==(const RegularDataBlockWKeys &lhs, const RegularDataBlockWKeys &rhs);
    bool operator==(const TemplatedDataBlock &lhs, const TemplatedDataBlock &rhs);
    bool operator==(const DataBlock &lhs, const DataBlock &rhs);
    bool operator==(const Rule &lhs, const Rule &rhs);
    bool operator<(const String &lhs, const String &rhs);
    bool operator<(const Number &lhs, const Number &rhs);
    bool operator<(const Boolean &lhs, const Boolean &rhs);
    bool operator<(const Array &lhs, const Array &rhs);
    bool operator<(const Object &lhs, const Object &rhs);
    bool operator<(const ID &lhs, const ID &rhs);
    bool operator<(const rvalue &lhs, const rvalue &rhs);
    bool operator<(const CllCompareOp &lhs, const CllCompareOp &rhs);
    bool operator<(const CllLogicalOp &lhs, const CllLogicalOp &rhs);
    bool operator<(const CllType &lhs, const CllType &rhs);
    bool operator<(const CllFunctionArguments &lhs, const CllFunctionArguments &rhs);
    bool operator<(const CllFunctionParameters &lhs, const CllFunctionParameters &rhs);
    bool operator<(const CllFunctionCall &lhs, const CllFunctionCall &rhs);
    bool operator<(const CllFunctionDecl &lhs, const CllFunctionDecl &rhs);
    bool operator<(const CllMethodCall &lhs, const CllMethodCall &rhs);
    bool operator<(const CllExprValue &lhs, const CllExprValue &rhs);
    bool operator<(const CllExprTerm &lhs, const CllExprTerm &rhs);
    bool operator<(const CllExprAddition &lhs, const CllExprAddition &rhs);
    bool operator<(const CllExprCompare &lhs, const CllExprCompare &rhs);
    bool operator<(const CllExprLogical &lhs, const CllExprLogical &rhs);
    bool operator<(const CllExpr &lhs, const CllExpr &rhs);
    bool operator<(const CllVariableMention &lhs, const CllVariableMention &rhs);
    bool operator<(const CllIf &lhs, const CllIf &rhs);
    bool operator<(const CllVar &lhs, const CllVar &rhs);
    bool operator<(const CllLoopWhile &lhs, const CllLoopWhile &rhs);
    bool operator<(const Cll &first, const Cll &second);
    bool operator<(const RulePrefix &lhs, const RulePrefix &rhs);
    bool operator<(const RuleMemberName &lhs, const RuleMemberName &rhs);
    bool operator<(const RuleMemberGroup &lhs, const RuleMemberGroup &rhs);
    bool operator<(const RuleMemberOp &lhs, const RuleMemberOp &rhs);
    bool operator<(const RuleMemberCsequence &lhs, const RuleMemberCsequence &rhs);
    bool operator<(const RuleMemberAny &lhs, const RuleMemberAny &rhs);
    bool operator<(const RuleMemberNospace &lhs, const RuleMemberNospace &rhs);
    bool operator<(const RuleMemberEscaped &lhs, const RuleMemberEscaped &rhs);
    bool operator<(const RuleMemberHex &lhs, const RuleMemberHex &rhs);
    bool operator<(const RuleMemberBin &lhs, const RuleMemberBin &rhs);
    bool operator<(const RuleMember &lhs, const RuleMember &rhs);
    bool operator<(const RuleMemberKey &lhs, const RuleMemberKey &rhs);
    bool operator<(const RegularDataBlockWKeys &lhs, const RegularDataBlockWKeys &rhs);
    bool operator<(const TemplatedDataBlock &lhs, const TemplatedDataBlock &rhs);
    bool operator<(const DataBlock &lhs, const DataBlock &rhs);
    bool operator<(const Rule &lhs, const Rule &rhs);
    std::ostream &operator<<(std::ostream &os, const RulePrefix &prefix);
    std::ostream &operator<<(std::ostream &os, const RuleMemberName &name);
    std::ostream &operator<<(std::ostream &os, const RuleMemberGroup &group);
    std::ostream &operator<<(std::ostream &os, const RuleMemberOp &op);
    std::ostream &operator<<(std::ostream &os, const RuleMemberCsequence &cs);
    std::ostream &operator<<(std::ostream &os, const RuleMemberAny &any);
    std::ostream &operator<<(std::ostream &os, const RuleMemberNospace &ns);
    std::ostream &operator<<(std::ostream &os, const RuleMemberEscaped &esc);
    std::ostream &operator<<(std::ostream &os, const RuleMemberHex &hex);
    std::ostream &operator<<(std::ostream &os, const RuleMemberBin &bin);
    std::ostream &operator<<(std::ostream &os, const String &str);
    std::ostream &operator<<(std::ostream &os, const Cll &str);
    std::ostream &operator<<(std::ostream &os, const RuleMember &member);
    std::ostream &operator<<(std::ostream &os, const RegularDataBlockWKeys &block);
    std::ostream &operator<<(std::ostream &os, const TemplatedDataBlock &block);
    std::ostream &operator<<(std::ostream &os, const DataBlock &block);
    std::ostream &operator<<(std::ostream &os, const Rule &rule);
};