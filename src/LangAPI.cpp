module LangAPI;

namespace LangAPI {
    template<typename Param>
    inline void printTemplateParameters(std::ostream &os, stdu::vector<Param> params) {
        os << '<';
        for (const auto &p : params) {
            std::visit([&os](const auto &v) { os << v; }, p);
        }
        os << '>';
    }
    template<typename Param>
    inline void printTemplateParametersShared(std::ostream &os, stdu::vector<Param> params) {
        os << '<';
        for (const auto &p : params) {
            std::visit([&os](const auto &v) { os << *v; }, p);
        }
        os << '>';
    }
    inline void printExpressionArgs(std::ostream &os, stdu::vector<Expression> args) {
        bool first = true;
        for (const auto &arg : args) {
            os << arg;
            if (!first) {
                os << ", ";
            }
            first = false;
        }
    }
    auto operator<<(std::ostream &os, const Char &c) -> std::ostream& {
        return os << (c.escaped ? "\\" : "") << c.value;
    }
    auto operator<<(std::ostream &os, const Int &i) -> std::ostream& {
        return os << i.value;
    }
    auto operator<<(std::ostream &os, const Bool &b) -> std::ostream& {
        return os << b.value;
    }
    auto operator<<(std::ostream &os, const Float &f) -> std::ostream& {
        return os << f.value;
    }
    auto operator<<(std::ostream &os, const String &s) -> std::ostream& {
        return os << '"' << s.value << '"';
    }

    auto operator<(const Declarations& a, const Declarations& b) -> bool {
        return a.size() < b.size();
    }
    auto operator<(const Statements& a, const Statements& b) -> bool {
        return a.size() < b.size();
    }
    auto operator<(const Expression& a, const Expression& b) -> bool {
        return a.size() < b.size();
    }
    auto operator<<(std::ostream &os, ExpressionElement e) -> std::ostream& {
        switch (e) {
            case ExpressionElement::GroupOpen: os << "GroupOpen"; break;
            case ExpressionElement::GroupClose: os << "GroupClose"; break;
            case ExpressionElement::SquareBraceOpen: os << "SquareBraceOpen"; break;
            case ExpressionElement::SquareBraceClose: os << "SquareBraceClose"; break;
            case ExpressionElement::And: os << "And"; break;
            case ExpressionElement::Or: os << "Or"; break;
            case ExpressionElement::Not: os << "Not"; break;
            case ExpressionElement::Equal: os << "Equal"; break;
            case ExpressionElement::NotEqual: os << "NotEqual"; break;
            case ExpressionElement::Higher: os << "Higher"; break;
            case ExpressionElement::Lower: os << "Lower"; break;
            case ExpressionElement::HigherOrEqual: os << "HigherOrEqual"; break;
            case ExpressionElement::LowerOrEqual: os << "LowerOrEqual"; break;
            case ExpressionElement::Add: os << "Add"; break;
            case ExpressionElement::Minus: os << "Minus"; break;
            case ExpressionElement::Multiply: os << "Multiply"; break;
            case ExpressionElement::Divide: os << "Divide"; break;
            case ExpressionElement::Modulo: os << "Modulo"; break;
            case ExpressionElement::PlusPlus: os << "PlusPlus"; break;
            case ExpressionElement::MinusMinus: os << "MinusMinus"; break;
        }
        return os;
    }

    auto operator<<(std::ostream &os, OperatorType op) -> std::ostream& {
        switch (op) {
            case OperatorType::Assign: os << "Assign"; break;
            case OperatorType::Add: os << "Add"; break;
            case OperatorType::Minus: os << "Minus"; break;
            case OperatorType::Multiply: os << "Multiply"; break;
            case OperatorType::Divide: os << "Divide"; break;
            case OperatorType::Modulo: os << "Modulo"; break;
        }
        return os;
    }

