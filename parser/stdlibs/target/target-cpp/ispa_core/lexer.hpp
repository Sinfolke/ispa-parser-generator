// ISPA_CORE::Lexer_base<Token>: the ispastdlib lexer interface with DFA execution done by the lowered core
// (python::ispa_std::dfa_run::run). Everything else is inherited from ISPA_STD::Lexer_base unchanged.
#pragma once
#include <ispa_core/namespace.hpp>
#include <ispastdlib/lexer.hpp>
#include <ispa_core_lowered.h>
#include <memory>
#include <optional>
#include <unordered_map>

namespace ISPA_CORE {
namespace core_detail {
namespace lo = ::python::ispa_std;

// The typed static tables, converted to the core's dynamic ones once per table.
inline std::shared_ptr<lo::dfa_run::DfaTables> core_tables(
    const void* key, const std::vector<std::vector<long long>>& table, const std::vector<long long>& classes,
    const std::vector<std::vector<long long>>& actions, std::size_t registers,
    const std::vector<ISPA_STD::DFA::API::DFADebug>& debug, const ISPA_STD::DFA::API::DebugIndex& debug_index) {
    static std::unordered_map<const void*, std::shared_ptr<lo::dfa_run::DfaTables>> cache;
    if (auto found = cache.find(key); found != cache.end()) return found->second;
    auto layout = ISPA_STD::construct<lo::dfa_api::TdfaLayout>(static_cast<long long>(registers), 0LL);
    std::vector<std::shared_ptr<lo::dfa_api::DFADebug>> debug_array;
    for (const auto& d : debug)
        debug_array.push_back(ISPA_STD::construct<lo::dfa_api::DFADebug>(d.member, d.token_name, d.position_in_token,
            d.call, d.group, d.rule_run, d.offset, d.length, std::string(1, d.ch)));
    auto index = ISPA_STD::construct<lo::dfa_api::DebugIndex>();
    for (const auto& [state, classes_of_state] : debug_index)
        for (const auto& [cls, value] : classes_of_state) index->put(state, cls, value);
    auto made = ISPA_STD::construct<lo::dfa_run::DfaTables>(table, classes, actions, layout, debug_array, index);
    cache.emplace(key, made);
    return made;
}
}

// The DFA interpreter hook of ISPA_STD::Lexer_base::lookup (ISPA_STDLIB_DFA_RUN): same arguments as
// ISPA_STD::DFA::run, executed by the lowered core.
template<typename Token, typename SemanticFunc, std::size_t table_states, std::size_t table_classes,
         std::size_t action_table_states, std::size_t debug_size, std::size_t registers, std::size_t outputs>
Token core_dfa_run_typed(const char*& pos, long long start_pos, long long& scanning_line,
                   const ISPA_STD::DFA::API::Table<table_states, table_classes>& table,
                   const ISPA_STD::DFA::API::CharToClass& class_table,
                   const ISPA_STD::DFA::API::LRTable<action_table_states>& action_table,
                   SemanticFunc semantic,
                   const ISPA_STD::DFA::API::DFADebugTable<debug_size>& debug_array,
                   const ISPA_STD::DFA::API::DebugIndex& debug_index,
                   ISPA_STD::DFA::API::TdfaLayout<registers, outputs>) {
    namespace lo = core_detail::lo;
    const char* const base = pos - start_pos;
    // The core scans a std::string: keep one copy of the input per buffer.
    // A scan that starts at the beginning of the input refreshes the copy; later tokens reuse it.
    static thread_local const char* source_key = nullptr;
    static thread_local std::shared_ptr<lo::dfa_api::SourceText> source;
    if (source_key != base || start_pos == 0) {
        ISPA_STD::HeapScope heap;
        source_key = base;
        source = ISPA_STD::construct<lo::dfa_api::SourceText>(std::string(base));
    }

    static std::unordered_map<const void*, std::shared_ptr<lo::dfa_run::DfaTables>> cache;
    std::shared_ptr<lo::dfa_run::DfaTables> tables;
    if (auto found = cache.find(&table); found != cache.end()) tables = found->second;
    else {
        ISPA_STD::HeapScope heap;
        std::vector<std::vector<long long>> dynamic_table, dynamic_actions;
        std::vector<long long> dynamic_classes;
        for (const auto& row : table) dynamic_table.emplace_back(row.begin(), row.end());
        for (const auto& row : action_table) dynamic_actions.emplace_back(row.begin(), row.end());
        for (auto cls : class_table) dynamic_classes.push_back(static_cast<long long>(cls));
        std::vector<ISPA_STD::DFA::API::DFADebug> debug(debug_array.begin(), debug_array.end());
        tables = core_detail::core_tables(&table, dynamic_table, dynamic_classes, dynamic_actions, registers, debug, debug_index);
        cache.emplace(&table, tables);
    }

    // One cursor and one result pair serve every token: the core does not keep them.
    static thread_local std::shared_ptr<lo::refs::Cursor> cursor;
    static thread_local std::shared_ptr<lo::utility::Pair<long long, std::shared_ptr<lo::raw_token::RawToken>>> result_pair;
    if (!cursor) {
        ISPA_STD::HeapScope heap;
        cursor = ISPA_STD::construct<lo::refs::Cursor>(0LL, 1LL);
        result_pair = ISPA_STD::construct<lo::utility::Pair<long long, std::shared_ptr<lo::raw_token::RawToken>>>(0LL, std::shared_ptr<lo::raw_token::RawToken>{});
    }
    cursor->pos = static_cast<long long>(pos - base);
    cursor->line = scanning_line;
    // The pair must not keep an arena object alive past the scan that created it.
    struct Release { decltype(result_pair)& pair; ~Release() { pair->second.reset(); } } release{result_pair};
    // The typed semantic action builds the typed token; it is kept here, not boxed in std::any, while the
    // core only sees an empty carrier token.
    std::optional<Token> accepted;
    // the empty token the core carries in place of the typed one: one for the whole program
    static const std::shared_ptr<lo::raw_token::RawToken> carrier = [] {
        ISPA_STD::HeapScope heap;
        return lo::raw_token::RawToken::empty_token();
    }();
    auto adapter = [&](long long index, std::shared_ptr<lo::dfa_api::Captures> captures, long long first_pos,
                       long long length, long long start_line, std::shared_ptr<lo::refs::Cursor> cur)
        -> std::shared_ptr<lo::utility::Pair<long long, std::shared_ptr<lo::raw_token::RawToken>>> {
        // The registers live in the scan state shared by every token: read them in place.
        auto& scan_state = *captures->_state;
        std::vector<ISPA_STD::DFA::API::HistoryNode> history;  // empty unless the token has list captures
        for (const auto& node : scan_state.history)
            history.push_back({static_cast<std::ptrdiff_t>(node->pos), static_cast<std::ptrdiff_t>(node->prev)});
        const char* const start = base + scan_state.base;
        const std::ptrdiff_t* regs;
        std::vector<std::ptrdiff_t> narrowed_regs;  // only where ptrdiff_t is narrower than long long (wasm32)
        if constexpr (sizeof(long long) == sizeof(std::ptrdiff_t)) {
            regs = reinterpret_cast<const std::ptrdiff_t*>(scan_state.regs.data());
        } else {
            narrowed_regs.assign(scan_state.regs.begin(), scan_state.regs.end());
            regs = narrowed_regs.data();
        }
        ISPA_STD::DFA::API::Captures typed(start, regs, history.data(), start_line);
        const char* at = base + cur->pos;
        auto result = [&] {
            if constexpr (std::is_invocable_v<SemanticFunc, std::size_t, const ISPA_STD::DFA::API::Captures&,
                                              long long, const char*, long long, long long, const char*&>)
                return semantic(static_cast<std::size_t>(index), typed, first_pos, start, length, start_line, at);
            else
                return semantic(static_cast<std::size_t>(index), typed, first_pos, start, length, start_line);
        }();
        cur->pos = at - base;
        const auto next = static_cast<long long>(std::get<0>(result));
        if (next == static_cast<long long>(ISPA_STD::DFA::API::null_state)) accepted = std::get<1>(std::move(result));
        result_pair->first = next;
        result_pair->second = carrier;
        return result_pair;
    };
    // The adapter captures a dozen references: handing it to the core's std::function would heap-allocate it for every
    // token. A one-pointer forwarder stays inside std::function's inline storage.
    using AdapterResult = std::shared_ptr<lo::utility::Pair<long long, std::shared_ptr<lo::raw_token::RawToken>>>;
    auto* const adapter_ptr = &adapter;
    std::function<AdapterResult(long long, std::shared_ptr<lo::dfa_api::Captures>, long long, long long, long long,
                                std::shared_ptr<lo::refs::Cursor>)>
        forward = [adapter_ptr](long long index, std::shared_ptr<lo::dfa_api::Captures> captures, long long first_pos,
                                long long length, long long start_line, std::shared_ptr<lo::refs::Cursor> cur) {
            return (*adapter_ptr)(index, std::move(captures), first_pos, length, start_line, std::move(cur));
        };
    lo::dfa_run::run(source, cursor, start_pos, tables, forward, carrier);
    pos = base + cursor->pos;
    scanning_line = cursor->line;
    return accepted ? std::move(*accepted) : Token{};
}

template<typename Token, typename... Arguments>
Token core_dfa_run(const char*& pos, long long start_pos, long long& scanning_line, Arguments&&... rest) {
    return core_dfa_run_typed<Token>(pos, start_pos, scanning_line, std::forward<Arguments>(rest)...);
}
}
