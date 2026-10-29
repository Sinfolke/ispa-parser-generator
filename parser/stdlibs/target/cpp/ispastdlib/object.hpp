/*
 * ispastdlib/object.hpp - object model of lowered Python classes.
 * An instance is held through std::shared_ptr (Python reference semantics). `__init__` is an ordinary
 * method run by construct() on a default-constructed object, so `self` can escape from it.
 */
#pragma once
#include <memory>
#include <type_traits>
#include <utility>

namespace ISPA_STD {
struct Object : std::enable_shared_from_this<Object> {
    virtual ~Object() = default;
};

namespace detail {
template<typename T, typename... A>
auto init_object(T& object, int, A&&... args) -> decltype(object.__init__(std::forward<A>(args)...), void()) {
    object.__init__(std::forward<A>(args)...);
}
template<typename T, typename... A>
void init_object(T&, long, A&&...) {}
}

template<typename T, typename... A>
std::shared_ptr<T> construct(A&&... args) {
    auto object = std::make_shared<T>();
    detail::init_object(*object, 0, std::forward<A>(args)...);
    return object;
}

// `self` used as a value inside a method: converts to the shared_ptr of whichever class receives it.
struct SelfPtr {
    Object* self;
    template<typename T> operator std::shared_ptr<T>() const { return std::static_pointer_cast<T>(self->shared_from_this()); }
};
inline SelfPtr self_ptr(Object* self) { return SelfPtr{self}; }
}
