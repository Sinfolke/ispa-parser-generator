export module NFA.IR.API;

import AST.API;
import hash;
import dstd;
import std;

export namespace NFA::IR {
    inline constexpr auto NULL_STATE = std::numeric_limits<std::size_t>::max();
    struct Capture {
        AST::RuleMember member;
        stdu::vector<std::string> token_name;
        std::size_t position_in_token = NULL_STATE;
        std::size_t call = NULL_STATE;
        std::size_t group = NULL_STATE;
        bool operator==(const Capture &other) const = default;
        bool operator<(const Capture &other) const {
            if (member == other.member) {
                return std::tie(token_name, position_in_token, call, group) <
               std::tie(other.token_name, other.position_in_token, other.call, other.group);
            } else {
                return &member < &other.member;
            }
        }
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(member, token_name, position_in_token, call, group);
        }
    };
    struct TokenID {
        AST::RuleMember member;
        AST::RuleMember original_member; // copy of the original member in the AST::Tree
        stdu::vector<std::string> token_name;
        std::size_t position_in_token;
        std::size_t call = NULL_STATE;
        std::size_t group = NULL_STATE;
        stdu::vector<std::size_t> alt;
        stdu::vector<TokenID*> capture;
        TokenID *prev;
        bool operator==(const TokenID &other) const = default;
        bool operator<(const TokenID &other) const {
            if (member == other.member) {
                return std::tie(token_name, position_in_token, call, group, alt) < std::tie(other.token_name, other.position_in_token, other.call, other.group, other.alt);
            } else {
                return &member < &other.member;
            }
        }
    private:
        friend struct ::uhash;
        auto members() const {
            return std::tie(member, token_name, position_in_token, call, group, capture, alt);
        }
    };
    using TransitionValue = TokenID;
    using Transition = std::pair<TokenID, stdu::vector<TransitionValue>>;
    using Transitions = utype::unordered_map<TokenID, stdu::vector<TransitionValue>>;
    struct Token {
        stdu::vector<std::string> name;
        Transitions transitions;
        const AST::DataBlock *data_block;
        bool top_level;

        bool operator==(const Token &) const = default;
        bool operator<(const Token& other) const {
            return name < other.name; // name can be the only distinguish
        }
    };

    auto operator<<(std::ostream &os, const TokenID &id) -> std::ostream &;
    auto operator<<(std::ostream &os, const Transition &transition) -> std::ostream &;
    auto operator<<(std::ostream &os, const Transitions &transitions) -> std::ostream &;
    auto operator<<(std::ostream &os, const Token &token) -> std::ostream &;
}