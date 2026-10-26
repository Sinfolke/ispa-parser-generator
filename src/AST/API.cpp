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
        present = false;
        present = false;
    }
    bool RulePrefix::empty() const {
        return !present;
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
    bool RuleMember::emptyQuantifier() const {
        return quantifier == '\0';
    }
    bool RuleMember::empty() const {
        return std::holds_alternative<std::monostate>(value) && features.empty();
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

    auto RegularDataBlockWKeys::begin() -> container_t::iterator {
        return value.begin();
    }
    auto RegularDataBlockWKeys::end() -> container_t::iterator {
        return value.end();
    }
    UAST::py::Expr& RegularDataBlockWKeys::operator[](const std::string &name) {
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
        return first.present == second.present && first.name == second.name;
    }

    bool operator==(const RuleMemberName &first, const RuleMemberName &second) {
        return first.name == second.name && first.templateArguments == second.templateArguments &&
            first.templateArgumentIsParameter == second.templateArgumentIsParameter &&
            first.templateParameter == second.templateParameter;
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
        }, first.value, second.value)
        && first.quantifier == second.quantifier
        && first.prefix == second.prefix
        && first.isAutoGenerated == second.isAutoGenerated
        && first.isInline == second.isInline
        && first.negative_match == second.negative_match;
    }
    bool operator==(const RuleMemberKey &first, const RuleMemberKey &second) {
        return first.base == second.base;
    }

    // Lexicographical ordering operators.
    //
    // The ordering follows the same semantic fields as operator==.
    // std::variant alternatives are ordered by their variant index first.

    namespace {
        template <typename T>
        concept LessComparable = requires(const T &a, const T &b) {
            { a < b } -> std::convertible_to<bool>;
        };

        template <typename T>
        bool value_less(const T &lhs, const T &rhs) {
            if constexpr (LessComparable<T>)
                return lhs < rhs;
            else
                return false;
        }
    }

    bool operator<(const String &lhs, const String &rhs) {
        return lhs.value < rhs.value;
    }
    bool operator<(const RegularDataBlockWKeys &lhs, const RegularDataBlockWKeys &rhs) {
        return lhs.value < rhs.value;
    }

    bool operator<(const TemplatedDataBlock &lhs, const TemplatedDataBlock &rhs) {
        return lhs.names < rhs.names;
    }

    bool operator<(const DataBlock &lhs, const DataBlock &rhs) {
        if (lhs.value.index() != rhs.value.index())
            return lhs.value.index() < rhs.value.index();

        return std::visit([](const auto &a, const auto &b) -> bool {
            using A = std::decay_t<decltype(a)>;
            using B = std::decay_t<decltype(b)>;
            if constexpr (std::is_same_v<A, B>)
                return value_less(a, b);
            else
                return false;
        }, lhs.value, rhs.value);
    }

    bool operator<(const Rule &lhs, const Rule &rhs) {
        return std::tie(lhs.rule_members, lhs.data_block) <
               std::tie(rhs.rule_members, rhs.data_block);
    }

    bool operator<(const RulePrefix &lhs, const RulePrefix &rhs) {
        return std::tie(lhs.present, lhs.name) <
               std::tie(rhs.present, rhs.name);
    }

    bool operator<(const RuleMemberName &lhs, const RuleMemberName &rhs) {
        return std::tie(lhs.name, lhs.templateArguments, lhs.templateArgumentIsParameter, lhs.templateParameter) <
            std::tie(rhs.name, rhs.templateArguments, rhs.templateArgumentIsParameter, rhs.templateParameter);
    }

    bool operator<(const RuleMemberGroup &lhs, const RuleMemberGroup &rhs) {
        return lhs.values < rhs.values;
    }

    bool operator<(const RuleMemberOp &lhs, const RuleMemberOp &rhs) {
        return lhs.options < rhs.options;
    }

    bool operator<(const RuleMemberCsequence &lhs, const RuleMemberCsequence &rhs) {
        if (lhs.negative != rhs.negative)
            return lhs.negative < rhs.negative;
        if (lhs.characters != rhs.characters)
            return lhs.characters < rhs.characters;
        if (lhs.escaped != rhs.escaped)
            return lhs.escaped < rhs.escaped;
        return lhs.diapasons < rhs.diapasons;
    }

    bool operator<(const RuleMemberAny &, const RuleMemberAny &) {
        return false;
    }

    bool operator<(const RuleMemberNospace &, const RuleMemberNospace &) {
        return false;
    }

    bool operator<(const RuleMemberEscaped &lhs, const RuleMemberEscaped &rhs) {
        return lhs.c < rhs.c;
    }

    bool operator<(const RuleMemberHex &lhs, const RuleMemberHex &rhs) {
        return lhs.hex_chars < rhs.hex_chars;
    }

    bool operator<(const RuleMemberBin &lhs, const RuleMemberBin &rhs) {
        return lhs.bin_chars < rhs.bin_chars;
    }

    bool operator<(const RuleMember &lhs, const RuleMember &rhs) {
        if (lhs.value.index() != rhs.value.index())
            return lhs.value.index() < rhs.value.index();

        if (std::visit([](const auto &a, const auto &b) -> bool {
            using A = std::decay_t<decltype(a)>;
            using B = std::decay_t<decltype(b)>;
            if constexpr (std::is_same_v<A, B>)
                return a != b;
            else
                return false;
        }, lhs.value, rhs.value)) {
            return std::visit([](const auto &a, const auto &b) -> bool {
                using A = std::decay_t<decltype(a)>;
                using B = std::decay_t<decltype(b)>;
                if constexpr (std::is_same_v<A, B>)
                    return value_less(a, b);
                else
                    return false;
            }, lhs.value, rhs.value);
        }

        return std::tie(
            lhs.quantifier,
            lhs.prefix,
            lhs.isAutoGenerated,
            lhs.isInline
        ) < std::tie(
            rhs.quantifier,
            rhs.prefix,
            rhs.isAutoGenerated,
            rhs.isInline
        );
    }

    bool operator<(const RuleMemberKey &lhs, const RuleMemberKey &rhs) {
        return lhs.base < rhs.base;
    }

    std::ostream& operator<<(std::ostream& os, const RulePrefix& p) {
        if (p.empty())
            return os;
        os << (p.present ? "@ " : "") << p.name;
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
        for (const auto& feature : rm.features) {
            if (const auto* counter = std::get_if<Counter>(&feature))
                os << " count(" << counter->kind << ':' << counter->bound << ')';
            if (const auto* registration = std::get_if<SharedCounterRegistration>(&feature))
                os << " =>" << registration->name;
        }
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
        if (r.external) os << "external " << r.external->cpp_type << ' ' << r.external->cpp_binding << ' ' << r.external->header << '\n';
        for (const auto& m : r.rule_members)
            os << "  ", print_shared_ptr(os, m), os << "\n";
        os << "  Data: " << r.data_block << "\n";
        os << "}";
        return os;
    }

    bool RuleMember::fullCompare(const RuleMember &second) {
        return *this == second;
    }
};

