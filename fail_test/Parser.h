#pragma once
#include "ispastdlib/ispa_debug.hpp"

#include <ispastdlib/to_string.hpp>
#include <ispastdlib/parser_ll.hpp>

namespace Parser {
	namespace Tokens {
		namespace ISPA_TEMPLATE_INSTANCE_0 {
			inline const std::size_t id = 2716153174906776339;
		} // namespace ISPA_TEMPLATE_INSTANCE_0
	} // namespace Tokens
} // namespace Parser
namespace Parser {
	namespace Tokens {
		namespace ISPA_TEMPLATE_INSTANCE_0 {
			inline const bool shared = true;
		} // namespace ISPA_TEMPLATE_INSTANCE_0
	} // namespace Tokens
} // namespace Parser
namespace Parser {
	namespace Tokens {
		namespace QUOTE_1 {
			inline const std::size_t id = 3187386487096426476;
		} // namespace QUOTE_1
	} // namespace Tokens
} // namespace Parser
namespace Parser {
	namespace Tokens {
		namespace QUOTE_1 {
			inline const bool shared = true;
		} // namespace QUOTE_1
	} // namespace Tokens
} // namespace Parser
namespace Parser {
	namespace Tokens {
		namespace QUOTE_2 {
			inline const std::size_t id = 4547711566745889232;
		} // namespace QUOTE_2
	} // namespace Tokens
} // namespace Parser
namespace Parser {
	namespace Tokens {
		namespace QUOTE_2 {
			inline const bool shared = true;
		} // namespace QUOTE_2
	} // namespace Tokens
} // namespace Parser
namespace Parser {
	namespace Tokens {
		namespace STRING {
			inline const std::size_t id = 5789426062968575464;
		} // namespace STRING
	} // namespace Tokens
} // namespace Parser
namespace Parser {
	namespace Tokens {
		namespace STRING {
			inline const bool shared = true;
		} // namespace STRING
	} // namespace Tokens
} // namespace Parser
namespace Parser {
	namespace Tokens {
		namespace __WS {
			inline const std::size_t id = 2288775507628772018;
		} // namespace __WS
	} // namespace Tokens
} // namespace Parser
namespace Parser {
	namespace Tokens {
		namespace __WS {
			inline const bool shared = true;
		} // namespace __WS
	} // namespace Tokens
} // namespace Parser
namespace Parser {
	namespace TokensStr {
		namespace ISPA_TEMPLATE_INSTANCE_0 {
			inline const char* const id = "ISPA_TEMPLATE_INSTANCE_0";
		} // namespace ISPA_TEMPLATE_INSTANCE_0
	} // namespace TokensStr
} // namespace Parser
namespace Parser {
	namespace TokensStr {
		namespace QUOTE_1 {
			inline const char* const id = "QUOTE_1";
		} // namespace QUOTE_1
	} // namespace TokensStr
} // namespace Parser
namespace Parser {
	namespace TokensStr {
		namespace QUOTE_2 {
			inline const char* const id = "QUOTE_2";
		} // namespace QUOTE_2
	} // namespace TokensStr
} // namespace Parser
namespace Parser {
	namespace TokensStr {
		namespace STRING {
			inline const char* const id = "STRING";
		} // namespace STRING
	} // namespace TokensStr
} // namespace Parser
namespace Parser {
	namespace TokensStr {
		namespace __WS {
			inline const char* const id = "__WS";
		} // namespace __WS
	} // namespace TokensStr
} // namespace Parser
namespace Parser {
	namespace Rules {
		namespace main {
			inline const std::size_t id = 0;
		} // namespace main
	} // namespace Rules
} // namespace Parser
namespace Parser {
	namespace RulesStr {
		namespace main {
			inline const char* const id = "main";
		} // namespace main
	} // namespace RulesStr
} // namespace Parser
namespace Parser {
	auto TokensToString(std::size_t id) -> const char*;
} // namespace Parser
namespace Parser {
	auto RulesToString(std::size_t id) -> const char*;
} // namespace Parser
namespace Parser::FlatTypes { struct QUOTE_2; }
namespace Parser::FlatTypes { struct __WS; }
namespace Parser::FlatTypes { struct QUOTE_1; }
namespace Parser::FlatTypes { template<typename quote> struct STRING; }
namespace Parser::FlatTypes { struct main; }
namespace Parser {
	namespace FlatTypes {
		using ISPA_TEMPLATE_INSTANCE_0 = ::Parser::FlatTypes::STRING<::Parser::FlatTypes::QUOTE_1>;
	} // namespace FlatTypes
} // namespace Parser
namespace Parser {
	namespace FlatTypes {
		auto operator<<(std::ostream & os, QUOTE_2 value) -> std::ostream &;
	} // namespace FlatTypes
} // namespace Parser
namespace Parser {
	namespace FlatTypes {
		struct QUOTE_2 {
			auto write_to_output(::ISPA_STD::ASTPrinter<std::ostream>& printer) const -> void;
			auto print(std::ostream& os) const -> void;
			auto to_string() const -> std::string;
			constexpr auto members() {
				return std::tie();
			}
			constexpr auto members() const {
				return std::tie();
			}
			static constexpr auto members_str() {
				return std::array<std::string_view, 0>{{}};
			}
			constexpr auto methods() {
				return std::make_tuple(&QUOTE_2::write_to_output, &QUOTE_2::print);
			}
			constexpr auto methods() const {
				return std::make_tuple(&QUOTE_2::write_to_output, &QUOTE_2::print);
			}
			static constexpr auto methods_str() {
				return std::array<std::string_view, 2>{{"write_to_output", "print"}};
			}
		};
		inline std::string_view ispa_debug_type(const QUOTE_2&) { return "Parser.FlatTypes.QUOTE_2"; }
		template<class F> void ispa_debug_fields(const QUOTE_2& value, F&& field) {
		}
	} // namespace FlatTypes
} // namespace Parser
namespace Parser {
	namespace FlatTypes {
		auto operator<<(std::ostream & os, __WS value) -> std::ostream &;
	} // namespace FlatTypes
} // namespace Parser
namespace Parser {
	namespace FlatTypes {
		struct __WS {
			auto write_to_output(::ISPA_STD::ASTPrinter<std::ostream>& printer) const -> void;
			auto print(std::ostream& os) const -> void;
			auto to_string() const -> std::string;
			constexpr auto members() {
				return std::tie();
			}
			constexpr auto members() const {
				return std::tie();
			}
			static constexpr auto members_str() {
				return std::array<std::string_view, 0>{{}};
			}
			constexpr auto methods() {
				return std::make_tuple(&__WS::write_to_output, &__WS::print);
			}
			constexpr auto methods() const {
				return std::make_tuple(&__WS::write_to_output, &__WS::print);
			}
			static constexpr auto methods_str() {
				return std::array<std::string_view, 2>{{"write_to_output", "print"}};
			}
		};
		inline std::string_view ispa_debug_type(const __WS&) { return "Parser.FlatTypes.__WS"; }
		template<class F> void ispa_debug_fields(const __WS& value, F&& field) {
		}
	} // namespace FlatTypes
} // namespace Parser
namespace Parser {
	namespace FlatTypes {
		auto operator<<(std::ostream & os, QUOTE_1 value) -> std::ostream &;
	} // namespace FlatTypes
} // namespace Parser
namespace Parser {
	namespace FlatTypes {
		struct QUOTE_1 {
			auto write_to_output(::ISPA_STD::ASTPrinter<std::ostream>& printer) const -> void;
			auto print(std::ostream& os) const -> void;
			auto to_string() const -> std::string;
			constexpr auto members() {
				return std::tie();
			}
			constexpr auto members() const {
				return std::tie();
			}
			static constexpr auto members_str() {
				return std::array<std::string_view, 0>{{}};
			}
			constexpr auto methods() {
				return std::make_tuple(&QUOTE_1::write_to_output, &QUOTE_1::print);
			}
			constexpr auto methods() const {
				return std::make_tuple(&QUOTE_1::write_to_output, &QUOTE_1::print);
			}
			static constexpr auto methods_str() {
				return std::array<std::string_view, 2>{{"write_to_output", "print"}};
			}
		};
		inline std::string_view ispa_debug_type(const QUOTE_1&) { return "Parser.FlatTypes.QUOTE_1"; }
		template<class F> void ispa_debug_fields(const QUOTE_1& value, F&& field) {
		}
	} // namespace FlatTypes
} // namespace Parser
namespace Parser {
	namespace FlatTypes {
		template<typename quote>
		struct STRING {
			std::string value;
			auto write_to_output(::ISPA_STD::ASTPrinter<std::ostream>& printer) const -> void {
				printer.node("STRING", false);
				printer.up(false);
				printer.node_with_value("value", value, true);
				printer.down();
			}
			auto print(std::ostream& os) const -> void {
				::ISPA_STD::ASTPrinter<std::ostream> printer = ::ISPA_STD::ASTPrinter<std::ostream>{os};
				write_to_output(printer);
			}
			auto to_string() const -> std::string {
				return ::ISPA_STD::concat("STRING {", "value = ", value, "}");
			}
			constexpr auto members() {
				return std::tie(value);
			}
			constexpr auto members() const {
				return std::tie(value);
			}
			static constexpr auto members_str() {
				return std::array<std::string_view, 1>{{"value"}};
			}
			constexpr auto methods() {
				return std::make_tuple(&STRING::write_to_output, &STRING::print);
			}
			constexpr auto methods() const {
				return std::make_tuple(&STRING::write_to_output, &STRING::print);
			}
			static constexpr auto methods_str() {
				return std::array<std::string_view, 2>{{"write_to_output", "print"}};
			}
		};
	} // namespace FlatTypes
} // namespace Parser
namespace Parser {
	namespace FlatTypes {
		auto operator<<(std::ostream & os, main value) -> std::ostream &;
	} // namespace FlatTypes
} // namespace Parser
namespace Parser {
	namespace FlatTypes {
		struct main {
			::ISPA_STD::Node<::Parser::FlatTypes::ISPA_TEMPLATE_INSTANCE_0> str1;
			auto write_to_output(::ISPA_STD::ASTPrinter<std::ostream>& printer) const -> void;
			auto print(std::ostream& os) const -> void;
			auto to_string() const -> std::string;
			constexpr auto members() {
				return std::tie(str1);
			}
			constexpr auto members() const {
				return std::tie(str1);
			}
			static constexpr auto members_str() {
				return std::array<std::string_view, 1>{{"str1"}};
			}
			constexpr auto methods() {
				return std::make_tuple(&main::write_to_output, &main::print);
			}
			constexpr auto methods() const {
				return std::make_tuple(&main::write_to_output, &main::print);
			}
			static constexpr auto methods_str() {
				return std::array<std::string_view, 2>{{"write_to_output", "print"}};
			}
		};
		inline std::string_view ispa_debug_type(const main&) { return "Parser.FlatTypes.main"; }
		template<class F> void ispa_debug_fields(const main& value, F&& field) {
			field("str1", value.str1);
		}
	} // namespace FlatTypes
} // namespace Parser
namespace Parser {
	namespace Types {
		using ISPA_TEMPLATE_INSTANCE_0 = ::Parser::FlatTypes::ISPA_TEMPLATE_INSTANCE_0;
	} // namespace Types
} // namespace Parser
namespace Parser {
	namespace Types {
		using QUOTE_2 = ::Parser::FlatTypes::QUOTE_2;
	} // namespace Types
} // namespace Parser
namespace Parser {
	namespace Types {
		using __WS = ::Parser::FlatTypes::__WS;
	} // namespace Types
} // namespace Parser
namespace Parser {
	namespace Types {
		using QUOTE_1 = ::Parser::FlatTypes::QUOTE_1;
	} // namespace Types
} // namespace Parser
namespace Parser {
	namespace Types {
		template<typename quote>
		using STRING = ::Parser::FlatTypes::STRING<quote>;
	} // namespace Types
} // namespace Parser
namespace Parser {
	namespace Types {
		using main = ::Parser::FlatTypes::main;
	} // namespace Types
} // namespace Parser
namespace Parser {
	using Token = ::ISPA_STD::RawToken<Types::ISPA_TEMPLATE_INSTANCE_0, Types::QUOTE_2, Types::__WS, Types::QUOTE_1>;
} // namespace Parser
namespace Parser {
	class Lexer : public ::ISPA_STD::Lexer_base<Token> {
	public:
		auto clone() const -> std::shared_ptr<::ISPA_STD::Lexer_base<Token>> override;
	private:
		std::vector<Token> ispa_pending_tokens;
		bool ispa_skip_token = false;
		auto ispa_skip_current_token() -> void;
		auto ispa_token_error(std::string message) -> void;
	public:
		auto on_token(Token&& token) -> void override;
		auto on_eof() -> void override;
	private:
		static ::ISPA_STD::DFA::API::CharToClass char_class_table;
		static ::ISPA_STD::DFA::API::Table<5, 128> dfa_table;
		static ::ISPA_STD::DFA::API::Table<502, 4> action_table;
		auto semantic_action_exec(long long state, const ::ISPA_STD::DFA::API::Captures& captures, long long start_pos, const char* start, long long length, long long line, const char*& cursor) -> std::pair<long long, Token>;
		bool init_done = false;
		auto init() -> void override;
		::ISPA_STD::DFA::API::TdfaLayout<4, 4> tdfa_registers;
		static std::array<::ISPA_STD::DFA::API::DFADebug, 1> debug_array;
		static std::unordered_map<long long, std::unordered_map<long long, long long>> debug_index;
	public:
		auto makeToken(const char*& pos) -> Token override;
		constexpr auto members() {
			return std::tie();
		}
		constexpr auto members() const {
			return std::tie();
		}
		static constexpr auto members_str() {
			return std::array<std::string_view, 0>{{}};
		}
		constexpr auto methods() {
			return std::make_tuple(&Lexer::clone, &Lexer::on_token, &Lexer::on_eof, &Lexer::makeToken);
		}
		constexpr auto methods() const {
			return std::make_tuple(&Lexer::clone, &Lexer::on_token, &Lexer::on_eof, &Lexer::makeToken);
		}
		static constexpr auto methods_str() {
			return std::array<std::string_view, 4>{{"clone", "on_token", "on_eof", "makeToken"}};
		}
	};
} // namespace Parser
namespace Parser {
	class Parser : public ::ISPA_STD::LLParser<Types::main, Token> {
	protected:
		template<typename IT, class = std::enable_if_t<std::is_same_v<std::remove_const_t<std::remove_reference_t<IT>>, ::Parser::Lexer::iterator> || std::is_same_v<std::remove_const_t<std::remove_reference_t<IT>>, ::Parser::Lexer::lazy_iterator>>> auto main(IT pos) -> ::ISPA_STD::MatchResult<Types::main, IT>;
	public:
		auto parseFromTokens() -> void override;
	private:
		Lexer lazy_lexer_;
	public:
		auto lazyParse() -> ::ISPA_STD::MatchResult<Types::main, Lexer::lazy_iterator> override;
		constexpr auto members() {
			return std::tie();
		}
		constexpr auto members() const {
			return std::tie();
		}
		static constexpr auto members_str() {
			return std::array<std::string_view, 0>{{}};
		}
		constexpr auto methods() {
			return std::make_tuple(&Parser::parseFromTokens, &Parser::lazyParse);
		}
		constexpr auto methods() const {
			return std::make_tuple(&Parser::parseFromTokens, &Parser::lazyParse);
		}
		static constexpr auto methods_str() {
			return std::array<std::string_view, 2>{{"parseFromTokens", "lazyParse"}};
		}
	};
} // namespace Parser

#include <ispastdlib/external.hpp>
namespace Parser::Exports {
struct symbol_0 : ::Parser::Parser {
using node_type = ::Parser::Types::QUOTE_1;
static bool starts(const char* origin, const char* start);
static ISPA_STD::ExternalResult<node_type> parse(const char* origin, const char* start);
};
struct symbol_1 : ::Parser::Parser {
using node_type = ::Parser::Types::QUOTE_2;
static bool starts(const char* origin, const char* start);
static ISPA_STD::ExternalResult<node_type> parse(const char* origin, const char* start);
};
struct symbol_2 : ::Parser::Parser {
using node_type = ::Parser::Types::main;
static bool starts(const char* origin, const char* start);
static ISPA_STD::ExternalResult<node_type> parse(const char* origin, const char* start);
};
}
