module CppParser;
import utils;
import File;
import corelib;
import std;

auto CppParser::generate() -> int {
    return utils::command("{} -a LL -o {} --lang cpp {} --ddall --dd TestDumps", std::filesystem::path(BINARY_DIR) / "ispa", temp_directory / "Parser", File::getGrammarFile(name));
}
auto CppParser::compile(std::string bootloader_name, std::string clang) -> int {
    return utils::command("{} {} {} -I {} -I {} -ferror-limit=1 -o {} -g",
        clang,
        std::filesystem::path(TEST_ROOT_DIR) / "compile" / (bootloader_name + ".cpp"),
        parserSourcePath(),
        temp_directory,
        std::filesystem::path(ROOT_DIR) / "parser" / "stdlibs",
        binaryPath()
    );
}
auto CppParser::run(std::string input) -> int {
    return utils::command(
        "{} \"{}\"",
        binaryPath(),
        input
    );
}

auto CppParser::parserHeaderPath() const -> std::filesystem::path {
    return temp_directory / "Parser.h";
}

auto CppParser::parserSourcePath() const -> std::filesystem::path {
    return temp_directory / "Parser.cpp";
}

auto CppParser::binaryPath() const -> std::filesystem::path {
    std::filesystem::path path(name);
    path.replace_extension("");
    return temp_directory / path.filename();
}