auto AST::captureType(const AST::RuleMember& member) -> AST::DataType {
        using Type = AST::DataType;
        using Kind = Type::Kind;
        Type type{Kind::String};
        if (member.isName()) {
            type.kind = member.getName().isTerminal() ? Kind::Token : Kind::Rule;
            type.name = member.getName().templateParameter.empty()
                ? member.getName().name : stdu::vector<std::string>{member.getName().templateParameter};
            type.isTemplateParameter = !member.getName().templateParameter.empty();
            type.templateArguments = member.getName().templateArguments;
            type.templateArgumentIsParameter = member.getName().templateArgumentIsParameter;
        } else if (member.isGroup() || member.isOp()) {
            const auto& children = member.isGroup() ? member.getGroup().values : member.getOp().options;
            for (const auto& child : children) {
                auto next = captureType(*child);
                if (!member.isOp() || std::find(type.parameters.begin(), type.parameters.end(), next) == type.parameters.end())
                    type.parameters.push_back(std::move(next));
            }
            if (type.parameters.empty()) return Type{Kind::String};
            if (type.parameters.size() == 1) type = Type(type.parameters.front());
            else if (std::all_of(type.parameters.begin(), type.parameters.end(), [](const Type& t) {
                return t.kind == Kind::Character || t.kind == Kind::String;
            })) type = Type{Kind::String};
            else type.kind = member.isOp() ? Kind::Variant : Kind::Tuple;
        } else if ((member.isString() && member.getString().value.size() == 1) || member.isAny() ||
                   (member.isCsequence() && (member.quantifier == '\0' || member.quantifier == '?'))) {
            type.kind = Kind::Character;
        }
        if ((member.quantifier == '*' || member.quantifier == '+') &&
            (member.isGroup() || member.isOp()) && type.kind == Kind::Character)
            type.kind = Kind::String;
        if ((member.quantifier == '*' || member.quantifier == '+') &&
            type.kind != Kind::Character && type.kind != Kind::String)
            type = Type{Kind::Array, {}, {type}};
        return type;
    }

namespace {
    void capturedTypes(const stdu::vector<std::shared_ptr<AST::RuleMember>>& members,
                       std::vector<AST::DataType>& types) {
        for (const auto& member : members) {
            if (!member->prefix.empty()) types.push_back(captureType(*member));
            else if (member->isGroup()) {
                std::vector<AST::DataType> nested;
                capturedTypes(member->getGroup().values, nested);
                for (auto& type : nested) {
                    if (member->quantifier == '*' || member->quantifier == '+')
                        type = AST::DataType{AST::DataType::Kind::Array, {}, {type}};
                    types.push_back(std::move(type));
                }
            }
            else if (member->isOp()) {
                std::optional<std::vector<AST::DataType>> first;
                for (const auto& option : member->getOp().options) {
                    std::vector<AST::DataType> current;
                    capturedTypes({option}, current);
                    if (!first) first = std::move(current);
                    else if (*first != current)
                        throw Error("AST: alternatives disagree on captured data types");
                }
                if (first) types.insert(types.end(), first->begin(), first->end());
            }
        }
    }

}

void AST::inferDataTypes(AST::Rule& rule) {
    auto& block = rule.data_block;
    if (block.empty()) return;
    block.type.reset();
    block.fields.clear();
    std::vector<AST::DataType> captures;
    capturedTypes(rule.rule_members, captures);
    if (block.isRegularDataBlock() && !block.isWithKeys()) {
        if (captures.size() != 1)
            throw Error("AST: regular data block requires exactly one captured member, got {}", captures.size());
        block.type = captures.front();
    } else if (block.isTemplatedDataBlock()) {
        const auto& names = block.getTemplatedDataBlock().names;
        if (names.size() != captures.size())
            throw Error("AST: data block declares {} fields but has {} captures", names.size(), captures.size());
        for (std::size_t i = 0; i < names.size(); ++i)
            block.fields.emplace_back(names[i], captures[i]);
    }
}
