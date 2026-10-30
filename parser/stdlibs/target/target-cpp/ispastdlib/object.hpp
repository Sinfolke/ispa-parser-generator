/*
 * ispastdlib/object.hpp - object model of lowered Python classes.
 * An instance is held through std::shared_ptr (Python reference semantics). `__init__` is an ordinary
 * method run by construct() on a default-constructed object, so `self` can escape from it.
 */
#pragma once
#include <cstdlib>
#include <memory>
#include <memory_resource>
#include <type_traits>
#include <utility>
#ifdef ISPA_COUNT_CONSTRUCT
#include <map>
#include <string>
#include <typeinfo>
#include <cstdio>
#include <cstdlib>
#endif

namespace ISPA_STD {
#ifdef ISPA_COUNT_CONSTRUCT
// Diagnostics: how many instances of each class a run constructs (ALLOC_REPORT=1 prints them at exit).
struct ConstructCounts {
    std::map<std::string, unsigned long long> counts;
    ~ConstructCounts() {
        if (!std::getenv("ALLOC_REPORT")) return;
        for (const auto& [name, n] : counts) if (n > 1000) std::fprintf(stderr, "CONSTRUCT %10llu %s\n", n, name.c_str());
    }
};
inline ConstructCounts& construct_counts() { static ConstructCounts c; return c; }
template<typename T> void count_construct() { ++construct_counts().counts[typeid(T).name()]; }
#else
template<typename T> inline void count_construct() {}
#endif
// Objects built while a parse runs come from a pool owned by the parse (ArenaScope) instead of the heap: the
// control block and the object are one pool block, freed blocks are reused, and nothing is left behind when
// the scope ends. Outside a scope the heap is used.
inline std::pmr::memory_resource*& current_arena() { static thread_local std::pmr::memory_resource* resource = nullptr; return resource; }
struct ArenaScope {
    std::pmr::unsynchronized_pool_resource pool;
    std::pmr::memory_resource* previous;
    ArenaScope() : previous(current_arena()) { /* opt-in: measured no faster yet */ if (std::getenv("ISPA_ARENA")) current_arena() = &pool; }
    ~ArenaScope() { current_arena() = previous; }
    ArenaScope(const ArenaScope&) = delete;
    ArenaScope& operator=(const ArenaScope&) = delete;
};
// Objects that must outlive the scope (caches, statics, members of long-lived objects) are built under a HeapScope.
struct HeapScope {
    std::pmr::memory_resource* previous;
    HeapScope() : previous(current_arena()) { current_arena() = nullptr; }
    ~HeapScope() { current_arena() = previous; }
    HeapScope(const HeapScope&) = delete;
    HeapScope& operator=(const HeapScope&) = delete;
};
template<typename T, typename... A>
std::shared_ptr<T> make_object(A&&... args) {
    count_construct<T>();
    if (auto* arena = current_arena())
        return std::allocate_shared<T>(std::pmr::polymorphic_allocator<T>(arena), std::forward<A>(args)...);
    return std::make_shared<T>(std::forward<A>(args)...);
}

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
    auto object = make_object<T>();
    detail::init_object(*object, 0, std::forward<A>(args)...);
    return object;
}

// A stack object: constructed in place, no allocation. Used where the compiler proved it never outlives the call.
template<typename T, typename... A>
T make_value(A&&... args) {
    T object;
    detail::init_object(object, 0, std::forward<A>(args)...);
    return object;
}
// A stack object handed to a callee that is known not to keep it: a non-owning shared_ptr.
template<typename T>
std::shared_ptr<T> borrow(T& object) { return std::shared_ptr<T>(std::shared_ptr<void>(), &object); }

// `self` used as a value inside a method: converts to the shared_ptr of whichever class receives it.
struct SelfPtr {
    Object* self;
    template<typename T> operator std::shared_ptr<T>() const { return std::static_pointer_cast<T>(self->shared_from_this()); }
};
inline SelfPtr self_ptr(Object* self) { return SelfPtr{self}; }
}
