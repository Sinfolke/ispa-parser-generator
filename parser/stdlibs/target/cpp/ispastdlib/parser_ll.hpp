/**
 * ispastdlib/parser.hpp - LLParser_base
 * Part of the split ISPA std lib; include <ispastdlib.hpp> for everything.
 */
#pragma once
#include <ispastdlib/core.hpp>
#include <ispastdlib/node.hpp>
#include <ispastdlib/lexer.hpp>
#include <ispastdlib/raw_token.hpp>
#include <ispastdlib/token_flow.hpp>
#include <ispastdlib/traits.hpp>

#include <ispastdlib/parser_base.hpp>
#include <ispastdlib/ispa_recovery.hpp>
#include <ispastdlib/ispa_recovery_context.hpp>
#include <ispastdlib/counter.hpp>
#include <ispastdlib/recovery_scan.hpp>
#include <ispastdlib/external.hpp>

#include <optional>
#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <map>
#include <cctype>
#include <functional>
#include <cstring>
#include <typeinfo>

namespace ISPA_STD {
    template<class IT>
    Recovery::TokenId lookahead_kind(const IT& pos) {
        if constexpr (detail::detected_v<detail::is_end, IT>) {
            if (pos.isEnd()) return Recovery::EndOfInput;
        } else if (pos->isEnd()) {
            return Recovery::EndOfInput;
        }
        return static_cast<Recovery::TokenId>((*pos).kind());
    }

    template<class Lexer, class Token>
    class BoundedSourceLexer : public Lexer {
        const char* boundary_ = nullptr;
    public:
        explicit BoundedSourceLexer(const char* boundary) : boundary_(boundary) {
            this->setExternalBoundary(true);
        }
        Token makeToken(const char*& pos) override {
            if (pos >= boundary_) return {};
            return Lexer::makeToken(pos);
        }
        auto clone() const -> std::shared_ptr<Lexer_base<Token>> override {
            return std::make_shared<BoundedSourceLexer>(*this);
        }
    };

    // Run a generated rule against a separate token stream. The returned node
    // owns its data; the temporary cursor never escapes with the result.
    template<class Lexer, class Tokens, class Rule>
    auto parse_token_stream(const Tokens& tokens, Rule&& rule) {
        Lexer lexer;
        for (const auto& token : tokens) lexer.append_token(token);
        lexer.push();
        typename Lexer::iterator begin(lexer);
        auto result = std::invoke(std::forward<Rule>(rule), begin);
        using Node = std::decay_t<decltype(result.node)>;
        if (!result.status || !result.it.isEnd()) return Node{};
        return Node{std::move(result.node)};
    }

    // External rules need the original source bytes. Tokens collected from a
    // contiguous source region provide its bounds while retaining the source
    // pointers in the returned AST.
    template<class Lexer, class Tokens, class Rule>
    auto parse_source_span(const Tokens& tokens, Rule&& rule) {
        using Cursor = typename Lexer::lazy_iterator;
        using Result = std::invoke_result_t<Rule, Cursor>;
        using Node = std::decay_t<decltype(std::declval<Result>().node)>;
        if (tokens.empty()) return Node{};
        const auto& first = tokens.front().meta();
        const auto& last = tokens.back().meta();
        if (!first.start() || !last.end() || last.end() < first.start())
            throw std::invalid_argument("source stream requires tokens with source spans");
        const char* origin = first.start() - first.startpos();
        BoundedSourceLexer<Lexer, typename Tokens::value_type> lexer(last.end());
        Cursor begin(lexer, origin, first.start());
        auto result = std::invoke(std::forward<Rule>(rule), begin);
        if (!result.status || !result.it.isEnd() || result.it.sourcePosition() > last.end()) return Node{};
        return Node{std::move(result.node)};
    }

    /* PARSER */
    template<typename MainNode, typename Token>
    class LLParser : public Parser_base<MainNode, Token> {
    public:
        using Parser_base<MainNode, Token>::parse;

        // Diagnostics from the current/last parse, including a parse that threw.
        // A new parse clears them; callers control how they are displayed.
        [[nodiscard]] const std::vector<Recovery::Diagnostic>& getErrors() const noexcept {
            return recovery_state_.diagnostics;
        }
        // Diagnostics produced by hierarchical (context) recovery.
        [[nodiscard]] const std::vector<Recovery::ContextDiagnostic>& getContextErrors() const noexcept {
            return context_errors_;
        }

    protected:
        static constexpr std::size_t max_action_lookahead = 64;
        static constexpr std::size_t max_action_checkpoints = 32;

        template<class IT>
        static bool context_at(IT candidate, std::string_view spelling, std::size_t lookahead,
                               const char* (*token_name)(std::size_t)) {
            if (lookahead > max_action_lookahead)
                throw std::out_of_range("ctx.at lookahead exceeds 64 tokens");
            for (std::size_t i = 0; i < lookahead; ++i) {
                if (candidate.isEnd()) return false;
                ++candidate;
            }
            if (candidate.isEnd()) return false;
            const auto& meta = candidate->meta();
            if (meta.start() && std::string_view(meta.start(), meta.length()) == spelling) return true;
            if (!token_name) return false;
            if constexpr (detail::detected_v<detail::ambiguous_id, Token>)
                if (Recovery::tokenId(*candidate) == Token::ambiguous_id) return false;
            std::string name(spelling);
            for (std::size_t i = 0; (i = name.find("::", i)) != std::string::npos;)
                name.replace(i, 2, ".");
            const std::string_view actual = token_name(Recovery::tokenId(*candidate));
            return actual == name || (actual.size() > name.size() &&
                actual.compare(actual.size() - name.size(), name.size(), name) == 0 &&
                actual[actual.size() - name.size() - 1] == '.');
        }

        template<class IT, class AST>
        struct ActionCheckpoint {
            IT& pos;
            std::vector<std::pair<std::string, IT>> checkpoints;

