#include <iostream>
#include <mutex>
#include <vector>
#include <thread>
#include <chrono>
#include <iomanip>
#include <atomic>
#include <cstring>
#include <locale>

// Cross-platform color and console support
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>

// Enable UTF-8 support and ANSI color processing
void enableWindowsConsole() {
    // Set console to UTF-8 mode
    SetConsoleOutputCP(CP_UTF8);
    _setmode(_fileno(stdout), _O_U16TEXT);

    // Enable ANSI color support
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}
#else
// No-op for non-Windows platforms
void enableWindowsConsole() {}
#endif

// Rest of your existing code remains the same...

int main() {
    // Enable Windows-specific console features
    enableWindowsConsole();

    // Your existing main function code...
}