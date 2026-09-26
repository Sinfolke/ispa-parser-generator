/**
 * C++ ispa std lib 1.2
 * It contains standard declarations and every C++ generated parser would link against it
*/
#pragma once
#ifndef _ISPA_STD_LIB_CPP
#define _ISPA_STD_LIB_CPP
#include <sstream>
#include <map>
#include <vector>
#include <deque>
#include <string>
#include <any>
#include <stdexcept>
#include <cctype>
#include <fstream>
#include <variant>
#include <optional>
#include <limits>
#include <functional>
#include <algorithm>
#include <iostream>
#include <set>
#include <unordered_set>

#include "old/ispastdlib.hpp"
#include "old/ispastdlib.hpp"
#include "old/ispastdlib.hpp"
#include "old/ispastdlib.hpp"
#include "old/ispastdlib.hpp"
#ifndef STRINGIFY
/**
 * @brief does #x
 * 
 */
#define STRINGIFY(x) #x
#endif
#ifndef TOSTRING
/**
 * @brief Converts to a string literal non-quoted value
 * 
 */
#define TOSTRING(x) STRINGIFY(x)
#endif

/*
    Library version is not being updated with each change for now and remains 1.0
*/

#define _ISC_STD_LIB_VER 1 // version of the library
#define _ISC_STD_LIB_SUBVER 0 // the subversion of the library
// this defines the minimum version of an output to have a compatibility with the library version
// for example if the update did only change the way some classes work but not their structure, it is compatible with the downer version.
#define _ISC_STD_LIB_BACKDOWN 1
#define _ISC_STD_LIB_BACKDOWN_SUBVER 0
#define _ISC_GITHUB "https://github.com/Sinfolke/ISC-parser"
#define ISC_STD_LIBMARK \
    "iscstdlibc++ " TOSTRING(_ISC_STD_LIB_VER) "." TOSTRING(_ISC_STD_LIB_SUBVER) ": "
#define _ISC_INTERNAL_ERROR_MARK \
        "NOTE: this exception is likely an internal error of the ISPA generator and is not on the user side.\n" \
        "Please, show the developer this issue (" _ISC_GITHUB ")\n" \
        "If this issue persist you may try to recompile the project by another version (try newer if your's is too old and vise versa)" \
//#define _ISC_STD_LIB_CPP
/**
 * @brief standard library for the ISPA-generated parser. Do not try to use it directly but use instead auto generated API
 * 
*/
namespace ISPA_STD {
/**
 * @brief An error thrown when you're trying to access some features required with tokens only
 *
 */
class Lexer_No_Tokens_exception : public std::exception {
    public:
    const char* what() const noexcept override {
        return ISC_STD_LIBMARK "Lexer_No_Tokens_exception: the tokenizator has no tokens but some operation required them";
    }
};
class Lexer_No_Input_exception : public std::exception {
    public:
    const char* what() const noexcept override {
        return ISC_STD_LIBMARK "Lexer_No_Input_exception: the tokenizator has no input provided but the operation required it";
    }
};
class Parser_No_Input_exception : public std::exception {
    public:
    const char* what() const noexcept override {
        return ISC_STD_LIBMARK "Parser_No_Input_exception: the parser has no input provided but the operation required it";
    }
};
class AdvancedDFA_exception : public std::exception {
    std::string message;
public:
    AdvancedDFA_exception(const char* message) {
        this->message = "ispastdlib Internal Error in Advanced DFA: ";
        this->message += message;
    }
    [[nodiscard]] auto what() const noexcept -> const char* override {
        return message.c_str();
    }
};
class node_exception : public std::exception {
    private:
        std::string mes;
        void fill(const std::string& method) {
            mes =
            ISC_STD_LIBMARK "node_exception: the capture data has not been provided but called method '";
            mes += method;
            mes +=
            "' required it.\n"
            _ISC_INTERNAL_ERROR_MARK "\n"
            "E.G: This issue is because you have an opportunity to have token or rule be empty without first initialisation.\n"
            "But their methods like ";
            mes += method;
            mes += " etc. require those properties."
            "When they are called but the object is still uninitialised you get this error\n";
        }
        void fill() {
            mes =
            ISC_STD_LIBMARK "node_exception: the capture data has not been provided but called method required it.\n"
            _ISC_INTERNAL_ERROR_MARK "\n"
            "E.G: This issue is because you have an opportunity to have token or rule be empty without first initialisation.\n"
            "But their methods like line etc. require those properties."
            "When they are called but the object is still uninitialised you get this error\n"
            ;
        }
    public:
        node_exception(const char* method) {  fill(method);  }
        node_exception() {  fill();  }
    const char* what() const noexcept override {
        return mes.c_str();
    }
};
// prior C++20 span type
template<typename T>
class Span {
    public:
        using value_type = T;
        using pointer = T*;
        using reference = T&;
        using const_reference = const T&;
        using size_type = std::size_t;

        Span() : data_(nullptr), size_(0) {}

