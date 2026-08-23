export module LangAPI;
import hash;
import dstd;
import cpuf.printf;
import std;

// forward declarations
export namespace LangAPI {
    class RValue;
    struct ExpressionValue;
    struct Expression;
    struct Statement;
    struct Statements;
    struct Declaration;
    struct Declarations;
    struct ConditionalElement;

    auto operator<<(std::ostream& os, const Declarations& expr) -> std::ostream&;
    auto operator<(const Declarations& a, const Declarations& b) -> bool;
    auto operator<<(std::ostream& os, const Statements& expr) -> std::ostream&;
    auto operator<(const Statements& a, const Statements& b) -> bool;
    auto operator<<(std::ostream& os, const Expression& expr) -> std::ostream&;
    auto operator<(const Expression& a, const Expression& b) -> bool;

    // Default: no promotion
    template<typename T>
    struct promote_to {
        using type = void;
        static constexpr bool can_promote = false;
    };

    // Specializations
    template<> struct promote_to<LangAPI::RValue> {
        using type = LangAPI::ExpressionValue;
        static constexpr bool can_promote = true;
    };
    template<> struct promote_to<LangAPI::ExpressionValue> {
        using type = LangAPI::Expression;
        static constexpr bool can_promote = true;
    };
    template<> struct promote_to<LangAPI::Expression> {
        using type = LangAPI::Statement;
        static constexpr bool can_promote = true;
    };
    template<> struct promote_to<LangAPI::Statement> {
        using type = LangAPI::Statements;
        static constexpr bool can_promote = true;
    };
    template<> struct promote_to<LangAPI::Statements> {
        using type = LangAPI::Declaration;
        static constexpr bool can_promote = true;
    };
    template<> struct promote_to<LangAPI::Declaration> {
        using type = LangAPI::Declarations;
        static constexpr bool can_promote = true;
    };

    // Recursive promotion or construction
    template<typename whatToConstruct, typename fromWhatToConstruct>
    auto promote_or_construct(fromWhatToConstruct&& value) -> whatToConstruct {
        if constexpr (std::is_constructible_v<whatToConstruct, fromWhatToConstruct>) {
            return whatToConstruct{ std::forward<fromWhatToConstruct>(value) };
        }
        else if constexpr (requires { typename std::decay_t<fromWhatToConstruct>::promote_to; }) {
            using P = typename std::decay_t<fromWhatToConstruct>::promote_to;
            return promote_or_construct<whatToConstruct>(P{ std::forward<fromWhatToConstruct>(value) });
        }
        else {
            return {};
        }
    }
}
export namespace LangAPI {
    enum class ExpressionElement {
        GroupOpen, GroupClose, SquareBraceOpen, SquareBraceClose,
        And, Or, Not, Equal, NotEqual,
        Higher, Lower, HigherOrEqual, LowerOrEqual,
        Add, Minus, Multiply, Divide, Modulo,
        PlusPlus, MinusMinus
    };
    enum class OperatorType {
        Assign, Add, Minus, Multiply, Divide, Modulo
    };
    enum class ValueType {
        Undef, Void, Char, Int, Bool, Float, String, NonOwnedString, Array, FixedSizeArray, Map, Symbol, StorageSymbol, Inheritance, Token, Rule, TokenResult, RuleResult, Span, Variant, Box, Any, Const, Reference, Tuple
    };
    enum class RValueType {
        Undef, Char, Int, Bool, Float, String, Array, FixedSizeArray, Map, Pos, Symbol, IspaLibSymbol, StorageSymbol, Inheritance, IspaLibDfaTransition, IspaLibDfaSpanCharState, IspaLibDfaSpanMultiTableState, IspaLibDfaEmptyState, IspaLibDfaSpan, Reference, Span, MakeTuple, GetVariant
    };
    enum class ExpressionValueType {
        Empty, EmptyInitializer, RValue, ExpressionElement, FunctionCall, IspaLibFunctionCall, StringCompare, Return, Break, Continue, VariableAssignment, CounterIncreament, CounterIncreamentByLength,
        ResetPosCounter, PushPosCounter, PopPosCounter, SkipSpaces, DfaLookup, ReportError, Lambda
    };
    enum class ArrayMethods {
        Push, Pop
    };
    enum class Visibility {
        Private, Public
    };
    enum class ObjectMethods {};
    enum class Language {
        Cpp
    };
    enum class StdlibExports {
        Node, MatchResult, Lexer, Parser, LexerMakeTokenParameter,
        DfaState, DfaTable, DfaClassTable, DfaAcceptTable, DfaLRTable, DfaNullState,
        ParserFunctionParameter, Error
    };


    auto operator<<(std::ostream &os, ExpressionElement e) -> std::ostream&;
    auto operator<<(std::ostream &os, OperatorType op) -> std::ostream&;
    auto operator<<(std::ostream &os, ValueType v) -> std::ostream&;
    auto operator<<(std::ostream &os, RValueType r) -> std::ostream&;
    auto operator<<(std::ostream &os, ExpressionValueType e) -> std::ostream&;
    auto operator<<(std::ostream &os, ArrayMethods m) -> std::ostream&;
    auto operator<<(std::ostream &os, Visibility v) -> std::ostream&;
    auto operator<<(std::ostream &os, Language l) -> std::ostream&;
    auto operator<<(std::ostream &os, StdlibExports e) -> std::ostream&;
    struct DeclarationsLevel {
        using promote_to = Declarations;
        template<typename T>
        static auto createDeclarations(T&& v) {
            return promote_or_construct<Declarations>(std::forward<T>(v));
        }
    };
    struct DeclarationLevel : DeclarationsLevel {
        using promote_to = Declaration;
        template<typename T>
        static auto createDeclaration(T&& v) {
            return promote_or_construct<Declaration>(std::forward<T>(v));
        }
    };
    struct StatementsLevel : DeclarationLevel {
        using promote_to = Statements;
        template<typename T>
        static auto createStatements(T&& v) {
            return promote_or_construct<Statements>(std::forward<T>(v));
        }
    };
    struct StatementLevel : StatementsLevel {
        using promote_to = Statement;
        template<typename T>
        static auto createStatement(T&& v) {
            return promote_or_construct<Statement>(std::forward<T>(v));
        }
    };
    struct ExpressionLevel : StatementLevel {
        using promote_to = Expression;
        template<typename T>
        static auto createExpression(T&& v) {
            return promote_or_construct<Expression>(std::forward<T>(v));
        }
    };
    struct ExpressionValueLevel : ExpressionLevel {
        using promote_to = ExpressionValue;
        template<typename T>
        requires std::is_constructible_v<ExpressionValue, std::decay_t<T>>
        static auto createExpressionValue(T&& v) {
            return promote_or_construct<ExpressionValue>(std::forward<T>(v));
        }
    };
    struct RValueLevel : ExpressionValueLevel {
        using promote_to = RValue;
        template<typename T>
        static auto createRValue(T&& v) {
            return promote_or_construct<RValue>(std::forward<T>(v));
        }
    };
    struct Declarations : stdu::vector<Declaration> {
        using vector::vector;
        Declarations();
        Declarations(const Declarations&);
        Declarations(Declarations&&) noexcept;
        Declarations(const stdu::vector<Declaration>&);
        Declarations(stdu::vector<Declaration>&&);
        Declarations& operator=(const Declarations&);
        Declarations& operator=(Declarations&&) noexcept;
        ~Declarations();
        friend auto operator<<(std::ostream& os, const Declarations &expr) -> std::ostream&;
        friend auto operator<(const Declarations& a, const Declarations& b) -> bool;
    };
    struct Statements : stdu::vector<Statement>, DeclarationLevel {
        using vector::vector;
        Statements();
        Statements(const Statements&);
        Statements(Statements&&) noexcept;
        Statements(const stdu::vector<Statement>&);
        Statements(stdu::vector<Statement>&&);
        Statements& operator=(const Statements&);
        Statements& operator=(Statements&&) noexcept;
        Statements(const Statement&);
        Statements(Statement&&);
        Statements(std::initializer_list<Statement>);
        ~Statements();
        friend auto operator<<(std::ostream& os, const Statements &expr) -> std::ostream&;
        friend auto operator<(const Statements& a, const Statements& b) -> bool;

    };
    struct Expression : stdu::vector<ExpressionValue>, StatementLevel {
        using vector::vector;
        Expression();
        Expression(const Expression&);
        Expression(Expression&&) noexcept;
        Expression(const stdu::vector<ExpressionValue>&);
        Expression(stdu::vector<ExpressionValue>&&);
        Expression& operator=(const Expression&);
        Expression& operator=(Expression&&) noexcept;
        Expression(const ExpressionValue&);
        Expression(ExpressionValue&&);
        Expression(std::initializer_list<ExpressionValue>);
        ~Expression();
        friend auto operator<<(std::ostream& os, const Expression &expr) -> std::ostream&;
        friend auto operator<(const Expression& a, const Expression& b) -> bool;
    };
    // forward declarations
    struct Type;
    struct Lambda;
    struct IspaLibSymbol;
    struct Symbol;
    struct Char : RValueLevel {
        char value;
        bool escaped = false;

