module CppParser;
import utils;
import File;
import corelib;
import std;

auto CppParser::generate() -> int {
    return utils::command("{} -a LL --lang cpp {} --ddall --dd TestDumps --no-incremental -o {}", std::filesystem::path(BINARY_DIR) / "ispa", File::getGrammarFile(name), temp_directory / "Parser");
}
auto CppParser::compile(std::string bootloader_name, std::string clang) -> int {
    return utils::command("{} {} {} -I {} -I {} -ferror-limit=1 -o {} -g -Wno-switch -L {} -lispa-core -lispastdlib",
        clang,
        std::filesystem::path(TEST_ROOT_DIR) / "compile" / (bootloader_name + ".cpp"),
        parserSourcePath(),
        temp_directory,
        std::filesystem::path(ROOT_DIR) / "parser" / "stdlibs" / "target" / "target-cpp",
        binaryPath(),
        std::filesystem::path(BINARY_DIR) / "parser" / "ispastdlib_build"

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
