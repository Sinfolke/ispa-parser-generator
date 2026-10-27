#include "Parser.h"

#include <ispastdlib.hpp>

auto Parser::TokensToString(std::size_t id) -> const char* {
	switch (id) {
		case Tokens::ISPA_TEMPLATE_INSTANCE_0::id: {
			return TokensStr::ISPA_TEMPLATE_INSTANCE_0::id;
			break;
		}
		case Tokens::QUOTE_2::id: {
			return TokensStr::QUOTE_2::id;
			break;
		}
		case Tokens::__WS::id: {
			return TokensStr::__WS::id;
			break;
		}
		case Tokens::QUOTE_1::id: {
			return TokensStr::QUOTE_1::id;
			break;
		}
		case Tokens::STRING::id: {
			return TokensStr::STRING::id;
			break;
		}
	}
	throw "Invalid Tokens ID";
}
auto Parser::RulesToString(std::size_t id) -> const char* {
	switch (id) {
		case Rules::main::id: {
			return RulesStr::main::id;
			break;
		}
	}
	throw "Invalid Rules ID";
}
auto Parser::FlatTypes::QUOTE_2::write_to_output(::ISPA_STD::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("QUOTE_2", true);
}
auto Parser::FlatTypes::QUOTE_2::print(std::ostream& os) const -> void {
	::ISPA_STD::ASTPrinter<std::ostream> printer = ::ISPA_STD::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto Parser::FlatTypes::QUOTE_2::to_string() const -> std::string {
	return ::ISPA_STD::concat("QUOTE_2 {", "}");
}
auto Parser::FlatTypes::operator<<(std::ostream & os, QUOTE_2 value) -> std::ostream & {
	value.print(os);
	return os;
}
auto Parser::FlatTypes::__WS::write_to_output(::ISPA_STD::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("__WS", true);
}
auto Parser::FlatTypes::__WS::print(std::ostream& os) const -> void {
	::ISPA_STD::ASTPrinter<std::ostream> printer = ::ISPA_STD::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto Parser::FlatTypes::__WS::to_string() const -> std::string {
	return ::ISPA_STD::concat("__WS {", "}");
}
auto Parser::FlatTypes::operator<<(std::ostream & os, __WS value) -> std::ostream & {
	value.print(os);
	return os;
}
auto Parser::FlatTypes::QUOTE_1::write_to_output(::ISPA_STD::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("QUOTE_1", true);
}
auto Parser::FlatTypes::QUOTE_1::print(std::ostream& os) const -> void {
	::ISPA_STD::ASTPrinter<std::ostream> printer = ::ISPA_STD::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto Parser::FlatTypes::QUOTE_1::to_string() const -> std::string {
	return ::ISPA_STD::concat("QUOTE_1 {", "}");
}
auto Parser::FlatTypes::operator<<(std::ostream & os, QUOTE_1 value) -> std::ostream & {
	value.print(os);
	return os;
}
auto Parser::FlatTypes::main::write_to_output(::ISPA_STD::ASTPrinter<std::ostream>& printer) const -> void {
	printer.node("main", false);
	printer.up(false);
	printer.node("str1", true);
	printer.up(false);
	str1.print(printer);
	printer.down();
	printer.down();
}
auto Parser::FlatTypes::main::print(std::ostream& os) const -> void {
	::ISPA_STD::ASTPrinter<std::ostream> printer = ::ISPA_STD::ASTPrinter<std::ostream>{os};
	write_to_output(printer);
}
auto Parser::FlatTypes::main::to_string() const -> std::string {
	return ::ISPA_STD::concat("main {", "str1 = ", str1.to_string(), "}");
}
auto Parser::FlatTypes::operator<<(std::ostream & os, main value) -> std::ostream & {
	value.print(os);
	return os;
}
auto Parser::Lexer::clone() const -> std::shared_ptr<::ISPA_STD::Lexer_base<Token>> {
	return std::make_shared<Lexer>(*this);
}
auto Parser::Lexer::ispa_skip_current_token() -> void {
	ispa_skip_token = true;
}
auto Parser::Lexer::ispa_token_error(std::string message) -> void {
	throw std::runtime_error(message);
}
auto Parser::Lexer::on_token(Token&& token) -> void {
	if (ispa_skip_token) {
	} else {
		append_token(token);
	}
	ispa_skip_token = false;
	tokens.insert(tokens.end(), ispa_pending_tokens.begin(), ispa_pending_tokens.end());
	ispa_pending_tokens.clear();
}
auto Parser::Lexer::on_eof() -> void {
	tokens.insert(tokens.end(), ispa_pending_tokens.begin(), ispa_pending_tokens.end());
	ispa_pending_tokens.clear();
	tokens.emplace_back();
}
auto Parser::Lexer::semantic_action_exec(long long state, const ::ISPA_STD::DFA::API::Captures& captures, long long start_pos, const char* start, long long length, long long line, const char*& cursor) -> std::pair<long long, Token> {
	switch (state) {
		case 0: {
			std::string q = captures.text(0);
			if (::ispa::debug::enabled()) ::ispa::debug::lexer_capture("q", q);
			std::vector<std::string> ispa_dynamic_choices = {"\n", "\r", q};
			if (::ISPA_STD::DFA::API::dynamic_match(cursor, ispa_dynamic_choices, true, "*")) {
				return std::make_pair(1, Token{});
			} else {
				return std::make_pair(::ISPA_STD::DFA::API::null_state, Token{});
			}
			break;
		}
		case 1: {
			std::string q = captures.text(0);
			if (::ispa::debug::enabled()) ::ispa::debug::lexer_capture("q", q);
			std::vector<std::string> ispa_dynamic_choices = {"\n", "\r", q};
			if (::ISPA_STD::DFA::API::dynamic_match(cursor, ispa_dynamic_choices, true, "*")) {
				return std::make_pair(4, Token{});
			} else {
				return std::make_pair(::ISPA_STD::DFA::API::null_state, Token{});
			}
			break;
		}
		case 2: {
			std::string q = captures.text(0);
			if (::ispa::debug::enabled()) ::ispa::debug::lexer_capture("q", q);
			std::vector<std::string> ispa_dynamic_choices = {"\n", "\r", q};
			if (::ISPA_STD::DFA::API::dynamic_match(cursor, ispa_dynamic_choices, true, "*")) {
				return std::make_pair(3, Token{});
			} else {
				return std::make_pair(::ISPA_STD::DFA::API::null_state, Token{});
			}
			break;
		}
		case 3: {
			std::string q = captures.text(0);
			if (::ispa::debug::enabled()) ::ispa::debug::lexer_capture("q", q);
			std::vector<std::string> ispa_dynamic_choices = {"\n", "\r", q};
			if (::ISPA_STD::DFA::API::dynamic_match(cursor, ispa_dynamic_choices, true, "*")) {
				return std::make_pair(2, Token{});
			} else {
				return std::make_pair(::ISPA_STD::DFA::API::null_state, Token{});
			}
			break;
		}
		case 4: {
			std::string value = captures.text(1);
			if (::ispa::debug::enabled()) ::ispa::debug::lexer_capture("value", value);
			std::string ispa_action_q = captures.text(0);
			if (::ispa::debug::enabled()) ::ispa::debug::lexer_capture("ispa_action_q", ispa_action_q);
			return std::make_pair(::ISPA_STD::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::ISPA_TEMPLATE_INSTANCE_0::id, Types::ISPA_TEMPLATE_INSTANCE_0{value}));
			break;
		}
		case 5: {
			return std::make_pair(::ISPA_STD::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::__WS::id, Types::__WS{}));
			break;
		}
		case 6: {
			return std::make_pair(::ISPA_STD::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::QUOTE_1::id, Types::QUOTE_1{}));
			break;
		}
		case 7: {
			return std::make_pair(::ISPA_STD::DFA::API::null_state, Token::create(start_pos, start, length, line, Tokens::QUOTE_2::id, Types::QUOTE_2{}));
			break;
		}
	}
	throw std::runtime_error("Out of bound semantic action");
}
auto Parser::Lexer::init() -> void {
	return ;
}
auto Parser::Lexer::makeToken(const char*& pos) -> Token {
	return lookup(dfa_table, char_class_table, action_table, tdfa_registers, [&](long long state, const ::ISPA_STD::DFA::API::Captures& captures, long long start_pos, const char* start, long long length, long long line, const char*& cursor) {
return semantic_action_exec(state, captures, start_pos, start, length, line, cursor);
}, debug_array, debug_index, pos);
}
::ISPA_STD::DFA::API::CharToClass Parser::Lexer::char_class_table = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
::ISPA_STD::DFA::API::Table<5, 128> Parser::Lexer::dfa_table = {{{::ISPA_STD::DFA::API::null_state, 507, 507, 507, 507, 507, 507, 507, 507, 510, 510, 510, 510, 510, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 510, 507, 509, 507, 507, 507, 507, 508, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507}, {511, 131, 130, 129, 128, 127, 126, 125, 124, 123, 122, 121, 120, 119, 118, 117, 116, 115, 114, 113, 112, 111, 110, 109, 108, 107, 106, 105, 104, 103, 102, 101, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5}, {512, 252, 251, 250, 249, 248, 247, 246, 245, 510, 510, 510, 510, 510, 244, 243, 242, 241, 240, 239, 238, 237, 236, 235, 234, 233, 232, 231, 230, 229, 228, 227, 510, 226, 225, 224, 223, 222, 221, 220, 219, 218, 217, 216, 215, 214, 213, 212, 211, 210, 209, 208, 207, 206, 205, 204, 203, 202, 201, 200, 199, 198, 197, 196, 195, 194, 193, 192, 191, 190, 189, 188, 187, 186, 185, 184, 183, 182, 181, 180, 179, 178, 177, 176, 175, 174, 173, 172, 171, 170, 169, 168, 167, 166, 165, 164, 163, 162, 161, 160, 159, 158, 157, 156, 155, 154, 153, 152, 151, 150, 149, 148, 147, 146, 145, 144, 143, 142, 141, 140, 139, 138, 137, 136, 135, 134, 133, 132}, {513, 379, 378, 377, 376, 375, 374, 373, 372, 371, 370, 369, 368, 367, 366, 365, 364, 363, 362, 361, 360, 359, 358, 357, 356, 355, 354, 353, 352, 351, 350, 349, 348, 347, 346, 345, 344, 343, 342, 341, 340, 339, 338, 337, 336, 335, 334, 333, 332, 331, 330, 329, 328, 327, 326, 325, 324, 323, 322, 321, 320, 319, 318, 317, 316, 315, 314, 313, 312, 311, 310, 309, 308, 307, 306, 305, 304, 303, 302, 301, 300, 299, 298, 297, 296, 295, 294, 293, 292, 291, 290, 289, 288, 287, 286, 285, 284, 283, 282, 281, 280, 279, 278, 277, 276, 275, 274, 273, 272, 271, 270, 269, 268, 267, 266, 265, 264, 263, 262, 261, 260, 259, 258, 257, 256, 255, 254, 253}, {514, 506, 505, 504, 503, 502, 501, 500, 499, 498, 497, 496, 495, 494, 493, 492, 491, 490, 489, 488, 487, 486, 485, 484, 483, 482, 481, 480, 479, 478, 477, 476, 475, 474, 473, 472, 471, 470, 469, 468, 467, 466, 465, 464, 463, 462, 461, 460, 459, 458, 457, 456, 455, 454, 453, 452, 451, 450, 449, 448, 447, 446, 445, 444, 443, 442, 441, 440, 439, 438, 437, 436, 435, 434, 433, 432, 431, 430, 429, 428, 427, 426, 425, 424, 423, 422, 421, 420, 419, 418, 417, 416, 415, 414, 413, 412, 411, 410, 409, 408, 407, 406, 405, 404, 403, 402, 401, 400, 399, 398, 397, 396, 395, 394, 393, 392, 391, 390, 389, 388, 387, 386, 385, 384, 383, 382, 381, 380}}};
::ISPA_STD::DFA::API::Table<502, 4> Parser::Lexer::action_table = {{{1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}, {1, 1, ::ISPA_STD::DFA::API::null_state, 507}}};
std::array<::ISPA_STD::DFA::API::DFADebug, 1> Parser::Lexer::debug_array = {::ISPA_STD::DFA::API::DFADebug {"<Undefined>", "", 0, -1, -1, -1, 0, 1, '\000'}};
std::unordered_map<long long, std::unordered_map<long long, long long>> Parser::Lexer::debug_index = {  };
template<typename IT, class> auto Parser::Parser::main(IT pos) -> ::ISPA_STD::MatchResult<Types::main, IT> {
	try {
		::ispa::debug::RuleScope ispa_debug_rule{"main", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/test.isc", 9, 1}};
		::ispa::debug::ProductionScope ispa_debug_production{"main", {"/mnt/5EE9F38E0E9F2DC9/ispa-parser/test.isc", 9, 1}};
		if (::ispa::debug::enabled()) ::ispa::debug::line({"/mnt/5EE9F38E0E9F2DC9/ispa-parser/test.isc", 9, 1}, ::ispa::debug::Snapshot{}.add("pos", pos), "before");
		Types::main result = {};
		::ISPA_STD::Node<Types::ISPA_TEMPLATE_INSTANCE_0> value_ISPA_TEMPLATE_INSTANCE_0;
		auto recovery_scope = recovery_frame("main");
		if (unwinding()) {
			return action_result(result, pos);
		}
		recovery_at<ISPA_STD::Recovery::Tokens<Tokens::ISPA_TEMPLATE_INSTANCE_0::id>>("ISPA_TEMPLATE_INSTANCE_0", "ISPA_TEMPLATE_INSTANCE_0");
		value_ISPA_TEMPLATE_INSTANCE_0 = required<Types::ISPA_TEMPLATE_INSTANCE_0>(pos, ISPA_STD::Recovery::handler<ISPA_STD::Recovery::Tokens<Tokens::ISPA_TEMPLATE_INSTANCE_0::id>, ISPA_STD::Recovery::EmptyTokens, ISPA_STD::Recovery::EmptyTokens, ISPA_STD::Recovery::Tokens<Tokens::ISPA_TEMPLATE_INSTANCE_0::id>, ISPA_STD::Recovery::Policy<false, true, false, 32>>("main", "ISPA_TEMPLATE_INSTANCE_0"));
		::ISPA_STD::assign_compatible(result.str1, value_ISPA_TEMPLATE_INSTANCE_0);
		if (::ispa::debug::enabled()) ::ispa::debug::value("result", result, ::ispa::debug::active_location());
		::ISPA_STD::MatchResult<Types::main, IT> match_result;
		match_result.status = true;
		match_result.node.set_data(result);
		match_result.it = pos;
		return match_result;
	} catch (const NoMatch&) { throw;
	} catch (const ::ISPA_STD::ExternalParseError&) { throw;
	} catch (...) { ::ispa::debug::breakpoint("uncaught parser failure"); throw; }
}
template auto Parser::Parser::main<::Parser::Lexer::iterator>(::Parser::Lexer::iterator pos) -> ::ISPA_STD::MatchResult<Types::main, ::Parser::Lexer::iterator>;
template auto Parser::Parser::main<::Parser::Lexer::lazy_iterator>(::Parser::Lexer::lazy_iterator pos) -> ::ISPA_STD::MatchResult<Types::main, ::Parser::Lexer::lazy_iterator>;
auto Parser::Parser::parseFromTokens() -> void {
	tree = main(Lexer::iterator(lexer)).node.data();
}
auto Parser::Parser::lazyParse() -> ::ISPA_STD::MatchResult<Types::main, Lexer::lazy_iterator> {
	lazy_lexer_.clearTokens();
	return main(Lexer::lazy_iterator(lazy_lexer_, text));
}

#include "Parser.h"

bool Parser::Exports::symbol_0::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::QUOTE_1::id));
}
ISPA_STD::ExternalResult<Parser::Exports::symbol_0::node_type> Parser::Exports::symbol_0::parse(const char* origin, const char* start) {
symbol_0 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it);
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_STD::external_node(origin, start, end, std::move(node)), end};
}

