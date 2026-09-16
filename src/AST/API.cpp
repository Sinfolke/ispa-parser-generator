module AST.API;
import std;

namespace AST {
    template <typename T>
    static std::ostream& print_shared_ptr(std::ostream& os, const std::shared_ptr<T>& ptr) {
        if (!ptr) return os << "null";
        return os << *ptr;
    }
    std::size_t String::count_strlen(const std::string &str) {
        std::size_t count = 0;
        for (auto it = str.begin(); it != str.end(); it++) {
            if (*it == '\\') {
                if (it + 1 == str.end())
                    break;
                it++;
                count++;
                continue;
            }
            count++;
        }
        return count;
    }
    std::string String::format_str(std::string str) {
        std::string res;
        for (auto &c : str) {
            if (c == '"') {
                res += "\\\"";
                continue;
            }
            res += c;
        }
        return res;
    }
    std::size_t String::count_strlen() const {
        return count_strlen(value);
    }
    std::string String::format_str() const {
        return format_str(value);
    }
    bool Number::hasDec() const {
        return dec.empty();
    }
    bool Number::hasSign() const {
        return sign == '\0';
    };
    std::string Number::getFull() const {
        std::string full;
        if (sign != '\0')
            full += sign;
        full += main + '.' + dec;
        return full;
    }
    double Number::getFullNumber() const {
        return std::stod(main + '.' + dec);
    };
    unsigned Number::getMain() const {
        return std::stoul(main);
    }
    unsigned Number::getDecimal() const {
        return std::stoul(dec);
    }
    bool Boolean::getBoolean() const {
        return value == "true";
    }
    bool rvalue::isString() const {
        return std::holds_alternative<String>(value);
    }
    bool rvalue::isNumber() const {
        return std::holds_alternative<Number>(value);
    }
    bool rvalue::isBoolean() const {
        return std::holds_alternative<Boolean>(value);
    }
    bool rvalue::isArray() const {
        return std::holds_alternative<Array>(value);
    }
    bool rvalue::isObject() const {
        return std::holds_alternative<Object>(value);
    };
    bool rvalue::isAt() const {
        return std::holds_alternative<At>(value);
    }
    bool rvalue::isID() const {
        return std::holds_alternative<ID>(value);
    }
    String& rvalue::getString() {
        return std::get<String>(value);
    }
    Number& rvalue::getNumber() {
        return std::get<Number>(value);
    }
    Boolean& rvalue::getBoolean() {
        return std::get<Boolean>(value);
    }
    Array& rvalue::getArray() {
        return std::get<Array>(value);
    }
    Object& rvalue::getObject() {
        return std::get<Object>(value);
    }
    ID& rvalue::getID() {
        return std::get<ID>(value);
    }
    const String& rvalue::getString() const {
        return std::get<String>(value);
    }

    const Number& rvalue::getNumber() const {
        return std::get<Number>(value);
    }

    const Boolean& rvalue::getBoolean() const {
        return std::get<Boolean>(value);
    }

    const Array& rvalue::getArray() const {
        return std::get<Array>(value);
    }

    const Object& rvalue::getObject() const {
        return std::get<Object>(value);
    }

    const ID& rvalue::getID() const {
        return std::get<ID>(value);
    }
    bool CllExprValue::isGroup() const {
        return std::holds_alternative<std::shared_ptr<CllExprGroup>>(value);
    }
    bool CllExprValue::isVariable() const {
        return std::holds_alternative<std::shared_ptr<CllVariableMention>>(value);
    }
    bool CllExprValue::isFunctionCall() const {
        return std::holds_alternative<CllFunctionCall>(value);
    }
    bool CllExprValue::isMethodCall() const {
        return std::holds_alternative<CllMethodCall>(value);
    }
    bool CllExprValue::isrvalue() const {
        return std::holds_alternative<rvalue>(value);
    }
    CllExprGroup& CllExprValue::getGroup() {
        return *std::get<std::shared_ptr<CllExprGroup>>(value);
    }
    CllVariableMention& CllExprValue::getVariable() {
        return *std::get<std::shared_ptr<CllVariableMention>>(value);
    }
    CllFunctionCall& CllExprValue::getFunctionCall() {
        return std::get<CllFunctionCall>(value);
    }
    CllMethodCall& CllExprValue::getMethodCall() {
        return std::get<CllMethodCall>(value);
    }
    rvalue& CllExprValue::getrvalue() {
        return std::get<rvalue>(value);
    };
    const CllExprGroup& CllExprValue::getGroup() const {
        return *std::get<std::shared_ptr<CllExprGroup>>(value);
    }