    auto operator<<(std::ostream &os, ValueType v) -> std::ostream& {
        switch (v) {
            case ValueType::Undef: os << "Undef"; break;
            case ValueType::Void: os << "Void"; break;
            case ValueType::Char: os << "Char"; break;
            case ValueType::Int: os << "Int"; break;
            case ValueType::Bool: os << "Bool"; break;
            case ValueType::Float: os << "Float"; break;
            case ValueType::String: os << "String"; break;
            case ValueType::NonOwnedString: os << "NonOwnedString"; break;
            case ValueType::Array: os << "Array"; break;
            case ValueType::FixedSizeArray: os << "FixedSizeArray"; break;
            case ValueType::Map: os << "Map"; break;
            case ValueType::Symbol: os << "Symbol"; break;
            case ValueType::StorageSymbol: os << "StorageSymbol"; break;
            case ValueType::Inheritance: os << "Inheritance"; break;
            case ValueType::Token: os << "Token"; break;
            case ValueType::Rule: os << "Rule"; break;
            case ValueType::TokenResult: os << "TokenResult"; break;
            case ValueType::RuleResult: os << "RuleResult"; break;
            case ValueType::Span: os << "Span"; break;
            case ValueType::Variant: os << "Variant"; break;
            case ValueType::Box: os << "Box"; break;
            case ValueType::Any: os << "Any"; break;
            case ValueType::Const: os << "Const"; break;
        }
        return os;
    }

    auto operator<<(std::ostream &os, RValueType r) -> std::ostream& {
        switch (r) {
            case RValueType::Undef: os << "Undef"; break;
            case RValueType::Char: os << "Char"; break;
            case RValueType::Int: os << "Int"; break;
            case RValueType::Bool: os << "Bool"; break;
            case RValueType::Float: os << "Float"; break;
            case RValueType::String: os << "String"; break;
            case RValueType::Array: os << "Array"; break;
            case RValueType::FixedSizeArray: os << "FixedSizeArray"; break;
            case RValueType::Map: os << "Map"; break;
            case RValueType::Pos: os << "Pos"; break;
            case RValueType::Symbol: os << "Symbol"; break;
            case RValueType::IspaLibSymbol:os << "IspaLibSymbol"; break;
            case RValueType::StorageSymbol: os << "StorageSymbol"; break;
            case RValueType::Inheritance: os << "Inheritance"; break;
            case RValueType::IspaLibDfaTransition: os << "IspaLibDfaTransition"; break;
            case RValueType::IspaLibDfaSpanCharState: os << "IspaLibDfaSpanCharState"; break;
            case RValueType::IspaLibDfaSpanMultiTableState: os << "IspaLibDfaSpanMultiTableState"; break;
            case RValueType::IspaLibDfaEmptyState: os << "IspaLibDfaEmptyState"; break;
            case RValueType::IspaLibDfaSpan: os << "IspaLibDfaSpan"; break;
            case RValueType::Reference: os << "Reference"; break;
            case RValueType::Span: os << "Span"; break;
        }
        return os;
    }

    auto operator<<(std::ostream &os, ExpressionValueType e) -> std::ostream& {
        switch (e) {
            case ExpressionValueType::Empty: os << "Empty"; break;
            case ExpressionValueType::EmptyInitializer: os << "EmptyInitializer"; break;
            case ExpressionValueType::RValue: os << "RValue"; break;
            case ExpressionValueType::ExpressionElement: os << "ExpressionElement"; break;
            case ExpressionValueType::FunctionCall: os << "FunctionCall"; break;
            case ExpressionValueType::IspaLibFunctionCall: os << "IspaLibFunctionCall"; break;
            case ExpressionValueType::StringCompare: os << "StringCompare"; break;
            case ExpressionValueType::Return: os << "Return"; break;
            case ExpressionValueType::Break: os << "Break"; break;
            case ExpressionValueType::Continue: os << "Continue"; break;
            case ExpressionValueType::VariableAssignment: os << "VariableAssignment"; break;
            case ExpressionValueType::CounterIncreament: os << "CounterIncreament"; break;
            case ExpressionValueType::CounterIncreamentByLength: os << "CounterIncreamentByLength"; break;
            case ExpressionValueType::ResetPosCounter: os << "ResetPosCounter"; break;
            case ExpressionValueType::PushPosCounter: os << "PushPosCounter"; break;
            case ExpressionValueType::PopPosCounter: os << "PopPosCounter"; break;
            case ExpressionValueType::SkipSpaces: os << "SkipSpaces"; break;
            case ExpressionValueType::DfaLookup: os << "DfaLookup"; break;
            case ExpressionValueType::ReportError: os << "ReportError"; break;
            case ExpressionValueType::Lambda: os << "Lambda"; break;
        }
        return os;
    }