        bool operator==(const Char& c) const {
            return value == c.value && escaped == c.escaped;
        }
        bool operator!=(const Char& c) const {
            return !(*this == c);
        }
        bool operator<(const Char& a) const {
            if (value != a.value) return value < a.value;
            else return escaped < a.escaped;
        }
        friend auto operator<<(std::ostream& os, const Char &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(value, escaped);
        }
    };

    struct Int : RValueLevel {
        long long value;

        bool operator==(const Int& other) const { return value == other.value; }
        friend bool operator!=(const Int &a, const Int &b) { return !(a == b); }
        bool operator<(const Int& other) const { return value < other.value; }
        friend auto operator<<(std::ostream& os, const Int &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(value);
        }
    };

    struct Bool : RValueLevel {
        bool value;

        bool operator==(const Bool& other) const { return value == other.value; }
        friend bool operator!=(const Bool &a, const Bool &b) { return !(a == b); }
        bool operator<(const Bool& other) const { return value < other.value; }
        friend auto operator<<(std::ostream& os, const Bool &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(value);
        }
    };

    struct Float : RValueLevel {
        double value;

        bool operator==(const Float& other) const { return value == other.value; }
        friend bool operator!=(const Float &a, const Float &b) { return !(a == b); }
        bool operator<(const Float& other) const { return value < other.value; }
        friend auto operator<<(std::ostream& os, const Float &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(value);
        }
    };

    struct String : RValueLevel {
        std::string value;

        bool operator==(const String& other) const { return value == other.value; }
        friend bool operator!=(const String &a, const String &b) { return !(a == b); }
        bool operator<(const String& other) const { return value < other.value; }
        friend auto operator<<(std::ostream& os, const String &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(value);
        }
    };

    struct Array : RValueLevel {
        stdu::vector<Expression> values;
        stdu::vector<std::variant<std::shared_ptr<Type>, std::shared_ptr<RValue>>> template_parameters;
        friend bool operator==(const Array &a, const Array &b); // must be declared in .cpp file to resolve incomplete type errors
        friend bool operator!=(const Array &a, const Array &b) { return !(a == b); }
        friend bool operator<(const Array &a, const Array &b);
        friend auto operator<<(std::ostream& os, const Array &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(values, template_parameters);
        }
    };

    struct FixedSizeArray : RValueLevel {
        stdu::vector<Expression> values;
        stdu::vector<std::variant<std::shared_ptr<Type>, std::shared_ptr<RValue>>> template_parameters;
        friend bool operator==(const FixedSizeArray &a, const FixedSizeArray &b);
        friend bool operator!=(const FixedSizeArray &a, const FixedSizeArray &b) { return !(a == b); }
        friend bool operator<(const FixedSizeArray &a, const FixedSizeArray &b);
        friend auto operator<<(std::ostream& os, const FixedSizeArray &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(values);
        }
    };
    struct Map : RValueLevel {
        stdu::vector<std::variant<Int, String>> keys;
        stdu::vector<Expression> values;
        stdu::vector<std::variant<std::shared_ptr<Type>, std::shared_ptr<RValue>>> template_parameters;

        friend bool operator==(const Map &a, const Map &b);
        friend bool operator!=(const Map &a, const Map &b) { return !(a == b); }
        friend bool operator<(const Map &a, const Map &b);
        friend auto operator<<(std::ostream& os, const Map &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(keys, values, template_parameters);
        }
    };

    struct Pos : RValueLevel {
        bool dereference = true;
        std::size_t offset;

        bool operator==(const Pos& other) const {
            return offset == other.offset && dereference == other.dereference;
        }
        friend bool operator!=(const Pos &a, const Pos &b) { return !(a == b); }
        bool operator<(const Pos& other) const { return offset < other.offset; }
        friend auto operator<<(std::ostream& os, const Pos &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(dereference, offset);
        }
    };
    struct ArrayMethodCall : RValueLevel {
        ArrayMethods method;
        stdu::vector<Expression> args;

        friend bool operator==(const ArrayMethodCall &a, const ArrayMethodCall &b);
        friend bool operator!=(const ArrayMethodCall &a, const ArrayMethodCall &b) { return !(a == b); }
        friend bool operator<(const ArrayMethodCall &a, const ArrayMethodCall &b);
        friend auto operator<<(std::ostream& os, const ArrayMethodCall &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(method, args);
        }
    };
    struct FunctionCall : ExpressionValueLevel {
        std::variant<std::shared_ptr<Symbol>, std::shared_ptr<IspaLibSymbol>> name;
        stdu::vector<std::variant<std::shared_ptr<Type>, std::shared_ptr<RValue>>> template_parameters;
        stdu::vector<Expression> args;
        ~FunctionCall();
        friend bool operator==(const FunctionCall &a, const FunctionCall &b);
        friend bool operator!=(const FunctionCall &a, const FunctionCall &b) { return !(a == b); }
        friend bool operator<(const FunctionCall &a, const FunctionCall &b);
        friend auto operator<<(std::ostream& os, const FunctionCall &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const;
    };
    struct IspaLibSymbol {
        StdlibExports exports;
        stdu::vector<std::variant<std::shared_ptr<Type>, std::shared_ptr<RValue>>> template_parameters;
        bool Const = false;
        bool Reference = false;
        friend bool operator==(const IspaLibSymbol &a, const IspaLibSymbol& b);
        friend bool operator!=(const IspaLibSymbol &a, const IspaLibSymbol &b) { return !(a == b); }
        friend bool operator<(const IspaLibSymbol& a, const IspaLibSymbol &b);
        friend auto operator<<(std::ostream& os, const IspaLibSymbol &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(exports);
        }
    };
    struct Symbol : RValueLevel {
        using PathPart = std::variant<FunctionCall, IspaLibSymbol, std::string>;
        stdu::vector<PathPart> path;

        template<typename... Args>
        requires (std::constructible_from<PathPart, Args> && ...)
        Symbol(Args&&... args) {
            (path.emplace_back(std::forward<Args>(args)), ...);
        }
        template<std::ranges::input_range R>
        requires std::constructible_from<PathPart, std::ranges::range_value_t<R>>
        Symbol(const R& el) {
            for (auto &e : el) {
                path.emplace_back(e);
            }
        }
        Symbol(const stdu::vector<PathPart> &path) : path(path) {}
        Symbol(stdu::vector<PathPart> &&path) : path(std::move(path)) {}
        friend bool operator==(const Symbol& a, const Symbol& b);
        friend bool operator!=(const Symbol &a, const Symbol &b) { return !(a == b); }
        friend bool operator<(const Symbol &a, const Symbol& b);
        friend auto operator<<(std::ostream& os, const Symbol &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(path);
        }
    };
    struct StorageOffset : RValueLevel {
        Expression offset;

        friend bool operator==(const StorageOffset &a, const StorageOffset &b);
        friend bool operator!=(const StorageOffset &a, const StorageOffset &b) { return !(a == b); }
        friend bool operator<(const StorageOffset &a, const StorageOffset &b);
        friend auto operator<<(std::ostream& os, const StorageOffset &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(offset);
        }

    };
    struct StorageSymbol : RValueLevel {
        using PathPart = std::variant<FunctionCall, ArrayMethodCall, IspaLibSymbol, StorageOffset, std::string>;
        Expression what;
        stdu::vector<PathPart> path;

