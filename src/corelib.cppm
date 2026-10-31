module;
#ifdef __EMSCRIPTEN__
#include "ispa_icu_shim.h"
#else
#include <unicode/umachine.h>
#endif
export module corelib;
import dstd;
import std;
export namespace corelib {
    namespace file {
        std::string readFile(const std::filesystem::path filePath);
        std::string readFile(const std::string& filePath);
        stdu::vector<std::string> getFilesRecursively(const std::string& dir, const std::string& ext);
    }
    namespace text {
        bool startsWith(const std::string& str, const std::string& prefix);
        bool startsWithRange(const std::string& str, char from, char to);
        bool isAllAlpha(const std::string& str);
        bool isUpper(const char* str);
        bool isUpper(const std::string str);
        bool isLower(const char* str);
        bool isLower(const std::string str);
        std::string ToUpper(std::string str);
        std::string ToLower(std::string str);
        std::string join(const stdu::vector<std::string> &elements, const std::string &delimiter);
        stdu::vector<std::string> split(const std::string &str, const std::string &delimiter);
        char getCharFromEscaped(char in);
        std::string getCharFromEscapedAsStr(char in, bool string);
        std::string getEscapedAsStr(char in, bool string);
        char getEscapedFromChar(char in);
        char getEscapedFromStr(const std::string& in, bool isStringContext);
        std::string characterName(UChar32 cp);
    }
    namespace range {
        template<typename InputIt, typename T>
        std::vector<InputIt> find_all(InputIt first, InputIt last, const T& value) {
            std::vector<InputIt> result;
            while (first != last) {
                first = std::find(first, last, value);
                if (first != last) {
                    result.push_back(first);
                    ++first;
                }
            }
            return result;
        }
        template<typename InputIt, typename Func>
        std::vector<InputIt> find_all_if(InputIt first, InputIt last, const Func func) {
            std::vector<InputIt> result;
            while (first != last) {
                first = std::find_if(first, last, func);
                if (first != last) {
                    result.push_back(first);
                    ++first;
                }
            }
            return result;
        }
    }
}
