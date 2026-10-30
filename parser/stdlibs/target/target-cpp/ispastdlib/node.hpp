/**
 * ispastdlib/node.hpp - Node, MatchResult, Seq
 * Part of the split ISPA std lib; include <ispastdlib.hpp> for everything.
 */
#pragma once
#include <ispastdlib/traits.hpp>
#include <ispastdlib/ast_printer.hpp>
#include <ispastdlib/core.hpp>
#include <ispastdlib/ispa_debug.hpp>

namespace ISPA_STD {
    template<class Destination, class Source>
    struct is_node_storage_conversion_allowed : std::false_type {};

    template<class T>
    struct is_node_storage_conversion_allowed<std::shared_ptr<T>, T> : std::true_type {};

    template<class T>
    struct is_node_storage_conversion_allowed<T, std::shared_ptr<T>> : std::true_type {};

    template<class DataStorageType, class = std::enable_if_t<std::is_class_v<DataStorageType>>>
    class Node {
        std::size_t _startpos = std::string::npos;
        std::size_t _length = 0;
        std::size_t _line = 0;
        std::size_t _column = 0;
        const char* _start = nullptr;
        const char* _end = nullptr;
        std::size_t _name = std::numeric_limits<std::size_t>::max();
        bool _empty = true;
        DataStorageType _data{};

        template<typename Other>
        void copy_metadata(const Other& other) {
            _startpos = other.startpos();
            _length = other.length();
            _line = other.line();
            _column = other.column();
            _start = other.start();
            _end = other.end();
            _name = other.name();
            _empty = other.empty();
        }

        // Conversions are instantiated only when invoked, not when a recursive
        // Node<EnumT, shared_ptr<Incomplete>> is first instantiated.
        // The payload of `other`, movable when the caller passed an rvalue node (converting a temporary must not deep-copy).
        template<typename Other>
        static decltype(auto) payload_of(Other&& other) {
            if constexpr (!std::is_lvalue_reference_v<Other> && !std::is_const_v<std::remove_reference_t<Other>>)
                return std::move(other.data());
            else
                return (other.data());
        }

        template<typename Other>
        void assign_converted(Other&& other) {
            using SourceStorage = typename std::remove_cv_t<std::remove_reference_t<Other>>::data_type;
            if constexpr (is_shared_ptr<DataStorageType>::value &&
                          !is_shared_ptr<SourceStorage>::value) {
                using Element = typename is_shared_ptr<DataStorageType>::element_type;
                static_assert(std::is_same_v<Element, SourceStorage>,
                              "Incompatible value-to-pointer Node conversion");
                if (other.empty()) {
                    _data.reset();
                } else {
                    _data = std::make_shared<Element>(payload_of(std::forward<Other>(other)));
                }
            } else if constexpr (!is_shared_ptr<DataStorageType>::value &&
                                 is_shared_ptr<SourceStorage>::value) {
                using Element = typename is_shared_ptr<SourceStorage>::element_type;
                static_assert(std::is_same_v<Element, DataStorageType>,
                              "Incompatible pointer-to-value Node conversion");
                if (other.empty() || !other.data()) {
                    _data = DataStorageType{};
                } else {
                    _data = *other.data();
                }
            } else {
                static_assert(!std::is_same_v<SourceStorage, SourceStorage>,
                              "Unsupported cross-storage Node conversion");
            }
            copy_metadata(other);
            if constexpr (is_shared_ptr<DataStorageType>::value) {
                if (!_data) _empty = true;
            }
        }

    public:
        using data_type = DataStorageType;

        Node() = default;

        Node(std::size_t startpos, const char* start, const char* end,
             std::size_t length, std::size_t line, std::size_t column, std::size_t name)
            : _startpos(startpos), _length(length), _line(line),
              _column(column), _start(start), _end(end), _name(name) {}

        template<class... Args,
                 std::enable_if_t<(sizeof...(Args) > 0 &&
                                   std::is_constructible_v<DataStorageType, Args...>), int> = 0>
        Node(std::size_t startpos, const char* start, const char* end,
             std::size_t length, std::size_t line, std::size_t column,
             std::size_t name, Args&&... args)
            : _startpos(startpos), _length(length), _line(line),
              _column(column), _start(start), _end(end), _name(name),
              _empty(false), _data(std::forward<Args>(args)...) {}

        Node(const Node&) = default;
        Node(Node&&) noexcept(std::is_nothrow_move_constructible_v<DataStorageType>) = default;
        Node& operator=(const Node&) = default;
        Node& operator=(Node&&) noexcept(std::is_nothrow_move_assignable_v<DataStorageType>) = default;

        template<typename SourceStorage,
                 std::enable_if_t<is_node_storage_conversion_allowed<DataStorageType, SourceStorage>::value, int> = 0>
        Node(const Node<SourceStorage>& other) {
            assign_converted(other);
        }

        template<typename SourceStorage,
                 std::enable_if_t<is_node_storage_conversion_allowed<DataStorageType, SourceStorage>::value, int> = 0>
        Node& operator=(const Node<SourceStorage>& other) {
            assign_converted(other);
            return *this;
        }

