// The C++ reference for rust_parity.py: parse argv[1] with the generated C++ parser and print the tree.
#include "generated.h"
#include <iostream>
int main(int argc, char** argv) {
    generated::Parser parser;
    generated::Lexer lexer;
    const bool lazy = getenv("LAZY") != nullptr;
    lexer.setInput(argv[1]);
    try {
        if (!lazy) lexer.makeTokens();
        auto& tree = lazy ? parser.parse(argv[1]) : parser.parse(lexer);
        tree.print(std::cout);
        for (const auto& d : parser.getErrors()) std::cout << "ERR " << d.rule << " | " << d.message << "\n";
        for (const auto& d : parser.getContextErrors()) std::cout << "ERR " << d.rule << " | " << d.message << "\n";
    } catch (const std::exception& e) { std::cout << "exception: " << e.what() << "\n"; }
}