        template<typename... Args>
        requires (std::constructible_from<PathPart, Args> && ...)
        StorageSymbol(Args&&... args) {
            (path.emplace_back(std::forward<Args>(args)), ...);
        }
        template<std::ranges::input_range R>
        requires std::constructible_from<PathPart, std::ranges::range_value_t<R>>
        StorageSymbol(const R& el) {
            for (auto &e : el) {
                path.emplace_back(e);
            }
        }
        StorageSymbol(const stdu::vector<PathPart> &path) : path(path) {}
        StorageSymbol(stdu::vector<PathPart> &&path) : path(std::move(path)) {}

        friend bool operator==(const StorageSymbol &a, const StorageSymbol &b);
        friend bool operator!=(const StorageSymbol &a, const StorageSymbol &b) { return !(a == b); }
        friend bool operator<(const StorageSymbol &a, const StorageSymbol &b);
        friend auto operator<<(std::ostream& os, const StorageSymbol &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(what, path);
        }
    };
    struct IspaLibFunctionCall : ExpressionValueLevel {
        IspaLibSymbol symbol;
        stdu::vector<Expression> args;

        friend bool operator==(const IspaLibFunctionCall &a, const IspaLibFunctionCall &b);
        friend bool operator!=(const IspaLibFunctionCall &a, const IspaLibFunctionCall &b) { return !(a == b); }
        friend bool operator<(const IspaLibFunctionCall &a, const IspaLibFunctionCall &b);
        friend auto operator<<(std::ostream& os, const IspaLibFunctionCall &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(symbol, args);
        }
    };
    struct Inheritance : RValueLevel {
        std::variant<Symbol, IspaLibSymbol> name;
        stdu::vector<Expression> args;

        friend bool operator==(const Inheritance &a, const Inheritance &b);
        friend bool operator!=(const Inheritance &a, const Inheritance &b) { return !(a == b); }
        friend bool operator<(const Inheritance &a, const Inheritance &b);
        friend auto operator<<(std::ostream& os, const Inheritance &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(name, args);
        }
    };

    struct Token : RValueLevel {
        bool operator==(const Token&) const { return true; }
        bool operator!=(const Token&) const { return false; }
        bool operator<(const Token&) const { return false; }
        friend auto operator<<(std::ostream& os, const Token &c) -> std::ostream&;
    };

    struct Rule : RValueLevel {
        bool operator==(const Rule&) const { return true; }
        bool operator!=(const Rule&) const { return false; }
        bool operator<(const Rule&) const { return false; }
        friend auto operator<<(std::ostream& os, const Rule &c) -> std::ostream&;
    };

    struct TokenResult {
        bool operator==(const TokenResult&) const { return true; }
        bool operator!=(const TokenResult&) const { return false; }
        bool operator<(const TokenResult&) const { return false; }
        friend auto operator<<(std::ostream& os, const TokenResult &c) -> std::ostream&;
    };

    struct RuleResult {
        bool operator==(const RuleResult&) const { return true; }
        bool operator!=(const RuleResult&) const { return false; }
        bool operator<(const RuleResult&) const { return false; }
        friend auto operator<<(std::ostream& os, const RuleResult &c) -> std::ostream&;
    };
    struct IspaLibDfaTransition : RValueLevel {
        std::variant<stdu::vector<std::string>, std::size_t, char> symbol;
        std::size_t next;
        bool new_cst_node;
        bool new_member;
        bool close_cst_node;
        std::size_t new_group;
        std::size_t group_close;
        std::size_t accept;
        IspaLibSymbol transition_type;
        bool is_refferring_char_table = false;

        auto operator==(const IspaLibDfaTransition& other) const {
            return  symbol == other.symbol &&
                    next == other.next &&
                    new_cst_node == other.new_cst_node &&
                    new_member == other.new_member &&
                    close_cst_node == other.close_cst_node &&
                    new_group == other.new_group &&
                    group_close == other.group_close &&
                    accept == other.accept;
        }
        auto operator!=(const IspaLibDfaTransition& other) const { return !(*this == other); }
        auto operator<(const IspaLibDfaTransition& other) const {
            if (symbol != other.symbol) return symbol < other.symbol;
            else if (next != other.next) return next < other.next;
            else if (new_cst_node != other.new_cst_node) return new_cst_node < other.new_cst_node;
            else if (new_member != other.new_member) return new_member < other.new_member;
            else if (close_cst_node != other.close_cst_node) return close_cst_node < other.close_cst_node;
            else if (new_group != other.new_group) return new_group < other.new_group;
            else if (group_close != other.group_close) return group_close < other.group_close;
            else if (accept != other.accept) return accept < other.accept;
            else return transition_type < other.transition_type;
        }
        friend auto operator<<(std::ostream& os, const IspaLibDfaTransition &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(symbol, next, new_cst_node, new_member, close_cst_node, new_group, group_close, accept);
        }
    };
    struct IspaLibDfaState : RValueLevel {
        stdu::vector<IspaLibDfaTransition> transitions;

        auto operator==(const IspaLibDfaState& other) const {
            return transitions == other.transitions;
        }
        auto operator!=(const IspaLibDfaState& other) const { return !(*this == other); }
        auto operator<(const IspaLibDfaState& other) const { return transitions < other.transitions;}
        friend auto operator<<(std::ostream& os, const IspaLibDfaState &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(transitions);
        }
    };
    struct IspaLibDfaSpanCharState : RValueLevel {
        std::size_t else_goto = std::numeric_limits<std::size_t>::max();
        std::size_t else_goto_accept = std::numeric_limits<std::size_t>::max();
        std::size_t state_id;
        auto operator==(const IspaLibDfaSpanCharState& other) const {
            return else_goto == other.else_goto && else_goto_accept == other.else_goto_accept && state_id == other.state_id;
        }
        auto operator!=(const IspaLibDfaSpanCharState& other) const { return !(*this == other); }
        auto operator<(const IspaLibDfaSpanCharState& other) const {
            if (else_goto != other.else_goto) return else_goto < other.else_goto;
            else if (else_goto_accept != other.else_goto_accept) return else_goto_accept < other.else_goto_accept;
            else return state_id < other.state_id;
        }
        friend auto operator<<(std::ostream& os, const IspaLibDfaSpanCharState &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(else_goto, else_goto_accept, state_id);
        }
    };
    struct IspaLibDfaSpanMultiTableState : RValueLevel {
        std::size_t else_goto = std::numeric_limits<std::size_t>::max();
        std::size_t else_goto_accept = std::numeric_limits<std::size_t>::max();
        std::size_t state_id;
        stdu::vector<IspaLibSymbol> mutli_table_transitions;
        auto operator==(const IspaLibDfaSpanMultiTableState& other) const {
            return else_goto == other.else_goto && else_goto_accept == other.else_goto_accept && state_id == other.state_id && mutli_table_transitions == other.mutli_table_transitions;
        }
        auto operator!=(const IspaLibDfaSpanMultiTableState& other) const { return !(*this == other); }
        auto operator<(const IspaLibDfaSpanMultiTableState& other) const {
            if (else_goto != other.else_goto) return else_goto < other.else_goto;
            else if (else_goto_accept != other.else_goto_accept) return else_goto_accept < other.else_goto_accept;
            else if (state_id != other.state_id) return state_id < other.state_id;
            else return mutli_table_transitions < other.mutli_table_transitions;
        }
        friend auto operator<<(std::ostream& os, const IspaLibDfaSpanMultiTableState &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(else_goto, else_goto_accept, state_id, mutli_table_transitions);
        }
    };
    struct IspaLibDfaEmptyState : RValueLevel {
        stdu::vector<std::string> token_name;
        std::shared_ptr<Lambda> construction_lambda;
        auto operator==(const IspaLibDfaEmptyState& other) const {
            return token_name == other.token_name && construction_lambda == other.construction_lambda;
        }
        auto operator!=(const IspaLibDfaEmptyState& other) const { return !(*this == other); }
        auto operator<(const IspaLibDfaEmptyState& other) const {
            if (token_name != other.token_name) return token_name < other.token_name;
            else return construction_lambda < other.construction_lambda;
        }
        friend auto operator<<(std::ostream& os, const IspaLibDfaEmptyState &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(token_name, construction_lambda);
        }
    };
    struct IspaLibDfaSpan : RValueLevel {
        IspaLibSymbol type;
        LangAPI::Symbol assing_name;
        auto operator==(const IspaLibDfaSpan& other) const {
            return type == other.type && assing_name == other.assing_name;
        }
        auto operator!=(const IspaLibDfaSpan& other) const { return !(*this == other); }
        auto operator<(const IspaLibDfaSpan& other) const {
            if (type != other.type) return type < other.type;
            else return assing_name < other.assing_name;
        }
        friend auto operator<<(std::ostream& os, const IspaLibDfaSpan &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(type);
        }
    };
    struct Reference : RValueLevel {
        std::shared_ptr<RValue> value;

