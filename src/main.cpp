#include <list>         //! Used for managing a list of memory blocks in the memory stress test.
#include <mutex>        //! Provides std::mutex for thread synchronization, ensuring safe access to shared resources.
#include <cmath>        //? Used for mathematical functions like sin, cos, and M_PI in the CPU stress test.
#include <vector>       //? Provides std::vector, used for dynamically allocated memory blocks.
#include <thread>       //! Enables multithreading with std::thread.
#include <chrono>       //! Provides utilities for handling time, including measuring elapsed time and thread sleeping.
#include <atomic>       //! Provides std::atomic, enabling thread-safe access to shared counters and flags.
#include <memory>       //! Provides std::unique_ptr, used for managing dynamically allocated memory.
#include <iostream>     //? Provides input/output functionality, used for displaying progress and results.
#include <cassert>      //! Provides assert(), used for runtime checks like ensuring system capabilities.

/*
 * Platform-specific console initialization
 * Encapsulates all Windows-specific code in a dedicated namespace
 */
namespace ConsoleInitializer {
#ifdef _WIN32
    #include <io.h>         //> Windows-specific library for low-level I/O operations (used in console initialization).
    #include <fcntl.h>      //> Windows-specific library for file control operations (used in console initialization).
    #include <windows.h>    //> Windows-specific library for system calls like SetConsoleOutputCP and SetConsoleMode.

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
    void initialize() {} //* No-op for non-Windows platforms
#endif
}

/*
? ANSI color definitions moved to a dedicated namespace for better organization
! The declaration
> const char* const
? specifies a constant pointer to a constant character string.
! SECURITY REASONS THAT ARE UNNEGOTIABLE
*/

namespace ConsoleColors {
    const char* const RED = "\033[31m";
    const char* const BLUE = "\033[34m";
    const char* const CYAN = "\033[36m";
    const char* const RESET = "\033[0m";
    const char* const GREEN = "\033[32m";
    const char* const YELLOW = "\033[33m";
    const char* const MAGENTA = "\033[35m";
}

template<typename T>
class LinkedList {
private:
    struct Node {
        T data;
        Node* next;

        Node(T&& data) : data(std::move(data)), next(nullptr) {}
    };

    Node* head;
    Node* tail;
    size_t size;

public:
    LinkedList() : head(nullptr), tail(nullptr), size(0) {}

    ~LinkedList() {
        while (head) {
            Node* temp = head;
            head = head->next;
            delete temp;
        }
    }

    void push_back(T&& value) { // Accept rvalue reference
        Node* newNode = new Node(std::move(value));
        if (!head) {
            head = tail = newNode;
        } else {
            tail->next = newNode;
            tail = newNode;
        }
        ++size;
    }

    void clear() {
        while (head) {
            Node* temp = head;
            head = head->next;
            delete temp;
        }
        head = tail = nullptr;
        size = 0;
    }

    size_t getSize() const {
        return size;
    }
};



class SystemStressTest {
private:
    static constexpr int BAR_WIDTH = 30;     // Progress bar width for time and memory displays
    static constexpr int MULTIPLIER = 8;     // Memory multiplier for stress test
    static constexpr int TEST_DURATION = 30; // seconds
    static constexpr size_t TARGET_MEMORY = 1024 * 1024 * 1024; //? 1 GB Scaling bytes -> Mega -> Giga

    // Shared atomic variables to track system metrics
    std::atomic<bool> running{true};        // Flag to indicate if the test is running
    std::atomic<uint64_t> totalIntOps{0};   // Total integer operations
    std::atomic<uint64_t> totalFloatOps{0}; // Total floating-point operations
    std::atomic<size_t> memoryAllocated{0}; // Memory allocated in bytes

    std::mutex consoleMutex; // Protects console output from race conditions

    // Display helper methods
    void clearLine() const {
        std::cout << "\r\033[K"; // Clear the current console line
    }

    void moveCursor(int lines, bool up) const {
        std::cout << "\033[" << lines << (up ? 'A' : 'B'); // Move the cursor up or down
    }

    void displayMemoryStatus() const {
        float adjustedTargetMemory = TARGET_MEMORY * MULTIPLIER;
        float progress = static_cast<float>(memoryAllocated) / adjustedTargetMemory;
        int pos = static_cast<int>(BAR_WIDTH * progress);

        // Build progress bar for memory usage
        std::string progressBar = "Memory: [";
        for (int i = 0; i < BAR_WIDTH; ++i) {
            progressBar += (i < pos) ?
                std::string(ConsoleColors::GREEN) + "■" + ConsoleColors::RESET :
                "□";
        }

        // Display the progress bar and memory usage
        clearLine();
        std::cout << progressBar << "] "
                  << memoryAllocated / (1024 * 1024) << "MB / "
                  << adjustedTargetMemory / (1024 * 1024) << "MB" << std::flush;
    }

    void displayTimeProgress(int elapsedSeconds) const {
        float progress = static_cast<float>(elapsedSeconds) / TEST_DURATION;
        int pos = static_cast<int>(BAR_WIDTH * progress);

        // Build progress bar for elapsed time
        std::string progressBar = "Time:   [";
        for (int i = 0; i < BAR_WIDTH; ++i) {
            progressBar += (i < pos) ?
                std::string(ConsoleColors::CYAN) + "■" + ConsoleColors::RESET :
                "□";
        }

        // Display the progress bar and elapsed time
        clearLine();
        std::cout << progressBar << "] "
                  << elapsedSeconds << "s / "
                  << TEST_DURATION << "s" << std::flush;
    }

