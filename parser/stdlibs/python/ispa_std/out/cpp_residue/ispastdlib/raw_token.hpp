#pragma once
#include <ispastdlib/node.hpp>
#include <ispastdlib/traits.hpp>
#include <cassert>
namespace ISPA_STD {

    class RawTokenMeta {
        std::size_t _startpos = static_cast<std::size_t>(-1);
        std::size_t _length = 0;
        std::size_t _line = 0;
        std::size_t _column = 0;
        const char* _start = nullptr;
        const char* _end = nullptr;
        std::size_t _name = std::numeric_limits<std::size_t>::max();

    public:
        RawTokenMeta() = default;
        RawTokenMeta(std::size_t startpos, const char* start, const char* end,
                     std::size_t length, std::size_t line, std::size_t column, std::size_t name)
            : _startpos(startpos), _length(length), _line(line), _column(column),
              _start(start), _end(end), _name(name) {}

        std::size_t startpos() const { return _startpos; }
        std::size_t length()   const { return _length; }
        std::size_t line()     const { return _line; }
        std::size_t column()   const { return _column; }
        const char* start()    const { return _start; }
        const char* end()      const { return _end; }
        std::size_t name()     const { return _name; }
    };

    template<typename... Ts>
    class RawToken {
        static_assert(sizeof...(Ts) > 0 && sizeof...(Ts) < 0xFFFF, "RawToken: bad alternative count");
        static_assert((std::is_nothrow_move_constructible_v<Ts> && ...),
                      "RawToken requires all alternatives to be nothrow-move-constructible");

        static constexpr std::size_t storage_size  = std::max({sizeof(Ts)...});
        static constexpr std::size_t storage_align = std::max({alignof(Ts)...});
        static constexpr std::uint16_t npos_id = static_cast<std::uint16_t>(-1);

        template<typename T>
        static constexpr std::size_t index_of_v = [] {
            constexpr bool m[] = { std::is_same_v<T, Ts>... };
            std::size_t found = static_cast<std::size_t>(-1), count = 0;
            for (std::size_t i = 0; i < sizeof...(Ts); ++i)
                if (m[i]) { found = i; ++count; }
            return count == 1 ? found : static_cast<std::size_t>(-1);
        }();

        alignas(storage_align) unsigned char _storage[storage_size];
        std::uint16_t _type_id = npos_id;
        RawTokenMeta _meta;

        template<typename T> static void destroy_impl(void* p) noexcept { static_cast<T*>(p)->~T(); }
        template<typename T> static void copy_ctor_impl(void* d, const void* s) {
            ::new (d) T(*static_cast<const T*>(s));
        }
        template<typename T> static void move_ctor_impl(void* d, void* s) noexcept {
            ::new (d) T(std::move(*static_cast<T*>(s)));
        }
        using destroy_fn_t   = void(*)(void*) noexcept;
        using copy_ctor_fn_t = void(*)(void*, const void*);
        using move_ctor_fn_t = void(*)(void*, void*) noexcept;

        static constexpr destroy_fn_t   destroy_table[] = { &destroy_impl<Ts>... };
        static constexpr copy_ctor_fn_t copy_table[]    = { &copy_ctor_impl<Ts>... };
        static constexpr move_ctor_fn_t move_table[]    = { &move_ctor_impl<Ts>... };

        void destroy() noexcept {
            if (_type_id != npos_id) destroy_table[_type_id](_storage);
            _type_id = npos_id;
        }

    public:
        using meta_type = RawTokenMeta;
        static constexpr std::size_t ambiguous_id = std::numeric_limits<std::size_t>::max() - 1;
        static RawToken ambiguous(meta_type meta) { RawToken out; out._meta = meta; return out; }
        bool isAmbiguous() const { return kind() == ambiguous_id; }

        RawToken() = default;

        template<typename T>
        RawToken(meta_type meta, T&& value) : _meta(meta) {
            using U = std::decay_t<T>;
            constexpr std::size_t idx = index_of_v<U>;
            static_assert(idx != static_cast<std::size_t>(-1),
                          "RawToken: payload type is not an alternative (or is listed twice)");
            ::new (_storage) U(std::forward<T>(value));
            _type_id = static_cast<std::uint16_t>(idx);
        }

        template<typename D, typename E>
        static RawToken from_node(const Node<D, E>& n) {
            meta_type m{n.startpos(), n.start(), n.end(), n.length(), n.line(), n.column(), n.name()};
            if (n.empty()) { RawToken r; r._meta = m; return r; }
            return RawToken(m, n.data());
        }
        
        template<typename D, typename E>
        static RawToken from_node(Node<D, E>&& n) {
            meta_type m{n.startpos(), n.start(), n.end(), n.length(), n.line(), n.column(), n.name()};
            if (n.empty()) { RawToken r; r._meta = m; return r; }
            return RawToken(m, std::move(n.data()));
        }

        RawToken(const RawToken& o) : _type_id(o._type_id), _meta(o._meta) {
            if (_type_id != npos_id) copy_table[_type_id](_storage, o._storage);
        }
        RawToken(RawToken&& o) noexcept : _type_id(o._type_id), _meta(o._meta) {
            if (_type_id != npos_id) move_table[_type_id](_storage, o._storage);
            o.destroy();
        }
        RawToken& operator=(const RawToken& o) {
            if (this == &o) return *this;
            RawToken tmp(o);
            return *this = std::move(tmp);
        }
        RawToken& operator=(RawToken&& o) noexcept {
            if (this == &o) return *this;
            destroy();
            _type_id = o._type_id;
            _meta = o._meta;
            if (_type_id != npos_id) move_table[_type_id](_storage, o._storage);
            o.destroy();
            return *this;
        }
        ~RawToken() { destroy(); }

