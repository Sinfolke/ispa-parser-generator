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
    for (auto &t : lexer.getTokensReference()) {
        std::cout << "token index=" << t.index() << "\n";
    }
    std::cout << node << std::endl;
}