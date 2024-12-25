#include <list>
#include <mutex>
#include <cmath>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <memory>
#include <iostream>
#include <assert.h>

 /*
 * Platform-specific console initialization
 * Encapsulates all Windows-specific code in a dedicated namespace
 */

namespace ConsoleInitializer {
    #ifdef _WIN32
        #include <io.h>
        #include <fcntl.h>
        #include <windows.h>

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

// ANSI color definitions moved to a dedicated namespace for better organization
namespace ConsoleColors {
    const char* const RED = "\033[31m";
    const char* const BLUE = "\033[34m";
    const char* const CYAN = "\033[36m";
    const char* const RESET = "\033[0m";
    const char* const GREEN = "\033[32m";
    const char* const YELLOW = "\033[33m";
    const char* const MAGENTA = "\033[35m";
}

class SystemStressTest {
 private:
    static constexpr size_t TARGET_MEMORY = 1024 * 1024 * 1024; // 1 GB
    static constexpr int TEST_DURATION = 30; // seconds
    static constexpr int Multiplyier = 8;   //TODO: User Input

    std::atomic<uint64_t> totalIntOps{0};
    std::atomic<uint64_t> totalFloatOps{0};
    std::atomic<bool> running{true};
    std::atomic<size_t> memoryAllocated{0};
    std::mutex consoleMutex;

    // Display helper methods
    void clearLine() const {
        std::cout << "\r\033[K";
    }

    void moveCursor(int lines, bool up) const {
        std::cout << "\033[" << lines << (up ? 'A' : 'B');
    }

    void displayMemoryStatus() const {
        static constexpr int barWidth = 30;

        // Use Multiplyier to adjust target memory for progress calculation
        float adjustedTargetMemory = TARGET_MEMORY * Multiplyier;
        float progress = static_cast<float>(memoryAllocated) / adjustedTargetMemory;
        int pos = static_cast<int>(barWidth * progress);

        // Build progress bar
        std::string progressBar = "Memory: [";
        for (int i = 0; i < barWidth; ++i) {
            progressBar += (i < pos) ?
                std::string(ConsoleColors::GREEN) + "■" + ConsoleColors::RESET :
                "□";
        }

        // Clear the current console line and print the memory status
        clearLine();
        std::cout << progressBar << "] "
                  << memoryAllocated / (1024 * 1024) << "MB / "
                  << adjustedTargetMemory / (1024 * 1024) << "MB" << std::flush;
    }

    void displayTimeProgress(int elapsedSeconds) const {
        static constexpr int barWidth = 30;
        float progress = static_cast<float>(elapsedSeconds) / TEST_DURATION;
        int pos = static_cast<int>(barWidth * progress);

        std::string progressBar = "Time:   [";
        for (int i = 0; i < barWidth; ++i) {
            progressBar += (i < pos) ?
                std::string(ConsoleColors::CYAN) + "■" + ConsoleColors::RESET :
                "□";
        }

        clearLine();
        std::cout << progressBar << "] "
                  << elapsedSeconds << "s / "
                  << TEST_DURATION << "s" << std::flush;
    }

    void updateDisplay(int elapsedSeconds) {
        std::lock_guard<std::mutex> lock(consoleMutex);
        clearLine();
        displayTimeProgress(elapsedSeconds);
        std::cout << std::endl;
        displayMemoryStatus();
        std::cout << std::endl;

        // Display integer and floating-point operation statistics
        std::cout << "INT:   "    << totalIntOps.load(std::memory_order_relaxed) << " ops"
                  << " | FLOAT: " << totalFloatOps.load(std::memory_order_relaxed) << " ops"
                  << std::flush;
    }


