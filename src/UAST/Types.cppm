module;
#include <output/Parser.h>
#include <PyRawParser.h>
export module UAST;
export import UAST.PyAST;
import std;

// The UAST *is* the ISPA-generated parser tree (Parser::Types); nothing is
// copied. These aliases only give the structs their short, stable UAST names, so
// the rest of the code base does not depend on the generator's namespaces.
//
// A UAST::Node<T> is the generated ::ISPA_STD::Node<T>: it carries the offset,
// length, line and column of the match (see positionOf) but not the file name.
//
// Python actions (`expr` / `stmt`) stay as raw text here; AST.Builder parses them
// into the typed UAST::py tree through UAST.Python when it needs them.
export namespace UAST {
template<class T> using Node = ::ISPA_STD::Node<T>;

template<class T> SourcePosition positionOf(const Node<T>& node, const std::string& file = {}) {
    return {file, node.startpos(), node.length(), node.line(), node.column()};
}

// Generic, lossless view of a raw parser tree (node kinds, named fields, sequence
// and variant structure, scalar values, absent nodes, positions). Used by
// describe() and, in UAST.Python, as the intermediate form the typed Python AST is
// built from.
struct PythonNode {
    std::string kind, field, scalar;
    SourcePosition position;
    bool absent = false;
    std::vector<PythonNode> children;

