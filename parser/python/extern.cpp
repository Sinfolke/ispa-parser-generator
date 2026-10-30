#include <PyRawParser.h>
#include "parser.hpp"
#include <ispa/lexical.hpp>
#include <cctype>
#include <ispastdlib/recovery_scan.hpp>

namespace PyRawParser::Exports {
namespace {
struct Parser : PyParser {
    using PyParser::main;
};
const char* blockStart(const char* start) {
    while (start && *start && std::isspace(static_cast<unsigned char>(*start))) ++start;
    return start;
}
template<bool expression>
auto parseBlock(const char* origin, const char* start) {
    using Data = std::conditional_t<expression, PyRawParser::Types::expr, PyRawParser::Types::main>;
    start = blockStart(start);
    const char* end = IspaSyntax::scanPythonBlock(start);
    try {
        PyLexer lexer;
        lexer.setBoundary(end - 1);
        Parser parser;
        PyLexer::lazy_iterator begin(lexer, origin, start + 2);
        auto result = [&] {
            if constexpr (expression) return parser.parseExpression(begin);
            else return parser.main(begin);
        }();
        auto cursor = result.it;
        if constexpr (expression) {
            if (!cursor.isEnd() && cursor->template holds<PyRawParser::Types::NEWLINE>()) ++cursor;
        }
        if (!result.status || !cursor.isEnd() || !parser.getErrors().empty() ||
            !parser.getContextErrors().empty()) {
            std::string detail = "invalid Python action or unconsumed Python tokens";
            if (!parser.getErrors().empty()) detail += ": " + parser.getErrors().front().message;
            else if (!cursor.isEnd()) detail += ": " + std::string(cursor->meta().start(), cursor->meta().length());
            throw std::runtime_error(detail);
        }
        auto node = std::move(result.node);
        return ISPA_STD::ExternalResult<Data>{true,
            ISPA_STD::external_node(origin, start + 2, end - 1, std::move(node)), end};
    } catch (const std::exception& error) {
        const auto line = 1 + std::count(origin, start, '\n');
        throw std::runtime_error("Python action at line " + std::to_string(line) + ": " + error.what());
    }
}
}
bool ExpressionBlock::starts(const char*, const char* start) {
    start = blockStart(start);
    if (!start || !*start) return false;
    const auto first = static_cast<unsigned char>(*start);
    return std::isalnum(first) || first == '_' ||
        std::string_view("'\"([{+-~").find(*start) != std::string_view::npos;
}
bool StatementsBlock::starts(const char*, const char* start) {
    start = blockStart(start);
    return start && start[0] == 'p' && start[1] == '{';
}
ISPA_STD::ExternalResult<ExpressionBlock::node_type> ExpressionBlock::parse(const char* origin, const char* start) {
    start = blockStart(start);
    if (!starts(origin, start)) return {};
    if (start[0] == 'p' && start[1] == '{')
        throw std::runtime_error("use a direct Python expression; p{...} wrappers are unsupported");
    const std::string_view stops[]{"}", ")", ";"};
    const auto boundary = ISPA_STD::Recovery::scanBalanced(start, stops, std::char_traits<char>::length(start));
    if (boundary.exhausted) throw std::runtime_error("unterminated string in Python expression");
    PyLexer lexer;
    lexer.setBoundary(boundary.end);
    Parser parser;
    auto result = parser.parseExpression(PyLexer::lazy_iterator(lexer, origin, start));
    const char* end = result.status ? result.it.sourcePosition() : start;
    if (!result.status || !parser.getErrors().empty() || !parser.getContextErrors().empty()) {
        std::string message = "invalid Python expression";
        if (!parser.getErrors().empty()) message += ": " + parser.getErrors().front().message;
        throw std::runtime_error(message);
    }
    return {true, ISPA_STD::external_node(origin, start, end, std::move(result.node)), end};
}
ISPA_STD::ExternalResult<StatementsBlock::node_type> StatementsBlock::parse(const char* origin, const char* start) { return parseBlock<false>(origin, start); }
bool RawStatements::starts(const char*, const char* start) {
    start = blockStart(start);
    return start && *start && *start != '}' && !IspaSyntax::isIspaMatch(start);
}
ISPA_STD::ExternalResult<RawStatements::node_type> RawStatements::parse(const char* origin, const char* start) {
    start = blockStart(start);
    if (start && start[0] == 'p' && start[1] == '{')
        throw std::runtime_error("Python action at line " + std::to_string(1 + std::count(origin, start, '\n')) +
            ": use direct Python in advanced rules and fail blocks; p{...} wrappers are unsupported");
    const char* end = IspaSyntax::scanPythonStatements(start, origin);
    if (end == start) return {};
    const char* line = start;
    while (line != origin && line[-1] != '\n' && line[-1] != '\r') --line;
    std::size_t indent = 0;
    while (*line == ' ' || *line == '\t') {
        indent = *line++ == '\t' ? (indent / 8 + 1) * 8 : indent + 1;
    }
    PyLexer lexer;
    lexer.setBoundary(end);
    lexer.setBaseIndent(indent);
    lexer.setIspaMatchEnabled(true);
    Parser parser;
    auto result = parser.main(PyLexer::lazy_iterator(lexer, origin, start));
    if (!result.status || !result.it.isEnd() || !parser.getErrors().empty() || !parser.getContextErrors().empty()) {
        std::string message = "invalid Python action";
        auto location = start;
        if (!parser.getErrors().empty()) {
            const auto& error = parser.getErrors().front();
            message += ": " + error.message;
            if (error.offset != std::string::npos && error.offset <= std::char_traits<char>::length(origin))
                location = origin + error.offset;
        }
        throw std::runtime_error("Python action at line " + std::to_string(1 + std::count(origin, location, '\n')) + ": " + message);
    }
    return {true, ISPA_STD::external_node(origin, start, end, std::move(result.node)), end};
}
}
