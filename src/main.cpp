#include <mutex>        //! Provides std::mutex for thread synchronization, ensuring safe access to shared resources.
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


    // Template class for a LinkedList
    template <typename T>
    class LinkedList {
    private:
    // Internal structure representing a node in the LinkedList
    struct Node {
        T data;       // Data stored in the node
        Node *next;   // Pointer to the next node

        // Constructor to initialize a node with given data (move semantics used for efficiency)
        explicit Node(T &&data) : data(std::move(data)), next(nullptr) {}
    };

    Node *head;     // Pointer to the first node in the LinkedList
    Node *tail;     // Pointer to the last node in the LinkedList
    size_t size;    // Tracks the number of nodes in the LinkedList

    public:
    // Constructor: Initializes an empty LinkedList
    LinkedList() : head(nullptr), tail(nullptr), size(0) {}

    // Destructor: Clean up resources (not implemented here, relying on T to manage its own memory)
    ~LinkedList() {}

    // Adds a new element to the end of the LinkedList using move semantics
    void push_back(T &&value) {
        Node *newNode = new Node(std::move(value)); // Create a new node with the given value
        if (!head) { // If the list is empty, initialize both head and tail to the new node
        head = tail = newNode;
        } else { // Otherwise, append the new node to the end of the list
        tail->next = newNode;
        tail = newNode;
        }
        ++size; // Increment the size of the list
    }

    // Returns the current size of the LinkedList
    size_t getSize() const { return size; }
    };


class SystemStressTest {
private:
    static constexpr int BAR_WIDTH = 30;                        // Progress bar width for time and memory displays
    static constexpr int MULTIPLIER = 15;                        // Memory multiplier for stress test
    static constexpr int TEST_DURATION = 30;                    // seconds
    static constexpr size_t TARGET_MEMORY = 1024 * 1024 * 1024; //? 1 GB Scaling bytes -> Mega -> Giga

    // Shared atomic variables to track system metrics
    std::atomic<bool> running{true};             // Flag to indicate if the test is running
    std::atomic<size_t> memoryAllocated{0};     // Memory allocated in bytes
    std::atomic<uint64_t> hashOps{0};          // Total Hashing operations

    std::mutex consoleMutex; // Protects console output from race conditions

    // Display helper methods
    void clearLine() const {
        std::cout << "\r\033[K"; // Clear the current console line
    }

    void moveCursor(int lines, bool up) const {
        std::cout << "\033[" << lines << (up ? 'A' : 'B'); // Move the cursor up or down
    }

    void displayMemoryStatus() const {
        // [O(1)] Calculate adjusted target memory based on the multiplier.
        float adjustedTargetMemory = TARGET_MEMORY * MULTIPLIER;

        // [O(1)] Calculate memory usage progress as a percentage (0.0 to 1.0).
        float progress = static_cast<float>(memoryAllocated) / adjustedTargetMemory;

        // [O(1)] Calculate the number of filled positions in the progress bar.
        int pos = static_cast<int>(BAR_WIDTH * progress);

        // [O(BAR_WIDTH)] Build the memory usage progress bar.
        std::string progressBar = "Memory: [";
        for (int i = 0; i < BAR_WIDTH; ++i) { // Iterate over BAR_WIDTH positions
            progressBar += (i < pos) ? // Add filled or empty bar segments
                std::string(ConsoleColors::GREEN) + "■" + ConsoleColors::RESET :
                "□";
        }

        // [O(1)] Clear the current console line to prepare for new output.
        clearLine();

        // [O(1)] Display the memory progress bar and memory usage details.
        std::cout << progressBar << "] "
                  << memoryAllocated / (1024 * 1024) << "MB / " // Convert bytes to MB
                  << adjustedTargetMemory / (1024 * 1024) << "MB" << std::flush;
    }

