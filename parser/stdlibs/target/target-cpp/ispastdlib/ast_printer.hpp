/**
 * ispastdlib/ast_printer.hpp - ASTPrinter tree printer
 * Part of the split ISPA std lib; include <ispastdlib.hpp> for everything.
 */
#pragma once
#include <tuple>
#include <ispastdlib/traits.hpp>

namespace ISPA_STD {
    /*
     * An AST print API that simply generator's work on every language internals
     */
    template<typename OS>
    class ASTPrinter {
    public:
        explicit ASTPrinter(OS& output)
            : out(&output) {}

        explicit ASTPrinter(OS* output = nullptr)
            : out(output) {}

        void set_output(OS* output) {
            out = output;
        }

        void up(bool has_more_siblings = true) {
            branches.push_back(has_more_siblings);
        }

        void down() {
            if (!branches.empty())
                branches.pop_back();
        }

        void node(const char* name, bool last = false) {
            print_prefix(last);
            *out << name << '\n';
        }

        template<typename T>
        void node_with_value(
            const char* name,
            const T& value,
            bool last = false
        ) {
            print_named_value(name, value, last);
        }

        // Print a structured child without creating another printer.
        template<typename T>
        void subtree(
            const char* name,
            const T& value,
            bool last = false
        ) {
            node(name, last);
            up(!last);
            print_subtree(value);
            down();
        }

    private:
        template<typename T>
        void print_named_value(
            const char* name,
            const T& value,
            bool last
        ) {
            using Type = std::decay_t<T>;

            if constexpr (is_shared_ptr<Type>::value) {
                if (value) {
                    // Important: do not stream *value.
                    print_named_value(name, *value, last);
                } else {
                    print_prefix(last);
                    *out << name << ": nullptr\n";
                }
            } else if constexpr (
                is_vector<Type>::value ||
                is_variant<Type>::value ||
                is_tuple_like_v<Type> ||
                has_write_to_output<Type, ASTPrinter<OS>>::value ||
                has_ast_print<Type, ASTPrinter<OS>>::value
            ) {
                subtree(name, value, last);

            } else {
                print_prefix(last);
                *out << name << ": " << value << '\n';
            }
        }
        template<typename T>
        void print_value(const T& value) {
            if constexpr (is_shared_ptr<std::decay_t<T>>::value) {
                if (value) {
                    print_value(*value);
                } else {
                    *out << "nullptr";
                }
            } else {
                *out << value;
            }
        }

        template<typename T>
        void print_subtree(const T& value) {
            using Type = std::decay_t<T>;

            if constexpr (is_shared_ptr<Type>::value) {
                if (value)
                    print_subtree(*value);
                else
                    node("nullptr", true);

            } else if constexpr (is_vector<Type>::value) {
                for (std::size_t i = 0; i < value.size(); ++i) {
                    const bool last = i + 1 == value.size();

                    node("item", last);
                    up(!last);
                    print_subtree(value[i]);
                    down();
                }

            } else if constexpr (is_tuple_like_v<Type>) {
                std::apply([this](const auto&... items) {
                    std::size_t index = 0;
                    (node_with_value("item", items, ++index == sizeof...(items)), ...);
                }, value);
            } else if constexpr (is_variant<Type>::value) {
                std::visit([this](const auto& alternative) {
                    using Alt = std::decay_t<decltype(alternative)>;

                    if constexpr (!std::is_same_v<Alt, std::monostate>)
                        print_subtree(alternative);
                }, value);

            } else if constexpr (
                has_write_to_output<Type, ASTPrinter<OS>>::value
            ) {
                value.write_to_output(*this);

            } else if constexpr (
                has_ast_print<Type, ASTPrinter<OS>>::value
            ) {
                value.print(*this);

            } else {
                node_with_value("value", value, true);
            }
        }

        void print_prefix(bool last) {
            const std::size_t depth = branches.size();

            if (depth == 0)
                return;

            for (std::size_t i = 0; i + 1 < depth; ++i) {
                *out << (branches[i] ? "│   " : "    ");
            }

            *out << (last ? "└── " : "├── ");
        }

    private:
        OS* out = nullptr;
        std::vector<bool> branches;
    };
} // namespace ISPA_STD