        Span(pointer data, size_type size)
            : data_(data), size_(size) {}

        template<std::size_t N>
        Span(T (&arr)[N]) // from raw array
            : data_(arr), size_(N) {}
        // Robust converting constructor (like std::span)
        template<typename U,
                 typename = std::enable_if_t<
                     std::is_convertible_v<U (*)[], T (*)[]>
                 >
        >
        Span(const Span<U>& other) noexcept
            : data_(other.data()), size_(other.size()) {}
        [[nodiscard]] pointer data() const { return data_; }
        [[nodiscard]] size_type size() const { return size_; }
        [[nodiscard]] bool empty() const { return size_ == 0; }
        reference operator[](size_type index) {
            return data_[index];
        }
        const_reference operator[](size_type index) const {
            return data_[index];
        }
        reference at(size_type index) const {
            if (index >= size_) throw std::out_of_range("Span::at");
            return data_[index];
        }

        pointer begin() const { return data_; }
        pointer end() const { return data_ + size_; }
    private:
        pointer data_;
        size_type size_;
};
template<class TOKEN_T, const char* (*ToString)(TOKEN_T)>
class bad_get : public std::bad_cast {
    TOKEN_T required_name;
    TOKEN_T get_name;
    std::string message; // cache the message for `what()`
public:
    bad_get(TOKEN_T required_name, TOKEN_T get_name, std::string namespace_name = "<Parser>")
        : required_name(required_name), get_name(get_name) {
        message = std::string("Expected ") + namespace_name + "::get::" + ToString(required_name) + "(), but got " + namespace_name + "::get::" + ToString(get_name) + "()";
    }

    const char* what() const noexcept override {
        return message.c_str();
    }
};
template<typename T>
std::ostream& operator<<(std::ostream& os, const Span<T>& span) {
    os << "[";
    for (auto i = 0; i < span.size(); ++i) {
        if (i != 0) os << ", ";
        os << span[i];
    }
    os << "]";
    return os;
}
template<typename T, std::size_t N>
std::ostream& operator<<(std::ostream& os, const std::array<T, N>& arr) {
    os << "[";
    for (auto i = 0; i < arr.size(); ++i) {
        if (i != 0) os << ", ";
        os << arr[i];
    }
    os << "]";
    return os;
}
template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& arr) {
    os << "[";
    for (auto i = 0; i < arr.size(); ++i) {
        if (i != 0) os << ", ";
        os << arr[i];
    }
    os << "]";
    return os;
}
template<class EnumT, class DataStorageType, class = std::enable_if_t<std::is_class_v<DataStorageType>>>
class Node : public DataStorageType {
    std::size_t _startpos = std::string::npos;
    std::size_t _length = 0;
    std::size_t _line = 0;
    std::size_t _column = 0;
    const char* _start = nullptr;
    const char* _end = nullptr;
    EnumT _name = EnumT::NONE;
    bool _empty = false;
public:
    Node(const std::size_t startpos, const char* start, const char* end, std::size_t length, std::size_t line, std::size_t column, EnumT name)
        : _startpos(startpos), _start(start), _end(end), _length(length), _line(line), _column(column), _name(name) {}
    template<class  ...Args, class = std::enable_if_t<std::is_constructible_v<DataStorageType, Args...>>>
    Node(const std::size_t startpos, const char* start, const char* end, std::size_t length, std::size_t line, std::size_t column, EnumT name, Args&& ...args)
        : _startpos(startpos), _start(start), _end(end), _length(length), _line(line), _column(column), _name(name), DataStorageType(std::forward<Args>(args)...) {}
    Node(const std::size_t startpos, const char* start, const char* end, std::size_t length, std::size_t line, std::size_t column, EnumT name, DataStorageType data)
        : _startpos(startpos), _start(start), _end(end), _length(length), _line(line), _column(column), _name(name), DataStorageType(data) {}
    Node() : _empty(true) {}

    Node(const Node&) = default;
    Node(Node&&) noexcept = default;

