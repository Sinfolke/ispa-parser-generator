#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <stdexcept>

#include <yaml_parser.h>

int main(int argc, const char** argv) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " \"<input>\" OR " << argv[0] << " --file <path>\n";
        return 1;
    }

    // 1. Read input file into memory BEFORE starting the timer
    std::string input_buffer;
    if (std::string(argv[1]) == "--file") {
        if (argc < 3) {
            std::cout << "Usage: " << argv[0] << " --file <path>\n";
            return 1;
        }
        std::ifstream inputf(argv[2]);
        if (!inputf.is_open()) {
            throw std::runtime_error("Failed to open input file");
        }
        input_buffer.assign((std::istreambuf_iterator<char>(inputf)),
                             std::istreambuf_iterator<char>());
    } else {
        input_buffer = argv[1];
    }

    auto start = std::chrono::steady_clock::now();

    yaml_parser::Parser parser;
    yaml_parser::Lexer lex;
    lex.makeTokens(input_buffer.c_str());
    yaml_parser::Types::main node = parser.parse(lex);

    auto end = std::chrono::steady_clock::now();

    std::chrono::duration<double, std::milli> elapsed = end - start;

    std::cout << "ISPA total: " << elapsed.count() << " ms\n";
    std::cout << node << '\n';

    return 0;
}