            void mark(const std::string& name) {
                if (name.empty()) throw std::invalid_argument("checkpoint name cannot be empty");
                if (checkpoints.size() >= max_action_checkpoints)
                    throw std::length_error("action checkpoint limit (32) exceeded");
                checkpoints.emplace_back(name, pos);
            }
            void restore(const std::string& name) {
                const auto found = std::find_if(checkpoints.rbegin(), checkpoints.rend(),
                    [&](const auto& checkpoint) { return checkpoint.first == name; });
                if (found == checkpoints.rend())
                    throw std::logic_error("unknown action checkpoint '" + name + "'");
                const auto index = checkpoints.size() - 1 - static_cast<std::size_t>(found - checkpoints.rbegin());
                pos = checkpoints[index].second;
                checkpoints.erase(checkpoints.begin() + static_cast<std::ptrdiff_t>(index + 1), checkpoints.end());
            }
            void commit(const std::string& name) {
                const auto found = std::find_if(checkpoints.rbegin(), checkpoints.rend(),
                    [&](const auto& checkpoint) { return checkpoint.first == name; });
                if (found == checkpoints.rend())
                    throw std::logic_error("unknown action checkpoint '" + name + "'");
                const auto index = checkpoints.size() - 1 - static_cast<std::size_t>(found - checkpoints.rbegin());
                checkpoints.erase(checkpoints.begin() + static_cast<std::ptrdiff_t>(index), checkpoints.end());
            }
        };

        // Contexts borrow invocation-local storage. Cursor and AST changes are
        // immediately visible to subsequent matches and to the rule's caller.
        template<class IT, class AST>
        struct RuleContext : ActionCheckpoint<IT, AST> {
            IT& pos;
            AST& ast;
            const IT start_pos;
            const std::string rule;
            const char* (*token_name)(std::size_t) = nullptr;

            bool at(std::string_view spelling, std::size_t lookahead = 0) const {
                return LLParser::template context_at<IT>(pos, spelling, lookahead, token_name);
            }

            // Collect the contents of a braced region after its opening token
            // has been consumed. The closing token is consumed, but is not
            // included in the returned stream. Nested braces are preserved.
            TokenFlow<Token> collect_braced() {
                TokenFlow<Token> collected;
                if constexpr (detail::detected_v<detail::source_position, IT>) {
                    const char* begin = pos.sourcePosition();
                    const std::string_view stops[]{"}"};
                    const auto scanned = Recovery::scanBalanced(begin, stops, std::strlen(begin));
                    if (scanned.exhausted || *scanned.end != '}')
                        throw std::runtime_error("unterminated braced region in parser action");
                    while (!pos.isEnd() && pos.sourcePosition() < scanned.end) {
                        collected.push_back(*pos);
                        ++pos;
                    }
                    if (pos.isEnd() || pos.sourcePosition() != scanned.end || !at("}"))
                        throw std::runtime_error("braced region boundary is not a token");
                    ++pos;
                    return collected;
                }
                std::size_t depth = 1;
                while (!pos.isEnd()) {
                    if (at("{")) {
                        ++depth;
                    } else if (at("}")) {
                        if (--depth == 0) {
                            ++pos;
                            return collected;
                        }
                    }
                    collected.push_back(*pos);
                    ++pos;
                }
                throw std::runtime_error("unterminated braced region in parser action");
            }
        };
        template<class IT, class AST>
        struct FailContext : ActionCheckpoint<IT, AST> {
            IT& pos;
            AST& ast;
            const IT start_pos;
            const std::string rule;
            const std::string failed_rule;
            Recovery::State& recovery;
            const char* source;
            const std::size_t diagnostic_begin;
            const char* (*token_name)(std::size_t) = nullptr;
            std::optional<Recovery::Mode> requested;
            std::size_t skipped = 0;

            bool at(std::string_view spelling, std::size_t lookahead = 0) const {
                return LLParser::template context_at<IT>(pos, spelling, lookahead, token_name);
            }

            // Custom diagnostics use the same collection as automatic repairs.
            void error(const std::string& message, const std::string& code = {},
                       std::optional<std::size_t> source_offset = {},
                       std::optional<std::size_t> source_length = {}) {
                if (source_offset.has_value() != source_length.has_value())
                    throw std::invalid_argument("ctx.error source span requires both offset and length");
                if (recovery.exhausted()) { abort(); return; }
                Recovery::Diagnostic diagnostic;
                diagnostic.rule = rule;
                diagnostic.expected_text = failed_rule;
                diagnostic.message = message;
                diagnostic.code = code;
                if constexpr (detail::detected_v<detail::source_origin_position, IT>) {
                    // Do not force host tokenization of a broken foreign payload.
                    const auto* p = pos.sourcePosition();
                    if (p) {
                        diagnostic.offset = p - pos.sourceOrigin();
                        diagnostic.length = *p ? 1 : 0;
                    }
                } else if (!pos.isEnd()) {
                    diagnostic.unexpected = Recovery::tokenId(*pos);
                    diagnostic.offset = pos->meta().startpos();
                    diagnostic.length = pos->meta().length();
                } else if (source) {
                    diagnostic.offset = std::char_traits<char>::length(source);
                }
                if (source_offset) {
                    const auto input_length = source ? std::char_traits<char>::length(source) : 0;
                    if (*source_offset > input_length || *source_length > input_length - *source_offset)
                        throw std::out_of_range("ctx.error source span is outside parser input");
                    diagnostic.offset = *source_offset;
                    diagnostic.length = *source_length;
                }
                recovery.diagnostics.push_back(std::move(diagnostic));
            }
            void note(const std::string& message) {
                if (recovery.diagnostics.size() <= diagnostic_begin)
                    throw std::logic_error("ctx.note requires a preceding ctx.error in this fail block");
                recovery.diagnostics.back().notes.push_back(message);
            }
            template<class... Boundaries>
            void sync(Boundaries... boundaries) {
                static_assert(sizeof...(Boundaries) > 0);
                if (recovery.exhausted() || requested == Recovery::Mode::Abort) { abort(); return; }
                const std::string_view stops[]{boundaries...};
                for (auto stop : stops) if (stop.empty())
                    throw std::invalid_argument("ctx.sync requires nonempty boundaries");
                if constexpr (detail::detected_v<detail::consume_external, IT>) {
                    const char* begin = pos.sourcePosition();
                    const auto budget = std::min<std::size_t>(4096,
                        recovery.max_total_skipped - recovery.total_skipped);
                    const auto scanned = Recovery::scanBalanced(begin, stops, budget);
                    const auto count = static_cast<std::size_t>(scanned.end - begin);
                    if (count) pos.consumeExternal(scanned.end);
                    skipped += count;
                    recovery.total_skipped += count;
                    if (scanned.exhausted || recovery.exhausted()) abort();
                } else {
                    throw std::logic_error("ctx.sync requires a lazy source cursor");
                }
            }
            // Preserve token-stream structure (including synthetic indentation)
            // instead of bypassing the lexer as source-level sync does.
            template<class... Boundaries>
            void sync_tokens(Boundaries... boundaries) {
                static_assert(sizeof...(Boundaries) > 0);
                const std::string_view stops[]{boundaries...};
                for (auto stop : stops) if (stop.empty())
                    throw std::invalid_argument("ctx.sync_tokens requires nonempty boundaries");
                while (true) {
                    if (recovery.exhausted() || requested == Recovery::Mode::Abort) { abort(); return; }
                    if (pos.isEnd()) return;
                    for (auto stop : stops) if (at(stop)) return;
                    skip();
                }
            }
            void skip() {
                if (recovery.exhausted()) { abort(); return; }
                if (!pos.isEnd()) { ++pos; ++skipped; ++recovery.total_skipped; }
            }
            void insert() { if (requested != Recovery::Mode::Abort) requested = Recovery::Mode::Insert; }
            void replace() { skip(); insert(); }
            void retry() { if (requested != Recovery::Mode::Abort) requested = Recovery::Mode::Retry; }
            void abort() { requested = Recovery::Mode::Abort; }

