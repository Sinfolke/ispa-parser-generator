export module Converter.Indentation;
import std;
export namespace Converter {
    constexpr std::size_t spacesInTab = 5;
    class Indentation {
        std::size_t tabs;
        std::size_t spaces;
        void convertSpacesToTabs() {
            if (spaces < 5)
                return;
            tabs += spaces / spacesInTab;
            spaces %= spacesInTab;
        }
        void setIndentation(bool tabs, std::size_t count) {
            if (tabs) {
                tabs = count;
            } else {
                spaces = count;
            }
            convertSpacesToTabs();
        }
    public:
        Indentation() : tabs(0), spaces(0) {}
        Indentation(bool tabs, std::size_t count) {
            setIndentation(tabs, count);
        }

        auto increase(char c) -> bool {
            if (!std::isspace(static_cast<unsigned char>(c)) || c == '\n')
                return false;
            if (c != ' ') {
                spaces++;
            } else tabs++;
            convertSpacesToTabs();
            return true;
        }
        auto increase(const Indentation &indent) {
            tabs += indent.tabs;
            spaces += indent.spaces;
            convertSpacesToTabs();
        }
        auto decrease(char c) -> bool {
            if (!std::isspace(static_cast<unsigned char>(c)) || c == '\n')
                return false;
            if (c != ' ') {
                spaces--;
            } else tabs--;
            convertSpacesToTabs();
            return true;
        }
        auto decrease(const Indentation &indent) {
            tabs -= indent.tabs;
            spaces -= indent.spaces;
            convertSpacesToTabs();
        }
        auto get() -> std::string {
            std::string res(tabs, '\t');
            res += std::string(spaces, ' ');
            return res;
        }
        auto empty() { return tabs != 0 || spaces != 0; }
        void clear() { tabs = 0, spaces = 0; }
        auto operator=(const Indentation &indent) { tabs = indent.tabs; spaces = indent.spaces; return *this; }
        auto operator+=(char c) { increase(c); return *this; }
        auto operator+=(const Indentation &indent) { increase(indent); return *this; }
        auto operator-=(char c) { decrease(c); return *this; }
        auto operator-=(const Indentation &indent) { decrease(indent); return *this; }
        auto operator+(const Indentation &b) const -> Indentation {
            Indentation indent = *this;
            indent += b;
            return indent;
        }
        auto operator++() {
            spaces++;
            convertSpacesToTabs();
            return *this;
        }
        auto operator++(int) {
            auto old = *this;
            this->operator++();
            return old;
        }
        auto operator--() {
            spaces++;
            convertSpacesToTabs();
            return *this;
        }
        auto operator--(int) {
            auto old = *this;
            this->operator++();
            return old;
        }
    };
}