#include "Parser.h"
namespace Parser {
::ISPA_STD::DFA::API::CharToClass Lexer::char_class_table = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 140735147283576, 140735147285328, 132563490614346, 0, 163840, 160656, 160656, 4096, 0, 1, 163840, 1769472, 1769145, 1769145, 4096, 163840, 5, 1769472, 2093056, 2089090, 2089090, 4096, 1769472, 1, 2093056, 2117632, 2115272, 2170256, 4096, 2088960, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 132563490629175, 0, 0, 132563490807808, 132563490047296, 140735147285328, 132563490613640, 17179869190, 64, 64, 64, 784, 784, 8, 17179869187, 1927072, 1927072, 1927072}};
::ISPA_STD::DFA::API::Table<0, 0> Lexer::dfa_table = {};
::ISPA_STD::DFA::API::Table<0, 3> Lexer::lr_table = {};
auto Parser::Lexer::semantic_action_exec (long long state, long long start_pos, const char* start, long long length, long long line, std::vector<std::variant<std::monostate, Token, char, std::string>>& values, std::vector<std::vector<std::variant<std::monostate, Token, char, std::string>>>& vec_values) -> std::pair<long long, Token>{
	switch (state) {
	}
	throw std::runtime_error("Out of bound semantic action");
}
auto Parser::Lexer::makeToken (const char*& pos) -> Token{
	return lookup(dfa_table, char_class_table, lr_table, values, vec_values, registers, register_ids, semantic_action_exec, pos);
}

}
