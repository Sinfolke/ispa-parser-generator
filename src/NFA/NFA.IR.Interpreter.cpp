module NFA.IR.Interpreter;

namespace NFA::Interpreter {
    auto IRInterpreter::matchMember(const AST::RuleMember *member, std::string_view input, std::size_t pos) -> std::size_t {
        if (!member || member->empty()) return 0;

        if (member->isString()) {
            const auto &str = member->getString().value;
            if (input.substr(pos).starts_with(str)) return str.size();
            return 0;
        }

        if (member->isEscaped()) {
            if (pos < input.size() && input[pos] == member->getEscaped().c) return 1;
            return 0;
        }

        if (member->isCsequence()) {
            if (pos >= input.size()) return 0;
            char ch = input[pos];
            const auto &cs = member->getCsequence();

            bool found = false;
            for (char c : cs.characters) if (ch == c) { found = true; break; }
            if (!found) {
                for (char c : cs.escaped) if (ch == c) { found = true; break; }
            }
            if (!found) {
                for (const auto &[low, high] : cs.diapasons) {
                    if (ch >= low && ch <= high) { found = true; break; }
                }
            }

            bool matched = cs.negative ? !found : found;
            return matched ? 1 : 0;
        }

        if (member->isAny()) {
            return (pos < input.size()) ? 1 : 0;
        }

        if (member->isNospace()) {
            return 0; // Zero-width match
        }

        if (member->isHex()) {
            const auto &hex = member->getHex().hex_chars;
            if (!hex.empty() && input.substr(pos).starts_with(hex)) return hex.size();
            return 0;
        }

        if (member->isBin()) {
            const auto &bin = member->getBin().bin_chars;
            if (!bin.empty() && input.substr(pos).starts_with(bin)) return bin.size();
            return 0;
        }

        return 0;
    }

    auto IRInterpreter::buildASTFromTrace(const std::vector<TraceStep> &trace) -> stdu::vector<ParsedASTNode> {
        if (trace.empty()) return {};

        stdu::vector<ParsedASTNode> root_nodes;
        std::vector<ParsedASTNode *> node_stack;

        for (const auto &step : trace) {
            if (step.matched_text.empty() && step.state.member == nullptr) {
                continue; // Skip structural/epsilon states with no text
            }

            const auto &rule_name = step.state.token_name;
            if (rule_name.empty()) continue;

            // Unwind stack to find common prefix parent node
            while (!node_stack.empty() && node_stack.back()->rule_name != rule_name) {
                const auto &stack_name = node_stack.back()->rule_name;
                bool is_parent = (rule_name.size() > stack_name.size()) &&
                                 std::equal(stack_name.begin(), stack_name.end(), rule_name.begin());
                if (is_parent) break;
                node_stack.pop_back();
            }

            if (node_stack.empty() || node_stack.back()->rule_name != rule_name) {
                ParsedASTNode new_node;
                new_node.rule_name = rule_name;
                new_node.start_pos = step.pos;
                new_node.end_pos = step.pos + step.matched_text.size();
                new_node.lexeme = step.matched_text;

                if (node_stack.empty()) {
                    root_nodes.push_back(std::move(new_node));
                    node_stack.push_back(&root_nodes.back());
                } else {
                    node_stack.back()->children.push_back(std::move(new_node));
                    node_stack.push_back(&node_stack.back()->children.back());
                }
            } else {
                auto *current = node_stack.back();
                current->lexeme += step.matched_text;
                current->end_pos = step.pos + step.matched_text.size();
            }
        }

        return root_nodes;
    }

    auto IRInterpreter::dfs(const NFA::InitialAPI::TokenID &curr_state,
             std::string_view input,
             std::size_t pos,
             std::vector<TraceStep> &current_trace,
             utype::unordered_set<std::pair<NFA::InitialAPI::TokenID, std::size_t>> &visited) const -> std::optional<std::vector<TraceStep>> {

        auto state_key = std::make_pair(curr_state, pos);
        if (visited.contains(state_key)) return std::nullopt;
        visited.insert(state_key);

        std::size_t consumed = 0;
        if (curr_state.member != nullptr) {
            consumed = matchMember(curr_state.member, input, pos);
            if (consumed == 0 && !curr_state.member->isNospace() && !curr_state.member->empty()) {
                return std::nullopt; // Terminal failed to match
            }
        }

        std::string matched_str = std::string(input.substr(pos, consumed));
        current_trace.push_back({curr_state, pos, matched_str});
        std::size_t next_pos = pos + consumed;

        // Check if current state reaches ACCEPT or end of input
        auto it = main_token.transitions.find(curr_state);
        if (it == main_token.transitions.end() || it->second.empty()) {
            if (next_pos == input.size()) {
                return current_trace;
            }
            current_trace.pop_back();
            return std::nullopt;
        }

        for (const auto &next_state : it->second) {
            auto result = dfs(next_state, input, next_pos, current_trace, visited);
            if (result.has_value()) return result;
        }

        current_trace.pop_back();
        return std::nullopt;
    }
    auto IRInterpreter::parse(std::string_view input) const -> ParseResult {
        ParseResult result;
        result.input = std::string(input);

        if (main_token.transitions.empty()) return result;

        // Find initial entry state (position_in_token == 0)
        InitialAPI::TokenID start_state;
        bool found_start = false;
        for (const auto &[key, _] : main_token.transitions) {
            if (key.position_in_token == 0) {
                start_state = key;
                found_start = true;
                break;
            }
        }

        if (!found_start) {
            start_state = main_token.transitions.begin()->first;
        }

        std::vector<TraceStep> trace;
        utype::unordered_set<std::pair<InitialAPI::TokenID, std::size_t>> visited;

        auto final_trace = dfs(start_state, input, 0, trace, visited);
        if (final_trace.has_value()) {
            result.success = true;
            result.consumed_bytes = input.size();
            result.ast_nodes = buildASTFromTrace(*final_trace);
        }

        return result;
    }

    auto IRInterpreter::parseAll(const stdu::vector<std::string> &inputs) const -> stdu::vector<ParseResult> {
        stdu::vector<ParseResult> results;
        results.reserve(inputs.size());
        for (const auto &input : inputs) {
            results.push_back(parse(input));
        }
        return results;
    }
}