    auto operator<<(std::ostream &os, ArrayMethods m) -> std::ostream& {
        switch (m) {
            case ArrayMethods::Push: os << "Push"; break;
            case ArrayMethods::Pop: os << "Pop"; break;
        }
        return os;
    }

    auto operator<<(std::ostream &os, Visibility v) -> std::ostream& {
        switch (v) {
            case Visibility::Private: os << "Private"; break;
            case Visibility::Public: os << "Public"; break;
        }
        return os;
    }

    auto operator<<(std::ostream &os, Language l) -> std::ostream& {
        switch (l) {
            case Language::Cpp: os << "Cpp"; break;
        }
        return os;
    }

    auto operator<<(std::ostream &os, StdlibExports e) -> std::ostream& {
        switch (e) {
            case StdlibExports::Node: os << "Node"; break;
            case StdlibExports::MatchResult: os << "MatchResult"; break;
            case StdlibExports::Lexer: os << "Lexer"; break;
            case StdlibExports::Parser: os << "Parser"; break;
            case StdlibExports::LexerMakeTokenParameter: os << "LexerMakeTokenParameter"; break;
            case StdlibExports::DfaState: os << "DfaState"; break;
            case StdlibExports::DfaTable: os << "DfaTable"; break;
            case StdlibExports::DfaClassTable: os << "DfaClassTable"; break;
            case StdlibExports::DfaAcceptTable: os << "DfaAcceptTable"; break;
            case StdlibExports::DfaLRTable: os << "DfaLRTable"; break;
            case StdlibExports::DfaNullState: os << "DfaNullState"; break;
            case StdlibExports::ParserFunctionParameter: os << "ParserFunctionParameter"; break;
        }
        return os;
    }
    auto operator<<(std::ostream& os, const Declarations &expression) -> std::ostream& {
        for (const auto &declaration : expression) {
            os << declaration << "\n";
        }
        return os;
    }
    auto operator<<(std::ostream& os, const Statements &statements) -> std::ostream& {
        for (const auto &statement : statements) {
            os << statement << "\n";
        }
        return os;
    }
    auto operator<<(std::ostream& os, const Expression &expression) -> std::ostream& {
        for (const auto &expr : expression) {
            os << expr;
        }
        return os;
    }
    bool operator==(const Array &a, const Array &b) {
        return a.values == b.values && a.template_parameters == b.template_parameters;
    }
    bool operator<(const Array &a, const Array &b) {
        return a.values < b.values;
    }
    auto operator<<(std::ostream& os, const Array &arr) -> std::ostream& {
        printTemplateParameters(os, arr.template_parameters);
        printExpressionArgs(os, arr.values);
        return os;
    }

    bool operator==(const FixedSizeArray &a, const FixedSizeArray &other) {
        return a.values == other.values && a.template_parameters == other.template_parameters;
    }

    bool operator<(const FixedSizeArray &a, const FixedSizeArray &other) {
        if (a.values != other.values) return a.values < other.values;
        else if (a.template_parameters.size() != other.template_parameters.size()) return a.template_parameters.size() < other.template_parameters.size();
        else for (std::size_t i = 0; i < a.template_parameters.size(); ++i) {
            if (a.template_parameters[i].index() != other.template_parameters[i].index()) return a.template_parameters[i].index() < other.template_parameters[i].index();
            else if (std::holds_alternative<std::shared_ptr<Type>>(a.template_parameters[i])) {
                if (const auto &a_ = std::get<std::shared_ptr<Type>>(a.template_parameters[i]), &b_ = std::get<std::shared_ptr<Type>>(other.template_parameters[i]); a_ != b_) {
                    return a_ < b_;
                };
            } else if (std::holds_alternative<std::shared_ptr<RValue>>(a.template_parameters[i])) {
                if (const auto &a_ = std::get<std::shared_ptr<RValue>>(a.template_parameters[i]), &b_ = std::get<std::shared_ptr<RValue>>(other.template_parameters[i]); a_ != b_) {
                    return a_ < b_;
                }
            }
        }
        return false;
    }

