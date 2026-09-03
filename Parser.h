#ifndef PARSER_H
#define PARSER_H
#include <string>
#include <vector>
#include <unordered_map>
#include <array>
#include <string>
#include <variant>
#include <optional>
#include <memory>
#include <ispastdlib.hpp>
namespace Parser {
	enum class Tokens {
		NONE,
		__WHITESPACE,
		AUTO_29,
		AUTO_27,
		AUTO_26,
		AUTO_25,
		AUTO_24,
		AUTO_23,
		AUTO_22,
		AUTO_20,
		AUTO_18,
		AUTO_17,
		AUTO_28,
		AUTO_19,
		AUTO_16,
		AUTO_15,
		AUTO_14,
		AUTO_10,
		AUTO_8,
		AUTO_6,
		rule_BIN,
		rule_HEX,
		rule_ESCAPED,
		rule_NOSPACE,
		BOOLEAN,
		AUTO_1,
		rule_CSEQUENCE_ESCAPE,
		rule_CSEQUENCE_DIAPASON,
		rule_CSEQUENCE_SYMBOL,
		AUTO_12,
		AUTO_9,
		AUTO_13,
		AUTO_30,
		AUTO_3,
		AUTO_7,
		AUTO_21,
		NUMBER,
		ID,
		cll_LOGICAL_NOT,
		MODULO,
		PLUS,
		QUESTION_MARK,
		rule_CSEQUENCE,
		MULTIPLE,
		AT,
		AUTO_11,
		DOT,
		DIVIDE,
		STRING,
		SPACEMODE,
		AUTO_4,
		MINUS,
		cll_ASSIGNMENT_OP,
		cll_COMPARE_OP,
		LINEAR_COMMENT,
		rule_OP,
		cll_LOGICAL_OR,
		AUTO_0,
		cll_LOGICAL_AND,
		cll_LOGICAL_OP,
		AUTO_2,
		NAME,
		AUTO_5,
	};
	enum class Rules {
		NONE,
		cll_expr_value,
		cll_function_arguments,
		cll_method_call,
		moduleImport_from,
		cll_function_parameters,
		rule_quantifier,
		rule_data_block_regular_datablock_key,
		rule_data_block_templated_datablock,
		rule_data_block_regular_datablock,
		cll_function_body_decl,
		rule_group,
		rule_name,
		rule_keyvalue,
		rule_member,
		rule,
		_use_unit,
		main,
		moduleImport_from_import_list,
		cll_type,
		cll_expr_compare,
		moduleDeclaration,
		cll_expr_group,
		array,
		rvalue,
		cll__variable,
		object,
		cll_stmt,
		cll__var,
		cll_expr_logical,
		cll_expr_term,
		cll_expr,
		cll_function_call,
		rule_value,
		rule_data_block,
		moduleImport,
		cll,
		rule_nested_rule,
		cll_function_body_call,
		cll_expr_arithmetic,
		cll__if,
		_use,
		cll_templ,
		cll_loop_for,
		cll_loop_while,
	};
	namespace FlatTypes {
		struct cll_expr_value;
		struct cll_stmt;
		struct cll_function_call;
		struct cll_expr_compare;
		struct cll__var;
		struct cll_type;
		struct _use_unit;
		struct rule_member;
		struct main;
		struct cll_expr_term;
		struct cll_expr;
		struct rvalue;
		struct rule;
		struct cll_expr_arithmetic;
		struct __WHITESPACE {
		};
		struct AUTO_29 {
			std::string value;
		};
		struct AUTO_27 {
			std::string value;
		};
		struct AUTO_26 {
			char value;
		};
		struct AUTO_25 {
			char value;
		};
		struct AUTO_24 {
			std::string value;
		};
		struct AUTO_23 {
			std::string value;
		};
		struct AUTO_22 {
			char value;
		};
		struct AUTO_20 {
			char value;
		};
		struct AUTO_18 {
			char value;
		};
		struct AUTO_17 {
			std::string value;
		};
		struct AUTO_28 {
			std::string value;
		};
		struct AUTO_19 {
			char value;
		};
		struct AUTO_16 {
			std::string value;
		};
		struct AUTO_15 {
			std::string value;
		};
		struct AUTO_14 {
			std::string value;
		};
		struct AUTO_10 {
			std::string value;
		};
		struct AUTO_8 {
			char value;
		};
		struct AUTO_6 {
			char value;
		};
		struct rule_BIN {
			std::string value;
		};
		struct rule_HEX {
			std::string value;
		};
		struct rule_ESCAPED {
			char value;
		};
		struct rule_NOSPACE {
		};
		struct BOOLEAN {
			std::string value;
		};
		struct AUTO_1 {
			char value;
		};
		struct rule_CSEQUENCE_ESCAPE {
			char value;
		};
		struct rule_CSEQUENCE_SYMBOL {
			char value;
		};
		struct AUTO_12 {
			std::string value;
		};
		struct AUTO_9 {
			std::string value;
		};
		struct AUTO_13 {
			std::string value;
		};
		struct AUTO_30 {
			std::string value;
		};
		struct AUTO_3 {
			char value;
		};
		struct AUTO_7 {
			char value;
		};
		struct AUTO_21 {
			std::string value;
		};
		struct NUMBER {
			char sign;
			std::string main;
			std::string dec;
		};
		struct ID {
			std::string value;
		};
		struct cll_LOGICAL_NOT {
		};
		struct MODULO {
		};
		struct PLUS {
		};
		struct QUESTION_MARK {
		};
		struct rule_CSEQUENCE {
			char _not;
			char val;
		};
		struct MULTIPLE {
		};
		struct AT {
		};
		struct AUTO_11 {
			std::string value;
		};
		struct DOT {
		};
		struct DIVIDE {
		};
		struct STRING {
			std::variant<std::monostate, char, std::string> value;
		};
		struct SPACEMODE {
			std::string value;
		};
		struct AUTO_4 {
			char value;
		};
		struct MINUS {
		};
		struct cll_ASSIGNMENT_OP {
			std::string value;
		};
		struct cll_COMPARE_OP {
			std::variant<std::monostate, char, std::string> value;
		};
		struct LINEAR_COMMENT {
		};
		struct rule_OP {
		};
		struct cll_LOGICAL_OR {
		};
		struct AUTO_0 {
			char value;
		};
		struct cll_LOGICAL_AND {
		};
		struct AUTO_2 {
			char value;
		};
		struct AUTO_5 {
			char value;
		};
		struct rule_CSEQUENCE_DIAPASON {
			::ISPA_STD::Node<Tokens, rule_CSEQUENCE_SYMBOL> begin;
			std::string end;
		};
		struct rule_data_block_templated_datablock {
			::ISPA_STD::Node<Tokens, ID> first_name;
			std::vector<::ISPA_STD::Node<Tokens, ID>> second_name;
		};
		struct rule_keyvalue {
			::ISPA_STD::Node<Tokens, ID> value;
		};
		struct cll_function_parameters {
			::ISPA_STD::Node<Tokens, ID> first;
			std::vector<::ISPA_STD::Node<Tokens, ID>> second;
		};
		struct NAME {
			::ISPA_STD::Node<Tokens, ID> value;
		};
		struct rule_name {
			::ISPA_STD::Node<Tokens, AUTO_6> is_nested;
			::ISPA_STD::Node<Tokens, ID> name;
			std::vector<::ISPA_STD::Node<Tokens, ID>> nested_name;
		};
		struct rule_value {
			::ISPA_STD::Node<Tokens, ID> value;
		};
		struct moduleImport_from_import_list {
			::ISPA_STD::Node<Tokens, ID> first;
			std::vector<::ISPA_STD::Node<Tokens, ID>> sequence;
		};
		struct moduleDeclaration {
			::ISPA_STD::Node<Tokens, ID> name;
			::ISPA_STD::Node<Tokens, ID> base;
		};
		struct rule_quantifier {
			std::variant<std::monostate, ::ISPA_STD::Node<Tokens, MULTIPLE>, ::ISPA_STD::Node<Tokens, PLUS>, ::ISPA_STD::Node<Tokens, QUESTION_MARK>> value;
		};
		struct cll_LOGICAL_OP {
			std::variant<std::monostate, ::ISPA_STD::Node<Tokens, cll_LOGICAL_AND>, ::ISPA_STD::Node<Tokens, cll_LOGICAL_OR>> value;
		};
		struct cll_function_body_decl {
			::ISPA_STD::Node<Rules, cll_function_parameters> value;
		};
		struct moduleImport_from {
			std::variant<std::monostate, ::ISPA_STD::Node<Rules, moduleImport_from_import_list>, ::ISPA_STD::Node<Tokens, MULTIPLE>> what;
			::ISPA_STD::Node<Tokens, ID> from;
		};
		struct moduleImport {
			std::variant<std::monostate, ::ISPA_STD::Node<Rules, moduleImport_from>, ::ISPA_STD::Node<Tokens, ID>> value;
		};
		struct cll__if {
			::ISPA_STD::MatchResult<Rules, std::unique_ptr<cll_expr>> expr;
			::ISPA_STD::MatchResult<Rules, std::unique_ptr<cll_stmt>> stmt;
		};
		struct cll_function_arguments {
			::ISPA_STD::MatchResult<Rules, std::unique_ptr<cll_expr>> first;
			std::vector<::ISPA_STD::MatchResult<Rules, std::unique_ptr<cll_expr>>> second;
		};
		struct cll_method_call {
			::ISPA_STD::Node<Tokens, ID> name;
			::ISPA_STD::MatchResult<Rules, std::unique_ptr<cll_function_call>> body;
		};
		struct _use {
			::ISPA_STD::MatchResult<Rules, std::unique_ptr<_use_unit>> first;
			std::vector<::ISPA_STD::MatchResult<Rules, std::unique_ptr<_use_unit>>> second;
		};
		struct rule_group {
			std::vector<::ISPA_STD::MatchResult<Rules, std::unique_ptr<rule_member>>> value;
		};
		struct cll_loop_for {
			std::variant<std::monostate, ::ISPA_STD::Node<Rules, std::unique_ptr<cll__var>>, ::ISPA_STD::Node<Rules, std::unique_ptr<cll_expr>>> decl;
			::ISPA_STD::MatchResult<Rules, std::unique_ptr<cll_expr>> cond;
			::ISPA_STD::MatchResult<Rules, std::unique_ptr<cll_expr>> end;
			::ISPA_STD::MatchResult<Rules, std::unique_ptr<cll_stmt>> stmt;
		};
		struct rule_nested_rule {
			::ISPA_STD::Node<Rules, std::unique_ptr<rule>> value;
		};
		struct cll__variable {
			std::variant<std::monostate, ::ISPA_STD::Node<Tokens, AUTO_30>, ::ISPA_STD::Node<Tokens, AUTO_29>> pre;
			::ISPA_STD::Node<Tokens, ID> name;
			::ISPA_STD::MatchResult<Rules, std::unique_ptr<cll_expr>> brace_expression;
			std::variant<std::monostate, ::ISPA_STD::Node<Tokens, AUTO_30>, ::ISPA_STD::Node<Tokens, AUTO_29>> pos;
		};
		struct cll_expr_logical {
			::ISPA_STD::MatchResult<Rules, std::unique_ptr<cll_expr_compare>> left;
			std::vector<::ISPA_STD::Node<Tokens, cll_LOGICAL_OP>> op;
			std::vector<::ISPA_STD::MatchResult<Rules, std::unique_ptr<cll_expr_compare>>> right;
		};
		struct object {
			std::variant<std::monostate, ::ISPA_STD::Node<Tokens, NUMBER>, ::ISPA_STD::Node<Tokens, ID>> key;
			::ISPA_STD::MatchResult<Rules, std::unique_ptr<cll_expr>> value;
			std::variant<std::monostate, ::ISPA_STD::Node<Tokens, NUMBER>, ::ISPA_STD::Node<Tokens, ID>> keys;
			std::vector<::ISPA_STD::MatchResult<Rules, std::unique_ptr<cll_expr>>> values;
		};
		struct rule_data_block_regular_datablock_key {
			::ISPA_STD::Node<Tokens, ID> name;
			::ISPA_STD::MatchResult<Rules, std::unique_ptr<cll_expr>> dt;
		};
		struct cll_loop_while {
			::ISPA_STD::MatchResult<Rules, std::unique_ptr<cll_expr>> expr;
			::ISPA_STD::MatchResult<Rules, std::unique_ptr<cll_stmt>> stmt;
		};
		struct cll_templ {
			::ISPA_STD::MatchResult<Rules, std::unique_ptr<cll_type>> first;
			std::vector<::ISPA_STD::MatchResult<Rules, std::unique_ptr<cll_type>>> second;
		};
		struct cll_expr_group {
			::ISPA_STD::Node<Rules, std::unique_ptr<cll_expr>> value;
		};
		struct array {
			std::vector<::ISPA_STD::MatchResult<Rules, std::unique_ptr<cll_expr>>> value;
		};
		struct cll_function_body_call {
			::ISPA_STD::Node<Rules, cll_function_arguments> value;
		};
		struct rule_data_block_regular_datablock {
			std::variant<std::monostate, ::ISPA_STD::Node<Rules, rule_data_block_regular_datablock_key>, ::ISPA_STD::Node<Rules, std::unique_ptr<cll_expr>>> value;
		};
		struct cll {
			std::variant<std::monostate, ::ISPA_STD::Node<Rules, std::unique_ptr<cll_expr>>, ::ISPA_STD::Node<Rules, cll_loop_for>, ::ISPA_STD::Node<Rules, cll__if>, ::ISPA_STD::Node<Rules, std::unique_ptr<cll__var>>, ::ISPA_STD::Node<Rules, cll_loop_while>> value;
		};
		struct rule_data_block {
			std::variant<std::monostate, ::ISPA_STD::Node<Rules, rule_data_block_templated_datablock>, ::ISPA_STD::Node<Rules, rule_data_block_regular_datablock>> value;
		};
		struct cll_expr_value {
			std::variant<std::monostate, ::ISPA_STD::Node<Rules, std::unique_ptr<cll_function_call>>, ::ISPA_STD::Node<Rules, cll_method_call>, ::ISPA_STD::Node<Rules, cll_expr_group>, ::ISPA_STD::Node<Rules, std::unique_ptr<rvalue>>, ::ISPA_STD::Node<Rules, cll__variable>> value;
		};
		struct rule_member {
			std::variant<std::monostate, ::ISPA_STD::Node<Rules, rule_value>, ::ISPA_STD::Node<Rules, rule_keyvalue>> prefix;
			std::variant<std::monostate, ::ISPA_STD::Node<Rules, rule_name>, ::ISPA_STD::Node<Tokens, rule_CSEQUENCE>, ::ISPA_STD::Node<Rules, cll>, ::ISPA_STD::Node<Tokens, DOT>, ::ISPA_STD::Node<Rules, rule_group>, ::ISPA_STD::Node<Tokens, rule_OP>, ::ISPA_STD::Node<Tokens, LINEAR_COMMENT>, ::ISPA_STD::Node<Tokens, rule_BIN>, ::ISPA_STD::Node<Tokens, STRING>, ::ISPA_STD::Node<Tokens, rule_HEX>, ::ISPA_STD::Node<Tokens, rule_ESCAPED>, ::ISPA_STD::Node<Tokens, rule_NOSPACE>> val;
			::ISPA_STD::MatchResult<Rules, rule_quantifier> quantifier;
		};
		struct rule {
			::ISPA_STD::Node<Tokens, ID> name;
			std::vector<::ISPA_STD::MatchResult<Rules, std::unique_ptr<rule_member>>> rule;
			::ISPA_STD::MatchResult<Rules, rule_data_block> data_block;
			std::vector<::ISPA_STD::MatchResult<Rules, rule_nested_rule>> nested_rules;
		};
		struct _use_unit {
			::ISPA_STD::Node<Tokens, ID> name;
			::ISPA_STD::MatchResult<Rules, std::unique_ptr<rvalue>> value;
		};
		struct main {
			std::variant<std::monostate, ::ISPA_STD::Node<Rules, std::unique_ptr<rule>>, ::ISPA_STD::Node<Rules, _use>, ::ISPA_STD::Node<Tokens, NAME>, ::ISPA_STD::Node<Tokens, SPACEMODE>> value;
		};
		struct cll_type {
			::ISPA_STD::Node<Tokens, AUTO_13> type;
			::ISPA_STD::MatchResult<Rules, cll_templ> templ;
		};
		struct cll_expr_compare {
			::ISPA_STD::MatchResult<Rules, std::unique_ptr<cll_expr_arithmetic>> first;
			std::vector<::ISPA_STD::Node<Tokens, cll_COMPARE_OP>> operators;
			std::vector<::ISPA_STD::MatchResult<Rules, std::unique_ptr<cll_expr_arithmetic>>> sequence;
		};
		struct rvalue {
			std::variant<std::monostate, ::ISPA_STD::Node<Tokens, ID>, ::ISPA_STD::Node<Tokens, NUMBER>, ::ISPA_STD::Node<Tokens, STRING>, ::ISPA_STD::Node<Tokens, AT>, ::ISPA_STD::Node<Rules, array>, ::ISPA_STD::Node<Rules, object>, ::ISPA_STD::Node<Tokens, BOOLEAN>> value;
		};
		struct cll_stmt {
			std::vector<::ISPA_STD::MatchResult<Rules, std::unique_ptr<rule_member>>> value;
		};
		struct cll__var {
			::ISPA_STD::MatchResult<Rules, std::unique_ptr<cll_type>> type;
			::ISPA_STD::Node<Tokens, ID> id;
			::ISPA_STD::Node<Tokens, cll_ASSIGNMENT_OP> op;
			::ISPA_STD::MatchResult<Rules, std::unique_ptr<cll_expr>> value;
		};
		struct cll_expr_term {
			::ISPA_STD::MatchResult<Rules, std::unique_ptr<cll_expr_value>> first;
			std::variant<std::monostate, ::ISPA_STD::Node<Tokens, MULTIPLE>, ::ISPA_STD::Node<Tokens, MODULO>, ::ISPA_STD::Node<Tokens, DIVIDE>> operators;
			std::vector<::ISPA_STD::MatchResult<Rules, std::unique_ptr<cll_expr_value>>> sequence;
		};
		struct cll_expr {
			::ISPA_STD::Node<Rules, cll_expr_logical> value;
		};
		struct cll_function_call {
			::ISPA_STD::Node<Tokens, ID> name;
			::ISPA_STD::MatchResult<Rules, cll_function_body_call> body;
		};
		struct cll_expr_arithmetic {
			::ISPA_STD::MatchResult<Rules, std::unique_ptr<cll_expr_term>> first;
			std::variant<std::monostate, ::ISPA_STD::Node<Tokens, PLUS>, ::ISPA_STD::Node<Tokens, MINUS>> operators;
			std::vector<::ISPA_STD::MatchResult<Rules, std::unique_ptr<cll_expr_term>>> sequence;
		};
	}
	namespace Types {
		using AT = FlatTypes::AT;
		using AUTO_0 = FlatTypes::AUTO_0;
		using AUTO_1 = FlatTypes::AUTO_1;
		using AUTO_10 = FlatTypes::AUTO_10;
		using AUTO_11 = FlatTypes::AUTO_11;
		using AUTO_12 = FlatTypes::AUTO_12;
		using AUTO_13 = FlatTypes::AUTO_13;
		using AUTO_14 = FlatTypes::AUTO_14;
		using AUTO_15 = FlatTypes::AUTO_15;
		using AUTO_16 = FlatTypes::AUTO_16;
		using AUTO_17 = FlatTypes::AUTO_17;
		using AUTO_18 = FlatTypes::AUTO_18;
		using AUTO_19 = FlatTypes::AUTO_19;
		using AUTO_2 = FlatTypes::AUTO_2;
		using AUTO_20 = FlatTypes::AUTO_20;
		using AUTO_21 = FlatTypes::AUTO_21;
		using AUTO_22 = FlatTypes::AUTO_22;
		using AUTO_23 = FlatTypes::AUTO_23;
		using AUTO_24 = FlatTypes::AUTO_24;
		using AUTO_25 = FlatTypes::AUTO_25;
		using AUTO_26 = FlatTypes::AUTO_26;
		using AUTO_27 = FlatTypes::AUTO_27;
		using AUTO_28 = FlatTypes::AUTO_28;
		using AUTO_29 = FlatTypes::AUTO_29;
		using AUTO_3 = FlatTypes::AUTO_3;
		using AUTO_30 = FlatTypes::AUTO_30;
		using AUTO_4 = FlatTypes::AUTO_4;
		using AUTO_5 = FlatTypes::AUTO_5;
		using AUTO_6 = FlatTypes::AUTO_6;
		using AUTO_7 = FlatTypes::AUTO_7;
		using AUTO_8 = FlatTypes::AUTO_8;
		using AUTO_9 = FlatTypes::AUTO_9;
		using BOOLEAN = FlatTypes::BOOLEAN;
		using DIVIDE = FlatTypes::DIVIDE;
		using DOT = FlatTypes::DOT;
		using ID = FlatTypes::ID;
		using LINEAR_COMMENT = FlatTypes::LINEAR_COMMENT;
		using MINUS = FlatTypes::MINUS;
		using MODULO = FlatTypes::MODULO;
		using MULTIPLE = FlatTypes::MULTIPLE;
		using NAME = FlatTypes::NAME;
		using NUMBER = FlatTypes::NUMBER;
		using PLUS = FlatTypes::PLUS;
		using QUESTION_MARK = FlatTypes::QUESTION_MARK;
		using SPACEMODE = FlatTypes::SPACEMODE;
		using STRING = FlatTypes::STRING;
		using __WHITESPACE = FlatTypes::__WHITESPACE;
		struct _use : FlatTypes::_use {
			using unit = FlatTypes::_use_unit;
		};
		using array = FlatTypes::array;
		struct cll : FlatTypes::cll {
			using ASSIGNMENT_OP = FlatTypes::cll_ASSIGNMENT_OP;
			using COMPARE_OP = FlatTypes::cll_COMPARE_OP;
			using LOGICAL_AND = FlatTypes::cll_LOGICAL_AND;
			using LOGICAL_NOT = FlatTypes::cll_LOGICAL_NOT;
			using LOGICAL_OP = FlatTypes::cll_LOGICAL_OP;
			using LOGICAL_OR = FlatTypes::cll_LOGICAL_OR;
			using _if = FlatTypes::cll__if;
			using _var = FlatTypes::cll__var;
			using _variable = FlatTypes::cll__variable;
			struct expr : FlatTypes::cll_expr {
				using arithmetic = FlatTypes::cll_expr_arithmetic;
				using compare = FlatTypes::cll_expr_compare;
				using group = FlatTypes::cll_expr_group;
				using logical = FlatTypes::cll_expr_logical;
				using term = FlatTypes::cll_expr_term;
				using value = FlatTypes::cll_expr_value;
			};
			using function_arguments = FlatTypes::cll_function_arguments;
			using function_body_call = FlatTypes::cll_function_body_call;
			using function_body_decl = FlatTypes::cll_function_body_decl;
			using function_call = FlatTypes::cll_function_call;
			using function_parameters = FlatTypes::cll_function_parameters;
			using loop_for = FlatTypes::cll_loop_for;
			using loop_while = FlatTypes::cll_loop_while;
			using method_call = FlatTypes::cll_method_call;
			using stmt = FlatTypes::cll_stmt;
			using templ = FlatTypes::cll_templ;
			using type = FlatTypes::cll_type;
		};
		using main = FlatTypes::main;
		using moduleDeclaration = FlatTypes::moduleDeclaration;
		struct moduleImport : FlatTypes::moduleImport {
			struct from : FlatTypes::moduleImport_from {
				using import_list = FlatTypes::moduleImport_from_import_list;
			};
		};
		using object = FlatTypes::object;
		struct rule : FlatTypes::rule {
			using BIN = FlatTypes::rule_BIN;
			struct CSEQUENCE : FlatTypes::rule_CSEQUENCE {
				using DIAPASON = FlatTypes::rule_CSEQUENCE_DIAPASON;
				using ESCAPE = FlatTypes::rule_CSEQUENCE_ESCAPE;
				using SYMBOL = FlatTypes::rule_CSEQUENCE_SYMBOL;
			};
			using ESCAPED = FlatTypes::rule_ESCAPED;
			using HEX = FlatTypes::rule_HEX;
			using NOSPACE = FlatTypes::rule_NOSPACE;
			using OP = FlatTypes::rule_OP;
			struct data_block : FlatTypes::rule_data_block {
				struct regular_datablock : FlatTypes::rule_data_block_regular_datablock {
					using key = FlatTypes::rule_data_block_regular_datablock_key;
				};
				using templated_datablock = FlatTypes::rule_data_block_templated_datablock;
			};
			using group = FlatTypes::rule_group;
			using keyvalue = FlatTypes::rule_keyvalue;
			using member = FlatTypes::rule_member;
			using name = FlatTypes::rule_name;
			using nested_rule = FlatTypes::rule_nested_rule;
			using quantifier = FlatTypes::rule_quantifier;
			using value = FlatTypes::rule_value;
		};
		using rvalue = FlatTypes::rvalue;
	}
	using Token = std::variant<std::monostate, Types::__WHITESPACE, Types::AUTO_29, Types::AUTO_27, Types::AUTO_26, Types::AUTO_25, Types::AUTO_24, Types::AUTO_23, Types::AUTO_22, Types::AUTO_20, Types::AUTO_18, Types::AUTO_17, Types::AUTO_28, Types::AUTO_19, Types::AUTO_16, Types::AUTO_15, Types::AUTO_14, Types::AUTO_10, Types::AUTO_8, Types::AUTO_6, Types::rule::BIN, Types::rule::HEX, Types::rule::ESCAPED, Types::rule::NOSPACE, Types::BOOLEAN, Types::AUTO_1, Types::rule::CSEQUENCE::ESCAPE, Types::rule::CSEQUENCE::DIAPASON, Types::rule::CSEQUENCE::SYMBOL, Types::AUTO_12, Types::AUTO_9, Types::AUTO_13, Types::AUTO_30, Types::AUTO_3, Types::AUTO_7, Types::AUTO_21, Types::NUMBER, Types::ID, Types::cll::LOGICAL_NOT, Types::MODULO, Types::PLUS, Types::QUESTION_MARK, Types::rule::CSEQUENCE, Types::MULTIPLE, Types::AT, Types::AUTO_11, Types::DOT, Types::DIVIDE, Types::STRING, Types::SPACEMODE, Types::AUTO_4, Types::MINUS, Types::cll::ASSIGNMENT_OP, Types::cll::COMPARE_OP, Types::LINEAR_COMMENT, Types::rule::OP, Types::cll::LOGICAL_OR, Types::AUTO_0, Types::cll::LOGICAL_AND, Types::cll::LOGICAL_OP, Types::AUTO_2, Types::NAME, Types::AUTO_5>;
	class Lexer : public ::ISPA_STD::Lexer_base<Tokens, Token> {
		static ::ISPA_STD::DFA::API::CharToClass char_class_table;
		static ::ISPA_STD::DFA::API::Table<249, 256> dfa_table;
		static ::ISPA_STD::DFA::API::Table<32937, 2> lr_table;
		static auto semantic_action_exec(int state, std::vector<std::variant<std::monostate, Token, char, std::string>>& values, std::vector<std::vector<std::variant<std::monostate, Token, char, std::string>>>& vec_values) -> std::pair<int, Token>;
		bool init_done;
		auto init() -> void override;
		std::vector<std::variant<std::monostate, Token, char, std::string>> values;
		std::vector<std::vector<std::variant<std::monostate, Token, char, std::string>>> vec_values;
		std::array<const char*, 8> registers;
	public: 
		auto makeToken(const char*& pos) -> Token override;
	};
}
#endif // PARSER_H
