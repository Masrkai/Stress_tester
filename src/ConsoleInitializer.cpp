#include "../include/ConsoleInitializer.h"
#include <cassert>

namespace ConsoleInitializer {
    #ifdef _WIN32
        #include <io.h>         // Windows-specific library for low-level I/O operations
        #include <fcntl.h>      // Windows-specific library for file control operations
        #include <windows.h>    // Windows-specific library for system calls

        void initialize() {
            SetConsoleOutputCP(CP_UTF8);
            _setmode(_fileno(stdout), _O_U16TEXT);

            auto consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD consoleMode = 0;
            assert(GetConsoleMode(consoleHandle, &consoleMode) && "Failed to get console mode");

            consoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            assert(SetConsoleMode(consoleHandle, consoleMode) && "Failed to set console mode");
        }
    #else
        void initialize() {} // No-op for non-Windows platforms
    #endif
}