        auto operator==(const Reference& other) const {
            return value == other.value;
        }
        auto operator!=(const Reference& other) const { return !(*this == other); }
        auto operator<(const Reference& other) const { return value < other.value; }
        friend auto operator<<(std::ostream& os, const Reference &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(value);
        }
    };
    struct Span : RValueLevel {
        std::shared_ptr<Type> type;
        Symbol sym;

        auto operator==(const Span& other) const {
            return sym == other.sym;
        }
        auto operator!=(const Span& other) const { return !(*this == other); }
        auto operator<(const Span& other) const {
            if (type != other.type) return type < other.type;
            else return sym < other.sym;
        }
        friend auto operator<<(std::ostream& os, const Span &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(sym);
        }
    };
    struct EmptyInitializer : ExpressionValueLevel {
        bool operator==(const EmptyInitializer&) const { return true; }
        bool operator!=(const EmptyInitializer&) const { return false; }
        bool operator<(const EmptyInitializer&) const { return false; }
        friend auto operator<<(std::ostream& os, const EmptyInitializer &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie();
        }
    };
    struct MakeTuple : RValueLevel {
        stdu::vector<Expression> args;
        bool operator==(const MakeTuple&) const { return true; }
        bool operator!=(const MakeTuple&) const { return false; }
        bool operator<(const MakeTuple&) const { return false; }
        friend auto operator<<(std::ostream& os, const MakeTuple &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(args);
        }
    };
    struct GetVariant : RValueLevel {
        std::shared_ptr<Type> type;
        Expression sym;
        bool operator==(const GetVariant&) const { return true; }
        bool operator!=(const GetVariant&) const { return false; }
        bool operator<(const GetVariant&) const { return false; }
        friend auto operator<<(std::ostream& os, const GetVariant &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(type, sym);
        }
    };
    class RValue : public ExpressionValueLevel {
        std::variant<std::monostate, Char, Int, Bool, Float, String, Array, FixedSizeArray, Map, Pos, Symbol, IspaLibSymbol, StorageSymbol, Inheritance, IspaLibDfaTransition, IspaLibDfaSpanCharState, IspaLibDfaSpanMultiTableState, IspaLibDfaEmptyState, IspaLibDfaSpan, Reference, Span, MakeTuple, GetVariant> value;
        friend struct ::uhash;
        auto members() const {
            return std::tie(value);
        }
    public:
        using promote_to = ExpressionValue;
        RValue() {}
        template<typename T>
        requires std::is_constructible_v<decltype(value), std::decay_t<T>> && (!std::is_same_v<std::decay_t<T>, std::monostate>)
        RValue(const T &value) : value(value) {}
        template<typename T>
        requires std::is_constructible_v<decltype(value), std::decay_t<T>> && (!std::is_same_v<std::decay_t<T>, std::monostate>)
        RValue(T &&value) : value(std::move(value)) {}
        template<typename T>
        requires std::is_constructible_v<decltype(value), std::decay_t<T>> && (!std::is_same_v<std::decay_t<T>, std::monostate>)
        void set(T &&value)  {this->value = std::move(value); }
        template<typename T>
        requires std::is_constructible_v<decltype(value), std::decay_t<T>> && (!std::is_same_v<std::decay_t<T>, std::monostate>)
        void set(const T &value)  {this->value = std::move(value); }

        bool operator==(const RValue& other) const {
            return value == other.value;
        }
        friend bool operator!=(const RValue &a, const RValue &b) { return !(a == b); }
        bool isChar()    const { return std::holds_alternative<Char>(value); }
        bool isInt()     const { return std::holds_alternative<Int>(value); }
        bool isBool()    const { return std::holds_alternative<Bool>(value); }
        bool isFloat()   const { return std::holds_alternative<Float>(value); }
        bool isString()  const { return std::holds_alternative<String>(value); }
        bool isArray()   const { return std::holds_alternative<Array>(value); }
        bool isFixedSizeArray()   const { return std::holds_alternative<FixedSizeArray>(value); }
        bool isMap()     const { return std::holds_alternative<Map>(value); }
        bool isPos()     const { return std::holds_alternative<Pos>(value); }
        bool isSymbol()  const { return std::holds_alternative<Symbol>(value); }
        bool isIspaSymbol()  const { return std::holds_alternative<IspaLibSymbol>(value); }
        bool isStorageSymbol()  const { return std::holds_alternative<StorageSymbol>(value); }
        bool isInheritance()  const { return std::holds_alternative<Inheritance>(value); }
        bool isIspaLibDfaTransition()  const { return std::holds_alternative<IspaLibDfaTransition>(value); }
        bool isIspaLibDfaSpanState()  const { return std::holds_alternative<IspaLibDfaSpanCharState>(value); }
        bool isIspaLibDfaSpanMultiTableState()  const { return std::holds_alternative<IspaLibDfaSpanMultiTableState>(value); }
        bool isIspaLibDfaEmptyState()  const { return std::holds_alternative<IspaLibDfaEmptyState>(value); }
        bool isIspaLibDfaSpan()  const { return std::holds_alternative<IspaLibDfaSpan>(value); }
        // bool isIspaLibDfaState()  const { return std::holds_alternative<IspaLibDfaState>(value); }
        bool isReference()  const { return std::holds_alternative<Reference>(value); }
        bool isSpan()  const { return std::holds_alternative<Span>(value); }
        bool isMakeTuple()  const { return std::holds_alternative<MakeTuple>(value); }
        bool isGetVariant()  const { return std::holds_alternative<GetVariant>(value); }
        bool isUndef() const { return std::holds_alternative<std::monostate>(value); }
        bool empty() const { return std::holds_alternative<std::monostate>(value); }

        Char&           getChar()    { return std::get<Char>(value); }
        Int&            getInt()     { return std::get<Int>(value); }
        Bool&           getBool()    { return std::get<Bool>(value); }
        Float&          getFloat()   { return std::get<Float>(value); }
        String&         getString()  { return std::get<String>(value); }
        Array&          getArray()   { return std::get<Array>(value); }
        FixedSizeArray& getFixedSizeArray()   { return std::get<FixedSizeArray>(value); }
        Map&            getMap()     { return std::get<Map>(value); }
        Pos&            getPos()     { return std::get<Pos>(value); }
        Symbol&         getSymbol()  { return std::get<Symbol>(value); }
        IspaLibSymbol&  getIspaSymbol()  { return std::get<IspaLibSymbol>(value); }
        StorageSymbol&  getStorageSymbol()  { return std::get<StorageSymbol>(value); }
        Inheritance&  getInheritance()  { return std::get<Inheritance>(value); }
        IspaLibDfaTransition&  getIspaLibDfaTransition()  { return std::get<IspaLibDfaTransition>(value); }
        IspaLibDfaSpanCharState&  getIspaLibDfaSpanState()  { return std::get<IspaLibDfaSpanCharState>(value); }
        IspaLibDfaSpanMultiTableState&  getIspaLibDfaMultiTableState()  { return std::get<IspaLibDfaSpanMultiTableState>(value); }
        IspaLibDfaEmptyState&  getIspaLibDfaEmptyState()  { return std::get<IspaLibDfaEmptyState>(value); }
        IspaLibDfaSpan&  getIspaLibDfaSpan()  { return std::get<IspaLibDfaSpan>(value); }
        // IspaLibDfaState&  getIspaLibDfaState()  { return std::get<IspaLibDfaState>(value); }
        Reference&  getReference()  { return std::get<Reference>(value); }
        Span&  getSpan()  { return std::get<Span>(value); }
        MakeTuple&  getMakeTuple()  { return std::get<MakeTuple>(value); }
        GetVariant&  getVariantCast()  { return std::get<GetVariant>(value); }

