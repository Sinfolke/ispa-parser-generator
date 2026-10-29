// The C++ reference for rust_parity.py: parse argv[1] with the generated C++ parser and print the tree.
#include "generated.h"
#include <iostream>
int main(int argc, char** argv) {
    generated::Parser parser;
    generated::Lexer lexer;
    lexer.setInput(argv[1]);
    try {
        lexer.makeTokens();
        auto& tree = parser.parse(lexer);
        tree.print(std::cout);
    } catch (const std::exception& e) { std::cout << "exception: " << e.what() << "\n"; }
}
