// file_io.hpp - makeTokensFromFile's file read, kept in C++. The Python port only
// has makeTokensFromText(text); pass the returned string (decode as latin-1 on hosts
// whose String is not 8-bit-unit, see docs/BACKEND.md).
#pragma once
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace ispa_std_io {
inline std::string read_file(const char* path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file) throw std::runtime_error(std::string("Failed to open file '") + path + "'");
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}
} // namespace ispa_std_io