    const CllVariableMention& CllExprValue::getVariable() const {
        return *std::get<std::shared_ptr<CllVariableMention>>(value);
    }

    const CllFunctionCall& CllExprValue::getFunctionCall() const {
        return std::get<CllFunctionCall>(value);
    }
    const CllMethodCall& CllExprValue::getMethodCall() const {
        return std::get<CllMethodCall>(value);
    }
    const rvalue& CllExprValue::getrvalue() const {
        return std::get<rvalue>(value);
    }
    bool AST::Cll::isVar() const {
        return std::holds_alternative<CllVar>(value);
    }

    bool AST::Cll::isIf() const {
        return std::holds_alternative<CllIf>(value);
    }

    bool AST::Cll::isExpr() const {
        return std::holds_alternative<CllExpr>(value);
    }
    AST::CllVar &AST::Cll::getVar() {
        return std::get<CllVar>(value);
    }

    AST::CllIf &AST::Cll::getIf() {
        return std::get<CllIf>(value);
    }

    const AST::CllExpr &AST::Cll::getExpr() const {
        return std::get<CllExpr>(value);
    }
    const AST::CllVar &AST::Cll::getVar() const {
        return std::get<CllVar>(value);
    }

    const AST::CllIf &AST::Cll::getIf() const {
        return std::get<CllIf>(value);
    }

    AST::CllExpr &AST::Cll::getExpr() {
        return std::get<CllExpr>(value);
    }
    std::string::iterator RuleMemberHex::begin() {
        return hex_chars.begin();
    }
    std::string::iterator RuleMemberHex::end() {
        return hex_chars.end();
    }
    std::string::const_iterator RuleMemberHex::begin() const{
        return hex_chars.begin();
    }
    std::string::const_iterator RuleMemberHex::end() const {
        return hex_chars.end();
    }
    std::string::iterator RuleMemberBin::begin() {
        return bin_chars.begin();
    }
    std::string::iterator RuleMemberBin::end() {
        return bin_chars.end();
    }
    std::string::const_iterator RuleMemberBin::begin() const {
        return bin_chars.begin();
    }
    std::string::const_iterator RuleMemberBin::end() const {
        return bin_chars.end();
    }
    void RulePrefix::clear() {
        name = "";
        is_key_value = false;
    }
    bool RulePrefix::empty() const {
        return name.empty() && !is_key_value;
    }
    bool RuleMember::isString() const {
        return std::holds_alternative<String>(value);
    }
    bool RuleMember::isName() const {
        return std::holds_alternative<RuleMemberName>(value);
    }
    bool RuleMember::isGroup() const {
        return std::holds_alternative<RuleMemberGroup>(value);
    }
    bool RuleMember::isOp() const {
        return std::holds_alternative<RuleMemberOp>(value);
    }
    bool RuleMember::isCsequence() const {
        return std::holds_alternative<RuleMemberCsequence>(value);
    }
    bool RuleMember::isAny() const {
        return std::holds_alternative<RuleMemberAny>(value);
    }
    bool RuleMember::isNospace() const {
        return std::holds_alternative<RuleMemberNospace>(value);
    }
    bool RuleMember::isEscaped() const {
        return std::holds_alternative<RuleMemberEscaped>(value);
    }
    bool RuleMember::isHex() const {
        return std::holds_alternative<RuleMemberHex>(value);
    }
    bool RuleMember::isBin() const {
        return std::holds_alternative<RuleMemberBin>(value);
    }
    bool RuleMember::isCll() const {
        return std::holds_alternative<Cll>(value);
    }
    bool RuleMember::emptyQuantifier() const {
        return quantifier == '\0';
    }
    bool RuleMember::empty() const {
        return std::holds_alternative<std::monostate>(value);
    }
    String& RuleMember::getString() {
        return std::get<String>(value);
    }
    RuleMemberName& RuleMember::getName() {
        return std::get<RuleMemberName>(value);
    }
    RuleMemberGroup &RuleMember::getGroup() {
        return std::get<RuleMemberGroup>(value);
    }
    RuleMemberOp& RuleMember::getOp() {
        return std::get<RuleMemberOp>(value);
    }
    RuleMemberCsequence& RuleMember::getCsequence() {
        return std::get<RuleMemberCsequence>(value);
    }
    RuleMemberEscaped& RuleMember::getEscaped() {
        return std::get<RuleMemberEscaped>(value);
    }
    RuleMemberHex& RuleMember::getHex() {
        return std::get<RuleMemberHex>(value);
    }
    RuleMemberBin& RuleMember::getBin() {
        return std::get<RuleMemberBin>(value);
    }
    Cll& RuleMember::getCll() {
        return std::get<Cll>(value);
    }
    const String& RuleMember::getString() const {
        return std::get<String>(value);
    }

