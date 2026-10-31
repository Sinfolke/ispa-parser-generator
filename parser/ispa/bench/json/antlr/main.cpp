#include "antlr4-runtime.h"
#include "JSONLexer.h"
#include "JSONParser.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
int main(int argc, char** argv) {
    std::ifstream in(argv[1], std::ios::binary); std::stringstream ss; ss << in.rdbuf();
    std::string text = ss.str();
    bool tree = argc > 2 && std::string(argv[2]) == "tree";
    auto t0 = std::chrono::steady_clock::now();
    antlr4::ANTLRInputStream input(text);
    JSONLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    JSONParser parser(&tokens);
    parser.setBuildParseTree(tree);
    parser.main();
    auto t1 = std::chrono::steady_clock::now();
    if (parser.getNumberOfSyntaxErrors()) return 1;
    std::cout << "{\"total_ms\":" << std::chrono::duration<double, std::milli>(t1 - t0).count() << "}\n";
}
