// LibraryDeps.hpp - compile-time view of ispastdlib_deps.json and ispaext_deps.json
#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#ifndef ISPA_DEPS_INC
#define ISPA_DEPS_INC "ispastdlib_deps.inc"
#endif
#ifndef ISPAEXT_DEPS_INC
#define ISPAEXT_DEPS_INC "ispaext_deps.inc"
#endif

namespace Cpp::LibraryDeps {
    namespace detail {
        using sv = std::string_view;

        inline constexpr sv json =
#include ISPA_DEPS_INC
            ;
        inline constexpr sv ext_json =
#include ISPAEXT_DEPS_INC

        ;

        struct Range {
            std::size_t begin = 0;
            std::size_t count = 0;
        };
        struct FileInfo {
            sv path;
            Range includes;      // direct project includes
            Range closure;       // transitive project includes
            Range depends_on;    // files whose symbols this file names
        };
        struct SymbolInfo {
            sv name;
            sv kind;
            std::size_t file = 0;
        };

        template<std::size_t NF, std::size_t NS, std::size_t NP>
        struct Tables {
            std::array<FileInfo, NF> files{};
            std::array<SymbolInfo, NS> symbols{};
            std::array<sv, NP> pool{};          // every string of every list, ranges index into it
            std::size_t file_count = 0, symbol_count = 0, pool_count = 0;
            sv default_namespace;
            sv umbrella;
        };

        // Minimal JSON reader for the schema ispa_deps.py writes.
        struct Reader {
            sv s;
            std::size_t i = 0;

            constexpr void fail(const char *what) const { if (what) throw "LibraryDeps: malformed JSON"; }
            constexpr void ws() { while (i < s.size() && (s[i] == ' ' || s[i] == '\n' || s[i] == '\r' || s[i] == '\t')) ++i; }
            constexpr bool peek(char c) { ws(); return i < s.size() && s[i] == c; }
            constexpr bool eat(char c) { if (peek(c)) { ++i; return true; } return false; }
            constexpr void expect(char c) { 
                if (!eat(c)) {
                    throw "LibraryDeps: malformed JSON (unexpected character)";
                }
            }
            constexpr sv string() {
                expect('"');
                const std::size_t b = i;
                while (i < s.size() && s[i] != '"') {
                    if (s[i] == '\\') fail("escape sequences are not supported");
                    ++i;
                }
                const sv out = s.substr(b, i - b);
                expect('"');
                return out;
            }
            constexpr std::size_t integer() {
                ws();
                std::size_t v = 0;
                while (i < s.size() && s[i] >= '0' && s[i] <= '9') v = v * 10 + static_cast<std::size_t>(s[i++] - '0');
                return v;
            }
            constexpr void skip_value() {
                ws();
                if (peek('"')) { string(); return; }
                if (peek('{') || peek('[')) {
                    int depth = 0;
                    do {
                        ws();
                        if (s[i] == '"') { string(); continue; }
                        if (s[i] == '{' || s[i] == '[') ++depth;
                        else if (s[i] == '}' || s[i] == ']') --depth;
                        ++i;
                    } while (depth > 0 && i < s.size());
                    return;
                }
                while (i < s.size() && s[i] != ',' && s[i] != '}' && s[i] != ']') ++i;
            }
        };

        // Reads a JSON snippet into the provided table object (supports incremental accumulation)
        template<std::size_t NF, std::size_t NS, std::size_t NP>
        constexpr void parse_into(sv text, Tables<NF, NS, NP>& t) {
            Reader r{text};

            const auto push = [&](sv value) {
                if (t.pool_count < NP) t.pool[t.pool_count] = value;
                ++t.pool_count;
            };
            const auto string_list = [&]() {
                Range range{t.pool_count, 0};
                r.expect('[');
                if (!r.peek(']')) {
                    do { push(r.string()); ++range.count; } while (r.eat(','));
                }
                r.expect(']');
                return range;
            };
            const auto file_index = [&](sv path) {
                for (std::size_t k = 0; k < t.file_count && k < NF; ++k)
                    if (t.files[k].path == path) return k;
                return std::size_t(-1);
            };

            r.expect('{');
            if (!r.peek('}')) {
                do {
                    const sv key = r.string();
                    r.expect(':');
                    if (key == "default_namespace") {
                        sv ns = r.string();
                        if (t.default_namespace.empty()) t.default_namespace = ns;
                    } else if (key == "umbrella") {
                        sv umb = r.string();
                        if (t.umbrella.empty()) t.umbrella = umb;
                    } else if (key == "files") {
                        r.expect('[');
                        if (!r.peek(']')) {
                            do {
                                FileInfo f{};
                                r.expect('{');
                                do {
                                    const sv k = r.string();
                                    r.expect(':');
                                    if (k == "path") f.path = r.string();
                                    else if (k == "includes") f.includes = string_list();
                                    else if (k == "closure") f.closure = string_list();
                                    else if (k == "depends_on") f.depends_on = string_list();
                                    else if (k == "system_includes" || k == "uses" || k == "defines") (void)string_list();
                                    else r.skip_value();
                                } while (r.eat(','));
                                r.expect('}');
                                if (t.file_count < NF) t.files[t.file_count] = f;
                                ++t.file_count;
                            } while (r.eat(','));
                        }
                        r.expect(']');
                    } else if (key == "symbols") {
                        r.expect('[');
                        if (!r.peek(']')) {
                            do {
                                SymbolInfo sym{};
                                sv file;
                                r.expect('{');
                                do {
                                    const sv k = r.string();
                                    r.expect(':');
                                    if (k == "name") sym.name = r.string();
                                    else if (k == "kind") sym.kind = r.string();
                                    else if (k == "file") file = r.string();
                                    else r.skip_value();
                                } while (r.eat(','));
                                r.expect('}');
                                const std::size_t idx = file_index(file);
                                if (idx == std::size_t(-1) && NF > 1) throw "LibraryDeps: symbol refers to an unknown file";
                                sym.file = idx;
                                if (t.symbol_count < NS) t.symbols[t.symbol_count] = sym;
                                ++t.symbol_count;
                            } while (r.eat(','));
                        }
                        r.expect(']');
                    } else if (key == "version") {
                        if (r.integer() != 1) throw "LibraryDeps: unsupported schema version";
                    } else r.skip_value();
                } while (r.eat(','));
            }
            r.expect('}');
        }