    const RuleMemberName& RuleMember::getName() const {
        return std::get<RuleMemberName>(value);
    }

    const RuleMemberGroup& RuleMember::getGroup() const {
        return std::get<RuleMemberGroup>(value);
    }

    const RuleMemberOp& RuleMember::getOp() const {
        return std::get<RuleMemberOp>(value);
    }

    const RuleMemberCsequence& RuleMember::getCsequence() const {
        return std::get<RuleMemberCsequence>(value);
    }

    const RuleMemberEscaped& RuleMember::getEscaped() const {
        return std::get<RuleMemberEscaped>(value);
    }

    const RuleMemberHex& RuleMember::getHex() const {
        return std::get<RuleMemberHex>(value);
    }

    const RuleMemberBin& RuleMember::getBin() const {
        return std::get<RuleMemberBin>(value);
    }

    const Cll& RuleMember::getCll() const {
        return std::get<Cll>(value);
    }

    std::unordered_map<std::string, CllExpr>::iterator RegularDataBlockWKeys::begin() {
        return value.begin();
    }
    std::unordered_map<std::string, CllExpr>::iterator RegularDataBlockWKeys::end() {
        return value.end();
    }
    std::unordered_map<std::string, CllExpr>::const_iterator RegularDataBlockWKeys::begin() const {
        return value.begin();
    }
    std::unordered_map<std::string, CllExpr>::const_iterator RegularDataBlockWKeys::end() const {
        return value.end();
    }
    CllExpr& RegularDataBlockWKeys::operator[](const std::string &name) {
        return value[name];
    }
    bool TemplatedDataBlock::empty() const {
        return names.empty();
    }

    stdu::vector<std::string>::iterator TemplatedDataBlock::begin() {
        return names.begin();
    }

    stdu::vector<std::string>::iterator TemplatedDataBlock::end() {
        return names.end();
    }
    stdu::vector<std::string>::const_iterator TemplatedDataBlock::begin() const {
        return names.cbegin();
    }

    stdu::vector<std::string>::const_iterator TemplatedDataBlock::end() const {
        return names.cend();
    }

    bool DataBlock::isRegularDataBlock() const {
        return std::holds_alternative<RegularDataBlock>(value) || std::holds_alternative<RegularDataBlockWKeys>(value);
    }

    bool DataBlock::isWithKeys() const {
        return std::holds_alternative<RegularDataBlockWKeys>(value);
    }