            Recovery::Decision finish(const IT& before) {
                const auto begin = cursor_position(before), end = cursor_position(pos);
                const auto moved = end > begin ? end - begin : 0;
                // Also account for legacy handlers that change ctx.pos directly.
                if (moved > skipped) recovery.total_skipped += moved - skipped;
                Recovery::Decision decision;
                decision.mode = recovery.exhausted() ? Recovery::Mode::Abort :
                    requested.value_or(begin == end ? Recovery::Mode::Insert : Recovery::Mode::Retry);
                if (decision.mode == Recovery::Mode::Insert && moved) decision.mode = Recovery::Mode::Replace;
                decision.skipped = std::max(moved, skipped);
                for (auto i = diagnostic_begin; i < recovery.diagnostics.size(); ++i) {
                    auto& diagnostic = recovery.diagnostics[i];
                    diagnostic.decision = decision;
                    diagnostic.decision.unexpected = diagnostic.unexpected;
                }
                return decision;
            }
        };
        template<class IT, class AST>
        static auto rule_context(IT& pos, AST& ast, IT start, std::string rule) {
            return RuleContext<IT, AST>{{pos, {}}, pos, ast, start, std::move(rule)};
        }
        template<class IT, class AST>
        static auto rule_context(IT& pos, AST& ast, IT start, std::string rule,
                                 const char* (*token_name)(std::size_t)) {
            return RuleContext<IT, AST>{{pos, {}}, pos, ast, start, std::move(rule), token_name};
        }
        template<class IT, class AST>
        auto fail_context(IT& pos, AST& ast, IT start,
                          std::string rule, std::string failed_rule,
                          const char* (*token_name)(std::size_t) = nullptr) {
            return FailContext<IT, AST>{{pos, {}}, pos, ast, start, std::move(rule), std::move(failed_rule),
                recovery_state_, this->text, recovery_state_.diagnostics.size(), token_name};
        }
        template<class IT>
        static auto custom_failure_decision(const IT& before, const IT& after) {
            Recovery::Decision decision;
            decision.mode = cursor_position(before) == cursor_position(after)
                ? Recovery::Mode::Insert : Recovery::Mode::Retry;
            return decision;
        }
        template<class IT, class AST>
        static auto custom_failure_decision(const IT& before, const IT&, FailContext<IT, AST>& ctx) {
            return ctx.finish(before);
        }
        template<class Action>
        struct ActionFailure {
            static constexpr bool custom_action = true;
            Action action;
            template<class IT>
            auto operator()(IT& pos, Recovery::State& state) { return action(pos, state); }
        };
        template<class Action>
        static auto custom_failure(Action action) { return ActionFailure<Action>{std::move(action)}; }
        template <typename AST, typename IT>
        static MatchResult<AST, std::decay_t<IT>> action_result(AST& ast, IT pos) {
            MatchResult<AST, std::decay_t<IT>> result;
            result.status = true;
            result.node.set_data(std::move(ast));
            result.it = std::move(pos);
            return result;
        }
        Recovery::State recovery_state_;
        std::map<std::string, std::size_t, std::less<>> counters_;

        // Bindings belong to an invocation; callees see their caller's bindings.
        struct CounterScope {
            LLParser& parser;
            decltype(counters_) saved;
            explicit CounterScope(LLParser& parser)
                : parser(parser), saved(parser.counters_) {}
            ~CounterScope() { parser.counters_.swap(saved); }
            CounterScope(const CounterScope&) = delete;
            CounterScope& operator=(const CounterScope&) = delete;
        };
        auto counter_scope() { return CounterScope(*this); }

        template<class Value>
        void bind_counter(std::string_view name, const Value& value) {
            const auto count = [&]() -> std::size_t {
                if constexpr (detail::detected_v<detail::size, Value>) return value.size();
                else return value.empty() ? 0 : 1;
            }();
            const auto [it, inserted] = counters_.try_emplace(std::string(name), count);
            if (!inserted && it->second != count)
                throw std::logic_error("Shared counter '" + std::string(name) + "' has a different count");
        }