        const Char&           getChar()   const { return std::get<Char>(value); }
        const Int&            getInt()    const { return std::get<Int>(value); }
        const Bool&           getBool()   const { return std::get<Bool>(value); }
        const Float&          getFloat()  const { return std::get<Float>(value); }
        const String&         getString() const { return std::get<String>(value); }
        const Array&          getArray()  const { return std::get<Array>(value); }
        const FixedSizeArray& getFixedSizeArray() const { return std::get<FixedSizeArray>(value); }
        const Map&            getMap()    const { return std::get<Map>(value); }
        const Pos&            getPos()    const { return std::get<Pos>(value); }
        const Symbol&         getSymbol() const { return std::get<Symbol>(value); }
        const IspaLibSymbol&  getIspaSymbol() const { return std::get<IspaLibSymbol>(value); }
        const StorageSymbol&  getStorageSymbol() const { return std::get<StorageSymbol>(value); }
        const Inheritance&  getInheritance() const  { return std::get<Inheritance>(value); }
        const IspaLibDfaTransition&  getIspaLibDfaTransition() const  { return std::get<IspaLibDfaTransition>(value); }
        const IspaLibDfaSpanCharState&  getIspaLibDfaSpanCharState() const  { return std::get<IspaLibDfaSpanCharState>(value); }
        const IspaLibDfaSpanMultiTableState&  getIspaLibDfaSpanMultiTableState() const  { return std::get<IspaLibDfaSpanMultiTableState>(value); }
        const IspaLibDfaEmptyState&  getIspaLibDfaEmptyState() const  { return std::get<IspaLibDfaEmptyState>(value); }
        const IspaLibDfaSpan&  getIspaLibDfaSpan() const { return std::get<IspaLibDfaSpan>(value); }
        // const IspaLibDfaState&  getIspaLibDfaState() const  { return std::get<IspaLibDfaState>(value); }
        const Reference&  getReference() const  { return std::get<Reference>(value); }
        const Span&  getSpan() const  { return std::get<Span>(value); }
        const MakeTuple&  getMakeTuple() const { return std::get<MakeTuple>(value); }
        const GetVariant&  getVariantCast() const { return std::get<GetVariant>(value); }

        auto type() const -> RValueType { return static_cast<RValueType>(value.index()); }
        auto get() const { return value; }
        bool operator<(const RValue& rhs) const {
            return std::visit([&](const auto &v1, const auto &v2) {
                if constexpr (std::is_same_v<std::decay_t<decltype(v1)>, std::monostate>) {
                    if constexpr (std::is_same_v<std::decay_t<decltype(v2)>, std::monostate>) {
                        return false;
                    } else return true;
                } else if constexpr (std::is_same_v<std::decay_t<decltype(v2)>, std::monostate>) return false;
                else if constexpr (std::is_same_v<std::decay_t<decltype(v1)>, std::decay_t<decltype(v2)>>) return v1 < v2;
                else return value.index() < rhs.value.index();
            }, value, rhs.value);
        }
        friend auto operator<<(std::ostream& os, const RValue &c) -> std::ostream&;
    };
    struct Type {
        std::variant<ValueType, Symbol, IspaLibSymbol> type;
        stdu::vector<std::variant<Type, RValue>> template_parameters;
        template<typename TypeOrPath, typename ...Templates>
        requires (std::is_same_v<std::decay_t<TypeOrPath>, ValueType> || std::is_same_v<std::decay_t<TypeOrPath>, Symbol> || std::is_same_v<std::decay_t<TypeOrPath>, IspaLibSymbol>)
        Type(TypeOrPath vtype, Templates&& ...templates) {
            type = vtype;
            (push(templates), ...);
        }
        Type() = default;
        bool isValueType() const {
            return std::holds_alternative<ValueType>(type);
        }
        bool isSymbol() const {
            return std::holds_alternative<Symbol>(type);
        }
        bool isIspaLibSymbol() const {
            return std::holds_alternative<IspaLibSymbol>(type);
        }
        auto &getValueType() {
            return std::get<ValueType>(type);
        }
        auto &getSymbol() {
            return std::get<Symbol>(type);
        }
        auto &getIspaLibSymbol() {
            return std::get<IspaLibSymbol>(type);
        }
        const auto &getIspaLibSymbol() const {
            return std::get<IspaLibSymbol>(type);
        }
        auto &getValueType() const {
            return std::get<ValueType>(type);
        }
        auto &getSymbol() const {
            return std::get<Symbol>(type);
        }
        bool operator==(const ValueType &v) const {
            return std::holds_alternative<ValueType>(type) && std::get<ValueType>(type) == v;
        }
        bool operator!=(const ValueType &v) const {
            return !(*this == v);
        }
        bool operator==(const Symbol &v) const {
            return std::holds_alternative<Symbol>(type) && std::get<Symbol>(type) == v;
        }
        bool operator!=(const Symbol &v) const {
            return !(*this == v);
        }
        bool operator==(const Type& other) const {
            if (type != other.type)
                return false;
            const auto templ1 = template_parameters;
            const auto templ2 = other.template_parameters;
            if (templ1.size() != templ2.size()) return false;

            for (std::size_t i = 0; i < templ1.size(); ++i) {
                if (templ1[i].index() != templ2[i].index()) return false;
                if (std::holds_alternative<Type>(templ1[i])) {
                    const auto &first = std::get<Type>(templ1[i]);
                    const auto &second = std::get<Type>(templ2[i]);
                    return first == second;
                } else {
                    const auto &first = std::get<RValue>(templ1[i]);
                    const auto &second = std::get<RValue>(templ2[i]);
                    return first == second;
                }
            }
            return true;
        }
        bool operator!=(const Type &other) const {
            return !(*this == other);
        }
        bool operator<(const Type &other) const {
            if (type != other.type) return type < other.type;
            else return template_parameters < other.template_parameters;
        }
        friend auto operator<<(std::ostream& os, const Type &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(type, template_parameters);
        }
        template<typename T>
        requires std::ranges::range<T>
        void push(const T &el) {
            for (const auto &e : el) {
                template_parameters.push_back(e);
            }
        }
        template<typename T>
        requires (!std::ranges::range<std::remove_cvref_t<T>>)
        void push(T &&el) {
            template_parameters.push_back(el);
        }
    };
    struct ForwardDeclaredClass : DeclarationLevel {
        std::string name;
        bool isStruct = true;
        auto operator==(const ForwardDeclaredClass& other) const {
            return name == other.name;
        }
        auto operator!=(const ForwardDeclaredClass& other) const {
            return !(*this == other);
        }
        auto operator<(const ForwardDeclaredClass& other) const {
            return name < other.name;
        }
        friend auto operator<<(std::ostream& os, const ForwardDeclaredClass &c) -> std::ostream&;
    };
    struct Class : DeclarationLevel {
        std::string name;
        stdu::vector<std::pair<std::shared_ptr<Declaration>, Visibility>> data;
        stdu::vector<std::pair<Visibility, std::variant<Symbol, IspaLibSymbol>>> inherit_members;
        Visibility default_visibility = Visibility::Public;
        bool operator==(const Class& other) const {
            return name == other.name && data == other.data && inherit_members == other.inherit_members && default_visibility == other.default_visibility;
        }
        bool operator!=(const Class& other) const {
            return !(*this == other);
        }
        bool operator<(const Class& other) const {
            if (name != other.name) return name < other.name;
            else if (data != other.data) return data < other.data;
            else if (inherit_members != other.inherit_members) return inherit_members < other.inherit_members;
            else return default_visibility < other.default_visibility;
        }
        friend auto operator<<(std::ostream& os, const Class &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(name, data, inherit_members, default_visibility);
        }
    };
    struct Namespace : DeclarationLevel {
        std::string name;
        Declarations declarations;
        friend bool operator==(const Namespace &a, const Namespace &b);
        bool operator!=(const Namespace& other) const {
            return !(*this == other);
        }
        friend bool operator<(const Namespace &a, const Namespace &b);
        friend auto operator<<(std::ostream& os, const Namespace &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(name,declarations);
        }
    };
    struct Function : DeclarationLevel {
        Type type;
        std::string name;
        stdu::vector<std::pair<Type, std::string>> parameters;
        Statements statements;
        stdu::vector<std::string> template_parameters;
        bool override = false;
        bool is_static = false;
        friend bool operator==(const Function &a, const Function &b);
        bool operator!=(const Function& other) const {
            return !(*this == other);
        }
        friend bool operator<(const Function &a, const Function &b);
        friend auto operator<<(std::ostream& os, const Function &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(type, name, parameters, statements, template_parameters);
        }
    };
    struct TypeAlias : DeclarationLevel {
        std::string name;
        Type type;
        bool operator==(const TypeAlias& other) const {
            return name == other.name && type == other.type;
        }
        bool operator!=(const TypeAlias& other) const {
            return !(*this == other);
        }
        bool operator<(const TypeAlias& other) const {
            if (name != other.name) return name < other.name;
            else return type < other.type;
        }
        friend auto operator<<(std::ostream& os, const TypeAlias &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(name, type);
        }
    };
    struct Enum : DeclarationLevel {
        std::string name;
        stdu::vector<std::string> value;