    bool DataBlock::isTemplatedDataBlock() const {
        return std::holds_alternative<TemplatedDataBlock>(value);
    }
    bool DataBlock::empty() const {
        return std::holds_alternative<std::monostate>(value);
    }
    RegularDataBlock& DataBlock::getRegDataBlock() {
        return std::get<RegularDataBlock>(value);
    }

    RegularDataBlockWKeys& DataBlock::getRegDataBlockWKeys() {
        return std::get<RegularDataBlockWKeys>(value);
    }

    TemplatedDataBlock& DataBlock::getTemplatedDataBlock() {
        return std::get<TemplatedDataBlock>(value);
    }
    const RegularDataBlock& DataBlock::getRegDataBlock() const {
        return std::get<RegularDataBlock>(value);
    }

    const RegularDataBlockWKeys& DataBlock::getRegDataBlockWKeys() const {
        return std::get<RegularDataBlockWKeys>(value);
    }

    const TemplatedDataBlock& DataBlock::getTemplatedDataBlock() const {
        return std::get<TemplatedDataBlock>(value);
    }
    // operator== for rules

    bool operator==(const String &first, const String &second) {
        return first.value == second.value;
    }
    // Comparison operator for Number
    bool operator==(const AST::Number &lhs, const AST::Number &rhs) {
        return lhs.sign == rhs.sign &&
            lhs.main == rhs.main &&
            lhs.dec == rhs.dec;
    }

    // Comparison operator for Boolean
    bool operator==(const AST::Boolean &lhs, const AST::Boolean &rhs) {
        return lhs.value == rhs.value;
    }

    // Comparison operator for Array
    bool operator==(const AST::Array &lhs, const AST::Array &rhs) {
        return lhs.value == rhs.value;
    }


    // Comparison operator for Object
    bool operator==(const AST::Object &lhs, const AST::Object &rhs) {
        return lhs.value == rhs.value;
    }
    // Comparison operator for ID
    bool operator==(const AST::At &lhs, const AST::At &rhs) {
        return true;
    }
    // Comparison operator for ID
    bool operator==(const AST::ID &lhs, const AST::ID &rhs) {
        return lhs.value == rhs.value;
    }

    // Comparison operator for rvalue
    bool operator==(const rvalue& lhs, const rvalue& rhs) {
        return std::visit([](const auto& a, const auto& b) -> bool {
            using A = std::decay_t<decltype(a)>;
            using B = std::decay_t<decltype(b)>;
            if constexpr (std::is_same_v<A, B>) {
                return a == b;
            } else {
                return false;
            }
        }, lhs.value, rhs.value);
    }
    // Comparison operator for CllCompareOp
    bool operator==(const AST::CllCompareOp &lhs, const AST::CllCompareOp &rhs) {
        return lhs.op == rhs.op;
    }

    // Comparison operator for CllLogicalOp
    bool operator==(const AST::CllLogicalOp &lhs, const AST::CllLogicalOp &rhs) {
        return lhs.isAnd == rhs.isAnd;
    }

    bool operator==(const CllType &lhs, const CllType &rhs) {
        return lhs.type == rhs.type && lhs.templ == rhs.templ;
    }

    bool operator==(const CllFunctionArguments &lhs, const CllFunctionArguments &rhs) {
        return lhs.expr == rhs.expr;
    }

    bool operator==(const CllFunctionParameters &lhs, const CllFunctionParameters &rhs) {
        return lhs.names == rhs.names;
    }

    // Comparison operator for CllFunctionCall
    bool operator==(const AST::CllFunctionCall &lhs, const AST::CllFunctionCall &rhs) {
        return lhs.name == rhs.name &&
            lhs.body == rhs.body;
    }