    auto operator<<(std::ostream& os, const FixedSizeArray &arr)  -> std::ostream&{
        printTemplateParameters(os, arr.template_parameters);
        printExpressionArgs(os, arr.values);
        return os;
    }

    bool operator==(const Map &a, const Map &b) {
        return a.keys == b.keys && a.values == b.values && a.template_parameters == b.template_parameters;
    }

    bool operator<(const Map &a, const Map &b) {
        if (a.keys != b.keys) return a.keys < b.keys;
        else if (a.values != b.values) return a.values < b.values;
        else return a.template_parameters < b.template_parameters;
    }

    auto operator<<(std::ostream& os, const Map &map) -> std::ostream& {
        printTemplateParameters(os, map.template_parameters);
        os << '{';
        for (std::size_t i = 0; i < map.values.size(); ++i) {
            std::visit([&os](const auto &key) {
                os << key;
            }, map.keys[i]);
            os << ": " << map.values[i];
            if (i != map.values.size() - 1) os << ", ";
        }
        return os << '}';
    }
    auto operator<<(std::ostream& os, const Pos &pos)  -> std::ostream&{
        os << (pos.dereference ? "*" : "");
        if (pos.offset) {
            os << "(pos + " << pos.offset << ")";
        } else {
            os << "pos";
        }
        return os;
    }

    bool operator==(const ArrayMethodCall &a, const ArrayMethodCall &b) {
        return a.method == b.method && a.args == b.args;
    }

    bool operator<(const ArrayMethodCall &a, const ArrayMethodCall &b) {
        if (a.method != b.method) return a.method < b.method;
        else return a.args < b.args;
    }

    auto operator<<(std::ostream& os, const ArrayMethodCall &amc)  -> std::ostream&{
        os << amc.method;
        os << "(";
        printExpressionArgs(os, amc.args);
        os << ")";
        return os;
    }
    FunctionCall::~FunctionCall() = default;
    bool operator==(const FunctionCall &a, const FunctionCall &b) {
        return a.name == b.name && a.args == b.args;
    }

    bool operator<(const FunctionCall &a, const FunctionCall &b) {
        if (a.name != b.name) return a.name < b.name;
        else return a.args < b.args;
    }