    Node& operator=(const Node&) = default;
    Node& operator=(Node&&) noexcept = default;
    /**
     * @brief Get the end position based on startpos and length
     *
     * @return long long
     */
    std::size_t endpos() const {
        if (_startpos == std::string::npos || _end == nullptr || _start == nullptr)
            throw node_exception("endpos");
        return _startpos + (_end - _start);
    }
    /* clear rule */
    void clear() {
        _startpos = std::string::npos;
        _line = 0;
        _column = 0;
        _length = 0;
        _start = nullptr;
        _end = nullptr;
        _name = EnumT::NONE;
        _empty = true;
    }
    auto empty() const { return _empty; }
    auto startpos() const { return _startpos; }
    auto line() const { return _line; }
    auto column() const{ return _column; }
    auto length() const { return _length; }
    auto start() const { return _start; }
    auto end() const { return _end; }
    auto name() const { return _name; }
    auto& data() { return static_cast<DataStorageType&>(*this); }
    const auto& data() const { return static_cast<const DataStorageType&>(*this); }
    static auto create(const long long startpos, const char* start, const long long length, const long long line, EnumT enumv, DataStorageType t) {
        std::size_t column = 0;
        for (const char* count = start; start - count != startpos && *count != '\n'; --count) {
            ++column;
        }
        return Node {(std::size_t) startpos, start, start + length, (std::size_t) length, (std::size_t) line, column, enumv, std::move(t)};
    }
};
template<class EnumT, class DataStorageType>
struct MatchResult {
    bool status = false;
    Node<EnumT, DataStorageType> node = {};
};
template<class TOKEN_T, typename Token>
using TokenFlow = std::vector<Token>;
template<class RULE_T, class DataStorageType>
using Seq = std::vector<Node<RULE_T, DataStorageType>>;
namespace DFA::API {
    struct DFADebug;
    inline auto null_state = std::numeric_limits<std::size_t>::max();
    // Same numbering as NFA::TNFA::Action, so the converter emits the value as is.
    enum class Action { UNDEF, SET, SET_NEXT, COPY, APPEND, APPEND_NEXT };
    template<std::size_t Classes>
    using State = std::array<std::size_t, Classes>;
    template<std::size_t States, std::size_t Classes>
    using Table = std::array<State<Classes>, States>;
    using CharToClass = std::array<std::size_t, 256>;
    // Row: {op, a, b, next_state}.
    //   SET / SET_NEXT        reg[a] = position (NEXT: one past the character being consumed)
    //   COPY                  reg[a] = reg[b]
    //   APPEND / APPEND_NEXT  reg[a] = append(reg[b], position)   (b == null_state: empty list)
    template<std::size_t States>
    using LRTable = std::array<State<4>, States>;
    template<std::size_t Size>
    using DFADebugTable = std::array<DFADebug, Size>;
    using DebugIndex = std::unordered_map<long long, std::unordered_map<long long, long long>>;

    /*
     * Can be emit by parser generator.
     * This turns the DFA walker from class -> next state into something can be reversed to actual grammar structures
    */
    struct DFADebug {
        std::string member;
        std::string token_name;
        long long position_in_token;
        long long call;
        long long group;

        long long rule_run = null_state;
        long long offset = 0;
        long long length = 1;
        char ch = '\0';

        auto operator<(const DFADebug &other) const {
            return std::tie(member, token_name, position_in_token, call, group, rule_run, offset, length, ch) < std::tie(other.member, other.token_name, other.position_in_token, other.call, other.group, other.rule_run, other.offset, other.length, other.ch);
        }
        auto operator==(const DFADebug &other) const{
            return std::tie(member, token_name, position_in_token, call, group, rule_run, offset, length, ch) == std::tie(other.member, other.token_name, other.position_in_token, other.call, other.group, other.rule_run, other.offset, other.length, other.ch);
        }
    };

    // Capture k owns registers 2k (begin, or the head of a list capture's history)
    // and 2k + 1 (end). A register that was never written is `unset`.
    inline constexpr std::ptrdiff_t unset = -1;

    struct HistoryNode {
        std::ptrdiff_t pos;  // offset from the start of the token
        std::ptrdiff_t prev; // previous node, `unset` ends the list
    };

    class Captures {
        const char *base_ = nullptr;
        const std::ptrdiff_t *out_ = nullptr;
        const HistoryNode *history_ = nullptr;
        long long first_line_ = 1;

        auto begin_off(std::size_t k) const { return out_[2 * k]; }
        auto end_off(std::size_t k) const { return out_[2 * k + 1]; }

    public:
        Captures() = default;
        Captures(const char *base, const std::ptrdiff_t *out, const HistoryNode *history, long long first_line)
            : base_(base), out_(out), history_(history), first_line_(first_line) {}

        // scalar capture: both boundaries were recorded
        auto is_set(std::size_t k) const -> bool { return begin_off(k) != unset && end_off(k) != unset; }
        auto begin(std::size_t k) const -> const char * { return base_ + begin_off(k); }
        auto end(std::size_t k) const -> const char * { return base_ + end_off(k); }
        auto view(std::size_t k) const -> std::string_view {
            return std::string_view(begin(k), static_cast<std::size_t>(end_off(k) - begin_off(k)));
        }
        auto text(std::size_t k) const -> std::string { return std::string(view(k)); }
        auto character(std::size_t k) const -> char { return *begin(k); }

        // list capture: one entry per iteration, in input order
        auto list_views(std::size_t k) const -> std::vector<std::string_view> {
            std::vector<std::ptrdiff_t> positions;
            for (auto h = begin_off(k); h != unset; h = history_[h].prev)
                positions.push_back(history_[h].pos);
            std::reverse(positions.begin(), positions.end());
            std::vector<std::string_view> spans;
            for (std::size_t i = 0; i + 1 < positions.size(); i += 2)
                spans.emplace_back(base_ + positions[i], static_cast<std::size_t>(positions[i + 1] - positions[i]));
            return spans;
        }
        template<typename T = std::string>
        auto list(std::size_t k) const -> std::vector<T> {
            std::vector<T> out;
            for (const auto v : list_views(k)) {
                if constexpr (std::is_same_v<T, char>)
                    out.push_back(v.empty() ? '\0' : v.front());
                else
                    out.push_back(T(v));
            }
            return out;
        }