    // Comparison operator for CllFunctionDecl
    bool operator==(const AST::CllFunctionDecl &lhs, const AST::CllFunctionDecl &rhs) {
        return lhs.name == rhs.name &&
            lhs.body == rhs.body;
    }
    bool operator==(const AST::CllMethodCall &lhs, const AST::CllMethodCall &rhs) {
        return lhs.name == rhs.name && lhs.body == rhs.body;
    }
    bool operator==(const CllExpr &lhs, const CllExpr &rhs) {
        return lhs.value == rhs.value;
    }

    bool operator==(const CllExprLogical &lhs, const CllExprLogical &rhs) {
        return lhs.value == rhs.value && lhs.rights == rhs.rights;
    }

    bool operator==(const CllExprCompare &lhs, const CllExprCompare &rhs) {
        return lhs.value == rhs.value && lhs.rights == rhs.rights;
    }

    bool operator==(const CllExprAddition &lhs, const CllExprAddition &rhs) {
        return lhs.value == rhs.value && lhs.rights == rhs.rights;
    }

    bool operator==(const CllExprTerm &lhs, const CllExprTerm &rhs) {
        return lhs.value == rhs.value && lhs.rights == rhs.rights;
    }

    bool operator==(const CllExprValue &lhs, const CllExprValue &rhs) {
        return std::visit([] (auto &&a, auto &&b) -> bool {
            using A = std::decay_t<decltype(a)>;
            using B = std::decay_t<decltype(b)>;
            if constexpr (std::is_same_v<A, B>) {
                return a == b;
            } else {
                return false;
            }
        }, lhs.value, rhs.value);
    }

    bool operator==(const CllVariableMention &lhs, const CllVariableMention &rhs) {
        return lhs.pre_increament == rhs.pre_increament &&
               lhs.post_increament == rhs.post_increament &&
               lhs.name == rhs.name &&
               lhs.braceExpression == rhs.braceExpression;
    }

    bool operator==(const CllIf &lhs, const CllIf &rhs) {
        return lhs.expr == rhs.expr && lhs.stmt == rhs.stmt;
    }

    bool operator==(const CllVar &lhs, const CllVar &rhs) {
        return lhs.type == rhs.type && lhs.name == rhs.name && lhs.op == rhs.op && lhs.value == rhs.value;
    }

    bool operator==(const CllLoopWhile &lhs, const CllLoopWhile &rhs) {
        return lhs.expr == rhs.expr && lhs.stmt == rhs.stmt;
    }

    bool operator==(const DataBlock &lhs, const DataBlock &rhs) {
        return std::visit([](auto &&a, auto &&b) -> bool {
            if constexpr(!std::is_same_v<decltype(a), decltype(b)>) {
                return false;
            } else {
                return a == b;
            }
        }, lhs.value, rhs.value);
    }

    bool operator==(const RegularDataBlockWKeys &lhs, const RegularDataBlockWKeys &rhs) {
        return lhs.value == rhs.value;
    }

    bool operator==(const TemplatedDataBlock &lhs, const TemplatedDataBlock &rhs) {
        return lhs.names == rhs.names;
    }

    bool operator==(const Rule &lhs, const Rule &rhs) {
        return lhs.rule_members == rhs.rule_members && lhs.data_block == rhs.data_block;
    }

    bool operator==(const RulePrefix &first, const RulePrefix &second) {
        return first.is_key_value == second.is_key_value && first.name == second.name;
    }

    bool operator==(const Cll &first, const Cll &second) {
        return std::visit([](auto &&first, auto &&second) -> bool {
            if constexpr (std::is_same_v<decltype(first), decltype(second)>) {
                return first == second;
            } else {
                return false;
            }
        }, first.value, second.value);
    }

    bool operator==(const RuleMemberName &first, const RuleMemberName &second) {
        return first.name == second.name;
    }

    bool operator==(const RuleMemberGroup &first, const RuleMemberGroup &second) {
        return first.values == second.values;
    }

    bool operator==(const RuleMemberOp &first, const RuleMemberOp &second) {
        return first.options == second.options;
    }