        bool empty() const { return _type_id == npos_id; }
        const meta_type& meta() const { return _meta; }

        std::size_t kind() const { return _meta.name(); }
        std::size_t index() const { return _type_id == npos_id ? static_cast<std::size_t>(-1) : _type_id; }

        template<typename T>
        bool holds() const {
            constexpr std::size_t idx = index_of_v<T>;
            static_assert(idx != static_cast<std::size_t>(-1), "RawToken::holds<T>: not an alternative");
            return _type_id == idx;
        }

        template<typename T> T& get() {
            assert((holds<T>()) && "RawToken::get<T>: alternative mismatch");
            return *std::launder(reinterpret_cast<T*>(_storage));
        }
        template<typename T> const T& get() const {
            assert((holds<T>()) && "RawToken::get<T>: alternative mismatch");
            return *std::launder(reinterpret_cast<const T*>(_storage));
        }

        // Returns Node<EnumT, TargetType> payload directly
        template<typename TargetType>
        auto to_node() const {
            if constexpr (is_raw_token_v<TargetType>) {
                return *this;
            } else if constexpr (is_node_v<TargetType>) {
                using InnerPayload = typename TargetType::data_type;
                if (_type_id == npos_id || !holds<InnerPayload>()) {
                    return TargetType(_meta.startpos(), _meta.start(), _meta.end(),
                                     _meta.length(), _meta.line(), _meta.column(), _meta.name());
                }
                return TargetType(_meta.startpos(), _meta.start(), _meta.end(),
                                 _meta.length(), _meta.line(), _meta.column(), _meta.name(),
                                 get<InnerPayload>());
            } else {
                if (_type_id == npos_id || !holds<TargetType>()) {
                    return Node<TargetType>(_meta.startpos(), _meta.start(), _meta.end(),
                                                   _meta.length(), _meta.line(), _meta.column(), _meta.name());
                }
                return Node<TargetType>(_meta.startpos(), _meta.start(), _meta.end(),
                                               _meta.length(), _meta.line(), _meta.column(), _meta.name(),
                                               get<TargetType>());
            }
        }
        template<typename ValueType>
        static RawToken create(long long startpos, const char* start, long long length,
                        long long line, std::size_t enumv, ValueType&& value) {
            const std::size_t u_startpos = static_cast<std::size_t>(startpos);
            const std::size_t u_length   = static_cast<std::size_t>(length);
            const std::size_t u_line     = static_cast<std::size_t>(line);
            const char* end              = start ? start + u_length : nullptr;

            meta_type meta{u_startpos, start, end, u_length, u_line, 0, enumv};

            if constexpr (is_node_v<std::decay_t<ValueType>>) {
                if (value.empty()) {
                    RawToken r;
                    r._meta = meta;
                    return r;
                }
                return RawToken(meta, std::forward<ValueType>(value).data());
            } else {
                return RawToken(meta, std::forward<ValueType>(value));
            }
        }
        void print(ASTPrinter<std::ostream>& printer) const {
            if (empty()) {
                printer.node("<empty>");
                return;
            }

            using print_ast_fn_t = void(*)(const void*, ASTPrinter<std::ostream>&);

            static constexpr print_ast_fn_t print_ast_table[] = {
                (+[](const void* p, ASTPrinter<std::ostream>& pr) {
                    const auto& item = *static_cast<const Ts*>(p);
                    item.write_to_output(pr);
                })...
            };

            print_ast_table[_type_id](_storage, printer);
        }

        void print(std::ostream& os) const {
            if (empty()) {
                os << "<empty>";
                return;
            }

            using print_fn_t = void(*)(const void*, std::ostream&);

            static constexpr print_fn_t print_table[] = {
                (+[](const void* p, std::ostream& stream) {
                    const auto& item = *static_cast<const Ts*>(p);
                    stream << item;
                })...
            };

            print_table[_type_id](_storage, os);
        }

        auto to_string() const -> std::string {
            if (empty()) {
                throw IspaAPIExcept("Can't retrieve string representation of an empty node");
            }

            using to_string_fn_t = std::string(*)(const void*);

            static constexpr to_string_fn_t to_string_table[] = {
                (+[](const void* p) -> std::string {
                    using ItemType = Ts; // Current type in pack expansion
                    const auto& item = *static_cast<const ItemType*>(p);

                    if constexpr (has_to_string<ItemType>::value) {
                        return item.to_string();
                    } else if constexpr (is_streamable<ItemType>::value) {
                        std::ostringstream oss;
                        oss << item;
                        return oss.str();
                    } else if constexpr (std::is_same_v<ItemType, std::string>) {
                        return item;
                    } else {
                        throw IspaAPIExcept("Alternative type does not support to_string() or operator<<");
                    }
                })...
            };

            return to_string_table[_type_id](_storage);
        }
    };
    template<typename ...Ts>
    inline auto operator<<(std::ostream &os, const RawToken<Ts...>& token) {
        return os << token.print(os);
    }
} // namespace ISPA_STD
