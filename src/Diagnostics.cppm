module;
#include <cstdio>
#include <cstdlib>
#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

export module ISPA.Diagnostics;
import std;

export namespace ISPA::Diagnostics {
    enum class Severity { Error, Warning, Info };
    enum class ColorPolicy { Auto, Always, Never };

    struct Location {
        std::string file;
        std::size_t offset = std::string::npos;
        std::size_t length = 0;
        std::size_t line = 0;     // 1 based
        std::size_t column = 0;   // 1 based, byte columns
    };

    struct Diagnostic {
        Severity severity = Severity::Error;
        std::string code;
        std::string message;
        Location location;
        std::vector<std::string> notes;
        std::string help;
    };

    // Central, reusable compiler diagnostic service. Locations point into the original source.
    class Reporter {
    public:
        void source(std::string file, std::string content);
        const std::string* sourceText(const std::string& file) const;
        void emit(Diagnostic diagnostic);
        void error(std::string code, std::string message, Location location = {});
        void warning(std::string code, std::string message, Location location = {});
        void info(std::string code, std::string message, Location location = {});
        const std::vector<Diagnostic>& diagnostics() const noexcept { return entries_; }
        std::size_t count(Severity severity) const;
        bool hasErrors() const { return count(Severity::Error) != 0; }
        void clear() { entries_.clear(); }
        void print(std::ostream& out, ColorPolicy policy = ColorPolicy::Auto) const;
        void printSummary(std::ostream& out, ColorPolicy policy = ColorPolicy::Auto) const;
    private:
        std::map<std::string, std::string> sources_;
        std::vector<Diagnostic> entries_;
    };
}

namespace {
    bool enabled(ISPA::Diagnostics::ColorPolicy policy) {
        if (policy == ISPA::Diagnostics::ColorPolicy::Never) return false;
        if (policy == ISPA::Diagnostics::ColorPolicy::Always) return true;
        if (std::getenv("NO_COLOR")) return false;
#if defined(_WIN32)
        return _isatty(_fileno(stderr)) != 0;
#else
        return isatty(fileno(stderr)) != 0;
#endif
    }
    std::string_view label(ISPA::Diagnostics::Severity s) {
        using S = ISPA::Diagnostics::Severity;
        switch (s) { case S::Error: return "error"; case S::Warning: return "warning"; case S::Info: return "note"; }
        return "diagnostic";
    }
    std::string_view tint(ISPA::Diagnostics::Severity s) {
        using S = ISPA::Diagnostics::Severity;
        switch (s) { case S::Error: return "\033[1;31m"; case S::Warning: return "\033[1;33m"; case S::Info: return "\033[1;34m"; }
        return "";
    }
    constexpr std::string_view reset = "\033[0m";
    constexpr std::string_view dim = "\033[2m";

    enum class Syntax { Plain, Keyword, String, Comment, Number, Capture, Operator };
    std::string_view syntaxTint(Syntax syntax) {
        switch (syntax) {
            case Syntax::Keyword: return "\033[35m";
            case Syntax::String: return "\033[32m";
            case Syntax::Comment: return "\033[90m";
            case Syntax::Number: return "\033[36m";
            case Syntax::Capture: return "\033[34m";
            case Syntax::Operator: return "\033[33m";
            case Syntax::Plain: return "";
        }
        return "";
    }
    bool wordStart(char c) {
        return c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }
    bool wordPart(char c) { return wordStart(c) || (c >= '0' && c <= '9'); }
    bool digit(char c) { return c >= '0' && c <= '9'; }

