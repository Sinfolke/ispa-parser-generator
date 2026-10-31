// WebAssembly build: the algorithm and converter plugins are linked into the executable instead of
// being separate shared libraries, and the executable is not a dynamically linked module at all.
// Linking with --wrap=dlopen/dlsym/dlclose routes the plugin loader here: any library name resolves
// to a fake handle and the plugin entry points are looked up in a static table.
#ifdef __EMSCRIPTEN__
#include <cstring>

extern "C" {
void ispa_algorithm_v1();
void python_construct();
void python_construct_finish();

void* __wrap_dlopen(const char*, int) {
    static int handle;
    return &handle;
}

void* __wrap_dlsym(void*, const char* name) {
    static const struct { const char* name; void (*entry)(); } entries[] = {
        {"ispa_algorithm_v1", ispa_algorithm_v1},
        {"python_construct", python_construct},
        {"python_construct_finish", python_construct_finish},
    };
    for (const auto& entry : entries)
        if (std::strcmp(entry.name, name) == 0) return reinterpret_cast<void*>(entry.entry);
    return nullptr;
}

int __wrap_dlclose(void*) { return 0; }
}
#endif
