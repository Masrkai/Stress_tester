#pragma once

/*
 * ANSI color definitions are encapsulated in a dedicated namespace
 * for improved organization and reusability across the codebase.
 * Each color is defined as a `constexpr const char*`, ensuring
 * compile-time evaluation and immutability for safety and performance.
 */
namespace ConsoleColors {
    constexpr const char* RED = "\033[31m";       // ANSI escape code for red text
    constexpr const char* BLUE = "\033[34m";      // ANSI escape code for blue text
    constexpr const char* CYAN = "\033[36m";      // ANSI escape code for cyan text
    constexpr const char* RESET = "\033[0m";      // Resets all text formatting to default
    constexpr const char* GREEN = "\033[32m";     // ANSI escape code for green text
    constexpr const char* YELLOW = "\033[33m";    // ANSI escape code for yellow text
    constexpr const char* MAGENTA = "\033[35m";   // ANSI escape code for magenta text
}