    void displayTimeProgress(int elapsedSeconds) const {
        // Clamp elapsedSeconds to not exceed TEST_DURATION.
        elapsedSeconds = std::min(elapsedSeconds, TEST_DURATION);

        // Calculate the progress as a fraction of the total test duration.
        float progress = static_cast<float>(elapsedSeconds) / TEST_DURATION;

        // Map the progress fraction to a position on the progress bar.
        int pos = static_cast<int>(BAR_WIDTH * progress);

        // Initialize the progress bar string with a label "Time:   [".
        std::string progressBar = "Time:   [";

        // Build the visual representation of the progress bar.
        for (int i = 0; i < BAR_WIDTH; ++i) {
            if (i < pos) {
                // Add a filled segment (■) for positions less than `pos`.
                progressBar += std::string(ConsoleColors::CYAN) + "■" + ConsoleColors::RESET;
            } else {
                // Add an empty segment (□) for positions greater than or equal to `pos`.
                progressBar += "□";
            }
        }

        // Clear the current console line (to overwrite the previous progress bar).
        clearLine();

        // Output the progress bar, current elapsed time, and total test duration.
        std::cout << progressBar << "] "
                << elapsedSeconds << "s / "
                << TEST_DURATION << "s" << std::flush;
    }



    void updateDisplay(int elapsedSeconds) {
        // [O(1)] Lock the console to prevent concurrent access by multiple threads.
        std::lock_guard<std::mutex> lock(consoleMutex); // Prevent concurrent console access

        // [O(1)] Clear the current console line to prepare for updated output.
        clearLine();

        // [O(1)] Display the elapsed time in seconds as part of the progress.
        displayTimeProgress(elapsedSeconds);

        // [O(1)] Output a newline to separate sections.
        std::cout << std::endl;

        // [O(1)] Display current memory usage status.
        displayMemoryStatus();

        // [O(1)] Output a newline for separation.
        std::cout << std::endl;

        // [O(1)] Display the total number of hash operations performed.
        // `hashOps.load()` is a single atomic operation, which is O(1).
        std::cout << "HASH OPS: "
                  << hashOps.load(std::memory_order_relaxed) // Fetch current hash operations count atomically
                  << " ops" << std::flush;
    }



    // ============================================================================================
    // CPU STRESS TEST FUNCTION
    // ============================================================================================
    // VISUAL REPRESENTATION OF THE OPERATION SEQUENCE:
    // +--------------------------+
    // | Define Compute-Intensive |
    // | Hash Function (Helper)   |
    // +--------------------------+
    //            |
    //            v
    // +---------------------------+
    // | While `running` is true:  |
    // +---------------------------+
    //            |
    //            v
    // +---------------------------------------+
    // | Process BATCH_SIZE Hash Operations:   |
    // | - Generate pseudo-random inputs       |
    // | - Compute hash value                  |
    // | - Increment operation counters        |
    // | - Update shared counter periodically  |
    // +---------------------------------------+
    //            |
    //            v
    // +---------------------------+
    // | If not running: Exit loop |
    // +---------------------------+
    //            |
    //            v
    // +----------------------------------------+
    // | Update any remaining local operations  |
    // | to shared counter before exiting       |
    // +----------------------------------------+
    //            |
    //            v
    // +------------------+
    // | End: Function    |
    // +------------------+
    //
    // ============================================================================================


    // Function to simulate a compute-intensive CPU stress test using a custom hash-like operation.
    // [O(1)] This function is designed to run on a specific thread and perform a large number of operations
    // [O(1)] involving modular exponentiation and nested computation to simulate high CPU load.