    //> CPU Stress test implementation
    void cpuStressTest(int threadId) {
        // Local counters for operations
        volatile uint64_t intOps = 0;
        volatile uint64_t floatOps = 0;

        // Constants to define the number of operations per loop iteration
        constexpr int OPERATIONS_PER_ITERATION = 1024;

        while (running) {
            volatile double result = 1.0;  // Initialize result for each batch

            for (int i = 0; i < OPERATIONS_PER_ITERATION && running; ++i) {
                // Perform a series of operations to stress the CPU
                result *= M_PI;
                result += std::sin(result);
                result /= std::cos(result);

                // Increment operation counters
                floatOps += 3;  // Three floating-point operations per iteration
                intOps += 1;    // One integer increment operation per iteration

                // Prevent over-optimization by the compiler
                if (result > 1e308) {
                    result = 1.0;
                }
            }

            // Update atomic counters with local counts
            totalIntOps.fetch_add(intOps, std::memory_order_relaxed);
            totalFloatOps.fetch_add(floatOps, std::memory_order_relaxed);

            // Reset local counters for the next batch
            intOps = 0;
            floatOps = 0;

            // Sleep briefly to avoid busy-waiting and allow other threads to run
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    }


    //> USES LINKED LISTS
    void memoryStressTest() {
        // Using smart pointers for automatic memory management
        std::list<std::unique_ptr<std::vector<int>>> memoryBlocks;

        try {
            while (running && memoryAllocated < Multiplyier * TARGET_MEMORY) {
                static constexpr size_t blockSize = 1024 * 1024; // 1 MB blocks
                auto block = std::make_unique<std::vector<int>>(
                    blockSize / sizeof(int), 1
                );

                memoryAllocated += blockSize;
                memoryBlocks.push_back(std::move(block));
            }
        } catch (const std::bad_alloc& e) {
            std::lock_guard<std::mutex> lock(consoleMutex);
            std::cout << "\n" << ConsoleColors::RED
                    << "Memory allocation failed: " << e.what()
                    << ConsoleColors::RESET << std::endl;
        }

        // Keep memory allocated until test ends
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }


 public:
    void run() {
        ConsoleInitializer::initialize();

        std::cout << ConsoleColors::MAGENTA
                  << "\n=== System Stress Test Starting ==="
                  << ConsoleColors::RESET << std::endl;

        std::cout << ConsoleColors::YELLOW
                  << "Warning: This program will stress your system for "
                  << TEST_DURATION << " seconds."
                  << ConsoleColors::RESET << std::endl;

        std::cout << "Press Enter to continue...";
        std::cin.get();

        const auto numCores = std::thread::hardware_concurrency();
        assert(numCores > 0 && "Failed to detect CPU cores");

        std::cout << ConsoleColors::BLUE << "\nDetected "
                  << numCores << " CPU cores"
                  << ConsoleColors::RESET << std::endl;

        std::cout << "\nStarting stress test...\n\n" << std::flush;

        auto startTime = std::chrono::steady_clock::now();

        // Launch CPU stress threads
        std::vector<std::thread> cpuThreads;
        for (unsigned int i = 0; i < numCores; ++i) {
            cpuThreads.emplace_back(&SystemStressTest::cpuStressTest, this, i);
        }

        // Launch memory stress thread
        std::thread memThread(&SystemStressTest::memoryStressTest, this);

        // Main monitoring loop
        while (running) {
            auto elapsedTime = std::chrono::steady_clock::now() - startTime;
            int elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(
                elapsedTime
            ).count();

            if (elapsedSeconds > TEST_DURATION) break;

            updateDisplay(elapsedSeconds);
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            moveCursor(2, true);
        }

        std::cout << std::endl;
        running = false;

        // Clean up threads
        for (auto& thread : cpuThreads) {
            thread.join();
        }
        memThread.join();

        // Display results
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime
        );

        std::cout << "\n\n" << ConsoleColors::MAGENTA
                  << "=== Test Results ==="
                  << ConsoleColors::RESET << std::endl;

        std::cout << ConsoleColors::CYAN
                  << "Total execution time: " << duration.count() / 1000.0
                  << " seconds" << ConsoleColors::RESET << std::endl;

        std::cout << ConsoleColors::CYAN
                  << "Maximum memory allocated: " << memoryAllocated / (1024 * 1024)
                  << "MB" << ConsoleColors::RESET << std::endl;

        std::cout << ConsoleColors::CYAN
                  << "CPU cores utilized: " << numCores
                  << ConsoleColors::RESET << std::endl;

        std::cout << ConsoleColors::CYAN
                  << "Total integer operations: " << totalIntOps.load(std::memory_order_relaxed)
                  << ConsoleColors::RESET << std::endl;

        std::cout << ConsoleColors::CYAN
                  << "Total floating-point operations: " << totalFloatOps.load(std::memory_order_relaxed)
                  << ConsoleColors::RESET << std::endl;

    }
};

int main() {
    SystemStressTest test;
    test.run();
    return 0;
}