        template<typename SourceStorage,
                 std::enable_if_t<is_node_storage_conversion_allowed<DataStorageType, SourceStorage>::value, int> = 0>
        Node(Node<SourceStorage>&& other) {
            assign_converted(std::move(other));
        }

        template<typename SourceStorage,
                 std::enable_if_t<is_node_storage_conversion_allowed<DataStorageType, SourceStorage>::value, int> = 0>
        Node& operator=(Node<SourceStorage>&& other) {
            assign_converted(std::move(other));
            return *this;
        }

        template<typename IT>
        auto operator=(const MatchResult<DataStorageType, IT>& mr) -> Node&;

        template<typename SourceDataType, typename IT>
        Node& operator=(MatchResult<SourceDataType, IT>&& result) {
            *this = std::move(result.node);
            return *this;
        }

        template<typename SourceDataType, typename IT>
        Node& operator=(MatchResult<SourceDataType, IT>& result) {
            *this = result.node;
            return *this;
        }

        std::size_t endpos() const {
            if (_startpos == std::string::npos || !_start || !_end)
                throw IspaAPIExcept("Can't retrieve end position on an empty token");
            return _startpos + static_cast<std::size_t>(_end - _start);
        }

        void clear() {
            _startpos = std::string::npos;
            _line = 0;
            _column = 0;
            _length = 0;
            _start = nullptr;
            _end = nullptr;
            _name = std::numeric_limits<std::size_t>::max();
            _data = DataStorageType{};
            _empty = true;
        }

        bool empty() const { return _empty; }
        auto startpos() const { return _startpos; }
        auto line() const { return _line; }
        auto column() const { return _column; }
        auto length() const { return _length; }
        auto start() const { return _start; }
        auto end() const { return _end; }
        auto name() const { return _name; }
        DataStorageType& data() { return _data; }
        const DataStorageType& data() const { return _data; }

        void set_data(DataStorageType&& value) {
            _data = std::move(value);
            _empty = false;
        }
        void set_data(const DataStorageType& value) {
            _data = value;
            _empty = false;
        }

        static Node create(long long startpos, const char* start, long long length,
                           long long line, std::size_t enumv, DataStorageType value) {
            // Column cannot be recovered safely from a pointer to the token's
            // beginning alone; callers needing column should use the full ctor.
            return Node{static_cast<std::size_t>(startpos), start,
                        start ? start + length : nullptr,
                        static_cast<std::size_t>(length),
                        static_cast<std::size_t>(line), 0, enumv,
                        std::move(value)};
        }

        void print(ASTPrinter<std::ostream>& printer) const {
            if (_empty) {
                printer.node("<empty>");
                return;
            }
            if constexpr (std::is_pointer_v<DataStorageType> ||
                          is_shared_ptr<DataStorageType>::value) {
                if (_data) _data->write_to_output(printer);
            } else {
                _data.write_to_output(printer);
            }
        }

        void print(std::ostream& os) const {
            if (_empty) {
                os << "<empty>";
                return;
            }
            if constexpr (std::is_pointer_v<DataStorageType> ||
                          is_shared_ptr<DataStorageType>::value) {
                if (_data) os << *_data;
                else os << "nullptr";
            } else {
                os << _data;
            }
        }

        auto to_string() const {
            if (_empty)
                throw IspaAPIExcept("Can't retrieve string representation of an empty node");
            if constexpr (std::is_pointer_v<DataStorageType> ||
                          is_shared_ptr<DataStorageType>::value) {
                if (!_data)
                    throw IspaAPIExcept("Can't retrieve string representation of a null pointer (std::shared_ptr)");
                return _data->to_string();
            } else {
                return _data.to_string();
            }
        }
    };

    template<typename ENUM_T, typename NodeType>
    auto operator<<(std::ostream &os, const Node<ENUM_T, NodeType> &node) -> std::ostream& {
        node.print(os);
        return os;
    }
    template<class DataStorageType, class IT>
    struct MatchResult {
        bool status = false;
        Node<DataStorageType> node = {};
        IT it;
    };

    template<typename DataStorageType, typename Enable>
    template<typename IT>
    auto Node<DataStorageType, Enable>::operator=(
        const MatchResult<DataStorageType, IT>& mr
    ) -> Node& {
        return *this = mr.node;
    }
    template<class RULE_T, class DataStorageType>
    using Seq = std::vector<Node<RULE_T, DataStorageType>>;
    template<class T, class E, class F>
    void ispa_debug_fields(const Node<T, E>& node, F&& field) {
        field("empty", node.empty());
        if (!node.empty()) {
            field("name", node.name());
            field("startpos", node.startpos());
            field("length", node.length());
            field("line", node.line());
            field("column", node.column());
            field("data", node.data());
        }
    }
    template<class T, class IT, class F>
    void ispa_debug_fields(const MatchResult<T, IT>& result, F&& field) {
        field("status", result.status);
        field("node", result.node);
    }
} // namespace ISPA_STD

namespace ispa::debug {
    template<class T, class E> struct type_name<ISPA_STD::Node<T, E>> {
        static std::string get() { return "ISPA.Node"; }
    };
    template<class T, class IT> struct type_name<ISPA_STD::MatchResult<T, IT>> {
        static std::string get() { return "ISPA.MatchResult"; }
    };
}