    auto operator<<(std::ostream& os, const FunctionCall &fc)  -> std::ostream& {
        std::visit([&os](const auto &v) { os << v; }, fc.name);
        printExpressionArgs(os, fc.args);
        return os;
    }
    bool operator==(const Symbol &a, const Symbol &b) {
        return a.path == b.path;
    }
    bool operator<(const Symbol &a, const Symbol &b) {
        return a.path < b.path;
    }
    auto operator<<(std::ostream &os, const Symbol &obj) -> std::ostream& {
        bool first = true;
        for (const auto &part : obj.path) {
            std::visit([&os](const auto &v) { os << v; }, part);
            if (!first)
                os << "::";
            first = false;
        }
        return os;
    }
    bool operator==(const StorageOffset &a, const StorageOffset &b) {
        return a.offset == b.offset;
    }
    bool operator<(const StorageOffset &a, const StorageOffset &b) {
        return a.offset < b.offset;
    }
    auto operator<<(std::ostream &os, const StorageOffset &obj) -> std::ostream& {
        os << "[" << obj.offset << "]";
        return os;
    }
    bool operator==(const StorageSymbol &a, const StorageSymbol &b){
        return a.what == b.what && a.path == b.path;
    }
    bool operator<(const StorageSymbol &a, const StorageSymbol &b) {
        if (a.what != b.what) return a.what < b.what;
        else return a.path < b.path;
    }
    auto operator<<(std::ostream &os, const StorageSymbol &obj) -> std::ostream& {
        os << "(" << obj.what << ")";
        for (const auto &part : obj.path) {
            os << ".";
            std::visit([&os](const auto &v) { os << v; }, part);
        }
        return os;
    }
    bool operator==(const IspaLibSymbol &a, const IspaLibSymbol &b) {
        return a.exports == b.exports;
    }
    bool operator<(const IspaLibSymbol &a, const IspaLibSymbol &b) {
        if (a.exports != b.exports) return a.exports < b.exports;
        else return a.template_parameters < b.template_parameters;
    }
    auto operator<<(std::ostream &os, const IspaLibSymbol &obj) -> std::ostream& {
        os << obj.exports;
        printTemplateParametersShared(os, obj.template_parameters);
        return os;
    }
    bool operator==(const IspaLibFunctionCall &a, const IspaLibFunctionCall &b) {
        return a.symbol == b.symbol && a.args == b.args;
    }
    bool operator<(const IspaLibFunctionCall &a, const IspaLibFunctionCall &b) {
        if (a.symbol != b.symbol) return a.symbol < b.symbol;
        else return a.args < b.args;
    }
    auto operator<<(std::ostream &os, const IspaLibFunctionCall &obj) -> std::ostream& {
        os << obj.symbol;
        printExpressionArgs(os, obj.args);
        return os;
    }
    bool operator==(const Inheritance &a, const Inheritance &b) {
        return a.name == b.name && a.args == b.args;
    }
    bool operator<(const Inheritance &a, const Inheritance &b) {
        if (a.name != b.name) return a.name < b.name;
        else return a.args < b.args;
    }
    auto operator<<(std::ostream &os, const Inheritance &obj) -> std::ostream& {
        std::visit([&](const auto &el) {
            os << el;
        }, obj.name);
        printExpressionArgs(os, obj.args);
        return os;
    }

    auto operator<<(std::ostream &os, const Token &obj) -> std::ostream& {
        os << "[token]";
        return os;
    }

    auto operator<<(std::ostream &os, const Rule &obj) -> std::ostream& {
        os << "[Rule]";
        return os;
    }

    auto operator<<(std::ostream &os, const TokenResult &obj) -> std::ostream& {
        os << "[TokenResult]";
        return os;
    }

    auto operator<<(std::ostream &os, const RuleResult &obj) -> std::ostream& {
        os << "[RuleResult]";
        return os;
    }

    auto operator<<(std::ostream &os, const IspaLibDfaTransition &obj) -> std::ostream& {
        os << "[dfa_transition]";
        return os;
    }

    auto operator<<(std::ostream &os, const IspaLibDfaState &obj) -> std::ostream& {
        os << "[dfa_state]";
        return os;
    }

    auto operator<<(std::ostream &os, const IspaLibDfaSpanCharState &obj) -> std::ostream& {
        os << "[dfa_span_char_state]";
        return os;
    }

    auto operator<<(std::ostream &os, const IspaLibDfaSpanMultiTableState &obj) -> std::ostream& {
        os << "[dfa_span_multi_table_state]";
        return os;
    }

    auto operator<<(std::ostream &os, const IspaLibDfaEmptyState &obj) -> std::ostream& {
        os << "[dfa_empty_state]";
        return os;
    }

    auto operator<<(std::ostream &os, const Reference &obj) -> std::ostream& {
        os << obj.value;
        return os;
    }