        // ---------------------------------------------------------------
        // Hierarchical (context) recovery
        //
        // Every rule function owns a Frame (recovery_frame). Before each child
        // member, generated code calls recovery_at<Sets...>(expected, names...),
        // which records the *claims* of that frame at this point: the productions
        // / terminals the rule could still accept (the member itself if repeated,
        // plus the suffix up to the first required member).
        //
        // On a committed failure that local repair cannot fix, the parser scans
        // ahead and finds the earliest token claimed by any active frame
        // (innermost first), skips to it, and unwinds to the owning frame. The
        // owner's own loop/sequence then parses the construct normally.
        //
        // ASSUMPTION: every Set type passed to recovery_at provides
        //     static bool matches(const Token&)
        // ---------------------------------------------------------------
        struct Frame {
            std::string_view rule;
            std::string_view expecting;
            std::vector<Recovery::ContextClaim> claims;
        };
        std::vector<Frame> frames_;
        std::size_t unwind_ = 0;                       // frames still to abandon
        static constexpr std::size_t npos_ = static_cast<std::size_t>(-1);
        std::size_t last_resume_pos_ = npos_;          // progress guard
        bool recovering_ = false;                      // suppress cascading reports
        std::size_t context_max_skip_ = 64;
        std::vector<Recovery::ContextDiagnostic> context_errors_;

        void reset_recovery() {
            counters_.clear();
            recovery_state_.diagnostics.clear();
            recovery_state_.total_skipped = 0;
            recovery_state_.anchors.clear();
            frames_.clear();
            unwind_ = 0;
            last_resume_pos_ = npos_;
            recovering_ = false;
            context_errors_.clear();
        }
        MainNode& parse() override {
            reset_recovery();
            return Parser_base<MainNode, Token>::parse();
        }
        MainNode& parseLazy() override {
            reset_recovery();
            return Parser_base<MainNode, Token>::parseLazy();
        }

        struct NoMatch {};
        struct RuleProbe {
            LLParser& parser;
            RuleProbe* previous;
            std::size_t start;
            bool committed = false;
            RuleProbe(LLParser& parser, std::size_t start)
                : parser(parser), previous(parser.probe_), start(start) { parser.probe_ = this; }
            ~RuleProbe() { parser.probe_ = previous; }
            RuleProbe(const RuleProbe&) = delete;
            RuleProbe& operator=(const RuleProbe&) = delete;
        };
        RuleProbe* probe_ = nullptr;

        template<typename IT>
        static auto cursor_position(const IT& pos) -> std::size_t {
            if constexpr (detail::detected_v<detail::distance, IT>) return pos.distance();
            else return reinterpret_cast<std::size_t>(std::addressof(*pos));
        }
        template<typename IT>
        static std::string debug_actual(const IT& pos) {
            if (!ispa::debug::enabled()) return {};
            if (pos.isEnd()) return "<end of input>";
            std::string actual = "token #" + std::to_string(Recovery::tokenId(*pos));
            if constexpr (detail::detected_v<detail::meta_startpos_length, Token>) {
                const auto& meta = pos->meta();
                if (meta.start() && meta.length())
                    actual += " '" + std::string(meta.start(), std::min<std::size_t>(meta.length(), 80)) + "'";
            }
            return actual;
        }
        static const char* debug_recovery_mode(Recovery::Mode mode) {
            switch (mode) {
            case Recovery::Mode::PanicUntil: return "panic until synchronization";
            case Recovery::Mode::Insert: return "insert";
            case Recovery::Mode::Delete: return "delete and retry";
            case Recovery::Mode::Replace: return "replace";
            case Recovery::Mode::Retry: return "retry";
            case Recovery::Mode::Synchronize: return "synchronize";
            case Recovery::Mode::Propagate: return "propagate to parent";
            case Recovery::Mode::Abort: return "abort";
            }
            return "unknown";
        }

        // ----- frames -----
        template<typename Set>
        static bool claim_matches(const void* tok) {
            return Set::matches(*static_cast<const Token*>(tok));
        }

        struct FrameGuard {
            LLParser& p;
            FrameGuard(LLParser& parser, std::string_view rule) : p(parser) {
                p.frames_.push_back(Frame{rule, {}, {}});
                p.recovery_state_.anchors.emplace_back();
            }
            // Abandoned frames count down the unwind request; the frame that
            // is the resume target is the first one that sees it reach zero.
            ~FrameGuard() {
                p.frames_.pop_back();
                p.recovery_state_.anchors.pop_back();
                if (p.unwind_) --p.unwind_;
            }
            FrameGuard(const FrameGuard&) = delete;
            FrameGuard& operator=(const FrameGuard&) = delete;
        };
        [[nodiscard]] FrameGuard recovery_frame(std::string_view rule) {
            return FrameGuard(*this, rule);
        }

        // Emitted before each child member of the current rule.
        template<typename Set>
        static void append_claim_values(std::vector<Recovery::TokenId>& anchors) {
            const auto values = Set::values();
            anchors.insert(anchors.end(), values.begin(), values.end());
        }

        template<typename... Sets, typename... Names>
        void recovery_at(std::string_view expecting, Names... names) {
            static_assert(sizeof...(Sets) == sizeof...(Names),
                          "recovery_at: one claim name per claim set");
            if (frames_.empty()) return;
            auto& frame = frames_.back();
            frame.expecting = expecting;
            frame.claims.clear();
            auto& anchors = recovery_state_.anchors.back();
            anchors.clear();
            (append_claim_values<Sets>(anchors), ...);
            const std::string_view n[] = {std::string_view(names)..., std::string_view{}};
            std::size_t i = 0;
            (frame.claims.push_back({n[i++], &LLParser::claim_matches<Sets>}), ...);
            (void)i; (void)n;
        }

        [[nodiscard]] bool unwinding() const noexcept { return unwind_ != 0; }

        template<typename IT>
        static std::string describe_token(const IT& pos) {
            if (pos.isEnd()) return "end of input";
            [[maybe_unused]] const auto& tok = *pos;
            if constexpr (detail::detected_v<detail::meta_start_length, decltype(tok)>) {
                if (tok.meta().start())
                    return "'" + std::string(std::string_view(tok.meta().start(),
                                                              tok.meta().length())) + "'";
            }
            return "<token>";
        }

        [[nodiscard]] std::string breadcrumb() const {
            std::string out;
            for (const auto& f : frames_) {
                if (!out.empty()) out += " > ";
                out += f.rule;
            }
            return out;
        }

        struct Resume {
            std::size_t skip;
            std::size_t frame;
            const Recovery::ContextClaim* claim;   // null when resuming at end of input
        };

