#include <Parser.h>
#include <iostream>
#include <ostream>
#include <variant>
int main(int argc, const char** argv) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " \"<input>\"\n";
        return 1;
    }
    Parser::Lexer lexer;
    lexer.makeTokens(argv[1]);
    Parser::Parser parser;
    auto &node = parser.parse(lexer);
    std::cout << node << std::endl;
}