        template<typename ...Args>
        Enum(std::string name, std::string first_value, Args&& ...args) : name(name), value(std::vector<std::string>{first_value, std::forward<Args>(args)...}) {}
        Enum(std::string name, stdu::vector<std::string> value) : name(name), value(std::move(value)) {}
        Enum(std::string name) : name(name) {}
        bool operator==(const Enum& other) const {
            return value == other.value;
        }
        bool operator!=(const Enum& other) const {
            return !(*this == other);
        }
        bool operator<(const Enum& other) const {
            if (name != other.name) return name < other.name;
            else return value < other.value;
        }
        friend auto operator<<(std::ostream& os, const Enum &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(name, value);
        }
    };
    struct Variable : ExpressionValueLevel {
        std::string name;
        Type type;
        Expression value;
        stdu::vector<Expression> set; // language agnostic way to produce static values in non-initializer
        bool is_static = false;
        friend bool operator==(const Variable &a, const Variable &b);
        bool operator!=(const Variable& other) const {
            return !(*this == other);
        }
        friend bool operator<(const Variable &a, const Variable &b);
        friend auto operator<<(std::ostream& os, const Variable &c) -> std::ostream&;
        // Variable(const std::string &&n, const RValue &&v) : name(std::move(n)), value(std::move(v)) {}
        // Variable(const std::string &n, const RValue &v) : name(n), value(v)) {}
        auto empty() { return name.empty(); }

    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(name, type, value);
        }
    };
    struct Declaration : DeclarationsLevel {
        using promote_to = Declarations;
        std::variant<Class, ForwardDeclaredClass, Namespace, Function, TypeAlias, Enum, Variable> value;

        Declaration() {};
        template<typename T>
        requires std::is_constructible_v<decltype(value), T>
        Declaration(T value) : value(value) {}

        bool operator==(const Declaration& other) const {
            return value == other.value;
        }
        bool operator!=(const Declaration& other) const {
            return value != other.value;
        }
        bool operator<(const Declaration& other) const {
            return value < other.value;
        }
        friend auto operator<<(std::ostream& os, const Declaration &c) -> std::ostream&;
        // Is functions
        bool isClass() const { return std::holds_alternative<Class>(value); }
        bool isForwardDeclaredClass() const { return std::holds_alternative<ForwardDeclaredClass>(value); }
        bool isNamespace() const { return std::holds_alternative<Namespace>(value); }
        bool isFunction() const { return std::holds_alternative<Function>(value); }
        bool isTypeAlias() const { return std::holds_alternative<TypeAlias>(value); }
        bool isEnum() const { return std::holds_alternative<Enum>(value); }
        bool isVariable() const { return std::holds_alternative<Variable>(value); }

        // Get functions
        auto& getClass() { return std::get<Class>(value); }
        const auto& getClass() const { return std::get<Class>(value); }

        auto& getForwardDeclaredClass() { return std::get<ForwardDeclaredClass>(value); }
        const auto& getForwardDeclaredClass() const { return std::get<ForwardDeclaredClass>(value); }

        auto& getNamespace() { return std::get<Namespace>(value); }
        const auto& getNamespace() const { return std::get<Namespace>(value); }

        auto& getFunction() { return std::get<Function>(value); }
        const auto& getFunction() const { return std::get<Function>(value); }

        auto& getTypeAlias() { return std::get<TypeAlias>(value); }
        const auto& getTypeAlias() const { return std::get<TypeAlias>(value); }

        auto& getEnum() { return std::get<Enum>(value); }
        const auto& getEnum() const { return std::get<Enum>(value); }