        // Earliest token (within max_skip) claimed by any frame; ties go to the
        // innermost frame. If an identical spot already failed, the innermost frame
        // is not allowed to claim the current token again (guarantees progress).
        template<typename IT>
        std::optional<Resume> resolve_context(IT pos, std::size_t max_skip) const {
            const bool repeat_spot = cursor_position(pos) == last_resume_pos_;
            std::size_t k = 0;
            for (; k <= max_skip && !pos.isEnd(); ++k, ++pos) {
                const void* tok = std::addressof(*pos);
                for (std::size_t f = frames_.size(); f-- > 0;) {
                    if (k == 0 && repeat_spot && f + 1 == frames_.size()) continue;
                    for (const auto& c : frames_[f].claims)
                        if (c.matches && c.matches(tok))
                            return Resume{k, f, &c};
                }
            }
            if (pos.isEnd() && !frames_.empty())
                return Resume{k, 0, nullptr};
            return std::nullopt;
        }

        template<typename IT>
        void report_context(const IT& pos, Recovery::ContextKind kind, const Resume& r) {
            const auto& inner = frames_.back();
            const auto& target = frames_[r.frame];
            Recovery::ContextDiagnostic d;
            d.kind = kind;
            d.position = cursor_position(pos);
            if constexpr (detail::detected_v<detail::source_origin_position, IT>) {
                if (const auto* at = pos.sourcePosition()) {
                    d.offset = at - pos.sourceOrigin();
                    d.length = *at ? 1 : 0;
                }
            } else if (!pos.isEnd()) {
                if constexpr (detail::detected_v<detail::meta_startpos_length, decltype(*pos)>) {
                    d.offset = pos->meta().startpos();
                    d.length = pos->meta().length();
                }
            }
            if (d.offset == std::numeric_limits<std::size_t>::max() && pos.isEnd() && this->text)
                d.offset = std::char_traits<char>::length(this->text);
            d.skipped = r.skip;
            d.rule = std::string(inner.rule);
            d.resume_rule = std::string(target.rule);
            d.claim = r.claim ? std::string(r.claim->name) : std::string{};

            const std::string tok = describe_token(pos);
            const std::string where = breadcrumb();
            const std::string expected = inner.expecting.empty()
                ? std::string{} : "; expected " + std::string(inner.expecting);

            switch (kind) {
            case Recovery::ContextKind::Misplaced:
                d.message = "unexpected " + tok + " while parsing '" + d.rule +
                            "'" + expected + "; continuing at '" + d.claim +
                            "' in enclosing rule '" + d.resume_rule + "'";
                break;
            case Recovery::ContextKind::Stray:
                d.message = "unexpected " + tok + " in " + where +
                            "; no enclosing rule accepts it here; skipped " +
                            std::to_string(r.skip) + " token(s)" +
                            (r.claim ? "; resuming at '" + d.claim + "' of '" + d.resume_rule + "'"
                                     : std::string("; reached end of input"));
                break;
            case Recovery::ContextKind::Mismatch:
            default:
                d.message = "unexpected " + tok + " in " + where + expected;
                if (r.skip)
                    d.message += "; skipped " + std::to_string(r.skip) + " token(s)";
                d.message += r.claim
                    ? "; resuming at '" + d.claim + "' of '" + d.resume_rule + "'"
                    : std::string("; reached end of input");
                break;
            }
            context_errors_.push_back(std::move(d));
        }

        // Local repair already failed (or does not apply). Find a recovery point
        // in the enclosing frames, skip to it, and request unwinding.
        template<typename IT>
        bool recover_at_context(IT& pos, Recovery::ContextKind kind) {
            if (frames_.empty() || recovery_state_.exhausted()) return false;
            const auto remaining = recovery_state_.max_total_skipped - recovery_state_.total_skipped;
            auto r = resolve_context(pos, std::min(context_max_skip_, remaining));
            if (!r) return false;
            const bool innermost = r->frame + 1 == frames_.size();
            if (!innermost && r->skip == 0 && r->claim)
                kind = Recovery::ContextKind::Misplaced;
            if (!recovering_ || kind == Recovery::ContextKind::Misplaced)
                report_context(pos, kind, *r);
            for (std::size_t i = 0; i < r->skip && !pos.isEnd(); ++i) ++pos;
            recovery_state_.total_skipped += r->skip;
            last_resume_pos_ = cursor_position(pos);
            unwind_ = frames_.size() - 1 - r->frame;
            recovering_ = true;
            return true;
        }

        // A loop ended on a token that no active frame can accept: report here,
        // where the mistake is, rather than at the next required member.
        template<typename SpaceToken = void, typename IT>
        void sync_loop(IT& pos) {
            if (pos.isEnd() || frames_.empty() || unwinding()) return;
            auto candidate = pos;
            if constexpr (!std::is_void_v<SpaceToken>) skip_spaces<SpaceToken>(candidate);
            if (candidate.isEnd()) return;
            // External FIRST sets are predicates over source text. A boundary
            // token cannot be classified by the host recovery token sets.
            if constexpr (detail::detected_v<detail::is_ambiguous, decltype(*candidate)>) {
                if (candidate->isAmbiguous()) return;
            }
            const void* tok = std::addressof(*candidate);
            bool any_claims = false;
            for (const auto& f : frames_) {
                for (const auto& c : f.claims) {
                    any_claims = true;
                    if (c.matches && c.matches(tok)) return;   // someone owns it
                }
            }
            if (!any_claims) return;
            pos = candidate;
            recover_at_context(pos, Recovery::ContextKind::Stray);
        }