    bool operator==(const RuleMemberCsequence &first, const RuleMemberCsequence &second) {
        // Compare independently of character position in csequence
        if (first.negative != second.negative ||
            first.characters.size() != second.characters.size() ||
            first.escaped.size() != second.escaped.size() ||
            first.diapasons.size() != second.diapasons.size()) {
            return false;
        }
        for (std::size_t i = 0; i < first.characters.size(); i++) {
            if (std::find(second.characters.begin(), second.characters.end(), first.characters[i]) == second.characters.end()) {
                return false;
            }
        }
        for (std::size_t i = 0; i < first.escaped.size(); i++) {
            if (std::find(second.escaped.begin(), second.escaped.end(), first.escaped[i]) == second.escaped.end()) {
                return false;
            }
        }
        for (std::size_t i = 0; i < first.diapasons.size(); i++) {
            if (std::find_if(second.diapasons.begin(), second.diapasons.end(),
                             [&first, i](const std::pair<char, char> &pair) {
                                 return pair.first == first.diapasons[i].first && pair.second == first.diapasons[i].second;
                             }) == second.diapasons.end()) {
                return false;
            }
        }
        return true;
    }

    bool operator==(const RuleMemberAny &first, const RuleMemberAny &second) {
        return true; // Always equal
    }

    bool operator==(const RuleMemberNospace &first, const RuleMemberNospace &second) {
        return true; // Always equal
    }

    bool operator==(const RuleMemberEscaped &first, const RuleMemberEscaped &second) {
        return first.c == second.c;
    }

    bool operator==(const RuleMemberHex &first, const RuleMemberHex &second) {
        return first.hex_chars == second.hex_chars;
    }

    bool operator==(const RuleMemberBin &first, const RuleMemberBin &second) {
        return first.bin_chars == second.bin_chars;
    }

    bool operator==(const RuleMember &first, const RuleMember &second) {
        return std::visit([](auto&& lhs, auto&& rhs) -> bool {
            using L = std::decay_t<decltype(lhs)>;
            using R = std::decay_t<decltype(rhs)>;
            if constexpr (std::is_same_v<L, R> && requires (L a, R b) { a == b; }) {
                return lhs == rhs;
            } else {
                return false;
            }
        }, first.value, second.value);
    }
    bool operator==(const RuleMemberKey &first, const RuleMemberKey &second) {
        return first.base == second.base;
    }
    std::ostream& operator<<(std::ostream& os, const RulePrefix& p) {
        if (p.empty())
            return os;
        os << (p.is_key_value ? "@ " : "") << p.name;
        return os;
    }

    namespace {
        auto print_grammar_string(std::ostream& os, std::string_view value) -> std::ostream& {
            os << '"';
            for (const unsigned char c : value) {
                switch (c) {
                    case '\\': os << "\\\\"; break;
                    case '"': os << "\\\""; break;
                    case '\n': os << "\\n"; break;
                    case '\r': os << "\\r"; break;
                    case '\t': os << "\\t"; break;
                    default:
                        if (c < 0x20 || c == 0x7f) {
                            os << "\\x" << std::hex << std::uppercase << static_cast<int>(c) << std::dec << std::nouppercase;
                        } else {
                            os << static_cast<char>(c);
                        }
                        break;
                }
            }
            return os << '"';
        }

        auto print_grammar_csequence(std::ostream& os, const RuleMemberCsequence& cs) -> std::ostream& {
            os << (cs.negative ? "[^" : "[");
            auto emit_char = [&](char c) {
                switch (c) {
                    case '\\': os << "\\\\"; break;
                    case ']': os << "\\]"; break;
                    case '^': os << "\\^"; break;
                    case '-': os << "\\-"; break;
                    default: os << c; break;
                }
            };
            for (char c : cs.characters) emit_char(c);
            for (char e : cs.escaped) {
                os << "\\";
                emit_char(e);
            }
            for (const auto &[from, to] : cs.diapasons) {
                emit_char(from);
                os << '-';
                emit_char(to);
            }
            return os << ']';
        }
    }