    void cpuHashStressTest(int threadId) {
        constexpr int BATCH_SIZE = 1024; // [O(1)] Total number of hash operations in a batch.
        constexpr int CHUNK_SIZE = 1;       // [O(1)] Number of operations after which shared counter is updated.


        //! 1. MODULAR EXPONENTIATION: CORE OF PUBLIC-KEY CRYPTOGRAPHY
        // Define a compute-intensive hash-like function that uses nested modular exponentiation.
        // [O(exponent)] Time complexity depends on the value of 'exponent' due to the nested loop.
        auto computeIntensiveHash = [](  uint64_t  base,  uint64_t exponent,  uint64_t mod) -> uint64_t {
            uint64_t result = 1;       // [O(1)] Result of modular exponentiation
            uint64_t nestedFactor = 1; // [O(1)] Additional factor to amplify computation complexity

            for (uint64_t i = 0; i < exponent; ++i) { // [O(exponent)] Outer loop runs 'exponent' times
                result = (result * base) % mod;                 // [O(1)] Base modular exponentiation
                nestedFactor = (nestedFactor * result) % mod;   // [O(1)] Nested computation step

                for (uint64_t j = 0; j < exponent; ++j) {       // [O(exponent)] Inner loop runs 'exponent' times
                    nestedFactor += i + j;                      // Add nestedFactor with the current iteration values.
                    result *= nestedFactor;                     // Multiply result by nestedFactor
                }

                if (i % 10 == 0) { // [O(1)] Condition check occurs on every iteration
                    result = (result + nestedFactor) % mod;     // [O(1)] Add nested result periodically
                }
            }
            return result; // [O(1)] Return the final computed value
        };

        uint64_t localHashOps = 0; // [O(1)] Local count of hash operations performed by this thread.

        // 3. PSEUDO-RANDOM INPUT
        // Main loop for stress testing
        while (running) { // [O(∞)] Runs indefinitely unless 'running' is set to false externally
            volatile uint64_t hashValue = 0; // [O(1)] Temporary variable to store intermediate hash results.

            // Perform a batch of hash operations.
            for (int i = 0; i < BATCH_SIZE && running; ++i) { // [O(BATCH_SIZE)] Outer loop runs BATCH_SIZE times
                // Generate pseudo-random input values for hashing.
                volatile uint64_t randomBase = threadId * 123456789 + i * 987654321;  // [O(1)]
                volatile uint64_t randomExponent = ((i % 2000) + 500) * (threadId % 10 + 1); // [O(1)]
                volatile uint64_t randomModulus = 1e9 + 12347; // [O(1)]


                // 4. NESTED COMPUTATION AND HASHING
                // Compute the hash-like value with the custom function.
                hashValue = computeIntensiveHash(randomBase, randomExponent, randomModulus); // [O(randomExponent)]

                // Additional operation to avoid compiler optimizations on hashValue.
                if (hashValue % 1024 == 0) { // [O(1)] Condition check and operation
                    hashValue = (hashValue + threadId) * (randomBase % 7); // [O(1)]
                }

                ++localHashOps; // [O(1)] Increment local hash operation counter.

                // Update shared counter periodically to reduce contention.
                if (localHashOps % CHUNK_SIZE == 0) { // [O(1)] Condition check every CHUNK_SIZE iterations
                    hashOps.fetch_add(CHUNK_SIZE, std::memory_order_relaxed); // [O(1)] Atomic counter update
                    localHashOps = 0; // [O(1)] Reset local counter.
                }
            }

            // Add any remaining operations in the local counter to the shared counter.
            if (localHashOps > 0) { // [O(1)] Condition check at the end of the batch
                hashOps.fetch_add(localHashOps, std::memory_order_relaxed); // [O(1)] Atomic counter update
                localHashOps = 0; // [O(1)] Reset local counter.
            }
        }
    }


/*

makes something like this:

+-------------------+     +-------------------+     +-------------------+
| std::vector<int>  | --> | std::vector<int>  | --> | std::vector<int>  | --> NULL
| {1, 1, 1, ...}    |     | {1, 1, 1, ...}    |     | {1, 1, 1, ...}    |
+-------------------+     +-------------------+     +-------------------+

*/
    // Function to stress test memory allocation
    void memoryStressTest() {
        // Linked list to store memory blocks (using unique_ptr for automatic memory management)
        LinkedList<std::unique_ptr<std::vector<int>>> memoryBlocks;

        try {
            // Loop to allocate memory until the target threshold is reached or the test is stopped
            while (running && memoryAllocated < MULTIPLIER * TARGET_MEMORY) {
                static constexpr size_t blockSize = 1024 * 1024; // Block size of 1 MB

                // Create a unique pointer to a dynamically allocated vector of integers.
                // The vector will simulate a block of memory for operations like memory testing or simulation.
                auto block = std::make_unique<std::vector<int>>(
                    blockSize / sizeof(int),  // The size of the vector is determined by dividing blockSize by sizeof(int).
                                                // This calculates how many integers can fit into the given block size.
                                                // Example: If blockSize is 1024 bytes and sizeof(int) is 4 bytes,
                                                // the vector will have 1024 / 4 = 256 elements.

                    1                // The initial value of each element in the vector.
                                           // Here, all elements of the vector are initialized to the value 1.
                                           // This ensures the block is filled with a known value, which may
                                           // be useful for certain simulations or tests.
                );

                // Update the total allocated memory counter
                memoryAllocated += blockSize;

                // Store the allocated block in the linked list to maintain ownership and prevent deallocation
                memoryBlocks.push_back(std::move(block));
            }
        } catch (const std::bad_alloc &e) {
            // Handle memory allocation failure
            std::lock_guard<std::mutex> lock(consoleMutex); // Ensure thread-safe console output
            std::cout << "\n"
                    << ConsoleColors::RED
                    << "Memory allocation failed: " << e.what()
                    << ConsoleColors::RESET << std::endl;
        }
    }

public:
    void run() {
        // Initialize the console (platform-specific setup, e.g., enable colored output on Windows)
        ConsoleInitializer::initialize();

        // Display the start banner for the stress test
        std::cout << ConsoleColors::MAGENTA
                << "\n=== System Stress Test Starting ==="
                << ConsoleColors::RESET << std::endl;

        // Display a warning message about the test duration
        std::cout << ConsoleColors::YELLOW
                << "Warning: This program will stress your system for "
                << TEST_DURATION << " seconds."
                << ConsoleColors::RESET << std::endl;

        // Prompt the user to continue
        std::cout << "Press Enter to continue...";
        std::cin.get(); // Wait for user input

        // Detect the number of CPU cores available on the system
        const auto numCores = std::thread::hardware_concurrency();
        assert(numCores > 0 && "Failed to detect CPU cores"); // Ensure the number of cores is valid

        // Display the number of detected CPU cores
        std::cout << ConsoleColors::BLUE << "\nDetected "
                << numCores << " CPU cores"
                << ConsoleColors::RESET << std::endl;

        // Inform the user that the stress test is starting
        std::cout << "\nStarting stress test...\n\n" << std::flush;

        // Record the starting time of the test
        auto startTime = std::chrono::steady_clock::now();

        // ===================================================================
        // CPU STRESS TEST SETUP
        // ===================================================================
        // Launch a thread for each CPU core to perform the CPU stress test
        std::vector<std::thread> cpuThreads;
        for (unsigned int i = 0; i < numCores; ++i) {
            cpuThreads.emplace_back(&SystemStressTest::cpuHashStressTest, this, i); // Launch CPU hashing threads
        }

        // Launch a separate thread for memory stress testing
        std::thread memThread(&SystemStressTest::memoryStressTest, this);

        // ===================================================================
        // MONITORING LOOP
        // ===================================================================
        int elapsedSeconds = 0;
        while (elapsedSeconds <= TEST_DURATION) { // Run the loop until the test duration is reached
            // Calculate the elapsed time since the start
            auto elapsedTime = std::chrono::steady_clock::now() - startTime;
            elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(elapsedTime).count();

            // Update the console display with the current progress
            updateDisplay(elapsedSeconds);

            // Sleep briefly to reduce the frequency of updates
            std::this_thread::sleep_for(std::chrono::milliseconds(250));

            // Move the cursor up to overwrite the previous output in the console
            moveCursor(2, true);
        }

        // Signal all threads to stop their work
        running = false;

        // ===================================================================
        // THREAD CLEANUP
        // ===================================================================
        // Wait for all CPU threads to complete
        for (auto& thread : cpuThreads) {
            if (thread.joinable()) { // Ensure the thread is joinable before joining
                thread.join();
            }
        }

        // Wait for the memory stress test thread to complete
        if (memThread.joinable()) {
            memThread.join();
        }

        // ===================================================================
        // DISPLAY TEST RESULTS
        // ===================================================================
        // Calculate the total duration of the test
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        // Print an empty line for spacing
        std::cout << std::endl;

        // Display the test results banner
        std::cout << "\n\n" << ConsoleColors::MAGENTA
                << "=== Test Results ==="
                << ConsoleColors::RESET << std::endl;

        // Display the total number of hashing operations performed
        std::cout << ConsoleColors::CYAN
                << "Total hashing operations: " << hashOps.load(std::memory_order_relaxed) 
                << " ops" << ConsoleColors::RESET << std::endl;

        // Display the total execution time in seconds
        std::cout << ConsoleColors::CYAN
                << "Total execution time: " << duration.count() / 1000.0
                << " seconds" << ConsoleColors::RESET << std::endl;

        // Display the maximum amount of memory allocated during the test
        std::cout << ConsoleColors::CYAN
                << "Maximum memory allocated: " << memoryAllocated / (1024 * 1024)
                << "MB" << ConsoleColors::RESET << std::endl;

        // Display the number of CPU cores utilized
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