        // Called only after normal LL(k) selection failed. Prefer an
        // unambiguous FIRST token, then an unambiguous terminal inside an
        // alternative. Shared interior terminals do not identify a branch.
        template<class... Sets, class IT>
        int recover_choice(IT& pos, std::string_view rule, std::string_view expected) {
            reject_absent_rule();
            const std::vector<std::vector<Recovery::TokenId>> sets{Sets::values()...};
            static_assert(sizeof...(Sets) % 2 == 0);
            Recovery::Diagnostic diagnostic;
            diagnostic.rule = std::string(rule);
            diagnostic.expected_text = std::string(expected);
            diagnostic.message = "unexpected " + describe_token(pos) + " in " +
                std::string(rule) + "; expected one of: " + std::string(expected);
            if (!pos.isEnd()) {
                diagnostic.unexpected = Recovery::tokenId(*pos);
                if constexpr (detail::detected_v<detail::meta_startpos_length, decltype(*pos)>) {
                    diagnostic.offset = pos->meta().startpos();
                    diagnostic.length = pos->meta().length();
                }
            }
            for (std::size_t i = 0; i < sets.size(); i += 2)
                diagnostic.expected.insert(diagnostic.expected.end(), sets[i].begin(), sets[i].end());
            int selected = -1;
            std::size_t skipped = 0;
            if (!recovery_state_.exhausted()) {
                while (!pos.isEnd()) {
                    const auto id = Recovery::tokenId(*pos);
                    for (std::size_t phase = 0; phase < 2 && selected == -1; ++phase) {
                        int candidate = -1;
                        bool ambiguous = false;
                        for (std::size_t i = phase; i < sets.size(); i += 2) {
                            if (!Recovery::contains(sets[i], id)) continue;
                            if (candidate != -1) ambiguous = true;
                            candidate = static_cast<int>(i / 2);
                        }
                        if (ambiguous) break;
                        selected = candidate;
                    }
                    if (selected != -1) break;
                    bool boundary = false;
                    for (const auto& anchors : recovery_state_.anchors)
                        if (Recovery::contains(anchors, id)) boundary = true;
                    if (boundary || skipped >= context_max_skip_ ||
                        recovery_state_.total_skipped + skipped >= recovery_state_.max_total_skipped)
                        break;
                    ++pos;
                    ++skipped;
                }
            }
            recovery_state_.total_skipped += skipped;
            diagnostic.decision.skipped = skipped;
            diagnostic.decision.mode = selected == -1
                ? Recovery::Mode::Propagate : Recovery::Mode::Synchronize;
            diagnostic.message += selected == -1
                ? "; no unambiguous alternative at recovery boundary"
                : "; continuing in alternative " + std::to_string(selected + 1);
            if (skipped) diagnostic.message += "; skipped " + std::to_string(skipped) + " tokens";
            recovery_state_.diagnostics.push_back(std::move(diagnostic));
            if (recovery_state_.exhausted()) {
                recovery_state_.diagnostics.back().decision.mode = Recovery::Mode::Abort;
                recovery_state_.diagnostics.back().message += "; recovery limit reached";
                return -1;
            }
            return selected;
        }

        void reject_absent_rule() {
            if (probe_ && !probe_->committed)
                throw NoMatch{};
        }
        template<typename IT>
        void commit_rule(const IT& pos) {
            for (auto* probe = probe_; probe; probe = probe->previous)
                if (cursor_position(pos) != probe->start) probe->committed = true;
        }
        void note_match() noexcept { recovering_ = false; }

        // Use these only at committed required matches; unlike the legacy
        // required() they return an explicit Missing/Boundary result on repair.
        template<typename NodeStruct, typename IT, typename Fail>
        auto required_recovering(IT& pos, Fail&& fail) {
            using NodeT = decltype(pos->template to_node<NodeStruct>());
            using Result = Recovery::RequiredResult<NodeT>;
            if (unwinding()) return Result{};
            std::size_t retries = 0;
            while (true) {
                if (pos->template holds<NodeStruct>()) {
                    auto node = pos->template to_node<NodeStruct>();
                    ++pos;
                    commit_rule(pos);
                    note_match();
                    return Result{Recovery::RequiredStatus::Matched,
                                  std::move(node), {}};
                }
                reject_absent_rule();
                auto decision = fail(pos, recovery_state_);
                switch (decision.mode) {
                case Recovery::Mode::Delete:
                    if (++retries <= 2) continue;
                    break;
                case Recovery::Mode::Retry:
                    if (++retries <= 1) continue;
                    break;
                case Recovery::Mode::Insert:
                case Recovery::Mode::Replace:
                    return Result{Recovery::RequiredStatus::Missing,
                                  std::nullopt, decision};
                case Recovery::Mode::Abort:
                    return Result{Recovery::RequiredStatus::Aborted,
                                  std::nullopt, decision};
                default:
                    break;
                }
                // Local repair did not work: look for a recovery point in the
                // enclosing frames (may skip tokens and request unwinding).
                if (decision.mode == Recovery::Mode::Propagate)
                    recover_at_context(pos, Recovery::ContextKind::Mismatch);
                return Result{Recovery::RequiredStatus::Boundary,
                              std::nullopt, decision};
            }
        }

        template<typename IT, typename Recursive, typename Fail>
        auto required_rule_recovering(IT& pos, Recursive&& rec, Fail&& fail) {
            using NodeT = std::decay_t<decltype(rec(pos).node)>;
            using Result = Recovery::RequiredResult<NodeT>;
            if (unwinding()) return Result{};
            std::size_t retries = 0;
            while (true) {
                // Recursive alternatives may advance their input argument
                // before failing. Never let a failed probe mutate pos.
                IT attempt = pos;
                auto match = [&] {
                    if constexpr (detail::detected_v<detail::custom_action, std::decay_t<Fail>>) {
                        decltype(rec(attempt)) result{};
                        RuleProbe probe(*this, cursor_position(attempt));
                        try { result = rec(attempt); }
                        catch (const NoMatch&) { /* invoke the explicitly bound parent handler */ }
                        catch (const ExternalParseError&) { /* parent owns foreign syntax recovery */ }
                        return result;
                    } else return rec(attempt);
                }();
                if (match.status) {
                    pos = match.it;
                    commit_rule(pos);
                    note_match();
                    return Result{Recovery::RequiredStatus::Matched,
                                  std::move(match.node), {}};
                }
                if (unwinding()) {
                    pos = match.it;
                    // The child abandoned itself for an outer frame; do not
                    // repair here, just propagate.
                    return Result{Recovery::RequiredStatus::Boundary,
                                  std::nullopt, {}};
                }
                reject_absent_rule();
                auto decision = fail(pos, recovery_state_);
                // We cannot synthesize an arbitrary nonterminal from one token.
                if ((decision.mode == Recovery::Mode::Retry ||
                     decision.mode == Recovery::Mode::Delete) && ++retries <= 2)
                    continue;
                if (decision.mode == Recovery::Mode::Propagate)
                    recover_at_context(pos, Recovery::ContextKind::Mismatch);
                return Result{
                    decision.mode == Recovery::Mode::Abort
                        ? Recovery::RequiredStatus::Aborted
                        : (decision.mode == Recovery::Mode::Insert || decision.mode == Recovery::Mode::Replace)
                            ? Recovery::RequiredStatus::Missing : Recovery::RequiredStatus::Boundary,
                    std::nullopt, decision
                };
            }
        }