    // Scan from the beginning so snippets inside multiline comments and strings keep their style.
    std::vector<Syntax> syntaxStyles(std::string_view source, std::string_view file) {
        std::vector<Syntax> styles(source.size(), Syntax::Plain);
        const bool python = file.ends_with(".py");
        const auto paint = [&](std::size_t begin, std::size_t end, Syntax style) {
            std::fill(styles.begin() + begin, styles.begin() + end, style);
        };
        constexpr std::string_view ispaKeywords =
            " module import from as detach feature fail match inline skip extern global auto ";
        constexpr std::string_view pythonKeywords =
            " False None True and as assert async await break class continue def del elif else except "
            " finally for from global if import in is lambda nonlocal not or pass raise return try while with yield ";
        for (std::size_t i = 0; i < source.size();) {
            const std::size_t begin = i;
            const char c = source[i];
            const char next = i + 1 < source.size() ? source[i + 1] : '\0';
            if ((python && c == '#') || (!python && c == '/' && next == '/')) {
                while (i < source.size() && source[i] != '\n') ++i;
                paint(begin, i, Syntax::Comment);
            } else if (!python && c == '/' && next == '*') {
                i += 2;
                while (i < source.size() && !(source[i - 1] == '*' && source[i] == '/')) ++i;
                if (i < source.size()) ++i;
                paint(begin, i, Syntax::Comment);
            } else if (c == '\'' || c == '"') {
                const bool triple = python && i + 2 < source.size() && source[i + 1] == c && source[i + 2] == c;
                i += triple ? 3 : 1;
                while (i < source.size()) {
                    if (source[i] == '\\') { i = std::min(i + 2, source.size()); continue; }
                    if (source[i] == c && (!triple || (i + 2 < source.size() &&
                            source[i + 1] == c && source[i + 2] == c))) {
                        i += triple ? 3 : 1;
                        break;
                    }
                    if (!triple && source[i] == '\n') break;
                    ++i;
                }
                paint(begin, i, Syntax::String);
            } else if (!python && c == '[') {
                ++i;
                while (i < source.size()) {
                    if (source[i] == '\\') { i = std::min(i + 2, source.size()); continue; }
                    if (source[i++] == ']') break;
                }
                paint(begin, i, Syntax::String);
            } else if (digit(c)) {
                do { ++i; } while (i < source.size() && (wordPart(source[i]) || source[i] == '.'));
                paint(begin, i, Syntax::Number);
            } else if (wordStart(c)) {
                do { ++i; } while (i < source.size() && wordPart(source[i]));
                const auto word = source.substr(begin, i - begin);
                const auto keywords = python ? pythonKeywords : ispaKeywords;
                if (keywords.find(std::string(" ") + std::string(word) + " ") != std::string_view::npos)
                    paint(begin, i, Syntax::Keyword);
                else if (!python && begin && (source[begin - 1] == '@' || source[begin - 1] == '%'))
                    paint(begin, i, Syntax::Capture);
            } else {
                ++i;
                if (!python && (c == '@' || c == '%' || c == '&' || c == '#'))
                    paint(begin, i, Syntax::Capture);
                else if (std::string_view("=:;|*+?(){}.,<>").find(c) != std::string_view::npos)
                    paint(begin, i, Syntax::Operator);
            }
        }
        return styles;
    }
    void printSourceLine(std::ostream& out, std::string_view raw,
                         const std::vector<Syntax>& styles, std::size_t offset, bool color) {
        std::size_t column = 0;
        Syntax active = Syntax::Plain;
        for (std::size_t i = 0; i < raw.size(); ++i) {
            const Syntax style = color ? styles[offset + i] : Syntax::Plain;
            if (style != active) {
                out << reset << syntaxTint(style);
                active = style;
            }
            if (raw[i] == '\t') {
                const auto width = 4 - column % 4;
                out << std::string(width, ' ');
                column += width;
            } else if (raw[i] != '\r') {
                out << raw[i];
                ++column;
            }
        }
        if (active != Syntax::Plain) out << reset;
    }

    // Prefer byte offsets. Some frontends only retain a line and column.
    ISPA::Diagnostics::Location fixLocation(ISPA::Diagnostics::Location p,
                                            const std::string* source) {
        if (!source) return p;
        if (p.offset == std::string::npos || p.offset > source->size()) {
            if (!p.line) return p;
            std::size_t begin = 0;
            for (std::size_t line = 1; line < p.line; ++line) {
                const auto newline = source->find('\n', begin);
                if (newline == std::string::npos) return p;
                begin = newline + 1;
            }
            const auto end = source->find('\n', begin);
            const auto limit = end == std::string::npos ? source->size() : end;
            p.offset = std::min(begin + (p.column ? p.column - 1 : 0), limit);
        }
        p.line = p.column = 1;
        for (std::size_t i = 0; i < p.offset; ++i) {
            if ((*source)[i] == '\n') { ++p.line; p.column = 1; }
            else ++p.column;
        }
        return p;
    }
    std::string expandTabs(std::string_view s, std::size_t column = 0) {
        std::string result;
        for (char c : s) {
            if (c == '\t') result.append(4 - ((column + result.size()) % 4), ' ');
            else if (c != '\r') result += c;
        }
        return result;
    }
}

