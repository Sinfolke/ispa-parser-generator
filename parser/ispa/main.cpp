#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>

#include <Parser.h>

#include "parser.h"

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

    IspaParser parser;
    std::optional<Parser::Types::main> node;
    std::string failure;
    try {
        node.emplace(parser.parse(input_buffer.c_str()));
    } catch (const std::exception& error) {
        failure = error.what();
    }

    auto end = std::chrono::steady_clock::now();

    std::chrono::duration<double, std::milli> elapsed = end - start;

    std::cout << "ISPA total: " << elapsed.count() << " ms\n";
    const auto show_location = [&](std::size_t offset) {
        if (offset == std::numeric_limits<std::size_t>::max()) return;
        std::size_t line = 1, column = 1;
        for (std::size_t i = 0; i < std::min(offset, input_buffer.size()); ++i) {
            if (input_buffer[i] == '\n') { ++line; column = 1; }
            else ++column;
        }
        std::cout << " at " << (std::string(argv[1]) == "--file" ? argv[2] : "<input>")
                  << ':' << line << ':' << column;
    };
    std::set<std::pair<std::string, std::size_t>> shown;
    for (const auto& err : parser.getErrors()) {
        const auto message = err.message.empty() ? "syntax error" : err.message;
        if (!shown.emplace(message, err.offset).second) continue;
        std::cout << "Error: " << message;
        show_location(err.offset);
        std::cout << '\n';
        for (const auto& note : err.notes) std::cout << "  Note: " << note << '\n';
    }
    for (const auto& err : parser.getContextErrors()) {
        if (!shown.emplace(err.message, err.offset).second) continue;
        std::cout << "Error: " << err.message;
        show_location(err.offset);
        std::cout << '\n';
    }
    if (!failure.empty() && std::none_of(shown.begin(), shown.end(),
                                          [&](const auto& item) { return item.first == failure; }))
        std::cout << "Parse stopped: " << failure << '\n';
    if (!failure.empty() || !parser.getErrors().empty() || !parser.getContextErrors().empty()) return 1;
    // 6. AST Output / File Write AFTER timer has stopped
    std::ofstream of("tree.txt");
    of << *node << "\n\n";
    std::cout << "Successfully parsed and saved to tree.txt\n";

    return 0;
}