        // skip spaces for tokens
        template <typename SpaceTokenType, typename IT>
        void skip_spaces(IT& pos) {
            if ((*pos).template holds<SpaceTokenType>())
                ++pos;
        }
        template <typename SpaceTokenType, typename IT>
        void skip_external_spaces(IT& pos) {
            if constexpr (detail::detected_v<detail::consume_external, IT>) {
                const char* begin = pos.sourcePosition();
                const char* end = begin;
                while (end && *end && std::isspace(static_cast<unsigned char>(*end))) ++end;
                if (end != begin) pos.consumeExternal(end);
            } else skip_spaces<SpaceTokenType>(pos);
        }
        void raise_error(const Token& node, const std::string& message) {
            reject_absent_rule();
            Recovery::Diagnostic diagnostic;
            diagnostic.rule = frames_.empty() ? "<rule>" : std::string(frames_.back().rule);
            diagnostic.message = message;
            if constexpr (detail::detected_v<detail::meta_startpos_length, Token>) {
                diagnostic.offset = node.meta().startpos();
                diagnostic.length = node.meta().length();
            }
            recovery_state_.diagnostics.push_back(std::move(diagnostic));

        }
        template<typename NodeStruct, typename IT, typename FailFunc>
        auto required(IT &pos, FailFunc&& fail) {
            ispa::debug::MatchScope debug_match(ispa::debug::display_type<NodeStruct>(), debug_actual(pos));
            if constexpr (std::is_invocable_v<FailFunc&, IT&, Recovery::State&>) {
                auto result = required_recovering<NodeStruct>(pos, fail);
                if (result.node) { debug_match.success(); return std::move(*result.node); }
                if (result.status != Recovery::RequiredStatus::Matched)
                    debug_match.outcome(std::string("recovery: ") + debug_recovery_mode(result.recovery.mode) +
                        ", skipped " + std::to_string(result.recovery.skipped));
                if (result.status != Recovery::RequiredStatus::Aborted &&
                    (result.status == Recovery::RequiredStatus::Missing ||
                     unwinding() ||
                     (!pos.isEnd() && result.recovery.mode != Recovery::Mode::Propagate)))
                    return std::decay_t<decltype(*result.node)>{};
            } else {
                if (pos->template holds<NodeStruct>()) {
                    auto node = pos->template to_node<NodeStruct>();
                    ++pos;
                    commit_rule(pos);
                    debug_match.success();
                    return node;
                }
                reject_absent_rule();
                fail(pos);
            }
            ispa::debug::breakpoint("required token failed");
            throw std::logic_error(recovery_state_.diagnostics.empty()
                ? "ISPA required(): required token could not be recovered"
                : recovery_state_.diagnostics.back().message);
        }
        // All rule combinators take (pos, rec, [fail]), matching LL lowering.
        template<typename NodeStruct, typename IT, typename Recursive, typename FailFunc>
        auto required(IT &pos, Recursive&& rec, FailFunc&& fail) {
            ispa::debug::MatchScope debug_match(ispa::debug::display_type<NodeStruct>(), debug_actual(pos));
            if constexpr (std::is_invocable_v<FailFunc&, IT&, Recovery::State&>) {
                auto result = required_rule_recovering(pos, rec, fail);
                if (result.node) { debug_match.success(); return std::move(*result.node); }
                if (result.status != Recovery::RequiredStatus::Matched)
                    debug_match.outcome(std::string("recovery: ") + debug_recovery_mode(result.recovery.mode) +
                        ", skipped " + std::to_string(result.recovery.skipped));
                if (result.status != Recovery::RequiredStatus::Aborted &&
                    (result.status == Recovery::RequiredStatus::Missing || unwinding() ||
                     (!pos.isEnd() && result.recovery.mode != Recovery::Mode::Propagate)))
                    return std::decay_t<decltype(*result.node)>{};
            } else {
                auto match = rec(pos);
                if (match.status) {
                    pos = match.it;
                    commit_rule(pos);
                    debug_match.success();
                    return std::move(match.node);
                }
                reject_absent_rule();
                fail(pos);
            }
            ispa::debug::breakpoint("required rule failed");
            throw std::logic_error(recovery_state_.diagnostics.empty()
                ? "ISPA required(): required rule could not be recovered"
                : recovery_state_.diagnostics.back().message);
        }
        template<typename NodeStruct, typename IT>
        auto optional(IT &pos) {
            ispa::debug::MatchScope debug_match(ispa::debug::display_type<NodeStruct>(), debug_actual(pos));
            if (pos->template holds<NodeStruct>()) {
                auto node = pos->template to_node<NodeStruct>();
                ++pos;
                commit_rule(pos);
                debug_match.success();
                return node;
            }
            return Node<NodeStruct>{};
        }
        // NOTE: a rule that abandons itself during unwinding must still return
        // status == true (a partial match) or the committed-probe check below
        // will throw.
        template<typename NodeStruct, typename IT, typename Recursive>
        auto optional(IT &pos, Recursive&& rec) {
            auto attempt = pos;
            RuleProbe probe(*this, cursor_position(pos));
            try {
                if (auto match = rec(attempt); match.status) {
                    pos = match.it;
                    commit_rule(pos);
                    return std::move(match.node);
                }
                if (probe.committed)
                    throw std::logic_error("ISPA optional(): rule failed after consuming input");
            } catch (const NoMatch&) {
                // A required descendant did not start. Only the optional
                // caller handles this signal; recovery has not run.
            }
            return Node<NodeStruct>{};
        }
        template<class NodeStruct, class SpaceToken, class IT, class Match>
        auto counted_impl(IT& pos, CountSpec spec, Match&& match) {
            auto limit = spec.count;
            if (!spec.name.empty()) {
                if (auto found = counters_.find(spec.name); found != counters_.end())
                    limit = found->second;
            }
            std::vector<Node<NodeStruct>> nodes;
            auto cursor = pos;
            while (!limit || nodes.size() < *limit) {
                auto attempt = cursor;
                if constexpr (!std::is_void_v<SpaceToken>) {
                    if (!nodes.empty()) skip_spaces<SpaceToken>(attempt);
                }
                const auto before = cursor_position(attempt);
                auto node = match(attempt);
                if (node.empty()) break;
                if (!limit && cursor_position(attempt) == before)
                    throw std::logic_error("Unbound counted repetition matched without consuming input");
                nodes.push_back(std::move(node));
                cursor = attempt;
            }
            if ((limit && nodes.size() != *limit) || nodes.size() < spec.minimum) {
                reject_absent_rule();
                Recovery::Diagnostic diagnostic;
                diagnostic.rule = frames_.empty() ? "<rule>" : std::string(frames_.back().rule);
                diagnostic.message = "Counted repetition" +
                    (spec.name.empty() ? std::string{} : " '" + spec.name + "'") +
                    " expected " + std::to_string(std::max(limit.value_or(0), spec.minimum)) +
                    " matches, got " + std::to_string(nodes.size());
                if (!cursor.isEnd()) {
                    diagnostic.unexpected = Recovery::tokenId(*cursor);
                    if constexpr (detail::detected_v<detail::meta_startpos, decltype(*cursor)>)
                        diagnostic.offset = cursor->meta().startpos();
                }
                const auto message = diagnostic.message;
                recovery_state_.diagnostics.push_back(std::move(diagnostic));
                throw std::logic_error(message);
            }
            if (!spec.name.empty()) counters_.try_emplace(spec.name, nodes.size());
            pos = cursor;
            commit_rule(pos);
            return nodes;
        }

