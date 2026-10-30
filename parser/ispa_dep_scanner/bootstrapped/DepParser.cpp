#include "DepParser.h"

#include <ispa_core/variant_io.hpp>
#include <ispa_core/assign.hpp>
#include <ispa_core/to_string.hpp>
#include <ispa_core/parser_ll.hpp>

auto DepParser::TokensToString(std::size_t id) -> const char* {
	switch (id) {
		case Tokens::__WS::id: {
			return TokensStr::__WS::id;
			break;
		}
		case Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id: {
			return TokensStr::modules::ispa_dep_scanner::mod_module::MODULE::id;
			break;
		}
		case Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id: {
			return TokensStr::modules::ispa_dep_scanner::mod_detach::DETACH::id;
			break;
		}
		case Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id: {
			return TokensStr::modules::ispa_dep_scanner::mod_import::IMPORT::id;
			break;
		}
		case Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id: {
			return TokensStr::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id;
			break;
		}
		case Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id: {
			return TokensStr::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id;
			break;
		}
		case Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id: {
			return TokensStr::modules::ispa_dep_scanner::parentheses::AUTO_1::id;
			break;
		}
		case Tokens::AUTO::id: {
			return TokensStr::AUTO::id;
			break;
		}
		case Tokens::RIGHT_CURLY_BRACKET::id: {
			return TokensStr::RIGHT_CURLY_BRACKET::id;
			break;
		}
		case Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id: {
			return TokensStr::modules::ispa_dep_scanner::fail_decl::FAIL::id;
			break;
		}
		case Tokens::LEFT_CURLY_BRACKET::id: {
			return TokensStr::LEFT_CURLY_BRACKET::id;
			break;
		}
		case Tokens::FULL_STOP::id: {
			return TokensStr::FULL_STOP::id;
			break;
		}
		case Tokens::modules::scanner_tokens::OTHER::id: {
			return TokensStr::modules::scanner_tokens::OTHER::id;
			break;
		}
		case Tokens::GREATER_THAN_SIGN::id: {
			return TokensStr::GREATER_THAN_SIGN::id;
			break;
		}
		case Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id: {
			return TokensStr::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id;
			break;
		}
		case Tokens::LESS_THAN_SIGN::id: {
			return TokensStr::LESS_THAN_SIGN::id;
			break;
		}
		case Tokens::RIGHT_PARENTHESIS::id: {
			return TokensStr::RIGHT_PARENTHESIS::id;
			break;
		}
		case Tokens::modules::common::NUMBER::id: {
			return TokensStr::modules::common::NUMBER::id;
			break;
		}
		case Tokens::COMMA::id: {
			return TokensStr::COMMA::id;
			break;
		}
		case Tokens::modules::scanner_tokens::STRING::id: {
			return TokensStr::modules::scanner_tokens::STRING::id;
			break;
		}
		case Tokens::FEATURE::id: {
			return TokensStr::FEATURE::id;
			break;
		}
		case Tokens::SEMICOLON::id: {
			return TokensStr::SEMICOLON::id;
			break;
		}
		case Tokens::AUTO_1::id: {
			return TokensStr::AUTO_1::id;
			break;
		}
		case Tokens::modules::scanner_tokens::COMMENT::id: {
			return TokensStr::modules::scanner_tokens::COMMENT::id;
			break;
		}
		case Tokens::ASTERISK::id: {
			return TokensStr::ASTERISK::id;
			break;
		}
		case Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id: {
			return TokensStr::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id;
			break;
		}
		case Tokens::modules::scanner_tokens::CGROUP::id: {
			return TokensStr::modules::scanner_tokens::CGROUP::id;
			break;
		}
		case Tokens::modules::ispa_dep_scanner::mod_import::PY::id: {
			return TokensStr::modules::ispa_dep_scanner::mod_import::PY::id;
			break;
		}
		case Tokens::COLON::id: {
			return TokensStr::COLON::id;
			break;
		}
		case Tokens::NUMBER_SIGN::id: {
			return TokensStr::NUMBER_SIGN::id;
			break;
		}
		case Tokens::modules::common::ID::id: {
			return TokensStr::modules::common::ID::id;
			break;
		}
	}
	throw "Invalid Tokens ID";
}
auto DepParser::RulesToString(std::size_t id) -> const char* {
	switch (id) {
		case Rules::modules::ispa_dep_scanner::mod_detach::detach_value::id: {
			return RulesStr::modules::ispa_dep_scanner::mod_detach::detach_value::id;
			break;
		}
		case Rules::modules::common::param::id: {
			return RulesStr::modules::common::param::id;
			break;
		}
		case Rules::modules::ispa_dep_scanner::mod_import::import_value::from_import::id: {
			return RulesStr::modules::ispa_dep_scanner::mod_import::import_value::from_import::id;
			break;
		}
		case Rules::modules::ispa_dep_scanner::atom::id: {
			return RulesStr::modules::ispa_dep_scanner::atom::id;
			break;
		}
		case Rules::modules::ispa_dep_scanner::braces::id: {
			return RulesStr::modules::ispa_dep_scanner::braces::id;
			break;
		}
		case Rules::modules::ispa_dep_scanner::symbol_list::id: {
			return RulesStr::modules::ispa_dep_scanner::symbol_list::id;
			break;
		}
		case Rules::modules::ispa_dep_scanner::parentheses::id: {
			return RulesStr::modules::ispa_dep_scanner::parentheses::id;
			break;
		}
		case Rules::modules::ispa_dep_scanner::nested_rule::id: {
			return RulesStr::modules::ispa_dep_scanner::nested_rule::id;
			break;
		}
		case Rules::modules::ispa_dep_scanner::body::regular::id: {
			return RulesStr::modules::ispa_dep_scanner::body::regular::id;
			break;
		}
		case Rules::modules::ispa_dep_scanner::body::id: {
			return RulesStr::modules::ispa_dep_scanner::body::id;
			break;
		}
		case Rules::modules::ispa_dep_scanner::fail_decl::id: {
			return RulesStr::modules::ispa_dep_scanner::fail_decl::id;
			break;
		}
		case Rules::modules::ispa_dep_scanner::declaration::form::id: {
			return RulesStr::modules::ispa_dep_scanner::declaration::form::id;
			break;
		}
		case Rules::modules::ispa_dep_scanner::declaration::id: {
			return RulesStr::modules::ispa_dep_scanner::declaration::id;
			break;
		}
		case Rules::modules::ispa_dep_scanner::member::id: {
			return RulesStr::modules::ispa_dep_scanner::member::id;
			break;
		}
		case Rules::modules::ispa_dep_scanner::source_name::id: {
			return RulesStr::modules::ispa_dep_scanner::source_name::id;
			break;
		}
		case Rules::modules::ispa_dep_scanner::mod_detach::detach_value::feature_ref::id: {
			return RulesStr::modules::ispa_dep_scanner::mod_detach::detach_value::feature_ref::id;
			break;
		}
		case Rules::modules::ispa_dep_scanner::modifier::id: {
			return RulesStr::modules::ispa_dep_scanner::modifier::id;
			break;
		}
		case Rules::modules::ispa_dep_scanner::mod_import::import_value::module_import::id: {
			return RulesStr::modules::ispa_dep_scanner::mod_import::import_value::module_import::id;
			break;
		}
		case Rules::modules::common::name::id: {
			return RulesStr::modules::common::name::id;
			break;
		}
		case Rules::modules::ispa_dep_scanner::mod_import::import_value::id: {
			return RulesStr::modules::ispa_dep_scanner::mod_import::import_value::id;
			break;
		}
		case Rules::modules::ispa_dep_scanner::mod_import::external_import::id: {
			return RulesStr::modules::ispa_dep_scanner::mod_import::external_import::id;
			break;
		}
		case Rules::modules::ispa_dep_scanner::mod_import::id: {
			return RulesStr::modules::ispa_dep_scanner::mod_import::id;
			break;
		}
		case Rules::modules::common::rule_params::id: {
			return RulesStr::modules::common::rule_params::id;
			break;
		}
		case Rules::modules::common::type_annotation::id: {
			return RulesStr::modules::common::type_annotation::id;
			break;
		}
		case Rules::modules::ispa_dep_scanner::module_path::id: {
			return RulesStr::modules::ispa_dep_scanner::module_path::id;
			break;
		}
		case Rules::modules::entry::main::id: {
			return RulesStr::modules::entry::main::id;
			break;
		}
		case Rules::modules::ispa_dep_scanner::feature_decl::id: {
			return RulesStr::modules::ispa_dep_scanner::feature_decl::id;
			break;
		}
		case Rules::modules::common::template_params::id: {
			return RulesStr::modules::common::template_params::id;
			break;
		}
		case Rules::modules::ispa_dep_scanner::mod_import::import_value::from_import::selection::items::id: {
			return RulesStr::modules::ispa_dep_scanner::mod_import::import_value::from_import::selection::items::id;
			break;
		}
		case Rules::modules::common::template_param::id: {
			return RulesStr::modules::common::template_param::id;
			break;
		}
		case Rules::modules::ispa_dep_scanner::mod_detach::id: {
			return RulesStr::modules::ispa_dep_scanner::mod_detach::id;
			break;
		}
		case Rules::modules::ispa_dep_scanner::mod_module::id: {
			return RulesStr::modules::ispa_dep_scanner::mod_module::id;
			break;
		}
		case Rules::modules::common::parameter::id: {
			return RulesStr::modules::common::parameter::id;
			break;
		}
		case Rules::main::id: {
			return RulesStr::main::id;
			break;
		}
		case Rules::modules::ispa_dep_scanner::mod_import::import_value::from_import::selection::id: {
			return RulesStr::modules::ispa_dep_scanner::mod_import::import_value::from_import::selection::id;
			break;
		}
		case Rules::modules::ispa_dep_scanner::balanced_item::id: {
			return RulesStr::modules::ispa_dep_scanner::balanced_item::id;
			break;
		}
	}
	throw "Invalid Rules ID";
}
auto DepParser::FlatTypes::__WS::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("__WS", true);
}
auto DepParser::FlatTypes::__WS::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::__WS::to_string() const -> std::string {
	return ::ISPA_CORE::concat("__WS {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, __WS value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_module_MODULE::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_mod_module_MODULE", true);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_module_MODULE::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_module_MODULE::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_mod_module_MODULE {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_mod_module_MODULE value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules", true);
}
auto DepParser::FlatTypes::modules::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner", true);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_detach_DETACH::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_mod_detach_DETACH", true);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_detach_DETACH::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_detach_DETACH::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_mod_detach_DETACH {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_mod_detach_DETACH value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_IMPORT::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_mod_import_IMPORT", true);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_IMPORT::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_IMPORT::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_mod_import_IMPORT {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_mod_import_IMPORT value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_import_value_module_import_AS::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_mod_import_import_value_module_import_AS", true);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_import_value_module_import_AS::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_import_value_module_import_AS::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_mod_import_import_value_module_import_AS {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_mod_import_import_value_module_import_AS value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_import_value_from_import_FROM::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_mod_import_import_value_from_import_FROM", true);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_import_value_from_import_FROM::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_import_value_from_import_FROM::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_mod_import_import_value_from_import_FROM {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_mod_import_import_value_from_import_FROM value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_parentheses_AUTO_1::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_parentheses_AUTO_1", true);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_parentheses_AUTO_1::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_parentheses_AUTO_1::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_parentheses_AUTO_1 {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_parentheses_AUTO_1 value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_parentheses::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_parentheses", true);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_parentheses::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_parentheses::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_parentheses {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_parentheses value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::AUTO::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("AUTO", true);
}
auto DepParser::FlatTypes::AUTO::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::AUTO::to_string() const -> std::string {
	return ::ISPA_CORE::concat("AUTO {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, AUTO value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::RIGHT_CURLY_BRACKET::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("RIGHT_CURLY_BRACKET", true);
}
auto DepParser::FlatTypes::RIGHT_CURLY_BRACKET::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::RIGHT_CURLY_BRACKET::to_string() const -> std::string {
	return ::ISPA_CORE::concat("RIGHT_CURLY_BRACKET {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, RIGHT_CURLY_BRACKET value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_fail_decl_FAIL::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_fail_decl_FAIL", true);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_fail_decl_FAIL::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_fail_decl_FAIL::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_fail_decl_FAIL {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_fail_decl_FAIL value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_fail_decl::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_fail_decl", true);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_fail_decl::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_fail_decl::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_fail_decl {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_fail_decl value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::LEFT_CURLY_BRACKET::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("LEFT_CURLY_BRACKET", true);
}
auto DepParser::FlatTypes::LEFT_CURLY_BRACKET::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::LEFT_CURLY_BRACKET::to_string() const -> std::string {
	return ::ISPA_CORE::concat("LEFT_CURLY_BRACKET {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, LEFT_CURLY_BRACKET value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::FULL_STOP::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("FULL_STOP", true);
}
auto DepParser::FlatTypes::FULL_STOP::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::FULL_STOP::to_string() const -> std::string {
	return ::ISPA_CORE::concat("FULL_STOP {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, FULL_STOP value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_scanner_tokens_OTHER::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_scanner_tokens_OTHER", false);
	printer.up(false);
	printer.node_with_value("value", value, true);
	printer.down();
}
auto DepParser::FlatTypes::modules_scanner_tokens_OTHER::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_scanner_tokens_OTHER::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_scanner_tokens_OTHER {", "value = ", value, "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_scanner_tokens_OTHER value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_scanner_tokens::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_scanner_tokens", true);
}
auto DepParser::FlatTypes::modules_scanner_tokens::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_scanner_tokens::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_scanner_tokens {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_scanner_tokens value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::GREATER_THAN_SIGN::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("GREATER_THAN_SIGN", true);
}
auto DepParser::FlatTypes::GREATER_THAN_SIGN::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::GREATER_THAN_SIGN::to_string() const -> std::string {
	return ::ISPA_CORE::concat("GREATER_THAN_SIGN {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, GREATER_THAN_SIGN value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_parentheses_LEFT_PARENTHESIS::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_parentheses_LEFT_PARENTHESIS", true);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_parentheses_LEFT_PARENTHESIS::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_parentheses_LEFT_PARENTHESIS::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_parentheses_LEFT_PARENTHESIS {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_parentheses_LEFT_PARENTHESIS value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::LESS_THAN_SIGN::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("LESS_THAN_SIGN", true);
}
auto DepParser::FlatTypes::LESS_THAN_SIGN::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::LESS_THAN_SIGN::to_string() const -> std::string {
	return ::ISPA_CORE::concat("LESS_THAN_SIGN {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, LESS_THAN_SIGN value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::RIGHT_PARENTHESIS::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("RIGHT_PARENTHESIS", true);
}
auto DepParser::FlatTypes::RIGHT_PARENTHESIS::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::RIGHT_PARENTHESIS::to_string() const -> std::string {
	return ::ISPA_CORE::concat("RIGHT_PARENTHESIS {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, RIGHT_PARENTHESIS value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_common_NUMBER::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_common_NUMBER", false);
	printer.up(false);
	printer.node_with_value("full", full, false);
	printer.node_with_value("floating", floating, true);
	printer.down();
}
auto DepParser::FlatTypes::modules_common_NUMBER::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_common_NUMBER::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_common_NUMBER {", "full = ", full, ", ", "floating = ", floating, "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_common_NUMBER value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_common::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_common", true);
}
auto DepParser::FlatTypes::modules_common::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_common::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_common {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_common value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::COMMA::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("COMMA", true);
}
auto DepParser::FlatTypes::COMMA::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::COMMA::to_string() const -> std::string {
	return ::ISPA_CORE::concat("COMMA {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, COMMA value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_scanner_tokens_STRING::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_scanner_tokens_STRING", false);
	printer.up(false);
	printer.node_with_value("value", value, true);
	printer.down();
}
auto DepParser::FlatTypes::modules_scanner_tokens_STRING::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_scanner_tokens_STRING::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_scanner_tokens_STRING {", "value = ", value, "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_scanner_tokens_STRING value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::FEATURE::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("FEATURE", true);
}
auto DepParser::FlatTypes::FEATURE::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::FEATURE::to_string() const -> std::string {
	return ::ISPA_CORE::concat("FEATURE {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, FEATURE value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::SEMICOLON::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("SEMICOLON", true);
}
auto DepParser::FlatTypes::SEMICOLON::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::SEMICOLON::to_string() const -> std::string {
	return ::ISPA_CORE::concat("SEMICOLON {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, SEMICOLON value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::AUTO_1::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("AUTO_1", true);
}
auto DepParser::FlatTypes::AUTO_1::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::AUTO_1::to_string() const -> std::string {
	return ::ISPA_CORE::concat("AUTO_1 {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, AUTO_1 value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_scanner_tokens_COMMENT::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_scanner_tokens_COMMENT", false);
	printer.up(false);
	printer.node_with_value("value", value, true);
	printer.down();
}
auto DepParser::FlatTypes::modules_scanner_tokens_COMMENT::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_scanner_tokens_COMMENT::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_scanner_tokens_COMMENT {", "value = ", value, "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_scanner_tokens_COMMENT value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::ASTERISK::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("ASTERISK", true);
}
auto DepParser::FlatTypes::ASTERISK::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::ASTERISK::to_string() const -> std::string {
	return ::ISPA_CORE::concat("ASTERISK {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, ASTERISK value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_external_import_EXTERN::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_mod_import_external_import_EXTERN", true);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_external_import_EXTERN::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_external_import_EXTERN::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_mod_import_external_import_EXTERN {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_mod_import_external_import_EXTERN value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_scanner_tokens_CGROUP::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_scanner_tokens_CGROUP", false);
	printer.up(false);
	printer.node_with_value("value", value, true);
	printer.down();
}
auto DepParser::FlatTypes::modules_scanner_tokens_CGROUP::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_scanner_tokens_CGROUP::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_scanner_tokens_CGROUP {", "value = ", value, "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_scanner_tokens_CGROUP value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_PY::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_mod_import_PY", true);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_PY::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_PY::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_mod_import_PY {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_mod_import_PY value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::COLON::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("COLON", true);
}
auto DepParser::FlatTypes::COLON::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::COLON::to_string() const -> std::string {
	return ::ISPA_CORE::concat("COLON {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, COLON value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::NUMBER_SIGN::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("NUMBER_SIGN", true);
}
auto DepParser::FlatTypes::NUMBER_SIGN::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::NUMBER_SIGN::to_string() const -> std::string {
	return ::ISPA_CORE::concat("NUMBER_SIGN {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, NUMBER_SIGN value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_common_ID::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_common_ID", false);
	printer.up(false);
	printer.node_with_value("value", value, true);
	printer.down();
}
auto DepParser::FlatTypes::modules_common_ID::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_common_ID::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_common_ID {", "value = ", value, "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_common_ID value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_atom::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_atom", true);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_atom::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_atom::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_atom {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_atom value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_braces::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_braces", true);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_braces::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_braces::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_braces {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_braces value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_nested_rule::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_nested_rule", false);
	printer.up(false);
	printer.node("value", true);
	printer.up(false);
	value.print(printer);
	printer.down();
	printer.down();
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_nested_rule::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_nested_rule::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_nested_rule {", "value = ", value.to_string(), "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_nested_rule value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_body_regular::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_body_regular", false);
	printer.up(false);
	printer.subtree("value", value, true);
	printer.down();
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_body_regular::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_body_regular::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_body_regular {", "value = ", value, "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_body_regular value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_modifier::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_modifier", true);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_modifier::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_modifier::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_modifier {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_modifier value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_entry::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_entry", true);
}
auto DepParser::FlatTypes::modules_entry::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_entry::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_entry {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_entry value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_balanced_item::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_balanced_item", true);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_balanced_item::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_balanced_item::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_balanced_item {", "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_balanced_item value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_external_import::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_mod_import_external_import", false);
	printer.up(false);
	printer.node("value", true);
	printer.up(false);
	value.print(printer);
	printer.down();
	printer.down();
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_external_import::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_external_import::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_mod_import_external_import {", "value = ", value.to_string(), "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_mod_import_external_import value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_common_type_annotation::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_common_type_annotation", false);
	printer.up(false);
	printer.node("base", false);
	printer.up(true);
	base.print(printer);
	printer.down();
	printer.subtree("nested", nested, true);
	printer.down();
}
auto DepParser::FlatTypes::modules_common_type_annotation::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_common_type_annotation::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_common_type_annotation {", "base = ", base.to_string(), ", ", "nested = ", nested, "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_common_type_annotation value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_module_path::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_module_path", false);
	printer.up(false);
	printer.node("primary", false);
	printer.up(true);
	primary.print(printer);
	printer.down();
	printer.subtree("nested", nested, true);
	printer.down();
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_module_path::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_module_path::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_module_path {", "primary = ", primary.to_string(), ", ", "nested = ", nested, "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_module_path value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_common_template_param::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_common_template_param", false);
	printer.up(false);
	printer.node("first_p", false);
	printer.up(true);
	first_p.print(printer);
	printer.down();
	printer.subtree("following_p", following_p, true);
	printer.down();
}
auto DepParser::FlatTypes::modules_common_template_param::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_common_template_param::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_common_template_param {", "first_p = ", first_p.to_string(), ", ", "following_p = ", following_p, "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_common_template_param value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_body::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_body", false);
	printer.up(false);
	printer.node_with_value("value", value, true);
	printer.down();
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_body::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_body::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_body {", "value = ", value, "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_body value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_member::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_member", false);
	printer.up(false);
	printer.node_with_value("value", value, true);
	printer.down();
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_member::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_member::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_member {", "value = ", value, "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_member value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_declaration::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_declaration", false);
	printer.up(false);
	printer.subtree("modifiers", modifiers, false);
	printer.node("value", true);
	printer.up(false);
	value.print(printer);
	printer.down();
	printer.down();
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_declaration::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_declaration::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_declaration {", "modifiers = ", modifiers, ", ", "value = ", value.to_string(), "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_declaration value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_common_parameter::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_common_parameter", false);
	printer.up(false);
	printer.node("name", false);
	printer.up(true);
	name.print(printer);
	printer.down();
	printer.node("annotation", true);
	printer.up(false);
	annotation.print(printer);
	printer.down();
	printer.down();
}
auto DepParser::FlatTypes::modules_common_parameter::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_common_parameter::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_common_parameter {", "name = ", name.to_string(), ", ", "annotation = ", annotation.to_string(), "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_common_parameter value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_import_value_module_import::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_mod_import_import_value_module_import", false);
	printer.up(false);
	printer.node("source", false);
	printer.up(true);
	source.print(printer);
	printer.down();
	printer.node("alias", true);
	printer.up(false);
	alias.print(printer);
	printer.down();
	printer.down();
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_import_value_module_import::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_import_value_module_import::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_mod_import_import_value_module_import {", "source = ", source.to_string(), ", ", "alias = ", alias.to_string(), "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_mod_import_import_value_module_import value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_common_template_params::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_common_template_params", false);
	printer.up(false);
	printer.node("value", true);
	printer.up(false);
	value.print(printer);
	printer.down();
	printer.down();
}
auto DepParser::FlatTypes::modules_common_template_params::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_common_template_params::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_common_template_params {", "value = ", value.to_string(), "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_common_template_params value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_common_param::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_common_param", false);
	printer.up(false);
	printer.node("first_p", false);
	printer.up(true);
	first_p.print(printer);
	printer.down();
	printer.subtree("following_p", following_p, true);
	printer.down();
}
auto DepParser::FlatTypes::modules_common_param::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_common_param::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_common_param {", "first_p = ", first_p.to_string(), ", ", "following_p = ", following_p, "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_common_param value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_common_rule_params::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_common_rule_params", false);
	printer.up(false);
	printer.node("value", true);
	printer.up(false);
	value.print(printer);
	printer.down();
	printer.down();
}
auto DepParser::FlatTypes::modules_common_rule_params::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_common_rule_params::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_common_rule_params {", "value = ", value.to_string(), "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_common_rule_params value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_declaration_form::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_declaration_form", false);
	printer.up(false);
	printer.node("name", false);
	printer.up(true);
	name.print(printer);
	printer.down();
	printer.node("body", true);
	printer.up(false);
	body.print(printer);
	printer.down();
	printer.down();
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_declaration_form::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_declaration_form::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_declaration_form {", "name = ", name.to_string(), ", ", "body = ", body.to_string(), "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_declaration_form value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_source_name::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_source_name", false);
	printer.up(false);
	printer.node("value", true);
	printer.up(false);
	value.print(printer);
	printer.down();
	printer.down();
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_source_name::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_source_name::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_source_name {", "value = ", value.to_string(), "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_source_name value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_module::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_mod_module", false);
	printer.up(false);
	printer.node("begin", false);
	printer.up(true);
	begin.print(printer);
	printer.down();
	printer.node("name", false);
	printer.up(true);
	name.print(printer);
	printer.down();
	printer.node("parameters", false);
	printer.up(true);
	parameters.print(printer);
	printer.down();
	printer.node("entry", false);
	printer.up(true);
	entry.print(printer);
	printer.down();
	printer.node("end", true);
	printer.up(false);
	end.print(printer);
	printer.down();
	printer.down();
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_module::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_module::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_mod_module {", "begin = ", begin.to_string(), ", ", "name = ", name.to_string(), ", ", "parameters = ", parameters.to_string(), ", ", "entry = ", entry.to_string(), ", ", "end = ", end.to_string(), "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_mod_module value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_symbol_list::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_symbol_list", false);
	printer.up(false);
	printer.node("first", false);
	printer.up(true);
	first.print(printer);
	printer.down();
	printer.subtree("following", following, true);
	printer.down();
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_symbol_list::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_symbol_list::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_symbol_list {", "first = ", first.to_string(), ", ", "following = ", following, "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_symbol_list value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_detach_detach_value_feature_ref::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_mod_detach_detach_value_feature_ref", false);
	printer.up(false);
	printer.node("value", true);
	printer.up(false);
	value.print(printer);
	printer.down();
	printer.down();
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_detach_detach_value_feature_ref::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_detach_detach_value_feature_ref::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_mod_detach_detach_value_feature_ref {", "value = ", value.to_string(), "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_mod_detach_detach_value_feature_ref value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_import_value_from_import_selection_items::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_mod_import_import_value_from_import_selection_items", false);
	printer.up(false);
	printer.node("first", false);
	printer.up(true);
	first.print(printer);
	printer.down();
	printer.subtree("sequence", sequence, true);
	printer.down();
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_import_value_from_import_selection_items::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_import_value_from_import_selection_items::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_mod_import_import_value_from_import_selection_items {", "first = ", first.to_string(), ", ", "sequence = ", sequence, "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_mod_import_import_value_from_import_selection_items value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_feature_decl::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_feature_decl", false);
	printer.up(false);
	printer.node("begin", false);
	printer.up(true);
	begin.print(printer);
	printer.down();
	printer.node("name", false);
	printer.up(true);
	name.print(printer);
	printer.down();
	printer.node("symbols", false);
	printer.up(true);
	symbols.print(printer);
	printer.down();
	printer.node("end", true);
	printer.up(false);
	end.print(printer);
	printer.down();
	printer.down();
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_feature_decl::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_feature_decl::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_feature_decl {", "begin = ", begin.to_string(), ", ", "name = ", name.to_string(), ", ", "symbols = ", symbols.to_string(), ", ", "end = ", end.to_string(), "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_feature_decl value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_detach_detach_value::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_mod_detach_detach_value", false);
	printer.up(false);
	printer.node_with_value("value", value, true);
	printer.down();
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_detach_detach_value::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_detach_detach_value::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_mod_detach_detach_value {", "value = ", value, "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_mod_detach_detach_value value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_import_value_from_import_selection::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_mod_import_import_value_from_import_selection", false);
	printer.up(false);
	printer.node_with_value("value", value, true);
	printer.down();
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_import_value_from_import_selection::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_import_value_from_import_selection::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_mod_import_import_value_from_import_selection {", "value = ", value, "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_mod_import_import_value_from_import_selection value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_detach::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_mod_detach", false);
	printer.up(false);
	printer.node("begin", false);
	printer.up(true);
	begin.print(printer);
	printer.down();
	printer.node("value", false);
	printer.up(true);
	value.print(printer);
	printer.down();
	printer.node("end", true);
	printer.up(false);
	end.print(printer);
	printer.down();
	printer.down();
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_detach::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_detach::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_mod_detach {", "begin = ", begin.to_string(), ", ", "value = ", value.to_string(), ", ", "end = ", end.to_string(), "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_mod_detach value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_import_value_from_import::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_mod_import_import_value_from_import", false);
	printer.up(false);
	printer.node("sel", false);
	printer.up(true);
	sel.print(printer);
	printer.down();
	printer.node("source", true);
	printer.up(false);
	source.print(printer);
	printer.down();
	printer.down();
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_import_value_from_import::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_import_value_from_import::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_mod_import_import_value_from_import {", "sel = ", sel.to_string(), ", ", "source = ", source.to_string(), "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_mod_import_import_value_from_import value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_import_value::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_mod_import_import_value", false);
	printer.up(false);
	printer.node_with_value("value", value, true);
	printer.down();
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_import_value::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import_import_value::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_mod_import_import_value {", "value = ", value, "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_mod_import_import_value value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_ispa_dep_scanner_mod_import", false);
	printer.up(false);
	printer.node("begin", false);
	printer.up(true);
	begin.print(printer);
	printer.down();
	printer.node("py_import", false);
	printer.up(true);
	py_import.print(printer);
	printer.down();
	printer.node("external", false);
	printer.up(true);
	external.print(printer);
	printer.down();
	printer.node("value", false);
	printer.up(true);
	value.print(printer);
	printer.down();
	printer.node("end", true);
	printer.up(false);
	end.print(printer);
	printer.down();
	printer.down();
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_ispa_dep_scanner_mod_import::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_ispa_dep_scanner_mod_import {", "begin = ", begin.to_string(), ", ", "py_import = ", py_import.to_string(), ", ", "external = ", external.to_string(), ", ", "value = ", value.to_string(), ", ", "end = ", end.to_string(), "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_ispa_dep_scanner_mod_import value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::modules_entry_main::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("modules_entry_main", false);
	printer.up(false);
	printer.node_with_value("value", value, true);
	printer.down();
}
auto DepParser::FlatTypes::modules_entry_main::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::modules_entry_main::to_string() const -> std::string {
	return ::ISPA_CORE::concat("modules_entry_main {", "value = ", value, "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, modules_entry_main value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::FlatTypes::main::write_to_output(::ISPA_CORE::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("main", false);
	printer.up(false);
	printer.node("value", true);
	printer.up(false);
	value.print(printer);
	printer.down();
	printer.down();
}
auto DepParser::FlatTypes::main::print(std::ostream& os) const -> void {
	::ISPA_CORE::ASTPrinter<std::ostream> printer = ::ISPA_CORE::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto DepParser::FlatTypes::main::to_string() const -> std::string {
	return ::ISPA_CORE::concat("main {", "value = ", value.to_string(), "}");
}
auto DepParser::FlatTypes::operator<<(std::ostream & os, main value) -> std::ostream & {
	value.print(os);
	return os;
}
auto DepParser::Lexer::clone() const -> std::shared_ptr<::ISPA_CORE::Lexer_base<Token>> {
	return std::make_shared<Lexer>(*this);
}
auto DepParser::Lexer::ispa_skip_current_token() -> void {
	ispa_skip_token = true;
}
auto DepParser::Lexer::ispa_token_error(std::string message) -> void {
	throw std::runtime_error(message);
}
auto DepParser::Lexer::on_token(Token&& token) -> void {
	if (ispa_skip_token) {
	} else {
		append_token(token);
	}
	ispa_skip_token = false;
	tokens.insert(tokens.end(), ispa_pending_tokens.begin(), ispa_pending_tokens.end());
	ispa_pending_tokens.clear();
}
auto DepParser::Lexer::on_eof() -> void {
	tokens.insert(tokens.end(), ispa_pending_tokens.begin(), ispa_pending_tokens.end());
	ispa_pending_tokens.clear();
	tokens.emplace_back();
}
auto DepParser::Lexer::semantic_action_exec(long long state, const ::ISPA_CORE::DFA::API::Captures& captures, long long start_pos, const char* start, long long length, long long line, const char*& cursor) -> std::pair<long long, Token> {
	switch (state) {
		case 0: {
			char value = captures.character(6);
			if (::ispa::debug::enabled()) ::ispa::debug::lexer_capture("value", value);
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::modules::scanner_tokens::OTHER::id, Types::modules::scanner_tokens::OTHER{value}));
			break;
		}
		case 1: {
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::__WS::id, Types::__WS{}));
			break;
		}
		case 2: {
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::NUMBER_SIGN::id, Types::NUMBER_SIGN{}));
			break;
		}
		case 3: {
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Types::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS{}));
			break;
		}
		case 4: {
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::RIGHT_PARENTHESIS::id, Types::RIGHT_PARENTHESIS{}));
			break;
		}
		case 5: {
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::ASTERISK::id, Types::ASTERISK{}));
			break;
		}
		case 6: {
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::COMMA::id, Types::COMMA{}));
			break;
		}
		case 7: {
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::FULL_STOP::id, Types::FULL_STOP{}));
			break;
		}
		case 8: {
			std::string floating = captures.text(5);
			if (::ispa::debug::enabled()) ::ispa::debug::lexer_capture("floating", floating);
			std::string full = captures.text(4);
			if (::ispa::debug::enabled()) ::ispa::debug::lexer_capture("full", full);
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::modules::common::NUMBER::id, Types::modules::common::NUMBER{full, floating}));
			break;
		}
		case 9: {
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::COLON::id, Types::COLON{}));
			break;
		}
		case 10: {
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::SEMICOLON::id, Types::SEMICOLON{}));
			break;
		}
		case 11: {
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::LESS_THAN_SIGN::id, Types::LESS_THAN_SIGN{}));
			break;
		}
		case 12: {
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::GREATER_THAN_SIGN::id, Types::GREATER_THAN_SIGN{}));
			break;
		}
		case 13: {
			std::string value = captures.text(0);
			if (::ispa::debug::enabled()) ::ispa::debug::lexer_capture("value", value);
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::modules::common::ID::id, Types::modules::common::ID{value}));
			break;
		}
		case 14: {
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::LEFT_CURLY_BRACKET::id, Types::LEFT_CURLY_BRACKET{}));
			break;
		}
		case 15: {
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::RIGHT_CURLY_BRACKET::id, Types::RIGHT_CURLY_BRACKET{}));
			break;
		}
		case 16: {
			std::string value = captures.text(8);
			if (::ispa::debug::enabled()) ::ispa::debug::lexer_capture("value", value);
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::modules::scanner_tokens::STRING::id, Types::modules::scanner_tokens::STRING{value}));
			break;
		}
		case 17: {
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::AUTO::id, Types::AUTO{}));
			break;
		}
		case 18: {
			std::string value = captures.text(8);
			if (::ispa::debug::enabled()) ::ispa::debug::lexer_capture("value", value);
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::modules::scanner_tokens::STRING::id, Types::modules::scanner_tokens::STRING{value}));
			break;
		}
		case 19: {
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::AUTO_1::id, Types::AUTO_1{}));
			break;
		}
		case 20: {
			std::string value = captures.text(2);
			if (::ispa::debug::enabled()) ::ispa::debug::lexer_capture("value", value);
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::modules::scanner_tokens::COMMENT::id, Types::modules::scanner_tokens::COMMENT{value}));
			break;
		}
		case 21: {
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Types::modules::ispa_dep_scanner::parentheses::AUTO_1{}));
			break;
		}
		case 22: {
			std::string value = captures.text(0);
			if (::ispa::debug::enabled()) ::ispa::debug::lexer_capture("value", value);
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::modules::common::ID::id, Types::modules::common::ID{value}));
			break;
		}
		case 23: {
			char value = captures.character(1);
			if (::ispa::debug::enabled()) ::ispa::debug::lexer_capture("value", value);
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::modules::scanner_tokens::CGROUP::id, Types::modules::scanner_tokens::CGROUP{value}));
			break;
		}
		case 24: {
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id, Types::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS{}));
			break;
		}
		case 25: {
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::modules::ispa_dep_scanner::mod_import::PY::id, Types::modules::ispa_dep_scanner::mod_import::PY{}));
			break;
		}
		case 26: {
			std::string value = captures.text(2);
			if (::ispa::debug::enabled()) ::ispa::debug::lexer_capture("value", value);
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::modules::scanner_tokens::COMMENT::id, Types::modules::scanner_tokens::COMMENT{value}));
			break;
		}
		case 27: {
			std::string floating = captures.text(5);
			if (::ispa::debug::enabled()) ::ispa::debug::lexer_capture("floating", floating);
			std::string full = captures.text(4);
			if (::ispa::debug::enabled()) ::ispa::debug::lexer_capture("full", full);
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::modules::common::NUMBER::id, Types::modules::common::NUMBER{full, floating}));
			break;
		}
		case 28: {
			std::string value = captures.text(2);
			if (::ispa::debug::enabled()) ::ispa::debug::lexer_capture("value", value);
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::modules::scanner_tokens::COMMENT::id, Types::modules::scanner_tokens::COMMENT{value}));
			break;
		}
		case 29: {
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Types::modules::ispa_dep_scanner::fail_decl::FAIL{}));
			break;
		}
		case 30: {
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Types::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM{}));
			break;
		}
		case 31: {
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Types::modules::ispa_dep_scanner::mod_detach::DETACH{}));
			break;
		}
		case 32: {
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id, Types::modules::ispa_dep_scanner::mod_import::external_import::EXTERN{}));
			break;
		}
		case 33: {
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Types::modules::ispa_dep_scanner::mod_import::IMPORT{}));
			break;
		}
		case 34: {
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Types::modules::ispa_dep_scanner::mod_module::MODULE{}));
			break;
		}
		case 35: {
			return std::make_pair(::ISPA_CORE::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::FEATURE::id, Types::FEATURE{}));
			break;
		}
		case 36: {
			auto ctx = lexer_failure_context(start + length, "modules.common.NUMBER", "modules.common.NUMBER");
			return lexer_failure_result(ctx, Token::create(start_pos, start, length, line, Tokens::modules::common::NUMBER::id, Types::modules::common::NUMBER{}));
			break;
		}
		case 37: {
			ispa_token_error("unmatched lexer input");
			break;
		}
	}
	throw std::runtime_error("Out of bound semantic action");
}
auto DepParser::Lexer::init() -> void {
	return ;
}
auto DepParser::Lexer::makeToken(const char*& pos) -> Token {
	return lookup(dfa_table, char_class_table, action_table, tdfa_registers, [&](long long state, const ::ISPA_CORE::DFA::API::Captures& captures, long long start_pos, const char* start, long long length, long long line, const char*& cursor) {
return semantic_action_exec(state, captures, start_pos, start, length, line, cursor);
}, debug_array, debug_index, pos);
}
::ISPA_CORE::DFA::API::CharToClass DepParser::Lexer::char_class_table = {0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 3, 2, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 4, 5, 1, 1, 6, 7, 8, 9, 10, 1, 11, 12, 13, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 16, 17, 18, 1, 19, 1, 1, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21, 22, 23, 1, 20, 1, 24, 20, 25, 26, 27, 28, 20, 29, 30, 20, 20, 31, 32, 33, 34, 35, 20, 36, 37, 38, 39, 20, 20, 40, 41, 20, 42, 1, 43, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
::ISPA_CORE::DFA::API::Table<82, 44> DepParser::Lexer::dfa_table = {{{223, 82, 2, 2, 94, 4, 93, 92, 7, 8, 9, 10, 91, 12, 90, 89, 15, 16, 17, 18, 83, 88, 82, 223, 87, 83, 97, 95, 96, 83, 86, 83, 85, 83, 83, 84, 83, 83, 83, 83, 83, 83, 28, 29}, {98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98}, {188, 188, 2, 2, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188}, {223, 3, 3, 3, 30, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 31, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3}, {189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189, 189}, {100, 100, 100, 100, 100, 100, 100, 100, 32, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100}, {223, 6, 6, 6, 6, 6, 6, 33, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 34, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6}, {190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190}, {191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191}, {192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192}, {193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193}, {102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 35, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102}, {194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194, 194}, {104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 36, 104, 104, 104, 37, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104}, {105, 105, 105, 105, 105, 105, 105, 105, 105, 105, 105, 105, 105, 106, 105, 14, 105, 105, 105, 105, 105, 105, 105, 105, 105, 105, 105, 105, 105, 105, 105, 105, 105, 105, 105, 105, 105, 105, 105, 105, 105, 105, 105, 105}, {196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196}, {197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197}, {198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198}, {199, 199, 199, 199, 199, 199, 199, 199, 39, 199, 199, 199, 199, 199, 199, 199, 199, 199, 199, 199, 199, 199, 199, 199, 199, 199, 199, 199, 199, 199, 199, 199, 199, 199, 199, 199, 199, 199, 199, 199, 199, 199, 199, 199}, {107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 40, 107, 107, 107, 107, 40, 107, 107, 107, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 107, 107}, {223, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 110, 109, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108}, {112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 113, 112, 112, 112, 112, 113, 112, 112, 112, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 44, 113, 113, 113, 113, 112, 112}, {115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 113, 115, 115, 115, 115, 113, 115, 115, 115, 113, 113, 113, 45, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 115, 115}, {117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 113, 117, 117, 117, 117, 113, 117, 117, 117, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 46, 113, 117, 117}, {119, 119, 119, 119, 119, 119, 119, 119, 119, 119, 119, 119, 119, 119, 119, 113, 119, 119, 119, 119, 113, 119, 119, 119, 47, 113, 113, 48, 113, 113, 113, 113, 113, 113, 113, 113, 49, 113, 113, 113, 113, 113, 119, 119}, {121, 121, 121, 121, 121, 121, 121, 121, 121, 121, 121, 121, 121, 121, 121, 113, 121, 121, 121, 121, 113, 121, 121, 121, 113, 113, 113, 113, 113, 113, 113, 113, 50, 113, 113, 113, 113, 113, 113, 113, 113, 113, 121, 121}, {123, 123, 123, 123, 123, 123, 123, 123, 123, 123, 123, 123, 123, 123, 123, 113, 123, 123, 123, 123, 113, 123, 123, 123, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 51, 113, 113, 113, 113, 113, 113, 113, 123, 123}, {125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 113, 125, 125, 125, 125, 113, 125, 125, 125, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 52, 125, 125}, {201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201}, {202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202, 202}, {126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126}, {223, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3}, {204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204}, {127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127}, {223, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6}, {206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206}, {223, 36, 36, 36, 36, 36, 36, 36, 36, 36, 53, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36}, {129, 54, 54, 129, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54}, {223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 130, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223}, {208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208}, {131, 131, 131, 131, 131, 131, 131, 131, 131, 131, 131, 131, 131, 131, 131, 40, 131, 131, 131, 131, 40, 131, 131, 131, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 131, 131}, {223, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 42, 43, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41}, {223, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41}, {210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210}, {211, 211, 211, 211, 211, 211, 211, 211, 211, 211, 211, 211, 211, 211, 211, 113, 211, 211, 211, 211, 113, 211, 211, 211, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 211, 211}, {133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 113, 133, 133, 133, 133, 113, 133, 133, 133, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 56, 113, 113, 113, 133, 133}, {135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 113, 135, 135, 135, 135, 113, 135, 135, 135, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 57, 113, 113, 113, 135, 135}, {137, 137, 137, 137, 137, 137, 137, 137, 137, 137, 137, 137, 137, 137, 137, 113, 137, 137, 137, 137, 113, 137, 137, 137, 113, 113, 113, 113, 113, 113, 58, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 137, 137}, {139, 139, 139, 139, 139, 139, 139, 139, 139, 139, 139, 139, 139, 139, 139, 113, 139, 139, 139, 139, 113, 139, 139, 139, 59, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 139, 139}, {141, 141, 141, 141, 141, 141, 141, 141, 141, 141, 141, 141, 141, 141, 141, 113, 141, 141, 141, 141, 113, 141, 141, 141, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 60, 113, 113, 113, 113, 113, 113, 113, 141, 141}, {143, 143, 143, 143, 143, 143, 143, 143, 143, 143, 143, 143, 143, 143, 143, 113, 143, 143, 143, 143, 113, 143, 143, 143, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 61, 113, 113, 113, 113, 113, 113, 143, 143}, {145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 113, 145, 145, 145, 145, 113, 145, 145, 145, 113, 113, 62, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 145, 145}, {212, 212, 212, 212, 212, 212, 212, 212, 212, 212, 212, 212, 212, 212, 212, 113, 212, 212, 212, 212, 113, 212, 212, 212, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 212, 212}, {223, 36, 36, 36, 36, 36, 36, 36, 36, 36, 53, 36, 36, 36, 63, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36}, {147, 54, 54, 147, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54}, {148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 55, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148}, {150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 113, 150, 150, 150, 150, 113, 150, 150, 150, 64, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 150, 150}, {152, 152, 152, 152, 152, 152, 152, 152, 152, 152, 152, 152, 152, 152, 152, 113, 152, 152, 152, 152, 113, 152, 152, 152, 113, 113, 113, 65, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 152, 152}, {154, 154, 154, 154, 154, 154, 154, 154, 154, 154, 154, 154, 154, 154, 154, 113, 154, 154, 154, 154, 113, 154, 154, 154, 113, 113, 113, 113, 113, 113, 113, 66, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 154, 154}, {156, 156, 156, 156, 156, 156, 156, 156, 156, 156, 156, 156, 156, 156, 156, 113, 156, 156, 156, 156, 113, 156, 156, 156, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 67, 113, 113, 113, 156, 156}, {158, 158, 158, 158, 158, 158, 158, 158, 158, 158, 158, 158, 158, 158, 158, 113, 158, 158, 158, 158, 113, 158, 158, 158, 113, 113, 113, 113, 113, 113, 113, 113, 68, 113, 113, 113, 113, 113, 113, 113, 113, 113, 158, 158}, {160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 113, 160, 160, 160, 160, 113, 160, 160, 160, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 69, 113, 113, 113, 113, 113, 113, 113, 160, 160}, {162, 162, 162, 162, 162, 162, 162, 162, 162, 162, 162, 162, 162, 162, 162, 113, 162, 162, 162, 162, 113, 162, 162, 162, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 70, 113, 113, 162, 162}, {164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164}, {166, 166, 166, 166, 166, 166, 166, 166, 166, 166, 166, 166, 166, 166, 166, 113, 166, 166, 166, 166, 113, 166, 166, 166, 113, 71, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 166, 166}, {168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 113, 168, 168, 168, 168, 113, 168, 168, 168, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 72, 113, 113, 113, 113, 113, 168, 168}, {216, 216, 216, 216, 216, 216, 216, 216, 216, 216, 216, 216, 216, 216, 216, 113, 216, 216, 216, 216, 113, 216, 216, 216, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 216, 216}, {170, 170, 170, 170, 170, 170, 170, 170, 170, 170, 170, 170, 170, 170, 170, 113, 170, 170, 170, 170, 113, 170, 170, 170, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 73, 113, 113, 170, 170}, {217, 217, 217, 217, 217, 217, 217, 217, 217, 217, 217, 217, 217, 217, 217, 113, 217, 217, 217, 217, 113, 217, 217, 217, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 217, 217}, {172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 113, 172, 172, 172, 172, 113, 172, 172, 172, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 74, 113, 113, 113, 113, 113, 172, 172}, {174, 174, 174, 174, 174, 174, 174, 174, 174, 174, 174, 174, 174, 174, 174, 113, 174, 174, 174, 174, 113, 174, 174, 174, 113, 113, 113, 113, 113, 113, 113, 75, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 174, 174}, {176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 113, 176, 176, 176, 176, 113, 176, 176, 176, 113, 113, 113, 113, 113, 76, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 176, 176}, {178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 113, 178, 178, 178, 178, 113, 178, 178, 178, 113, 113, 113, 113, 113, 113, 113, 113, 113, 77, 113, 113, 113, 113, 113, 113, 113, 113, 178, 178}, {180, 180, 180, 180, 180, 180, 180, 180, 180, 180, 180, 180, 180, 180, 180, 113, 180, 180, 180, 180, 113, 180, 180, 180, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 78, 113, 113, 113, 113, 113, 180, 180}, {182, 182, 182, 182, 182, 182, 182, 182, 182, 182, 182, 182, 182, 182, 182, 113, 182, 182, 182, 182, 113, 182, 182, 182, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 79, 113, 113, 113, 182, 182}, {184, 184, 184, 184, 184, 184, 184, 184, 184, 184, 184, 184, 184, 184, 184, 113, 184, 184, 184, 184, 113, 184, 184, 184, 113, 113, 113, 80, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 184, 184}, {218, 218, 218, 218, 218, 218, 218, 218, 218, 218, 218, 218, 218, 218, 218, 113, 218, 218, 218, 218, 113, 218, 218, 218, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 218, 218}, {219, 219, 219, 219, 219, 219, 219, 219, 219, 219, 219, 219, 219, 219, 219, 113, 219, 219, 219, 219, 113, 219, 219, 219, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 219, 219}, {186, 186, 186, 186, 186, 186, 186, 186, 186, 186, 186, 186, 186, 186, 186, 113, 186, 186, 186, 186, 113, 186, 186, 186, 113, 113, 113, 81, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 186, 186}, {220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 113, 220, 220, 220, 220, 113, 220, 220, 220, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 220, 220}, {221, 221, 221, 221, 221, 221, 221, 221, 221, 221, 221, 221, 221, 221, 221, 113, 221, 221, 221, 221, 113, 221, 221, 221, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 221, 221}, {222, 222, 222, 222, 222, 222, 222, 222, 222, 222, 222, 222, 222, 222, 222, 113, 222, 222, 222, 222, 113, 222, 222, 222, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 222, 222}}};
::ISPA_CORE::DFA::API::Table<105, 4> DepParser::Lexer::action_table = {{{1, 12, ::ISPA_CORE::DFA::API::null_state, 1}, {1, 0, ::ISPA_CORE::DFA::API::null_state, 19}, {1, 18, ::ISPA_CORE::DFA::API::null_state, 27}, {1, 18, ::ISPA_CORE::DFA::API::null_state, 26}, {1, 18, ::ISPA_CORE::DFA::API::null_state, 25}, {1, 18, ::ISPA_CORE::DFA::API::null_state, 21}, {1, 2, ::ISPA_CORE::DFA::API::null_state, 20}, {1, 8, ::ISPA_CORE::DFA::API::null_state, 14}, {1, 18, ::ISPA_CORE::DFA::API::null_state, 13}, {1, 18, ::ISPA_CORE::DFA::API::null_state, 11}, {1, 16, ::ISPA_CORE::DFA::API::null_state, 6}, {1, 18, ::ISPA_CORE::DFA::API::null_state, 5}, {1, 14, ::ISPA_CORE::DFA::API::null_state, 3}, {1, 18, ::ISPA_CORE::DFA::API::null_state, 23}, {1, 18, ::ISPA_CORE::DFA::API::null_state, 24}, {1, 18, ::ISPA_CORE::DFA::API::null_state, 22}, {1, 13, ::ISPA_CORE::DFA::API::null_state, 187}, {3, 12, 18, 187}, {1, 13, ::ISPA_CORE::DFA::API::null_state, 99}, {3, 12, 18, 187}, {1, 13, ::ISPA_CORE::DFA::API::null_state, 101}, {3, 12, 18, 187}, {1, 13, ::ISPA_CORE::DFA::API::null_state, 103}, {1, 9, ::ISPA_CORE::DFA::API::null_state, 195}, {1, 9, ::ISPA_CORE::DFA::API::null_state, 38}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 200}, {1, 3, ::ISPA_CORE::DFA::API::null_state, 41}, {1, 3, ::ISPA_CORE::DFA::API::null_state, 43}, {1, 3, ::ISPA_CORE::DFA::API::null_state, 42}, {3, 0, 18, 200}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 111}, {3, 0, 18, 40}, {3, 0, 18, 200}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 114}, {3, 0, 18, 200}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 116}, {3, 0, 18, 200}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 118}, {3, 0, 18, 200}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 120}, {3, 0, 18, 200}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 122}, {3, 0, 18, 200}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 124}, {1, 15, ::ISPA_CORE::DFA::API::null_state, 203}, {1, 17, ::ISPA_CORE::DFA::API::null_state, 205}, {3, 6, 18, 207}, {1, 7, ::ISPA_CORE::DFA::API::null_state, 128}, {1, 10, ::ISPA_CORE::DFA::API::null_state, 55}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 209}, {3, 0, 18, 209}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 132}, {3, 0, 18, 209}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 134}, {3, 0, 18, 209}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 136}, {3, 0, 18, 209}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 138}, {3, 0, 18, 209}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 140}, {3, 0, 18, 209}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 142}, {3, 0, 18, 209}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 144}, {3, 6, 18, 213}, {1, 7, ::ISPA_CORE::DFA::API::null_state, 146}, {1, 11, ::ISPA_CORE::DFA::API::null_state, 214}, {3, 0, 18, 209}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 149}, {3, 0, 18, 209}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 151}, {3, 0, 18, 209}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 153}, {3, 0, 18, 209}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 155}, {3, 0, 18, 209}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 157}, {3, 0, 18, 209}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 159}, {3, 0, 18, 209}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 161}, {3, 4, 18, 215}, {1, 5, ::ISPA_CORE::DFA::API::null_state, 163}, {3, 0, 18, 209}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 165}, {3, 0, 18, 209}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 167}, {3, 0, 18, 209}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 169}, {3, 0, 18, 209}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 171}, {3, 0, 18, 209}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 173}, {3, 0, 18, 209}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 175}, {3, 0, 18, 209}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 177}, {3, 0, 18, 209}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 179}, {3, 0, 18, 209}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 181}, {3, 0, 18, 209}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 183}, {3, 0, 18, 209}, {1, 1, ::ISPA_CORE::DFA::API::null_state, 185}}};
std::array<::ISPA_CORE::DFA::API::DFADebug, 1> DepParser::Lexer::debug_array = {::ISPA_CORE::DFA::API::DFADebug {"<Undefined>", "", 0, -1, -1, -1, 0, 1, '\000'}};
std::unordered_map<long long, std::unordered_map<long long, long long>> DepParser::Lexer::debug_index = {  };
template<typename IT, class> auto DepParser::Parser::modulesIspa_dep_scannerMod_detachDetach_value(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_detach::detach_value, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.ispa_dep_scanner.mod_detach.detach_value", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 39, 6}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.ispa_dep_scanner.mod_detach.detach_value", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 39, 6}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 39, 6}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::ispa_dep_scanner::mod_detach::detach_value result = {};
		std::variant<std::monostate, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_detach::detach_value::feature_ref>, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::source_name>> choice0;
		bool matched0;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_detach::detach_value::feature_ref> value_modules_ispa_dep_scanner_mod_detach_detach_value_feature_ref;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::source_name> value_modules_ispa_dep_scanner_source_name;
		auto recovery_scope = recovery_frame("modules::ispa_dep_scanner::mod_detach::detach_value");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>>("<group>", "<group>");
		// grammar `@  ( modules::ispa_dep_scanner::mod_detach::detach_value::feature_ref |  modules::ispa_dep_scanner::source_name)`: choice; alternative selected by LL(1) lookahead
		matched0 = false;
		auto la0_1 = pos;
		switch (::ISPA_CORE::lookahead_kind(la0_1)) {
			case Tokens::FEATURE::id: {
				value_modules_ispa_dep_scanner_mod_detach_detach_value_feature_ref = required<Types::modules::ispa_dep_scanner::mod_detach::detach_value::feature_ref>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerMod_detachDetach_valueFeature_ref(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::mod_detach::detach_value", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_mod_detach_detach_value_feature_ref;
				matched0 = true;
				break;
			}
			case Tokens::modules::common::ID::id: {
				value_modules_ispa_dep_scanner_source_name = required<Types::modules::ispa_dep_scanner::source_name>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerSource_name(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::mod_detach::detach_value", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_source_name;
				matched0 = true;
				break;
			}
		}
		pos = la0_1;
		if (!matched0) {
			switch (recover_choice<::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>>(pos, "modules::ispa_dep_scanner::mod_detach::detach_value", "FEATURE, modules::common::ID")) {
				case 0: {
					value_modules_ispa_dep_scanner_mod_detach_detach_value_feature_ref = required<Types::modules::ispa_dep_scanner::mod_detach::detach_value::feature_ref>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerMod_detachDetach_valueFeature_ref(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::mod_detach::detach_value", "<group>"));
					choice0 = value_modules_ispa_dep_scanner_mod_detach_detach_value_feature_ref;
					matched0 = true;
					break;
				}
				case 1: {
					value_modules_ispa_dep_scanner_source_name = required<Types::modules::ispa_dep_scanner::source_name>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerSource_name(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::mod_detach::detach_value", "<group>"));
					choice0 = value_modules_ispa_dep_scanner_source_name;
					matched0 = true;
					break;
				}
			}
		}
		if (!matched0) {
			return {};
		}
		::ISPA_CORE::assign_compatible(result.value, choice0);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_detach::detach_value, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesIspa_dep_scannerMod_detachDetach_value<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_detach::detach_value, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesIspa_dep_scannerMod_detachDetach_value<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_detach::detach_value, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesCommonParam(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::common::param, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.common.param", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa/grammar/common.isc", 30, 1}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.common.param", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa/grammar/common.isc", 30, 1}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa/grammar/common.isc", 30, 1}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::common::param result = {};
		::ISPA_CORE::Node<Types::modules::common::parameter> value_modules_common_parameter;
		::ISPA_CORE::Node<Types::COMMA> value_COMMA;
		::ISPA_CORE::Node<Types::modules::common::parameter> value_modules_common_parameter_1;
		std::pair<::ISPA_CORE::Node<Types::COMMA>, ::ISPA_CORE::Node<Types::modules::common::parameter>> group0;
		std::vector<std::pair<::ISPA_CORE::Node<Types::COMMA>, ::ISPA_CORE::Node<Types::modules::common::parameter>>> group_list1;
		std::vector<::ISPA_CORE::Node<Types::modules::common::parameter>> capture_list2;
		auto recovery_scope = recovery_frame("modules::common::param");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id, Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>>("modules::common::parameter", "modules::common::parameter", "__WS", "<group>");
		value_modules_common_parameter = required<Types::modules::common::parameter>(pos, [this](auto cursor) { return this->modulesCommonParameter(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::RIGHT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::common::param", "modules::common::parameter"));
		::ISPA_CORE::assign_compatible(result.first_p, value_modules_common_parameter);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>>("__WS", "__WS", "<group>");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>>("<group>", "<group>");
		// grammar ` ( COMMA  __WS?; @  modules::common::parameter)*;`: repeat while lookahead is in FIRST
		while ((::ISPA_CORE::lookahead_kind(pos) == Tokens::COMMA::id)) {
			value_COMMA = required<Types::COMMA>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::common::param", "<group>"));
			skip_spaces<Types::__WS>(pos);
			value_modules_common_parameter_1 = required<Types::modules::common::parameter>(pos, [this](auto cursor) { return this->modulesCommonParameter(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::common::param", "<group>"));
			group0 = std::make_pair(value_COMMA, value_modules_common_parameter_1);
			group_list1.push_back(group0);
			capture_list2.push_back(value_modules_common_parameter_1);
			skip_spaces<Types::__WS>(pos);
		}
		::ISPA_CORE::assign_compatible(result.following_p, capture_list2);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		::ISPA_CORE::MatchResult<Types::modules::common::param, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesCommonParam<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::common::param, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesCommonParam<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::common::param, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesIspa_dep_scannerMod_importImport_valueFrom_import(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_import::import_value::from_import, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.ispa_dep_scanner.mod_import.import_value.from_import", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 22, 10}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.ispa_dep_scanner.mod_import.import_value.from_import", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 22, 10}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 22, 10}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::ispa_dep_scanner::mod_import::import_value::from_import result = {};
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_import::import_value::from_import::selection> value_modules_ispa_dep_scanner_mod_import_import_value_from_import_selection;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM> value_modules_ispa_dep_scanner_mod_import_import_value_from_import_FROM;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::module_path> value_modules_ispa_dep_scanner_module_path;
		auto recovery_scope = recovery_frame("modules::ispa_dep_scanner::mod_import::import_value::from_import");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>>("modules::ispa_dep_scanner::mod_import::import_value::from_import::selection", "modules::ispa_dep_scanner::mod_import::import_value::from_import::selection", "__WS", "modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM", "__WS", "modules::ispa_dep_scanner::module_path");
		value_modules_ispa_dep_scanner_mod_import_import_value_from_import_selection = required<Types::modules::ispa_dep_scanner::mod_import::import_value::from_import::selection>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerMod_importImport_valueFrom_importSelection(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::mod_import::import_value::from_import", "modules::ispa_dep_scanner::mod_import::import_value::from_import::selection"));
		::ISPA_CORE::assign_compatible(result.sel, value_modules_ispa_dep_scanner_mod_import_import_value_from_import_selection);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>>("__WS", "__WS", "modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM", "__WS", "modules::ispa_dep_scanner::module_path");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>>("modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM", "modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM", "__WS", "modules::ispa_dep_scanner::module_path");
		value_modules_ispa_dep_scanner_mod_import_import_value_from_import_FROM = required<Types::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::ispa_dep_scanner::mod_import::import_value::from_import", "modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM"));
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>>("__WS", "__WS", "modules::ispa_dep_scanner::module_path");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>>("modules::ispa_dep_scanner::module_path", "modules::ispa_dep_scanner::module_path");
		value_modules_ispa_dep_scanner_module_path = required<Types::modules::ispa_dep_scanner::module_path>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerModule_path(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::mod_import::import_value::from_import", "modules::ispa_dep_scanner::module_path"));
		::ISPA_CORE::assign_compatible(result.source, value_modules_ispa_dep_scanner_module_path);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_import::import_value::from_import, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesIspa_dep_scannerMod_importImport_valueFrom_import<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_import::import_value::from_import, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesIspa_dep_scannerMod_importImport_valueFrom_import<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_import::import_value::from_import, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesIspa_dep_scannerAtom(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::atom, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.ispa_dep_scanner.atom", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 96, 1}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.ispa_dep_scanner.atom", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 96, 1}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 96, 1}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::ispa_dep_scanner::atom result = {};
		std::variant<std::monostate, ::ISPA_CORE::Node<Types::modules::common::ID>, ::ISPA_CORE::Node<Types::modules::common::NUMBER>, ::ISPA_CORE::Node<Types::modules::scanner_tokens::STRING>, ::ISPA_CORE::Node<Types::modules::scanner_tokens::CGROUP>, ::ISPA_CORE::Node<Types::modules::scanner_tokens::COMMENT>, ::ISPA_CORE::Node<Types::modules::scanner_tokens::OTHER>, ::ISPA_CORE::Node<Types::AUTO_1>, ::ISPA_CORE::Node<Types::COMMA>, ::ISPA_CORE::Node<Types::FULL_STOP>, ::ISPA_CORE::Node<Types::LESS_THAN_SIGN>, ::ISPA_CORE::Node<Types::GREATER_THAN_SIGN>, ::ISPA_CORE::Node<Types::COLON>, ::ISPA_CORE::Node<Types::ASTERISK>> choice0;
		bool matched0;
		::ISPA_CORE::Node<Types::modules::common::ID> value_modules_common_ID;
		::ISPA_CORE::Node<Types::modules::common::NUMBER> value_modules_common_NUMBER;
		::ISPA_CORE::Node<Types::modules::scanner_tokens::STRING> value_modules_scanner_tokens_STRING;
		::ISPA_CORE::Node<Types::modules::scanner_tokens::CGROUP> value_modules_scanner_tokens_CGROUP;
		::ISPA_CORE::Node<Types::modules::scanner_tokens::COMMENT> value_modules_scanner_tokens_COMMENT;
		::ISPA_CORE::Node<Types::modules::scanner_tokens::OTHER> value_modules_scanner_tokens_OTHER;
		::ISPA_CORE::Node<Types::AUTO_1> value_AUTO_1;
		::ISPA_CORE::Node<Types::COMMA> value_COMMA;
		::ISPA_CORE::Node<Types::FULL_STOP> value_FULL_STOP;
		::ISPA_CORE::Node<Types::LESS_THAN_SIGN> value_LESS_THAN_SIGN;
		::ISPA_CORE::Node<Types::GREATER_THAN_SIGN> value_GREATER_THAN_SIGN;
		::ISPA_CORE::Node<Types::COLON> value_COLON;
		::ISPA_CORE::Node<Types::ASTERISK> value_ASTERISK;
		auto recovery_scope = recovery_frame("modules::ispa_dep_scanner::atom");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>>("<group>", "<group>");
		// grammar ` ( modules::common::ID |  modules::common::NUMBER |  modules::scanner_tokens::STRING |  modules::scanner_tokens::CGROUP |  modules::scanner_tokens::COMMENT |  modules::scanner_tokens::OTHER |  AUTO_1 |  COMMA |  FULL_STOP |  LESS_THAN_SIGN |  GREATER_THAN_SIGN |  COLON |  ASTERISK)`: choice; alternative selected by LL(1) lookahead
		matched0 = false;
		auto la0_1 = pos;
		switch (::ISPA_CORE::lookahead_kind(la0_1)) {
			case Tokens::ASTERISK::id: {
				value_ASTERISK = required<Types::ASTERISK>(la0_1, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::atom", "<group>"));
				choice0 = value_ASTERISK;
				matched0 = true;
				break;
			}
			case Tokens::AUTO_1::id: {
				value_AUTO_1 = required<Types::AUTO_1>(la0_1, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::atom", "<group>"));
				choice0 = value_AUTO_1;
				matched0 = true;
				break;
			}
			case Tokens::COLON::id: {
				value_COLON = required<Types::COLON>(la0_1, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::atom", "<group>"));
				choice0 = value_COLON;
				matched0 = true;
				break;
			}
			case Tokens::COMMA::id: {
				value_COMMA = required<Types::COMMA>(la0_1, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::atom", "<group>"));
				choice0 = value_COMMA;
				matched0 = true;
				break;
			}
			case Tokens::FULL_STOP::id: {
				value_FULL_STOP = required<Types::FULL_STOP>(la0_1, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::atom", "<group>"));
				choice0 = value_FULL_STOP;
				matched0 = true;
				break;
			}
			case Tokens::GREATER_THAN_SIGN::id: {
				value_GREATER_THAN_SIGN = required<Types::GREATER_THAN_SIGN>(la0_1, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::atom", "<group>"));
				choice0 = value_GREATER_THAN_SIGN;
				matched0 = true;
				break;
			}
			case Tokens::LESS_THAN_SIGN::id: {
				value_LESS_THAN_SIGN = required<Types::LESS_THAN_SIGN>(la0_1, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::atom", "<group>"));
				choice0 = value_LESS_THAN_SIGN;
				matched0 = true;
				break;
			}
			case Tokens::modules::common::ID::id: {
				value_modules_common_ID = required<Types::modules::common::ID>(la0_1, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::atom", "<group>"));
				choice0 = value_modules_common_ID;
				matched0 = true;
				break;
			}
			case Tokens::modules::common::NUMBER::id: {
				value_modules_common_NUMBER = required<Types::modules::common::NUMBER>(la0_1, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::atom", "<group>"));
				choice0 = value_modules_common_NUMBER;
				matched0 = true;
				break;
			}
			case Tokens::modules::scanner_tokens::CGROUP::id: {
				value_modules_scanner_tokens_CGROUP = required<Types::modules::scanner_tokens::CGROUP>(la0_1, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::atom", "<group>"));
				choice0 = value_modules_scanner_tokens_CGROUP;
				matched0 = true;
				break;
			}
			case Tokens::modules::scanner_tokens::COMMENT::id: {
				value_modules_scanner_tokens_COMMENT = required<Types::modules::scanner_tokens::COMMENT>(la0_1, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::atom", "<group>"));
				choice0 = value_modules_scanner_tokens_COMMENT;
				matched0 = true;
				break;
			}
			case Tokens::modules::scanner_tokens::OTHER::id: {
				value_modules_scanner_tokens_OTHER = required<Types::modules::scanner_tokens::OTHER>(la0_1, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::atom", "<group>"));
				choice0 = value_modules_scanner_tokens_OTHER;
				matched0 = true;
				break;
			}
			case Tokens::modules::scanner_tokens::STRING::id: {
				value_modules_scanner_tokens_STRING = required<Types::modules::scanner_tokens::STRING>(la0_1, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::atom", "<group>"));
				choice0 = value_modules_scanner_tokens_STRING;
				matched0 = true;
				break;
			}
		}
		pos = la0_1;
		if (!matched0) {
			switch (recover_choice<::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::NUMBER::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::NUMBER::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::scanner_tokens::CGROUP::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::scanner_tokens::CGROUP::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::scanner_tokens::OTHER::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::scanner_tokens::OTHER::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO_1::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO_1::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::LESS_THAN_SIGN::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::LESS_THAN_SIGN::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::GREATER_THAN_SIGN::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::GREATER_THAN_SIGN::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id>>(pos, "modules::ispa_dep_scanner::atom", "modules::common::ID, modules::common::NUMBER, modules::scanner_tokens::STRING, modules::scanner_tokens::CGROUP, modules::scanner_tokens::COMMENT, modules::scanner_tokens::OTHER, AUTO_1, COMMA, FULL_STOP, LESS_THAN_SIGN, GREATER_THAN_SIGN, COLON, ASTERISK")) {
				case 0: {
					value_modules_common_ID = required<Types::modules::common::ID>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::atom", "<group>"));
					choice0 = value_modules_common_ID;
					matched0 = true;
					break;
				}
				case 1: {
					value_modules_common_NUMBER = required<Types::modules::common::NUMBER>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::atom", "<group>"));
					choice0 = value_modules_common_NUMBER;
					matched0 = true;
					break;
				}
				case 2: {
					value_modules_scanner_tokens_STRING = required<Types::modules::scanner_tokens::STRING>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::atom", "<group>"));
					choice0 = value_modules_scanner_tokens_STRING;
					matched0 = true;
					break;
				}
				case 3: {
					value_modules_scanner_tokens_CGROUP = required<Types::modules::scanner_tokens::CGROUP>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::atom", "<group>"));
					choice0 = value_modules_scanner_tokens_CGROUP;
					matched0 = true;
					break;
				}
				case 4: {
					value_modules_scanner_tokens_COMMENT = required<Types::modules::scanner_tokens::COMMENT>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::atom", "<group>"));
					choice0 = value_modules_scanner_tokens_COMMENT;
					matched0 = true;
					break;
				}
				case 5: {
					value_modules_scanner_tokens_OTHER = required<Types::modules::scanner_tokens::OTHER>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::atom", "<group>"));
					choice0 = value_modules_scanner_tokens_OTHER;
					matched0 = true;
					break;
				}
				case 6: {
					value_AUTO_1 = required<Types::AUTO_1>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::atom", "<group>"));
					choice0 = value_AUTO_1;
					matched0 = true;
					break;
				}
				case 7: {
					value_COMMA = required<Types::COMMA>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::atom", "<group>"));
					choice0 = value_COMMA;
					matched0 = true;
					break;
				}
				case 8: {
					value_FULL_STOP = required<Types::FULL_STOP>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::atom", "<group>"));
					choice0 = value_FULL_STOP;
					matched0 = true;
					break;
				}
				case 9: {
					value_LESS_THAN_SIGN = required<Types::LESS_THAN_SIGN>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::atom", "<group>"));
					choice0 = value_LESS_THAN_SIGN;
					matched0 = true;
					break;
				}
				case 10: {
					value_GREATER_THAN_SIGN = required<Types::GREATER_THAN_SIGN>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::atom", "<group>"));
					choice0 = value_GREATER_THAN_SIGN;
					matched0 = true;
					break;
				}
				case 11: {
					value_COLON = required<Types::COLON>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::atom", "<group>"));
					choice0 = value_COLON;
					matched0 = true;
					break;
				}
				case 12: {
					value_ASTERISK = required<Types::ASTERISK>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::atom", "<group>"));
					choice0 = value_ASTERISK;
					matched0 = true;
					break;
				}
			}
		}
		if (!matched0) {
			return {};
		}
		::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::atom, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesIspa_dep_scannerAtom<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::atom, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesIspa_dep_scannerAtom<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::atom, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesIspa_dep_scannerBraces(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::braces, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.ispa_dep_scanner.braces", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 94, 1}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.ispa_dep_scanner.braces", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 94, 1}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 94, 1}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::ispa_dep_scanner::braces result = {};
		::ISPA_CORE::Node<Types::LEFT_CURLY_BRACKET> value_LEFT_CURLY_BRACKET;
		std::vector<::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::balanced_item>> value_list_modules_ispa_dep_scanner_balanced_item;
		::ISPA_CORE::Node<Types::RIGHT_CURLY_BRACKET> value_RIGHT_CURLY_BRACKET;
		auto recovery_scope = recovery_frame("modules::ispa_dep_scanner::braces");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>>("LEFT_CURLY_BRACKET", "LEFT_CURLY_BRACKET", "__WS", "modules::ispa_dep_scanner::balanced_item", "__WS", "RIGHT_CURLY_BRACKET");
		value_LEFT_CURLY_BRACKET = required<Types::LEFT_CURLY_BRACKET>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::ispa_dep_scanner::braces", "LEFT_CURLY_BRACKET"));
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>>("__WS", "__WS", "modules::ispa_dep_scanner::balanced_item", "__WS", "RIGHT_CURLY_BRACKET");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>>("modules::ispa_dep_scanner::balanced_item", "modules::ispa_dep_scanner::balanced_item", "__WS", "RIGHT_CURLY_BRACKET");
		value_list_modules_ispa_dep_scanner_balanced_item = range_optional<Types::modules::ispa_dep_scanner::balanced_item, Types::__WS>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerBalanced_item(cursor); });
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>>("__WS", "__WS", "RIGHT_CURLY_BRACKET");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>>("RIGHT_CURLY_BRACKET", "RIGHT_CURLY_BRACKET");
		value_RIGHT_CURLY_BRACKET = required<Types::RIGHT_CURLY_BRACKET>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::ispa_dep_scanner::braces", "RIGHT_CURLY_BRACKET"));
		::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::braces, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesIspa_dep_scannerBraces<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::braces, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesIspa_dep_scannerBraces<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::braces, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesIspa_dep_scannerSymbol_list(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::symbol_list, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.ispa_dep_scanner.symbol_list", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 61, 1}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.ispa_dep_scanner.symbol_list", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 61, 1}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 61, 1}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::ispa_dep_scanner::symbol_list result = {};
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::source_name> value_modules_ispa_dep_scanner_source_name;
		::ISPA_CORE::Node<Types::COMMA> value_COMMA;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::source_name> value_modules_ispa_dep_scanner_source_name_1;
		std::pair<::ISPA_CORE::Node<Types::COMMA>, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::source_name>> group0;
		std::vector<std::pair<::ISPA_CORE::Node<Types::COMMA>, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::source_name>>> group_list1;
		std::vector<::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::source_name>> capture_list2;
		auto recovery_scope = recovery_frame("modules::ispa_dep_scanner::symbol_list");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>>("modules::ispa_dep_scanner::source_name", "modules::ispa_dep_scanner::source_name", "__WS", "<group>");
		value_modules_ispa_dep_scanner_source_name = required<Types::modules::ispa_dep_scanner::source_name>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerSource_name(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::RIGHT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::symbol_list", "modules::ispa_dep_scanner::source_name"));
		::ISPA_CORE::assign_compatible(result.first, value_modules_ispa_dep_scanner_source_name);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>>("__WS", "__WS", "<group>");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>>("<group>", "<group>");
		// grammar ` ( COMMA  __WS?; @  modules::ispa_dep_scanner::source_name)*;`: repeat while lookahead is in FIRST
		while ((::ISPA_CORE::lookahead_kind(pos) == Tokens::COMMA::id)) {
			value_COMMA = required<Types::COMMA>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::symbol_list", "<group>"));
			skip_spaces<Types::__WS>(pos);
			value_modules_ispa_dep_scanner_source_name_1 = required<Types::modules::ispa_dep_scanner::source_name>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerSource_name(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::symbol_list", "<group>"));
			group0 = std::make_pair(value_COMMA, value_modules_ispa_dep_scanner_source_name_1);
			group_list1.push_back(group0);
			capture_list2.push_back(value_modules_ispa_dep_scanner_source_name_1);
			skip_spaces<Types::__WS>(pos);
		}
		::ISPA_CORE::assign_compatible(result.following, capture_list2);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::symbol_list, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesIspa_dep_scannerSymbol_list<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::symbol_list, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesIspa_dep_scannerSymbol_list<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::symbol_list, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesIspa_dep_scannerParentheses(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::parentheses, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.ispa_dep_scanner.parentheses", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 91, 1}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.ispa_dep_scanner.parentheses", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 91, 1}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 91, 1}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::ispa_dep_scanner::parentheses result = {};
		std::variant<std::monostate, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS>, ::ISPA_CORE::Node<Types::AUTO>, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::parentheses::AUTO_1>> choice0;
		bool matched0;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS> value_modules_ispa_dep_scanner_parentheses_LEFT_PARENTHESIS;
		::ISPA_CORE::Node<Types::AUTO> value_AUTO;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::parentheses::AUTO_1> value_modules_ispa_dep_scanner_parentheses_AUTO_1;
		std::vector<::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::balanced_item>> value_list_modules_ispa_dep_scanner_balanced_item;
		::ISPA_CORE::Node<Types::RIGHT_PARENTHESIS> value_RIGHT_PARENTHESIS;
		auto recovery_scope = recovery_frame("modules::ispa_dep_scanner::parentheses");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_PARENTHESIS::id>>("<group>", "<group>", "__WS", "modules::ispa_dep_scanner::balanced_item", "__WS", "RIGHT_PARENTHESIS");
		// grammar ` ( modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS |  AUTO |  modules::ispa_dep_scanner::parentheses::AUTO_1)`: choice; alternative selected by LL(1) lookahead
		matched0 = false;
		auto la0_1 = pos;
		switch (::ISPA_CORE::lookahead_kind(la0_1)) {
			case Tokens::AUTO::id: {
				value_AUTO = required<Types::AUTO>(la0_1, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::parentheses", "<group>"));
				choice0 = value_AUTO;
				matched0 = true;
				break;
			}
			case Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id: {
				value_modules_ispa_dep_scanner_parentheses_AUTO_1 = required<Types::modules::ispa_dep_scanner::parentheses::AUTO_1>(la0_1, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::parentheses", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_parentheses_AUTO_1;
				matched0 = true;
				break;
			}
			case Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id: {
				value_modules_ispa_dep_scanner_parentheses_LEFT_PARENTHESIS = required<Types::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS>(la0_1, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::parentheses", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_parentheses_LEFT_PARENTHESIS;
				matched0 = true;
				break;
			}
		}
		pos = la0_1;
		if (!matched0) {
			switch (recover_choice<::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id>>(pos, "modules::ispa_dep_scanner::parentheses", "modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS, AUTO, modules::ispa_dep_scanner::parentheses::AUTO_1")) {
				case 0: {
					value_modules_ispa_dep_scanner_parentheses_LEFT_PARENTHESIS = required<Types::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::parentheses", "<group>"));
					choice0 = value_modules_ispa_dep_scanner_parentheses_LEFT_PARENTHESIS;
					matched0 = true;
					break;
				}
				case 1: {
					value_AUTO = required<Types::AUTO>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::parentheses", "<group>"));
					choice0 = value_AUTO;
					matched0 = true;
					break;
				}
				case 2: {
					value_modules_ispa_dep_scanner_parentheses_AUTO_1 = required<Types::modules::ispa_dep_scanner::parentheses::AUTO_1>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::parentheses", "<group>"));
					choice0 = value_modules_ispa_dep_scanner_parentheses_AUTO_1;
					matched0 = true;
					break;
				}
			}
		}
		if (!matched0) {
			return {};
		}
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_PARENTHESIS::id>>("__WS", "__WS", "modules::ispa_dep_scanner::balanced_item", "__WS", "RIGHT_PARENTHESIS");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_PARENTHESIS::id>>("modules::ispa_dep_scanner::balanced_item", "modules::ispa_dep_scanner::balanced_item", "__WS", "RIGHT_PARENTHESIS");
		value_list_modules_ispa_dep_scanner_balanced_item = range_optional<Types::modules::ispa_dep_scanner::balanced_item, Types::__WS>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerBalanced_item(cursor); });
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_PARENTHESIS::id>>("__WS", "__WS", "RIGHT_PARENTHESIS");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_PARENTHESIS::id>>("RIGHT_PARENTHESIS", "RIGHT_PARENTHESIS");
		value_RIGHT_PARENTHESIS = required<Types::RIGHT_PARENTHESIS>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::ispa_dep_scanner::parentheses", "RIGHT_PARENTHESIS"));
		::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::parentheses, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesIspa_dep_scannerParentheses<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::parentheses, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesIspa_dep_scannerParentheses<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::parentheses, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesIspa_dep_scannerNested_rule(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::nested_rule, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.ispa_dep_scanner.nested_rule", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 87, 1}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.ispa_dep_scanner.nested_rule", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 87, 1}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 87, 1}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::ispa_dep_scanner::nested_rule result = {};
		::ISPA_CORE::Node<Types::NUMBER_SIGN> value_NUMBER_SIGN;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::declaration> value_modules_ispa_dep_scanner_declaration;
		auto recovery_scope = recovery_frame("modules::ispa_dep_scanner::nested_rule");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::NUMBER_SIGN::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>>("NUMBER_SIGN", "NUMBER_SIGN", "__WS", "modules::ispa_dep_scanner::declaration");
		value_NUMBER_SIGN = required<Types::NUMBER_SIGN>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::NUMBER_SIGN::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::ispa_dep_scanner::nested_rule", "NUMBER_SIGN"));
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>>("__WS", "__WS", "modules::ispa_dep_scanner::declaration");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>>("modules::ispa_dep_scanner::declaration", "modules::ispa_dep_scanner::declaration");
		value_modules_ispa_dep_scanner_declaration = required<Types::modules::ispa_dep_scanner::declaration>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerDeclaration(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::nested_rule", "modules::ispa_dep_scanner::declaration"));
		::ISPA_CORE::assign_compatible(result.value, value_modules_ispa_dep_scanner_declaration);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::nested_rule, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesIspa_dep_scannerNested_rule<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::nested_rule, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesIspa_dep_scannerNested_rule<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::nested_rule, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesIspa_dep_scannerBodyRegular(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::body::regular, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.ispa_dep_scanner.body.regular", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 78, 6}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.ispa_dep_scanner.body.regular", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 78, 6}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 78, 6}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::ispa_dep_scanner::body::regular result = {};
		::ISPA_CORE::Node<Types::COLON> value_COLON;
		std::vector<::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::member>> value_list_modules_ispa_dep_scanner_member;
		::ISPA_CORE::Node<Types::SEMICOLON> value_SEMICOLON;
		auto recovery_scope = recovery_frame("modules::ispa_dep_scanner::body::regular");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("COLON", "COLON", "__WS", "modules::ispa_dep_scanner::member", "__WS", "SEMICOLON");
		value_COLON = required<Types::COLON>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::ispa_dep_scanner::body::regular", "COLON"));
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("__WS", "__WS", "modules::ispa_dep_scanner::member", "__WS", "SEMICOLON");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("modules::ispa_dep_scanner::member", "modules::ispa_dep_scanner::member", "__WS", "SEMICOLON");
		value_list_modules_ispa_dep_scanner_member = range_optional<Types::modules::ispa_dep_scanner::member, Types::__WS>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerMember(cursor); });
		::ISPA_CORE::assign_compatible(result.value, value_list_modules_ispa_dep_scanner_member);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("__WS", "__WS", "SEMICOLON");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("SEMICOLON", "SEMICOLON");
		value_SEMICOLON = required<Types::SEMICOLON>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::ispa_dep_scanner::body::regular", "SEMICOLON"));
		::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::body::regular, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesIspa_dep_scannerBodyRegular<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::body::regular, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesIspa_dep_scannerBodyRegular<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::body::regular, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesIspa_dep_scannerBody(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::body, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.ispa_dep_scanner.body", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 75, 1}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.ispa_dep_scanner.body", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 75, 1}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 75, 1}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::ispa_dep_scanner::body result = {};
		std::variant<std::monostate, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::body::regular>, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::braces>> choice0;
		bool matched0;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::body::regular> value_modules_ispa_dep_scanner_body_regular;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::braces> value_modules_ispa_dep_scanner_braces;
		auto recovery_scope = recovery_frame("modules::ispa_dep_scanner::body");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>>("<group>", "<group>");
		// grammar `@  ( modules::ispa_dep_scanner::body::regular |  modules::ispa_dep_scanner::braces)`: choice; alternative selected by LL(1) lookahead
		matched0 = false;
		auto la0_1 = pos;
		switch (::ISPA_CORE::lookahead_kind(la0_1)) {
			case Tokens::COLON::id: {
				value_modules_ispa_dep_scanner_body_regular = required<Types::modules::ispa_dep_scanner::body::regular>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerBodyRegular(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id, Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::body", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_body_regular;
				matched0 = true;
				break;
			}
			case Tokens::LEFT_CURLY_BRACKET::id: {
				value_modules_ispa_dep_scanner_braces = required<Types::modules::ispa_dep_scanner::braces>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerBraces(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id, Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::body", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_braces;
				matched0 = true;
				break;
			}
		}
		pos = la0_1;
		if (!matched0) {
			switch (recover_choice<::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>>(pos, "modules::ispa_dep_scanner::body", "COLON, LEFT_CURLY_BRACKET")) {
				case 0: {
					value_modules_ispa_dep_scanner_body_regular = required<Types::modules::ispa_dep_scanner::body::regular>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerBodyRegular(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id, Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::body", "<group>"));
					choice0 = value_modules_ispa_dep_scanner_body_regular;
					matched0 = true;
					break;
				}
				case 1: {
					value_modules_ispa_dep_scanner_braces = required<Types::modules::ispa_dep_scanner::braces>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerBraces(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id, Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::body", "<group>"));
					choice0 = value_modules_ispa_dep_scanner_braces;
					matched0 = true;
					break;
				}
			}
		}
		if (!matched0) {
			return {};
		}
		::ISPA_CORE::assign_compatible(result.value, choice0);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::body, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesIspa_dep_scannerBody<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::body, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesIspa_dep_scannerBody<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::body, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesIspa_dep_scannerFail_decl(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::fail_decl, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.ispa_dep_scanner.fail_decl", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 74, 1}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.ispa_dep_scanner.fail_decl", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 74, 1}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 74, 1}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::ispa_dep_scanner::fail_decl result = {};
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::fail_decl::FAIL> value_modules_ispa_dep_scanner_fail_decl_FAIL;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::source_name> value_modules_ispa_dep_scanner_source_name;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::braces> value_modules_ispa_dep_scanner_braces;
		auto recovery_scope = recovery_frame("modules::ispa_dep_scanner::fail_decl");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>>("modules::ispa_dep_scanner::fail_decl::FAIL", "modules::ispa_dep_scanner::fail_decl::FAIL", "__WS", "modules::ispa_dep_scanner::source_name", "__WS", "modules::ispa_dep_scanner::braces");
		value_modules_ispa_dep_scanner_fail_decl_FAIL = required<Types::modules::ispa_dep_scanner::fail_decl::FAIL>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::ispa_dep_scanner::fail_decl", "modules::ispa_dep_scanner::fail_decl::FAIL"));
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>>("__WS", "__WS", "modules::ispa_dep_scanner::source_name", "__WS", "modules::ispa_dep_scanner::braces");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>>("modules::ispa_dep_scanner::source_name", "modules::ispa_dep_scanner::source_name", "__WS", "modules::ispa_dep_scanner::braces");
		value_modules_ispa_dep_scanner_source_name = required<Types::modules::ispa_dep_scanner::source_name>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerSource_name(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::fail_decl", "modules::ispa_dep_scanner::source_name"));
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>>("__WS", "__WS", "modules::ispa_dep_scanner::braces");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>>("modules::ispa_dep_scanner::braces", "modules::ispa_dep_scanner::braces");
		value_modules_ispa_dep_scanner_braces = required<Types::modules::ispa_dep_scanner::braces>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerBraces(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::fail_decl", "modules::ispa_dep_scanner::braces"));
		::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::fail_decl, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesIspa_dep_scannerFail_decl<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::fail_decl, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesIspa_dep_scannerFail_decl<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::fail_decl, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesIspa_dep_scannerDeclarationForm(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::declaration::form, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.ispa_dep_scanner.declaration.form", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 68, 6}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.ispa_dep_scanner.declaration.form", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 68, 6}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 68, 6}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::ispa_dep_scanner::declaration::form result = {};
		::ISPA_CORE::Node<Types::modules::common::name<Types::modules::common::rule_params>> value_modules_common_name;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::body> value_modules_ispa_dep_scanner_body;
		auto recovery_scope = recovery_frame("modules::ispa_dep_scanner::declaration::form");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>>("modules::common::name", "modules::common::name", "__WS", "modules::ispa_dep_scanner::body");
		value_modules_common_name = required<Types::modules::common::name<Types::modules::common::rule_params>>(pos, [&](IT rule_pos) {
return modulesCommonName<Types::modules::common::rule_params>(rule_pos);
}, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id, Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::declaration::form", "modules::common::name"));
		::ISPA_CORE::assign_compatible(result.name, value_modules_common_name);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>>("__WS", "__WS", "modules::ispa_dep_scanner::body");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>>("modules::ispa_dep_scanner::body", "modules::ispa_dep_scanner::body");
		value_modules_ispa_dep_scanner_body = required<Types::modules::ispa_dep_scanner::body>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerBody(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id, Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::declaration::form", "modules::ispa_dep_scanner::body"));
		::ISPA_CORE::assign_compatible(result.body, value_modules_ispa_dep_scanner_body);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::declaration::form, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesIspa_dep_scannerDeclarationForm<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::declaration::form, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesIspa_dep_scannerDeclarationForm<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::declaration::form, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesIspa_dep_scannerDeclaration(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::declaration, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.ispa_dep_scanner.declaration", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 65, 1}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.ispa_dep_scanner.declaration", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 65, 1}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 65, 1}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::ispa_dep_scanner::declaration result = {};
		std::vector<::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::modifier>> value_list_modules_ispa_dep_scanner_modifier;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::declaration::form> value_modules_ispa_dep_scanner_declaration_form;
		auto recovery_scope = recovery_frame("modules::ispa_dep_scanner::declaration");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::NUMBER_SIGN::id, Tokens::modules::scanner_tokens::CGROUP::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>>("modules::ispa_dep_scanner::modifier", "modules::ispa_dep_scanner::modifier", "__WS", "modules::ispa_dep_scanner::declaration::form");
		value_list_modules_ispa_dep_scanner_modifier = range_optional<Types::modules::ispa_dep_scanner::modifier, Types::__WS>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerModifier(cursor); });
		::ISPA_CORE::assign_compatible(result.modifiers, value_list_modules_ispa_dep_scanner_modifier);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>>("__WS", "__WS", "modules::ispa_dep_scanner::declaration::form");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>>("modules::ispa_dep_scanner::declaration::form", "modules::ispa_dep_scanner::declaration::form");
		value_modules_ispa_dep_scanner_declaration_form = required<Types::modules::ispa_dep_scanner::declaration::form>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerDeclarationForm(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::declaration", "modules::ispa_dep_scanner::declaration::form"));
		::ISPA_CORE::assign_compatible(result.value, value_modules_ispa_dep_scanner_declaration_form);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::declaration, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesIspa_dep_scannerDeclaration<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::declaration, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesIspa_dep_scannerDeclaration<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::declaration, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesIspa_dep_scannerMember(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::member, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.ispa_dep_scanner.member", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 83, 1}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.ispa_dep_scanner.member", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 83, 1}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 83, 1}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::ispa_dep_scanner::member result = {};
		std::variant<std::monostate, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::nested_rule>, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::parentheses>, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::braces>, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::atom>> choice0;
		bool matched0;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::nested_rule> value_modules_ispa_dep_scanner_nested_rule;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::parentheses> value_modules_ispa_dep_scanner_parentheses;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::braces> value_modules_ispa_dep_scanner_braces;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::atom> value_modules_ispa_dep_scanner_atom;
		auto recovery_scope = recovery_frame("modules::ispa_dep_scanner::member");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>>("<group>", "<group>");
		// grammar `@  ( modules::ispa_dep_scanner::nested_rule |  modules::ispa_dep_scanner::parentheses |  modules::ispa_dep_scanner::braces |  modules::ispa_dep_scanner::atom)`: choice; alternative selected by LL(1) lookahead
		matched0 = false;
		auto la0_1 = pos;
		switch (::ISPA_CORE::lookahead_kind(la0_1)) {
			case Tokens::ASTERISK::id: {
				value_modules_ispa_dep_scanner_atom = required<Types::modules::ispa_dep_scanner::atom>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerAtom(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::member", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_atom;
				matched0 = true;
				break;
			}
			case Tokens::AUTO::id: {
				value_modules_ispa_dep_scanner_parentheses = required<Types::modules::ispa_dep_scanner::parentheses>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerParentheses(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::member", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_parentheses;
				matched0 = true;
				break;
			}
			case Tokens::AUTO_1::id: {
				value_modules_ispa_dep_scanner_atom = required<Types::modules::ispa_dep_scanner::atom>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerAtom(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::member", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_atom;
				matched0 = true;
				break;
			}
			case Tokens::COLON::id: {
				value_modules_ispa_dep_scanner_atom = required<Types::modules::ispa_dep_scanner::atom>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerAtom(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::member", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_atom;
				matched0 = true;
				break;
			}
			case Tokens::COMMA::id: {
				value_modules_ispa_dep_scanner_atom = required<Types::modules::ispa_dep_scanner::atom>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerAtom(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::member", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_atom;
				matched0 = true;
				break;
			}
			case Tokens::FULL_STOP::id: {
				value_modules_ispa_dep_scanner_atom = required<Types::modules::ispa_dep_scanner::atom>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerAtom(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::member", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_atom;
				matched0 = true;
				break;
			}
			case Tokens::GREATER_THAN_SIGN::id: {
				value_modules_ispa_dep_scanner_atom = required<Types::modules::ispa_dep_scanner::atom>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerAtom(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::member", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_atom;
				matched0 = true;
				break;
			}
			case Tokens::LEFT_CURLY_BRACKET::id: {
				value_modules_ispa_dep_scanner_braces = required<Types::modules::ispa_dep_scanner::braces>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerBraces(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::member", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_braces;
				matched0 = true;
				break;
			}
			case Tokens::LESS_THAN_SIGN::id: {
				value_modules_ispa_dep_scanner_atom = required<Types::modules::ispa_dep_scanner::atom>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerAtom(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::member", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_atom;
				matched0 = true;
				break;
			}
			case Tokens::NUMBER_SIGN::id: {
				value_modules_ispa_dep_scanner_nested_rule = required<Types::modules::ispa_dep_scanner::nested_rule>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerNested_rule(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::member", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_nested_rule;
				matched0 = true;
				break;
			}
			case Tokens::modules::common::ID::id: {
				value_modules_ispa_dep_scanner_atom = required<Types::modules::ispa_dep_scanner::atom>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerAtom(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::member", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_atom;
				matched0 = true;
				break;
			}
			case Tokens::modules::common::NUMBER::id: {
				value_modules_ispa_dep_scanner_atom = required<Types::modules::ispa_dep_scanner::atom>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerAtom(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::member", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_atom;
				matched0 = true;
				break;
			}
			case Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id: {
				value_modules_ispa_dep_scanner_parentheses = required<Types::modules::ispa_dep_scanner::parentheses>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerParentheses(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::member", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_parentheses;
				matched0 = true;
				break;
			}
			case Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id: {
				value_modules_ispa_dep_scanner_parentheses = required<Types::modules::ispa_dep_scanner::parentheses>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerParentheses(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::member", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_parentheses;
				matched0 = true;
				break;
			}
			case Tokens::modules::scanner_tokens::CGROUP::id: {
				value_modules_ispa_dep_scanner_atom = required<Types::modules::ispa_dep_scanner::atom>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerAtom(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::member", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_atom;
				matched0 = true;
				break;
			}
			case Tokens::modules::scanner_tokens::COMMENT::id: {
				value_modules_ispa_dep_scanner_atom = required<Types::modules::ispa_dep_scanner::atom>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerAtom(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::member", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_atom;
				matched0 = true;
				break;
			}
			case Tokens::modules::scanner_tokens::OTHER::id: {
				value_modules_ispa_dep_scanner_atom = required<Types::modules::ispa_dep_scanner::atom>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerAtom(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::member", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_atom;
				matched0 = true;
				break;
			}
			case Tokens::modules::scanner_tokens::STRING::id: {
				value_modules_ispa_dep_scanner_atom = required<Types::modules::ispa_dep_scanner::atom>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerAtom(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::member", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_atom;
				matched0 = true;
				break;
			}
		}
		pos = la0_1;
		if (!matched0) {
			switch (recover_choice<::ISPA_CORE::Recovery::Tokens<Tokens::NUMBER_SIGN::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>>(pos, "modules::ispa_dep_scanner::member", "NUMBER_SIGN, AUTO, modules::ispa_dep_scanner::parentheses::AUTO_1, modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS, LEFT_CURLY_BRACKET, ASTERISK, AUTO_1, COLON, COMMA, FULL_STOP, GREATER_THAN_SIGN, LESS_THAN_SIGN, modules::common::ID, modules::common::NUMBER, modules::scanner_tokens::CGROUP, modules::scanner_tokens::COMMENT, modules::scanner_tokens::OTHER, modules::scanner_tokens::STRING")) {
				case 0: {
					value_modules_ispa_dep_scanner_nested_rule = required<Types::modules::ispa_dep_scanner::nested_rule>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerNested_rule(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::member", "<group>"));
					choice0 = value_modules_ispa_dep_scanner_nested_rule;
					matched0 = true;
					break;
				}
				case 1: {
					value_modules_ispa_dep_scanner_parentheses = required<Types::modules::ispa_dep_scanner::parentheses>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerParentheses(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::member", "<group>"));
					choice0 = value_modules_ispa_dep_scanner_parentheses;
					matched0 = true;
					break;
				}
				case 2: {
					value_modules_ispa_dep_scanner_braces = required<Types::modules::ispa_dep_scanner::braces>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerBraces(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::member", "<group>"));
					choice0 = value_modules_ispa_dep_scanner_braces;
					matched0 = true;
					break;
				}
				case 3: {
					value_modules_ispa_dep_scanner_atom = required<Types::modules::ispa_dep_scanner::atom>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerAtom(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::member", "<group>"));
					choice0 = value_modules_ispa_dep_scanner_atom;
					matched0 = true;
					break;
				}
			}
		}
		if (!matched0) {
			return {};
		}
		::ISPA_CORE::assign_compatible(result.value, choice0);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::member, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesIspa_dep_scannerMember<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::member, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesIspa_dep_scannerMember<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::member, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesIspa_dep_scannerSource_name(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::source_name, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.ispa_dep_scanner.source_name", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 57, 1}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.ispa_dep_scanner.source_name", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 57, 1}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 57, 1}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::ispa_dep_scanner::source_name result = {};
		::ISPA_CORE::Node<Types::modules::common::name<Types::modules::common::rule_params>> value_modules_common_name;
		auto recovery_scope = recovery_frame("modules::ispa_dep_scanner::source_name");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>>("modules::common::name", "modules::common::name");
		value_modules_common_name = required<Types::modules::common::name<Types::modules::common::rule_params>>(pos, [&](IT rule_pos) {
return modulesCommonName<Types::modules::common::rule_params>(rule_pos);
}, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::source_name", "modules::common::name"));
		::ISPA_CORE::assign_compatible(result.value, value_modules_common_name);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::source_name, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesIspa_dep_scannerSource_name<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::source_name, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesIspa_dep_scannerSource_name<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::source_name, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesIspa_dep_scannerMod_detachDetach_valueFeature_ref(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_detach::detach_value::feature_ref, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.ispa_dep_scanner.mod_detach.detach_value.feature_ref", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 42, 10}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.ispa_dep_scanner.mod_detach.detach_value.feature_ref", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 42, 10}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 42, 10}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::ispa_dep_scanner::mod_detach::detach_value::feature_ref result = {};
		::ISPA_CORE::Node<Types::FEATURE> value_FEATURE;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::source_name> value_modules_ispa_dep_scanner_source_name;
		auto recovery_scope = recovery_frame("modules::ispa_dep_scanner::mod_detach::detach_value::feature_ref");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>>("FEATURE", "FEATURE", "__WS", "modules::ispa_dep_scanner::source_name");
		value_FEATURE = required<Types::FEATURE>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::ispa_dep_scanner::mod_detach::detach_value::feature_ref", "FEATURE"));
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>>("__WS", "__WS", "modules::ispa_dep_scanner::source_name");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>>("modules::ispa_dep_scanner::source_name", "modules::ispa_dep_scanner::source_name");
		value_modules_ispa_dep_scanner_source_name = required<Types::modules::ispa_dep_scanner::source_name>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerSource_name(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::mod_detach::detach_value::feature_ref", "modules::ispa_dep_scanner::source_name"));
		::ISPA_CORE::assign_compatible(result.value, value_modules_ispa_dep_scanner_source_name);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_detach::detach_value::feature_ref, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesIspa_dep_scannerMod_detachDetach_valueFeature_ref<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_detach::detach_value::feature_ref, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesIspa_dep_scannerMod_detachDetach_valueFeature_ref<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_detach::detach_value::feature_ref, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesIspa_dep_scannerModifier(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::modifier, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.ispa_dep_scanner.modifier", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 73, 1}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.ispa_dep_scanner.modifier", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 73, 1}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 73, 1}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::ispa_dep_scanner::modifier result = {};
		::ISPA_CORE::Node<Types::NUMBER_SIGN> value_NUMBER_SIGN;
		::ISPA_CORE::Node<Types::modules::scanner_tokens::CGROUP> value_modules_scanner_tokens_CGROUP;
		auto recovery_scope = recovery_frame("modules::ispa_dep_scanner::modifier");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::NUMBER_SIGN::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::scanner_tokens::CGROUP::id>>("NUMBER_SIGN", "NUMBER_SIGN", "__WS", "modules::scanner_tokens::CGROUP");
		value_NUMBER_SIGN = required<Types::NUMBER_SIGN>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::NUMBER_SIGN::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::scanner_tokens::CGROUP::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::NUMBER_SIGN::id, Tokens::modules::scanner_tokens::CGROUP::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::ispa_dep_scanner::modifier", "NUMBER_SIGN"));
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::scanner_tokens::CGROUP::id>>("__WS", "__WS", "modules::scanner_tokens::CGROUP");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::modules::scanner_tokens::CGROUP::id>>("modules::scanner_tokens::CGROUP", "modules::scanner_tokens::CGROUP");
		value_modules_scanner_tokens_CGROUP = required<Types::modules::scanner_tokens::CGROUP>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::modules::scanner_tokens::CGROUP::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::scanner_tokens::CGROUP::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::ispa_dep_scanner::modifier", "modules::scanner_tokens::CGROUP"));
		::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::modifier, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesIspa_dep_scannerModifier<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::modifier, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesIspa_dep_scannerModifier<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::modifier, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesIspa_dep_scannerMod_importImport_valueModule_import(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_import::import_value::module_import, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.ispa_dep_scanner.mod_import.import_value.module_import", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 18, 10}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.ispa_dep_scanner.mod_import.import_value.module_import", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 18, 10}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 18, 10}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::ispa_dep_scanner::mod_import::import_value::module_import result = {};
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::module_path> value_modules_ispa_dep_scanner_module_path;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS> value_modules_ispa_dep_scanner_mod_import_import_value_module_import_AS;
		::ISPA_CORE::Node<Types::modules::common::ID> value_modules_common_ID;
		std::pair<::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS>, ::ISPA_CORE::Node<Types::modules::common::ID>> group0;
		auto recovery_scope = recovery_frame("modules::ispa_dep_scanner::mod_import::import_value::module_import");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>>("modules::ispa_dep_scanner::module_path", "modules::ispa_dep_scanner::module_path", "__WS", "<group>");
		value_modules_ispa_dep_scanner_module_path = required<Types::modules::ispa_dep_scanner::module_path>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerModule_path(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::mod_import::import_value::module_import", "modules::ispa_dep_scanner::module_path"));
		::ISPA_CORE::assign_compatible(result.source, value_modules_ispa_dep_scanner_module_path);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>>("__WS", "__WS", "<group>");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>>("<group>", "<group>");
		// grammar ` ( modules::ispa_dep_scanner::mod_import::import_value::module_import::AS  __WS?; @  modules::common::ID)?;`: optional; entered only when lookahead is in FIRST
		if ((::ISPA_CORE::lookahead_kind(pos) == Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id)) {
			value_modules_ispa_dep_scanner_mod_import_import_value_module_import_AS = required<Types::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::mod_import::import_value::module_import", "<group>"));
			skip_spaces<Types::__WS>(pos);
			value_modules_common_ID = required<Types::modules::common::ID>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::mod_import::import_value::module_import", "<group>"));
			group0 = std::make_pair(value_modules_ispa_dep_scanner_mod_import_import_value_module_import_AS, value_modules_common_ID);
		}
		::ISPA_CORE::assign_compatible(result.alias, value_modules_common_ID);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_import::import_value::module_import, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesIspa_dep_scannerMod_importImport_valueModule_import<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_import::import_value::module_import, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesIspa_dep_scannerMod_importImport_valueModule_import<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_import::import_value::module_import, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesIspa_dep_scannerMod_importImport_value(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_import::import_value, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.ispa_dep_scanner.mod_import.import_value", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 15, 6}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.ispa_dep_scanner.mod_import.import_value", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 15, 6}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 15, 6}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::ispa_dep_scanner::mod_import::import_value result = {};
		std::variant<std::monostate, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_import::import_value::module_import>, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_import::import_value::from_import>> choice0;
		bool matched0;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_import::import_value::module_import> value_modules_ispa_dep_scanner_mod_import_import_value_module_import;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_import::import_value::from_import> value_modules_ispa_dep_scanner_mod_import_import_value_from_import;
		auto recovery_scope = recovery_frame("modules::ispa_dep_scanner::mod_import::import_value");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>>("<group>", "<group>");
		// grammar `@  ( modules::ispa_dep_scanner::mod_import::import_value::module_import |  modules::ispa_dep_scanner::mod_import::import_value::from_import)`: choice; alternative selected by LL(1) lookahead
		matched0 = false;
		auto la0_1 = pos;
		switch (::ISPA_CORE::lookahead_kind(la0_1)) {
			case Tokens::ASTERISK::id: {
				value_modules_ispa_dep_scanner_mod_import_import_value_from_import = required<Types::modules::ispa_dep_scanner::mod_import::import_value::from_import>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerMod_importImport_valueFrom_import(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::mod_import::import_value", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_mod_import_import_value_from_import;
				matched0 = true;
				break;
			}
			case Tokens::LEFT_CURLY_BRACKET::id: {
				value_modules_ispa_dep_scanner_mod_import_import_value_from_import = required<Types::modules::ispa_dep_scanner::mod_import::import_value::from_import>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerMod_importImport_valueFrom_import(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::mod_import::import_value", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_mod_import_import_value_from_import;
				matched0 = true;
				break;
			}
			case Tokens::modules::common::ID::id: {
				value_modules_ispa_dep_scanner_mod_import_import_value_module_import = required<Types::modules::ispa_dep_scanner::mod_import::import_value::module_import>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerMod_importImport_valueModule_import(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::mod_import::import_value", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_mod_import_import_value_module_import;
				matched0 = true;
				break;
			}
		}
		pos = la0_1;
		if (!matched0) {
			switch (recover_choice<::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id>>(pos, "modules::ispa_dep_scanner::mod_import::import_value", "modules::common::ID, ASTERISK, LEFT_CURLY_BRACKET")) {
				case 0: {
					value_modules_ispa_dep_scanner_mod_import_import_value_module_import = required<Types::modules::ispa_dep_scanner::mod_import::import_value::module_import>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerMod_importImport_valueModule_import(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::mod_import::import_value", "<group>"));
					choice0 = value_modules_ispa_dep_scanner_mod_import_import_value_module_import;
					matched0 = true;
					break;
				}
				case 1: {
					value_modules_ispa_dep_scanner_mod_import_import_value_from_import = required<Types::modules::ispa_dep_scanner::mod_import::import_value::from_import>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerMod_importImport_valueFrom_import(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::mod_import::import_value", "<group>"));
					choice0 = value_modules_ispa_dep_scanner_mod_import_import_value_from_import;
					matched0 = true;
					break;
				}
			}
		}
		if (!matched0) {
			return {};
		}
		::ISPA_CORE::assign_compatible(result.value, choice0);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_import::import_value, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesIspa_dep_scannerMod_importImport_value<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_import::import_value, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesIspa_dep_scannerMod_importImport_value<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_import::import_value, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesIspa_dep_scannerMod_importExternal_import(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_import::external_import, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.ispa_dep_scanner.mod_import.external_import", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 11, 6}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.ispa_dep_scanner.mod_import.external_import", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 11, 6}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 11, 6}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::ispa_dep_scanner::mod_import::external_import result = {};
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_import::external_import::EXTERN> value_modules_ispa_dep_scanner_mod_import_external_import_EXTERN;
		::ISPA_CORE::Node<Types::modules::common::ID> value_modules_common_ID;
		auto recovery_scope = recovery_frame("modules::ispa_dep_scanner::mod_import::external_import");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>>("modules::ispa_dep_scanner::mod_import::external_import::EXTERN", "modules::ispa_dep_scanner::mod_import::external_import::EXTERN", "__WS", "modules::common::ID");
		value_modules_ispa_dep_scanner_mod_import_external_import_EXTERN = required<Types::modules::ispa_dep_scanner::mod_import::external_import::EXTERN>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::ispa_dep_scanner::mod_import::external_import", "modules::ispa_dep_scanner::mod_import::external_import::EXTERN"));
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>>("__WS", "__WS", "modules::common::ID");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>>("modules::common::ID", "modules::common::ID");
		value_modules_common_ID = required<Types::modules::common::ID>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::ispa_dep_scanner::mod_import::external_import", "modules::common::ID"));
		::ISPA_CORE::assign_compatible(result.value, value_modules_common_ID);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_import::external_import, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesIspa_dep_scannerMod_importExternal_import<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_import::external_import, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesIspa_dep_scannerMod_importExternal_import<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_import::external_import, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesIspa_dep_scannerMod_import(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_import, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.ispa_dep_scanner.mod_import", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 8, 1}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.ispa_dep_scanner.mod_import", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 8, 1}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 8, 1}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::ispa_dep_scanner::mod_import result = {};
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_import::IMPORT> value_modules_ispa_dep_scanner_mod_import_IMPORT;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_import::PY> value_modules_ispa_dep_scanner_mod_import_PY;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_import::external_import> value_modules_ispa_dep_scanner_mod_import_external_import;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_import::import_value> value_modules_ispa_dep_scanner_mod_import_import_value;
		::ISPA_CORE::Node<Types::SEMICOLON> value_SEMICOLON;
		auto recovery_scope = recovery_frame("modules::ispa_dep_scanner::mod_import");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::PY::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("modules::ispa_dep_scanner::mod_import::IMPORT", "modules::ispa_dep_scanner::mod_import::IMPORT", "__WS", "modules::ispa_dep_scanner::mod_import::PY", "__WS", "modules::ispa_dep_scanner::mod_import::external_import", "__WS", "modules::ispa_dep_scanner::mod_import::import_value", "__WS", "SEMICOLON");
		value_modules_ispa_dep_scanner_mod_import_IMPORT = required<Types::modules::ispa_dep_scanner::mod_import::IMPORT>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::PY::id, Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_import::PY::id, Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::ispa_dep_scanner::mod_import", "modules::ispa_dep_scanner::mod_import::IMPORT"));
		::ISPA_CORE::assign_compatible(result.begin, value_modules_ispa_dep_scanner_mod_import_IMPORT);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::PY::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("__WS", "__WS", "modules::ispa_dep_scanner::mod_import::PY", "__WS", "modules::ispa_dep_scanner::mod_import::external_import", "__WS", "modules::ispa_dep_scanner::mod_import::import_value", "__WS", "SEMICOLON");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::PY::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("modules::ispa_dep_scanner::mod_import::PY", "modules::ispa_dep_scanner::mod_import::PY", "__WS", "modules::ispa_dep_scanner::mod_import::external_import", "__WS", "modules::ispa_dep_scanner::mod_import::import_value", "__WS", "SEMICOLON");
		value_modules_ispa_dep_scanner_mod_import_PY = optional<Types::modules::ispa_dep_scanner::mod_import::PY>(pos);
		::ISPA_CORE::assign_compatible(result.py_import, value_modules_ispa_dep_scanner_mod_import_PY);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("__WS", "__WS", "modules::ispa_dep_scanner::mod_import::external_import", "__WS", "modules::ispa_dep_scanner::mod_import::import_value", "__WS", "SEMICOLON");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("modules::ispa_dep_scanner::mod_import::external_import", "modules::ispa_dep_scanner::mod_import::external_import", "__WS", "modules::ispa_dep_scanner::mod_import::import_value", "__WS", "SEMICOLON");
		value_modules_ispa_dep_scanner_mod_import_external_import = optional<Types::modules::ispa_dep_scanner::mod_import::external_import>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerMod_importExternal_import(cursor); });
		::ISPA_CORE::assign_compatible(result.external, value_modules_ispa_dep_scanner_mod_import_external_import);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("__WS", "__WS", "modules::ispa_dep_scanner::mod_import::import_value", "__WS", "SEMICOLON");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("modules::ispa_dep_scanner::mod_import::import_value", "modules::ispa_dep_scanner::mod_import::import_value", "__WS", "SEMICOLON");
		value_modules_ispa_dep_scanner_mod_import_import_value = required<Types::modules::ispa_dep_scanner::mod_import::import_value>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerMod_importImport_value(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::mod_import", "modules::ispa_dep_scanner::mod_import::import_value"));
		::ISPA_CORE::assign_compatible(result.value, value_modules_ispa_dep_scanner_mod_import_import_value);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("__WS", "__WS", "SEMICOLON");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("SEMICOLON", "SEMICOLON");
		value_SEMICOLON = required<Types::SEMICOLON>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::ispa_dep_scanner::mod_import", "SEMICOLON"));
		::ISPA_CORE::assign_compatible(result.end, value_SEMICOLON);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_import, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesIspa_dep_scannerMod_import<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_import, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesIspa_dep_scannerMod_import<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_import, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesCommonRule_params(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::common::rule_params, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.common.rule_params", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa/grammar/common.isc", 22, 1}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.common.rule_params", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa/grammar/common.isc", 22, 1}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa/grammar/common.isc", 22, 1}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::common::rule_params result = {};
		::ISPA_CORE::Node<Types::AUTO> value_AUTO;
		::ISPA_CORE::Node<Types::modules::common::param> value_modules_common_param;
		::ISPA_CORE::Node<Types::RIGHT_PARENTHESIS> value_RIGHT_PARENTHESIS;
		auto recovery_scope = recovery_frame("modules::common::rule_params");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_PARENTHESIS::id>>("AUTO", "AUTO", "__WS", "modules::common::param", "__WS", "RIGHT_PARENTHESIS");
		value_AUTO = required<Types::AUTO>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id, Tokens::COMMA::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::common::rule_params", "AUTO"));
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_PARENTHESIS::id>>("__WS", "__WS", "modules::common::param", "__WS", "RIGHT_PARENTHESIS");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_PARENTHESIS::id>>("modules::common::param", "modules::common::param", "__WS", "RIGHT_PARENTHESIS");
		value_modules_common_param = optional<Types::modules::common::param>(pos, [this](auto cursor) { return this->modulesCommonParam(cursor); });
		::ISPA_CORE::assign_compatible(result.value, value_modules_common_param);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_PARENTHESIS::id>>("__WS", "__WS", "RIGHT_PARENTHESIS");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_PARENTHESIS::id>>("RIGHT_PARENTHESIS", "RIGHT_PARENTHESIS");
		value_RIGHT_PARENTHESIS = required<Types::RIGHT_PARENTHESIS>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id, Tokens::COMMA::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id, Tokens::COMMA::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::common::rule_params", "RIGHT_PARENTHESIS"));
		::ISPA_CORE::MatchResult<Types::modules::common::rule_params, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesCommonRule_params<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::common::rule_params, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesCommonRule_params<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::common::rule_params, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesCommonType_annotation(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::common::type_annotation, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.common.type_annotation", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa/grammar/common.isc", 38, 1}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.common.type_annotation", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa/grammar/common.isc", 38, 1}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa/grammar/common.isc", 38, 1}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::common::type_annotation result = {};
		::ISPA_CORE::Node<Types::modules::common::ID> value_modules_common_ID;
		::ISPA_CORE::Node<Types::FULL_STOP> value_FULL_STOP;
		::ISPA_CORE::Node<Types::modules::common::ID> value_modules_common_ID_1;
		std::pair<::ISPA_CORE::Node<Types::FULL_STOP>, ::ISPA_CORE::Node<Types::modules::common::ID>> group0;
		std::vector<std::pair<::ISPA_CORE::Node<Types::FULL_STOP>, ::ISPA_CORE::Node<Types::modules::common::ID>>> group_list1;
		std::vector<::ISPA_CORE::Node<Types::modules::common::ID>> capture_list2;
		auto recovery_scope = recovery_frame("modules::common::type_annotation");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>>("modules::common::ID", "modules::common::ID", "__WS", "<group>");
		value_modules_common_ID = required<Types::modules::common::ID>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::RIGHT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::RIGHT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::common::type_annotation", "modules::common::ID"));
		::ISPA_CORE::assign_compatible(result.base, value_modules_common_ID);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>>("__WS", "__WS", "<group>");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>>("<group>", "<group>");
		// grammar ` ( FULL_STOP  __WS?; @  modules::common::ID)*;`: repeat while lookahead is in FIRST
		while ((::ISPA_CORE::lookahead_kind(pos) == Tokens::FULL_STOP::id)) {
			value_FULL_STOP = required<Types::FULL_STOP>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::RIGHT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::RIGHT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::common::type_annotation", "<group>"));
			skip_spaces<Types::__WS>(pos);
			value_modules_common_ID_1 = required<Types::modules::common::ID>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::RIGHT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::RIGHT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::common::type_annotation", "<group>"));
			group0 = std::make_pair(value_FULL_STOP, value_modules_common_ID_1);
			group_list1.push_back(group0);
			capture_list2.push_back(value_modules_common_ID_1);
			skip_spaces<Types::__WS>(pos);
		}
		::ISPA_CORE::assign_compatible(result.nested, capture_list2);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		::ISPA_CORE::MatchResult<Types::modules::common::type_annotation, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesCommonType_annotation<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::common::type_annotation, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesCommonType_annotation<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::common::type_annotation, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesIspa_dep_scannerModule_path(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::module_path, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.ispa_dep_scanner.module_path", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 52, 1}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.ispa_dep_scanner.module_path", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 52, 1}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 52, 1}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::ispa_dep_scanner::module_path result = {};
		::ISPA_CORE::Node<Types::modules::common::ID> value_modules_common_ID;
		::ISPA_CORE::Node<Types::FULL_STOP> value_FULL_STOP;
		::ISPA_CORE::Node<Types::modules::common::ID> value_modules_common_ID_1;
		std::pair<::ISPA_CORE::Node<Types::FULL_STOP>, ::ISPA_CORE::Node<Types::modules::common::ID>> group0;
		std::vector<std::pair<::ISPA_CORE::Node<Types::FULL_STOP>, ::ISPA_CORE::Node<Types::modules::common::ID>>> group_list1;
		std::vector<::ISPA_CORE::Node<Types::modules::common::ID>> capture_list2;
		auto recovery_scope = recovery_frame("modules::ispa_dep_scanner::module_path");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>>("modules::common::ID", "modules::common::ID", "__WS", "<group>");
		value_modules_common_ID = required<Types::modules::common::ID>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO_1::id, Tokens::FULL_STOP::id, Tokens::LESS_THAN_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO_1::id, Tokens::LESS_THAN_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::ispa_dep_scanner::module_path", "modules::common::ID"));
		::ISPA_CORE::assign_compatible(result.primary, value_modules_common_ID);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>>("__WS", "__WS", "<group>");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>>("<group>", "<group>");
		// grammar ` ( FULL_STOP  __WS?; @  modules::common::ID)*;`: repeat while lookahead is in FIRST
		while ((::ISPA_CORE::lookahead_kind(pos) == Tokens::FULL_STOP::id)) {
			value_FULL_STOP = required<Types::FULL_STOP>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO_1::id, Tokens::LESS_THAN_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO_1::id, Tokens::LESS_THAN_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::module_path", "<group>"));
			skip_spaces<Types::__WS>(pos);
			value_modules_common_ID_1 = required<Types::modules::common::ID>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO_1::id, Tokens::LESS_THAN_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO_1::id, Tokens::LESS_THAN_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::module_path", "<group>"));
			group0 = std::make_pair(value_FULL_STOP, value_modules_common_ID_1);
			group_list1.push_back(group0);
			capture_list2.push_back(value_modules_common_ID_1);
			skip_spaces<Types::__WS>(pos);
		}
		::ISPA_CORE::assign_compatible(result.nested, capture_list2);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::module_path, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesIspa_dep_scannerModule_path<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::module_path, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesIspa_dep_scannerModule_path<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::module_path, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesEntryMain(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::entry::main, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.entry.main", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/entry.isc", 4, 1}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.entry.main", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/entry.isc", 4, 1}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/entry.isc", 4, 1}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::entry::main result = {};
		std::variant<std::monostate, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_module>, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_import>, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_detach>, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::feature_decl>, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::declaration>, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::fail_decl>, ::ISPA_CORE::Node<Types::modules::scanner_tokens::COMMENT>> choice0;
		bool matched0;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_module> value_modules_ispa_dep_scanner_mod_module;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_import> value_modules_ispa_dep_scanner_mod_import;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_detach> value_modules_ispa_dep_scanner_mod_detach;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::feature_decl> value_modules_ispa_dep_scanner_feature_decl;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::declaration> value_modules_ispa_dep_scanner_declaration;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::fail_decl> value_modules_ispa_dep_scanner_fail_decl;
		::ISPA_CORE::Node<Types::modules::scanner_tokens::COMMENT> value_modules_scanner_tokens_COMMENT;
		std::vector<std::variant<std::monostate, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_module>, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_import>, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_detach>, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::feature_decl>, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::declaration>, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::fail_decl>, ::ISPA_CORE::Node<Types::modules::scanner_tokens::COMMENT>>> group_list1;
		auto recovery_scope = recovery_frame("modules::entry::main");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_import::PY::id, Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>>("<group>", "<group>");
		// grammar `@  ( ( modules::ispa_dep_scanner::mod_module |  modules::ispa_dep_scanner::mod_import |  modules::ispa_dep_scanner::mod_detach |  modules::ispa_dep_scanner::feature_decl |  modules::ispa_dep_scanner::declaration |  modules::ispa_dep_scanner::fail_decl |  modules::scanner_tokens::COMMENT))*;`: repeat while lookahead is in FIRST
		while ((::ISPA_CORE::lookahead_kind(pos) == Tokens::FEATURE::id || ::ISPA_CORE::lookahead_kind(pos) == Tokens::NUMBER_SIGN::id || ::ISPA_CORE::lookahead_kind(pos) == Tokens::modules::common::ID::id || ::ISPA_CORE::lookahead_kind(pos) == Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id || ::ISPA_CORE::lookahead_kind(pos) == Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id || ::ISPA_CORE::lookahead_kind(pos) == Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id || ::ISPA_CORE::lookahead_kind(pos) == Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id || ::ISPA_CORE::lookahead_kind(pos) == Tokens::modules::scanner_tokens::COMMENT::id)) {
			// grammar ` ( modules::ispa_dep_scanner::mod_module |  modules::ispa_dep_scanner::mod_import |  modules::ispa_dep_scanner::mod_detach |  modules::ispa_dep_scanner::feature_decl |  modules::ispa_dep_scanner::declaration |  modules::ispa_dep_scanner::fail_decl |  modules::scanner_tokens::COMMENT)`: choice; alternative selected by LL(1) lookahead
			matched0 = false;
			auto la0_1 = pos;
			switch (::ISPA_CORE::lookahead_kind(la0_1)) {
				case Tokens::FEATURE::id: {
					value_modules_ispa_dep_scanner_feature_decl = required<Types::modules::ispa_dep_scanner::feature_decl>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerFeature_decl(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_import::PY::id, Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::entry::main", "<group>"));
					choice0 = value_modules_ispa_dep_scanner_feature_decl;
					matched0 = true;
					break;
				}
				case Tokens::NUMBER_SIGN::id: {
					value_modules_ispa_dep_scanner_declaration = required<Types::modules::ispa_dep_scanner::declaration>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerDeclaration(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_import::PY::id, Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::entry::main", "<group>"));
					choice0 = value_modules_ispa_dep_scanner_declaration;
					matched0 = true;
					break;
				}
				case Tokens::modules::common::ID::id: {
					value_modules_ispa_dep_scanner_declaration = required<Types::modules::ispa_dep_scanner::declaration>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerDeclaration(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_import::PY::id, Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::entry::main", "<group>"));
					choice0 = value_modules_ispa_dep_scanner_declaration;
					matched0 = true;
					break;
				}
				case Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id: {
					value_modules_ispa_dep_scanner_fail_decl = required<Types::modules::ispa_dep_scanner::fail_decl>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerFail_decl(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_import::PY::id, Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::entry::main", "<group>"));
					choice0 = value_modules_ispa_dep_scanner_fail_decl;
					matched0 = true;
					break;
				}
				case Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id: {
					value_modules_ispa_dep_scanner_mod_detach = required<Types::modules::ispa_dep_scanner::mod_detach>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerMod_detach(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_import::PY::id, Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::entry::main", "<group>"));
					choice0 = value_modules_ispa_dep_scanner_mod_detach;
					matched0 = true;
					break;
				}
				case Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id: {
					value_modules_ispa_dep_scanner_mod_import = required<Types::modules::ispa_dep_scanner::mod_import>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerMod_import(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_import::PY::id, Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::entry::main", "<group>"));
					choice0 = value_modules_ispa_dep_scanner_mod_import;
					matched0 = true;
					break;
				}
				case Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id: {
					value_modules_ispa_dep_scanner_mod_module = required<Types::modules::ispa_dep_scanner::mod_module>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerMod_module(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_import::PY::id, Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::entry::main", "<group>"));
					choice0 = value_modules_ispa_dep_scanner_mod_module;
					matched0 = true;
					break;
				}
				case Tokens::modules::scanner_tokens::COMMENT::id: {
					value_modules_scanner_tokens_COMMENT = required<Types::modules::scanner_tokens::COMMENT>(la0_1, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_import::PY::id, Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::entry::main", "<group>"));
					choice0 = value_modules_scanner_tokens_COMMENT;
					matched0 = true;
					break;
				}
			}
			pos = la0_1;
			if (!matched0) {
				switch (recover_choice<::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_import::PY::id, Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::scanner_tokens::COMMENT::id>>(pos, "modules::entry::main", "modules::ispa_dep_scanner::mod_module::MODULE, modules::ispa_dep_scanner::mod_import::IMPORT, modules::ispa_dep_scanner::mod_detach::DETACH, FEATURE, NUMBER_SIGN, modules::common::ID, modules::ispa_dep_scanner::fail_decl::FAIL, modules::scanner_tokens::COMMENT")) {
					case 0: {
						value_modules_ispa_dep_scanner_mod_module = required<Types::modules::ispa_dep_scanner::mod_module>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerMod_module(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_import::PY::id, Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::entry::main", "<group>"));
						choice0 = value_modules_ispa_dep_scanner_mod_module;
						matched0 = true;
						break;
					}
					case 1: {
						value_modules_ispa_dep_scanner_mod_import = required<Types::modules::ispa_dep_scanner::mod_import>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerMod_import(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_import::PY::id, Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::entry::main", "<group>"));
						choice0 = value_modules_ispa_dep_scanner_mod_import;
						matched0 = true;
						break;
					}
					case 2: {
						value_modules_ispa_dep_scanner_mod_detach = required<Types::modules::ispa_dep_scanner::mod_detach>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerMod_detach(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_import::PY::id, Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::entry::main", "<group>"));
						choice0 = value_modules_ispa_dep_scanner_mod_detach;
						matched0 = true;
						break;
					}
					case 3: {
						value_modules_ispa_dep_scanner_feature_decl = required<Types::modules::ispa_dep_scanner::feature_decl>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerFeature_decl(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_import::PY::id, Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::entry::main", "<group>"));
						choice0 = value_modules_ispa_dep_scanner_feature_decl;
						matched0 = true;
						break;
					}
					case 4: {
						value_modules_ispa_dep_scanner_declaration = required<Types::modules::ispa_dep_scanner::declaration>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerDeclaration(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_import::PY::id, Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::entry::main", "<group>"));
						choice0 = value_modules_ispa_dep_scanner_declaration;
						matched0 = true;
						break;
					}
					case 5: {
						value_modules_ispa_dep_scanner_fail_decl = required<Types::modules::ispa_dep_scanner::fail_decl>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerFail_decl(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_import::PY::id, Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::entry::main", "<group>"));
						choice0 = value_modules_ispa_dep_scanner_fail_decl;
						matched0 = true;
						break;
					}
					case 6: {
						value_modules_scanner_tokens_COMMENT = required<Types::modules::scanner_tokens::COMMENT>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_import::PY::id, Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::entry::main", "<group>"));
						choice0 = value_modules_scanner_tokens_COMMENT;
						matched0 = true;
						break;
					}
				}
			}
			if (!matched0) {
				return {};
			}
			group_list1.push_back(choice0);
			skip_spaces<Types::__WS>(pos);
		}
		::ISPA_CORE::assign_compatible(result.value, group_list1);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		::ISPA_CORE::MatchResult<Types::modules::entry::main, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesEntryMain<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::entry::main, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesEntryMain<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::entry::main, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesIspa_dep_scannerFeature_decl(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::feature_decl, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.ispa_dep_scanner.feature_decl", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 48, 1}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.ispa_dep_scanner.feature_decl", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 48, 1}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 48, 1}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::ispa_dep_scanner::feature_decl result = {};
		::ISPA_CORE::Node<Types::FEATURE> value_FEATURE;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::source_name> value_modules_ispa_dep_scanner_source_name;
		::ISPA_CORE::Node<Types::LEFT_CURLY_BRACKET> value_LEFT_CURLY_BRACKET;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::symbol_list> value_modules_ispa_dep_scanner_symbol_list;
		::ISPA_CORE::Node<Types::RIGHT_CURLY_BRACKET> value_RIGHT_CURLY_BRACKET;
		::ISPA_CORE::Node<Types::SEMICOLON> value_SEMICOLON;
		auto recovery_scope = recovery_frame("modules::ispa_dep_scanner::feature_decl");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("FEATURE", "FEATURE", "__WS", "modules::ispa_dep_scanner::source_name", "__WS", "LEFT_CURLY_BRACKET", "__WS", "modules::ispa_dep_scanner::symbol_list", "__WS", "RIGHT_CURLY_BRACKET", "__WS", "SEMICOLON");
		value_FEATURE = required<Types::FEATURE>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::ispa_dep_scanner::feature_decl", "FEATURE"));
		::ISPA_CORE::assign_compatible(result.begin, value_FEATURE);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("__WS", "__WS", "modules::ispa_dep_scanner::source_name", "__WS", "LEFT_CURLY_BRACKET", "__WS", "modules::ispa_dep_scanner::symbol_list", "__WS", "RIGHT_CURLY_BRACKET", "__WS", "SEMICOLON");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("modules::ispa_dep_scanner::source_name", "modules::ispa_dep_scanner::source_name", "__WS", "LEFT_CURLY_BRACKET", "__WS", "modules::ispa_dep_scanner::symbol_list", "__WS", "RIGHT_CURLY_BRACKET", "__WS", "SEMICOLON");
		value_modules_ispa_dep_scanner_source_name = required<Types::modules::ispa_dep_scanner::source_name>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerSource_name(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::feature_decl", "modules::ispa_dep_scanner::source_name"));
		::ISPA_CORE::assign_compatible(result.name, value_modules_ispa_dep_scanner_source_name);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("__WS", "__WS", "LEFT_CURLY_BRACKET", "__WS", "modules::ispa_dep_scanner::symbol_list", "__WS", "RIGHT_CURLY_BRACKET", "__WS", "SEMICOLON");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("LEFT_CURLY_BRACKET", "LEFT_CURLY_BRACKET", "__WS", "modules::ispa_dep_scanner::symbol_list", "__WS", "RIGHT_CURLY_BRACKET", "__WS", "SEMICOLON");
		value_LEFT_CURLY_BRACKET = required<Types::LEFT_CURLY_BRACKET>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::ispa_dep_scanner::feature_decl", "LEFT_CURLY_BRACKET"));
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("__WS", "__WS", "modules::ispa_dep_scanner::symbol_list", "__WS", "RIGHT_CURLY_BRACKET", "__WS", "SEMICOLON");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("modules::ispa_dep_scanner::symbol_list", "modules::ispa_dep_scanner::symbol_list", "__WS", "RIGHT_CURLY_BRACKET", "__WS", "SEMICOLON");
		value_modules_ispa_dep_scanner_symbol_list = optional<Types::modules::ispa_dep_scanner::symbol_list>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerSymbol_list(cursor); });
		::ISPA_CORE::assign_compatible(result.symbols, value_modules_ispa_dep_scanner_symbol_list);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("__WS", "__WS", "RIGHT_CURLY_BRACKET", "__WS", "SEMICOLON");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("RIGHT_CURLY_BRACKET", "RIGHT_CURLY_BRACKET", "__WS", "SEMICOLON");
		value_RIGHT_CURLY_BRACKET = required<Types::RIGHT_CURLY_BRACKET>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id, Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::ispa_dep_scanner::feature_decl", "RIGHT_CURLY_BRACKET"));
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("__WS", "__WS", "SEMICOLON");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("SEMICOLON", "SEMICOLON");
		value_SEMICOLON = required<Types::SEMICOLON>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::ispa_dep_scanner::feature_decl", "SEMICOLON"));
		::ISPA_CORE::assign_compatible(result.end, value_SEMICOLON);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::feature_decl, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesIspa_dep_scannerFeature_decl<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::feature_decl, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesIspa_dep_scannerFeature_decl<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::feature_decl, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesCommonTemplate_params(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::common::template_params, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.common.template_params", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa/grammar/common.isc", 26, 1}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.common.template_params", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa/grammar/common.isc", 26, 1}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa/grammar/common.isc", 26, 1}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::common::template_params result = {};
		::ISPA_CORE::Node<Types::LESS_THAN_SIGN> value_LESS_THAN_SIGN;
		::ISPA_CORE::Node<Types::modules::common::template_param> value_modules_common_template_param;
		::ISPA_CORE::Node<Types::GREATER_THAN_SIGN> value_GREATER_THAN_SIGN;
		auto recovery_scope = recovery_frame("modules::common::template_params");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::LESS_THAN_SIGN::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::GREATER_THAN_SIGN::id>>("LESS_THAN_SIGN", "LESS_THAN_SIGN", "__WS", "modules::common::template_param", "__WS", "GREATER_THAN_SIGN");
		value_LESS_THAN_SIGN = required<Types::LESS_THAN_SIGN>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::LESS_THAN_SIGN::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::GREATER_THAN_SIGN::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::common::template_params", "LESS_THAN_SIGN"));
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::GREATER_THAN_SIGN::id>>("__WS", "__WS", "modules::common::template_param", "__WS", "GREATER_THAN_SIGN");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::GREATER_THAN_SIGN::id>>("modules::common::template_param", "modules::common::template_param", "__WS", "GREATER_THAN_SIGN");
		value_modules_common_template_param = optional<Types::modules::common::template_param>(pos, [this](auto cursor) { return this->modulesCommonTemplate_param(cursor); });
		::ISPA_CORE::assign_compatible(result.value, value_modules_common_template_param);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::GREATER_THAN_SIGN::id>>("__WS", "__WS", "GREATER_THAN_SIGN");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::GREATER_THAN_SIGN::id>>("GREATER_THAN_SIGN", "GREATER_THAN_SIGN");
		value_GREATER_THAN_SIGN = required<Types::GREATER_THAN_SIGN>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::GREATER_THAN_SIGN::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::GREATER_THAN_SIGN::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::common::template_params", "GREATER_THAN_SIGN"));
		::ISPA_CORE::MatchResult<Types::modules::common::template_params, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesCommonTemplate_params<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::common::template_params, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesCommonTemplate_params<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::common::template_params, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesIspa_dep_scannerMod_importImport_valueFrom_importSelectionItems(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_import::import_value::from_import::selection::items, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.ispa_dep_scanner.mod_import.import_value.from_import.selection.items", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 28, 18}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.ispa_dep_scanner.mod_import.import_value.from_import.selection.items", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 28, 18}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 28, 18}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::ispa_dep_scanner::mod_import::import_value::from_import::selection::items result = {};
		::ISPA_CORE::Node<Types::LEFT_CURLY_BRACKET> value_LEFT_CURLY_BRACKET;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::source_name> value_modules_ispa_dep_scanner_source_name;
		::ISPA_CORE::Node<Types::COMMA> value_COMMA;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::source_name> value_modules_ispa_dep_scanner_source_name_1;
		std::pair<::ISPA_CORE::Node<Types::COMMA>, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::source_name>> group0;
		std::vector<std::pair<::ISPA_CORE::Node<Types::COMMA>, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::source_name>>> group_list1;
		std::vector<::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::source_name>> capture_list2;
		::ISPA_CORE::Node<Types::RIGHT_CURLY_BRACKET> value_RIGHT_CURLY_BRACKET;
		auto recovery_scope = recovery_frame("modules::ispa_dep_scanner::mod_import::import_value::from_import::selection::items");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>>("LEFT_CURLY_BRACKET", "LEFT_CURLY_BRACKET", "__WS", "modules::ispa_dep_scanner::source_name", "__WS", "<group>", "__WS", "RIGHT_CURLY_BRACKET");
		value_LEFT_CURLY_BRACKET = required<Types::LEFT_CURLY_BRACKET>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::ispa_dep_scanner::mod_import::import_value::from_import::selection::items", "LEFT_CURLY_BRACKET"));
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>>("__WS", "__WS", "modules::ispa_dep_scanner::source_name", "__WS", "<group>", "__WS", "RIGHT_CURLY_BRACKET");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>>("modules::ispa_dep_scanner::source_name", "modules::ispa_dep_scanner::source_name", "__WS", "<group>", "__WS", "RIGHT_CURLY_BRACKET");
		value_modules_ispa_dep_scanner_source_name = required<Types::modules::ispa_dep_scanner::source_name>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerSource_name(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::RIGHT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::mod_import::import_value::from_import::selection::items", "modules::ispa_dep_scanner::source_name"));
		::ISPA_CORE::assign_compatible(result.first, value_modules_ispa_dep_scanner_source_name);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>>("__WS", "__WS", "<group>", "__WS", "RIGHT_CURLY_BRACKET");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>>("<group>", "<group>", "__WS", "RIGHT_CURLY_BRACKET");
		// grammar ` ( COMMA  __WS?; @  modules::ispa_dep_scanner::source_name)*;`: repeat while lookahead is in FIRST
		while ((::ISPA_CORE::lookahead_kind(pos) == Tokens::COMMA::id)) {
			value_COMMA = required<Types::COMMA>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::mod_import::import_value::from_import::selection::items", "<group>"));
			skip_spaces<Types::__WS>(pos);
			value_modules_ispa_dep_scanner_source_name_1 = required<Types::modules::ispa_dep_scanner::source_name>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerSource_name(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::mod_import::import_value::from_import::selection::items", "<group>"));
			group0 = std::make_pair(value_COMMA, value_modules_ispa_dep_scanner_source_name_1);
			group_list1.push_back(group0);
			capture_list2.push_back(value_modules_ispa_dep_scanner_source_name_1);
			skip_spaces<Types::__WS>(pos);
		}
		::ISPA_CORE::assign_compatible(result.sequence, capture_list2);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>>("__WS", "__WS", "RIGHT_CURLY_BRACKET");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>>("RIGHT_CURLY_BRACKET", "RIGHT_CURLY_BRACKET");
		value_RIGHT_CURLY_BRACKET = required<Types::RIGHT_CURLY_BRACKET>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::RIGHT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::ispa_dep_scanner::mod_import::import_value::from_import::selection::items", "RIGHT_CURLY_BRACKET"));
		::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_import::import_value::from_import::selection::items, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesIspa_dep_scannerMod_importImport_valueFrom_importSelectionItems<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_import::import_value::from_import::selection::items, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesIspa_dep_scannerMod_importImport_valueFrom_importSelectionItems<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_import::import_value::from_import::selection::items, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesCommonTemplate_param(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::common::template_param, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.common.template_param", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa/grammar/common.isc", 42, 1}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.common.template_param", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa/grammar/common.isc", 42, 1}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa/grammar/common.isc", 42, 1}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::common::template_param result = {};
		::ISPA_CORE::Node<Types::modules::common::ID> value_modules_common_ID;
		::ISPA_CORE::Node<Types::COMMA> value_COMMA;
		::ISPA_CORE::Node<Types::modules::common::ID> value_modules_common_ID_1;
		std::pair<::ISPA_CORE::Node<Types::COMMA>, ::ISPA_CORE::Node<Types::modules::common::ID>> group0;
		std::vector<std::pair<::ISPA_CORE::Node<Types::COMMA>, ::ISPA_CORE::Node<Types::modules::common::ID>>> group_list1;
		std::vector<::ISPA_CORE::Node<Types::modules::common::ID>> capture_list2;
		auto recovery_scope = recovery_frame("modules::common::template_param");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::modules::common::ID::id>>("modules::common::ID", "modules::common::ID", "__WS", "<group>");
		value_modules_common_ID = required<Types::modules::common::ID>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::GREATER_THAN_SIGN::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::GREATER_THAN_SIGN::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::common::template_param", "modules::common::ID"));
		::ISPA_CORE::assign_compatible(result.first_p, value_modules_common_ID);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::modules::common::ID::id>>("__WS", "__WS", "<group>");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::modules::common::ID::id>>("<group>", "<group>");
		// grammar ` ( COMMA  __WS?; @  modules::common::ID)*;`: repeat while lookahead is in FIRST
		while ((::ISPA_CORE::lookahead_kind(pos) == Tokens::COMMA::id)) {
			value_COMMA = required<Types::COMMA>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::GREATER_THAN_SIGN::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::GREATER_THAN_SIGN::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::common::template_param", "<group>"));
			skip_spaces<Types::__WS>(pos);
			value_modules_common_ID_1 = required<Types::modules::common::ID>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::GREATER_THAN_SIGN::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::GREATER_THAN_SIGN::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::common::template_param", "<group>"));
			group0 = std::make_pair(value_COMMA, value_modules_common_ID_1);
			group_list1.push_back(group0);
			capture_list2.push_back(value_modules_common_ID_1);
			skip_spaces<Types::__WS>(pos);
		}
		::ISPA_CORE::assign_compatible(result.following_p, capture_list2);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		::ISPA_CORE::MatchResult<Types::modules::common::template_param, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesCommonTemplate_param<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::common::template_param, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesCommonTemplate_param<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::common::template_param, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesIspa_dep_scannerMod_detach(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_detach, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.ispa_dep_scanner.mod_detach", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 36, 1}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.ispa_dep_scanner.mod_detach", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 36, 1}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 36, 1}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::ispa_dep_scanner::mod_detach result = {};
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_detach::DETACH> value_modules_ispa_dep_scanner_mod_detach_DETACH;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_detach::detach_value> value_modules_ispa_dep_scanner_mod_detach_detach_value;
		::ISPA_CORE::Node<Types::SEMICOLON> value_SEMICOLON;
		auto recovery_scope = recovery_frame("modules::ispa_dep_scanner::mod_detach");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("modules::ispa_dep_scanner::mod_detach::DETACH", "modules::ispa_dep_scanner::mod_detach::DETACH", "__WS", "modules::ispa_dep_scanner::mod_detach::detach_value", "__WS", "SEMICOLON");
		value_modules_ispa_dep_scanner_mod_detach_DETACH = required<Types::modules::ispa_dep_scanner::mod_detach::DETACH>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::ispa_dep_scanner::mod_detach", "modules::ispa_dep_scanner::mod_detach::DETACH"));
		::ISPA_CORE::assign_compatible(result.begin, value_modules_ispa_dep_scanner_mod_detach_DETACH);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("__WS", "__WS", "modules::ispa_dep_scanner::mod_detach::detach_value", "__WS", "SEMICOLON");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("modules::ispa_dep_scanner::mod_detach::detach_value", "modules::ispa_dep_scanner::mod_detach::detach_value", "__WS", "SEMICOLON");
		value_modules_ispa_dep_scanner_mod_detach_detach_value = required<Types::modules::ispa_dep_scanner::mod_detach::detach_value>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerMod_detachDetach_value(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::mod_detach", "modules::ispa_dep_scanner::mod_detach::detach_value"));
		::ISPA_CORE::assign_compatible(result.value, value_modules_ispa_dep_scanner_mod_detach_detach_value);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("__WS", "__WS", "SEMICOLON");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("SEMICOLON", "SEMICOLON");
		value_SEMICOLON = required<Types::SEMICOLON>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::ispa_dep_scanner::mod_detach", "SEMICOLON"));
		::ISPA_CORE::assign_compatible(result.end, value_SEMICOLON);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_detach, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesIspa_dep_scannerMod_detach<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_detach, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesIspa_dep_scannerMod_detach<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_detach, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesIspa_dep_scannerMod_module(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_module, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.ispa_dep_scanner.mod_module", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 4, 1}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.ispa_dep_scanner.mod_module", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 4, 1}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 4, 1}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::ispa_dep_scanner::mod_module result = {};
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_module::MODULE> value_modules_ispa_dep_scanner_mod_module_MODULE;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::module_path> value_modules_ispa_dep_scanner_module_path;
		::ISPA_CORE::Node<Types::modules::common::template_params> value_modules_common_template_params;
		::ISPA_CORE::Node<Types::AUTO_1> value_AUTO_1;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::source_name> value_modules_ispa_dep_scanner_source_name;
		std::pair<::ISPA_CORE::Node<Types::AUTO_1>, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::source_name>> group0;
		::ISPA_CORE::Node<Types::SEMICOLON> value_SEMICOLON;
		auto recovery_scope = recovery_frame("modules::ispa_dep_scanner::mod_module");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("modules::ispa_dep_scanner::mod_module::MODULE", "modules::ispa_dep_scanner::mod_module::MODULE", "__WS", "modules::ispa_dep_scanner::module_path", "__WS", "modules::common::template_params", "__WS", "<group>", "__WS", "SEMICOLON");
		value_modules_ispa_dep_scanner_mod_module_MODULE = required<Types::modules::ispa_dep_scanner::mod_module::MODULE>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::ispa_dep_scanner::mod_module", "modules::ispa_dep_scanner::mod_module::MODULE"));
		::ISPA_CORE::assign_compatible(result.begin, value_modules_ispa_dep_scanner_mod_module_MODULE);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("__WS", "__WS", "modules::ispa_dep_scanner::module_path", "__WS", "modules::common::template_params", "__WS", "<group>", "__WS", "SEMICOLON");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("modules::ispa_dep_scanner::module_path", "modules::ispa_dep_scanner::module_path", "__WS", "modules::common::template_params", "__WS", "<group>", "__WS", "SEMICOLON");
		value_modules_ispa_dep_scanner_module_path = required<Types::modules::ispa_dep_scanner::module_path>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerModule_path(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO_1::id, Tokens::LESS_THAN_SIGN::id, Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::mod_module", "modules::ispa_dep_scanner::module_path"));
		::ISPA_CORE::assign_compatible(result.name, value_modules_ispa_dep_scanner_module_path);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("__WS", "__WS", "modules::common::template_params", "__WS", "<group>", "__WS", "SEMICOLON");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("modules::common::template_params", "modules::common::template_params", "__WS", "<group>", "__WS", "SEMICOLON");
		value_modules_common_template_params = optional<Types::modules::common::template_params>(pos, [this](auto cursor) { return this->modulesCommonTemplate_params(cursor); });
		::ISPA_CORE::assign_compatible(result.parameters, value_modules_common_template_params);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("__WS", "__WS", "<group>", "__WS", "SEMICOLON");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("<group>", "<group>", "__WS", "SEMICOLON");
		// grammar ` ( AUTO_1  __WS?; @  modules::ispa_dep_scanner::source_name)?;`: optional; entered only when lookahead is in FIRST
		if ((::ISPA_CORE::lookahead_kind(pos) == Tokens::AUTO_1::id)) {
			value_AUTO_1 = required<Types::AUTO_1>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::AUTO_1::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::mod_module", "<group>"));
			skip_spaces<Types::__WS>(pos);
			value_modules_ispa_dep_scanner_source_name = required<Types::modules::ispa_dep_scanner::source_name>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerSource_name(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::AUTO_1::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::mod_module", "<group>"));
			group0 = std::make_pair(value_AUTO_1, value_modules_ispa_dep_scanner_source_name);
		}
		::ISPA_CORE::assign_compatible(result.entry, value_modules_ispa_dep_scanner_source_name);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("__WS", "__WS", "SEMICOLON");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>>("SEMICOLON", "SEMICOLON");
		value_SEMICOLON = required<Types::SEMICOLON>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::ispa_dep_scanner::mod_module", "SEMICOLON"));
		::ISPA_CORE::assign_compatible(result.end, value_SEMICOLON);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_module, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesIspa_dep_scannerMod_module<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_module, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesIspa_dep_scannerMod_module<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_module, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesCommonParameter(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::common::parameter, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.common.parameter", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa/grammar/common.isc", 34, 1}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.common.parameter", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa/grammar/common.isc", 34, 1}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa/grammar/common.isc", 34, 1}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::common::parameter result = {};
		::ISPA_CORE::Node<Types::modules::common::ID> value_modules_common_ID;
		::ISPA_CORE::Node<Types::COLON> value_COLON;
		::ISPA_CORE::Node<Types::modules::common::type_annotation> value_modules_common_type_annotation;
		std::pair<::ISPA_CORE::Node<Types::COLON>, ::ISPA_CORE::Node<Types::modules::common::type_annotation>> group0;
		auto recovery_scope = recovery_frame("modules::common::parameter");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id, Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>>("modules::common::ID", "modules::common::ID", "__WS", "<group>");
		value_modules_common_ID = required<Types::modules::common::ID>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id, Tokens::COMMA::id, Tokens::RIGHT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::RIGHT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id, Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<true, true, true, 32>>("modules::common::parameter", "modules::common::ID"));
		::ISPA_CORE::assign_compatible(result.name, value_modules_common_ID);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id, Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>>("__WS", "__WS", "<group>");
		skip_spaces<Types::__WS>(pos);
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id, Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>>("<group>", "<group>");
		// grammar ` ( COLON  __WS?; @  modules::common::type_annotation)?;`: optional; entered only when lookahead is in FIRST
		if ((::ISPA_CORE::lookahead_kind(pos) == Tokens::COLON::id)) {
			value_COLON = required<Types::COLON>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::RIGHT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::RIGHT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id, Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::common::parameter", "<group>"));
			skip_spaces<Types::__WS>(pos);
			value_modules_common_type_annotation = required<Types::modules::common::type_annotation>(pos, [this](auto cursor) { return this->modulesCommonType_annotation(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::RIGHT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COMMA::id, Tokens::RIGHT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::COLON::id, Tokens::FULL_STOP::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::common::parameter", "<group>"));
			group0 = std::make_pair(value_COLON, value_modules_common_type_annotation);
		}
		::ISPA_CORE::assign_compatible(result.annotation, value_modules_common_type_annotation);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		::ISPA_CORE::MatchResult<Types::modules::common::parameter, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesCommonParameter<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::common::parameter, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesCommonParameter<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::common::parameter, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::main(IT pos) -> ::ISPA_CORE::MatchResult<Types::main, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"main", {"", 1, 1}};
		::ispa::debug::ProductionScope ispa_debug_production{"main", {"", 1, 1}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"", 1, 1}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::main result = {};
		::ISPA_CORE::Node<Types::modules::entry::main> value_modules_entry_main;
		auto recovery_scope = recovery_frame("main");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_import::PY::id, Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>>("modules::entry::main", "modules::entry::main");
		value_modules_entry_main = required<Types::modules::entry::main>(pos, [this](auto cursor) { return this->modulesEntryMain(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::FEATURE::id, Tokens::NUMBER_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::scanner_tokens::COMMENT::id>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FEATURE::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id, Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id, Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id, Tokens::modules::ispa_dep_scanner::mod_import::PY::id, Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id, Tokens::modules::ispa_dep_scanner::mod_import::import_value::module_import::AS::id, Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("main", "modules::entry::main"));
		::ISPA_CORE::assign_compatible(result.value, value_modules_entry_main);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		::ISPA_CORE::MatchResult<Types::main, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::main<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::main, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::main<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::main, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesIspa_dep_scannerMod_importImport_valueFrom_importSelection(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_import::import_value::from_import::selection, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.ispa_dep_scanner.mod_import.import_value.from_import.selection", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 25, 14}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.ispa_dep_scanner.mod_import.import_value.from_import.selection", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 25, 14}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 25, 14}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::ispa_dep_scanner::mod_import::import_value::from_import::selection result = {};
		std::variant<std::monostate, ::ISPA_CORE::Node<Types::ASTERISK>, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_import::import_value::from_import::selection::items>> choice0;
		bool matched0;
		::ISPA_CORE::Node<Types::ASTERISK> value_ASTERISK;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::mod_import::import_value::from_import::selection::items> value_modules_ispa_dep_scanner_mod_import_import_value_from_import_selection_items;
		auto recovery_scope = recovery_frame("modules::ispa_dep_scanner::mod_import::import_value::from_import::selection");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>>("<group>", "<group>");
		// grammar `@  ( ASTERISK |  modules::ispa_dep_scanner::mod_import::import_value::from_import::selection::items)`: choice; alternative selected by LL(1) lookahead
		matched0 = false;
		auto la0_1 = pos;
		switch (::ISPA_CORE::lookahead_kind(la0_1)) {
			case Tokens::ASTERISK::id: {
				value_ASTERISK = required<Types::ASTERISK>(la0_1, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::mod_import::import_value::from_import::selection", "<group>"));
				choice0 = value_ASTERISK;
				matched0 = true;
				break;
			}
			case Tokens::LEFT_CURLY_BRACKET::id: {
				value_modules_ispa_dep_scanner_mod_import_import_value_from_import_selection_items = required<Types::modules::ispa_dep_scanner::mod_import::import_value::from_import::selection::items>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerMod_importImport_valueFrom_importSelectionItems(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::mod_import::import_value::from_import::selection", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_mod_import_import_value_from_import_selection_items;
				matched0 = true;
				break;
			}
		}
		pos = la0_1;
		if (!matched0) {
			switch (recover_choice<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>>(pos, "modules::ispa_dep_scanner::mod_import::import_value::from_import::selection", "ASTERISK, LEFT_CURLY_BRACKET")) {
				case 0: {
					value_ASTERISK = required<Types::ASTERISK>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::mod_import::import_value::from_import::selection", "<group>"));
					choice0 = value_ASTERISK;
					matched0 = true;
					break;
				}
				case 1: {
					value_modules_ispa_dep_scanner_mod_import_import_value_from_import_selection_items = required<Types::modules::ispa_dep_scanner::mod_import::import_value::from_import::selection::items>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerMod_importImport_valueFrom_importSelectionItems(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::modules::ispa_dep_scanner::mod_import::import_value::from_import::FROM::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::modules::common::ID::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::mod_import::import_value::from_import::selection", "<group>"));
					choice0 = value_modules_ispa_dep_scanner_mod_import_import_value_from_import_selection_items;
					matched0 = true;
					break;
				}
			}
		}
		if (!matched0) {
			return {};
		}
		::ISPA_CORE::assign_compatible(result.value, choice0);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_import::import_value::from_import::selection, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesIspa_dep_scannerMod_importImport_valueFrom_importSelection<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_import::import_value::from_import::selection, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesIspa_dep_scannerMod_importImport_valueFrom_importSelection<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::mod_import::import_value::from_import::selection, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::modulesIspa_dep_scannerBalanced_item(IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::balanced_item, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"modules.ispa_dep_scanner.balanced_item", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 95, 1}};
		::ispa::debug::ProductionScope ispa_debug_production{"modules.ispa_dep_scanner.balanced_item", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 95, 1}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/parser/ispa_dep_scanner/grammar/grammar.isc", 95, 1}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::modules::ispa_dep_scanner::balanced_item result = {};
		std::variant<std::monostate, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::parentheses>, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::braces>, ::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::atom>, ::ISPA_CORE::Node<Types::SEMICOLON>, ::ISPA_CORE::Node<Types::NUMBER_SIGN>> choice0;
		bool matched0;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::parentheses> value_modules_ispa_dep_scanner_parentheses;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::braces> value_modules_ispa_dep_scanner_braces;
		::ISPA_CORE::Node<Types::modules::ispa_dep_scanner::atom> value_modules_ispa_dep_scanner_atom;
		::ISPA_CORE::Node<Types::SEMICOLON> value_SEMICOLON;
		::ISPA_CORE::Node<Types::NUMBER_SIGN> value_NUMBER_SIGN;
		auto recovery_scope = recovery_frame("modules::ispa_dep_scanner::balanced_item");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>>("<group>", "<group>");
		// grammar ` ( modules::ispa_dep_scanner::parentheses |  modules::ispa_dep_scanner::braces |  modules::ispa_dep_scanner::atom |  SEMICOLON |  NUMBER_SIGN)`: choice; alternative selected by LL(1) lookahead
		matched0 = false;
		auto la0_1 = pos;
		switch (::ISPA_CORE::lookahead_kind(la0_1)) {
			case Tokens::ASTERISK::id: {
				value_modules_ispa_dep_scanner_atom = required<Types::modules::ispa_dep_scanner::atom>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerAtom(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::balanced_item", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_atom;
				matched0 = true;
				break;
			}
			case Tokens::AUTO::id: {
				value_modules_ispa_dep_scanner_parentheses = required<Types::modules::ispa_dep_scanner::parentheses>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerParentheses(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::balanced_item", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_parentheses;
				matched0 = true;
				break;
			}
			case Tokens::AUTO_1::id: {
				value_modules_ispa_dep_scanner_atom = required<Types::modules::ispa_dep_scanner::atom>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerAtom(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::balanced_item", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_atom;
				matched0 = true;
				break;
			}
			case Tokens::COLON::id: {
				value_modules_ispa_dep_scanner_atom = required<Types::modules::ispa_dep_scanner::atom>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerAtom(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::balanced_item", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_atom;
				matched0 = true;
				break;
			}
			case Tokens::COMMA::id: {
				value_modules_ispa_dep_scanner_atom = required<Types::modules::ispa_dep_scanner::atom>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerAtom(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::balanced_item", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_atom;
				matched0 = true;
				break;
			}
			case Tokens::FULL_STOP::id: {
				value_modules_ispa_dep_scanner_atom = required<Types::modules::ispa_dep_scanner::atom>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerAtom(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::balanced_item", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_atom;
				matched0 = true;
				break;
			}
			case Tokens::GREATER_THAN_SIGN::id: {
				value_modules_ispa_dep_scanner_atom = required<Types::modules::ispa_dep_scanner::atom>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerAtom(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::balanced_item", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_atom;
				matched0 = true;
				break;
			}
			case Tokens::LEFT_CURLY_BRACKET::id: {
				value_modules_ispa_dep_scanner_braces = required<Types::modules::ispa_dep_scanner::braces>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerBraces(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::balanced_item", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_braces;
				matched0 = true;
				break;
			}
			case Tokens::LESS_THAN_SIGN::id: {
				value_modules_ispa_dep_scanner_atom = required<Types::modules::ispa_dep_scanner::atom>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerAtom(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::balanced_item", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_atom;
				matched0 = true;
				break;
			}
			case Tokens::NUMBER_SIGN::id: {
				value_NUMBER_SIGN = required<Types::NUMBER_SIGN>(la0_1, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::balanced_item", "<group>"));
				choice0 = value_NUMBER_SIGN;
				matched0 = true;
				break;
			}
			case Tokens::SEMICOLON::id: {
				value_SEMICOLON = required<Types::SEMICOLON>(la0_1, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::balanced_item", "<group>"));
				choice0 = value_SEMICOLON;
				matched0 = true;
				break;
			}
			case Tokens::modules::common::ID::id: {
				value_modules_ispa_dep_scanner_atom = required<Types::modules::ispa_dep_scanner::atom>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerAtom(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::balanced_item", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_atom;
				matched0 = true;
				break;
			}
			case Tokens::modules::common::NUMBER::id: {
				value_modules_ispa_dep_scanner_atom = required<Types::modules::ispa_dep_scanner::atom>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerAtom(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::balanced_item", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_atom;
				matched0 = true;
				break;
			}
			case Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id: {
				value_modules_ispa_dep_scanner_parentheses = required<Types::modules::ispa_dep_scanner::parentheses>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerParentheses(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::balanced_item", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_parentheses;
				matched0 = true;
				break;
			}
			case Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id: {
				value_modules_ispa_dep_scanner_parentheses = required<Types::modules::ispa_dep_scanner::parentheses>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerParentheses(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::balanced_item", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_parentheses;
				matched0 = true;
				break;
			}
			case Tokens::modules::scanner_tokens::CGROUP::id: {
				value_modules_ispa_dep_scanner_atom = required<Types::modules::ispa_dep_scanner::atom>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerAtom(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::balanced_item", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_atom;
				matched0 = true;
				break;
			}
			case Tokens::modules::scanner_tokens::COMMENT::id: {
				value_modules_ispa_dep_scanner_atom = required<Types::modules::ispa_dep_scanner::atom>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerAtom(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::balanced_item", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_atom;
				matched0 = true;
				break;
			}
			case Tokens::modules::scanner_tokens::OTHER::id: {
				value_modules_ispa_dep_scanner_atom = required<Types::modules::ispa_dep_scanner::atom>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerAtom(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::balanced_item", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_atom;
				matched0 = true;
				break;
			}
			case Tokens::modules::scanner_tokens::STRING::id: {
				value_modules_ispa_dep_scanner_atom = required<Types::modules::ispa_dep_scanner::atom>(la0_1, [this](auto cursor) { return this->modulesIspa_dep_scannerAtom(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::balanced_item", "<group>"));
				choice0 = value_modules_ispa_dep_scanner_atom;
				matched0 = true;
				break;
			}
		}
		pos = la0_1;
		if (!matched0) {
			switch (recover_choice<::ISPA_CORE::Recovery::Tokens<Tokens::AUTO::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::LEFT_CURLY_BRACKET::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LESS_THAN_SIGN::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::SEMICOLON::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::NUMBER_SIGN::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::NUMBER_SIGN::id>>(pos, "modules::ispa_dep_scanner::balanced_item", "AUTO, modules::ispa_dep_scanner::parentheses::AUTO_1, modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS, LEFT_CURLY_BRACKET, ASTERISK, AUTO_1, COLON, COMMA, FULL_STOP, GREATER_THAN_SIGN, LESS_THAN_SIGN, modules::common::ID, modules::common::NUMBER, modules::scanner_tokens::CGROUP, modules::scanner_tokens::COMMENT, modules::scanner_tokens::OTHER, modules::scanner_tokens::STRING, SEMICOLON, NUMBER_SIGN")) {
				case 0: {
					value_modules_ispa_dep_scanner_parentheses = required<Types::modules::ispa_dep_scanner::parentheses>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerParentheses(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::balanced_item", "<group>"));
					choice0 = value_modules_ispa_dep_scanner_parentheses;
					matched0 = true;
					break;
				}
				case 1: {
					value_modules_ispa_dep_scanner_braces = required<Types::modules::ispa_dep_scanner::braces>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerBraces(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::balanced_item", "<group>"));
					choice0 = value_modules_ispa_dep_scanner_braces;
					matched0 = true;
					break;
				}
				case 2: {
					value_modules_ispa_dep_scanner_atom = required<Types::modules::ispa_dep_scanner::atom>(pos, [this](auto cursor) { return this->modulesIspa_dep_scannerAtom(cursor); }, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::balanced_item", "<group>"));
					choice0 = value_modules_ispa_dep_scanner_atom;
					matched0 = true;
					break;
				}
				case 3: {
					value_SEMICOLON = required<Types::SEMICOLON>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::balanced_item", "<group>"));
					choice0 = value_SEMICOLON;
					matched0 = true;
					break;
				}
				case 4: {
					value_NUMBER_SIGN = required<Types::NUMBER_SIGN>(pos, ::ISPA_CORE::Recovery::handler<::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Tokens<Tokens::ASTERISK::id, Tokens::AUTO::id, Tokens::AUTO_1::id, Tokens::COLON::id, Tokens::COMMA::id, Tokens::FULL_STOP::id, Tokens::GREATER_THAN_SIGN::id, Tokens::LEFT_CURLY_BRACKET::id, Tokens::LESS_THAN_SIGN::id, Tokens::NUMBER_SIGN::id, Tokens::RIGHT_CURLY_BRACKET::id, Tokens::RIGHT_PARENTHESIS::id, Tokens::SEMICOLON::id, Tokens::modules::common::ID::id, Tokens::modules::common::NUMBER::id, Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id, Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id, Tokens::modules::scanner_tokens::CGROUP::id, Tokens::modules::scanner_tokens::COMMENT::id, Tokens::modules::scanner_tokens::OTHER::id, Tokens::modules::scanner_tokens::STRING::id>, ::ISPA_CORE::Recovery::Policy<false, true, false, 32>>("modules::ispa_dep_scanner::balanced_item", "<group>"));
					choice0 = value_NUMBER_SIGN;
					matched0 = true;
					break;
				}
			}
		}
		if (!matched0) {
			return {};
		}
		::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::balanced_item, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(std::move(result));
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_CORE::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto DepParser::Parser::modulesIspa_dep_scannerBalanced_item<::DepParser::Lexer::iterator>(::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::balanced_item, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::modulesIspa_dep_scannerBalanced_item<::DepParser::Lexer::lazy_iterator>(::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::ispa_dep_scanner::balanced_item, ::DepParser::Lexer::lazy_iterator>;
template<typename IT, class> auto DepParser::Parser::ispa_match_template(::ISPA_CORE::type_tag<Types::modules::common::rule_params> tag, IT pos) -> ::ISPA_CORE::MatchResult<Types::modules::common::rule_params, IT> {
	return modulesCommonRule_params(pos);
}
template auto DepParser::Parser::ispa_match_template<::DepParser::Lexer::iterator>(::ISPA_CORE::type_tag<Types::modules::common::rule_params> tag, ::DepParser::Lexer::iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::common::rule_params, ::DepParser::Lexer::iterator>;
template auto DepParser::Parser::ispa_match_template<::DepParser::Lexer::lazy_iterator>(::ISPA_CORE::type_tag<Types::modules::common::rule_params> tag, ::DepParser::Lexer::lazy_iterator pos) -> ::ISPA_CORE::MatchResult<Types::modules::common::rule_params, ::DepParser::Lexer::lazy_iterator>;
auto DepParser::Parser::parseFromTokens() -> void {
	tree = main(::DepParser::Lexer::iterator(lexer)).node.data();
}
auto DepParser::Parser::lazyParse() -> ::ISPA_CORE::MatchResult<Types::main, ::DepParser::Lexer::lazy_iterator> {
	lazy_lexer_.clearTokens();
	return main(::DepParser::Lexer::lazy_iterator(lazy_lexer_, text));
}

#include "DepParser.h"

bool DepParser::Exports::symbol_0::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::modules::common::ID::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_0::node_type> DepParser::Exports::symbol_0::parse(const char* origin, const char* start) {
symbol_0 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it);
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_1::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::modules::common::NUMBER::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_1::node_type> DepParser::Exports::symbol_1::parse(const char* origin, const char* start) {
symbol_1 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it);
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_2::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::modules::common::ID::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_2::node_type> DepParser::Exports::symbol_2::parse(const char* origin, const char* start) {
symbol_2 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesCommonParam(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_3::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::modules::common::ID::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_3::node_type> DepParser::Exports::symbol_3::parse(const char* origin, const char* start) {
symbol_3 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesCommonParameter(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_4::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::AUTO::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_4::node_type> DepParser::Exports::symbol_4::parse(const char* origin, const char* start) {
symbol_4 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesCommonRule_params(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_5::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::modules::common::ID::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_5::node_type> DepParser::Exports::symbol_5::parse(const char* origin, const char* start) {
symbol_5 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesCommonTemplate_param(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_6::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::LESS_THAN_SIGN::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_6::node_type> DepParser::Exports::symbol_6::parse(const char* origin, const char* start) {
symbol_6 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesCommonTemplate_params(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_7::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::modules::common::ID::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_7::node_type> DepParser::Exports::symbol_7::parse(const char* origin, const char* start) {
symbol_7 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesCommonType_annotation(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_8::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return true;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::FEATURE::id || token.kind() == Tokens::NUMBER_SIGN::id || token.kind() == Tokens::modules::common::ID::id || token.kind() == Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id || token.kind() == Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id || token.kind() == Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id || token.kind() == Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id || token.kind() == Tokens::modules::scanner_tokens::COMMENT::id)) || true;
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_8::node_type> DepParser::Exports::symbol_8::parse(const char* origin, const char* start) {
symbol_8 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesEntryMain(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_9::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::ASTERISK::id || token.kind() == Tokens::AUTO_1::id || token.kind() == Tokens::COLON::id || token.kind() == Tokens::COMMA::id || token.kind() == Tokens::FULL_STOP::id || token.kind() == Tokens::GREATER_THAN_SIGN::id || token.kind() == Tokens::LESS_THAN_SIGN::id || token.kind() == Tokens::modules::common::ID::id || token.kind() == Tokens::modules::common::NUMBER::id || token.kind() == Tokens::modules::scanner_tokens::CGROUP::id || token.kind() == Tokens::modules::scanner_tokens::COMMENT::id || token.kind() == Tokens::modules::scanner_tokens::OTHER::id || token.kind() == Tokens::modules::scanner_tokens::STRING::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_9::node_type> DepParser::Exports::symbol_9::parse(const char* origin, const char* start) {
symbol_9 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesIspa_dep_scannerAtom(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_10::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::ASTERISK::id || token.kind() == Tokens::AUTO::id || token.kind() == Tokens::AUTO_1::id || token.kind() == Tokens::COLON::id || token.kind() == Tokens::COMMA::id || token.kind() == Tokens::FULL_STOP::id || token.kind() == Tokens::GREATER_THAN_SIGN::id || token.kind() == Tokens::LEFT_CURLY_BRACKET::id || token.kind() == Tokens::LESS_THAN_SIGN::id || token.kind() == Tokens::NUMBER_SIGN::id || token.kind() == Tokens::SEMICOLON::id || token.kind() == Tokens::modules::common::ID::id || token.kind() == Tokens::modules::common::NUMBER::id || token.kind() == Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id || token.kind() == Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id || token.kind() == Tokens::modules::scanner_tokens::CGROUP::id || token.kind() == Tokens::modules::scanner_tokens::COMMENT::id || token.kind() == Tokens::modules::scanner_tokens::OTHER::id || token.kind() == Tokens::modules::scanner_tokens::STRING::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_10::node_type> DepParser::Exports::symbol_10::parse(const char* origin, const char* start) {
symbol_10 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesIspa_dep_scannerBalanced_item(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_11::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::COLON::id || token.kind() == Tokens::LEFT_CURLY_BRACKET::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_11::node_type> DepParser::Exports::symbol_11::parse(const char* origin, const char* start) {
symbol_11 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesIspa_dep_scannerBody(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_12::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::COLON::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_12::node_type> DepParser::Exports::symbol_12::parse(const char* origin, const char* start) {
symbol_12 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesIspa_dep_scannerBodyRegular(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_13::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::LEFT_CURLY_BRACKET::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_13::node_type> DepParser::Exports::symbol_13::parse(const char* origin, const char* start) {
symbol_13 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesIspa_dep_scannerBraces(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_14::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::NUMBER_SIGN::id || token.kind() == Tokens::modules::common::ID::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_14::node_type> DepParser::Exports::symbol_14::parse(const char* origin, const char* start) {
symbol_14 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesIspa_dep_scannerDeclaration(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_15::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::modules::common::ID::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_15::node_type> DepParser::Exports::symbol_15::parse(const char* origin, const char* start) {
symbol_15 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesIspa_dep_scannerDeclarationForm(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_16::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::modules::ispa_dep_scanner::fail_decl::FAIL::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_16::node_type> DepParser::Exports::symbol_16::parse(const char* origin, const char* start) {
symbol_16 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesIspa_dep_scannerFail_decl(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_17::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::FEATURE::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_17::node_type> DepParser::Exports::symbol_17::parse(const char* origin, const char* start) {
symbol_17 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesIspa_dep_scannerFeature_decl(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_18::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::ASTERISK::id || token.kind() == Tokens::AUTO::id || token.kind() == Tokens::AUTO_1::id || token.kind() == Tokens::COLON::id || token.kind() == Tokens::COMMA::id || token.kind() == Tokens::FULL_STOP::id || token.kind() == Tokens::GREATER_THAN_SIGN::id || token.kind() == Tokens::LEFT_CURLY_BRACKET::id || token.kind() == Tokens::LESS_THAN_SIGN::id || token.kind() == Tokens::NUMBER_SIGN::id || token.kind() == Tokens::modules::common::ID::id || token.kind() == Tokens::modules::common::NUMBER::id || token.kind() == Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id || token.kind() == Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id || token.kind() == Tokens::modules::scanner_tokens::CGROUP::id || token.kind() == Tokens::modules::scanner_tokens::COMMENT::id || token.kind() == Tokens::modules::scanner_tokens::OTHER::id || token.kind() == Tokens::modules::scanner_tokens::STRING::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_18::node_type> DepParser::Exports::symbol_18::parse(const char* origin, const char* start) {
symbol_18 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesIspa_dep_scannerMember(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_19::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::modules::ispa_dep_scanner::mod_detach::DETACH::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_19::node_type> DepParser::Exports::symbol_19::parse(const char* origin, const char* start) {
symbol_19 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesIspa_dep_scannerMod_detach(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_20::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::FEATURE::id || token.kind() == Tokens::modules::common::ID::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_20::node_type> DepParser::Exports::symbol_20::parse(const char* origin, const char* start) {
symbol_20 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesIspa_dep_scannerMod_detachDetach_value(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_21::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::FEATURE::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_21::node_type> DepParser::Exports::symbol_21::parse(const char* origin, const char* start) {
symbol_21 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesIspa_dep_scannerMod_detachDetach_valueFeature_ref(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_22::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::modules::ispa_dep_scanner::mod_import::IMPORT::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_22::node_type> DepParser::Exports::symbol_22::parse(const char* origin, const char* start) {
symbol_22 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesIspa_dep_scannerMod_import(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_23::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::modules::ispa_dep_scanner::mod_import::external_import::EXTERN::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_23::node_type> DepParser::Exports::symbol_23::parse(const char* origin, const char* start) {
symbol_23 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesIspa_dep_scannerMod_importExternal_import(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_24::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::ASTERISK::id || token.kind() == Tokens::LEFT_CURLY_BRACKET::id || token.kind() == Tokens::modules::common::ID::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_24::node_type> DepParser::Exports::symbol_24::parse(const char* origin, const char* start) {
symbol_24 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesIspa_dep_scannerMod_importImport_value(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_25::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::ASTERISK::id || token.kind() == Tokens::LEFT_CURLY_BRACKET::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_25::node_type> DepParser::Exports::symbol_25::parse(const char* origin, const char* start) {
symbol_25 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesIspa_dep_scannerMod_importImport_valueFrom_import(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_26::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::ASTERISK::id || token.kind() == Tokens::LEFT_CURLY_BRACKET::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_26::node_type> DepParser::Exports::symbol_26::parse(const char* origin, const char* start) {
symbol_26 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesIspa_dep_scannerMod_importImport_valueFrom_importSelection(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_27::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::LEFT_CURLY_BRACKET::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_27::node_type> DepParser::Exports::symbol_27::parse(const char* origin, const char* start) {
symbol_27 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesIspa_dep_scannerMod_importImport_valueFrom_importSelectionItems(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_28::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::modules::common::ID::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_28::node_type> DepParser::Exports::symbol_28::parse(const char* origin, const char* start) {
symbol_28 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesIspa_dep_scannerMod_importImport_valueModule_import(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_29::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::modules::ispa_dep_scanner::mod_module::MODULE::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_29::node_type> DepParser::Exports::symbol_29::parse(const char* origin, const char* start) {
symbol_29 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesIspa_dep_scannerMod_module(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_30::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::NUMBER_SIGN::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_30::node_type> DepParser::Exports::symbol_30::parse(const char* origin, const char* start) {
symbol_30 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesIspa_dep_scannerModifier(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_31::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::modules::common::ID::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_31::node_type> DepParser::Exports::symbol_31::parse(const char* origin, const char* start) {
symbol_31 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesIspa_dep_scannerModule_path(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_32::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::NUMBER_SIGN::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_32::node_type> DepParser::Exports::symbol_32::parse(const char* origin, const char* start) {
symbol_32 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesIspa_dep_scannerNested_rule(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_33::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::AUTO::id || token.kind() == Tokens::modules::ispa_dep_scanner::parentheses::AUTO_1::id || token.kind() == Tokens::modules::ispa_dep_scanner::parentheses::LEFT_PARENTHESIS::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_33::node_type> DepParser::Exports::symbol_33::parse(const char* origin, const char* start) {
symbol_33 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesIspa_dep_scannerParentheses(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_34::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::modules::common::ID::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_34::node_type> DepParser::Exports::symbol_34::parse(const char* origin, const char* start) {
symbol_34 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesIspa_dep_scannerSource_name(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_35::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::modules::common::ID::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_35::node_type> DepParser::Exports::symbol_35::parse(const char* origin, const char* start) {
symbol_35 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.modulesIspa_dep_scannerSymbol_list(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_36::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::modules::scanner_tokens::CGROUP::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_36::node_type> DepParser::Exports::symbol_36::parse(const char* origin, const char* start) {
symbol_36 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it);
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_37::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::modules::scanner_tokens::COMMENT::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_37::node_type> DepParser::Exports::symbol_37::parse(const char* origin, const char* start) {
symbol_37 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it);
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_38::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::modules::scanner_tokens::OTHER::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_38::node_type> DepParser::Exports::symbol_38::parse(const char* origin, const char* start) {
symbol_38 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it);
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}

bool DepParser::Exports::symbol_39::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::modules::scanner_tokens::STRING::id));
}
ISPA_CORE::ExternalResult<DepParser::Exports::symbol_39::node_type> DepParser::Exports::symbol_39::parse(const char* origin, const char* start) {
symbol_39 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it);
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_CORE::external_node(origin, start, end, std::move(node)), end};
}
