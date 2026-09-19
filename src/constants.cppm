export module constants;
import std;

export namespace constants {
    const std::vector<std::string> whitespace = {"__WS"};
    const std::vector<std::string> whitespace_token = {"__WSTOKEN"};
    const std::vector<std::string> whitespace_rule = {"__WSRULE"};
    const std::vector<char> whitespace_chars = {'\t', '\n', '\r', '\v', '\f', ' '};

    constexpr std::size_t NULL_STATE = std::numeric_limits<std::size_t>::max();
}