namespace ISPA::Diagnostics {
    void Reporter::source(std::string file, std::string content) {
        sources_.insert_or_assign(std::move(file), std::move(content));
    }
    const std::string* Reporter::sourceText(const std::string& file) const {
        const auto it = sources_.find(file);
        return it == sources_.end() ? nullptr : &it->second;
    }
    void Reporter::emit(Diagnostic d) { entries_.push_back(std::move(d)); }
    void Reporter::error(std::string c, std::string m, Location p) {
        emit({Severity::Error, std::move(c), std::move(m), std::move(p), {}, {}});
    }
    void Reporter::warning(std::string c, std::string m, Location p) {
        emit({Severity::Warning, std::move(c), std::move(m), std::move(p), {}, {}});
    }
    void Reporter::info(std::string c, std::string m, Location p) {
        emit({Severity::Info, std::move(c), std::move(m), std::move(p), {}, {}});
    }
    std::size_t Reporter::count(Severity s) const {
        return static_cast<std::size_t>(std::count_if(entries_.begin(), entries_.end(),
            [s](const auto& d) { return d.severity == s; }));
    }
    void Reporter::print(std::ostream& out, ColorPolicy policy) const {
        const bool color = enabled(policy);
        for (const Diagnostic& d : entries_) {
            const auto* src = sourceText(d.location.file);
            const Location p = fixLocation(d.location, src);
            const std::string_view accent = color ? tint(d.severity) : std::string_view{};
            const std::string_view normal = color ? reset : std::string_view{};
            const std::string_view faint = color ? dim : std::string_view{};
            out << accent << label(d.severity) << normal;
            if (!d.code.empty()) out << "[" << d.code << "]";
            out << ": " << d.message << '\n';
            if (!p.file.empty()) {
                out << faint << "  --> " << normal << p.file;
                if (p.line) out << ':' << p.line << ':' << (p.column ? p.column : 1);
                out << '\n';
            }
            if (src && p.offset != std::string::npos && p.offset <= src->size()) {
                const auto styles = color ? syntaxStyles(*src, p.file) : std::vector<Syntax>{};
                const auto start = p.offset == 0 ? std::string::npos : src->rfind('\n', p.offset - 1);
                const auto begin = start == std::string::npos ? 0 : start + 1;
                auto end = src->find('\n', p.offset);
                if (end == std::string::npos) end = src->size();
                const auto contextStart = begin < 2 ? std::string::npos : src->rfind('\n', begin - 2);
                const auto previousBegin = contextStart == std::string::npos ? 0 : contextStart + 1;
                const auto nextBegin = end < src->size() ? end + 1 : src->size();
                std::string_view raw(src->data() + begin, end - begin);
                const auto local = std::min(p.offset - begin, raw.size());
                const auto n = std::min(std::max<std::size_t>(p.length, 1), raw.size() - local);
                const std::string before = expandTabs(raw.substr(0, local));
                const std::string marked = expandTabs(raw.substr(local, n), before.size());
                const std::string gutter = std::to_string(p.line ? p.line : 1);
                if (begin != 0) {
                    out << faint << ' ' << (p.line - 1) << " | " << normal;
                    printSourceLine(out, std::string_view(src->data() + previousBegin,
                        begin - previousBegin - 1), styles, previousBegin, color);
                    out << '\n';
                }
                out << faint << ' ' << gutter << " | " << normal;
                printSourceLine(out, raw, styles, begin, color);
                out << '\n';
                out << faint << std::string(gutter.size() + 2, ' ') << "| " << normal
                    << std::string(before.size(), ' ') << accent
                    << std::string(std::max<std::size_t>(1, marked.size()), '^') << normal << '\n';
                if (nextBegin < src->size()) {
                    auto nextEnd = src->find('\n', nextBegin);
                    if (nextEnd == std::string::npos) nextEnd = src->size();
                    out << faint << ' ' << (p.line + 1) << " | " << normal;
                    printSourceLine(out, std::string_view(src->data() + nextBegin,
                        nextEnd - nextBegin), styles, nextBegin, color);
                    out << '\n';
                }
            }
            for (const auto& note : d.notes) out << "  = note: " << note << '\n';
            if (!d.help.empty()) out << "  = help: " << d.help << '\n';
        }
    }
    void Reporter::printSummary(std::ostream& out, ColorPolicy policy) const {
        (void)policy;
        out << count(Severity::Error) << " error(s), " << count(Severity::Warning)
            << " warning(s), " << count(Severity::Info) << " info message(s)\n";
    }
}
