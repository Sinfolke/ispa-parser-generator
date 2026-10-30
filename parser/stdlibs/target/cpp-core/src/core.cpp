// stdlibs/new/src/ispastdlib.cpp
#include <ispastdlib/core.hpp>
#include <sstream>

#include <ispastdlib/platform.hpp>

namespace ISPA_STD {

    std::string capture_stacktrace() {
        std::ostringstream out;

#ifdef _WIN32
        constexpr USHORT max_frames = 64;
        void* frames[max_frames]{};

        const USHORT count = CaptureStackBackTrace(0, max_frames, frames, nullptr);
        HANDLE process = GetCurrentProcess();

        if (!SymInitialize(process, nullptr, TRUE))
            return "Symbol initialization failed";

        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);

        alignas(SYMBOL_INFO) char buffer[sizeof(SYMBOL_INFO) + 1024]{};
        auto* symbol = reinterpret_cast<SYMBOL_INFO*>(buffer);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = 1024;

        for (USHORT i = 0; i < count; ++i) {
            DWORD64 address = reinterpret_cast<DWORD64>(frames[i]);
            DWORD64 displacement = 0;

            out << "  #" << i << ' ';
            if (SymFromAddr(process, address, &displacement, symbol)) {
                out << symbol->Name << " +0x" << std::hex << displacement << std::dec;
            } else {
                out << frames[i];
            }
            out << '\n';
        }

        SymCleanup(process);
#else
        void* frames[64];
        int count = backtrace(frames, 64);
        char** symbols = backtrace_symbols(frames, count);

        if (!symbols)
            return "Stack trace unavailable";

        for (int i = 0; i < count; ++i)
            out << "  #" << i << ' ' << symbols[i] << '\n';

        std::free(symbols);
#endif

        return out.str();
    }

} // namespace ISPA_STD