        template<class NodeStruct, class SpaceToken = void, class IT>
        auto counted(IT& pos, CountSpec spec) {
            return counted_impl<NodeStruct, SpaceToken>(pos, std::move(spec),
                [&](IT& attempt) { return optional<NodeStruct>(attempt); });
        }
        template<class NodeStruct, class SpaceToken = void, class IT, class Recursive>
        auto counted(IT& pos, CountSpec spec, Recursive&& rec) {
            return counted_impl<NodeStruct, SpaceToken>(pos, std::move(spec),
                [&](IT& attempt) { return optional<NodeStruct>(attempt, rec); });
        }

        // Separators belong to a successful next iteration, not to the end
        // of the list. Leave pos (including whitespace) intact on absence.
        template<typename SpaceToken, typename IT, typename Match>
        auto repeat_optional(IT& pos, Match&& match, bool separated = true) {
            auto attempt = pos;
            if constexpr (!std::is_void_v<SpaceToken>) {
                if (separated) skip_spaces<SpaceToken>(attempt);
            }
            const auto start = cursor_position(attempt);
            auto node = match(attempt);
            if (node.empty() || cursor_position(attempt) == start)
                return decltype(node){};
            pos = attempt;
            return node;
        }

        template<typename NodeStruct, typename SpaceToken = void, typename IT, typename FailFunc>
        auto range(IT &pos, FailFunc&& fail) {
            std::vector<Node<NodeStruct>> result;
            if constexpr (std::is_invocable_v<FailFunc&, IT&, Recovery::State&>) {
                auto first = required_recovering<NodeStruct>(pos, fail);
                if (!first.node) return result;   // repaired/unwound: nothing consumed
                result.push_back(std::move(*first.node));
            } else {
                result.push_back(required<NodeStruct>(pos, fail));
            }
            if (unwinding()) return result;
            while (!pos.isEnd() && !unwinding()) {
                auto node = repeat_optional<SpaceToken>(pos, [&](auto& attempt) {
                    return optional<NodeStruct>(attempt);
                });
                if (node.empty()) break;
                result.push_back(std::move(node));
            }
            sync_loop<SpaceToken>(pos);
            return result;
        }
        template<typename NodeStruct, typename SpaceToken = void, typename IT, typename Recursive, typename FailFunc>
        auto range(IT &pos, Recursive&& rec, FailFunc&& fail) {
            const auto start = cursor_position(pos);
            std::vector<Node<NodeStruct>> result;
            if constexpr (std::is_invocable_v<FailFunc&, IT&, Recovery::State&>) {
                auto first = required_rule_recovering(pos, rec, fail);
                if (!first.node) return result;   // repaired/unwound: nothing consumed
                result.push_back(std::move(*first.node));
            } else {
                result.push_back(required<NodeStruct>(pos, rec, fail));
            }
            if (cursor_position(pos) == start || unwinding()) return result;
            while (!pos.isEnd() && !unwinding()) {
                auto node = repeat_optional<SpaceToken>(pos, [&](auto& attempt) {
                    return optional<NodeStruct>(attempt, rec);
                });
                if (node.empty()) break;
                result.push_back(std::move(node));
            }
            sync_loop<SpaceToken>(pos);
            return result;
        }
        template<typename NodeStruct, typename SpaceToken = void, typename IT>
        auto range_optional(IT &pos) {
            std::vector<Node<NodeStruct>> result;
            while (!pos.isEnd() && !unwinding()) {
                auto node = repeat_optional<SpaceToken>(pos, [&](auto& attempt) {
                    return optional<NodeStruct>(attempt);
                }, !result.empty());
                if (node.empty()) break;
                result.push_back(std::move(node));
            }
            sync_loop<SpaceToken>(pos);
            return result;
        }
        template<typename NodeStruct, typename SpaceToken = void, typename IT, typename Recursive>
        auto range_optional(IT &pos, Recursive&& rec) {
            std::vector<Node<NodeStruct>> result;
            while (!pos.isEnd() && !unwinding()) {
                auto node = repeat_optional<SpaceToken>(pos, [&](auto& attempt) {
                    return optional<NodeStruct>(attempt, rec);
                }, !result.empty());
                if (node.empty()) break;
                result.push_back(std::move(node));
            }
            sync_loop<SpaceToken>(pos);
            return result;
        }
    };
} // namespace ISPA_STD