    void updateDisplay(int elapsedSeconds) {
        std::lock_guard<std::mutex> lock(consoleMutex); // Prevent concurrent console access

        clearLine();
        displayTimeProgress(elapsedSeconds);
        std::cout << std::endl;
        displayMemoryStatus();
        std::cout << std::endl;

        // Display integer and floating-point operation statistics
        std::cout << "HASH OPS: "
                  << totalIntOps.load(std::memory_order_relaxed)
                  << " ops" << std::flush;

    }

    //> CPU Stress test implementation with an extremely compute-intensive hash "SHA-256" simulation
    void cpuHashStressTest(int threadId) {
        volatile uint64_t hashOps = 0; // Local counter for hashing operations
        constexpr int BATCH_SIZE = 128; // Lower batch size for more computation per op

        // Compute-intensive hash function using modular exponentiation
        auto computeIntensiveHash = [](uint64_t base, uint64_t exponent, uint64_t mod) -> uint64_t {
            uint64_t result = 1;
            for (uint64_t i = 0; i < exponent; ++i) {
                result = (result * base) % mod;
            }
            return result;
        };

        while (running) {
            volatile uint64_t hashValue = 0; // Intermediate computation result

            // Perform a batch of hashing operations
            for (int i = 0; i < BATCH_SIZE && running; ++i) {
                uint64_t randomBase = threadId * 12345 + i;  // Simulated random data
                uint64_t randomExponent = (i % 1000) + 500;  // Arbitrary exponent
                uint64_t randomModulus = 1e9 + 7;            // Large prime modulus

                // Compute-intensive hashing
                hashValue = computeIntensiveHash(randomBase, randomExponent, randomModulus);

                // Simulate work on the hashValue to avoid compiler optimizations
                if (hashValue % 1024 == 0) {
                    hashValue += threadId;
                }

                hashOps += 1; // Increment hash operation count
            }

            // Update shared atomic counters after each batch
            totalIntOps.fetch_add(hashOps, std::memory_order_relaxed);

            // Reset local counter
            hashOps = 0;

            // No sleep to maximize CPU stress
        }
    }

    // Memory Stress test
    void memoryStressTest() {
        LinkedList<std::unique_ptr<std::vector<int>>> memoryBlocks;

        try {
            while (running && memoryAllocated < MULTIPLIER * TARGET_MEMORY) {
                static constexpr size_t blockSize = 1024 * 1024;

                auto block = std::make_unique<std::vector<int>>(
                    blockSize / sizeof(int), 1 // Fill block with dummy data
                );

                memoryAllocated += blockSize;
                memoryBlocks.push_back(std::move(block)); // Transfer ownership
            }
        } catch (const std::bad_alloc& e) {
            std::lock_guard<std::mutex> lock(consoleMutex);
            std::cout << "\n" << ConsoleColors::RED
                    << "Memory allocation failed: " << e.what()
                    << ConsoleColors::RESET << std::endl;
        }

        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

public:
  void run() {
    ConsoleInitializer::initialize(); // Platform-specific console initialization

    std::cout << ConsoleColors::MAGENTA
              << "\n=== System Stress Test Starting ==="
              << ConsoleColors::RESET << std::endl;

    std::cout << ConsoleColors::YELLOW
              << "Warning: This program will stress your system for "
              << TEST_DURATION << " seconds."
              << ConsoleColors::RESET << std::endl;

    std::cout << "Press Enter to continue...";
    std::cin.get();

    const auto numCores = std::thread::hardware_concurrency(); // Detect CPU cores
    assert(numCores > 0 && "Failed to detect CPU cores");

    std::cout << ConsoleColors::BLUE << "\nDetected "
              << numCores << " CPU cores"
              << ConsoleColors::RESET << std::endl;

    std::cout << "\nStarting stress test...\n\n" << std::flush;

    auto startTime = std::chrono::steady_clock::now();

    // Launch CPU stress threads
    std::vector<std::thread> cpuThreads;
    for (unsigned int i = 0; i < numCores; ++i) {
        cpuThreads.emplace_back(&SystemStressTest::cpuHashStressTest, this, i); // Launch hashing threads
    }

    // Launch memory stress thread
    std::thread memThread(&SystemStressTest::memoryStressTest, this);

    // Main monitoring loop
    int elapsedSeconds = 0;
    while (elapsedSeconds <= TEST_DURATION) {
        auto elapsedTime = std::chrono::steady_clock::now() - startTime;
        elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(elapsedTime).count();

        updateDisplay(elapsedSeconds);
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        moveCursor(2, true); // Move cursor up to overwrite previous output
    }

    running = false; // Signal threads to stop

    // Wait for all threads to finish
    for (auto& thread : cpuThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    if (memThread.joinable()) {
        memThread.join();
    }

    // Display test results
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    std::cout << std::endl;

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
            << "Total hashing operations: " << totalIntOps.load(std::memory_order_relaxed) 
            << " ops" << ConsoleColors::RESET << std::endl;

  }

};

int main() {
    SystemStressTest test;
    test.run(); // Start the stress test
    return 0;
}