    std::ostream& operator<<(std::ostream& os, const RuleMemberName& n) {
        for (std::size_t i = 0; i < n.name.size(); ++i) {
            os << n.name[i];
            if (i + 1 < n.name.size()) os << "::";
        }
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const RuleMemberGroup& g) {
        os << "(";
        for (std::size_t i = 0; i < g.values.size(); ++i) {
            print_shared_ptr(os, g.values[i]);
            if (i + 1 < g.values.size()) os << ' ';
        }
        os << ")";
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const RuleMemberOp& o) {
        os << "(";
        for (std::size_t i = 0; i < o.options.size(); ++i) {
            print_shared_ptr(os, o.options[i]);
            if (i + 1 < o.options.size()) os << " | ";
        }
        os << ")";
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const RuleMemberCsequence& cs) {
        return print_grammar_csequence(os, cs);
    }

    std::ostream& operator<<(std::ostream& os, const RuleMemberAny&) {
        return os << '.';
    }

    std::ostream& operator<<(std::ostream& os, const RuleMemberNospace&) {
        return os << "\\s0";
    }

    std::ostream& operator<<(std::ostream& os, const RuleMemberEscaped& e) {
        return os << '\\' << e.c;
    }

    std::ostream& operator<<(std::ostream& os, const RuleMemberHex& h) {
        return os << "0x" << h.hex_chars;
    }

    std::ostream& operator<<(std::ostream& os, const RuleMemberBin& b) {
        return os << "0b" << b.bin_chars;
    }
    std::ostream& operator<<(std::ostream& os, const String& str) {
        return print_grammar_string(os, str.value);
    }
    std::ostream& operator<<(std::ostream& os, const Cll& str) {
        os << "<CLL>";
        return os;
    }
    std::ostream& operator<<(std::ostream& os, const RuleMember& rm) {
        os << rm.prefix << " ";

        std::visit([&os](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (!std::is_same_v<T, std::monostate>)
                os << arg;
            else
                os << "Empty";
        }, rm.value);
        if (rm.quantifier)
            os << rm.quantifier << ";";
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const RegularDataBlockWKeys& block) {
        os << "DataBlockWithKeys {\n";
        for (const auto& [key, val] : block.value) {
            os << "  " << key << ": [CllExpr]\n"; // Placeholder
        }
        os << "}";
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const TemplatedDataBlock& block) {
        os << "TemplatedDataBlock(";
        for (std::size_t i = 0; i < block.names.size(); ++i) {
            os << block.names[i];
            if (i + 1 < block.names.size()) os << ", ";
        }
        os << ")";
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const DataBlock& db) {
        std::visit([&os](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                os << "EmptyDataBlock";
            } else if constexpr (std::is_same_v<T, RegularDataBlock>) {
                os << "[RegularDataBlock: CllExpr]";
            } else if constexpr (std::is_same_v<T, RegularDataBlockWKeys>) {
                os << arg;
            } else if constexpr (std::is_same_v<T, TemplatedDataBlock>) {
                os << arg;
            }
        }, db.value);
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const Rule& r) {
        os << "Rule {\n";
        for (const auto& m : r.rule_members)
            os << "  ", print_shared_ptr(os, m), os << "\n";
        os << "  Data: " << r.data_block << "\n";
        os << "}";
        return os;
    }

    bool RuleMember::fullCompare(const RuleMember &second) {
        return *this == second && quantifier == second.quantifier && prefix == second.prefix && isAutoGenerated == second.isAutoGenerated && isInline == second.isInline;
    }
    CllExpr make_expr_from_value(const AST::CllExprValue& val) {
        AST::CllExprTerm term;
        term.value = val;

        AST::CllExprAddition addition;
        addition.value = term;

        AST::CllExprCompare compare;
        compare.value = addition;

        AST::CllExprLogical logical;
        logical.value = compare;

        AST::CllExpr expr;
        expr.value = logical;
    
        return expr;
    }
};