        auto& getVariable() { return std::get<Variable>(value); }
        const auto& getVariable() const { return std::get<Variable>(value); }
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(value);
        }
    };
    inline Declarations::Declarations() = default;
    inline Declarations::Declarations(const Declarations&) = default;
    inline Declarations::Declarations(Declarations&&) noexcept = default;
    inline Declarations::Declarations(const stdu::vector<Declaration>& v) : vector(v) {}
    inline Declarations::Declarations(stdu::vector<Declaration>&& v) : vector(std::move(v)) {}
    inline Declarations& Declarations::operator=(const Declarations&) = default;
    inline Declarations& Declarations::operator=(Declarations&&) noexcept = default;
    inline Declarations::~Declarations() = default;
    struct Break : ExpressionValueLevel {
        bool operator==(const Break&) const { return true; }
        bool operator!=(const Break&) const { return false; }
        bool operator<(const Break&) const { return false; }
        friend auto operator<<(std::ostream& os, const Break &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie();
        }
    };
    struct Continue : ExpressionValueLevel {
        bool operator==(const Continue&) const { return true; }
        bool operator!=(const Continue&) const { return false; }
        bool operator<(const Continue&) const { return false; }
        friend auto operator<<(std::ostream& os, const Continue &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie();
        }
    };
    struct Return : ExpressionValueLevel {
        Expression value;
        friend bool operator==(const Return& a, const Return &b);
        bool operator!=(const Return& ret) const { return !(*this == ret); }
        friend bool operator<(const Return &a, const Return &b);
        friend auto operator<<(std::ostream& os, const Return &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(value);
        }
    };
    struct StringCompare : ExpressionValueLevel {
        String str;
        bool is_string;

        bool operator==(const StringCompare& s) const { return str == s.str && is_string == s.is_string;  }
        bool operator!=(const StringCompare& s) const { return !(*this == s); }
        bool operator<(const StringCompare& other) const {
            if (str != other.str) return str < other.str;
            else return is_string < other.is_string;
        }
        friend auto operator<<(std::ostream& os, const StringCompare &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(str, is_string);
        }
    };
    struct VariableAssignment : ExpressionValueLevel {
        std::variant<Symbol, StorageSymbol> name;
        OperatorType type = OperatorType::Assign;
        Expression value;

        friend bool operator==(const VariableAssignment& a, const VariableAssignment &b);
        bool operator!=(const VariableAssignment& v) const { return !(*this == v); }
        friend bool operator<(const VariableAssignment &a, const VariableAssignment &b);
        friend auto operator<<(std::ostream& os, const VariableAssignment &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(name, type, value);
        }
    };
    // possibly will be removed *
    struct CounterIncreament : ExpressionValueLevel {
        bool operator==(const CounterIncreament& c) const { return true; }
        bool operator!=(const CounterIncreament& c) const { return !(*this == c); }
        bool operator<(const CounterIncreament& other) const { return false; }
        friend auto operator<<(std::ostream& os, const CounterIncreament &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie();
        }
    };
    struct CounterIncreamentByLength : ExpressionValueLevel {
        bool operator==(const CounterIncreamentByLength& n) const { return name == n.name; }
        bool operator!=(const CounterIncreamentByLength& n) const { return !(*this == n); }
        bool operator<(const CounterIncreamentByLength& other) const { return name < other.name; }
        friend auto operator<<(std::ostream& os, const CounterIncreamentByLength &c) -> std::ostream&;
        std::string name;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(name);
        }
    };
    struct ResetPosCounter : ExpressionValueLevel {
        bool operator==(const ResetPosCounter& c) const { return true; }
        bool operator!=(const ResetPosCounter& c) const { return !(*this == c); }
        bool operator<(const ResetPosCounter& c) const { return !(*this == c); }
        friend auto operator<<(std::ostream& os, const ResetPosCounter &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie();
        }
    };
    struct PushPosCounter : ExpressionValueLevel {
        bool operator==(const PushPosCounter& c) const { return name == c.name; }
        bool operator!=(const PushPosCounter& c) const { return !(*this == c); }
        bool operator<(const PushPosCounter& other) const { return name < other.name; }
        friend auto operator<<(std::ostream& os, const PushPosCounter &c) -> std::ostream&;
        std::string name;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(name);
        }
    };
    struct PopPosCounter : ExpressionValueLevel {
        bool operator==(const PopPosCounter& c) const { return true; }
        bool operator!=(const PopPosCounter& c) const { return !(*this == c); }
        bool operator<(const PopPosCounter& c) const { return false; }
        friend auto operator<<(std::ostream& os, const PopPosCounter &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie();
        }
    };
    //                         *
    struct SkipSpaces : ExpressionValueLevel {
        bool operator==(const SkipSpaces& s) const { return isToken == s.isToken; }
        bool operator!=(const SkipSpaces& s) const { return !(*this == s); }
        bool operator<(const SkipSpaces& other) const { return isToken < other.isToken; }
        friend auto operator<<(std::ostream& os, const SkipSpaces &c) -> std::ostream&;
        bool isToken;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(isToken);
        }
    };
    struct DfaLookup : ExpressionValueLevel {
        bool operator==(const DfaLookup &d) const { return output_name == d.output_name && dfa_count == d.dfa_count; }
        bool operator!=(const DfaLookup &d) const { return !(*this == d); }
        bool operator<(const DfaLookup &other) const {
            if (output_name != other.output_name) return output_name < other.output_name;
            else return dfa_count < other.dfa_count;
        }
        friend auto operator<<(std::ostream& os, const DfaLookup &c) -> std::ostream&;
        std::size_t dfa_count;
        LangAPI::Type return_type;
        std::string output_name;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(dfa_count, output_name);
        }
    };
    struct ReportError : ExpressionValueLevel {
        bool operator==(const ReportError& e) const { return message == e.message; }
        bool operator!=(const ReportError& e) const { return !(*this == e); }
        bool operator<(const ReportError& other) const { return message < other.message; }
        friend auto operator<<(std::ostream& os, const ReportError &c) -> std::ostream&;
        std::string message;

    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(message);
        }
    };
    struct Lambda : ExpressionValueLevel {
        decltype(Function::parameters) parameters;
        Statements statements;
        friend bool operator==(const Lambda &a, const Lambda &b);
        friend bool operator!=(const Lambda &a, const Lambda &b) { return !(a == b); }
        friend bool operator<(const Lambda &a, const Lambda &b);
        friend auto operator<<(std::ostream& os, const Lambda &c) -> std::ostream&;
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(parameters, statements);
        }
    };
    struct ExpressionValue : ExpressionLevel {
        std::variant<
            std::monostate,
            EmptyInitializer,
            RValue,
            ExpressionElement,
            FunctionCall,
            IspaLibFunctionCall,
            StringCompare,
            Return,
            Break,
            Continue,
            VariableAssignment,
            CounterIncreament,
            CounterIncreamentByLength,
            ResetPosCounter,
            PushPosCounter,
            PopPosCounter,
            SkipSpaces,
            DfaLookup,
            ReportError,
            Lambda
        > value;
        ExpressionValue() {};
        template<typename T>
        requires std::is_constructible_v<decltype(value), T>
        ExpressionValue(const T &t) : value(t) {}
        template<typename T>
        requires std::is_constructible_v<decltype(value), std::decay_t<T>>
        ExpressionValue(T &&t) : value(std::forward<T>(t)) {}
        bool operator==(const ExpressionValue& other) const {
            return value == other.value;
        }
        bool operator!=(const ExpressionValue& other) const {
            return !(*this == other);
        }
        bool operator<(const ExpressionValue& other) const {
            return std::visit([&](const auto &v1, const auto &v2) {
                if constexpr (std::is_same_v<std::decay_t<decltype(v1)>, std::monostate>) {
                    if constexpr (std::is_same_v<std::decay_t<decltype(v2)>, std::monostate>) {
                        return false;
                    } else return true;
                } else if constexpr (std::is_same_v<std::decay_t<decltype(v2)>, std::monostate>) return false;
                else if constexpr (std::is_same_v<std::decay_t<decltype(v1)>, std::decay_t<decltype(v2)>>) return v1 < v2;
                else return value.index() < other.value.index();
            }, value, other.value);
        }
        friend auto operator<<(std::ostream& os, const ExpressionValue &c) -> std::ostream&;
        // ======= isXXX functions =======
        bool empty() const { return std::holds_alternative<std::monostate>(value); }
        bool isEmptyInitializer() const { return std::holds_alternative<EmptyInitializer>(value); }
        bool isRvalue() const { return std::holds_alternative<RValue>(value); }
        bool isExpressionElement() const { return std::holds_alternative<ExpressionElement>(value); }
        bool isFunctionCall() const { return std::holds_alternative<FunctionCall>(value); }
        bool isIspaLibFunctionCall() const { return std::holds_alternative<IspaLibFunctionCall>(value); }
        bool isStringCompare() const { return std::holds_alternative<StringCompare>(value); }
        bool isReturn() const { return std::holds_alternative<Return>(value); }
        bool isBreak() const { return std::holds_alternative<Break>(value); }
        bool isContinue() const { return std::holds_alternative<Continue>(value); }
        bool isVariableAssignment() const { return std::holds_alternative<VariableAssignment>(value); }
        bool isCounterIncreament() const { return std::holds_alternative<CounterIncreament>(value); }
        bool isCounterIncreamentByLength() const { return std::holds_alternative<CounterIncreamentByLength>(value); }
        bool isResetPosCounter() const { return std::holds_alternative<ResetPosCounter>(value); }
        bool isPushPosCounter() const { return std::holds_alternative<PushPosCounter>(value); }
        bool isPopPosCounter() const { return std::holds_alternative<PopPosCounter>(value); }
        bool isSkipSpaces() const { return std::holds_alternative<SkipSpaces>(value); }
        bool isDfaLookup() const { return std::holds_alternative<DfaLookup>(value); }
        bool isReportError() const { return std::holds_alternative<ReportError>(value); }
        bool isLambda() const { return std::holds_alternative<Lambda>(value); }

        // ======= getXXX functions =======
        RValue& getRValue() { return std::get<RValue>(value); }
        ExpressionElement& getExpressionElement() { return std::get<ExpressionElement>(value); }
        FunctionCall& getFunctionCall() { return std::get<FunctionCall>(value); }
        IspaLibFunctionCall& getIspaLibFunctionCall() { return std::get<IspaLibFunctionCall>(value); }
        StringCompare& getStringCompare() { return std::get<StringCompare>(value); }
        Return& getReturn() { return std::get<Return>(value); }
        Break& getBreak() { return std::get<Break>(value); }
        Continue& getContinue() { return std::get<Continue>(value); }
        VariableAssignment& getVariableAssignment() { return std::get<VariableAssignment>(value); }
        CounterIncreament& getCounterIncreament() { return std::get<CounterIncreament>(value); }
        CounterIncreamentByLength& getCounterIncreamentByLength() { return std::get<CounterIncreamentByLength>(value); }
        ResetPosCounter& getResetPosCounter() { return std::get<ResetPosCounter>(value); }
        PushPosCounter& getPushPosCounter() { return std::get<PushPosCounter>(value); }
        PopPosCounter& getPopPosCounter() { return std::get<PopPosCounter>(value); }
        SkipSpaces& getSkipSpaces() { return std::get<SkipSpaces>(value); }
        DfaLookup& getDfaLookup() { return std::get<DfaLookup>(value); }
        ReportError& getReportError() { return std::get<ReportError>(value); }
        Lambda& getLambda() { return std::get<Lambda>(value); }

        // const versions
        const RValue& getRValue() const { return std::get<RValue>(value); }
        const ExpressionElement& getExpressionElement() const { return std::get<ExpressionElement>(value); }
        const FunctionCall& getFunctionCall() const { return std::get<FunctionCall>(value); }
        const IspaLibFunctionCall& getIspaLibFunctionCall() const { return std::get<IspaLibFunctionCall>(value); }
        const StringCompare& getStringCompare() const { return std::get<StringCompare>(value); }
        const Return& getReturn() const { return std::get<Return>(value); }
        const Break& getBreak() const { return std::get<Break>(value); }
        const Continue& getContinue() const { return std::get<Continue>(value); }
        const VariableAssignment& getVariableAssignment() const { return std::get<VariableAssignment>(value); }
        const CounterIncreament& getCounterIncreament() const { return std::get<CounterIncreament>(value); }
        const CounterIncreamentByLength& getCounterIncreamentByLength() const { return std::get<CounterIncreamentByLength>(value); }
        const ResetPosCounter& getResetPosCounter() const { return std::get<ResetPosCounter>(value); }
        const PushPosCounter& getPushPosCounter() const { return std::get<PushPosCounter>(value); }
        const PopPosCounter& getPopPosCounter() const { return std::get<PopPosCounter>(value); }
        const SkipSpaces& getSkipSpaces() const { return std::get<SkipSpaces>(value); }
        const DfaLookup& getDfaLookup() const { return std::get<DfaLookup>(value); }
        const ReportError& getDfaReportError() const { return std::get<ReportError>(value); }
        const Lambda& getLambda() const { return std::get<Lambda>(value); }

        auto type() const { return static_cast<ExpressionValueType>(value.index()); }

    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(value);
        }
    };
    inline Expression::Expression() = default;
    inline Expression::Expression(const Expression&) = default;
    inline Expression::Expression(Expression&&) noexcept = default;
    inline Expression::Expression(const ExpressionValue& v) : vector{v} {}
    inline Expression::Expression(ExpressionValue&& v) : vector{std::move(v)} {}
    inline Expression::Expression(const stdu::vector<ExpressionValue>& v) : vector(v) {}
    inline Expression::Expression(stdu::vector<ExpressionValue>&& v) : vector(std::move(v)) {}
    inline Expression::Expression(std::initializer_list<ExpressionValue> v) : vector(v) {}
    inline Expression& Expression::operator=(const Expression&) = default;
    inline Expression& Expression::operator=(Expression&&) noexcept = default;
    inline Expression::~Expression() = default;
    // made for if, while, do-while
    struct ConditionalElement {
        friend bool operator==(const ConditionalElement &a, const ConditionalElement &b);
        bool operator!=(const ConditionalElement& other) const {
            return !(*this == other);
        }
        friend bool operator<(const ConditionalElement &a, const ConditionalElement &b);
        Expression expr;
        Statements stmt;

    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(expr, stmt);
        }
    };
    struct If : ConditionalElement, StatementLevel {
        Statements else_stmt;
        friend bool operator==(const If &a, const If &b);
        bool operator!=(const If& other) const {
            return !(*this == other);
        }
        friend bool operator<(const If &a, const If &b);
        friend auto operator<<(std::ostream& os, const If &c) -> std::ostream&;
        If(const Expression &e, const Statements &s, const Statements &else_stmt) : ConditionalElement {.expr = e, .stmt = s}, else_stmt(else_stmt) {}
        If(const Expression &e, const Statements &s) : ConditionalElement {.expr = e, .stmt = s} {}
        If(const Expression &e) : ConditionalElement {.expr = e} {}
        If(Expression &&e, Statements &&s, Statements &&else_stmt) : ConditionalElement {.expr = std::move(e), .stmt = std::move(s)}, else_stmt(else_stmt) {}
        If(Expression &&e, Statements &&s) : ConditionalElement {.expr = std::move(e), .stmt = std::move(s)} {}
        If(Expression &&e) : ConditionalElement {.expr = std::move(e)} {}
        If() {};

    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(else_stmt);
        }
    };
    struct While : ConditionalElement, StatementLevel {
        While(const Expression &e, const Statements &s) : ConditionalElement {.expr = e, .stmt = s} {}
        While(const Expression &e) : ConditionalElement {.expr = e} {}
        While() {}
        friend auto operator<<(std::ostream& os, const While &c) -> std::ostream&;
    };
    struct DoWhile : ConditionalElement, StatementLevel {
        DoWhile(const Expression &e, const Statements &s) : ConditionalElement {.expr = e, .stmt = s} {}
        DoWhile(const Expression &e) : ConditionalElement {.expr = e} {}
        DoWhile() {}
        friend auto operator<<(std::ostream& os, const DoWhile &c) -> std::ostream&;
    };
    struct Switch : StatementLevel {
        friend bool operator==(const Switch &a, const Switch &b);
        friend bool operator!=(const Switch &a, const Switch &b) { return !(a == b); }
        friend bool operator<(const Switch &a, const Switch &b);
        friend auto operator<<(std::ostream& os, const Switch &c) -> std::ostream&;
        Expression expression;
        stdu::vector<std::pair<RValue, Statements>> cases;

    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(expression, cases);
        }
    };
    struct Throw : StatementLevel {
        friend bool operator==(const Throw &a, const Throw &b);
        friend bool operator!=(const Throw &a, const Throw &b) { return !(a == b); }
        friend bool operator<(const Throw &a, const Throw &b);
        friend auto operator<<(std::ostream& os, const Throw &c) -> std::ostream&;
        Expression throw_value;

    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(throw_value);
        }
    };
    struct Statement : StatementsLevel {
        std::variant<std::monostate, Variable, If, While, DoWhile, Switch, Expression, Throw> value;
        Statement() = default;
        template<typename T>
        requires std::is_constructible_v<decltype(value), std::decay_t<T>>
        Statement(T &&value) : value(std::forward<T>(value)) {}
        Statement(const ConditionalElement &value) : value(While {value.expr, value.stmt}) {}
        bool operator==(const Statement& other) const { return value == other.value; }
        friend bool operator!=(const Statement &a, const Statement &b) { return !(a == b); }
        bool operator<(const Statement& other) const { return value < other.value; }
        friend auto operator<<(std::ostream& os, const Statement &c) -> std::ostream&;
        // ======= isXXX functions =======
        bool isVariable() const { return std::holds_alternative<Variable>(value); }
        bool isIf() const { return std::holds_alternative<If>(value); }
        bool isWhile() const { return std::holds_alternative<While>(value); }
        bool isDoWhile() const { return std::holds_alternative<DoWhile>(value); }
        bool isSwitch() const { return std::holds_alternative<Switch>(value); }
        bool isExpression() const { return std::holds_alternative<Expression>(value); }
        bool isThrow() const { return std::holds_alternative<Throw>(value); }

        // ======= getXXX functions =======
        Variable& getVariable() { return std::get<Variable>(value); }
        If& getIf() { return std::get<If>(value); }
        While& getWhile() { return std::get<While>(value); }
        DoWhile& getDoWhile() { return std::get<DoWhile>(value); }
        ConditionalElement &getWhileOrDoWhile() { return std::holds_alternative<While>(value) ? static_cast<ConditionalElement &>(std::get<While>(value)) : static_cast<ConditionalElement &>(std::get<DoWhile>(value)); }
        Switch& getSwitch() { return std::get<Switch>(value); }
        Expression& getExpression() { return std::get<Expression>(value); }
        Throw& getThrow() { return std::get<Throw>(value); }
        // ======= const versions =======
        const Variable& getVariable() const { return std::get<Variable>(value); }
        const If& getIf() const { return std::get<If>(value); }
        const While& getWhile() const { return std::get<While>(value); }
        const DoWhile& getDoWhile() const { return std::get<DoWhile>(value); }
        const ConditionalElement& getWhileOrDoWhile() const { return std::holds_alternative<While>(value) ? static_cast<const ConditionalElement &>(std::get<While>(value)) : static_cast<const ConditionalElement &>(std::get<DoWhile>(value)); }
        const Switch& getSwitch() const { return std::get<Switch>(value); }
        const Expression& getExpression() const { return std::get<Expression>(value); }
        const Throw& getThrow() const { return std::get<Throw>(value); }

        static auto createStatements(const RValue &value) -> stdu::vector<Statement>;

    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(value);
        }
    };
    inline Statements::Statements() = default;
    inline Statements::Statements(const Statements&) = default;
    inline Statements::Statements(Statements&&) noexcept = default;
    inline Statements::Statements(const Statement& v) : vector{v} {}
    inline Statements::Statements(Statement&& v) : vector{std::move(v)} {}
    inline Statements::Statements(const stdu::vector<Statement>& v) : vector(v) {}
    inline Statements::Statements(stdu::vector<Statement>&& v) : vector(std::move(v)) {}
    inline Statements::Statements(std::initializer_list<Statement> v) : vector(v) {}
    inline Statements& Statements::operator=(const Statements&) = default;
    inline Statements& Statements::operator=(Statements&&) noexcept = default;
    inline Statements::~Statements() = default;

    inline auto FunctionCall::members() const {
        return std::tie(name, template_parameters, args);
    }
}