        auto line(std::size_t k) const -> long long {
            return first_line_ + std::count(base_, begin(k), '\n');
        }
        auto column(std::size_t k) const -> long long {
            const char *b = begin(k);
            const char *line_start = b;
            while (line_start > base_ && line_start[-1] != '\n')
                --line_start;
            return (b - line_start) + 1;
        }
    };

    template<std::size_t Registers, std::size_t Outputs>
    struct TdfaLayout {
        static constexpr std::size_t registers = Registers;
        static constexpr std::size_t outputs = Outputs;
    };
}
namespace DFA::detail {
    // Debug trace of one DFA step.
    template<std::size_t debug_size>
    inline void trace_step(
        const API::DFADebugTable<debug_size> &debug_array,
        const API::DebugIndex &debug_index,
        std::size_t state,
        std::size_t cls,
        char current_ch,
        const char *pos,
        bool immediate_action
    ) {
        const API::DFADebug *debug = nullptr;

        // 2. Compute 1D index using base offset + char class
        // 1. Safe lookup without mutating map or inserting null keys
        if (debug_size > 0) {
            if (auto state_it = debug_index.find(state); state_it != debug_index.end()) {
                if (auto cls_it = state_it->second.find(cls); cls_it != state_it->second.end()) {
                    debug = &debug_array[cls_it->second];
                }
            }
        }

        if (debug) {
            std::stringstream ss;
            ss << debug->token_name << ": ";
            std::cout << ss.str() << debug->member;
            if (debug->call != -1) {
                std::cout << "$call" << debug->call;
            }
            if (debug->group != -1) {
                std::cout << "$group" << debug->group;
            }
            std::cout << "; " << debug->offset + 1 << "/" << debug->length << ";\n";
            std::cout << std::string(ss.str().size() + debug->offset, ' ');

            const auto &mem = debug->member;
            if (mem.size() >= 2 && mem.front() == '[') {
                bool escaped = false;

                for (std::size_t i = 1; i < mem.size(); ++i) {
                    char c = mem[i];

                    if (escaped) {
                        escaped = false;
                        if (current_ch == c) {
                            std::cout << std::string(i, ' ') << "^\n";
                            break;
                        }
                        continue; // Skip further range/escape processing for this character
                    }

                    if (c == '\\') {
                        escaped = true;
                        continue; // Skip to next character after setting escape flag
                    }

                    if (current_ch == c) {
                        std::cout << std::string(i, ' ') << "^\n";
                        break;
                    }

                    // Check range e.g., a-z
                    if (c == '-' && i > 1 && (i + 1) < mem.size() && mem[i + 1] != ']') {
                        char range_begin = mem[i - 1];
                        char range_end = mem[i + 1];

                        if (current_ch >= range_begin && current_ch <= range_end) {
                            std::cout << std::string(i - 1, ' ') << "^ ^\n";
                            break;
                        }
                    }
                }
            } else {
                std::cout << "^\n";
            }
        } else {
            std::cout << "State " << state << " char '"
                      << *(immediate_action ? pos + 1 : pos)
                      << "'\n";
        }


    }
}
namespace DFA {
    // Walks the DFA, moving positions between registers. Reaching a semantic state calls
    //     semantic(index, captures, start_pos, start, length, line) -> (next_state, Token)
    // and next_state == null_state ends the match with that Token; any other value resumes the
    // walk there. Returns an empty Token when nothing matches.
    template<
        typename Token,
        typename SemanticFunc,
        std::size_t table_states,
        std::size_t table_classes,
        std::size_t action_table_states,
        std::size_t debug_size,
        std::size_t registers,
        std::size_t outputs
    >
    auto run(
        const char* &pos,
        long long start_pos,
        long long &scanning_line,
        const API::Table<table_states, table_classes> &table,
        const API::CharToClass &class_table,
        const API::LRTable<action_table_states> &action_table,
        SemanticFunc semantic,
        const API::DFADebugTable<debug_size> &debug_array,
        const API::DebugIndex &debug_index,
        API::TdfaLayout<registers, outputs>
    ) -> Token {
        static_assert(registers >= outputs, "output registers are registers 0 .. outputs-1");

        const char *const start = pos;
        const long long start_line = scanning_line;

        std::array<std::ptrdiff_t, (registers > 0 ? registers : 1)> regs;
        regs.fill(API::unset);
        std::vector<API::HistoryNode> history;

        std::size_t state = 0;
        bool immediate_action = false;

        while (state != API::null_state) {
            const char current_ch = *(immediate_action && *pos != '\0' ? pos + 1 : pos);
            detail::trace_step(debug_array, debug_index, state,
                               class_table[static_cast<unsigned char>(current_ch)], current_ch, pos, immediate_action);

            if (state < table.size()) {
                if (immediate_action) {
                    immediate_action = false;
                    if (*pos == '\n')
                        scanning_line++;
                    if (*pos != '\0')
                        ++pos;
                }
                const std::size_t next = table[state][class_table[static_cast<unsigned char>(*pos)]];
                if (next == API::null_state)
                    break;
                state = next;

                // Entering a DFA state consumes the character; entering an action chain
                // does not, and the chain sees `pos` at that character.
                if (next < table.size() && *pos != '\0') {
                    if (*pos == '\n')
                        scanning_line++;
                    ++pos;
                } else {
                    immediate_action = true;
                }
            } else if (state < table.size() + action_table.size()) {
                const auto &row = action_table[state - table.size()];
                const auto action = static_cast<API::Action>(row[0]);
                const auto here = static_cast<std::ptrdiff_t>(pos - start);

                if (row[1] >= registers || (row[2] != API::null_state && row[2] >= registers))
                    throw std::runtime_error("DFA: register out of range");

                switch (action) {
                    case API::Action::SET:
                    case API::Action::SET_NEXT:
                        regs[row[1]] = here + (action == API::Action::SET_NEXT ? 1 : 0);
                        break;
                    case API::Action::COPY:
                        regs[row[1]] = regs[row[2]];
                        break;
                    case API::Action::APPEND:
                    case API::Action::APPEND_NEXT:
                        history.push_back({
                            here + (action == API::Action::APPEND_NEXT ? 1 : 0),
                            row[2] == API::null_state ? API::unset : regs[row[2]]
                        });
                        regs[row[1]] = static_cast<std::ptrdiff_t>(history.size()) - 1;
                        break;
                    default:
                        throw std::runtime_error("DFA: unknown action; Report this error to github");
                }
                state = row[3];
            } else {
                const API::Captures captures(start, regs.data(), history.data(), start_line);
                auto result = semantic(
                    state - table.size() - action_table.size(),
                    captures, start_pos, start, static_cast<long long>(pos - start), start_line
                );
                state = static_cast<std::size_t>(std::get<0>(result));
                if (state == API::null_state)
                    return std::get<1>(std::move(result));
            }
        }
        return Token {};
    }
} // namespace DFA
template<class TOKEN_T, typename Token>
class Lexer_base {
protected:
    const char* _in = nullptr;
    std::string _owned_input;
    TokenFlow<TOKEN_T, Token> tokens;
    long long line;
    std::size_t getCurrentPos(const char* pos) const {
        return pos - _in;
    }
    std::size_t skip_spaces(const char*& in) {
        auto prev = in;
        while (isspace(*in)) in++;
        return in - prev;
    }
    std::size_t __line(const char* pos) const {
        std::size_t count = 1;
        for (const char* in = _in; in < pos; in++)
            if (*in == '\n') count++;
        return count;
    }
    std::size_t __column(const char* pos) const {
        std::size_t count = 1;
        for (const char* in = _in; in < pos; in++)
            count = (*in == '\n') ? 0 : count + 1;
        return count;
    }
    void panic_mode(const char*& pos) {
        if (*pos != '\0') ++pos;
    }
    template<
        typename SemanticFunc,
        std::size_t table_states,
        std::size_t table_classes,
        std::size_t action_table_states,
        std::size_t debug_size,
        typename Layout
    >
    Token lookup(
        const DFA::API::Table<table_states, table_classes> &table,
        const DFA::API::CharToClass &class_table,
        const DFA::API::LRTable<action_table_states> &action_table,
        Layout layout,
        SemanticFunc semantic,
        const DFA::API::DFADebugTable<debug_size> &debug_array,
        const DFA::API::DebugIndex &debug_index,
        const char* &pos
    ) {
        if (*pos == '\0')
            return Token {};
        return DFA::run<Token>(pos, getCurrentPos(pos), line, table, class_table, action_table,
                               semantic, debug_array, debug_index, layout);
    }

public:
    // Accumulates tokens lazily; does not populate `tokens` on the owning lexer.
    class lazy_iterator {
        Lexer_base* owner = nullptr;
        Token current;
        const char* pos = nullptr;
        std::size_t counter = 0;

