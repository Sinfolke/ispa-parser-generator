#pragma once
#include <ispastdlib/node.hpp>
#include <stdexcept>
#include <algorithm>

namespace ISPA_STD {
class ExternalParseError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};
template<class Data> struct ExternalResult {
    bool status = false;
    Node<Data> node;
    const char* end = nullptr;
};
template<class Data>
Node<Data> external_node(const char* origin, const char* start, const char* end, Node<Data> node) {
    if (node.empty()) return {};
    const auto line = 1 + std::count(origin, start, '\n');
    const char* line_start = start;
    while (line_start > origin && line_start[-1] != '\n') --line_start;
    return Node<Data>{static_cast<std::size_t>(start - origin), start, end,
        static_cast<std::size_t>(end - start), static_cast<std::size_t>(line),
        static_cast<std::size_t>(start - line_start + 1), node.name(), std::move(node.data())};
}

template<class Binding, class IT>
bool external_starts(const IT& pos) {
    if constexpr (detail::detected_v<detail::source_origin_position, IT>)
        return Binding::starts(pos.sourceOrigin(), pos.sourcePosition());
    else throw std::logic_error("AMBIGUOUS eager token stream: external parsers require lazy tokenization");
}

template<class Binding, class IT>
auto external_match(IT pos) -> MatchResult<typename Binding::node_type, IT> {
    if constexpr (detail::detected_v<detail::source_origin_position, IT>) {
        if (!Binding::starts(pos.sourceOrigin(), pos.sourcePosition())) return {};
        auto result = [&] {
            try { return Binding::parse(pos.sourceOrigin(), pos.sourcePosition()); }
            catch (const ExternalParseError&) { throw; }
            catch (const std::runtime_error& error) { throw ExternalParseError(error.what()); }
        }();
        if (!result.status) return {};
        pos.consumeExternal(result.end);
        return {true, std::move(result.node), std::move(pos)};
    } else throw std::logic_error("AMBIGUOUS eager token stream: external parsers require lazy tokenization");
}
}
