module;
module corelib;
import logging;
import cpuf.printf;
import std;

namespace corelib::file {
    // Function to read the content of a file into a string
    std::string readFile(const std::filesystem::path filePath) {
        std::ifstream file(filePath, std::ios::in | std::ios::binary);
        if (!file) {
            throw UError("Failed to open file: %$", filePath.string());
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        return content;
    }
    std::string readFile(const std::string& filePath) {
        std::ifstream file(filePath, std::ios::in | std::ios::binary);
        if (!file) {
            throw UError("Failed to open file: %s", filePath.c_str());
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        return content;
    }
    // Function to recursively get file contents
    stdu::vector<std::string> getFilesRecursively(const std::string& dir, const std::string& ext = "") {
        stdu::vector<std::string> contents;

        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
                if (entry.is_regular_file() && 
                    (ext.empty() || entry.path().extension() == ext)) {
                    contents.push_back(entry.path().string());
                }
            }
        } catch (const std::exception& e) {
            throw UError("Error accessing directory %s: %$", dir, e.what());
        }

        return contents;
    }
}
namespace corelib::text {
    bool startsWith(const std::string& str, const std::string& prefix) {
        // Ensure the prefix is not longer than the string
        if (prefix.size() > str.size()) {
            return false;
        }

        // Compare the beginning of the string with the prefix
        return std::equal(prefix.begin(), prefix.end(), str.begin());
    }
    bool startsWithRange(const std::string& str, char from, char to) {
        return (str[0] >= from && str[0] <= to);
    }
    bool isAllAlpha(const std::string& str) {
        for (const char &ch : str) {
            if (!std::isalpha(ch))
                return false;
        }
        return true;
    }

    bool isUpper(const char* str) {
        for (; *str; str++) {
            if (std::isalpha(*str) && std::islower(*str))
                return false;
        }
        return true;
    }

    bool isUpper(const std::string str) {
        for (char ch : str) {
            if (std::isalpha(ch) && std::islower(ch))
                return false;
        }
        return true;
    }
    bool isLower(const char* str) {
        return !isUpper(str);
    }
    bool isLower(const std::string str) {
        return !isUpper(str);
    }
    std::string ToUpper(std::string str) {
        for (auto &c : str)
            c = std::toupper(c);
        return str; 
    }
    std::string ToLower(std::string str) {
        for (auto &c : str)
            c = std::tolower(c);
        return str; 
    }
    // Returns the escaped representation of a character, e.g., '\n' -> "\\n", 'A' -> "A"
    std::string getEscapedAsStr(char in, bool /*stringContext*/) {
        switch (in) {
            case '\0': return "\\0";
            case '\n': return "\\n";
            case '\r': return "\\r";
            case '\t': return "\\t";
            case '\a': return "\\a";
            case '\b': return "\\b";
            case '\f': return "\\f";
            case '\v': return "\\v";
            case '\\': return "\\\\";
            case '\"': return "\\\"";
            case '\'': return "\\\'";
            default:
                if (std::isprint(static_cast<unsigned char>(in))) {
                    return std::string(1, in);
                } else {
                    std::ostringstream oss;
                    oss << "\\x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                        << (static_cast<unsigned int>(static_cast<unsigned char>(in)));
                    return oss.str();
                }
        }
    }

    // Maps escape code character ('n') to actual character ('\n')
    char getEscapedFromChar(char in) {
        switch (in) {
            case 'n': return '\n';
            case 'r': return '\r';
            case 't': return '\t';
            case 'a': return '\a';
            case 'b': return '\b';
            case 'f': return '\f';
            case 'v': return '\v';
            case '\\': return '\\';
            case '\'': return '\'';
            case '"': return '"';
            case '0': return '\0';
            default:
                return in;
        }
    }

    // Parses escaped string to character, e.g., "\\n" -> '\n', "\\x41" -> 'A'
    char getEscapedFromStr(const std::string& in, bool /*isStringContext*/) {
        if (in.empty()) return '\0';

        if (in[0] != '\\') {
            return in[0];
        }

        if (in.size() == 2) {
            return getEscapedFromChar(in[1]);
        }

        if (in.size() >= 4 && in[1] == 'x') {
            unsigned int value = 0;
            std::istringstream iss(in.substr(2));
            iss >> std::hex >> value;
            return static_cast<char>(value);
        }

        throw std::runtime_error("Invalid escape sequence: " + in);
    }

    // Returns the char code representation for escaped printable (e.g., '\n' -> 'n', 'A' -> 'A')
    // Useful for building tokens for error messages or serialization, not full escape strings
    char getCharFromEscaped(char in) {
        switch (in) {
            case '\n': return 'n';
            case '\r': return 'r';
            case '\t': return 't';
            case '\a': return 'a';
            case '\b': return 'b';
            case '\f': return 'f';
            case '\v': return 'v';
            case '\0': return '0';
            case '\\': return '\\';
            case '\"': return '"';
            case '\'': return '\'';
            default:
                return in;
        }
    }

    // Returns the char code representation as a string, e.g., '\n' -> "n", 'A' -> "A"
    // but does not prepend backslashes, for cases where the context is known
    std::string getCharFromEscapedAsStr(char in, bool stringContext) {
        switch (in) {
            case '\n': return "\\n";
            case '\r': return "\\r";
            case '\t': return "\\t";
            case '\a': return "\\a";
            case '\b': return "\\b";
            case '\f': return "\\f";
            case '\v': return "\\v";
            case '\0': return "\\0";
            case '\\': return "\\";
            case '"':
                return stringContext ? "\\\"" : "\"";
            case '\'':
                return stringContext ? "'" : "\\'";
            default:
                if (std::isprint(static_cast<unsigned char>(in))) {
                    return std::string(1, in);
                } else {
                    std::ostringstream oss;
                    oss << "x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                        << (static_cast<unsigned int>(static_cast<unsigned char>(in)));
                    return oss.str();
                }
        }
    }

    std::string join(const stdu::vector<std::string> &elements, const std::string &delimiter) {
        if (elements.empty()) return "";
        std::string joined = elements.front();
        for (auto it = elements.begin() + 1; it != elements.end(); ++it) {
            joined += delimiter + *it;
        }
        return joined;
    }
    stdu::vector<std::string> split(const std::string &str, const std::string &delimiter) {
        stdu::vector<std::string> tokens;
        std::size_t pos = 0;
        std::size_t prev_pos = 0;
        while ((pos = str.find(delimiter, prev_pos)) != std::string::npos) {
            tokens.push_back(str.substr(prev_pos, pos - prev_pos));
            prev_pos = pos + delimiter.length();
        }
        tokens.push_back(str.substr(prev_pos));
        return tokens;
    }
}