        void advance() {
            if (isEnd()) return;
            current = owner->makeToken(pos);
            if (!current.empty()) counter++;
        }

    public:
        lazy_iterator(Lexer_base& owner, const char* in) : owner(&owner), pos(in) {
            current = owner.makeToken(pos);
            counter = current.empty() ? 0 : 1;
        }
        lazy_iterator(const lazy_iterator& other)
            : owner(other.owner), current(other.current), pos(other.pos), counter(other.counter) {}

        bool isEnd() const { return current.empty(); }

        lazy_iterator& operator=(const lazy_iterator& other) {
            if (this != &other) {
                owner = other.owner;
                current = other.current;
                pos = other.pos;
                counter = other.counter;
            }
            return *this;
        }
        lazy_iterator& operator++() { advance(); return *this; }
        lazy_iterator operator++(int) { auto tmp = *this; advance(); return tmp; }
        void operator+=(std::size_t count) { while (count-- > 0 && !isEnd()) advance(); }

        ptrdiff_t operator-(const lazy_iterator& other) const {
            return static_cast<ptrdiff_t>(counter) - static_cast<ptrdiff_t>(other.counter);
        }
        const Token& operator*() const { return current; }
        const Token* operator->() const { return &current; }
        std::size_t distance() const { return counter; }
    };

