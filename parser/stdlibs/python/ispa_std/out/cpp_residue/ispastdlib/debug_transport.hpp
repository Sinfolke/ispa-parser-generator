// debug_transport.hpp - the OS-bound half of ispa_debug.hpp, as adapters for
// ispa_std/debug.py's configure(sink, control). Python side does framing,
// JSON and stepping; this file only moves bytes (getenv, write(2), read(2), mutex).
#pragma once
#include <cerrno>
#include <cstdlib>
#include <limits>
#include <mutex>
#include <string>
#ifdef _WIN32
# include <io.h>
#else
# include <unistd.h>
#endif

namespace ispa_std_transport {
inline int fd_env(const char* name) {
    const char* p = std::getenv(name);
    if (!p || !*p) return -1;
    char* end = nullptr;
    long n = std::strtol(p, &end, 10);
    return (*end || n < 0 || n > std::numeric_limits<int>::max()) ? -1 : int(n);
}

struct FdSink {  // debug.configure sink: write(text) -> bool
    int fd = fd_env("ISPA_DEBUG_FD");
    std::mutex m;
    bool enabled() const { return fd >= 0; }
    bool write(const std::string& s) {
        std::lock_guard<std::mutex> g(m);
        const char* p = s.data();
        std::size_t n = s.size();
        while (n) {
#ifdef _WIN32
            int r = ::_write(fd, p, static_cast<unsigned>(n > 0x7fffffff ? 0x7fffffff : n));
#else
            auto r = ::write(fd, p, n);
#endif
            if (r > 0) { p += r; n -= std::size_t(r); continue; }
#ifndef _WIN32
            if (r < 0 && errno == EINTR) continue;
#endif
            return false;
        }
        return true;
    }
};

struct FdControl {  // debug.configure control: read_byte() -> int (-1 at end)
    int fd = fd_env("ISPA_DEBUG_CONTROL_FD");
    int read_byte() {
        if (fd < 0) return -1;
        unsigned char c;
        for (;;) {
#ifdef _WIN32
            int r = ::_read(fd, &c, 1);
#else
            auto r = ::read(fd, &c, 1);
            if (r < 0 && errno == EINTR) continue;
#endif
            return r == 1 ? int(c) : -1;
        }
    }
};
} // namespace ispa_std_transport