        template<std::size_t NF, std::size_t NS, std::size_t NP>
        constexpr Tables<NF, NS, NP> parse_both(sv main_text, sv ext_text) {
            Tables<NF, NS, NP> t{};
            parse_into(main_text, t);
            parse_into(ext_text, t);
            return t;
        }

        inline constexpr auto sizes = parse_both<1, 1, 1>(json, ext_json);                             // pass 1: calculate combined counts
        inline constexpr auto tables = parse_both<sizes.file_count, sizes.symbol_count, sizes.pool_count>(json, ext_json); // pass 2: populate merged tables

        constexpr sv pooled(Range r, std::size_t k) { return tables.pool[r.begin + k]; }
    }   // namespace detail

    // ---- queries -----------------------------------------------------------------------------------------------
    inline constexpr std::string_view default_namespace = detail::tables.default_namespace;
    inline constexpr std::string_view umbrella = detail::tables.umbrella;
    inline constexpr std::size_t file_count = detail::tables.file_count;
    inline constexpr std::size_t symbol_count = detail::tables.symbol_count;

    constexpr std::optional<std::size_t> find_file(std::string_view path) {
        for (std::size_t k = 0; k < file_count; ++k)
            if (detail::tables.files[k].path == path) return k;
        return std::nullopt;
    }
    constexpr bool has_file(std::string_view path) { return find_file(path).has_value(); }

    constexpr std::optional<std::size_t> find_symbol(std::string_view qualified) {
        for (std::size_t k = 0; k < symbol_count; ++k)
            if (detail::tables.symbols[k].name == qualified) return k;
        return std::nullopt;
    }

    // Header that defines the (possibly qualified) name `chain`, e.g. {"ISPA_STD","Node"} or {"DFA","run"}.
    // The chain is tried as written, then inside the library's default namespace.
    inline std::optional<std::string_view> header_of(const std::vector<std::string> &chain) {
        if (chain.empty()) return std::nullopt;
        std::string name;
        for (const auto &c : chain) name += (name.empty() ? "" : "::") + c;
        auto k = find_symbol(name);
        if (!k) k = find_symbol(std::string(default_namespace) + "::" + name);
        if (!k) return std::nullopt;
        return detail::tables.files[detail::tables.symbols[*k].file].path;
    }

    // Is `chain` rooted in the library at all (`ISPA_STD::<anything>`)? Used to tell "unknown library name" from "not ours".
    inline bool in_library_namespace(const std::vector<std::string> &chain) {
        return !chain.empty() && chain.front() == default_namespace;
    }

    constexpr std::vector<std::string_view> includes_of(std::string_view path) {
        std::vector<std::string_view> out;
        if (const auto f = find_file(path))
            for (std::size_t k = 0; k < detail::tables.files[*f].includes.count; ++k)
                out.push_back(detail::pooled(detail::tables.files[*f].includes, k));
        return out;
    }
    constexpr bool pulls_in(std::string_view path, std::string_view other) {   // `other` in the transitive includes of `path`
        if (const auto f = find_file(path)) {
            const auto &r = detail::tables.files[*f].closure;
            for (std::size_t k = 0; k < r.count; ++k)
                if (detail::pooled(r, k) == other) return true;
        }
        return false;
    }

    // Smallest include list that still provides every requested header: duplicates and headers that another
    // requested header already includes (directly or not) are dropped. Result is ordered so that a header comes after
    // everything it depends on among the survivors (fewest dependencies first, ties by first request).
    inline std::vector<std::string_view> minimal_includes(const std::vector<std::string_view> &wanted) {
        std::vector<std::string_view> uniq;
        for (const auto w : wanted)
            if (std::find(uniq.begin(), uniq.end(), w) == uniq.end()) uniq.push_back(w);
        std::vector<std::string_view> out;
        for (const auto h : uniq) {
            const bool covered = std::any_of(uniq.begin(), uniq.end(),
                [&](std::string_view g) { return g != h && pulls_in(g, h); });
            if (!covered) out.push_back(h);
        }
        const auto deps = [](std::string_view p) {
            const auto f = find_file(p);
            return f ? detail::tables.files[*f].closure.count : std::size_t{0};
        };
        std::stable_sort(out.begin(), out.end(), [&](std::string_view a, std::string_view b) { return deps(a) < deps(b); });
        return out;
    }
}   // namespace Cpp::LibraryDeps