    auto operator<<(std::ostream &os, const Span &obj) -> std::ostream& {
        os << "Span<" << obj.type << "> of " << obj.sym;
        return os;
    }
    auto operator<<(std::ostream &os, const EmptyInitializer &obj) -> std::ostream& {
        os << "{}";
        return os;
    }
    auto operator<<(std::ostream &os, const MakeTuple &obj) -> std::ostream& {
        os << "make_tuple(";
        for (const auto &expr : obj.args) {
            os << expr << ", ";
        }
        os << ")";
        return os;
    }
    auto operator<<(std::ostream &os, const GetVariant &obj) -> std::ostream& {
        os << "get<" << obj.type << ">(" << obj.sym << ")";
        return os;
    }
    auto operator<<(std::ostream &os, const CheckVariant &obj) -> std::ostream& {
        os << "holds_alternative<" << obj.type << ">(" << obj.sym << ")";
        return os;
    }
    auto operator<<(std::ostream &os, const CharToStringConstructor &obj) -> std::ostream& {
        os << "std::string(1, '" << obj.what << "')";
        return os;
    }
    auto operator<<(std::ostream &os, const IspaLibDfaSpan &obj) -> std::ostream& {
        os << "Span<" << obj.type << "> {" << obj.assing_name << "}";
        return os;
    }
    auto operator<<(std::ostream &os, const RValue &obj) -> std::ostream& {
        std::visit([&os](const auto &value) { if constexpr (!std::is_same_v<std::decay_t<decltype(value)>, std::monostate>) os << value; }, obj.get());
        return os;
    }

    auto operator<<(std::ostream &os, const Type &obj) -> std::ostream& {
        if (obj.isSymbol()) {
            bool first = true;
            for (const auto &t : obj.getSymbol().path) {
                if (std::holds_alternative<std::string>(t)) {
                    if (!first) os << "::";
                    os << std::get<std::string>(t);
                    first = false;
                } else {
                    os << std::get<FunctionCall>(t);
                }
            }
        } else if (obj.isValueType()) {
            os << '[' << obj.getValueType() << ']';
        } else if (obj.isIspaLibSymbol()) {
            os << obj.getIspaLibSymbol();
        }
        if (!obj.template_parameters.empty()) {
            os << "<";
            for (const auto &t : obj.template_parameters) {
                if (std::holds_alternative<Type>(t)) {
                    os << std::get<Type>(t) << ",";
                } else os << std::get<RValue>(t);
            }
            os << ">";
        }
        return os;
    }

    auto operator<<(std::ostream &os, const ForwardDeclaredClass &obj) -> std::ostream& {
        os << (obj.isStruct ? "struct" : "class") << ' ' << obj.name;
        return os;
    }

    auto operator<<(std::ostream &os, const Class &obj) -> std::ostream& {
        os << "class " << obj.name;
        if (!obj.inherit_members.empty()) {
            os << " : ";
            for (const auto &inherit : obj.inherit_members) {
                os << inherit.first << ' ';
                std::visit([&os](const auto &value) { os << value; }, inherit.second);
            }
        }
        os << "{\n";
        Visibility visibility = obj.default_visibility;
        for (const auto &member : obj.data) {
            if (visibility != member.second) {
                os << member.second << ":\n";
                visibility = member.second;
            }
            os << member.first << ";\n";
        }
        return os;
    }
    bool operator==(const Namespace &a, const Namespace &b) {
        return a.name == b.name && a.declarations == b.declarations;
    }
    bool operator<(const Namespace &a, const Namespace &b) {
        if (a.name != b.name) return a.name < b.name;
        else return a.declarations < b.declarations;
    }
    bool operator==(const MakeTuple &a, const MakeTuple &b) {
        return a.args == b.args;
    }
    bool operator<(const MakeTuple &a, const MakeTuple &b) {
        return a.args < b.args;
    }
    bool operator==(const GetVariant &a, const GetVariant &b) {
        return a.type == b.type && a.sym == b.sym;
    }
    bool operator<(const GetVariant &a, const GetVariant &b) {
        if (a.type != b.type) return a.type < b.type;
        return a.sym < b.sym;
    }
    bool operator==(const CheckVariant &a, const CheckVariant &b) {
        return a.type == b.type && a.sym == b.sym;
    }
    bool operator<(const CheckVariant &a, const CheckVariant &b) {
        if (a.type != b.type) return a.type < b.type;
        return a.sym < b.sym;
    }
    bool operator==(const CharToStringConstructor &a, const CharToStringConstructor &b) {
        return a.what == b.what;
    }
    bool operator<(const CharToStringConstructor &a, const CharToStringConstructor &b) {
        return a.what < b.what;
    }
    auto operator<<(std::ostream &os, const Namespace &obj) -> std::ostream& {
        os << "namespace " << obj.name << "{\n";
        os << obj.declarations;
        os << "}\n";
        return os;
    }
    bool operator==(const Function &a, const Function &b) {
        return a.type == b.type &&  a.name == b.name &&  a.parameters == b.parameters && a.statements == b.statements;
    }
    bool operator<(const Function &a, const Function &b) {
        if (a.type != b.type) return a.type < b.type;
        if (a.name != b.name) return a.name < b.name;
        else if (a.parameters != b.parameters) return a.parameters < b.parameters;
        else if (a.statements != b.statements) return a.statements < b.statements;
        else return a.template_parameters < b.template_parameters;
    }
    auto operator<<(std::ostream &os, const Function &obj) -> std::ostream& {
        if (!obj.template_parameters.empty()) {
            os << "template<";
            bool first = true;
            for (const auto &t : obj.template_parameters) {
                if (!first)
                    os << ", ";
                os << "typename " << t;
                first = false;
            }
            os << ">\n";
        }
        os << "auto " << obj.name;
        bool first = true;
        for (const auto &[type, name] : obj.parameters) {
            os << type << ' ' << name;
            if (!first) os << ", ";
            first = false;
        }
        os << " -> " << obj.type << " {\n";
        os << obj.statements;
        os << "}";
        return os;
    }

