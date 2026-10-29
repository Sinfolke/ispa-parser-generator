#pragma once

#include <ispastdlib/lr_conflicts.hpp>
#include <ispastdlib.hpp>

namespace o {
	namespace Tokens {
		namespace __WS {
			inline const std::size_t id = 2288775507628772018;
		} // namespace __WS
	} // namespace Tokens
} // namespace o
namespace o {
	namespace Tokens {
		namespace __WS {
			inline const bool shared = true;
		} // namespace __WS
	} // namespace Tokens
} // namespace o
namespace o {
	namespace Tokens {
		namespace modules {
			inline const std::size_t id = 2;
		} // namespace modules
	} // namespace Tokens
} // namespace o
namespace o {
	namespace Tokens {
		namespace modules {
			inline const bool shared = true;
		} // namespace modules
	} // namespace Tokens
} // namespace o
namespace o {
	namespace Tokens {
		namespace modules {
			namespace c {
				inline const std::size_t id = 3;
			} // namespace c
		} // namespace modules
	} // namespace Tokens
} // namespace o
namespace o {
	namespace Tokens {
		namespace modules {
			namespace c {
				inline const bool shared = true;
			} // namespace c
		} // namespace modules
	} // namespace Tokens
} // namespace o
namespace o {
	namespace Tokens {
		namespace modules {
			namespace c {
				namespace main {
					inline const std::size_t id = 4;
				} // namespace main
			} // namespace c
		} // namespace modules
	} // namespace Tokens
} // namespace o
namespace o {
	namespace Tokens {
		namespace modules {
			namespace c {
				namespace main {
					inline const bool shared = true;
				} // namespace main
			} // namespace c
		} // namespace modules
	} // namespace Tokens
} // namespace o
namespace o {
	namespace Tokens {
		namespace modules {
			namespace c {
				namespace main {
					namespace LATIN_SMALL_LETTER_X {
						inline const std::size_t id = 4038058866088313642;
					} // namespace LATIN_SMALL_LETTER_X
				} // namespace main
			} // namespace c
		} // namespace modules
	} // namespace Tokens
} // namespace o
namespace o {
	namespace Tokens {
		namespace modules {
			namespace c {
				namespace main {
					namespace LATIN_SMALL_LETTER_X {
						inline const bool shared = true;
					} // namespace LATIN_SMALL_LETTER_X
				} // namespace main
			} // namespace c
		} // namespace modules
	} // namespace Tokens
} // namespace o
namespace o {
	namespace TokensStr {
		namespace __WS {
			inline const char* const id = "__WS";
		} // namespace __WS
	} // namespace TokensStr
} // namespace o
namespace o {
	namespace TokensStr {
		namespace modules {
			inline const char* const id = "modules";
		} // namespace modules
	} // namespace TokensStr
} // namespace o
namespace o {
	namespace TokensStr {
		namespace modules {
			namespace c {
				inline const char* const id = "modules.c";
			} // namespace c
		} // namespace modules
	} // namespace TokensStr
} // namespace o
namespace o {
	namespace TokensStr {
		namespace modules {
			namespace c {
				namespace main {
					inline const char* const id = "modules.c.main";
				} // namespace main
			} // namespace c
		} // namespace modules
	} // namespace TokensStr
} // namespace o
namespace o {
	namespace TokensStr {
		namespace modules {
			namespace c {
				namespace main {
					namespace LATIN_SMALL_LETTER_X {
						inline const char* const id = "modules.c.main.LATIN_SMALL_LETTER_X";
					} // namespace LATIN_SMALL_LETTER_X
				} // namespace main
			} // namespace c
		} // namespace modules
	} // namespace TokensStr
} // namespace o
namespace o {
	namespace Rules {
		namespace main {
			inline const std::size_t id = 0;
		} // namespace main
	} // namespace Rules
} // namespace o
namespace o {
	namespace Rules {
		namespace modules {
			inline const std::size_t id = 2;
		} // namespace modules
	} // namespace Rules
} // namespace o
namespace o {
	namespace Rules {
		namespace modules {
			namespace c {
				inline const std::size_t id = 3;
			} // namespace c
		} // namespace modules
	} // namespace Rules
} // namespace o
namespace o {
	namespace Rules {
		namespace modules {
			namespace c {
				namespace main {
					inline const std::size_t id = 1;
				} // namespace main
			} // namespace c
		} // namespace modules
	} // namespace Rules
} // namespace o
namespace o {
	namespace RulesStr {
		namespace main {
			inline const char* const id = "main";
		} // namespace main
	} // namespace RulesStr
} // namespace o
namespace o {
	namespace RulesStr {
		namespace modules {
			inline const char* const id = "modules";
		} // namespace modules
	} // namespace RulesStr
} // namespace o
namespace o {
	namespace RulesStr {
		namespace modules {
			namespace c {
				inline const char* const id = "modules.c";
			} // namespace c
		} // namespace modules
	} // namespace RulesStr
} // namespace o
namespace o {
	namespace RulesStr {
		namespace modules {
			namespace c {
				namespace main {
					inline const char* const id = "modules.c.main";
				} // namespace main
			} // namespace c
		} // namespace modules
	} // namespace RulesStr
} // namespace o
namespace o {
	auto TokensToString(std::size_t id) -> const char*;
} // namespace o
namespace o {
	auto RulesToString(std::size_t id) -> const char*;
} // namespace o
namespace o::FlatTypes { struct __WS; }
namespace o::FlatTypes { struct modules_c_main_LATIN_SMALL_LETTER_X; }
namespace o::FlatTypes { struct modules; }
namespace o::FlatTypes { struct modules_c; }
namespace o::FlatTypes { struct modules_c_main; }
namespace o::FlatTypes { struct main; }
namespace o {
	namespace FlatTypes {
		auto operator<<(std::ostream & os, __WS value) -> std::ostream &;
	} // namespace FlatTypes
} // namespace o
namespace o {
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
	} // namespace FlatTypes
} // namespace o
namespace o {
	namespace FlatTypes {
		auto operator<<(std::ostream & os, modules_c_main_LATIN_SMALL_LETTER_X value) -> std::ostream &;
	} // namespace FlatTypes
} // namespace o
namespace o {
	namespace FlatTypes {
		struct modules_c_main_LATIN_SMALL_LETTER_X {
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
				return std::make_tuple(&modules_c_main_LATIN_SMALL_LETTER_X::write_to_output, &modules_c_main_LATIN_SMALL_LETTER_X::print);
			}
			constexpr auto methods() const {
				return std::make_tuple(&modules_c_main_LATIN_SMALL_LETTER_X::write_to_output, &modules_c_main_LATIN_SMALL_LETTER_X::print);
			}
			static constexpr auto methods_str() {
				return std::array<std::string_view, 2>{{"write_to_output", "print"}};
			}
		};
	} // namespace FlatTypes
} // namespace o
namespace o {
	namespace FlatTypes {
		auto operator<<(std::ostream & os, modules value) -> std::ostream &;
	} // namespace FlatTypes
} // namespace o
namespace o {
	namespace FlatTypes {
		struct modules {
			using c = ::o::FlatTypes::modules_c;
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
				return std::make_tuple(&modules::write_to_output, &modules::print);
			}
			constexpr auto methods() const {
				return std::make_tuple(&modules::write_to_output, &modules::print);
			}
			static constexpr auto methods_str() {
				return std::array<std::string_view, 2>{{"write_to_output", "print"}};
			}
		};
	} // namespace FlatTypes
} // namespace o
namespace o {
	namespace FlatTypes {
		auto operator<<(std::ostream & os, modules_c value) -> std::ostream &;
	} // namespace FlatTypes
} // namespace o
namespace o {
	namespace FlatTypes {
		struct modules_c {
			using main = ::o::FlatTypes::modules_c_main;
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
				return std::make_tuple(&modules_c::write_to_output, &modules_c::print);
			}
			constexpr auto methods() const {
				return std::make_tuple(&modules_c::write_to_output, &modules_c::print);
			}
			static constexpr auto methods_str() {
				return std::array<std::string_view, 2>{{"write_to_output", "print"}};
			}
		};
	} // namespace FlatTypes
} // namespace o
namespace o {
	namespace FlatTypes {
		auto operator<<(std::ostream & os, modules_c_main value) -> std::ostream &;
	} // namespace FlatTypes
} // namespace o
namespace o {
	namespace FlatTypes {
		struct modules_c_main {
			using LATIN_SMALL_LETTER_X = ::o::FlatTypes::modules_c_main_LATIN_SMALL_LETTER_X;
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
				return std::make_tuple(&modules_c_main::write_to_output, &modules_c_main::print);
			}
			constexpr auto methods() const {
				return std::make_tuple(&modules_c_main::write_to_output, &modules_c_main::print);
			}
			static constexpr auto methods_str() {
				return std::array<std::string_view, 2>{{"write_to_output", "print"}};
			}
		};
	} // namespace FlatTypes
} // namespace o
namespace o {
	namespace FlatTypes {
		auto operator<<(std::ostream & os, main value) -> std::ostream &;
	} // namespace FlatTypes
} // namespace o
namespace o {
	namespace FlatTypes {
		struct main {
			::ISPA_STD::Node<::o::FlatTypes::modules_c_main> value;
			auto write_to_output(::ISPA_STD::ASTPrinter<std::ostream>& printer) const -> void;
			auto print(std::ostream& os) const -> void;
			auto to_string() const -> std::string;
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
				return std::make_tuple(&main::write_to_output, &main::print);
			}
			constexpr auto methods() const {
				return std::make_tuple(&main::write_to_output, &main::print);
			}
			static constexpr auto methods_str() {
				return std::array<std::string_view, 2>{{"write_to_output", "print"}};
			}
		};
	} // namespace FlatTypes
} // namespace o
namespace o {
	namespace Types {
		using __WS = ::o::FlatTypes::__WS;
	} // namespace Types
} // namespace o
namespace o {
	namespace Types {
		using modules = ::o::FlatTypes::modules;
	} // namespace Types
} // namespace o
namespace o {
	namespace Types {
		using main = ::o::FlatTypes::main;
	} // namespace Types
} // namespace o
namespace o {
	using Token = ::ISPA_STD::RawToken<Types::__WS, Types::modules::c::main::LATIN_SMALL_LETTER_X>;
} // namespace o
namespace o {
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
		static ::ISPA_STD::DFA::API::Table<3, 3> dfa_table;
		static ::ISPA_STD::DFA::API::Table<0, 4> action_table;
		auto semantic_action_exec(long long state, const ::ISPA_STD::DFA::API::Captures& captures, long long start_pos, const char* start, long long length, long long line, const char*& cursor) -> std::pair<long long, Token>;
		bool init_done = false;
		auto init() -> void override;
		::ISPA_STD::DFA::API::TdfaLayout<0, 0> tdfa_registers;
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
} // namespace o
namespace o {
	class Parser : public ::ISPA_STD::LLParser<Types::main, Token> {
	protected:
		template<typename IT, class = std::enable_if_t<std::is_same_v<std::remove_const_t<std::remove_reference_t<IT>>, ::o::Lexer::iterator> || std::is_same_v<std::remove_const_t<std::remove_reference_t<IT>>, ::o::Lexer::lazy_iterator>>> auto main(IT pos) -> ::ISPA_STD::MatchResult<Types::main, IT>;
		template<typename IT, class = std::enable_if_t<std::is_same_v<std::remove_const_t<std::remove_reference_t<IT>>, ::o::Lexer::iterator> || std::is_same_v<std::remove_const_t<std::remove_reference_t<IT>>, ::o::Lexer::lazy_iterator>>> auto modulesCMain(IT pos) -> ::ISPA_STD::MatchResult<Types::modules::c::main, IT>;
	public:
		auto parseFromTokens() -> void override;
	private:
		Lexer lazy_lexer_;
	public:
		auto lazyParse() -> ::ISPA_STD::MatchResult<Types::main, ::o::Lexer::lazy_iterator> override;
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
} // namespace o
namespace python {
	namespace ispa_std {
		namespace chars {
			auto code_of(const std::string& ch) -> long long;
		} // namespace chars
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace chars {
			auto char_of(long long code) -> std::string;
		} // namespace chars
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace chars {
			auto at(const std::string& s, long long index) -> std::string;
		} // namespace chars
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace chars {
			auto is_space(const std::string& ch) -> bool;
		} // namespace chars
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace chars {
			auto is_digit(const std::string& ch) -> bool;
		} // namespace chars
	} // namespace ispa_std
} // namespace python
namespace python::ispa_std::errors { struct ErrorKind; }
namespace python {
	namespace ispa_std {
		namespace errors {
			struct ErrorKind : public ::ISPA_STD::Object {
				static long long OutOfRange;
				static long long InvalidArgument;
				static long long LengthError;
				static long long BadOptionalAccess;
				static long long KeyNotFound;
				static long long LogicError;
				static long long RuntimeFailure;
				static long long InternalError;
				static long long ApiError;
				static long long NoMatch;
				static long long ExternalParse;
				constexpr auto members() {
					return std::tie(OutOfRange, InvalidArgument, LengthError, BadOptionalAccess, KeyNotFound, LogicError, RuntimeFailure, InternalError, ApiError, NoMatch, ExternalParse);
				}
				constexpr auto members() const {
					return std::tie(OutOfRange, InvalidArgument, LengthError, BadOptionalAccess, KeyNotFound, LogicError, RuntimeFailure, InternalError, ApiError, NoMatch, ExternalParse);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 11>{{"OutOfRange", "InvalidArgument", "LengthError", "BadOptionalAccess", "KeyNotFound", "LogicError", "RuntimeFailure", "InternalError", "ApiError", "NoMatch", "ExternalParse"}};
				}
				constexpr auto methods() {
					return std::make_tuple();
				}
				constexpr auto methods() const {
					return std::make_tuple();
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 0>{{}};
				}
			};
		} // namespace errors
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace string {
			inline long long NPOS = ( - (1));
		} // namespace string
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace string {
			auto substr(const std::string& s, long long pos, long long count) -> std::string;
		} // namespace string
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace string {
			auto compare(const std::string& a, const std::string& b) -> long long;
		} // namespace string
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace string {
			auto matches_at(const std::string& s, const std::string& needle, long long pos) -> bool;
		} // namespace string
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace string {
			auto starts_with(const std::string& s, const std::string& prefix) -> bool;
		} // namespace string
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace string {
			auto ends_with(const std::string& s, const std::string& suffix) -> bool;
		} // namespace string
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace string {
			auto find(const std::string& s, const std::string& needle, long long start) -> long long;
		} // namespace string
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace string {
			auto rfind(const std::string& s, const std::string& needle) -> long long;
		} // namespace string
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace string {
			auto contains(const std::string& s, const std::string& needle) -> bool;
		} // namespace string
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace format {
			auto int_to_string(long long n) -> std::string;
		} // namespace format
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace format {
			auto bool_to_string(bool value) -> std::string;
		} // namespace format
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace format {
			auto fmt(const std::string& format_text, const std::vector<std::string>& args) -> std::string;
		} // namespace format
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace format {
			auto replace_all(const std::string& s, const std::string& needle, const std::string& replacement) -> std::string;
		} // namespace format
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace format {
			auto repeat(const std::string& s, long long count) -> std::string;
		} // namespace format
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace format {
			auto join(const std::vector<std::string>& items, const std::string& separator) -> std::string;
		} // namespace format
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace format {
			auto concat_all(const std::vector<std::string>& items) -> std::string;
		} // namespace format
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace utility {
			template<typename A, typename B>
			struct Pair : public ::ISPA_STD::Object {
				A first;
				B second;
				auto __init__(A first, B second) -> void {
					((*this)).first = first;
					((*this)).second = second;
				}
				virtual auto __eq__(std::shared_ptr<python::ispa_std::utility::Pair<A, B>> other) -> bool {
					return ((((((*this)).first) == (((*other)).first))) && (((((*this)).second) == (((*other)).second))));
				}
				virtual auto __lt__(std::shared_ptr<python::ispa_std::utility::Pair<A, B>> other) -> bool {
					if (((((*this)).first) < (((*other)).first))) {
						return true;
					}
					if (((((*other)).first) < (((*this)).first))) {
						return false;
					}
					return ((((*this)).second) < (((*other)).second));
				}
				constexpr auto members() {
					return std::tie(first, second);
				}
				constexpr auto members() const {
					return std::tie(first, second);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 2>{{"first", "second"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&Pair::__init__, &Pair::__eq__, &Pair::__lt__);
				}
				constexpr auto methods() const {
					return std::make_tuple(&Pair::__init__, &Pair::__eq__, &Pair::__lt__);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 3>{{"__init__", "__eq__", "__lt__"}};
				}
			};
		} // namespace utility
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace utility {
			template<typename A, typename B> auto make_pair(A first, B second) -> std::shared_ptr<python::ispa_std::utility::Pair<A, B>>;
		} // namespace utility
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace utility {
			template<typename T> auto minimum(T a, T b) -> T;
		} // namespace utility
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace utility {
			template<typename T> auto maximum(T a, T b) -> T;
		} // namespace utility
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace utility {
			template<typename T> auto clamp(T value, T low, T high) -> T;
		} // namespace utility
	} // namespace ispa_std
} // namespace python
namespace python::ispa_std::debug { struct Kind; }
namespace python::ispa_std::debug { struct Location; }
namespace python::ispa_std::debug { struct DebugSink; }
namespace python::ispa_std::debug { struct DebugControl; }
namespace python::ispa_std::debug { struct ScopeBase; }
namespace python::ispa_std::debug { struct DebugState; }
namespace python::ispa_std::debug { struct Snapshot; }
namespace python::ispa_std::debug { struct RuleScope; }
namespace python::ispa_std::debug { struct ProductionScope; }
namespace python::ispa_std::debug { struct MatchScope; }
namespace python {
	namespace ispa_std {
		namespace debug {
			struct Kind : public ::ISPA_STD::Object {
				static long long trace;
				static long long value;
				static long long enter_rule;
				static long long exit_rule;
				static long long diagnostic;
				static long long breakpoint;
				static long long lexer_enter;
				static long long lexer_leave;
				static long long lexer_capture;
				static long long source_line;
				static long long production_enter;
				static long long production_leave;
				static long long snapshot;
				static long long session_end;
				constexpr auto members() {
					return std::tie(trace, value, enter_rule, exit_rule, diagnostic, breakpoint, lexer_enter, lexer_leave, lexer_capture, source_line, production_enter, production_leave, snapshot, session_end);
				}
				constexpr auto members() const {
					return std::tie(trace, value, enter_rule, exit_rule, diagnostic, breakpoint, lexer_enter, lexer_leave, lexer_capture, source_line, production_enter, production_leave, snapshot, session_end);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 14>{{"trace", "value", "enter_rule", "exit_rule", "diagnostic", "breakpoint", "lexer_enter", "lexer_leave", "lexer_capture", "source_line", "production_enter", "production_leave", "snapshot", "session_end"}};
				}
				constexpr auto methods() {
					return std::make_tuple();
				}
				constexpr auto methods() const {
					return std::make_tuple();
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 0>{{}};
				}
			};
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			inline long long MAX_PAYLOAD = 1048576;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			struct Location : public ::ISPA_STD::Object {
				std::string file;
				long long line;
				long long column;
				auto __init__(const std::string& file, long long line, long long column) -> void;
				static auto none() -> std::shared_ptr<python::ispa_std::debug::Location>;
				virtual auto copy() -> std::shared_ptr<python::ispa_std::debug::Location>;
				constexpr auto members() {
					return std::tie(file, line, column);
				}
				constexpr auto members() const {
					return std::tie(file, line, column);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 3>{{"file", "line", "column"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&Location::__init__, &Location::none, &Location::copy);
				}
				constexpr auto methods() const {
					return std::make_tuple(&Location::__init__, &Location::none, &Location::copy);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 3>{{"__init__", "none", "copy"}};
				}
			};
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			struct DebugSink : public ::ISPA_STD::Object {
				virtual auto write(const std::string& text) -> bool;
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
					return std::make_tuple(&DebugSink::write);
				}
				constexpr auto methods() const {
					return std::make_tuple(&DebugSink::write);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 1>{{"write"}};
				}
			};
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			struct DebugControl : public ::ISPA_STD::Object {
				virtual auto read_byte() -> long long;
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
					return std::make_tuple(&DebugControl::read_byte);
				}
				constexpr auto methods() const {
					return std::make_tuple(&DebugControl::read_byte);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 1>{{"read_byte"}};
				}
			};
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			struct ScopeBase : public ::ISPA_STD::Object {
				long long serial;
				virtual auto close() -> void;
				constexpr auto members() {
					return std::tie(serial);
				}
				constexpr auto members() const {
					return std::tie(serial);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 1>{{"serial"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&ScopeBase::close);
				}
				constexpr auto methods() const {
					return std::make_tuple(&ScopeBase::close);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 1>{{"close"}};
				}
			};
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			struct DebugState : public ::ISPA_STD::Object {
				std::shared_ptr<python::ispa_std::debug::DebugSink> sink;
				bool has_sink;
				std::shared_ptr<python::ispa_std::debug::DebugControl> control;
				bool has_control;
				bool stepping;
				long long target_depth;
				long long frame_depth;
				std::vector<std::string> break_files;
				std::vector<long long> break_lines;
				bool failure_seen;
				std::shared_ptr<python::ispa_std::debug::Location> location;
				std::vector<std::shared_ptr<python::ispa_std::debug::ScopeBase>> scopes;
				long long serial;
				auto __init__() -> void;
				constexpr auto members() {
					return std::tie(sink, has_sink, control, has_control, stepping, target_depth, frame_depth, break_files, break_lines, failure_seen, location, scopes, serial);
				}
				constexpr auto members() const {
					return std::tie(sink, has_sink, control, has_control, stepping, target_depth, frame_depth, break_files, break_lines, failure_seen, location, scopes, serial);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 13>{{"sink", "has_sink", "control", "has_control", "stepping", "target_depth", "frame_depth", "break_files", "break_lines", "failure_seen", "location", "scopes", "serial"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&DebugState::__init__);
				}
				constexpr auto methods() const {
					return std::make_tuple(&DebugState::__init__);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 1>{{"__init__"}};
				}
			};
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			inline std::shared_ptr<python::ispa_std::debug::DebugState> _S = ISPA_STD::construct<python::ispa_std::debug::DebugState>();
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto configure(std::shared_ptr<python::ispa_std::debug::DebugSink> sink, std::shared_ptr<python::ispa_std::debug::DebugControl> control) -> void;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto configure_sink_only(std::shared_ptr<python::ispa_std::debug::DebugSink> sink) -> void;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto reset() -> void;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto enabled() -> bool;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto initial_pause() -> void;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto active_location() -> std::shared_ptr<python::ispa_std::debug::Location>;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto frame(long long kind, const std::string& payload) -> std::string;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto emit(long long kind, const std::string& payload) -> bool;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto quote(const std::string& s) -> std::string;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto obj(const std::string& type_name, const std::string& data) -> std::string;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto location_json(std::shared_ptr<python::ispa_std::debug::Location> loc) -> std::string;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto event(long long kind, std::shared_ptr<python::ispa_std::debug::Location> loc, const std::string& name, const std::string& extra) -> bool;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto trace(const std::string& text) -> bool;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto diagnostic(const std::string& text) -> bool;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto enter_rule(const std::string& name, std::shared_ptr<python::ispa_std::debug::Location> loc) -> bool;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto exit_rule(const std::string& name, std::shared_ptr<python::ispa_std::debug::Location> loc) -> bool;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto enter_production(const std::string& name, std::shared_ptr<python::ispa_std::debug::Location> loc) -> bool;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto leave_production(const std::string& name, std::shared_ptr<python::ispa_std::debug::Location> loc) -> bool;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto lexer_enter(const std::string& token, std::shared_ptr<python::ispa_std::debug::Location> loc) -> bool;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto lexer_result(const std::string& token, bool success, const std::string& actual, const std::string& reason, std::shared_ptr<python::ispa_std::debug::Location> loc) -> bool;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto value(const std::string& name, const std::string& encoded, std::shared_ptr<python::ispa_std::debug::Location> loc) -> bool;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto lexer_capture(const std::string& name, const std::string& encoded, std::shared_ptr<python::ispa_std::debug::Location> loc) -> bool;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			struct Snapshot : public ::ISPA_STD::Object {
				std::vector<std::string> _values;
				auto __init__() -> void;
				virtual auto add(const std::string& name, const std::string& encoded) -> std::shared_ptr<python::ispa_std::debug::Snapshot>;
				virtual auto json() -> std::string;
				constexpr auto members() {
					return std::tie(_values);
				}
				constexpr auto members() const {
					return std::tie(_values);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 1>{{"_values"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&Snapshot::__init__, &Snapshot::add, &Snapshot::json);
				}
				constexpr auto methods() const {
					return std::make_tuple(&Snapshot::__init__, &Snapshot::add, &Snapshot::json);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 3>{{"__init__", "add", "json"}};
				}
			};
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto command() -> bool;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto line(std::shared_ptr<python::ispa_std::debug::Location> loc, std::shared_ptr<python::ispa_std::debug::Snapshot> snap, const std::string& phase) -> bool;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto rule_depth_enter() -> void;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto rule_depth_leave() -> void;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto breakpoint(const std::string& label) -> void;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto scope_depth() -> long long;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto unwind_to(long long depth) -> void;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto _next_serial() -> long long;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			auto _forget(std::shared_ptr<python::ispa_std::debug::ScopeBase> scope) -> void;
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			struct RuleScope : public python::ispa_std::debug::ScopeBase {
				std::string name;
				std::shared_ptr<python::ispa_std::debug::Location> loc;
				std::shared_ptr<python::ispa_std::debug::Location> previous;
				bool open;
				auto __init__(const std::string& name, std::shared_ptr<python::ispa_std::debug::Location> loc) -> void;
				virtual auto close() -> void;
				constexpr auto members() {
					return std::tie(name, loc, previous, open);
				}
				constexpr auto members() const {
					return std::tie(name, loc, previous, open);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 4>{{"name", "loc", "previous", "open"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&RuleScope::__init__, &RuleScope::close);
				}
				constexpr auto methods() const {
					return std::make_tuple(&RuleScope::__init__, &RuleScope::close);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 2>{{"__init__", "close"}};
				}
			};
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			struct ProductionScope : public python::ispa_std::debug::ScopeBase {
				std::string name;
				std::shared_ptr<python::ispa_std::debug::Location> loc;
				std::shared_ptr<python::ispa_std::debug::Location> previous;
				bool open;
				auto __init__(const std::string& name, std::shared_ptr<python::ispa_std::debug::Location> loc) -> void;
				virtual auto close() -> void;
				constexpr auto members() {
					return std::tie(name, loc, previous, open);
				}
				constexpr auto members() const {
					return std::tie(name, loc, previous, open);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 4>{{"name", "loc", "previous", "open"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&ProductionScope::__init__, &ProductionScope::close);
				}
				constexpr auto methods() const {
					return std::make_tuple(&ProductionScope::__init__, &ProductionScope::close);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 2>{{"__init__", "close"}};
				}
			};
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace debug {
			struct MatchScope : public python::ispa_std::debug::ScopeBase {
				std::string expected;
				std::string actual;
				std::string reason;
				std::shared_ptr<python::ispa_std::debug::Location> loc;
				bool matched;
				bool active;
				bool open;
				auto __init__(const std::string& want, const std::string& got) -> void;
				virtual auto success() -> void;
				virtual auto outcome(const std::string& why) -> void;
				virtual auto close() -> void;
				constexpr auto members() {
					return std::tie(expected, actual, reason, loc, matched, active, open);
				}
				constexpr auto members() const {
					return std::tie(expected, actual, reason, loc, matched, active, open);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 7>{{"expected", "actual", "reason", "loc", "matched", "active", "open"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&MatchScope::__init__, &MatchScope::success, &MatchScope::outcome, &MatchScope::close);
				}
				constexpr auto methods() const {
					return std::make_tuple(&MatchScope::__init__, &MatchScope::success, &MatchScope::outcome, &MatchScope::close);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 4>{{"__init__", "success", "outcome", "close"}};
				}
			};
		} // namespace debug
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace failures {
			auto logic_error(const std::string& message) -> void;
		} // namespace failures
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace failures {
			auto runtime_error(const std::string& message) -> void;
		} // namespace failures
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace failures {
			auto out_of_range(const std::string& message) -> void;
		} // namespace failures
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace failures {
			auto invalid_argument(const std::string& message) -> void;
		} // namespace failures
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace failures {
			auto length_error(const std::string& message) -> void;
		} // namespace failures
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace failures {
			auto api_error(const std::string& message, const std::vector<std::string>& args) -> void;
		} // namespace failures
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace failures {
			auto internal_error(const std::string& message, const std::string& issue, bool ispa_side, const std::vector<std::string>& args) -> void;
		} // namespace failures
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace failures {
			auto no_match() -> void;
		} // namespace failures
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace failures {
			auto external_parse_error(const std::string& message) -> void;
		} // namespace failures
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace failures {
			auto rethrow(long long kind, const std::string& message) -> void;
		} // namespace failures
	} // namespace ispa_std
} // namespace python
namespace python::ispa_std::node { struct Node; }
namespace python {
	namespace ispa_std {
		namespace node {
			inline long long NO_POS = ( - (1));
		} // namespace node
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace node {
			inline long long NO_NAME = ( - (1));
		} // namespace node
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace node {
			struct Node : public ::ISPA_STD::Object {
				long long _startpos;
				long long _length;
				long long _line;
				long long _column;
				std::string _source;
				bool _has_source;
				long long _name;
				bool _empty;
				std::any _data;
				auto __init__(long long startpos, const std::string& source, bool has_source, long long length, long long line, long long column, long long name, bool empty, std::any data) -> void;
				static auto blank() -> std::shared_ptr<python::ispa_std::node::Node>;
				static auto with_meta(long long startpos, const std::string& source, bool has_source, long long length, long long line, long long column, long long name) -> std::shared_ptr<python::ispa_std::node::Node>;
				static auto with_data(long long startpos, const std::string& source, bool has_source, long long length, long long line, long long column, long long name, std::any data) -> std::shared_ptr<python::ispa_std::node::Node>;
				static auto create(long long startpos, const std::string& source, long long length, long long line, long long enumv, std::any value) -> std::shared_ptr<python::ispa_std::node::Node>;
				virtual auto copy() -> std::shared_ptr<python::ispa_std::node::Node>;
				virtual auto assign(std::shared_ptr<python::ispa_std::node::Node> other) -> void;
				virtual auto endpos() -> long long;
				virtual auto clear() -> void;
				virtual auto empty() -> bool;
				virtual auto startpos() -> long long;
				virtual auto line() -> long long;
				virtual auto column() -> long long;
				virtual auto length() -> long long;
				virtual auto source() -> const std::string&;
				virtual auto has_source() -> bool;
				virtual auto name() -> long long;
				virtual auto data() -> std::any;
				virtual auto set_data(std::any value) -> void;
				virtual auto text() -> std::string;
				constexpr auto members() {
					return std::tie(_startpos, _length, _line, _column, _source, _has_source, _name, _empty, _data);
				}
				constexpr auto members() const {
					return std::tie(_startpos, _length, _line, _column, _source, _has_source, _name, _empty, _data);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 9>{{"_startpos", "_length", "_line", "_column", "_source", "_has_source", "_name", "_empty", "_data"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&Node::__init__, &Node::blank, &Node::with_meta, &Node::with_data, &Node::create, &Node::copy, &Node::assign, &Node::endpos, &Node::clear, &Node::empty, &Node::startpos, &Node::line, &Node::column, &Node::length, &Node::source, &Node::has_source, &Node::name, &Node::data, &Node::set_data, &Node::text);
				}
				constexpr auto methods() const {
					return std::make_tuple(&Node::__init__, &Node::blank, &Node::with_meta, &Node::with_data, &Node::create, &Node::copy, &Node::assign, &Node::endpos, &Node::clear, &Node::empty, &Node::startpos, &Node::line, &Node::column, &Node::length, &Node::source, &Node::has_source, &Node::name, &Node::data, &Node::set_data, &Node::text);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 20>{{"__init__", "blank", "with_meta", "with_data", "create", "copy", "assign", "endpos", "clear", "empty", "startpos", "line", "column", "length", "source", "has_source", "name", "data", "set_data", "text"}};
				}
			};
		} // namespace node
	} // namespace ispa_std
} // namespace python
namespace python::ispa_std::raw_token { struct RawTokenMeta; }
namespace python::ispa_std::raw_token { struct RawToken; }
namespace python {
	namespace ispa_std {
		namespace raw_token {
			inline long long NO_POS = ( - (1));
		} // namespace raw_token
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace raw_token {
			inline long long NO_NAME = ( - (1));
		} // namespace raw_token
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace raw_token {
			inline long long NO_TYPE = ( - (1));
		} // namespace raw_token
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace raw_token {
			inline long long AMBIGUOUS_ID = ( - (2));
		} // namespace raw_token
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace raw_token {
			struct RawTokenMeta : public ::ISPA_STD::Object {
				long long _startpos;
				long long _length;
				long long _line;
				long long _column;
				std::string _source;
				bool _has_source;
				long long _name;
				auto __init__(long long startpos, const std::string& source, bool has_source, long long length, long long line, long long column, long long name) -> void;
				static auto blank() -> std::shared_ptr<python::ispa_std::raw_token::RawTokenMeta>;
				virtual auto startpos() -> long long;
				virtual auto length() -> long long;
				virtual auto line() -> long long;
				virtual auto column() -> long long;
				virtual auto name() -> long long;
				virtual auto source() -> const std::string&;
				virtual auto has_source() -> bool;
				virtual auto end_pos() -> long long;
				virtual auto text() -> std::string;
				constexpr auto members() {
					return std::tie(_startpos, _length, _line, _column, _source, _has_source, _name);
				}
				constexpr auto members() const {
					return std::tie(_startpos, _length, _line, _column, _source, _has_source, _name);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 7>{{"_startpos", "_length", "_line", "_column", "_source", "_has_source", "_name"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&RawTokenMeta::__init__, &RawTokenMeta::blank, &RawTokenMeta::startpos, &RawTokenMeta::length, &RawTokenMeta::line, &RawTokenMeta::column, &RawTokenMeta::name, &RawTokenMeta::source, &RawTokenMeta::has_source, &RawTokenMeta::end_pos, &RawTokenMeta::text);
				}
				constexpr auto methods() const {
					return std::make_tuple(&RawTokenMeta::__init__, &RawTokenMeta::blank, &RawTokenMeta::startpos, &RawTokenMeta::length, &RawTokenMeta::line, &RawTokenMeta::column, &RawTokenMeta::name, &RawTokenMeta::source, &RawTokenMeta::has_source, &RawTokenMeta::end_pos, &RawTokenMeta::text);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 11>{{"__init__", "blank", "startpos", "length", "line", "column", "name", "source", "has_source", "end_pos", "text"}};
				}
			};
		} // namespace raw_token
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace raw_token {
			struct RawToken : public ::ISPA_STD::Object {
				long long _type_id;
				std::any _payload;
				std::shared_ptr<python::ispa_std::raw_token::RawTokenMeta> _meta;
				auto __init__(std::shared_ptr<python::ispa_std::raw_token::RawTokenMeta> meta, long long type_id, std::any payload) -> void;
				static auto empty_token() -> std::shared_ptr<python::ispa_std::raw_token::RawToken>;
				static auto ambiguous(std::shared_ptr<python::ispa_std::raw_token::RawTokenMeta> meta) -> std::shared_ptr<python::ispa_std::raw_token::RawToken>;
				static auto make(std::shared_ptr<python::ispa_std::raw_token::RawTokenMeta> meta, long long type_id, std::any payload) -> std::shared_ptr<python::ispa_std::raw_token::RawToken>;
				static auto from_node(std::shared_ptr<python::ispa_std::node::Node> node, long long type_id) -> std::shared_ptr<python::ispa_std::raw_token::RawToken>;
				static auto create(long long startpos, const std::string& source, bool has_source, long long length, long long line, long long enumv, long long type_id, std::any value) -> std::shared_ptr<python::ispa_std::raw_token::RawToken>;
				virtual auto copy() -> std::shared_ptr<python::ispa_std::raw_token::RawToken>;
				virtual auto empty() -> bool;
				virtual auto isAmbiguous() -> bool;
				virtual auto meta() -> std::shared_ptr<python::ispa_std::raw_token::RawTokenMeta>;
				virtual auto kind() -> long long;
				virtual auto index() -> long long;
				virtual auto holds(long long type_id) -> bool;
				virtual auto get() -> std::any;
				virtual auto to_node(long long type_id) -> std::shared_ptr<python::ispa_std::node::Node>;
				virtual auto text() -> std::string;
				constexpr auto members() {
					return std::tie(_type_id, _payload, _meta);
				}
				constexpr auto members() const {
					return std::tie(_type_id, _payload, _meta);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 3>{{"_type_id", "_payload", "_meta"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&RawToken::__init__, &RawToken::empty_token, &RawToken::ambiguous, &RawToken::make, &RawToken::from_node, &RawToken::create, &RawToken::copy, &RawToken::empty, &RawToken::isAmbiguous, &RawToken::meta, &RawToken::kind, &RawToken::index, &RawToken::holds, &RawToken::get, &RawToken::to_node, &RawToken::text);
				}
				constexpr auto methods() const {
					return std::make_tuple(&RawToken::__init__, &RawToken::empty_token, &RawToken::ambiguous, &RawToken::make, &RawToken::from_node, &RawToken::create, &RawToken::copy, &RawToken::empty, &RawToken::isAmbiguous, &RawToken::meta, &RawToken::kind, &RawToken::index, &RawToken::holds, &RawToken::get, &RawToken::to_node, &RawToken::text);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 16>{{"__init__", "empty_token", "ambiguous", "make", "from_node", "create", "copy", "empty", "isAmbiguous", "meta", "kind", "index", "holds", "get", "to_node", "text"}};
				}
			};
		} // namespace raw_token
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace raw_token {
			auto char_at_source(std::shared_ptr<python::ispa_std::raw_token::RawTokenMeta> meta, long long offset) -> std::string;
		} // namespace raw_token
	} // namespace ispa_std
} // namespace python
namespace python::ispa_std::dfa_api { struct Action; }
namespace python::ispa_std::dfa_api { struct TdfaLayout; }
namespace python::ispa_std::dfa_api { struct DFADebug; }
namespace python::ispa_std::dfa_api { struct DebugIndex; }
namespace python::ispa_std::dfa_api { struct HistoryNode; }
namespace python::ispa_std::dfa_api { struct SourceText; }
namespace python::ispa_std::dfa_api { struct CaptureState; }
namespace python::ispa_std::dfa_api { struct Captures; }
namespace python {
	namespace ispa_std {
		namespace dfa_api {
			inline long long NULL_STATE = ( - (1));
		} // namespace dfa_api
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace dfa_api {
			inline long long LEXER_DELETE_STATE = ( - (2));
		} // namespace dfa_api
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace dfa_api {
			inline long long DYNAMIC_FAIL_STATE = ( - (3));
		} // namespace dfa_api
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace dfa_api {
			inline long long UNSET = ( - (1));
		} // namespace dfa_api
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace dfa_api {
			struct Action : public ::ISPA_STD::Object {
				static long long UNDEF;
				static long long SET;
				static long long SET_NEXT;
				static long long COPY;
				static long long APPEND;
				static long long APPEND_NEXT;
				constexpr auto members() {
					return std::tie(UNDEF, SET, SET_NEXT, COPY, APPEND, APPEND_NEXT);
				}
				constexpr auto members() const {
					return std::tie(UNDEF, SET, SET_NEXT, COPY, APPEND, APPEND_NEXT);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 6>{{"UNDEF", "SET", "SET_NEXT", "COPY", "APPEND", "APPEND_NEXT"}};
				}
				constexpr auto methods() {
					return std::make_tuple();
				}
				constexpr auto methods() const {
					return std::make_tuple();
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 0>{{}};
				}
			};
		} // namespace dfa_api
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace dfa_api {
			struct TdfaLayout : public ::ISPA_STD::Object {
				long long registers;
				long long outputs;
				auto __init__(long long registers, long long outputs) -> void;
				constexpr auto members() {
					return std::tie(registers, outputs);
				}
				constexpr auto members() const {
					return std::tie(registers, outputs);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 2>{{"registers", "outputs"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&TdfaLayout::__init__);
				}
				constexpr auto methods() const {
					return std::make_tuple(&TdfaLayout::__init__);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 1>{{"__init__"}};
				}
			};
		} // namespace dfa_api
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace dfa_api {
			auto dynamic_match(const std::string& src, long long cursor, const std::vector<std::string>& alternatives, bool negative, const std::string& quantifier) -> long long;
		} // namespace dfa_api
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace dfa_api {
			auto _starts_at(const std::string& src, long long p, const std::string& value) -> bool;
		} // namespace dfa_api
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace dfa_api {
			auto _excluded(const std::string& src, long long p, const std::vector<std::string>& alternatives) -> bool;
		} // namespace dfa_api
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace dfa_api {
			struct DFADebug : public ::ISPA_STD::Object {
				std::string member;
				std::string token_name;
				long long position_in_token;
				long long call;
				long long group;
				long long rule_run;
				long long offset;
				long long length;
				std::string ch;
				auto __init__(const std::string& member, const std::string& token_name, long long position_in_token, long long call, long long group, long long rule_run, long long offset, long long length, const std::string& ch) -> void;
				constexpr auto members() {
					return std::tie(member, token_name, position_in_token, call, group, rule_run, offset, length, ch);
				}
				constexpr auto members() const {
					return std::tie(member, token_name, position_in_token, call, group, rule_run, offset, length, ch);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 9>{{"member", "token_name", "position_in_token", "call", "group", "rule_run", "offset", "length", "ch"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&DFADebug::__init__);
				}
				constexpr auto methods() const {
					return std::make_tuple(&DFADebug::__init__);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 1>{{"__init__"}};
				}
			};
		} // namespace dfa_api
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace dfa_api {
			struct DebugIndex : public ::ISPA_STD::Object {
				std::vector<long long> _keys;
				std::vector<long long> _values;
				auto __init__() -> void;
				virtual auto put(long long state, long long cls, long long index) -> void;
				virtual auto find(long long state, long long cls) -> long long;
				constexpr auto members() {
					return std::tie(_keys, _values);
				}
				constexpr auto members() const {
					return std::tie(_keys, _values);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 2>{{"_keys", "_values"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&DebugIndex::__init__, &DebugIndex::put, &DebugIndex::find);
				}
				constexpr auto methods() const {
					return std::make_tuple(&DebugIndex::__init__, &DebugIndex::put, &DebugIndex::find);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 3>{{"__init__", "put", "find"}};
				}
			};
		} // namespace dfa_api
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace dfa_api {
			struct HistoryNode : public ::ISPA_STD::Object {
				long long pos;
				long long prev;
				auto __init__(long long pos, long long prev) -> void;
				constexpr auto members() {
					return std::tie(pos, prev);
				}
				constexpr auto members() const {
					return std::tie(pos, prev);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 2>{{"pos", "prev"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&HistoryNode::__init__);
				}
				constexpr auto methods() const {
					return std::make_tuple(&HistoryNode::__init__);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 1>{{"__init__"}};
				}
			};
		} // namespace dfa_api
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace dfa_api {
			struct SourceText : public ::ISPA_STD::Object {
				std::string text;
				auto __init__(const std::string& text) -> void;
				constexpr auto members() {
					return std::tie(text);
				}
				constexpr auto members() const {
					return std::tie(text);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 1>{{"text"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&SourceText::__init__);
				}
				constexpr auto methods() const {
					return std::make_tuple(&SourceText::__init__);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 1>{{"__init__"}};
				}
			};
		} // namespace dfa_api
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace dfa_api {
			struct CaptureState : public ::ISPA_STD::Object {
				std::shared_ptr<python::ispa_std::dfa_api::SourceText> src;
				long long base;
				std::vector<long long> regs;
				std::vector<std::shared_ptr<python::ispa_std::dfa_api::HistoryNode>> history;
				long long first_line;
				auto __init__(std::shared_ptr<python::ispa_std::dfa_api::SourceText> src) -> void;
				constexpr auto members() {
					return std::tie(src, base, regs, history, first_line);
				}
				constexpr auto members() const {
					return std::tie(src, base, regs, history, first_line);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 5>{{"src", "base", "regs", "history", "first_line"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&CaptureState::__init__);
				}
				constexpr auto methods() const {
					return std::make_tuple(&CaptureState::__init__);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 1>{{"__init__"}};
				}
			};
		} // namespace dfa_api
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace dfa_api {
			struct Captures : public ::ISPA_STD::Object {
				std::shared_ptr<python::ispa_std::dfa_api::CaptureState> _state;
				auto __init__(std::shared_ptr<python::ispa_std::dfa_api::CaptureState> state) -> void;
				virtual auto begin_off(long long k) -> long long;
				virtual auto end_off(long long k) -> long long;
				virtual auto is_set(long long k) -> bool;
				virtual auto begin(long long k) -> long long;
				virtual auto end(long long k) -> long long;
				virtual auto text(long long k) -> std::string;
				virtual auto view(long long k) -> std::string;
				virtual auto character(long long k) -> std::string;
				virtual auto list_positions(long long k) -> std::vector<long long>;
				virtual auto list_texts(long long k) -> std::vector<std::string>;
				virtual auto list_chars(long long k) -> std::vector<std::string>;
				virtual auto line(long long k) -> long long;
				virtual auto column(long long k) -> long long;
				constexpr auto members() {
					return std::tie(_state);
				}
				constexpr auto members() const {
					return std::tie(_state);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 1>{{"_state"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&Captures::__init__, &Captures::begin_off, &Captures::end_off, &Captures::is_set, &Captures::begin, &Captures::end, &Captures::text, &Captures::view, &Captures::character, &Captures::list_positions, &Captures::list_texts, &Captures::list_chars, &Captures::line, &Captures::column);
				}
				constexpr auto methods() const {
					return std::make_tuple(&Captures::__init__, &Captures::begin_off, &Captures::end_off, &Captures::is_set, &Captures::begin, &Captures::end, &Captures::text, &Captures::view, &Captures::character, &Captures::list_positions, &Captures::list_texts, &Captures::list_chars, &Captures::line, &Captures::column);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 14>{{"__init__", "begin_off", "end_off", "is_set", "begin", "end", "text", "view", "character", "list_positions", "list_texts", "list_chars", "line", "column"}};
				}
			};
		} // namespace dfa_api
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace dfa_api {
			auto semantic_result(long long state, std::shared_ptr<python::ispa_std::raw_token::RawToken> token) -> std::shared_ptr<python::ispa_std::utility::Pair<long long, std::shared_ptr<python::ispa_std::raw_token::RawToken>>>;
		} // namespace dfa_api
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace dfa_api {
			auto class_of(const std::vector<long long>& class_table, const std::string& ch) -> long long;
		} // namespace dfa_api
	} // namespace ispa_std
} // namespace python
namespace python::ispa_std::refs { struct Cursor; }
namespace python::ispa_std::refs { struct IntRef; }
namespace python {
	namespace ispa_std {
		namespace refs {
			struct Cursor : public ::ISPA_STD::Object {
				long long pos;
				long long line;
				auto __init__(long long pos, long long line) -> void;
				virtual auto copy() -> std::shared_ptr<python::ispa_std::refs::Cursor>;
				constexpr auto members() {
					return std::tie(pos, line);
				}
				constexpr auto members() const {
					return std::tie(pos, line);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 2>{{"pos", "line"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&Cursor::__init__, &Cursor::copy);
				}
				constexpr auto methods() const {
					return std::make_tuple(&Cursor::__init__, &Cursor::copy);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 2>{{"__init__", "copy"}};
				}
			};
		} // namespace refs
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace refs {
			struct IntRef : public ::ISPA_STD::Object {
				long long value;
				auto __init__(long long value) -> void;
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
					return std::make_tuple(&IntRef::__init__);
				}
				constexpr auto methods() const {
					return std::make_tuple(&IntRef::__init__);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 1>{{"__init__"}};
				}
			};
		} // namespace refs
	} // namespace ispa_std
} // namespace python
namespace python::ispa_std::dfa_run { struct DfaTables; }
namespace python::ispa_std::dfa_run { struct TraceSink; }
namespace python {
	namespace ispa_std {
		namespace dfa_run {
			struct DfaTables : public ::ISPA_STD::Object {
				std::vector<std::vector<long long>> table;
				std::vector<long long> class_table;
				std::vector<std::vector<long long>> action_table;
				std::shared_ptr<python::ispa_std::dfa_api::TdfaLayout> layout;
				std::vector<std::shared_ptr<python::ispa_std::dfa_api::DFADebug>> debug_array;
				std::shared_ptr<python::ispa_std::dfa_api::DebugIndex> debug_index;
				std::shared_ptr<python::ispa_std::dfa_api::CaptureState> state;
				std::shared_ptr<python::ispa_std::dfa_api::Captures> captures;
				auto __init__(const std::vector<std::vector<long long>>& table, const std::vector<long long>& class_table, const std::vector<std::vector<long long>>& action_table, std::shared_ptr<python::ispa_std::dfa_api::TdfaLayout> layout, const std::vector<std::shared_ptr<python::ispa_std::dfa_api::DFADebug>>& debug_array, std::shared_ptr<python::ispa_std::dfa_api::DebugIndex> debug_index) -> void;
				constexpr auto members() {
					return std::tie(table, class_table, action_table, layout, debug_array, debug_index, state, captures);
				}
				constexpr auto members() const {
					return std::tie(table, class_table, action_table, layout, debug_array, debug_index, state, captures);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 8>{{"table", "class_table", "action_table", "layout", "debug_array", "debug_index", "state", "captures"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&DfaTables::__init__);
				}
				constexpr auto methods() const {
					return std::make_tuple(&DfaTables::__init__);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 1>{{"__init__"}};
				}
			};
		} // namespace dfa_run
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace dfa_run {
			struct TraceSink : public ::ISPA_STD::Object {
				virtual auto write(const std::string& text) -> void;
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
					return std::make_tuple(&TraceSink::write);
				}
				constexpr auto methods() const {
					return std::make_tuple(&TraceSink::write);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 1>{{"write"}};
				}
			};
		} // namespace dfa_run
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace dfa_run {
			inline std::vector<std::shared_ptr<python::ispa_std::dfa_run::TraceSink>> _TRACE = std::vector<std::shared_ptr<python::ispa_std::dfa_run::TraceSink>>{ISPA_STD::construct<python::ispa_std::dfa_run::TraceSink>()};
		} // namespace dfa_run
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace dfa_run {
			inline std::vector<bool> _TRACE_ON = std::vector<bool>{false};
		} // namespace dfa_run
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace dfa_run {
			auto set_trace_sink(std::shared_ptr<python::ispa_std::dfa_run::TraceSink> sink) -> void;
		} // namespace dfa_run
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace dfa_run {
			auto clear_trace_sink() -> void;
		} // namespace dfa_run
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace dfa_run {
			auto _trace_step(std::shared_ptr<python::ispa_std::dfa_run::DfaTables> tables, long long state, long long cls, const std::string& current_ch, const std::string& pos_ch) -> void;
		} // namespace dfa_run
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace dfa_run {
			auto run(std::shared_ptr<python::ispa_std::dfa_api::SourceText> source, std::shared_ptr<python::ispa_std::refs::Cursor> cur, long long start_pos, std::shared_ptr<python::ispa_std::dfa_run::DfaTables> tables, std::function<std::shared_ptr<python::ispa_std::utility::Pair<long long, std::shared_ptr<python::ispa_std::raw_token::RawToken>>>(long long, std::shared_ptr<python::ispa_std::dfa_api::Captures>, long long, long long, long long, std::shared_ptr<python::ispa_std::refs::Cursor>)> semantic, std::shared_ptr<python::ispa_std::raw_token::RawToken> empty_token) -> std::shared_ptr<python::ispa_std::raw_token::RawToken>;
		} // namespace dfa_run
	} // namespace ispa_std
} // namespace python
namespace python::ispa_std::lexer { struct LexerDecision; }
namespace python::ispa_std::lexer { struct LexerFailureContext; }
namespace python::ispa_std::lexer { struct _SharedState; }
namespace python::ispa_std::lexer { struct Lexer_base; }
namespace python::ispa_std::lexer { struct TokenCursor; }
namespace python::ispa_std::lexer { struct LazyIterator; }
namespace python::ispa_std::lexer { struct TokenIterator; }
namespace python {
	namespace ispa_std {
		namespace lexer {
			struct LexerDecision : public ::ISPA_STD::Object {
				static long long NONE;
				static long long DELETE;
				static long long INSERT;
				static long long ABORT;
				constexpr auto members() {
					return std::tie(NONE, DELETE, INSERT, ABORT);
				}
				constexpr auto members() const {
					return std::tie(NONE, DELETE, INSERT, ABORT);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 4>{{"NONE", "DELETE", "INSERT", "ABORT"}};
				}
				constexpr auto methods() {
					return std::make_tuple();
				}
				constexpr auto methods() const {
					return std::make_tuple();
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 0>{{}};
				}
			};
		} // namespace lexer
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lexer {
			struct LexerFailureContext : public ::ISPA_STD::Object {
				std::string src;
				long long cursor;
				std::vector<std::string> diagnostics;
				std::string rule;
				std::string failed_rule;
				long long decision;
				auto __init__(const std::string& src, long long cursor, const std::vector<std::string>& diagnostics, const std::string& rule, const std::string& failed_rule) -> void;
				virtual auto at(const std::string& value, long long offset) -> bool;
				virtual auto error(const std::string& message) -> void;
				virtual auto erase() -> void;
				virtual auto skip() -> void;
				virtual auto insert() -> void;
				virtual auto abort() -> void;
				constexpr auto members() {
					return std::tie(src, cursor, diagnostics, rule, failed_rule, decision);
				}
				constexpr auto members() const {
					return std::tie(src, cursor, diagnostics, rule, failed_rule, decision);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 6>{{"src", "cursor", "diagnostics", "rule", "failed_rule", "decision"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&LexerFailureContext::__init__, &LexerFailureContext::at, &LexerFailureContext::error, &LexerFailureContext::erase, &LexerFailureContext::skip, &LexerFailureContext::insert, &LexerFailureContext::abort);
				}
				constexpr auto methods() const {
					return std::make_tuple(&LexerFailureContext::__init__, &LexerFailureContext::at, &LexerFailureContext::error, &LexerFailureContext::erase, &LexerFailureContext::skip, &LexerFailureContext::insert, &LexerFailureContext::abort);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 7>{{"__init__", "at", "error", "erase", "skip", "insert", "abort"}};
				}
			};
		} // namespace lexer
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lexer {
			auto is_token_end(std::shared_ptr<python::ispa_std::raw_token::RawToken> token) -> bool;
		} // namespace lexer
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lexer {
			struct _SharedState : public ::ISPA_STD::Object {
				long long pos;
				bool eof_processed;
				std::vector<std::any> checkpoints;
				auto __init__(long long pos) -> void;
				constexpr auto members() {
					return std::tie(pos, eof_processed, checkpoints);
				}
				constexpr auto members() const {
					return std::tie(pos, eof_processed, checkpoints);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 3>{{"pos", "eof_processed", "checkpoints"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&_SharedState::__init__);
				}
				constexpr auto methods() const {
					return std::make_tuple(&_SharedState::__init__);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 1>{{"__init__"}};
				}
			};
		} // namespace lexer
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lexer {
			struct Lexer_base : public ::ISPA_STD::Object {
				std::shared_ptr<python::ispa_std::dfa_api::SourceText> _src;
				bool _has_input;
				std::vector<std::shared_ptr<python::ispa_std::raw_token::RawToken>> tokens;
				std::vector<std::string> errors;
				long long line;
				bool external_boundary;
				long long bound;
				auto __init__() -> void;
				virtual auto makeToken(std::shared_ptr<python::ispa_std::refs::Cursor> cur) -> std::shared_ptr<python::ispa_std::raw_token::RawToken>;
				virtual auto init() -> void;
				virtual auto empty_token() -> std::shared_ptr<python::ispa_std::raw_token::RawToken>;
				virtual auto ambiguous_token(long long before, long long after) -> std::shared_ptr<python::ispa_std::raw_token::RawToken>;
				virtual auto clone() -> std::shared_ptr<python::ispa_std::lexer::Lexer_base>;
				virtual auto checkpoint() -> std::any;
				virtual auto restoreCheckpoint(std::any state) -> void;
				virtual auto externalAdvance(long long begin, long long end) -> void;
				virtual auto hasExternalRules() -> bool;
				virtual auto on_token(std::shared_ptr<python::ispa_std::raw_token::RawToken> token) -> void;
				virtual auto on_eof() -> void;
				virtual auto push(std::shared_ptr<python::ispa_std::raw_token::RawToken> token) -> void;
				virtual auto push_eof() -> void;
				virtual auto append_token(std::shared_ptr<python::ispa_std::raw_token::RawToken> token) -> void;
				virtual auto has_buffered_tokens() -> bool;
				virtual auto pop_buffered_token() -> std::shared_ptr<python::ispa_std::raw_token::RawToken>;
				virtual auto setExternalBoundary(bool enabled) -> void;
				virtual auto set_bound(long long end) -> void;
				virtual auto hasInput() -> bool;
				virtual auto hasTokens() -> bool;
				virtual auto source() -> std::string;
				virtual auto setInput(const std::string& text) -> std::shared_ptr<python::ispa_std::lexer::Lexer_base>;
				virtual auto getTokens() -> const std::vector<std::shared_ptr<python::ispa_std::raw_token::RawToken>>&;
				virtual auto clearTokens() -> void;
				virtual auto getErrors() -> const std::vector<std::string>&;
				virtual auto copy_base_into(std::shared_ptr<python::ispa_std::lexer::Lexer_base> other) -> void;
				virtual auto assign_from(std::shared_ptr<python::ispa_std::lexer::Lexer_base> other) -> void;
				virtual auto equals(std::shared_ptr<python::ispa_std::lexer::Lexer_base> other) -> bool;
				virtual auto getCurrentPos(long long pos) -> long long;
				virtual auto skip_spaces(std::shared_ptr<python::ispa_std::refs::Cursor> cur) -> long long;
				virtual auto line_at(long long pos) -> long long;
				virtual auto column_at(long long pos) -> long long;
				virtual auto line_prefix(long long pos) -> std::string;
				virtual auto next_char(long long pos) -> std::string;
				virtual auto panic_mode(std::shared_ptr<python::ispa_std::refs::Cursor> cur) -> void;
				virtual auto lexer_failure_context(long long cursor, const std::string& rule, const std::string& failed_rule) -> std::shared_ptr<python::ispa_std::lexer::LexerFailureContext>;
				virtual auto lexer_failure_result(std::shared_ptr<python::ispa_std::lexer::LexerFailureContext> ctx, std::shared_ptr<python::ispa_std::raw_token::RawToken> inserted) -> std::shared_ptr<python::ispa_std::utility::Pair<long long, std::shared_ptr<python::ispa_std::raw_token::RawToken>>>;
				virtual auto lookup(std::shared_ptr<python::ispa_std::dfa_run::DfaTables> tables, std::function<std::shared_ptr<python::ispa_std::utility::Pair<long long, std::shared_ptr<python::ispa_std::raw_token::RawToken>>>(long long, std::shared_ptr<python::ispa_std::dfa_api::Captures>, long long, long long, long long, std::shared_ptr<python::ispa_std::refs::Cursor>)> semantic, std::shared_ptr<python::ispa_std::refs::Cursor> cur) -> std::shared_ptr<python::ispa_std::raw_token::RawToken>;
				virtual auto makeTokens() -> std::vector<std::shared_ptr<python::ispa_std::raw_token::RawToken>>;
				virtual auto makeTokensFromText(const std::string& text) -> std::vector<std::shared_ptr<python::ispa_std::raw_token::RawToken>>;
				constexpr auto members() {
					return std::tie(_src, _has_input, tokens, errors, line, external_boundary, bound);
				}
				constexpr auto members() const {
					return std::tie(_src, _has_input, tokens, errors, line, external_boundary, bound);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 7>{{"_src", "_has_input", "tokens", "errors", "line", "external_boundary", "bound"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&Lexer_base::__init__, &Lexer_base::makeToken, &Lexer_base::init, &Lexer_base::empty_token, &Lexer_base::ambiguous_token, &Lexer_base::clone, &Lexer_base::checkpoint, &Lexer_base::restoreCheckpoint, &Lexer_base::externalAdvance, &Lexer_base::hasExternalRules, &Lexer_base::on_token, &Lexer_base::on_eof, &Lexer_base::push, &Lexer_base::push_eof, &Lexer_base::append_token, &Lexer_base::has_buffered_tokens, &Lexer_base::pop_buffered_token, &Lexer_base::setExternalBoundary, &Lexer_base::set_bound, &Lexer_base::hasInput, &Lexer_base::hasTokens, &Lexer_base::source, &Lexer_base::setInput, &Lexer_base::getTokens, &Lexer_base::clearTokens, &Lexer_base::getErrors, &Lexer_base::copy_base_into, &Lexer_base::assign_from, &Lexer_base::equals, &Lexer_base::getCurrentPos, &Lexer_base::skip_spaces, &Lexer_base::line_at, &Lexer_base::column_at, &Lexer_base::line_prefix, &Lexer_base::next_char, &Lexer_base::panic_mode, &Lexer_base::lexer_failure_context, &Lexer_base::lexer_failure_result, &Lexer_base::lookup, &Lexer_base::makeTokens, &Lexer_base::makeTokensFromText);
				}
				constexpr auto methods() const {
					return std::make_tuple(&Lexer_base::__init__, &Lexer_base::makeToken, &Lexer_base::init, &Lexer_base::empty_token, &Lexer_base::ambiguous_token, &Lexer_base::clone, &Lexer_base::checkpoint, &Lexer_base::restoreCheckpoint, &Lexer_base::externalAdvance, &Lexer_base::hasExternalRules, &Lexer_base::on_token, &Lexer_base::on_eof, &Lexer_base::push, &Lexer_base::push_eof, &Lexer_base::append_token, &Lexer_base::has_buffered_tokens, &Lexer_base::pop_buffered_token, &Lexer_base::setExternalBoundary, &Lexer_base::set_bound, &Lexer_base::hasInput, &Lexer_base::hasTokens, &Lexer_base::source, &Lexer_base::setInput, &Lexer_base::getTokens, &Lexer_base::clearTokens, &Lexer_base::getErrors, &Lexer_base::copy_base_into, &Lexer_base::assign_from, &Lexer_base::equals, &Lexer_base::getCurrentPos, &Lexer_base::skip_spaces, &Lexer_base::line_at, &Lexer_base::column_at, &Lexer_base::line_prefix, &Lexer_base::next_char, &Lexer_base::panic_mode, &Lexer_base::lexer_failure_context, &Lexer_base::lexer_failure_result, &Lexer_base::lookup, &Lexer_base::makeTokens, &Lexer_base::makeTokensFromText);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 41>{{"__init__", "makeToken", "init", "empty_token", "ambiguous_token", "clone", "checkpoint", "restoreCheckpoint", "externalAdvance", "hasExternalRules", "on_token", "on_eof", "push", "push_eof", "append_token", "has_buffered_tokens", "pop_buffered_token", "setExternalBoundary", "set_bound", "hasInput", "hasTokens", "source", "setInput", "getTokens", "clearTokens", "getErrors", "copy_base_into", "assign_from", "equals", "getCurrentPos", "skip_spaces", "line_at", "column_at", "line_prefix", "next_char", "panic_mode", "lexer_failure_context", "lexer_failure_result", "lookup", "makeTokens", "makeTokensFromText"}};
				}
			};
		} // namespace lexer
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lexer {
			auto _token_equal(std::shared_ptr<python::ispa_std::raw_token::RawToken> a, std::shared_ptr<python::ispa_std::raw_token::RawToken> b) -> bool;
		} // namespace lexer
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lexer {
			struct TokenCursor : public ::ISPA_STD::Object {
				std::shared_ptr<python::ispa_std::lexer::Lexer_base> owner;
				std::shared_ptr<python::ispa_std::lexer::_SharedState> state;
				long long token_index;
				long long distance_base;
				std::shared_ptr<python::ispa_std::lexer::Lexer_base> owned_owner;
				bool has_owned;
				auto __init__() -> void;
				virtual auto copy() -> std::shared_ptr<python::ispa_std::lexer::TokenCursor>;
				virtual auto assign(std::shared_ptr<python::ispa_std::lexer::TokenCursor> other) -> void;
				virtual auto has_source() -> bool;
				virtual auto sourceOrigin() -> std::string;
				virtual auto getErrors() -> std::vector<std::string>;
				virtual auto sourcePosition() -> long long;
				virtual auto consumeExternal(long long end) -> void;
				virtual auto isEnd() -> bool;
				virtual auto next() -> std::shared_ptr<python::ispa_std::lexer::TokenCursor>;
				virtual auto prev() -> std::shared_ptr<python::ispa_std::lexer::TokenCursor>;
				virtual auto advance_by(long long count) -> std::shared_ptr<python::ispa_std::lexer::TokenCursor>;
				virtual auto plus(long long count) -> std::shared_ptr<python::ispa_std::lexer::TokenCursor>;
				virtual auto current() -> std::shared_ptr<python::ispa_std::raw_token::RawToken>;
				virtual auto minus(std::shared_ptr<python::ispa_std::lexer::TokenCursor> other) -> long long;
				virtual auto distance() -> long long;
				virtual auto resumePosition() -> long long;
				virtual auto advanceCharacters(long long count) -> bool;
				constexpr auto members() {
					return std::tie(owner, state, token_index, distance_base, owned_owner, has_owned);
				}
				constexpr auto members() const {
					return std::tie(owner, state, token_index, distance_base, owned_owner, has_owned);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 6>{{"owner", "state", "token_index", "distance_base", "owned_owner", "has_owned"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&TokenCursor::__init__, &TokenCursor::copy, &TokenCursor::assign, &TokenCursor::has_source, &TokenCursor::sourceOrigin, &TokenCursor::getErrors, &TokenCursor::sourcePosition, &TokenCursor::consumeExternal, &TokenCursor::isEnd, &TokenCursor::next, &TokenCursor::prev, &TokenCursor::advance_by, &TokenCursor::plus, &TokenCursor::current, &TokenCursor::minus, &TokenCursor::distance, &TokenCursor::resumePosition, &TokenCursor::advanceCharacters);
				}
				constexpr auto methods() const {
					return std::make_tuple(&TokenCursor::__init__, &TokenCursor::copy, &TokenCursor::assign, &TokenCursor::has_source, &TokenCursor::sourceOrigin, &TokenCursor::getErrors, &TokenCursor::sourcePosition, &TokenCursor::consumeExternal, &TokenCursor::isEnd, &TokenCursor::next, &TokenCursor::prev, &TokenCursor::advance_by, &TokenCursor::plus, &TokenCursor::current, &TokenCursor::minus, &TokenCursor::distance, &TokenCursor::resumePosition, &TokenCursor::advanceCharacters);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 18>{{"__init__", "copy", "assign", "has_source", "sourceOrigin", "getErrors", "sourcePosition", "consumeExternal", "isEnd", "next", "prev", "advance_by", "plus", "current", "minus", "distance", "resumePosition", "advanceCharacters"}};
				}
			};
		} // namespace lexer
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lexer {
			struct LazyIterator : public python::ispa_std::lexer::TokenCursor {
				std::shared_ptr<python::ispa_std::lexer::Lexer_base> owner;
				std::shared_ptr<python::ispa_std::lexer::_SharedState> state;
				long long token_index;
				long long distance_base;
				std::shared_ptr<python::ispa_std::lexer::Lexer_base> owned_owner;
				bool has_owned;
				auto __init__(std::shared_ptr<python::ispa_std::lexer::Lexer_base> lexer, std::shared_ptr<python::ispa_std::lexer::_SharedState> state, long long token_index, long long distance_base) -> void;
				static auto begin(std::shared_ptr<python::ispa_std::lexer::Lexer_base> lexer) -> std::shared_ptr<python::ispa_std::lexer::LazyIterator>;
				static auto at_position(std::shared_ptr<python::ispa_std::lexer::Lexer_base> lexer, long long start) -> std::shared_ptr<python::ispa_std::lexer::LazyIterator>;
				virtual auto copy() -> std::shared_ptr<python::ispa_std::lexer::TokenCursor>;
				virtual auto has_source() -> bool;
				virtual auto ensure_token() -> void;
				virtual auto sourceOrigin() -> std::string;
				virtual auto getErrors() -> std::vector<std::string>;
				virtual auto sourcePosition() -> long long;
				virtual auto consumeExternal(long long end) -> void;
				virtual auto isEnd() -> bool;
				virtual auto next() -> std::shared_ptr<python::ispa_std::lexer::TokenCursor>;
				virtual auto prev() -> std::shared_ptr<python::ispa_std::lexer::TokenCursor>;
				virtual auto advance_by(long long count) -> std::shared_ptr<python::ispa_std::lexer::TokenCursor>;
				virtual auto current() -> std::shared_ptr<python::ispa_std::raw_token::RawToken>;
				virtual auto resumePosition() -> long long;
				virtual auto advanceCharacters(long long count) -> bool;
				constexpr auto members() {
					return std::tie(owner, state, token_index, distance_base, owned_owner, has_owned);
				}
				constexpr auto members() const {
					return std::tie(owner, state, token_index, distance_base, owned_owner, has_owned);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 6>{{"owner", "state", "token_index", "distance_base", "owned_owner", "has_owned"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&LazyIterator::__init__, &LazyIterator::begin, &LazyIterator::at_position, &LazyIterator::copy, &LazyIterator::has_source, &LazyIterator::ensure_token, &LazyIterator::sourceOrigin, &LazyIterator::getErrors, &LazyIterator::sourcePosition, &LazyIterator::consumeExternal, &LazyIterator::isEnd, &LazyIterator::next, &LazyIterator::prev, &LazyIterator::advance_by, &LazyIterator::current, &LazyIterator::resumePosition, &LazyIterator::advanceCharacters);
				}
				constexpr auto methods() const {
					return std::make_tuple(&LazyIterator::__init__, &LazyIterator::begin, &LazyIterator::at_position, &LazyIterator::copy, &LazyIterator::has_source, &LazyIterator::ensure_token, &LazyIterator::sourceOrigin, &LazyIterator::getErrors, &LazyIterator::sourcePosition, &LazyIterator::consumeExternal, &LazyIterator::isEnd, &LazyIterator::next, &LazyIterator::prev, &LazyIterator::advance_by, &LazyIterator::current, &LazyIterator::resumePosition, &LazyIterator::advanceCharacters);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 17>{{"__init__", "begin", "at_position", "copy", "has_source", "ensure_token", "sourceOrigin", "getErrors", "sourcePosition", "consumeExternal", "isEnd", "next", "prev", "advance_by", "current", "resumePosition", "advanceCharacters"}};
				}
			};
		} // namespace lexer
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lexer {
			struct TokenIterator : public python::ispa_std::lexer::TokenCursor {
				std::shared_ptr<python::ispa_std::lexer::Lexer_base> owner;
				long long token_index;
				long long distance_base;
				std::shared_ptr<python::ispa_std::lexer::Lexer_base> owned_owner;
				bool has_owned;
				auto __init__(std::shared_ptr<python::ispa_std::lexer::Lexer_base> owner, long long index) -> void;
				static auto begin(std::shared_ptr<python::ispa_std::lexer::Lexer_base> owner) -> std::shared_ptr<python::ispa_std::lexer::TokenCursor>;
				virtual auto copy() -> std::shared_ptr<python::ispa_std::lexer::TokenCursor>;
				virtual auto next() -> std::shared_ptr<python::ispa_std::lexer::TokenCursor>;
				virtual auto prev() -> std::shared_ptr<python::ispa_std::lexer::TokenCursor>;
				virtual auto advance_by(long long count) -> std::shared_ptr<python::ispa_std::lexer::TokenCursor>;
				virtual auto isEnd() -> bool;
				virtual auto current() -> std::shared_ptr<python::ispa_std::raw_token::RawToken>;
				virtual auto resumePosition() -> long long;
				constexpr auto members() {
					return std::tie(owner, token_index, distance_base, owned_owner, has_owned);
				}
				constexpr auto members() const {
					return std::tie(owner, token_index, distance_base, owned_owner, has_owned);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 5>{{"owner", "token_index", "distance_base", "owned_owner", "has_owned"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&TokenIterator::__init__, &TokenIterator::begin, &TokenIterator::copy, &TokenIterator::next, &TokenIterator::prev, &TokenIterator::advance_by, &TokenIterator::isEnd, &TokenIterator::current, &TokenIterator::resumePosition);
				}
				constexpr auto methods() const {
					return std::make_tuple(&TokenIterator::__init__, &TokenIterator::begin, &TokenIterator::copy, &TokenIterator::next, &TokenIterator::prev, &TokenIterator::advance_by, &TokenIterator::isEnd, &TokenIterator::current, &TokenIterator::resumePosition);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 9>{{"__init__", "begin", "copy", "next", "prev", "advance_by", "isEnd", "current", "resumePosition"}};
				}
			};
		} // namespace lexer
	} // namespace ispa_std
} // namespace python
namespace python::ispa_std::recovery { struct Mode; }
namespace python::ispa_std::recovery { struct Decision; }
namespace python::ispa_std::recovery { struct Diagnostic; }
namespace python::ispa_std::recovery { struct State; }
namespace python::ispa_std::recovery { struct Plan; }
namespace python::ispa_std::recovery { struct TokenSet; }
namespace python::ispa_std::recovery { struct Policy; }
namespace python::ispa_std::recovery { struct FailHandler; }
namespace python::ispa_std::recovery { struct Handler; }
namespace python::ispa_std::recovery { struct ActionFailure; }
namespace python::ispa_std::recovery { struct LegacyFail; }
namespace python::ispa_std::recovery { struct RequiredStatus; }
namespace python::ispa_std::recovery { struct RequiredResult; }
namespace python::ispa_std::recovery { struct ContextKind; }
namespace python::ispa_std::recovery { struct ContextClaim; }
namespace python::ispa_std::recovery { struct ContextDiagnostic; }
namespace python {
	namespace ispa_std {
		namespace recovery {
			inline long long EOF_TOKEN = ( - (1));
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			inline long long END_OF_INPUT = python::ispa_std::recovery::EOF_TOKEN;
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			inline long long NO_OFFSET = ( - (1));
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			struct Mode : public ::ISPA_STD::Object {
				static long long PanicUntil;
				static long long Insert;
				static long long Delete;
				static long long Replace;
				static long long Retry;
				static long long Synchronize;
				static long long Propagate;
				static long long Abort;
				constexpr auto members() {
					return std::tie(PanicUntil, Insert, Delete, Replace, Retry, Synchronize, Propagate, Abort);
				}
				constexpr auto members() const {
					return std::tie(PanicUntil, Insert, Delete, Replace, Retry, Synchronize, Propagate, Abort);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 8>{{"PanicUntil", "Insert", "Delete", "Replace", "Retry", "Synchronize", "Propagate", "Abort"}};
				}
				constexpr auto methods() {
					return std::make_tuple();
				}
				constexpr auto methods() const {
					return std::make_tuple();
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 0>{{}};
				}
			};
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			struct Decision : public ::ISPA_STD::Object {
				long long mode;
				long long synthesized;
				long long unexpected;
				long long skipped;
				auto __init__(long long mode, long long synthesized, long long unexpected, long long skipped) -> void;
				static auto initial() -> std::shared_ptr<python::ispa_std::recovery::Decision>;
				virtual auto copy() -> std::shared_ptr<python::ispa_std::recovery::Decision>;
				constexpr auto members() {
					return std::tie(mode, synthesized, unexpected, skipped);
				}
				constexpr auto members() const {
					return std::tie(mode, synthesized, unexpected, skipped);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 4>{{"mode", "synthesized", "unexpected", "skipped"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&Decision::__init__, &Decision::initial, &Decision::copy);
				}
				constexpr auto methods() const {
					return std::make_tuple(&Decision::__init__, &Decision::initial, &Decision::copy);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 3>{{"__init__", "initial", "copy"}};
				}
			};
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			struct Diagnostic : public ::ISPA_STD::Object {
				std::string rule;
				long long unexpected;
				std::vector<long long> expected;
				std::shared_ptr<python::ispa_std::recovery::Decision> decision;
				std::string expected_text;
				long long offset;
				long long length;
				std::string message;
				std::string code;
				std::vector<std::string> notes;
				auto __init__() -> void;
				constexpr auto members() {
					return std::tie(rule, unexpected, expected, decision, expected_text, offset, length, message, code, notes);
				}
				constexpr auto members() const {
					return std::tie(rule, unexpected, expected, decision, expected_text, offset, length, message, code, notes);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 10>{{"rule", "unexpected", "expected", "decision", "expected_text", "offset", "length", "message", "code", "notes"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&Diagnostic::__init__);
				}
				constexpr auto methods() const {
					return std::make_tuple(&Diagnostic::__init__);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 1>{{"__init__"}};
				}
			};
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			struct State : public ::ISPA_STD::Object {
				long long max_errors;
				long long max_total_skipped;
				long long total_skipped;
				std::vector<std::shared_ptr<python::ispa_std::recovery::Diagnostic>> diagnostics;
				std::vector<std::vector<long long>> anchors;
				long long anchor_count;
				auto __init__() -> void;
				virtual auto exhausted() -> bool;
				constexpr auto members() {
					return std::tie(max_errors, max_total_skipped, total_skipped, diagnostics, anchors, anchor_count);
				}
				constexpr auto members() const {
					return std::tie(max_errors, max_total_skipped, total_skipped, diagnostics, anchors, anchor_count);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 6>{{"max_errors", "max_total_skipped", "total_skipped", "diagnostics", "anchors", "anchor_count"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&State::__init__, &State::exhausted);
				}
				constexpr auto methods() const {
					return std::make_tuple(&State::__init__, &State::exhausted);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 2>{{"__init__", "exhausted"}};
				}
			};
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			struct Plan : public ::ISPA_STD::Object {
				std::string rule;
				std::vector<long long> expected;
				std::vector<long long> continuation;
				std::vector<long long> follow;
				std::vector<long long> explicit_sync;
				bool allow_insertion;
				bool allow_deletion;
				bool allow_replacement;
				long long max_skip;
				auto __init__(const std::string& rule, const std::vector<long long>& expected, const std::vector<long long>& continuation, const std::vector<long long>& follow, const std::vector<long long>& explicit_sync, bool allow_insertion, bool allow_deletion, bool allow_replacement, long long max_skip) -> void;
				constexpr auto members() {
					return std::tie(rule, expected, continuation, follow, explicit_sync, allow_insertion, allow_deletion, allow_replacement, max_skip);
				}
				constexpr auto members() const {
					return std::tie(rule, expected, continuation, follow, explicit_sync, allow_insertion, allow_deletion, allow_replacement, max_skip);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 9>{{"rule", "expected", "continuation", "follow", "explicit_sync", "allow_insertion", "allow_deletion", "allow_replacement", "max_skip"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&Plan::__init__);
				}
				constexpr auto methods() const {
					return std::make_tuple(&Plan::__init__);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 1>{{"__init__"}};
				}
			};
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			auto contains(const std::vector<long long>& tokens, long long id) -> bool;
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			auto decode(const std::string& encoded) -> std::vector<long long>;
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			auto _digit(const std::string& c) -> long long;
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			auto token_id(std::shared_ptr<python::ispa_std::raw_token::RawToken> token) -> long long;
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			auto _kind(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> long long;
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			auto recover(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::shared_ptr<python::ispa_std::recovery::State> state, std::shared_ptr<python::ispa_std::recovery::Plan> plan, std::function<long long(std::shared_ptr<python::ispa_std::raw_token::RawToken>)> token_kind) -> std::shared_ptr<python::ispa_std::recovery::Decision>;
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			auto _anchor(std::shared_ptr<python::ispa_std::recovery::Plan> plan, std::shared_ptr<python::ispa_std::recovery::State> state, long long id) -> bool;
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			auto recover_default(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::shared_ptr<python::ispa_std::recovery::State> state, std::shared_ptr<python::ispa_std::recovery::Plan> plan) -> std::shared_ptr<python::ispa_std::recovery::Decision>;
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			auto recover_encoded(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::shared_ptr<python::ispa_std::recovery::State> state, const std::string& rule, const std::string& expected, const std::string& continuation, const std::string& follow, const std::string& explicit_sync, bool allow_insertion, bool allow_deletion, bool allow_replacement, long long max_skip) -> std::shared_ptr<python::ispa_std::recovery::Decision>;
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			struct TokenSet : public ::ISPA_STD::Object {
				std::vector<long long> _values;
				auto __init__(const std::vector<long long>& values) -> void;
				virtual auto values() -> const std::vector<long long>&;
				virtual auto matches(std::shared_ptr<python::ispa_std::raw_token::RawToken> token) -> bool;
				constexpr auto members() {
					return std::tie(_values);
				}
				constexpr auto members() const {
					return std::tie(_values);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 1>{{"_values"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&TokenSet::__init__, &TokenSet::values, &TokenSet::matches);
				}
				constexpr auto methods() const {
					return std::make_tuple(&TokenSet::__init__, &TokenSet::values, &TokenSet::matches);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 3>{{"__init__", "values", "matches"}};
				}
			};
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			auto empty_token_set() -> std::shared_ptr<python::ispa_std::recovery::TokenSet>;
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			struct Policy : public ::ISPA_STD::Object {
				bool allow_insertion;
				bool allow_deletion;
				bool allow_replacement;
				long long max_skip;
				auto __init__(bool allow_insertion, bool allow_deletion, bool allow_replacement, long long max_skip) -> void;
				static auto initial() -> std::shared_ptr<python::ispa_std::recovery::Policy>;
				constexpr auto members() {
					return std::tie(allow_insertion, allow_deletion, allow_replacement, max_skip);
				}
				constexpr auto members() const {
					return std::tie(allow_insertion, allow_deletion, allow_replacement, max_skip);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 4>{{"allow_insertion", "allow_deletion", "allow_replacement", "max_skip"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&Policy::__init__, &Policy::initial);
				}
				constexpr auto methods() const {
					return std::make_tuple(&Policy::__init__, &Policy::initial);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 2>{{"__init__", "initial"}};
				}
			};
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			struct FailHandler : public ::ISPA_STD::Object {
				virtual auto run(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::shared_ptr<python::ispa_std::recovery::State> state) -> std::shared_ptr<python::ispa_std::recovery::Decision>;
				virtual auto is_custom() -> bool;
				virtual auto is_recovering() -> bool;
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
					return std::make_tuple(&FailHandler::run, &FailHandler::is_custom, &FailHandler::is_recovering);
				}
				constexpr auto methods() const {
					return std::make_tuple(&FailHandler::run, &FailHandler::is_custom, &FailHandler::is_recovering);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 3>{{"run", "is_custom", "is_recovering"}};
				}
			};
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			struct Handler : public python::ispa_std::recovery::FailHandler {
				std::string rule;
				std::string expected_text;
				std::shared_ptr<python::ispa_std::recovery::TokenSet> expected;
				std::shared_ptr<python::ispa_std::recovery::TokenSet> continuation;
				std::shared_ptr<python::ispa_std::recovery::TokenSet> follow;
				std::shared_ptr<python::ispa_std::recovery::TokenSet> synchronization;
				std::shared_ptr<python::ispa_std::recovery::Policy> policy;
				auto __init__(const std::string& rule, const std::string& expected_text, std::shared_ptr<python::ispa_std::recovery::TokenSet> expected, std::shared_ptr<python::ispa_std::recovery::TokenSet> continuation, std::shared_ptr<python::ispa_std::recovery::TokenSet> follow, std::shared_ptr<python::ispa_std::recovery::TokenSet> synchronization, std::shared_ptr<python::ispa_std::recovery::Policy> policy) -> void;
				virtual auto run(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::shared_ptr<python::ispa_std::recovery::State> state) -> std::shared_ptr<python::ispa_std::recovery::Decision>;
				constexpr auto members() {
					return std::tie(rule, expected_text, expected, continuation, follow, synchronization, policy);
				}
				constexpr auto members() const {
					return std::tie(rule, expected_text, expected, continuation, follow, synchronization, policy);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 7>{{"rule", "expected_text", "expected", "continuation", "follow", "synchronization", "policy"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&Handler::__init__, &Handler::run);
				}
				constexpr auto methods() const {
					return std::make_tuple(&Handler::__init__, &Handler::run);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 2>{{"__init__", "run"}};
				}
			};
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			struct ActionFailure : public python::ispa_std::recovery::FailHandler {
				std::function<std::shared_ptr<python::ispa_std::recovery::Decision>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>, std::shared_ptr<python::ispa_std::recovery::State>)> action;
				auto __init__(std::function<std::shared_ptr<python::ispa_std::recovery::Decision>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>, std::shared_ptr<python::ispa_std::recovery::State>)> action) -> void;
				virtual auto run(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::shared_ptr<python::ispa_std::recovery::State> state) -> std::shared_ptr<python::ispa_std::recovery::Decision>;
				virtual auto is_custom() -> bool;
				constexpr auto members() {
					return std::tie(action);
				}
				constexpr auto members() const {
					return std::tie(action);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 1>{{"action"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&ActionFailure::__init__, &ActionFailure::run, &ActionFailure::is_custom);
				}
				constexpr auto methods() const {
					return std::make_tuple(&ActionFailure::__init__, &ActionFailure::run, &ActionFailure::is_custom);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 3>{{"__init__", "run", "is_custom"}};
				}
			};
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			struct LegacyFail : public python::ispa_std::recovery::FailHandler {
				std::function<void(std::shared_ptr<python::ispa_std::lexer::TokenCursor>)> action;
				auto __init__(std::function<void(std::shared_ptr<python::ispa_std::lexer::TokenCursor>)> action) -> void;
				virtual auto run(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::shared_ptr<python::ispa_std::recovery::State> state) -> std::shared_ptr<python::ispa_std::recovery::Decision>;
				virtual auto is_recovering() -> bool;
				constexpr auto members() {
					return std::tie(action);
				}
				constexpr auto members() const {
					return std::tie(action);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 1>{{"action"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&LegacyFail::__init__, &LegacyFail::run, &LegacyFail::is_recovering);
				}
				constexpr auto methods() const {
					return std::make_tuple(&LegacyFail::__init__, &LegacyFail::run, &LegacyFail::is_recovering);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 3>{{"__init__", "run", "is_recovering"}};
				}
			};
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			struct RequiredStatus : public ::ISPA_STD::Object {
				static long long Matched;
				static long long Missing;
				static long long Boundary;
				static long long Aborted;
				constexpr auto members() {
					return std::tie(Matched, Missing, Boundary, Aborted);
				}
				constexpr auto members() const {
					return std::tie(Matched, Missing, Boundary, Aborted);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 4>{{"Matched", "Missing", "Boundary", "Aborted"}};
				}
				constexpr auto methods() {
					return std::make_tuple();
				}
				constexpr auto methods() const {
					return std::make_tuple();
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 0>{{}};
				}
			};
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			struct RequiredResult : public ::ISPA_STD::Object {
				long long status;
				bool has_node;
				std::shared_ptr<python::ispa_std::node::Node> node;
				std::shared_ptr<python::ispa_std::recovery::Decision> recovery;
				auto __init__(long long status, bool has_node, std::shared_ptr<python::ispa_std::node::Node> node, std::shared_ptr<python::ispa_std::recovery::Decision> recovery) -> void;
				static auto boundary_only() -> std::shared_ptr<python::ispa_std::recovery::RequiredResult>;
				constexpr auto members() {
					return std::tie(status, has_node, node, recovery);
				}
				constexpr auto members() const {
					return std::tie(status, has_node, node, recovery);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 4>{{"status", "has_node", "node", "recovery"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&RequiredResult::__init__, &RequiredResult::boundary_only);
				}
				constexpr auto methods() const {
					return std::make_tuple(&RequiredResult::__init__, &RequiredResult::boundary_only);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 2>{{"__init__", "boundary_only"}};
				}
			};
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			struct ContextKind : public ::ISPA_STD::Object {
				static long long Mismatch;
				static long long Misplaced;
				static long long Stray;
				constexpr auto members() {
					return std::tie(Mismatch, Misplaced, Stray);
				}
				constexpr auto members() const {
					return std::tie(Mismatch, Misplaced, Stray);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 3>{{"Mismatch", "Misplaced", "Stray"}};
				}
				constexpr auto methods() {
					return std::make_tuple();
				}
				constexpr auto methods() const {
					return std::make_tuple();
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 0>{{}};
				}
			};
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			struct ContextClaim : public ::ISPA_STD::Object {
				std::string name;
				std::shared_ptr<python::ispa_std::recovery::TokenSet> tokens;
				auto __init__(const std::string& name, std::shared_ptr<python::ispa_std::recovery::TokenSet> tokens) -> void;
				constexpr auto members() {
					return std::tie(name, tokens);
				}
				constexpr auto members() const {
					return std::tie(name, tokens);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 2>{{"name", "tokens"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&ContextClaim::__init__);
				}
				constexpr auto methods() const {
					return std::make_tuple(&ContextClaim::__init__);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 1>{{"__init__"}};
				}
			};
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			struct ContextDiagnostic : public ::ISPA_STD::Object {
				long long kind;
				std::string message;
				long long position;
				long long offset;
				long long length;
				long long skipped;
				std::string rule;
				std::string resume_rule;
				std::string claim;
				auto __init__() -> void;
				constexpr auto members() {
					return std::tie(kind, message, position, offset, length, skipped, rule, resume_rule, claim);
				}
				constexpr auto members() const {
					return std::tie(kind, message, position, offset, length, skipped, rule, resume_rule, claim);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 9>{{"kind", "message", "position", "offset", "length", "skipped", "rule", "resume_rule", "claim"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&ContextDiagnostic::__init__);
				}
				constexpr auto methods() const {
					return std::make_tuple(&ContextDiagnostic::__init__);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 1>{{"__init__"}};
				}
			};
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace recovery {
			auto describe_ids(const std::vector<long long>& ids) -> std::string;
		} // namespace recovery
	} // namespace ispa_std
} // namespace python
namespace python::ispa_std::counter { struct CountSpec; }
namespace python {
	namespace ispa_std {
		namespace counter {
			struct CountSpec : public ::ISPA_STD::Object {
				std::string name;
				bool has_count;
				long long count;
				long long minimum;
				auto __init__(const std::string& name, bool has_count, long long count, long long minimum) -> void;
				static auto named(const std::string& name, long long minimum) -> std::shared_ptr<python::ispa_std::counter::CountSpec>;
				static auto fixed(long long value, long long minimum) -> std::shared_ptr<python::ispa_std::counter::CountSpec>;
				constexpr auto members() {
					return std::tie(name, has_count, count, minimum);
				}
				constexpr auto members() const {
					return std::tie(name, has_count, count, minimum);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 4>{{"name", "has_count", "count", "minimum"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&CountSpec::__init__, &CountSpec::named, &CountSpec::fixed);
				}
				constexpr auto methods() const {
					return std::make_tuple(&CountSpec::__init__, &CountSpec::named, &CountSpec::fixed);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 3>{{"__init__", "named", "fixed"}};
				}
			};
		} // namespace counter
	} // namespace ispa_std
} // namespace python
namespace python::ispa_std::match_result { struct MatchResult; }
namespace python {
	namespace ispa_std {
		namespace match_result {
			struct MatchResult : public ::ISPA_STD::Object {
				bool status;
				std::shared_ptr<python::ispa_std::node::Node> node;
				std::shared_ptr<python::ispa_std::lexer::TokenCursor> it;
				auto __init__(bool status, std::shared_ptr<python::ispa_std::node::Node> node, std::shared_ptr<python::ispa_std::lexer::TokenCursor> it) -> void;
				static auto failed(std::shared_ptr<python::ispa_std::lexer::TokenCursor> it) -> std::shared_ptr<python::ispa_std::match_result::MatchResult>;
				constexpr auto members() {
					return std::tie(status, node, it);
				}
				constexpr auto members() const {
					return std::tie(status, node, it);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 3>{{"status", "node", "it"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&MatchResult::__init__, &MatchResult::failed);
				}
				constexpr auto methods() const {
					return std::make_tuple(&MatchResult::__init__, &MatchResult::failed);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 2>{{"__init__", "failed"}};
				}
			};
		} // namespace match_result
	} // namespace ispa_std
} // namespace python
namespace python::ispa_std::parser_base { struct Parser_base; }
namespace python {
	namespace ispa_std {
		namespace parser_base {
			struct Parser_base : public ::ISPA_STD::Object {
				std::shared_ptr<python::ispa_std::lexer::Lexer_base> lexer;
				bool has_lexer;
				std::string text;
				bool has_text;
				std::any tree;
				auto __init__() -> void;
				static auto with_lexer(std::shared_ptr<python::ispa_std::parser_base::Parser_base> parser, std::shared_ptr<python::ispa_std::lexer::Lexer_base> lex) -> std::shared_ptr<python::ispa_std::parser_base::Parser_base>;
				virtual auto parse_lexer(std::shared_ptr<python::ispa_std::lexer::Lexer_base> lex) -> std::any;
				virtual auto parse_text(const std::string& text) -> std::any;
				virtual auto set_input_lexer(std::shared_ptr<python::ispa_std::lexer::Lexer_base> lex) -> void;
				virtual auto set_input_text(const std::string& text) -> void;
				virtual auto clear_input() -> void;
				virtual auto parse_tokens() -> std::any;
				virtual auto before_parse() -> void;
				virtual auto parseLazy() -> std::any;
				virtual auto parseFromTokens() -> void;
				virtual auto lazyParse() -> std::shared_ptr<python::ispa_std::match_result::MatchResult>;
				constexpr auto members() {
					return std::tie(lexer, has_lexer, text, has_text, tree);
				}
				constexpr auto members() const {
					return std::tie(lexer, has_lexer, text, has_text, tree);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 5>{{"lexer", "has_lexer", "text", "has_text", "tree"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&Parser_base::__init__, &Parser_base::with_lexer, &Parser_base::parse_lexer, &Parser_base::parse_text, &Parser_base::set_input_lexer, &Parser_base::set_input_text, &Parser_base::clear_input, &Parser_base::parse_tokens, &Parser_base::before_parse, &Parser_base::parseLazy, &Parser_base::parseFromTokens, &Parser_base::lazyParse);
				}
				constexpr auto methods() const {
					return std::make_tuple(&Parser_base::__init__, &Parser_base::with_lexer, &Parser_base::parse_lexer, &Parser_base::parse_text, &Parser_base::set_input_lexer, &Parser_base::set_input_text, &Parser_base::clear_input, &Parser_base::parse_tokens, &Parser_base::before_parse, &Parser_base::parseLazy, &Parser_base::parseFromTokens, &Parser_base::lazyParse);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 12>{{"__init__", "with_lexer", "parse_lexer", "parse_text", "set_input_lexer", "set_input_text", "clear_input", "parse_tokens", "before_parse", "parseLazy", "parseFromTokens", "lazyParse"}};
				}
			};
		} // namespace parser_base
	} // namespace ispa_std
} // namespace python
namespace python::ispa_std::scan { struct ScanResult; }
namespace python {
	namespace ispa_std {
		namespace scan {
			struct ScanResult : public ::ISPA_STD::Object {
				long long end;
				bool exhausted;
				auto __init__(long long end, bool exhausted) -> void;
				constexpr auto members() {
					return std::tie(end, exhausted);
				}
				constexpr auto members() const {
					return std::tie(end, exhausted);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 2>{{"end", "exhausted"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&ScanResult::__init__);
				}
				constexpr auto methods() const {
					return std::make_tuple(&ScanResult::__init__);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 1>{{"__init__"}};
				}
			};
		} // namespace scan
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace scan {
			auto _boundary_at(const std::string& src, long long p, const std::string& boundary) -> bool;
		} // namespace scan
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace scan {
			auto scan_balanced(const std::string& src, long long begin, const std::vector<std::string>& boundaries, long long budget) -> std::shared_ptr<python::ispa_std::scan::ScanResult>;
		} // namespace scan
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace scan {
			auto text_between(const std::string& src, long long begin, long long end) -> std::string;
		} // namespace scan
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace algorithm {
			template<typename T> auto _check(const std::vector<T>& arr, long long first, long long last) -> void;
		} // namespace algorithm
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace algorithm {
			template<typename T> auto for_each(const std::vector<T>& arr, long long first, long long last, std::function<void(T)> fn) -> void;
		} // namespace algorithm
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace algorithm {
			template<typename T> auto find(const std::vector<T>& arr, long long first, long long last, T value) -> long long;
		} // namespace algorithm
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace algorithm {
			template<typename T> auto find_if(const std::vector<T>& arr, long long first, long long last, std::function<bool(T)> pred) -> long long;
		} // namespace algorithm
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace algorithm {
			template<typename T> auto count(const std::vector<T>& arr, long long first, long long last, T value) -> long long;
		} // namespace algorithm
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace algorithm {
			template<typename T> auto count_if(const std::vector<T>& arr, long long first, long long last, std::function<bool(T)> pred) -> long long;
		} // namespace algorithm
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace algorithm {
			template<typename T> auto all_of(const std::vector<T>& arr, long long first, long long last, std::function<bool(T)> pred) -> bool;
		} // namespace algorithm
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace algorithm {
			template<typename T> auto any_of(const std::vector<T>& arr, long long first, long long last, std::function<bool(T)> pred) -> bool;
		} // namespace algorithm
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace algorithm {
			template<typename T> auto none_of(const std::vector<T>& arr, long long first, long long last, std::function<bool(T)> pred) -> bool;
		} // namespace algorithm
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace algorithm {
			template<typename T> auto equal(const std::vector<T>& a, long long a_first, long long a_last, const std::vector<T>& b, long long b_first) -> bool;
		} // namespace algorithm
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace algorithm {
			template<typename T, typename U> auto accumulate(const std::vector<T>& arr, long long first, long long last, U init, std::function<U(U, T)> fn) -> U;
		} // namespace algorithm
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace algorithm {
			template<typename T, typename U> auto transform(const std::vector<T>& arr, long long first, long long last, std::function<U(T)> fn) -> std::vector<U>;
		} // namespace algorithm
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace algorithm {
			template<typename T> auto fill(std::vector<T>& arr, long long first, long long last, T value) -> void;
		} // namespace algorithm
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace algorithm {
			template<typename T> auto reverse(std::vector<T>& arr, long long first, long long last) -> void;
		} // namespace algorithm
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace algorithm {
			template<typename T> auto min_element(const std::vector<T>& arr, long long first, long long last) -> long long;
		} // namespace algorithm
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace algorithm {
			template<typename T> auto max_element(const std::vector<T>& arr, long long first, long long last) -> long long;
		} // namespace algorithm
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace algorithm {
			template<typename T> auto is_sorted(const std::vector<T>& arr, long long first, long long last) -> bool;
		} // namespace algorithm
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace algorithm {
			template<typename T> auto lower_bound(const std::vector<T>& arr, long long first, long long last, T value) -> long long;
		} // namespace algorithm
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace algorithm {
			template<typename T> auto upper_bound(const std::vector<T>& arr, long long first, long long last, T value) -> long long;
		} // namespace algorithm
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace algorithm {
			template<typename T> auto binary_search(const std::vector<T>& arr, long long first, long long last, T value) -> bool;
		} // namespace algorithm
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace algorithm {
			template<typename T> auto stable_sort(std::vector<T>& arr, long long first, long long last, std::function<bool(T, T)> less) -> void;
		} // namespace algorithm
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace algorithm {
			template<typename T> auto sort(const std::vector<T>& arr, long long first, long long last) -> void;
		} // namespace algorithm
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace algorithm {
			template<typename T> auto erase_if(std::vector<T>& arr, std::function<bool(T)> pred) -> long long;
		} // namespace algorithm
	} // namespace ispa_std
} // namespace python
namespace python::ispa_std::sortedmap { struct StrIntMap; }
namespace python::ispa_std::sortedmap { struct IntSet; }
namespace python {
	namespace ispa_std {
		namespace sortedmap {
			struct StrIntMap : public ::ISPA_STD::Object {
				std::vector<std::string> keys;
				std::vector<long long> values;
				auto __init__() -> void;
				virtual auto find(const std::string& key) -> long long;
				virtual auto contains(const std::string& key) -> bool;
				virtual auto get(const std::string& key) -> long long;
				virtual auto set(const std::string& key, long long value) -> void;
				virtual auto try_emplace(const std::string& key, long long value) -> bool;
				virtual auto clear() -> void;
				virtual auto copy() -> std::shared_ptr<python::ispa_std::sortedmap::StrIntMap>;
				virtual auto assign(std::shared_ptr<python::ispa_std::sortedmap::StrIntMap> other) -> void;
				constexpr auto members() {
					return std::tie(keys, values);
				}
				constexpr auto members() const {
					return std::tie(keys, values);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 2>{{"keys", "values"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&StrIntMap::__init__, &StrIntMap::find, &StrIntMap::contains, &StrIntMap::get, &StrIntMap::set, &StrIntMap::try_emplace, &StrIntMap::clear, &StrIntMap::copy, &StrIntMap::assign);
				}
				constexpr auto methods() const {
					return std::make_tuple(&StrIntMap::__init__, &StrIntMap::find, &StrIntMap::contains, &StrIntMap::get, &StrIntMap::set, &StrIntMap::try_emplace, &StrIntMap::clear, &StrIntMap::copy, &StrIntMap::assign);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 9>{{"__init__", "find", "contains", "get", "set", "try_emplace", "clear", "copy", "assign"}};
				}
			};
		} // namespace sortedmap
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace sortedmap {
			struct IntSet : public ::ISPA_STD::Object {
				std::vector<long long> items;
				auto __init__() -> void;
				virtual auto add(long long value) -> bool;
				virtual auto contains(long long value) -> bool;
				virtual auto size() -> long long;
				virtual auto empty() -> bool;
				virtual auto smallest() -> long long;
				virtual auto copy() -> std::shared_ptr<python::ispa_std::sortedmap::IntSet>;
				constexpr auto members() {
					return std::tie(items);
				}
				constexpr auto members() const {
					return std::tie(items);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 1>{{"items"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&IntSet::__init__, &IntSet::add, &IntSet::contains, &IntSet::size, &IntSet::empty, &IntSet::smallest, &IntSet::copy);
				}
				constexpr auto methods() const {
					return std::make_tuple(&IntSet::__init__, &IntSet::add, &IntSet::contains, &IntSet::size, &IntSet::empty, &IntSet::smallest, &IntSet::copy);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 7>{{"__init__", "add", "contains", "size", "empty", "smallest", "copy"}};
				}
			};
		} // namespace sortedmap
	} // namespace ispa_std
} // namespace python
namespace python::ispa_std::parser_ll { struct ActionToken; }
namespace python::ispa_std::parser_ll { struct TokenBoundary; }
namespace python::ispa_std::parser_ll { struct TokenNames; }
namespace python::ispa_std::parser_ll { struct ActionCheckpoint; }
namespace python::ispa_std::parser_ll { struct RuleContext; }
namespace python::ispa_std::parser_ll { struct FailContext; }
namespace python::ispa_std::parser_ll { struct Frame; }
namespace python::ispa_std::parser_ll { struct FrameGuard; }
namespace python::ispa_std::parser_ll { struct CounterScope; }
namespace python::ispa_std::parser_ll { struct RuleProbe; }
namespace python::ispa_std::parser_ll { struct Resume; }
namespace python::ispa_std::parser_ll { struct LLParser; }
namespace python {
	namespace ispa_std {
		namespace parser_ll {
			inline long long MAX_ACTION_LOOKAHEAD = 64;
		} // namespace parser_ll
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace parser_ll {
			inline long long MAX_ACTION_CHECKPOINTS = 32;
		} // namespace parser_ll
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace parser_ll {
			inline long long NPOS = ( - (1));
		} // namespace parser_ll
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace parser_ll {
			auto lookahead_kind(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> long long;
		} // namespace parser_ll
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace parser_ll {
			auto cursor_position(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> long long;
		} // namespace parser_ll
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace parser_ll {
			auto parse_token_stream(std::shared_ptr<python::ispa_std::lexer::Lexer_base> lexer, const std::vector<std::shared_ptr<python::ispa_std::raw_token::RawToken>>& tokens, std::function<std::shared_ptr<python::ispa_std::match_result::MatchResult>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>)> rule) -> std::shared_ptr<python::ispa_std::node::Node>;
		} // namespace parser_ll
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace parser_ll {
			auto parse_source_span(std::shared_ptr<python::ispa_std::lexer::Lexer_base> lexer, const std::vector<std::shared_ptr<python::ispa_std::raw_token::RawToken>>& tokens, std::function<std::shared_ptr<python::ispa_std::match_result::MatchResult>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>)> rule) -> std::shared_ptr<python::ispa_std::node::Node>;
		} // namespace parser_ll
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace parser_ll {
			struct ActionToken : public ::ISPA_STD::Object {
				std::shared_ptr<python::ispa_std::raw_token::RawToken> raw;
				std::string text;
				std::string name;
				long long kind;
				long long offset;
				long long length;
				long long line;
				long long column;
				bool present;
				auto __init__() -> void;
				virtual auto empty() -> bool;
				virtual auto matches_name(const std::string& expected) -> bool;
				constexpr auto members() {
					return std::tie(raw, text, name, kind, offset, length, line, column, present);
				}
				constexpr auto members() const {
					return std::tie(raw, text, name, kind, offset, length, line, column, present);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 9>{{"raw", "text", "name", "kind", "offset", "length", "line", "column", "present"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&ActionToken::__init__, &ActionToken::empty, &ActionToken::matches_name);
				}
				constexpr auto methods() const {
					return std::make_tuple(&ActionToken::__init__, &ActionToken::empty, &ActionToken::matches_name);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 3>{{"__init__", "empty", "matches_name"}};
				}
			};
		} // namespace parser_ll
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace parser_ll {
			struct TokenBoundary : public ::ISPA_STD::Object {
				std::string value;
				bool token_name;
				auto __init__(const std::string& value, bool token_name) -> void;
				constexpr auto members() {
					return std::tie(value, token_name);
				}
				constexpr auto members() const {
					return std::tie(value, token_name);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 2>{{"value", "token_name"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&TokenBoundary::__init__);
				}
				constexpr auto methods() const {
					return std::make_tuple(&TokenBoundary::__init__);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 1>{{"__init__"}};
				}
			};
		} // namespace parser_ll
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace parser_ll {
			auto spelling(const std::string& value) -> std::shared_ptr<python::ispa_std::parser_ll::TokenBoundary>;
		} // namespace parser_ll
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace parser_ll {
			auto token_boundary(const std::string& name) -> std::shared_ptr<python::ispa_std::parser_ll::TokenBoundary>;
		} // namespace parser_ll
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace parser_ll {
			struct TokenNames : public ::ISPA_STD::Object {
				std::vector<std::string> names;
				auto __init__(const std::vector<std::string>& names) -> void;
				virtual auto lookup(long long id) -> std::string;
				constexpr auto members() {
					return std::tie(names);
				}
				constexpr auto members() const {
					return std::tie(names);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 1>{{"names"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&TokenNames::__init__, &TokenNames::lookup);
				}
				constexpr auto methods() const {
					return std::make_tuple(&TokenNames::__init__, &TokenNames::lookup);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 2>{{"__init__", "lookup"}};
				}
			};
		} // namespace parser_ll
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace parser_ll {
			inline std::vector<std::string> _NO_NAMES = std::vector<std::string>{};
		} // namespace parser_ll
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace parser_ll {
			inline std::shared_ptr<python::ispa_std::parser_ll::TokenNames> NO_TOKEN_NAMES = ISPA_STD::construct<python::ispa_std::parser_ll::TokenNames>(python::ispa_std::parser_ll::_NO_NAMES);
		} // namespace parser_ll
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace parser_ll {
			auto context_peek(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, long long lookahead, std::shared_ptr<python::ispa_std::parser_ll::TokenNames> token_names, long long ambiguous_id) -> std::shared_ptr<python::ispa_std::parser_ll::ActionToken>;
		} // namespace parser_ll
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace parser_ll {
			auto context_prev(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::shared_ptr<python::ispa_std::lexer::TokenCursor> start_pos, std::shared_ptr<python::ispa_std::parser_ll::TokenNames> token_names, long long ambiguous_id) -> std::shared_ptr<python::ispa_std::parser_ll::ActionToken>;
		} // namespace parser_ll
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace parser_ll {
			auto context_at(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, const std::string& text, long long lookahead, std::shared_ptr<python::ispa_std::parser_ll::TokenNames> token_names, bool match_name, long long ambiguous_id) -> bool;
		} // namespace parser_ll
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace parser_ll {
			struct ActionCheckpoint : public ::ISPA_STD::Object {
				std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos;
				std::vector<std::string> names;
				std::vector<std::shared_ptr<python::ispa_std::lexer::TokenCursor>> saved;
				auto __init__(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> void;
				virtual auto mark(const std::string& name) -> void;
				virtual auto _find_last(const std::string& name) -> long long;
				virtual auto restore(const std::string& name) -> void;
				virtual auto commit(const std::string& name) -> void;
				constexpr auto members() {
					return std::tie(pos, names, saved);
				}
				constexpr auto members() const {
					return std::tie(pos, names, saved);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 3>{{"pos", "names", "saved"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&ActionCheckpoint::__init__, &ActionCheckpoint::mark, &ActionCheckpoint::_find_last, &ActionCheckpoint::restore, &ActionCheckpoint::commit);
				}
				constexpr auto methods() const {
					return std::make_tuple(&ActionCheckpoint::__init__, &ActionCheckpoint::mark, &ActionCheckpoint::_find_last, &ActionCheckpoint::restore, &ActionCheckpoint::commit);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 5>{{"__init__", "mark", "_find_last", "restore", "commit"}};
				}
			};
		} // namespace parser_ll
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace parser_ll {
			struct RuleContext : public python::ispa_std::parser_ll::ActionCheckpoint {
				std::any ast;
				std::shared_ptr<python::ispa_std::lexer::TokenCursor> start_pos;
				std::string rule;
				std::shared_ptr<python::ispa_std::parser_ll::TokenNames> token_names;
				long long ambiguous_id;
				auto __init__(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::any ast, std::shared_ptr<python::ispa_std::lexer::TokenCursor> start_pos, const std::string& rule, std::shared_ptr<python::ispa_std::parser_ll::TokenNames> token_names, long long ambiguous_id) -> void;
				virtual auto at(const std::string& text, long long lookahead) -> bool;
				virtual auto at_token(const std::string& name, long long lookahead) -> bool;
				virtual auto peek(long long lookahead) -> std::shared_ptr<python::ispa_std::parser_ll::ActionToken>;
				virtual auto take() -> std::shared_ptr<python::ispa_std::parser_ll::ActionToken>;
				virtual auto prev() -> std::shared_ptr<python::ispa_std::parser_ll::ActionToken>;
				virtual auto collect_braced() -> std::vector<std::shared_ptr<python::ispa_std::raw_token::RawToken>>;
				constexpr auto members() {
					return std::tie(ast, start_pos, rule, token_names, ambiguous_id);
				}
				constexpr auto members() const {
					return std::tie(ast, start_pos, rule, token_names, ambiguous_id);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 5>{{"ast", "start_pos", "rule", "token_names", "ambiguous_id"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&RuleContext::__init__, &RuleContext::at, &RuleContext::at_token, &RuleContext::peek, &RuleContext::take, &RuleContext::prev, &RuleContext::collect_braced);
				}
				constexpr auto methods() const {
					return std::make_tuple(&RuleContext::__init__, &RuleContext::at, &RuleContext::at_token, &RuleContext::peek, &RuleContext::take, &RuleContext::prev, &RuleContext::collect_braced);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 7>{{"__init__", "at", "at_token", "peek", "take", "prev", "collect_braced"}};
				}
			};
		} // namespace parser_ll
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace parser_ll {
			struct FailContext : public python::ispa_std::parser_ll::ActionCheckpoint {
				std::any ast;
				std::shared_ptr<python::ispa_std::lexer::TokenCursor> start_pos;
				std::string rule;
				std::string failed_rule;
				std::shared_ptr<python::ispa_std::recovery::State> recovery_state;
				std::string source;
				bool has_source_text;
				long long diagnostic_begin;
				std::shared_ptr<python::ispa_std::parser_ll::TokenNames> token_names;
				long long ambiguous_id;
				long long requested;
				bool has_request;
				long long skipped;
				auto __init__(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::any ast, std::shared_ptr<python::ispa_std::lexer::TokenCursor> start_pos, const std::string& rule, const std::string& failed_rule, std::shared_ptr<python::ispa_std::recovery::State> state, const std::string& source, bool has_source_text, long long diagnostic_begin, std::shared_ptr<python::ispa_std::parser_ll::TokenNames> token_names, long long ambiguous_id) -> void;
				virtual auto at(const std::string& text, long long lookahead) -> bool;
				virtual auto at_token(const std::string& name, long long lookahead) -> bool;
				virtual auto peek(long long lookahead) -> std::shared_ptr<python::ispa_std::parser_ll::ActionToken>;
				virtual auto take() -> std::shared_ptr<python::ispa_std::parser_ll::ActionToken>;
				virtual auto prev() -> std::shared_ptr<python::ispa_std::parser_ll::ActionToken>;
				virtual auto error(const std::string& message, const std::string& code, long long source_offset, long long source_length) -> void;
				virtual auto note(const std::string& message) -> void;
				virtual auto sync(const std::vector<std::string>& boundaries) -> void;
				virtual auto sync_tokens(const std::vector<std::shared_ptr<python::ispa_std::parser_ll::TokenBoundary>>& boundaries) -> void;
				virtual auto skip() -> void;
				virtual auto insert() -> void;
				virtual auto replace() -> void;
				virtual auto retry() -> void;
				virtual auto abort() -> void;
				virtual auto finish(std::shared_ptr<python::ispa_std::lexer::TokenCursor> before) -> std::shared_ptr<python::ispa_std::recovery::Decision>;
				constexpr auto members() {
					return std::tie(ast, start_pos, rule, failed_rule, recovery_state, source, has_source_text, diagnostic_begin, token_names, ambiguous_id, requested, has_request, skipped);
				}
				constexpr auto members() const {
					return std::tie(ast, start_pos, rule, failed_rule, recovery_state, source, has_source_text, diagnostic_begin, token_names, ambiguous_id, requested, has_request, skipped);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 13>{{"ast", "start_pos", "rule", "failed_rule", "recovery_state", "source", "has_source_text", "diagnostic_begin", "token_names", "ambiguous_id", "requested", "has_request", "skipped"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&FailContext::__init__, &FailContext::at, &FailContext::at_token, &FailContext::peek, &FailContext::take, &FailContext::prev, &FailContext::error, &FailContext::note, &FailContext::sync, &FailContext::sync_tokens, &FailContext::skip, &FailContext::insert, &FailContext::replace, &FailContext::retry, &FailContext::abort, &FailContext::finish);
				}
				constexpr auto methods() const {
					return std::make_tuple(&FailContext::__init__, &FailContext::at, &FailContext::at_token, &FailContext::peek, &FailContext::take, &FailContext::prev, &FailContext::error, &FailContext::note, &FailContext::sync, &FailContext::sync_tokens, &FailContext::skip, &FailContext::insert, &FailContext::replace, &FailContext::retry, &FailContext::abort, &FailContext::finish);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 16>{{"__init__", "at", "at_token", "peek", "take", "prev", "error", "note", "sync", "sync_tokens", "skip", "insert", "replace", "retry", "abort", "finish"}};
				}
			};
		} // namespace parser_ll
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace parser_ll {
			struct Frame : public ::ISPA_STD::Object {
				std::string rule;
				std::string expecting;
				std::vector<std::shared_ptr<python::ispa_std::recovery::ContextClaim>> claims;
				auto __init__(const std::string& rule) -> void;
				constexpr auto members() {
					return std::tie(rule, expecting, claims);
				}
				constexpr auto members() const {
					return std::tie(rule, expecting, claims);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 3>{{"rule", "expecting", "claims"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&Frame::__init__);
				}
				constexpr auto methods() const {
					return std::make_tuple(&Frame::__init__);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 1>{{"__init__"}};
				}
			};
		} // namespace parser_ll
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace parser_ll {
			struct FrameGuard : public ::ISPA_STD::Object {
				std::shared_ptr<python::ispa_std::parser_ll::LLParser> parser;
				bool open;
				auto __init__(std::shared_ptr<python::ispa_std::parser_ll::LLParser> parser, const std::string& rule) -> void;
				virtual auto close() -> void;
				constexpr auto members() {
					return std::tie(parser, open);
				}
				constexpr auto members() const {
					return std::tie(parser, open);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 2>{{"parser", "open"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&FrameGuard::__init__, &FrameGuard::close);
				}
				constexpr auto methods() const {
					return std::make_tuple(&FrameGuard::__init__, &FrameGuard::close);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 2>{{"__init__", "close"}};
				}
			};
		} // namespace parser_ll
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace parser_ll {
			struct CounterScope : public ::ISPA_STD::Object {
				std::shared_ptr<python::ispa_std::parser_ll::LLParser> parser;
				std::shared_ptr<python::ispa_std::sortedmap::StrIntMap> saved;
				bool open;
				auto __init__(std::shared_ptr<python::ispa_std::parser_ll::LLParser> parser) -> void;
				virtual auto restore() -> void;
				constexpr auto members() {
					return std::tie(parser, saved, open);
				}
				constexpr auto members() const {
					return std::tie(parser, saved, open);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 3>{{"parser", "saved", "open"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&CounterScope::__init__, &CounterScope::restore);
				}
				constexpr auto methods() const {
					return std::make_tuple(&CounterScope::__init__, &CounterScope::restore);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 2>{{"__init__", "restore"}};
				}
			};
		} // namespace parser_ll
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace parser_ll {
			struct RuleProbe : public ::ISPA_STD::Object {
				std::shared_ptr<python::ispa_std::parser_ll::LLParser> parser;
				long long start;
				bool committed;
				bool open;
				auto __init__(std::shared_ptr<python::ispa_std::parser_ll::LLParser> parser, long long start) -> void;
				virtual auto close() -> void;
				constexpr auto members() {
					return std::tie(parser, start, committed, open);
				}
				constexpr auto members() const {
					return std::tie(parser, start, committed, open);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 4>{{"parser", "start", "committed", "open"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&RuleProbe::__init__, &RuleProbe::close);
				}
				constexpr auto methods() const {
					return std::make_tuple(&RuleProbe::__init__, &RuleProbe::close);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 2>{{"__init__", "close"}};
				}
			};
		} // namespace parser_ll
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace parser_ll {
			struct Resume : public ::ISPA_STD::Object {
				long long skip;
				long long frame;
				std::shared_ptr<python::ispa_std::recovery::ContextClaim> claim;
				bool has_claim;
				auto __init__(long long skip, long long frame, std::shared_ptr<python::ispa_std::recovery::ContextClaim> claim, bool has_claim) -> void;
				constexpr auto members() {
					return std::tie(skip, frame, claim, has_claim);
				}
				constexpr auto members() const {
					return std::tie(skip, frame, claim, has_claim);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 4>{{"skip", "frame", "claim", "has_claim"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&Resume::__init__);
				}
				constexpr auto methods() const {
					return std::make_tuple(&Resume::__init__);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 1>{{"__init__"}};
				}
			};
		} // namespace parser_ll
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace parser_ll {
			struct LLParser : public python::ispa_std::parser_base::Parser_base {
				std::shared_ptr<python::ispa_std::recovery::State> recovery_state_;
				std::shared_ptr<python::ispa_std::sortedmap::StrIntMap> counters_;
				std::vector<std::shared_ptr<python::ispa_std::parser_ll::Frame>> frames_;
				long long frame_count;
				long long unwind_;
				long long last_resume_pos_;
				bool recovering_;
				long long context_max_skip_;
				std::vector<std::shared_ptr<python::ispa_std::recovery::ContextDiagnostic>> context_errors_;
				std::vector<std::shared_ptr<python::ispa_std::parser_ll::RuleProbe>> probes_;
				std::shared_ptr<python::ispa_std::parser_ll::TokenNames> token_names_;
				long long ambiguous_id_;
				long long space_token_;
				bool has_space_token_;
				auto __init__() -> void;
				virtual auto set_token_names(std::shared_ptr<python::ispa_std::parser_ll::TokenNames> names, long long ambiguous_id) -> void;
				virtual auto set_space_token(long long type_id) -> void;
				virtual auto getErrors() -> std::vector<std::shared_ptr<python::ispa_std::recovery::Diagnostic>>;
				virtual auto getContextErrors() -> const std::vector<std::shared_ptr<python::ispa_std::recovery::ContextDiagnostic>>&;
				virtual auto reset_recovery() -> void;
				virtual auto before_parse() -> void;
				virtual auto parseLazy() -> std::any;
				virtual auto counter_scope() -> std::shared_ptr<python::ispa_std::parser_ll::CounterScope>;
				virtual auto bind_counter_count(const std::string& name, long long count) -> void;
				virtual auto bind_counter_list(const std::string& name, const std::vector<std::shared_ptr<python::ispa_std::node::Node>>& values) -> void;
				virtual auto bind_counter_node(const std::string& name, std::shared_ptr<python::ispa_std::node::Node> node) -> void;
				virtual auto begin_probe(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> std::shared_ptr<python::ispa_std::parser_ll::RuleProbe>;
				virtual auto reject_absent_rule() -> void;
				virtual auto commit_rule(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> void;
				virtual auto note_match() -> void;
				virtual auto debug_actual(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> std::string;
				virtual auto debug_recovery_mode(long long mode) -> std::string;
				virtual auto recovery_frame(const std::string& rule) -> std::shared_ptr<python::ispa_std::parser_ll::FrameGuard>;
				virtual auto enter_frame(const std::string& rule) -> void;
				virtual auto leave_frame() -> void;
				virtual auto recovery_at(const std::string& expecting, const std::vector<std::shared_ptr<python::ispa_std::recovery::ContextClaim>>& claims) -> void;
				virtual auto unwinding() -> bool;
				virtual auto describe_token(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> std::string;
				virtual auto breadcrumb() -> std::string;
				virtual auto resolve_context(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, long long max_skip) -> std::shared_ptr<python::ispa_std::parser_ll::Resume>;
				virtual auto report_context(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, long long kind, std::shared_ptr<python::ispa_std::parser_ll::Resume> r) -> void;
				virtual auto recover_at_context(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, long long kind) -> bool;
				virtual auto sync_loop(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> void;
				virtual auto recover_choice(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, const std::string& rule, const std::string& expected, const std::vector<std::shared_ptr<python::ispa_std::recovery::TokenSet>>& first_sets, const std::vector<std::shared_ptr<python::ispa_std::recovery::TokenSet>>& inner_sets) -> long long;
				virtual auto raise_error(std::shared_ptr<python::ispa_std::raw_token::RawToken> token, const std::string& message) -> void;
				virtual auto required_recovering(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, long long type_id, std::shared_ptr<python::ispa_std::recovery::FailHandler> fail) -> std::shared_ptr<python::ispa_std::recovery::RequiredResult>;
				virtual auto _attempt_rule(std::function<std::shared_ptr<python::ispa_std::match_result::MatchResult>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>)> rec, std::shared_ptr<python::ispa_std::lexer::TokenCursor> attempt) -> std::shared_ptr<python::ispa_std::match_result::MatchResult>;
				virtual auto required_rule_recovering(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::function<std::shared_ptr<python::ispa_std::match_result::MatchResult>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>)> rec, std::shared_ptr<python::ispa_std::recovery::FailHandler> fail) -> std::shared_ptr<python::ispa_std::recovery::RequiredResult>;
				virtual auto _required_outcome(std::shared_ptr<python::ispa_std::recovery::RequiredResult> result, std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::shared_ptr<python::ispa_std::debug::MatchScope> scope, const std::string& what) -> std::shared_ptr<python::ispa_std::node::Node>;
				virtual auto required_token(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, long long type_id, std::shared_ptr<python::ispa_std::recovery::FailHandler> fail) -> std::shared_ptr<python::ispa_std::node::Node>;
				virtual auto required_rule(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, const std::string& rule_name, std::function<std::shared_ptr<python::ispa_std::match_result::MatchResult>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>)> rec, std::shared_ptr<python::ispa_std::recovery::FailHandler> fail) -> std::shared_ptr<python::ispa_std::node::Node>;
				virtual auto optional_token(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, long long type_id) -> std::shared_ptr<python::ispa_std::node::Node>;
				virtual auto optional_rule(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::function<std::shared_ptr<python::ispa_std::match_result::MatchResult>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>)> rec) -> std::shared_ptr<python::ispa_std::node::Node>;
				virtual auto skip_spaces(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> void;
				virtual auto skip_external_spaces(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> void;
				virtual auto counted_impl(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::shared_ptr<python::ispa_std::counter::CountSpec> spec, std::function<std::shared_ptr<python::ispa_std::node::Node>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>)> match) -> std::vector<std::shared_ptr<python::ispa_std::node::Node>>;
				virtual auto counted_token(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, long long type_id, std::shared_ptr<python::ispa_std::counter::CountSpec> spec) -> std::vector<std::shared_ptr<python::ispa_std::node::Node>>;
				virtual auto counted_rule(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::function<std::shared_ptr<python::ispa_std::match_result::MatchResult>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>)> rec, std::shared_ptr<python::ispa_std::counter::CountSpec> spec) -> std::vector<std::shared_ptr<python::ispa_std::node::Node>>;
				virtual auto repeat_optional(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::function<std::shared_ptr<python::ispa_std::node::Node>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>)> match, bool separated) -> std::shared_ptr<python::ispa_std::node::Node>;
				virtual auto range_token(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, long long type_id, std::shared_ptr<python::ispa_std::recovery::FailHandler> fail) -> std::vector<std::shared_ptr<python::ispa_std::node::Node>>;
				virtual auto range_rule(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, const std::string& rule_name, std::function<std::shared_ptr<python::ispa_std::match_result::MatchResult>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>)> rec, std::shared_ptr<python::ispa_std::recovery::FailHandler> fail) -> std::vector<std::shared_ptr<python::ispa_std::node::Node>>;
				virtual auto range_optional_token(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, long long type_id) -> std::vector<std::shared_ptr<python::ispa_std::node::Node>>;
				virtual auto range_optional_rule(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::function<std::shared_ptr<python::ispa_std::match_result::MatchResult>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>)> rec) -> std::vector<std::shared_ptr<python::ispa_std::node::Node>>;
				virtual auto rule_context(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::any ast, std::shared_ptr<python::ispa_std::lexer::TokenCursor> start, const std::string& rule) -> std::shared_ptr<python::ispa_std::parser_ll::RuleContext>;
				virtual auto fail_context(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::any ast, std::shared_ptr<python::ispa_std::lexer::TokenCursor> start, const std::string& rule, const std::string& failed_rule) -> std::shared_ptr<python::ispa_std::parser_ll::FailContext>;
				virtual auto custom_failure_decision(std::shared_ptr<python::ispa_std::lexer::TokenCursor> before, std::shared_ptr<python::ispa_std::lexer::TokenCursor> after) -> std::shared_ptr<python::ispa_std::recovery::Decision>;
				virtual auto custom_failure_decision_ctx(std::shared_ptr<python::ispa_std::lexer::TokenCursor> before, std::shared_ptr<python::ispa_std::parser_ll::FailContext> ctx) -> std::shared_ptr<python::ispa_std::recovery::Decision>;
				virtual auto custom_failure(std::function<std::shared_ptr<python::ispa_std::recovery::Decision>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>, std::shared_ptr<python::ispa_std::recovery::State>)> action) -> std::shared_ptr<python::ispa_std::recovery::ActionFailure>;
				virtual auto action_result(std::any ast, std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> std::shared_ptr<python::ispa_std::match_result::MatchResult>;
				constexpr auto members() {
					return std::tie(recovery_state_, counters_, frames_, frame_count, unwind_, last_resume_pos_, recovering_, context_max_skip_, context_errors_, probes_, token_names_, ambiguous_id_, space_token_, has_space_token_);
				}
				constexpr auto members() const {
					return std::tie(recovery_state_, counters_, frames_, frame_count, unwind_, last_resume_pos_, recovering_, context_max_skip_, context_errors_, probes_, token_names_, ambiguous_id_, space_token_, has_space_token_);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 14>{{"recovery_state_", "counters_", "frames_", "frame_count", "unwind_", "last_resume_pos_", "recovering_", "context_max_skip_", "context_errors_", "probes_", "token_names_", "ambiguous_id_", "space_token_", "has_space_token_"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&LLParser::__init__, &LLParser::set_token_names, &LLParser::set_space_token, &LLParser::getErrors, &LLParser::getContextErrors, &LLParser::reset_recovery, &LLParser::before_parse, &LLParser::parseLazy, &LLParser::counter_scope, &LLParser::bind_counter_count, &LLParser::bind_counter_list, &LLParser::bind_counter_node, &LLParser::begin_probe, &LLParser::reject_absent_rule, &LLParser::commit_rule, &LLParser::note_match, &LLParser::debug_actual, &LLParser::debug_recovery_mode, &LLParser::recovery_frame, &LLParser::enter_frame, &LLParser::leave_frame, &LLParser::recovery_at, &LLParser::unwinding, &LLParser::describe_token, &LLParser::breadcrumb, &LLParser::resolve_context, &LLParser::report_context, &LLParser::recover_at_context, &LLParser::sync_loop, &LLParser::recover_choice, &LLParser::raise_error, &LLParser::required_recovering, &LLParser::_attempt_rule, &LLParser::required_rule_recovering, &LLParser::_required_outcome, &LLParser::required_token, &LLParser::required_rule, &LLParser::optional_token, &LLParser::optional_rule, &LLParser::skip_spaces, &LLParser::skip_external_spaces, &LLParser::counted_impl, &LLParser::counted_token, &LLParser::counted_rule, &LLParser::repeat_optional, &LLParser::range_token, &LLParser::range_rule, &LLParser::range_optional_token, &LLParser::range_optional_rule, &LLParser::rule_context, &LLParser::fail_context, &LLParser::custom_failure_decision, &LLParser::custom_failure_decision_ctx, &LLParser::custom_failure, &LLParser::action_result);
				}
				constexpr auto methods() const {
					return std::make_tuple(&LLParser::__init__, &LLParser::set_token_names, &LLParser::set_space_token, &LLParser::getErrors, &LLParser::getContextErrors, &LLParser::reset_recovery, &LLParser::before_parse, &LLParser::parseLazy, &LLParser::counter_scope, &LLParser::bind_counter_count, &LLParser::bind_counter_list, &LLParser::bind_counter_node, &LLParser::begin_probe, &LLParser::reject_absent_rule, &LLParser::commit_rule, &LLParser::note_match, &LLParser::debug_actual, &LLParser::debug_recovery_mode, &LLParser::recovery_frame, &LLParser::enter_frame, &LLParser::leave_frame, &LLParser::recovery_at, &LLParser::unwinding, &LLParser::describe_token, &LLParser::breadcrumb, &LLParser::resolve_context, &LLParser::report_context, &LLParser::recover_at_context, &LLParser::sync_loop, &LLParser::recover_choice, &LLParser::raise_error, &LLParser::required_recovering, &LLParser::_attempt_rule, &LLParser::required_rule_recovering, &LLParser::_required_outcome, &LLParser::required_token, &LLParser::required_rule, &LLParser::optional_token, &LLParser::optional_rule, &LLParser::skip_spaces, &LLParser::skip_external_spaces, &LLParser::counted_impl, &LLParser::counted_token, &LLParser::counted_rule, &LLParser::repeat_optional, &LLParser::range_token, &LLParser::range_rule, &LLParser::range_optional_token, &LLParser::range_optional_rule, &LLParser::rule_context, &LLParser::fail_context, &LLParser::custom_failure_decision, &LLParser::custom_failure_decision_ctx, &LLParser::custom_failure, &LLParser::action_result);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 55>{{"__init__", "set_token_names", "set_space_token", "getErrors", "getContextErrors", "reset_recovery", "before_parse", "parseLazy", "counter_scope", "bind_counter_count", "bind_counter_list", "bind_counter_node", "begin_probe", "reject_absent_rule", "commit_rule", "note_match", "debug_actual", "debug_recovery_mode", "recovery_frame", "enter_frame", "leave_frame", "recovery_at", "unwinding", "describe_token", "breadcrumb", "resolve_context", "report_context", "recover_at_context", "sync_loop", "recover_choice", "raise_error", "required_recovering", "_attempt_rule", "required_rule_recovering", "_required_outcome", "required_token", "required_rule", "optional_token", "optional_rule", "skip_spaces", "skip_external_spaces", "counted_impl", "counted_token", "counted_rule", "repeat_optional", "range_token", "range_rule", "range_optional_token", "range_optional_rule", "rule_context", "fail_context", "custom_failure_decision", "custom_failure_decision_ctx", "custom_failure", "action_result"}};
				}
			};
		} // namespace parser_ll
	} // namespace ispa_std
} // namespace python
namespace python::ispa_std::lr { struct LRControlPhase; }
namespace python::ispa_std::lr { struct LRControlState; }
namespace python::ispa_std::lr { struct LRRuleSpan; }
namespace python::ispa_std::lr { struct LRRegionFrame; }
namespace python::ispa_std::lr { struct LRGrammar; }
namespace python::ispa_std::lr { struct LRTokenValue; }
namespace python::ispa_std::lr { struct LRCaptures; }
namespace python::ispa_std::lr { struct LRParser; }
namespace python {
	namespace ispa_std {
		namespace lr {
			inline long long LR_INVALID = ( - (9223372036854775807));
		} // namespace lr
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lr {
			inline long long LR_CONFLICT = 9223372036854775807;
		} // namespace lr
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lr {
			inline long long ABSENT = ( - (1));
		} // namespace lr
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lr {
			struct LRControlPhase : public ::ISPA_STD::Object {
				static long long Decide;
				static long long Shift;
				static long long Reduce;
				static long long Semantic;
				static long long Recover;
				static long long Accept;
				constexpr auto members() {
					return std::tie(Decide, Shift, Reduce, Semantic, Recover, Accept);
				}
				constexpr auto members() const {
					return std::tie(Decide, Shift, Reduce, Semantic, Recover, Accept);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 6>{{"Decide", "Shift", "Reduce", "Semantic", "Recover", "Accept"}};
				}
				constexpr auto methods() {
					return std::make_tuple();
				}
				constexpr auto methods() const {
					return std::make_tuple();
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 0>{{}};
				}
			};
		} // namespace lr
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lr {
			struct LRControlState : public ::ISPA_STD::Object {
				long long phase;
				long long continuation;
				long long production;
				auto __init__() -> void;
				constexpr auto members() {
					return std::tie(phase, continuation, production);
				}
				constexpr auto members() const {
					return std::tie(phase, continuation, production);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 3>{{"phase", "continuation", "production"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&LRControlState::__init__);
				}
				constexpr auto methods() const {
					return std::make_tuple(&LRControlState::__init__);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 1>{{"__init__"}};
				}
			};
		} // namespace lr
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lr {
			struct LRRuleSpan : public ::ISPA_STD::Object {
				long long begin;
				long long end;
				bool leading;
				bool trailing;
				bool empty;
				auto __init__(long long begin, long long end, bool leading, bool trailing, bool empty) -> void;
				virtual auto copy() -> std::shared_ptr<python::ispa_std::lr::LRRuleSpan>;
				constexpr auto members() {
					return std::tie(begin, end, leading, trailing, empty);
				}
				constexpr auto members() const {
					return std::tie(begin, end, leading, trailing, empty);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 5>{{"begin", "end", "leading", "trailing", "empty"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&LRRuleSpan::__init__, &LRRuleSpan::copy);
				}
				constexpr auto methods() const {
					return std::make_tuple(&LRRuleSpan::__init__, &LRRuleSpan::copy);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 2>{{"__init__", "copy"}};
				}
			};
		} // namespace lr
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lr {
			struct LRRegionFrame : public ::ISPA_STD::Object {
				std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos;
				std::vector<long long> states;
				std::vector<std::any> values;
				std::vector<std::shared_ptr<python::ispa_std::lr::LRRuleSpan>> spans;
				std::vector<long long> capture_destinations;
				long long cursor;
				bool recovered_prefix;
				std::shared_ptr<python::ispa_std::lr::LRControlState> control;
				auto __init__(std::shared_ptr<python::ispa_std::lexer::TokenCursor> start, long long continuation) -> void;
				constexpr auto members() {
					return std::tie(pos, states, values, spans, capture_destinations, cursor, recovered_prefix, control);
				}
				constexpr auto members() const {
					return std::tie(pos, states, values, spans, capture_destinations, cursor, recovered_prefix, control);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 8>{{"pos", "states", "values", "spans", "capture_destinations", "cursor", "recovered_prefix", "control"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&LRRegionFrame::__init__);
				}
				constexpr auto methods() const {
					return std::make_tuple(&LRRegionFrame::__init__);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 1>{{"__init__"}};
				}
			};
		} // namespace lr
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lr {
			struct LRGrammar : public ::ISPA_STD::Object {
				virtual auto lr_k() -> long long;
				virtual auto lr_action(long long state, const std::vector<long long>& window) -> long long;
				virtual auto lr_conflict(long long state, const std::vector<long long>& window) -> std::vector<long long>;
				virtual auto lr_shift(std::shared_ptr<python::ispa_std::raw_token::RawToken> token) -> std::any;
				virtual auto lr_length(long long rule) -> long long;
				virtual auto lr_reduce(long long rule, const std::vector<std::any>& rhs) -> std::any;
				virtual auto lr_lhs(long long rule) -> long long;
				virtual auto lr_goto(long long state, long long lhs) -> long long;
				virtual auto has_is_space() -> bool;
				virtual auto lr_is_space(std::shared_ptr<python::ispa_std::raw_token::RawToken> token) -> bool;
				virtual auto has_space() -> bool;
				virtual auto lr_space(long long rule, long long index) -> bool;
				virtual auto has_expected() -> bool;
				virtual auto lr_expected(long long state) -> std::vector<long long>;
				virtual auto has_context() -> bool;
				virtual auto lr_context(long long state) -> std::string;
				virtual auto has_token_name() -> bool;
				virtual auto lr_token_name(long long id) -> std::string;
				virtual auto has_rule_label() -> bool;
				virtual auto lr_rule_label(long long rule) -> std::string;
				virtual auto has_recover() -> bool;
				virtual auto lr_recover(std::shared_ptr<python::ispa_std::lr::LRRegionFrame> region, std::shared_ptr<python::ispa_std::recovery::Diagnostic> diagnostic) -> bool;
				virtual auto has_semantic_reduce() -> bool;
				virtual auto lr_semantic_reduce(long long rule, std::shared_ptr<python::ispa_std::lr::LRRegionFrame> region, std::shared_ptr<python::ispa_std::lr::LRRuleSpan> span, const std::vector<std::any>& rhs) -> std::any;
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
					return std::make_tuple(&LRGrammar::lr_k, &LRGrammar::lr_action, &LRGrammar::lr_conflict, &LRGrammar::lr_shift, &LRGrammar::lr_length, &LRGrammar::lr_reduce, &LRGrammar::lr_lhs, &LRGrammar::lr_goto, &LRGrammar::has_is_space, &LRGrammar::lr_is_space, &LRGrammar::has_space, &LRGrammar::lr_space, &LRGrammar::has_expected, &LRGrammar::lr_expected, &LRGrammar::has_context, &LRGrammar::lr_context, &LRGrammar::has_token_name, &LRGrammar::lr_token_name, &LRGrammar::has_rule_label, &LRGrammar::lr_rule_label, &LRGrammar::has_recover, &LRGrammar::lr_recover, &LRGrammar::has_semantic_reduce, &LRGrammar::lr_semantic_reduce);
				}
				constexpr auto methods() const {
					return std::make_tuple(&LRGrammar::lr_k, &LRGrammar::lr_action, &LRGrammar::lr_conflict, &LRGrammar::lr_shift, &LRGrammar::lr_length, &LRGrammar::lr_reduce, &LRGrammar::lr_lhs, &LRGrammar::lr_goto, &LRGrammar::has_is_space, &LRGrammar::lr_is_space, &LRGrammar::has_space, &LRGrammar::lr_space, &LRGrammar::has_expected, &LRGrammar::lr_expected, &LRGrammar::has_context, &LRGrammar::lr_context, &LRGrammar::has_token_name, &LRGrammar::lr_token_name, &LRGrammar::has_rule_label, &LRGrammar::lr_rule_label, &LRGrammar::has_recover, &LRGrammar::lr_recover, &LRGrammar::has_semantic_reduce, &LRGrammar::lr_semantic_reduce);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 24>{{"lr_k", "lr_action", "lr_conflict", "lr_shift", "lr_length", "lr_reduce", "lr_lhs", "lr_goto", "has_is_space", "lr_is_space", "has_space", "lr_space", "has_expected", "lr_expected", "has_context", "lr_context", "has_token_name", "lr_token_name", "has_rule_label", "lr_rule_label", "has_recover", "lr_recover", "has_semantic_reduce", "lr_semantic_reduce"}};
				}
			};
		} // namespace lr
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lr {
			struct LRTokenValue : public ::ISPA_STD::Object {
				std::shared_ptr<python::ispa_std::node::Node> node;
				std::string text;
				auto __init__(std::shared_ptr<python::ispa_std::node::Node> node, const std::string& text) -> void;
				constexpr auto members() {
					return std::tie(node, text);
				}
				constexpr auto members() const {
					return std::tie(node, text);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 2>{{"node", "text"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&LRTokenValue::__init__);
				}
				constexpr auto methods() const {
					return std::make_tuple(&LRTokenValue::__init__);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 1>{{"__init__"}};
				}
			};
		} // namespace lr
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lr {
			auto lr_token(std::shared_ptr<python::ispa_std::node::Node> node) -> std::shared_ptr<python::ispa_std::lr::LRTokenValue>;
		} // namespace lr
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lr {
			struct LRCaptures : public ::ISPA_STD::Object {
				std::vector<std::vector<std::any>> columns;
				bool repeated;
				auto __init__(const std::vector<std::vector<std::any>>& columns, bool repeated) -> void;
				virtual auto is_empty() -> bool;
				constexpr auto members() {
					return std::tie(columns, repeated);
				}
				constexpr auto members() const {
					return std::tie(columns, repeated);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 2>{{"columns", "repeated"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&LRCaptures::__init__, &LRCaptures::is_empty);
				}
				constexpr auto methods() const {
					return std::make_tuple(&LRCaptures::__init__, &LRCaptures::is_empty);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 2>{{"__init__", "is_empty"}};
				}
			};
		} // namespace lr
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lr {
			auto lr_pack(const std::vector<std::any>& values, const std::vector<long long>& indices, const std::vector<long long>& offsets, const std::vector<long long>& splice, long long count, bool repeated) -> std::shared_ptr<python::ispa_std::lr::LRCaptures>;
		} // namespace lr
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lr {
			auto lr_get(const std::vector<std::any>& values, long long index) -> std::any;
		} // namespace lr
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lr {
			auto lr_node(std::any value) -> std::shared_ptr<python::ispa_std::node::Node>;
		} // namespace lr
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lr {
			struct LRParser : public ::ISPA_STD::Object {
				std::vector<std::shared_ptr<python::ispa_std::recovery::Diagnostic>> errors_;
				auto __init__() -> void;
				virtual auto getErrors() -> const std::vector<std::shared_ptr<python::ispa_std::recovery::Diagnostic>>&;
				virtual auto _skip(std::shared_ptr<python::ispa_std::lr::LRGrammar> g, std::shared_ptr<python::ispa_std::lexer::TokenCursor> it, std::vector<long long>& offset) -> void;
				virtual auto _gap(std::shared_ptr<python::ispa_std::lr::LRGrammar> g, long long rule, long long index) -> bool;
				virtual auto _window(std::shared_ptr<python::ispa_std::lr::LRGrammar> g, std::shared_ptr<python::ispa_std::lexer::TokenCursor> position) -> std::vector<long long>;
				virtual auto _unexpected(std::shared_ptr<python::ispa_std::lr::LRGrammar> g, const std::vector<long long>& states, const std::vector<long long>& window, std::shared_ptr<python::ispa_std::lexer::TokenCursor> position) -> std::shared_ptr<python::ispa_std::recovery::Diagnostic>;
				virtual auto _reduce_span(std::shared_ptr<python::ispa_std::lr::LRGrammar> g, long long rule, long long count, long long begin, const std::vector<std::shared_ptr<python::ispa_std::lr::LRRuleSpan>>& spans, long long cursor) -> std::shared_ptr<python::ispa_std::lr::LRRuleSpan>;
				virtual auto lr_parse(std::shared_ptr<python::ispa_std::lr::LRGrammar> g, std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> std::shared_ptr<python::ispa_std::match_result::MatchResult>;
				constexpr auto members() {
					return std::tie(errors_);
				}
				constexpr auto members() const {
					return std::tie(errors_);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 1>{{"errors_"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&LRParser::__init__, &LRParser::getErrors, &LRParser::_skip, &LRParser::_gap, &LRParser::_window, &LRParser::_unexpected, &LRParser::_reduce_span, &LRParser::lr_parse);
				}
				constexpr auto methods() const {
					return std::make_tuple(&LRParser::__init__, &LRParser::getErrors, &LRParser::_skip, &LRParser::_gap, &LRParser::_window, &LRParser::_unexpected, &LRParser::_reduce_span, &LRParser::lr_parse);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 8>{{"__init__", "getErrors", "_skip", "_gap", "_window", "_unexpected", "_reduce_span", "lr_parse"}};
				}
			};
		} // namespace lr
	} // namespace ispa_std
} // namespace python
namespace python::ispa_std::lr_conflicts { struct _Span; }
namespace python::ispa_std::lr_conflicts { struct _Alternative; }
namespace python::ispa_std::lr_conflicts { struct _Eval; }
namespace python::ispa_std::lr_conflicts { struct _Forest; }
namespace python::ispa_std::lr_conflicts { struct _Stack; }
namespace python::ispa_std::lr_conflicts { struct LRGeneralizedParser; }
namespace python {
	namespace ispa_std {
		namespace lr_conflicts {
			inline long long NODE_LIMIT = 250000;
		} // namespace lr_conflicts
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lr_conflicts {
			inline long long WORK_LIMIT = 2000000;
		} // namespace lr_conflicts
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lr_conflicts {
			struct _Span : public ::ISPA_STD::Object {
				long long begin;
				long long end;
				bool leading;
				bool trailing;
				bool empty;
				auto __init__(long long begin, long long end, bool leading, bool trailing, bool empty) -> void;
				constexpr auto members() {
					return std::tie(begin, end, leading, trailing, empty);
				}
				constexpr auto members() const {
					return std::tie(begin, end, leading, trailing, empty);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 5>{{"begin", "end", "leading", "trailing", "empty"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&_Span::__init__);
				}
				constexpr auto methods() const {
					return std::make_tuple(&_Span::__init__);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 1>{{"__init__"}};
				}
			};
		} // namespace lr_conflicts
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lr_conflicts {
			struct _Alternative : public ::ISPA_STD::Object {
				long long rule;
				std::vector<long long> children;
				auto __init__(long long rule, const std::vector<long long>& children) -> void;
				virtual auto equals(std::shared_ptr<python::ispa_std::lr_conflicts::_Alternative> other) -> bool;
				constexpr auto members() {
					return std::tie(rule, children);
				}
				constexpr auto members() const {
					return std::tie(rule, children);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 2>{{"rule", "children"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&_Alternative::__init__, &_Alternative::equals);
				}
				constexpr auto methods() const {
					return std::make_tuple(&_Alternative::__init__, &_Alternative::equals);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 2>{{"__init__", "equals"}};
				}
			};
		} // namespace lr_conflicts
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lr_conflicts {
			struct _Eval : public ::ISPA_STD::Object {
				long long node;
				long long child;
				std::vector<std::any> rhs;
				auto __init__(long long node) -> void;
				constexpr auto members() {
					return std::tie(node, child, rhs);
				}
				constexpr auto members() const {
					return std::tie(node, child, rhs);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 3>{{"node", "child", "rhs"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&_Eval::__init__);
				}
				constexpr auto methods() const {
					return std::make_tuple(&_Eval::__init__);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 1>{{"__init__"}};
				}
			};
		} // namespace lr_conflicts
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lr_conflicts {
			struct _Forest : public ::ISPA_STD::Object {
				std::shared_ptr<python::ispa_std::lr_conflicts::_Span> span;
				bool has_token;
				std::shared_ptr<python::ispa_std::lexer::TokenCursor> token;
				std::vector<std::shared_ptr<python::ispa_std::lr_conflicts::_Alternative>> alternatives;
				auto __init__(std::shared_ptr<python::ispa_std::lr_conflicts::_Span> span, bool has_token, std::shared_ptr<python::ispa_std::lexer::TokenCursor> token) -> void;
				constexpr auto members() {
					return std::tie(span, has_token, token, alternatives);
				}
				constexpr auto members() const {
					return std::tie(span, has_token, token, alternatives);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 4>{{"span", "has_token", "token", "alternatives"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&_Forest::__init__);
				}
				constexpr auto methods() const {
					return std::make_tuple(&_Forest::__init__);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 1>{{"__init__"}};
				}
			};
		} // namespace lr_conflicts
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lr_conflicts {
			struct _Stack : public ::ISPA_STD::Object {
				long long previous;
				long long state;
				long long forest;
				auto __init__(long long previous, long long state, long long forest) -> void;
				constexpr auto members() {
					return std::tie(previous, state, forest);
				}
				constexpr auto members() const {
					return std::tie(previous, state, forest);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 3>{{"previous", "state", "forest"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&_Stack::__init__);
				}
				constexpr auto methods() const {
					return std::make_tuple(&_Stack::__init__);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 1>{{"__init__"}};
				}
			};
		} // namespace lr_conflicts
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lr_conflicts {
			auto _b(bool flag) -> std::string;
		} // namespace lr_conflicts
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lr_conflicts {
			struct LRGeneralizedParser : public python::ispa_std::lr::LRParser {
				long long work;
				auto __init__() -> void;
				virtual auto _tick(const std::vector<std::shared_ptr<python::ispa_std::lr_conflicts::_Forest>>& forest, const std::vector<std::shared_ptr<python::ispa_std::lr_conflicts::_Stack>>& stacks) -> void;
				virtual auto lr_parse_generalized(std::shared_ptr<python::ispa_std::lr::LRGrammar> g, std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> std::shared_ptr<python::ispa_std::match_result::MatchResult>;
				virtual auto _key(const std::vector<std::string>& parts) -> std::string;
				virtual auto _intern(std::vector<std::shared_ptr<python::ispa_std::lr_conflicts::_Forest>>& forest, std::shared_ptr<python::ispa_std::sortedmap::StrIntMap> index, bool terminal, long long symbol, std::shared_ptr<python::ispa_std::lr_conflicts::_Span> span, bool has_token, std::shared_ptr<python::ispa_std::lexer::TokenCursor> token) -> long long;
				virtual auto _push(std::vector<std::shared_ptr<python::ispa_std::lr_conflicts::_Stack>>& stacks, std::shared_ptr<python::ispa_std::sortedmap::StrIntMap> index, long long previous, long long state, long long value) -> long long;
				constexpr auto members() {
					return std::tie(work);
				}
				constexpr auto members() const {
					return std::tie(work);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 1>{{"work"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&LRGeneralizedParser::__init__, &LRGeneralizedParser::_tick, &LRGeneralizedParser::lr_parse_generalized, &LRGeneralizedParser::_key, &LRGeneralizedParser::_intern, &LRGeneralizedParser::_push);
				}
				constexpr auto methods() const {
					return std::make_tuple(&LRGeneralizedParser::__init__, &LRGeneralizedParser::_tick, &LRGeneralizedParser::lr_parse_generalized, &LRGeneralizedParser::_key, &LRGeneralizedParser::_intern, &LRGeneralizedParser::_push);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 6>{{"__init__", "_tick", "lr_parse_generalized", "_key", "_intern", "_push"}};
				}
			};
		} // namespace lr_conflicts
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace lr_conflicts {
			auto min2(long long a, long long b) -> long long;
		} // namespace lr_conflicts
	} // namespace ispa_std
} // namespace python
namespace python::ispa_std::external { struct ExternalResult; }
namespace python::ispa_std::external { struct ExternalBinding; }
namespace python {
	namespace ispa_std {
		namespace external {
			struct ExternalResult : public ::ISPA_STD::Object {
				bool status;
				std::shared_ptr<python::ispa_std::node::Node> node;
				long long end;
				auto __init__(bool status, std::shared_ptr<python::ispa_std::node::Node> node, long long end) -> void;
				static auto failed() -> std::shared_ptr<python::ispa_std::external::ExternalResult>;
				constexpr auto members() {
					return std::tie(status, node, end);
				}
				constexpr auto members() const {
					return std::tie(status, node, end);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 3>{{"status", "node", "end"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&ExternalResult::__init__, &ExternalResult::failed);
				}
				constexpr auto methods() const {
					return std::make_tuple(&ExternalResult::__init__, &ExternalResult::failed);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 2>{{"__init__", "failed"}};
				}
			};
		} // namespace external
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace external {
			struct ExternalBinding : public ::ISPA_STD::Object {
				virtual auto starts(const std::string& origin, long long pos) -> bool;
				virtual auto parse(const std::string& origin, long long pos) -> std::shared_ptr<python::ispa_std::external::ExternalResult>;
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
					return std::make_tuple(&ExternalBinding::starts, &ExternalBinding::parse);
				}
				constexpr auto methods() const {
					return std::make_tuple(&ExternalBinding::starts, &ExternalBinding::parse);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 2>{{"starts", "parse"}};
				}
			};
		} // namespace external
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace external {
			auto external_node(const std::string& origin, long long start, long long end, std::shared_ptr<python::ispa_std::node::Node> node) -> std::shared_ptr<python::ispa_std::node::Node>;
		} // namespace external
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace external {
			auto _require_lazy(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> void;
		} // namespace external
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace external {
			auto external_starts(std::shared_ptr<python::ispa_std::external::ExternalBinding> binding, std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> bool;
		} // namespace external
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace external {
			auto external_match(std::shared_ptr<python::ispa_std::external::ExternalBinding> binding, std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> std::shared_ptr<python::ispa_std::match_result::MatchResult>;
		} // namespace external
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace optional {
			template<typename T>
			struct Optional : public ::ISPA_STD::Object {
				std::vector<T> _items;
				auto __init__() -> void {
					((*this))._items = {};
				}
				static auto some(T value) -> std::shared_ptr<python::ispa_std::optional::Optional<T>> {
					auto result = ISPA_STD::construct<python::ispa_std::optional::Optional<T>>();
					((result)._items).push_back(value);
					return result;
				}
				static auto none() -> std::shared_ptr<python::ispa_std::optional::Optional<T>> {
					return ISPA_STD::construct<python::ispa_std::optional::Optional<T>>();
				}
				virtual auto has_value() -> bool {
					return ((((*this))._items.size()) == (1));
				}
				virtual auto value() -> T {
					if (((((*this))._items.size()) == (0))) {
						::ISPA_STD::fail(python::ispa_std::errors::ErrorKind::BadOptionalAccess, "Optional.value: no value");
					}
					return (((*this))._items)[0];
				}
				virtual auto value_or(T fallback) -> T {
					if (((((*this))._items.size()) == (1))) {
						return (((*this))._items)[0];
					}
					return fallback;
				}
				virtual auto reset() -> void {
					(((*this))._items).clear();
				}
				virtual auto emplace(T value) -> void {
					(((*this))._items).clear();
					(((*this))._items).push_back(value);
				}
				template<typename U> auto transform(std::function<U(T)> fn) -> std::shared_ptr<python::ispa_std::optional::Optional<U>> {
					if (((((*this))._items.size()) == (1))) {
						auto result = ISPA_STD::construct<python::ispa_std::optional::Optional<U>>();
						((result)._items).push_back(fn((((*this))._items)[0]));
						return result;
					}
					return ISPA_STD::construct<python::ispa_std::optional::Optional<U>>();
				}
				template<typename U> auto and_then(std::function<std::shared_ptr<python::ispa_std::optional::Optional<U>>(T)> fn) -> std::shared_ptr<python::ispa_std::optional::Optional<U>> {
					if (((((*this))._items.size()) == (1))) {
						return fn((((*this))._items)[0]);
					}
					return ISPA_STD::construct<python::ispa_std::optional::Optional<U>>();
				}
				virtual auto or_else(std::function<std::shared_ptr<python::ispa_std::optional::Optional<T>>()> fn) -> std::shared_ptr<python::ispa_std::optional::Optional<T>> {
					if (((((*this))._items.size()) == (1))) {
						return ::ISPA_STD::self_ptr(this);
					}
					return fn();
				}
				virtual auto __eq__(std::shared_ptr<python::ispa_std::optional::Optional<T>> other) -> bool {
					if (((((*this))._items.size()) != (((*other))._items.size()))) {
						return false;
					}
					if (((((*this))._items.size()) == (0))) {
						return true;
					}
					return (((((*this))._items)[0]) == ((((*other))._items)[0]));
				}
				constexpr auto members() {
					return std::tie(_items);
				}
				constexpr auto members() const {
					return std::tie(_items);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 1>{{"_items"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&Optional::__init__, &Optional::some, &Optional::none, &Optional::has_value, &Optional::value, &Optional::value_or, &Optional::reset, &Optional::emplace, &Optional::transform, &Optional::and_then, &Optional::or_else, &Optional::__eq__);
				}
				constexpr auto methods() const {
					return std::make_tuple(&Optional::__init__, &Optional::some, &Optional::none, &Optional::has_value, &Optional::value, &Optional::value_or, &Optional::reset, &Optional::emplace, &Optional::transform, &Optional::and_then, &Optional::or_else, &Optional::__eq__);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 12>{{"__init__", "some", "none", "has_value", "value", "value_or", "reset", "emplace", "transform", "and_then", "or_else", "__eq__"}};
				}
			};
		} // namespace optional
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace map {
			template<typename K, typename V> auto contains(const std::unordered_map<K, V>& m, K key) -> bool;
		} // namespace map
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace map {
			template<typename K, typename V> auto at(const std::unordered_map<K, V>& m, K key) -> V;
		} // namespace map
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace map {
			template<typename K, typename V> auto get(const std::unordered_map<K, V>& m, K key) -> std::shared_ptr<python::ispa_std::optional::Optional<V>>;
		} // namespace map
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace map {
			template<typename K, typename V> auto insert(std::unordered_map<K, V>& m, K key, V value) -> bool;
		} // namespace map
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace map {
			template<typename K, typename V> auto insert_or_assign(std::unordered_map<K, V>& m, K key, V value) -> bool;
		} // namespace map
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace map {
			template<typename K, typename V> auto erase(std::unordered_map<K, V>& m, K key) -> bool;
		} // namespace map
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace span {
			template<typename T>
			struct Span : public ::ISPA_STD::Object {
				std::vector<T> _arr;
				long long _begin;
				long long _end;
				auto __init__(const std::vector<T>& arr, long long begin, long long end) -> void {
					if (((((((begin) < (0))) || (((end) < (begin))))) || (((end) > (arr.size()))))) {
						::ISPA_STD::fail(python::ispa_std::errors::ErrorKind::OutOfRange, "Span: window outside array");
					}
					((*this))._arr = arr;
					((*this))._begin = begin;
					((*this))._end = end;
				}
				virtual auto data() -> const std::vector<T>& {
					return ((*this))._arr;
				}
				virtual auto begin() -> long long {
					return ((*this))._begin;
				}
				virtual auto end() -> long long {
					return ((*this))._end;
				}
				virtual auto size() -> long long {
					return ((((*this))._end) - (((*this))._begin));
				}
				virtual auto empty() -> bool {
					return ((((*this))._end) == (((*this))._begin));
				}
				virtual auto at(long long index) -> T {
					if (((((index) < (0))) || (((index) >= (((((*this))._end) - (((*this))._begin))))))) {
						::ISPA_STD::fail(python::ispa_std::errors::ErrorKind::OutOfRange, "Span.at: index out of range");
					}
					return (((*this))._arr)[((((*this))._begin) + (index))];
				}
				virtual auto set(long long index, T value) -> void {
					if (((((index) < (0))) || (((index) >= (((((*this))._end) - (((*this))._begin))))))) {
						::ISPA_STD::fail(python::ispa_std::errors::ErrorKind::OutOfRange, "Span.set: index out of range");
					}
					(((*this))._arr)[((((*this))._begin) + (index))] = value;
				}
				virtual auto first(long long count) -> std::shared_ptr<python::ispa_std::span::Span<T>> {
					if (((((count) < (0))) || (((count) > (((((*this))._end) - (((*this))._begin))))))) {
						::ISPA_STD::fail(python::ispa_std::errors::ErrorKind::OutOfRange, "Span.first: count out of range");
					}
					return ISPA_STD::construct<python::ispa_std::span::Span>(((*this))._arr, ((*this))._begin, ((((*this))._begin) + (count)));
				}
				virtual auto last(long long count) -> std::shared_ptr<python::ispa_std::span::Span<T>> {
					if (((((count) < (0))) || (((count) > (((((*this))._end) - (((*this))._begin))))))) {
						::ISPA_STD::fail(python::ispa_std::errors::ErrorKind::OutOfRange, "Span.last: count out of range");
					}
					return ISPA_STD::construct<python::ispa_std::span::Span>(((*this))._arr, ((((*this))._end) - (count)), ((*this))._end);
				}
				virtual auto subspan(long long offset, long long count) -> std::shared_ptr<python::ispa_std::span::Span<T>> {
					if (((((((offset) < (0))) || (((count) < (0))))) || (((((offset) + (count))) > (((((*this))._end) - (((*this))._begin))))))) {
						::ISPA_STD::fail(python::ispa_std::errors::ErrorKind::OutOfRange, "Span.subspan: window out of range");
					}
					return ISPA_STD::construct<python::ispa_std::span::Span>(((*this))._arr, ((((*this))._begin) + (offset)), ((((((*this))._begin) + (offset))) + (count)));
				}
				constexpr auto members() {
					return std::tie(_arr, _begin, _end);
				}
				constexpr auto members() const {
					return std::tie(_arr, _begin, _end);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 3>{{"_arr", "_begin", "_end"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&Span::__init__, &Span::data, &Span::begin, &Span::end, &Span::size, &Span::empty, &Span::at, &Span::set, &Span::first, &Span::last, &Span::subspan);
				}
				constexpr auto methods() const {
					return std::make_tuple(&Span::__init__, &Span::data, &Span::begin, &Span::end, &Span::size, &Span::empty, &Span::at, &Span::set, &Span::first, &Span::last, &Span::subspan);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 11>{{"__init__", "data", "begin", "end", "size", "empty", "at", "set", "first", "last", "subspan"}};
				}
			};
		} // namespace span
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace vector {
			template<typename T> auto filled(long long count, T value) -> std::vector<T>;
		} // namespace vector
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace vector {
			template<typename T> auto at(const std::vector<T>& arr, long long index) -> T;
		} // namespace vector
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace vector {
			template<typename T> auto front(const std::vector<T>& arr) -> T;
		} // namespace vector
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace vector {
			template<typename T> auto back(const std::vector<T>& arr) -> T;
		} // namespace vector
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace vector {
			template<typename T> auto pop_back(std::vector<T>& arr) -> void;
		} // namespace vector
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace vector {
			template<typename T> auto insert_at(std::vector<T>& arr, long long index, T value) -> void;
		} // namespace vector
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace vector {
			template<typename T> auto erase_at(std::vector<T>& arr, long long index) -> void;
		} // namespace vector
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace vector {
			template<typename T> auto erase_range(std::vector<T>& arr, long long first, long long last) -> void;
		} // namespace vector
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace vector {
			template<typename T> auto resize(std::vector<T>& arr, long long count, T fill) -> void;
		} // namespace vector
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace vector {
			template<typename T> auto assign(std::vector<T>& arr, long long count, T value) -> void;
		} // namespace vector
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace vector {
			template<typename T> auto swap(std::vector<T>& a, std::vector<T>& b) -> void;
		} // namespace vector
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace vector {
			template<typename T> auto lexicographical_less(const std::vector<T>& a, const std::vector<T>& b) -> bool;
		} // namespace vector
	} // namespace ispa_std
} // namespace python
namespace python::ispa_std::ast_printer { struct ASTPrinter; }
namespace python {
	namespace ispa_std {
		namespace ast_printer {
			struct ASTPrinter : public ::ISPA_STD::Object {
				std::vector<std::string> _out;
				std::vector<bool> _branches;
				auto __init__() -> void;
				virtual auto text() -> std::string;
				virtual auto up(bool has_more_siblings) -> void;
				virtual auto down() -> void;
				virtual auto _prefix(bool last) -> void;
				virtual auto node(const std::string& name, bool last) -> void;
				virtual auto scalar(const std::string& name, const std::string& text, bool last) -> void;
				constexpr auto members() {
					return std::tie(_out, _branches);
				}
				constexpr auto members() const {
					return std::tie(_out, _branches);
				}
				static constexpr auto members_str() {
					return std::array<std::string_view, 2>{{"_out", "_branches"}};
				}
				constexpr auto methods() {
					return std::make_tuple(&ASTPrinter::__init__, &ASTPrinter::text, &ASTPrinter::up, &ASTPrinter::down, &ASTPrinter::_prefix, &ASTPrinter::node, &ASTPrinter::scalar);
				}
				constexpr auto methods() const {
					return std::make_tuple(&ASTPrinter::__init__, &ASTPrinter::text, &ASTPrinter::up, &ASTPrinter::down, &ASTPrinter::_prefix, &ASTPrinter::node, &ASTPrinter::scalar);
				}
				static constexpr auto methods_str() {
					return std::array<std::string_view, 7>{{"__init__", "text", "up", "down", "_prefix", "node", "scalar"}};
				}
			};
		} // namespace ast_printer
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace to_string {
			auto str_to_string(const std::string& value) -> std::string;
		} // namespace to_string
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace to_string {
			auto bool_value_to_string(bool value) -> std::string;
		} // namespace to_string
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace to_string {
			auto int_value_to_string(long long value) -> std::string;
		} // namespace to_string
	} // namespace ispa_std
} // namespace python
namespace python {
	namespace ispa_std {
		namespace to_string {
			template<typename T> auto list_to_string(const std::vector<T>& items, std::function<std::string(T)> element_to_string) -> std::string;
		} // namespace to_string
	} // namespace ispa_std
} // namespace python
template<typename A, typename B> auto python::ispa_std::utility::make_pair(A first, B second) -> std::shared_ptr<python::ispa_std::utility::Pair<A, B>> {
	return ISPA_STD::construct<python::ispa_std::utility::Pair<A, B>>(first, second);
}
template<typename T> auto python::ispa_std::utility::minimum(T a, T b) -> T {
	if (((b) < (a))) {
		return b;
	}
	return a;
}
template<typename T> auto python::ispa_std::utility::maximum(T a, T b) -> T {
	if (((a) < (b))) {
		return b;
	}
	return a;
}
template<typename T> auto python::ispa_std::utility::clamp(T value, T low, T high) -> T {
	if (((value) < (low))) {
		return low;
	}
	if (((high) < (value))) {
		return high;
	}
	return value;
}
template<typename T> auto python::ispa_std::algorithm::_check(const std::vector<T>& arr, long long first, long long last) -> void {
	if (((((((first) < (0))) || (((last) < (first))))) || (((last) > (arr.size()))))) {
		::ISPA_STD::fail(python::ispa_std::errors::ErrorKind::OutOfRange, "algorithm: range outside array");
	}
}
template<typename T> auto python::ispa_std::algorithm::for_each(const std::vector<T>& arr, long long first, long long last, std::function<void(T)> fn) -> void {
	python::ispa_std::algorithm::_check(arr, first, last);
	long long i = first;
	while (((i) < (last))) {
		fn((arr)[i]);
		i = ((i) + (1));
	}
}
template<typename T> auto python::ispa_std::algorithm::find(const std::vector<T>& arr, long long first, long long last, T value) -> long long {
	python::ispa_std::algorithm::_check(arr, first, last);
	long long i = first;
	while (((i) < (last))) {
		if ((((arr)[i]) == (value))) {
			return i;
		}
		i = ((i) + (1));
	}
	return ( - (1));
}
template<typename T> auto python::ispa_std::algorithm::find_if(const std::vector<T>& arr, long long first, long long last, std::function<bool(T)> pred) -> long long {
	python::ispa_std::algorithm::_check(arr, first, last);
	long long i = first;
	while (((i) < (last))) {
		if (pred((arr)[i])) {
			return i;
		}
		i = ((i) + (1));
	}
	return ( - (1));
}
template<typename T> auto python::ispa_std::algorithm::count(const std::vector<T>& arr, long long first, long long last, T value) -> long long {
	long long total = 0;
	long long i = first;
	python::ispa_std::algorithm::_check(arr, first, last);
	while (((i) < (last))) {
		if ((((arr)[i]) == (value))) {
			total = ((total) + (1));
		}
		i = ((i) + (1));
	}
	return total;
}
template<typename T> auto python::ispa_std::algorithm::count_if(const std::vector<T>& arr, long long first, long long last, std::function<bool(T)> pred) -> long long {
	long long total = 0;
	long long i = first;
	python::ispa_std::algorithm::_check(arr, first, last);
	while (((i) < (last))) {
		if (pred((arr)[i])) {
			total = ((total) + (1));
		}
		i = ((i) + (1));
	}
	return total;
}
template<typename T> auto python::ispa_std::algorithm::all_of(const std::vector<T>& arr, long long first, long long last, std::function<bool(T)> pred) -> bool {
	return ((python::ispa_std::algorithm::count_if(arr, first, last, pred)) == (((last) - (first))));
}
template<typename T> auto python::ispa_std::algorithm::any_of(const std::vector<T>& arr, long long first, long long last, std::function<bool(T)> pred) -> bool {
	return ((python::ispa_std::algorithm::find_if(arr, first, last, pred)) != (( - (1))));
}
template<typename T> auto python::ispa_std::algorithm::none_of(const std::vector<T>& arr, long long first, long long last, std::function<bool(T)> pred) -> bool {
	return ((python::ispa_std::algorithm::find_if(arr, first, last, pred)) == (( - (1))));
}
template<typename T> auto python::ispa_std::algorithm::equal(const std::vector<T>& a, long long a_first, long long a_last, const std::vector<T>& b, long long b_first) -> bool {
	python::ispa_std::algorithm::_check(a, a_first, a_last);
	python::ispa_std::algorithm::_check(b, b_first, ((b_first) + (((a_last) - (a_first)))));
	long long i = 0;
	while (((i) < (((a_last) - (a_first))))) {
		if ((((a)[((a_first) + (i))]) != ((b)[((b_first) + (i))]))) {
			return false;
		}
		i = ((i) + (1));
	}
	return true;
}
template<typename T, typename U> auto python::ispa_std::algorithm::accumulate(const std::vector<T>& arr, long long first, long long last, U init, std::function<U(U, T)> fn) -> U {
	python::ispa_std::algorithm::_check(arr, first, last);
	auto total = init;
	long long i = first;
	while (((i) < (last))) {
		total = fn(total, (arr)[i]);
		i = ((i) + (1));
	}
	return total;
}
template<typename T, typename U> auto python::ispa_std::algorithm::transform(const std::vector<T>& arr, long long first, long long last, std::function<U(T)> fn) -> std::vector<U> {
	python::ispa_std::algorithm::_check(arr, first, last);
	std::vector<U> result = std::vector<U>{};
	long long i = first;
	while (((i) < (last))) {
		(result).push_back(fn((arr)[i]));
		i = ((i) + (1));
	}
	return result;
}
template<typename T> auto python::ispa_std::algorithm::fill(std::vector<T>& arr, long long first, long long last, T value) -> void {
	python::ispa_std::algorithm::_check(arr, first, last);
	long long i = first;
	while (((i) < (last))) {
		(arr)[i] = value;
		i = ((i) + (1));
	}
}
template<typename T> auto python::ispa_std::algorithm::reverse(std::vector<T>& arr, long long first, long long last) -> void {
	python::ispa_std::algorithm::_check(arr, first, last);
	long long i = first;
	long long j = ((last) - (1));
	while (((i) < (j))) {
		auto tmp = (arr)[i];
		(arr)[i] = (arr)[j];
		(arr)[j] = tmp;
		i = ((i) + (1));
		j = ((j) - (1));
	}
}
template<typename T> auto python::ispa_std::algorithm::min_element(const std::vector<T>& arr, long long first, long long last) -> long long {
	python::ispa_std::algorithm::_check(arr, first, last);
	if (((first) == (last))) {
		return ( - (1));
	}
	long long best = first;
	long long i = ((first) + (1));
	while (((i) < (last))) {
		if ((((arr)[i]) < ((arr)[best]))) {
			best = i;
		}
		i = ((i) + (1));
	}
	return best;
}
template<typename T> auto python::ispa_std::algorithm::max_element(const std::vector<T>& arr, long long first, long long last) -> long long {
	python::ispa_std::algorithm::_check(arr, first, last);
	if (((first) == (last))) {
		return ( - (1));
	}
	long long best = first;
	long long i = ((first) + (1));
	while (((i) < (last))) {
		if ((((arr)[best]) < ((arr)[i]))) {
			best = i;
		}
		i = ((i) + (1));
	}
	return best;
}
template<typename T> auto python::ispa_std::algorithm::is_sorted(const std::vector<T>& arr, long long first, long long last) -> bool {
	python::ispa_std::algorithm::_check(arr, first, last);
	long long i = ((first) + (1));
	while (((i) < (last))) {
		if ((((arr)[i]) < ((arr)[((i) - (1))]))) {
			return false;
		}
		i = ((i) + (1));
	}
	return true;
}
template<typename T> auto python::ispa_std::algorithm::lower_bound(const std::vector<T>& arr, long long first, long long last, T value) -> long long {
	python::ispa_std::algorithm::_check(arr, first, last);
	long long lo = first;
	long long hi = last;
	while (((lo) < (hi))) {
		long long mid = ((lo) + (((((hi) - (lo))) / (2))));
		if ((((arr)[mid]) < (value))) {
			lo = ((mid) + (1));
		} else {
			hi = mid;
		}
	}
	return lo;
}
template<typename T> auto python::ispa_std::algorithm::upper_bound(const std::vector<T>& arr, long long first, long long last, T value) -> long long {
	python::ispa_std::algorithm::_check(arr, first, last);
	long long lo = first;
	long long hi = last;
	while (((lo) < (hi))) {
		long long mid = ((lo) + (((((hi) - (lo))) / (2))));
		if (((value) < ((arr)[mid]))) {
			hi = mid;
		} else {
			lo = ((mid) + (1));
		}
	}
	return lo;
}
template<typename T> auto python::ispa_std::algorithm::binary_search(const std::vector<T>& arr, long long first, long long last, T value) -> bool {
	long long pos = python::ispa_std::algorithm::lower_bound(arr, first, last, value);
	return ((((pos) < (last))) && ((!(((value) < ((arr)[pos]))))));
}
template<typename T> auto python::ispa_std::algorithm::stable_sort(std::vector<T>& arr, long long first, long long last, std::function<bool(T, T)> less) -> void {
	python::ispa_std::algorithm::_check(arr, first, last);
	long long n = ((last) - (first));
	if (((n) < (2))) {
		return ;
	}
	std::vector<T> tmp = std::vector<T>{};
	long long i = 0;
	while (((i) < (n))) {
		(tmp).push_back((arr)[((first) + (i))]);
		i = ((i) + (1));
	}
	long long width = 1;
	while (((width) < (n))) {
		long long lo = 0;
		while (((lo) < (n))) {
			long long mid = ((lo) + (width));
			if (((mid) > (n))) {
				mid = n;
			}
			long long hi = ((lo) + (((2) * (width))));
			if (((hi) > (n))) {
				hi = n;
			}
			long long a = lo;
			long long b = mid;
			long long k = lo;
			while (((((a) < (mid))) && (((b) < (hi))))) {
				if (less((arr)[((first) + (b))], (arr)[((first) + (a))])) {
					(tmp)[k] = (arr)[((first) + (b))];
					b = ((b) + (1));
				} else {
					(tmp)[k] = (arr)[((first) + (a))];
					a = ((a) + (1));
				}
				k = ((k) + (1));
			}
			while (((a) < (mid))) {
				(tmp)[k] = (arr)[((first) + (a))];
				a = ((a) + (1));
				k = ((k) + (1));
			}
			while (((b) < (hi))) {
				(tmp)[k] = (arr)[((first) + (b))];
				b = ((b) + (1));
				k = ((k) + (1));
			}
			lo = ((lo) + (((2) * (width))));
		}
		i = 0;
		while (((i) < (n))) {
			(arr)[((first) + (i))] = (tmp)[i];
			i = ((i) + (1));
		}
		width = ((width) * (2));
	}
}
template<typename T> auto python::ispa_std::algorithm::sort(const std::vector<T>& arr, long long first, long long last) -> void {
	python::ispa_std::algorithm::stable_sort(arr, first, last, [&](auto a, auto b) {
return ((a) < (b));
});
}
template<typename T> auto python::ispa_std::algorithm::erase_if(std::vector<T>& arr, std::function<bool(T)> pred) -> long long {
	long long n = arr.size();
	std::vector<T> kept = std::vector<T>{};
	long long i = 0;
	while (((i) < (n))) {
		if ((!(pred((arr)[i])))) {
			(kept).push_back((arr)[i]);
		}
		i = ((i) + (1));
	}
	(arr).clear();
	ISPA_STD::python_extend((arr), kept);
	return ((n) - (kept.size()));
}
template<typename K, typename V> auto python::ispa_std::map::contains(const std::unordered_map<K, V>& m, K key) -> bool {
	return ISPA_STD::python_contains((m), key);
}
template<typename K, typename V> auto python::ispa_std::map::at(const std::unordered_map<K, V>& m, K key) -> V {
	if ((!(ISPA_STD::python_contains((m), key)))) {
		::ISPA_STD::fail(python::ispa_std::errors::ErrorKind::KeyNotFound, "map.at: key not found");
	}
	return (m)[key];
}
template<typename K, typename V> auto python::ispa_std::map::get(const std::unordered_map<K, V>& m, K key) -> std::shared_ptr<python::ispa_std::optional::Optional<V>> {
	auto result = ISPA_STD::construct<python::ispa_std::optional::Optional<V>>();
	if (ISPA_STD::python_contains((m), key)) {
		(result).emplace((m)[key]);
	}
	return result;
}
template<typename K, typename V> auto python::ispa_std::map::insert(std::unordered_map<K, V>& m, K key, V value) -> bool {
	if (ISPA_STD::python_contains((m), key)) {
		return false;
	}
	(m)[key] = value;
	return true;
}
template<typename K, typename V> auto python::ispa_std::map::insert_or_assign(std::unordered_map<K, V>& m, K key, V value) -> bool {
	bool inserted = (!(ISPA_STD::python_contains((m), key)));
	(m)[key] = value;
	return inserted;
}
template<typename K, typename V> auto python::ispa_std::map::erase(std::unordered_map<K, V>& m, K key) -> bool {
	if ((!(ISPA_STD::python_contains((m), key)))) {
		return false;
	}
	ISPA_STD::python_pop((m), key);
	return true;
}
template<typename T> auto python::ispa_std::vector::filled(long long count, T value) -> std::vector<T> {
	std::vector<T> result = std::vector<T>{};
	python::ispa_std::vector::resize(result, count, value);
	return result;
}
template<typename T> auto python::ispa_std::vector::at(const std::vector<T>& arr, long long index) -> T {
	if (((((index) < (0))) || (((index) >= (arr.size()))))) {
		::ISPA_STD::fail(python::ispa_std::errors::ErrorKind::OutOfRange, "vector.at: index out of range");
	}
	return (arr)[index];
}
template<typename T> auto python::ispa_std::vector::front(const std::vector<T>& arr) -> T {
	if (((arr.size()) == (0))) {
		::ISPA_STD::fail(python::ispa_std::errors::ErrorKind::OutOfRange, "vector.front: empty");
	}
	return (arr)[0];
}
template<typename T> auto python::ispa_std::vector::back(const std::vector<T>& arr) -> T {
	if (((arr.size()) == (0))) {
		::ISPA_STD::fail(python::ispa_std::errors::ErrorKind::OutOfRange, "vector.back: empty");
	}
	return (arr)[((arr.size()) - (1))];
}
template<typename T> auto python::ispa_std::vector::pop_back(std::vector<T>& arr) -> void {
	if (((arr.size()) == (0))) {
		::ISPA_STD::fail(python::ispa_std::errors::ErrorKind::OutOfRange, "vector.pop_back: empty");
	}
	ISPA_STD::python_pop((arr));
}
template<typename T> auto python::ispa_std::vector::insert_at(std::vector<T>& arr, long long index, T value) -> void {
	if (((((index) < (0))) || (((index) > (arr.size()))))) {
		::ISPA_STD::fail(python::ispa_std::errors::ErrorKind::OutOfRange, "vector.insert_at: index out of range");
	}
	ISPA_STD::python_insert((arr), index, value);
}
template<typename T> auto python::ispa_std::vector::erase_at(std::vector<T>& arr, long long index) -> void {
	if (((((index) < (0))) || (((index) >= (arr.size()))))) {
		::ISPA_STD::fail(python::ispa_std::errors::ErrorKind::OutOfRange, "vector.erase_at: index out of range");
	}
	ISPA_STD::python_pop((arr), index);
}
template<typename T> auto python::ispa_std::vector::erase_range(std::vector<T>& arr, long long first, long long last) -> void {
	long long n = arr.size();
	if (((((((first) < (0))) || (((last) < (first))))) || (((last) > (n))))) {
		::ISPA_STD::fail(python::ispa_std::errors::ErrorKind::OutOfRange, "vector.erase_range: bad range");
	}
	auto kept = ISPA_STD::python_copy((arr));
	(arr).clear();
	long long i = 0;
	while (((i) < (n))) {
		if (((((i) < (first))) || (((i) >= (last))))) {
			(arr).push_back((kept)[i]);
		}
		i = ((i) + (1));
	}
}
template<typename T> auto python::ispa_std::vector::resize(std::vector<T>& arr, long long count, T fill) -> void {
	if (((count) < (0))) {
		::ISPA_STD::fail(python::ispa_std::errors::ErrorKind::LengthError, "vector.resize: negative size");
	}
	while (((arr.size()) > (count))) {
		ISPA_STD::python_pop((arr));
	}
	while (((arr.size()) < (count))) {
		(arr).push_back(fill);
	}
}
template<typename T> auto python::ispa_std::vector::assign(std::vector<T>& arr, long long count, T value) -> void {
	(arr).clear();
	python::ispa_std::vector::resize(arr, count, value);
}
template<typename T> auto python::ispa_std::vector::swap(std::vector<T>& a, std::vector<T>& b) -> void {
	auto tmp = ISPA_STD::python_copy((a));
	(a).clear();
	ISPA_STD::python_extend((a), b);
	(b).clear();
	ISPA_STD::python_extend((b), tmp);
}
template<typename T> auto python::ispa_std::vector::lexicographical_less(const std::vector<T>& a, const std::vector<T>& b) -> bool {
	long long i = 0;
	while (((((i) < (a.size()))) && (((i) < (b.size()))))) {
		if ((((a)[i]) < ((b)[i]))) {
			return true;
		}
		if ((((b)[i]) < ((a)[i]))) {
			return false;
		}
		i = ((i) + (1));
	}
	return ((a.size()) < (b.size()));
}
template<typename T> auto python::ispa_std::to_string::list_to_string(const std::vector<T>& items, std::function<std::string(T)> element_to_string) -> std::string {
	std::string out = "";
	long long i = 0;
	while (((i) < (items.size()))) {
		out = ((out) + (element_to_string((items)[i])));
		i = ((i) + (1));
	}
	return out;
}

#include <ispastdlib/external.hpp>
namespace o::Exports {
struct symbol_0 : ::o::Parser {
using node_type = ::o::Types::modules::c::main;
static bool starts(const char* origin, const char* start);
static ISPA_STD::ExternalResult<node_type> parse(const char* origin, const char* start);
};
}
