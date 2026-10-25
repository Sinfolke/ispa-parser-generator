#include "parser.hpp"
auto line_and_column(const std::string &str, std::size_t offset) {
    std::size_t line = 1;
    std::size_t column = 0;
    for (std::size_t i =0; i < offset; ++i) {
        const auto &c = str[i];
        if (str[i] == '\n') {
            ++line;
            column = 0;
        } else {
            ++column;
        }
    }
    return std::make_pair(line, column);
}
int main(int argc, const char** argv) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " \"<input>\"\n";
        return 1;
    }

    std::string i;
    PyLexer lexer;
    if (std::string(argv[1]) == "--file") {
        if (argc < 3) {
            std::cout << "Usage: " << argv[0] << " --file <path>\n";
            return 1;
        }
        std::ifstream inputf(argv[2]);
        if (!inputf.is_open()) {
            throw std::runtime_error("Failed to open input file");
        }
        std::string s((std::istreambuf_iterator<char>(inputf)), {});
        i = std::move(s);
    } else {
        i = argv[1];
    }
    PyRawParser::Parser parser;
    PyRawParser::Types::main node = parser.parse(i.c_str());
    for (const auto &e : parser.getErrors()) {
        auto [line, column] = line_and_column(i, e.offset);
        std::cout << "Error[" << line << ":" << column << "]: " << e.message << std::endl;
    }
    std::ofstream of("tree.txt");
    of << node << "\n\n";
    std::cout << "Parsed. AST dump saved to tree.txt\n";
}