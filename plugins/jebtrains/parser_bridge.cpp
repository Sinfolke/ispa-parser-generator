#include "ispa/parser.h"
#include <algorithm>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

namespace {
struct Span { std::string kind, role; std::size_t start, length; };
struct Diagnostic { std::string message; std::size_t start, length; };
std::string escaped(const std::string& s) {
    std::string out;
    constexpr char hex[] = "0123456789abcdef";
    for (unsigned char c : s) {
        if (c == '"' || c == '\\') { out += '\\'; out += c; }
        else if (c < 0x20) { out += "\\u00"; out += hex[c >> 4]; out += hex[c & 15]; }
        else out += c;
    }
    return out;
}
void add(std::vector<Diagnostic>& errors, std::string message, std::size_t offset, std::size_t length, std::size_t size) {
    offset = std::min(offset, size);
    length = std::min(length, size - offset);
    if (errors.size() >= 32) return;
    for (const auto& error : errors) if (error.start == offset && error.message == message) return;
    errors.push_back({std::move(message), offset, length});
}
std::size_t diagnosticOffset(const std::string& message, const std::string& source, std::size_t fallback) {
    static const std::regex linePattern("Python action at line ([0-9]+):");
    std::smatch match;
    if (!std::regex_search(message, match, linePattern)) return fallback;
    const auto line = std::stoull(match[1].str());
    if (line == 0) return fallback;
    std::size_t start = 0;
    for (std::size_t current = 1; current < line; ++current) {
        const auto next = source.find('\n', start);
        if (next == std::string::npos) return fallback;
        start = next + 1;
    }
    while (start < source.size() && (source[start] == ' ' || source[start] == '\t')) ++start;
    return start;
}
// Editor validation is file-local: strip preprocessing syntax without resolving
// imports against the filesystem. Spaces preserve the parser's byte offsets.
std::string prepare(const std::string& source, std::vector<Diagnostic>& errors) {
    auto text = source;
    const auto size = source.size();
    auto blank = [&](std::size_t start, std::size_t end) {
        for (; start < end; ++start) if (text[start] != '\n' && text[start] != '\r') text[start] = ' ';
    };
    auto quoted = [&](std::size_t start, char close, bool python = false) {
        bool triple = python && start + 2 < size && source[start + 1] == close && source[start + 2] == close;
        auto end = start + (triple ? 3 : 1);
        while (end < size) {
            if (source[end] == '\\') { end += std::min<std::size_t>(2, size - end); continue; }
            if (source[end] == close && (!triple || (end + 2 < size && source[end+1] == close && source[end+2] == close)))
                return end + (triple ? 3 : 1);
            ++end;
        }
        return size;
    };
    static const std::string id = "[A-Za-z_][A-Za-z_0-9]*";
    static const std::string modulePath = id + "(\\s*\\.\\s*" + id + ")*";
    static const std::string path = id + "(\\s*->\\s*" + id + ")*";
    static const std::string list = path + "(\\s*,\\s*" + path + ")*";
    static const std::regex module("module\\s+" + modulePath + "(\\s*->\\s*" + path + ")?\\s*;");
    static const std::regex imported("import\\s+(extern\\s+" + id + "\\s+)?((?!extern\\b)" + modulePath + "(\\s+as\\s+" + id + ")?|(" +
        "\\*|\\{\\s*" + list + "\\s*\\})\\s+from\\s+" + modulePath + ")\\s*;");
    static const std::regex detached("detach\\s+(feature\\s+)?" + path + "\\s*;");
    static const std::regex feature("feature\\s+" + path + "\\s*\\{\\s*(" + list + ")?\\s*\\}\\s*;");
    std::size_t ruleDepth = 0, braces = 0, parentheses = 0;
    bool declaration = true, header = false, advanced = false, failBlock = false;
    for (std::size_t i = 0; i < size;) {
        if (source.compare(i, 2, "//") == 0) {
            const auto end = source.find('\n', i);
            auto stop = end == std::string::npos ? size : end;
            blank(i, stop); i = stop; continue;
        }
        if (source.compare(i, 2, "/*") == 0) {
            auto end = source.find("*/", i + 2);
            if (end == std::string::npos) { add(errors, "Unterminated block comment", i, 2, size); end = size; }
            else end += 2;
            blank(i, end); i = end; continue;
        }
        if (source.compare(i, 2, "p{") == 0) {
            add(errors, "Legacy p{...} wrapper is unsupported; use direct Python", i, 2, size);
            std::size_t depth = 1; i += 2;
            while (i < size && depth) {
                if (source[i] == '\'' || source[i] == '"') { i = quoted(i, source[i], true); continue; }
                if (source[i] == '#') { while (i < size && source[i] != '\n') ++i; continue; }
                if (source[i] == '{') ++depth;
                else if (source[i] == '}') --depth;
                ++i;
            }
            if (failBlock && ruleDepth == 0) { declaration = true; header = false; failBlock = false; }
            continue;
        }
        if (source[i] == '\'' || source[i] == '"' || source[i] == '[') {
            i = quoted(i, source[i] == '[' ? ']' : source[i]); continue;
        }
        if (std::isalpha(static_cast<unsigned char>(source[i])) || source[i] == '_') {
            const auto start = i++;
            while (i < size && (std::isalnum(static_cast<unsigned char>(source[i])) || source[i] == '_')) ++i;
            const auto word = source.substr(start, i - start);
            auto next = i;
            while (next < size && std::isspace(static_cast<unsigned char>(source[next]))) ++next;
            if (declaration && next < size && source[next] != ':' &&
                (word == "module" || word == "import" || word == "detach" || word == "feature")) {
                auto end = source.find(';', next);
                if (end == std::string::npos) { add(errors, "Expected ';' after " + word + " directive", size, 0, size); end = size; }
                else ++end;
                auto directive = text.substr(start, end - start);
                const auto& pattern = word == "module" ? module : word == "import" ? imported : word == "detach" ? detached : feature;
                if (end < size || (end > start && source[end - 1] == ';')) {
                    if (!std::regex_match(directive, pattern)) add(errors, "Invalid " + word + " directive", start, end - start, size);
                }
                blank(start, end); i = end; continue;
            }
            if (declaration) { declaration = false; header = true; failBlock = word == "fail"; }
            continue;
        }
        const auto c = source[i++];
        if (c == '(') ++parentheses;
        else if (c == ')' && parentheses) --parentheses;
        else if (c == '{') { if (header && !parentheses) { advanced = ruleDepth == 0; header = false; } ++braces; }
        else if (c == '}' && braces) { if (--braces == 0 && advanced) { declaration = true; advanced = false; } }
        else if (c == '#' && i < size && source[i] != '[' && !braces) header = true;
        else if (c == ':' && header && !braces && !parentheses) { ++ruleDepth; header = false; }
        else if (c == ';' && !braces && !parentheses) {
            if (ruleDepth) --ruleDepth;
            declaration = ruleDepth == 0;
        }
    }
    return text;
}
#ifdef ISPA_MODULAR_PARSER
using Identifier = ::Parser::Types::modules::common::ID;
#else
using Identifier = ::Parser::Types::ID;
#endif
struct Lexer : IspaLexer {
    std::size_t stopped = 0;
    const char* input;
    explicit Lexer(const std::string& text) : input(text.c_str()) { setInput(input); }
    Token makeToken(const char*& cursor) override {
        stopped = cursor - input;
        // Keep whitespace explicit so custom boundaries are checked again at
        // the next token, including before Python and at the end of the file.
        if (std::isspace(static_cast<unsigned char>(*cursor))) {
            const char* start = cursor;
            while (std::isspace(static_cast<unsigned char>(*cursor))) ++cursor;
            return Token::create(getCurrentPos(start), start, cursor - start, 0,
                ::Parser::Tokens::__WS::id, ::Parser::Types::__WS{});
        }
        auto token = IspaLexer::makeToken(cursor);
        if (cursor == input + stopped) throw std::runtime_error("Unrecognized character");
        return token;
    }
    auto clone() const -> std::shared_ptr<ISPA_STD::Lexer_base<Token>> override {
        return std::make_shared<Lexer>(*this);
    }
};
struct BridgeParser : ::Parser::Parser {
    using ::Parser::Parser::main;
    using ::Parser::Parser::skip_spaces;
};
struct Semantics {
    const std::string& source;
    std::vector<Span>& spans;
    template<class T> static constexpr std::string_view typeName() {
#ifdef _MSC_VER
        return __FUNCSIG__;
#else
        return __PRETTY_FUNCTION__;
#endif
    }
    template<class T> void visit(const ISPA_STD::Node<T>& node, std::string_view role = {}) {
        if (node.empty()) return;
        if constexpr (std::is_same_v<T, Identifier>) {
            if (role.empty() || !node.start()) return;
            const auto address = reinterpret_cast<std::uintptr_t>(node.start());
            const auto base = reinterpret_cast<std::uintptr_t>(source.data());
            if (address >= base && address - base < source.size())
                spans.push_back({"ID", std::string(role), address - base, std::min(node.length(), source.size() - (address - base))});
        } else if constexpr (typeName<T>().find("PyRawParser::") == std::string_view::npos) {
            // Python identifiers belong to the embedded language, not ISPA captures.
            visit(node.data(), role);
        }
    }
    template<class T> void visit(const std::shared_ptr<T>& node, std::string_view role = {}) { if (node) visit(*node, role); }
    template<class... T> void visit(const std::variant<T...>& node, std::string_view role = {}) {
        std::visit([&](const auto& value) { visit(value, role); }, node);
    }
    template<class T, class A> void visit(const std::vector<T, A>& node, std::string_view role = {}) {
        for (const auto& value : node) visit(value, role);
    }
    template<class T> void visit(const T& node, std::string_view role = {}) {
        if constexpr (requires { node.members(); T::members_str(); }) {
            constexpr auto fields = T::members_str();
            constexpr bool declaration = std::ranges::find(fields, "entry") != fields.end() && std::ranges::find(fields, "name") != fields.end();
            constexpr auto type = typeName<T>();
            std::apply([&](const auto&... values) {
                std::size_t index = 0;
                (([&] {
                    const auto field = fields[index++];
                    auto context = role;
                    if (declaration && field == "name") context = "declaration";
                    else if (field == "first_p" || field == "following_p") context = "parameter";
                    else if (field == "key" || field == "assign_names" ||
                        (type.find("abbr") != type.npos && (field == "f" || field == "seq")) ||
                        ((type.find("ast_capture") != type.npos || type.find("var_capture") != type.npos || type.find("shared_counter_registration") != type.npos) && field == "value")) context = "capture";
                    else if (type.find("fail_block") != type.npos && field == "name") context = "declaration";
                    visit(values, context);
                }()), ...);
            }, node.members());
        }
    }
};
}
int main() {
    const std::string source((std::istreambuf_iterator<char>(std::cin)), {});
    std::vector<Span> spans;
    std::vector<Diagnostic> errors;
    {
        // Keep generated DFA traces off the stdout JSON protocol.
        struct Redirect { std::streambuf* previous = std::cout.rdbuf(std::cerr.rdbuf()); ~Redirect() { std::cout.rdbuf(previous); } } redirect;
        const auto text = prepare(source, errors);
        Lexer lexer(text);
        bool lexed = false;
        try {
            // Collect host tokens for highlighting without lexing Python as ISPA.
            // Parsing below uses a separate lazy stream for external rule calls.
            const char* cursor = text.c_str();
            while (*cursor) {
                lexer.stopped = cursor - text.c_str();
                if (cursor[0] == 'p' && cursor[1] == '{') {
                    cursor = IspaSyntax::scanPythonBlock(cursor);
                    continue;
                }
                try { lexer.push(lexer.makeToken(cursor)); }
                catch (const std::exception&) { ++cursor; }
                catch (const char*) { ++cursor; }
            }
            lexed = true;
        }
        catch (const std::exception& e) { add(errors, e.what(), lexer.stopped, 1, source.size()); }
        catch (const char* e) { add(errors, e, lexer.stopped, 1, source.size()); }
        for (const auto& token : lexer.getTokens()) {
            if (token.empty()) continue;
            const auto& meta = token.meta();
            const auto address = reinterpret_cast<std::uintptr_t>(meta.start());
            const auto base = reinterpret_cast<std::uintptr_t>(text.data());
            const auto start = address >= base && address - base <= text.size() ? address - base : meta.startpos();
            if (start < text.size() && meta.length())
                spans.push_back({::Parser::TokensToString(token.kind()), {}, start, std::min(meta.length(), text.size() - start)});
        }
        const bool legacyPython = std::any_of(errors.begin(), errors.end(), [](const auto& error) {
            return error.message.starts_with("Legacy p{...}");
        });
        if (lexed && !legacyPython) {
            BridgeParser parser;
            Lexer parserLexer(text);
            try {
                auto begin = ::Parser::Lexer::lazy_iterator(parserLexer, text.c_str());
                parser.skip_spaces<::Parser::Types::__WS>(begin);
                auto result = parser.main(begin);
                auto end = result.it;
                if (result.status) {
                    parser.skip_spaces<::Parser::Types::__WS>(end);
                    if (!end.isEnd()) add(errors, "Parsing stopped: expected a rule declaration", end->meta().startpos(), end->meta().length(), source.size());
                    Semantics{text, spans}.visit(result.node);
                } else add(errors, "Parsing stopped: expected a rule declaration", begin.isEnd() ? source.size() : begin->meta().startpos(), 1, source.size());
            } catch (const std::exception& e) {
                if (parser.getErrors().empty() && parser.getContextErrors().empty())
                    add(errors, e.what(), diagnosticOffset(e.what(), source, parserLexer.stopped), 1, source.size());
            } catch (const char* e) {
                if (parser.getErrors().empty() && parser.getContextErrors().empty())
                    add(errors, e, parserLexer.stopped, 1, source.size());
            }
            for (const auto& error : parser.getErrors()) {
                std::string expected = error.expected_text;
                if (expected.empty()) for (auto kind : error.expected) {
                    if (!expected.empty()) expected += ", ";
                    expected += ::Parser::TokensToString(kind);
                }
                const bool eof = error.unexpected == ISPA_STD::Recovery::eof_token;
                auto message = error.message.empty()
                    ? "Expected " + expected + (eof ? " at end of file" : "; got " + std::string(::Parser::TokensToString(error.unexpected)))
                    : error.message;
                add(errors, message, eof ? source.size() : error.offset, error.length, source.size());
            }
            for (const auto& error : parser.getContextErrors())
                add(errors, error.message, error.offset, error.length, source.size());
        }
        std::stable_sort(errors.begin(), errors.end(), [](const auto& a, const auto& b) { return a.start < b.start; });
    }
    std::cout << "{\"ok\":" << (errors.empty() ? "true" : "false") << ",\"tokens\":[";
    bool first = true;
    for (const auto& span : spans) {
        if (!first) std::cout << ','; first = false;
        std::cout << "{\"kind\":\"" << escaped(span.kind) << "\",\"role\":\"" << span.role << "\",\"start\":" << span.start << ",\"length\":" << span.length << '}';
    }
    std::cout << "],\"diagnostics\":[";
    first = true;
    for (const auto& error : errors) {
        if (!first) std::cout << ','; first = false;
        std::cout << "{\"message\":\"" << escaped(error.message) << "\",\"start\":" << error.start << ",\"length\":" << error.length << '}';
    }
    std::cout << "]}\n";
}