    // Iterates already-accumulated tokens; run makeTokens() before using this.
    class iterator {
        Lexer_base* owner = nullptr;
        typename TokenFlow<TOKEN_T, Token>::iterator pos;

    public:
        iterator(Lexer_base& owner) : owner(&owner), pos(owner.tokens.begin()) {}

        iterator& operator=(const iterator& other) { owner = other.owner; pos = other.pos; return *this; }
        void operator+=(std::size_t count) { pos += count; }
        iterator& operator++() { pos += 1; return *this; }
        iterator operator++(int) { auto tmp = *this; pos += 1; return tmp; }
        std::size_t operator-(const iterator& other) const { return pos - other.pos; }
        iterator operator+(std::size_t count) const { auto tmp = *this; tmp += count; return tmp; }

        bool isEnd() const { return pos->empty(); }
        Token& operator*() const { return *pos; }
        Token* operator->() const { return &(*pos); }
        std::size_t distance() const { return pos - owner->tokens.begin(); }
    };
    virtual Token makeToken(const char*& pos) = 0;
    virtual void init() {}

    Lexer_base() { init(); }
    explicit Lexer_base(const std::string& in) : _owned_input(in), _in(_owned_input.c_str()) { init(); }
    explicit Lexer_base(const char* in) : _in(in) { init(); }
    explicit Lexer_base(const TokenFlow<TOKEN_T, Token>& tokens) : tokens(tokens) { init(); }
    virtual ~Lexer_base() = default;

    bool hasInput() const { return _in != nullptr; }
    bool hasTokens() const { return !tokens.empty(); }

    Lexer_base& setInput(const std::string& in) {
        _owned_input = in;
        _in = _owned_input.c_str();
        return *this;
    }
    Lexer_base& setInput(const char* in) {
        _owned_input.clear();
        _in = in;
        return *this;
    }

    const TokenFlow<TOKEN_T, Token>& getTokens() const { return tokens; }
    TokenFlow<TOKEN_T, Token>& getTokensReference() { return tokens; }
    void clearTokens() { tokens.clear(); }

    TokenFlow<TOKEN_T, Token>& makeTokens() {
        if (_in == nullptr)
            throw Lexer_No_Input_exception();
        const char* pos = _in;
        while (*pos != '\0')

            push(makeToken(pos));
        push(Token{});
        return tokens;
    }
    TokenFlow<TOKEN_T, Token>& makeTokens(const std::string& in) {
        setInput(in);
        return makeTokens();
    }
    TokenFlow<TOKEN_T, Token>& makeTokens(const char* in) {
        setInput(in);
        return makeTokens();
    }
    TokenFlow<TOKEN_T, Token>& makeTokensFromFile(const char* path) {
        std::ifstream file(path, std::ios::in | std::ios::binary);
        if (!file)
            throw std::runtime_error(std::string("Failed to open file '") + path + "'");

        std::string str;
        file.seekg(0, std::ios::end);
        str.reserve(static_cast<std::size_t>(file.tellg()));
        file.seekg(0, std::ios::beg);
        str.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        return makeTokens(str);
    }

    const std::vector<std::string> getErrors() const { return {""}; }

    void push(const TokenFlow<TOKEN_T, Token>& input_tokens) { tokens.append_range(input_tokens); }
    void push(const Token& input_token) { tokens.push_back(input_token); }
    void push(const Lexer_base& other) {
        if (!other.hasTokens())
            throw Lexer_No_Tokens_exception();
        tokens.append_range(other.tokens);
    }
    void pop() { tokens.pop_back(); }
    void pop(std::size_t n) {
        if (n > tokens.size())
            throw std::length_error(ISC_STD_LIBMARK "Lexer_base::pop(): n exceeds token count");
        tokens.erase(tokens.end() - n, tokens.end());
    }

