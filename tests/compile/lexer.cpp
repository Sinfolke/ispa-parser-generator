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
    auto tokens = lexer.makeTokens(argv[1]);
    // for (auto token : tokens) {
    //     std::cout << token.index() << "\n";
    //     if (std::holds_alternative<ISPA_STD::Node<Parser::Tokens, Parser::Types::INTEGER>>(token)) {
    //         const auto tt = std::get<ISPA_STD::Node<Parser::Tokens, Parser::Types::INTEGER>>(token);
    //         std::cout << "INTEGER { " << tt.value << " }\n";
    //     } else if (std::holds_alternative<ISPA_STD::Node<Parser::Tokens, Parser::Types::ID>>(token)) {
    //         const auto tt = std::get<ISPA_STD::Node<Parser::Tokens, Parser::Types::ID>>(token);
    //         std::cout << "ID {" << tt.value << "}\n";
    //     } else if (std::holds_alternative<ISPA_STD::Node<Parser::Tokens, Parser::Types::__WS>>(token)) {
    //         std::cout << "WHITESPACE{}\n";
    //     }
    // }
}
// int main() {
//     Parser::Lexer lexer;
//     Parser::Parser parser;
//     lexer.makeTokensFromFile("parser/parser/rule.isc");
//     std::ofstream ofile("tokens");
//     if (!ofile) {
//         std::cerr << "could not open token file\n";
//         exit(1);
//     }
//     lexer.printTokens(ofile);
//     auto errors = lexer.getErrors();
//     for (auto error : errors) {
//         printf("Lexer: %zu:%zu: %s\n", error.line, error.column, error.message.c_str());
//     }
//     std::cout << std::endl;
//     auto tree = parser.parse(lexer);
//     auto parser_errors = parser.getErrors();
//     printf("errors size(): %zu\n", parser_errors.size());
//     for (auto error : parser_errors) {
//         printf("Parser: %zu:%zu(%zu): %s\n", error.line, error.column, error.pos, error.message.c_str());
//     }
//     std::ofstream file("AST");
//     parser.printAST(file);
// }