#include <mutex>        //! Provides std::mutex for thread synchronization, ensuring safe access to shared resources.
#include <cmath>        //? Used for mathematical functions like sin, cos, and M_PI in the CPU stress test.
#include <vector>       //? Provides std::vector, used for dynamically allocated memory blocks.
#include <thread>       //! Enables multithreading with std::thread.
#include <chrono>       //! Provides utilities for handling time, including measuring elapsed time and thread sleeping.
#include <atomic>       //! Provides std::atomic, enabling thread-safe access to shared counters and flags.
#include <memory>       //! Provides std::unique_ptr, used for managing dynamically allocated memory.
#include <cassert>      //! Provides assert(), used for runtime checks like ensuring system capabilities.
#include <iostream>     //? Provides input/output functionality, used for displaying progress and results.

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

        explicit Node(T&& data) : data(std::move(data)), next(nullptr) {}
        };

        Node* head;
        Node* tail;
        size_t size;

    public:
        LinkedList() : head(nullptr), tail(nullptr), size(0) {}

        ~LinkedList() {
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

        size_t getSize() const {
            return size;
        }
    };

    class Stack {
    private:
        struct Node {
            std::unique_ptr<std::vector<uint32_t>> block;
            Node* next;

            explicit Node(std::unique_ptr<std::vector<uint32_t>>&& blk) : block(std::move(blk)), next(nullptr) {}
        };

        Node* top;
        size_t size;

    public:
        Stack() : top(nullptr), size(0) {}

        ~Stack() {
        }

        void push(std::unique_ptr<std::vector<uint32_t>>&& block) {
            Node* newNode = new Node(std::move(block));
            newNode->next = top;
            top = newNode;
            ++size;
        }

        std::unique_ptr<std::vector<uint32_t>> pop() {
            if (!top) return nullptr;
            Node* temp = top;
            auto block = std::move(top->block);
            top = top->next;
            delete temp;
            --size;
            return block;
        }

        size_t getSize() const {
            return size;
        }
    };

    class Queue {
    private:
        struct Node {
            std::unique_ptr<std::vector<uint32_t>> block;
            Node* next;

            explicit Node(std::unique_ptr<std::vector<uint32_t>>&& blk) : block(std::move(blk)), next(nullptr) {}
        };

        Node *front, *rear;
        size_t size;

    public:
        Queue() : front(nullptr), rear(nullptr), size(0) {}

        ~Queue() {
        }

        void enqueue(std::unique_ptr<std::vector<uint32_t>>&& block) {
            Node* newNode = new Node(std::move(block));
            if (!rear) {
                front = rear = newNode;
            } else {
                rear->next = newNode;
                rear = newNode;
            }
            ++size;
        }

        std::unique_ptr<std::vector<uint32_t>> dequeue() {
            if (!front) return nullptr;
            Node* temp = front;
            auto block = std::move(front->block);
            front = front->next;
            if (!front) rear = nullptr;
            delete temp;
            --size;
            return block;
        }

        size_t getSize() const {
            return size;
        }
    };

class SystemStressTest {
private:
    static constexpr int BAR_WIDTH = 30;                        // Progress bar width for time and memory displays
    static constexpr int MULTIPLIER = 8;                        // Memory multiplier for stress test
    static constexpr int TEST_DURATION = 30;                    // seconds
    static constexpr size_t SMALL_BLOCK_SIZE = 256 * 1024;      // 256 KB blocks
    static constexpr size_t LARGE_BLOCK_SIZE = 1024 * 1024;     // 1 MB blocks
    static constexpr size_t TARGET_MEMORY = 1024 * 1024 * 1024; //? 1 GB Scaling bytes -> Mega -> Giga

    // Shared atomic variables to track system metrics
    std::atomic<bool> running{true};             // Flag to indicate if the test is running
    std::atomic<size_t> memoryAllocated{0};     // Memory allocated in bytes
    std::atomic<uint64_t> hashOps{0};   // Total Hashing operations

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
                  << hashOps.load(std::memory_order_relaxed)
                  << " ops" << std::flush;

    }

    //> CPU Stress test implementation with an extremely compute-intensive hash "SHA-256" simulation
    void cpuHashStressTest(int threadId) {
        constexpr int BATCH_SIZE = 1024 * 1024; // Large batch size for high compute intensity
        constexpr int CHUNK_SIZE = 1024;       // Smaller chunk size for frequent updates

        // Highly compute-intensive hash function using nested modular exponentiation
        auto computeIntensiveHash = [](uint64_t base, uint64_t exponent, uint64_t mod) -> uint64_t {
            uint64_t result = 1;
            uint64_t nestedFactor = 1;
            for (uint64_t i = 0; i < exponent; ++i) {
                result = (result * base) % mod;
                nestedFactor = (nestedFactor * result) % mod; // Nested computation to amplify cost
                if (i % 10 == 0) { // Periodic additional computation
                    result = (result + nestedFactor) % mod;
                }
            }
            return result;
        };

        uint64_t localHashOps = 0; // Local counter for hashing operations

        while (running) {
            volatile uint64_t hashValue = 0; // Intermediate computation result

            // Perform a batch of hashing operations
            for (int i = 0; i < BATCH_SIZE && running; ++i) {
                uint64_t randomBase = threadId * 123456789 + i * 987654321;  // Simulated complex random data
                uint64_t randomExponent = ((i % 2000) + 500) * (threadId % 10 + 1); // Larger arbitrary exponent
                uint64_t randomModulus = 1e9 + 12347;  // Larger prime modulus for increased complexity

                // Compute-intensive hashing with additional complexity
                hashValue = computeIntensiveHash(randomBase, randomExponent, randomModulus);

                // Simulate additional work on the hashValue to avoid compiler optimizations
                if (hashValue % 1024 == 0) {
                    hashValue = (hashValue + threadId) * (randomBase % 7);
                }

                ++localHashOps; // Increment local hash operation count

                // Update the shared counter after each chunk
                if (localHashOps % CHUNK_SIZE == 0) {
                    hashOps.fetch_add(CHUNK_SIZE, std::memory_order_relaxed);
                    localHashOps = 0;
                }
            }

            // Update any remaining operations in the local counter
            if (localHashOps > 0) {
                hashOps.fetch_add(localHashOps, std::memory_order_relaxed);
                localHashOps = 0;
            }
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
            << "Total hashing operations: " << hashOps.load(std::memory_order_relaxed) 
            << " ops" << ConsoleColors::RESET << std::endl;

    std::cout << ConsoleColors::CYAN
              << "Total execution time: " << duration.count() / 1000.0
              << " seconds" << ConsoleColors::RESET << std::endl;

    std::cout << ConsoleColors::CYAN
              << "Maximum memory allocated: " << memoryAllocated / (1024 * 1024)
              << "MB" << ConsoleColors::RESET << std::endl;

    std::cout << ConsoleColors::CYAN
              << "CPU cores utilized: " << numCores
              << ConsoleColors::RESET << std::endl;

  }

};

int main() {
    SystemStressTest test;
    test.run(); // Start the stress test
    return 0;
}