    Lexer_base& operator=(const Lexer_base& other) {
        if (this != &other) {
            tokens = other.tokens;
            _owned_input = other._owned_input;
            _in = _owned_input.empty() ? other._in : _owned_input.c_str();
        }
        return *this;
    }
    bool operator==(const Lexer_base& other) const { return tokens == other.tokens; }
    bool operator!=(const Lexer_base& other) const { return tokens != other.tokens; }
};
/* PARSER */
template<class TOKEN_T, class RULE_T, typename MainNode, typename Token>
class LLParser_base {
protected:
    Lexer_base<TOKEN_T, Token>* lexer = nullptr;
    const char* text = nullptr;
    MainNode tree;
    // skip spaces for tokens
    template <class IT>
    std::size_t skip_spaces(IT& pos) {
        auto prev = pos;
        while (pos->name() == TOKEN_T::__WHITESPACE)
            ++pos;
        
        return pos - prev;
    }
    static void PANIC_MODE() {}
public:
    virtual MatchResult<RULE_T, Token> getRule(typename Lexer_base<TOKEN_T, Token>::lazy_iterator &pos) = 0;
    virtual MatchResult<RULE_T, Token> getRule(typename Lexer_base<TOKEN_T, Token>::iterator &pos) = 0;
    virtual void parseFromTokens() = 0;
    virtual void lazyParse() = 0;
    // Constructors
    LLParser_base() {}
    LLParser_base(const Lexer_base<TOKEN_T, Token>& lexer) {
        if (lexer.hasTokens())
            this->lexer = &lexer;
    }
    LLParser_base(const char* text) : text(text) {}
    virtual ~LLParser_base() {}
    // Parsing methods
    MainNode& parse(Lexer_base<TOKEN_T, Token>& lex) {
        if (lex.hasTokens()) {
            lexer = &lex;
        }
        return parse();
    }
    MainNode& parse(const char* txt) {
        text = txt;
        return parse();
    }
    void setInput(Lexer_base<TOKEN_T, Token> &lex) {
        if (!lex.hasTokens())
            lexer = &lex;
    }
    void setInput(const char* txt) {
        text = txt;
    }
    void clearInput() {
        lexer = nullptr;
        text = nullptr;
    }
    /**
     * @brief Parser the tokens based on input provided before
     * 
     * @return Tree<RULE_T> 
     */
    MainNode& parse() {
        if (lexer != nullptr) {
            parseFromTokens();
        } else if (text != nullptr) {
            lazyParse();
        } else throw Parser_No_Input_exception();
        return tree;
    }
};
template <class TOKEN_T, class RULE_T, class MAIN_NODE, class Token, class Action, class ActionTable, class GotoTable, class RulesTable>
class LRParser_base : public LLParser_base<TOKEN_T, RULE_T, MAIN_NODE, Token> {
protected:
    std::vector<std::pair<std::variant<TOKEN_T, RULE_T>, std::size_t>> stack;
    template <class IT>
    void shift(IT& pos, std::size_t state) {
        stack.push_back({pos->name(), state});
        pos++;
    }
    void reduce(const std::size_t rules_id, const GotoTable &goto_table, const RulesTable rules_table) {
        const auto &rule_data = rules_table[rules_id];
        const auto &rule_name = rule_data.first;
        const auto &reduce_size = rule_data.second;
        if (stack.size() < reduce_size) {
            throw std::runtime_error("Stack underflow during reduce");
        }
        stack.erase(stack.end() - reduce_size, stack.end());
        printf("Reduce: goto_table[%d][%d]\n", (int) stack.back().second, (int) rule_name);
        // Perform the reduction
        const auto& goto_entry = goto_table[stack.back().second][static_cast<std::size_t>(rule_name)];
        if (!goto_entry.has_value()) {
            throw std::runtime_error("Invalid GOTO after reduction");
        }

        std::size_t next_state = goto_entry.value();
        stack.push_back({rule_name, next_state});
    }
    MatchResult<RULE_T, Token> getRule(typename Lexer_base<TOKEN_T, Token>::lazy_iterator &pos) {
        return {};
    }
    MatchResult<RULE_T, Token> getRule(typename Lexer_base<TOKEN_T, Token>::iterator &pos) {
        return {};
    }
    virtual std::string TokensToString(TOKEN_T token) = 0;
    virtual std::string RulesToString(RULE_T rule) = 0;
    template<class IT>
    void parseFromPos(IT& pos, const ActionTable &action_table, const GotoTable &goto_table, RulesTable rules_table) {
        stack.push_back({TOKEN_T::NONE, 0});
        while(true) {
            auto &current_state = stack.back().second;
            const auto &action = action_table[current_state][(std::size_t) pos->name()];
            printf("Token name: %s", TokensToString(pos->name()).c_str());
            if (pos->data().has_value()) {
                printf("[%s]", std::any_cast<std::string>(pos->data()).c_str());
            }
            printf(", state: %zu\n", current_state);
            if (action.has_value()) {
                auto& act = action.value();
                printf("action: %d, next state: %zu\n", (int) act.type, act.state);
                if (act.type == Action::SHIFT)
                    shift(pos, act.state);
                else if (act.type == Action::REDUCE)
                    reduce(act.state, goto_table, rules_table);
                else if (act.type == Action::ACCEPT)
                    break;
                else
                    throw std::runtime_error("Error state");
            } else {
                throw std::runtime_error(("Action is not defined. stack size: " + std::to_string(stack.size())).c_str());
            }
        }
        printf("Accepted. distance: %zu\n", pos.distance());
        stack.clear();
    }
};
template <class TOKEN_T, class RULE_T, class MAIN_NODE, class Token, class Action, class ActionTable, class GotoTable, class RulesTable, class DFATable>
class ELRParser_base : public LRParser_base<TOKEN_T, RULE_T, MAIN_NODE, Token, Action, ActionTable, GotoTable, RulesTable> {
private:
    // cache tokens because of lazy iterator which makes tokens on dereference
    std::deque<Node<TOKEN_T, Token>> dfa_token_cache;
protected:
    template <class IT>
    void shift(IT& pos, std::size_t state) {
        if (dfa_token_cache.empty()) {
            printf("Pushing directly\n");
            this->stack.push_back({pos->name(), state});
            pos++;
        } else {
            printf("Pushing from DFA cache, next token: ");
            printf("%s", TokensToString(pos->name()).c_str());
            if (pos->data().has_value()) {
                printf("[%s]", std::any_cast<std::string>(pos->data()).c_str());
            }
            printf("\n");
            this->stack.push_back({dfa_token_cache.front().name(), state});
            dfa_token_cache.pop_back();
        }
    }
    template<class IT>
    const std::optional<Action>& getAction(IT &pos, const ActionTable &action_table) {
        auto &current_state = this->stack.back().second;
        return dfa_token_cache.empty() ? action_table[current_state][(std::size_t) pos->name()] : action_table[current_state][(std::size_t) dfa_token_cache.front().name()];
    }
    template<class IT>
    const Action* resolveDFA(IT &pos, std::size_t dfa_index, const DFATable &dfa_table) {
        const Action* initial_action = nullptr;
        printf("Resolving conflict in DFA table\n");
        std::size_t current_dfa_length = dfa_token_cache.size();
        for (std::size_t offset = 0;; offset++) {
            if (offset >= current_dfa_length)
                dfa_token_cache.push_back(*pos++);
            const auto &[action, table] = dfa_table[dfa_index];
            std::size_t i = 1;
            while(table[i].first != dfa_token_cache[offset].name() && table[i].second != 0) i++;
            const auto &go_state = table[i].second;
            if (initial_action == nullptr) {
                initial_action = &action;
            }
            if (go_state == 0) {
                if (table[0].second != 0) {
                    dfa_index = table[0].second;
                    continue;
                }
                if (action.type == Action::ERR) {
                    printf("Returning initial action %d, state %zu\n", (int) initial_action->type, initial_action->state);
                    return initial_action;
                }
                printf("returning action %d, state %zu\n", (int) action.type, action.state);
                return &action;
            }
            dfa_index = go_state;

        }
    }
    template<class IT>
    void peformAction(IT &pos, Action act, GotoTable goto_table, RulesTable rules_table, DFATable dfa_table) {
        switch (act.type)
        {
        case Action::SHIFT:
            shift(pos, act.state);
            break;
        case Action::REDUCE:
            this->reduce(act.state, goto_table, rules_table);
            break;
        case Action::DFA_RESOLVE: {
            const auto resolved = resolveDFA(pos, act.state, dfa_table);
            if (!resolved) throw std::runtime_error("Unresolvable DFA lookahead");
            peformAction(pos, *resolved, goto_table, rules_table, dfa_table);
            break;
        }
        default:
            throw std::runtime_error("Error action");
        }
    }
    template<class IT>
    void parseFromPos(IT& pos, const ActionTable &action_table, const GotoTable &goto_table, RulesTable rules_table, DFATable dfa_table) {
        this->stack.push_back({TOKEN_T::NONE, 0});
        while(true) {
            auto &current_state = this->stack.back().second;
            const auto &action = getAction(pos, action_table);
            if (dfa_token_cache.empty()) {
                printf("Token name: %s", TokensToString(pos->name()).c_str());
                if (pos->data().has_value()) {
                    printf("[%s]", std::any_cast<std::string>(pos->data()).c_str());
                }
                printf(", state: %zu\n", current_state);
            } else {
                printf("Token name: %s", TokensToString(dfa_token_cache.front().name()).c_str());
                if (pos->data().has_value()) {
                    printf("[%s]", std::any_cast<std::string>(pos->data()).c_str());
                }
                printf(", state: %zu\n", current_state);
            }

            if (action.has_value()) {
                auto& act = action.value();
                printf("action: %d, next state: %zu\n", (int) act.type, act.state);
                if (act.type == Action::ACCEPT)
                    break;
                peformAction(pos, act, goto_table, rules_table, dfa_table);
            } else {
                throw std::runtime_error(("Action is not defined. stack size: " + std::to_string(this->stack.size())).c_str());
            }
        }
        printf("Accepted. distance: %zu\n", pos.distance());
        // clear
        this->stack.clear();
        dfa_token_cache.clear();
    }
};
} // namespace ISPA_STD

#undef _ISC_GITHUB
#undef _ISC_INTERNAL_ERROR_MARK
#endif // _ISPA_STD_LIB_CPP