    auto members() { return std::tie(kind, field, scalar, position, absent, children); }
    auto members() const { return std::tie(kind, field, scalar, position, absent, children); }
    bool operator==(const PythonNode& other) const { return members() == other.members(); }
    bool operator<(const PythonNode& other) const { return members() < other.members(); }
};

using __WS = ::Parser::Types::__WS;
using FAIL = ::Parser::Types::modules::fail_block::fail_block::FAIL;
using SEMICOLON = ::Parser::Types::SEMICOLON;
using COLON = ::Parser::Types::COLON;
using EQUALS_SIGN = ::Parser::Types::EQUALS_SIGN;
using AMPERSAND = ::Parser::Types::modules::rule::regular::value::var_capture::AMPERSAND;
using PERCENT_SIGN = ::Parser::Types::PERCENT_SIGN;
using rule_nested_rule_NUMBER_SIGN = ::Parser::Types::modules::rule::regular::nested_rule::NUMBER_SIGN;
using rule_nested_rule = ::Parser::Types::modules::rule::regular::nested_rule;
using LEFT_CURLY_BRACKET = ::Parser::Types::LEFT_CURLY_BRACKET;
using rule_value_shared_counter_registration_AUTO = ::Parser::Types::modules::rule::regular::value::shared_counter_registration::AUTO;
using rule_value = ::Parser::Types::modules::rule::regular::value;
using COMMERCIAL_AT = ::Parser::Types::COMMERCIAL_AT;
using rule_value_QUANTIFIER = ::Parser::Types::modules::rule::regular::value::QUANTIFIER;
using rule_value_NOSPACE = ::Parser::Types::modules::rule::regular::value::NOSPACE;
using rule_value_ANY = ::Parser::Types::modules::rule::regular::value::ANY;
using template_params_LESS_THAN_SIGN = ::Parser::Types::modules::common::template_params::LESS_THAN_SIGN;
using rule_value_STRING = ::Parser::Types::modules::rule::regular::value::STRING;
using grammar_modifier_AUTO = ::Parser::Types::modules::grammar::modifier::AUTO;
using grammar_modifier_MOD = ::Parser::Types::modules::grammar::modifier::MOD;
using rule_value_SPACE = ::Parser::Types::modules::rule::regular::value::SPACE;
using template_params_GREATER_THAN_SIGN = ::Parser::Types::modules::common::template_params::GREATER_THAN_SIGN;
using rule_value_alternative_VERTICAL_LINE = ::Parser::Types::modules::rule::regular::value::alternative::VERTICAL_LINE;
using rule_value_alternative = ::Parser::Types::modules::rule::regular::value::alternative;
using grammar_modifier_RIGHT_SQUARE_BRACKET = ::Parser::Types::modules::grammar::modifier::RIGHT_SQUARE_BRACKET;
using rule_value_dyn_DOLLAR_SIGN = ::Parser::Types::modules::rule::regular::value::dyn::DOLLAR_SIGN;
using advanced_rule_regular_match_MATCH = ::Parser::Types::modules::rule::advanced::advanced_rule::regular_match::MATCH;
using COMMA = ::Parser::Types::COMMA;
using RIGHT_PARENTHESIS = ::Parser::Types::RIGHT_PARENTHESIS;
using LEFT_PARENTHESIS = ::Parser::Types::LEFT_PARENTHESIS;
using RIGHT_CURLY_BRACKET = ::Parser::Types::RIGHT_CURLY_BRACKET;
#ifndef ISPA_EXTERNAL_PYTHON
using PY_RAW = ::Parser::Types::modules::common::PY_RAW;
#endif
using ExternalPythonExpression = ::PyRawParser::Types::expr;
using ExternalPythonStatements = ::PyRawParser::Types::main;
using ID = ::Parser::Types::modules::common::ID;
using rule_value_CGROUP = ::Parser::Types::modules::rule::regular::value::CGROUP;
using rule_value_group = ::Parser::Types::modules::rule::regular::value::group;
using rule_value_mul_capture = ::Parser::Types::modules::rule::regular::value::mul_capture;
using grammar_modifier = ::Parser::Types::modules::grammar::modifier;
using expr = ::Parser::Types::modules::ispa_python::expr;
using stmt = ::Parser::Types::modules::ispa_python::stmt;
using rule_value_shared_counter_registration = ::Parser::Types::modules::rule::regular::value::shared_counter_registration;
using advanced_rule_regular_match_unpack = ::Parser::Types::modules::rule::advanced::advanced_rule::regular_match::unpack;
using param = ::Parser::Types::modules::common::param;
using rule_ast_abbr = ::Parser::Types::modules::rule::regular::ast::abbr;
using rule_f_binding = ::Parser::Types::modules::rule::regular::f_binding;
using rule_value_ast_capture = ::Parser::Types::modules::rule::regular::value::ast_capture;
using rule_value_var_capture = ::Parser::Types::modules::rule::regular::value::var_capture;
using rule_ast_full_part = ::Parser::Types::modules::rule::regular::ast::full::part;
using rule_ast_single_value = ::Parser::Types::modules::rule::regular::ast::single_value;
using rule_value_counter_bounded = ::Parser::Types::modules::rule::regular::value::counter::bounded;
using rule_value_dyn_dyn_expr = ::Parser::Types::modules::rule::regular::value::dyn::dyn_expr;
using advanced_rule_regular_match = ::Parser::Types::modules::rule::advanced::advanced_rule::regular_match;
using template_params = ::Parser::Types::modules::common::template_params;
using call_params = ::Parser::Types::modules::common::call_params;
using rule_params = ::Parser::Types::modules::common::rule_params;
using rule_ast_full = ::Parser::Types::modules::rule::regular::ast::full;
using rule_value_counter = ::Parser::Types::modules::rule::regular::value::counter;
using rule_value_dyn = ::Parser::Types::modules::rule::regular::value::dyn;
using rule_value_not_a = ::Parser::Types::modules::rule::regular::value::not_a;
using advanced_rule = ::Parser::Types::modules::rule::advanced::advanced_rule;
using rule_name = ::Parser::Types::modules::common::name<Parser::Types::modules::common::rule_params>;
using call_name = ::Parser::Types::modules::common::name<Parser::Types::modules::common::call_params>;
using rule_ast = ::Parser::Types::modules::rule::regular::ast;
using grammar = ::Parser::Types::modules::grammar::grammar;
using fail_block = ::Parser::Types::modules::fail_block::fail_block;
using rule_value_fail_capture = ::Parser::Types::modules::rule::regular::value::fail_capture;
using rule_value_pattern = ::Parser::Types::modules::rule::regular::value::pattern;
using rule = ::Parser::Types::modules::rule::regular;
using main = ::Parser::Types::modules::main::main;
using rule_value_one_capture = ::Parser::Types::modules::rule::regular::value::one_capture;
}