    auto operator<<(std::ostream &os, const TypeAlias &obj) -> std::ostream& {
        os << "using " << obj.name << " = " << obj.type;
        return os;
    }

    auto operator<<(std::ostream &os, const Enum &obj) -> std::ostream& {
        os << "enum class " << obj.name << "{\n";
        for (const auto &member : obj.value) {
            os << member << ",\n";
        }
        return os << "};\n";
    }

    bool operator==(const Variable &a, const Variable &b) {
        return a.name == b.name && a.type == b.type && a.value == b.value;
    }
    bool operator<(const Variable &a, const Variable &b) {
        if (a.name != b.name) return a.name < b.name;
        else if (a.type != b.type) return a.type < b.type;
        else return a.value < b.value;
    }
    auto operator<<(std::ostream &os, const Variable &obj) -> std::ostream& {
        os << obj.type << ' ' << obj.name << " = " << obj.value;
        return os;
    }

    auto operator<<(std::ostream &os, const Declaration &obj) -> std::ostream& {
        std::visit([&os](const auto &v) { os << v; }, obj.value);
        return os;
    }

    auto operator<<(std::ostream &os, const Break &obj) -> std::ostream& {
        return os << "break;";
    }

    auto operator<<(std::ostream &os, const Continue &obj) -> std::ostream& {
        return os << "continue;";
    }

    bool operator==(const Return &a, const Return &b) {
        return a.value == b.value;
    }
    bool operator<(const Return &a, const Return &b) {
        return a.value < b.value;
    }
    auto operator<<(std::ostream &os, const Return &obj) -> std::ostream& {
        return os << "return " << obj.value << ";";
    }

    auto operator<<(std::ostream &os, const StringCompare &obj) -> std::ostream& {
        return os << "[string_compare]";
    }
    bool operator==(const VariableAssignment &a, const VariableAssignment &b) {
        return a.name == b.name && a.type == b.type && a.value == b.value;
    }
    bool operator<(const VariableAssignment &a, const VariableAssignment &b) {
        if (a.name != b.name) return a.name < b.name;
        else if (a.type != b.type) return a.type < b.type;
        else return a.value < b.value;
    }
    auto operator<<(std::ostream &os, const VariableAssignment &obj) -> std::ostream& {
        std::visit([&os](const auto &v) { os << v; }, obj.name);
        return os << ' ' << obj.type << "=" << ' ' << obj.value << ';';
    }

    auto operator<<(std::ostream &os, const CounterIncreament &obj) -> std::ostream& {
        return os << "++pos";
    }

    auto operator<<(std::ostream &os, const CounterIncreamentByLength &obj) -> std::ostream& {
        return os << "pos += " << obj.name << ".lenght();";
    }

    auto operator<<(std::ostream &os, const ResetPosCounter &obj) -> std::ostream& {
        return os << "[reset_pos_counter]";
    }