bool Parser::Exports::symbol_1::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::QUOTE_2::id));
}
ISPA_STD::ExternalResult<Parser::Exports::symbol_1::node_type> Parser::Exports::symbol_1::parse(const char* origin, const char* start) {
symbol_1 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it);
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_STD::external_node(origin, start, end, std::move(node)), end};
}

bool Parser::Exports::symbol_2::starts(const char* origin, const char* start) {
if (!start) return false;
if (!*start) return false;
Lexer lexer; lexer.setInput(origin); const char* p = start;
auto token = lexer.makeToken(p);
while (!token.empty() && token.kind() == Tokens::__WS::id && *p) token = lexer.makeToken(p);
return (!token.empty() && (false || token.kind() == Tokens::ISPA_TEMPLATE_INSTANCE_0::id));
}
ISPA_STD::ExternalResult<Parser::Exports::symbol_2::node_type> Parser::Exports::symbol_2::parse(const char* origin, const char* start) {
symbol_2 parser; Lexer lexer; lexer.setExternalBoundary(); Lexer::lazy_iterator it(lexer, origin, start);
parser.template skip_spaces<Types::__WS>(it);
auto node = parser.template optional<node_type>(it, [&parser](auto cursor) { return parser.main(cursor); });
if (node.empty()) return {};
if (!parser.getErrors().empty()) throw std::runtime_error(parser.getErrors().front().message);
const char* end = it.sourcePosition();
return {true, ISPA_STD::external_node(origin, start, end, std::move(node)), end};
}