    auto operator<<(std::ostream &os, const PushPosCounter &obj) -> std::ostream& {
        return os << "[push_pos_counter]: " << obj.name;
    }

    auto operator<<(std::ostream &os, const PopPosCounter &obj) -> std::ostream& {
        return os << "[pop_pos_counter]";
    }

    auto operator<<(std::ostream &os, const SkipSpaces &obj) -> std::ostream& {
        return os << "[skip_spaces]";
    }

    auto operator<<(std::ostream &os, const DfaLookup &obj) -> std::ostream& {
        return os << "[dfa_lookup]";
    }

    auto operator<<(std::ostream &os, const ReportError &obj) -> std::ostream& {
        return os << "[report_error]";
    }
    bool operator==(const Lambda &a, const Lambda &b) {
        return a.parameters == b.parameters && a.statements == b.statements;
    }
    bool operator<(const Lambda &a, const Lambda &b) {
        if (a.parameters != b.parameters) return a.parameters < b.parameters;
        else return a.statements < b.statements;
    }
    auto operator<<(std::ostream &os, const Lambda &obj) -> std::ostream& {
        os << "[]";
        bool first = true;
        for (const auto &[type, name] : obj.parameters) {
            os << type << ' ' << name;
            if (!first) os << ", ";
            first = false;
        }
        os << "{\n";
        os << obj.statements;
        os << "}\n";
        return os;
    }

    auto operator<<(std::ostream &os, const ExpressionValue &obj) -> std::ostream& {
        std::visit([&os](const auto &v) { os << v; }, obj.value);
        return os;
    }
    bool operator==(const ConditionalElement &a, const ConditionalElement &b) {
        return a.expr == b.expr && a.stmt == b.stmt;
    }
    bool operator<(const ConditionalElement &a, const ConditionalElement &b) {
        if (a.expr != b.expr) return a.expr < b.expr;
        else return a.stmt < b.stmt;
    }
    bool operator==(const If& a, const If &b) {
        return static_cast<ConditionalElement>(a) == static_cast<ConditionalElement>(b) && a.else_stmt == b.else_stmt;
    }
    bool operator<(const If& a, const If &b) {
        return static_cast<ConditionalElement>(a) < static_cast<ConditionalElement>(b) && a.else_stmt < b.else_stmt;
    }
    auto operator<<(std::ostream &os, const If &obj) -> std::ostream& {
        return os << "if (" << obj.expr << ") {\n" << obj.stmt << "} else {\n" << obj.else_stmt << "}";
    }

    auto operator<<(std::ostream &os, const While &obj) -> std::ostream& {
        return os << "while (" << obj.expr << ") {\n" << obj.stmt << "}";
    }

    auto operator<<(std::ostream &os, const DoWhile &obj) -> std::ostream& {
        return os << "do {\n" << obj.stmt << "} while (" << obj.expr << ");";
    }
    bool operator==(const Switch &a, const Switch &b) {
        return a.expression == b.expression && a.cases == b.cases;
    }
    bool operator<(const Switch &a, const Switch &b) {
        if (a.expression != b.expression) return a.expression < b.expression;
        else return a.cases < b.cases;
    }
    auto operator<<(std::ostream &os, const Switch &obj) -> std::ostream& {
        os << "switch (" << obj.expression << ") {\n";
        for (const auto &[value, statements] : obj.cases) {
            os << "case " << value << ": {\n" << statements << "}\n";
        }
        os << "}";
        return os;
    }
    bool operator==(const Throw &a, const Throw &b) {
        return a.throw_value == b.throw_value;
    }
    bool operator<(const Throw &a, const Throw &b) {
        return a.throw_value < b.throw_value;
    }
    auto operator<<(std::ostream& os, const Throw &c) -> std::ostream& {
        os << "throw " << c.throw_value << ";";
        return os;
    }
    auto operator<<(std::ostream &os, const Statement &obj) -> std::ostream& {
        std::visit([&os](const auto &v) {
            if constexpr (!std::is_same_v<std::decay_t<decltype(v)>, std::monostate>) {
                os << v;
            }
        }, obj.value);
        return os;
    }
}
