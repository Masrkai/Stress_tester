#include "SystemStressTest.h"
#include "ConsoleInitializer.h"
#include "ConsoleColors.h"
#include "LinkedList.h"

#include <chrono>
#include <memory>
#include <cassert>
#include <iostream>
#include <algorithm>

// Helper to get current system CPU load (simplified version)
float SystemStressTest::getCurrentSystemLoad() {
    // This is a placeholder implementation
    // In a real implementation, you would:
    // 1. Sample CPU usage over a short period
    // 2. Compare previous and current measurements
    // 3. Return a value between 0.0 and 1.0

    // For now, return a value based on hash operations rate
    static uint64_t lastOps = 0;
    static auto lastCheck = std::chrono::steady_clock::now();
    auto currentTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastCheck).count();

    if (duration == 0) return 0.5f;
    uint64_t currentOps = hashOps.load(std::memory_order_relaxed);
    float opsRate = static_cast<float>(currentOps - lastOps) / duration;

    lastOps = currentOps;
    lastCheck = currentTime;

    // Normalize the rate to a value between 0.0 and 1.0
    return std::min(1.0f, std::max(0.0f, opsRate / 1000.0f));
}

// Display helper methods
void SystemStressTest::clearLine() const {
    std::cout << "\r\033[K"; // Clear the current console line
}

void SystemStressTest::moveCursor(int lines, bool up) const {
    std::cout << "\033[" << lines << (up ? 'A' : 'B'); // Move the cursor up or down
}

void SystemStressTest::displayMemoryStatus() const {
    // Calculate adjusted target memory based on the multiplier
    float adjustedTargetMemory = TARGET_MEMORY * MULTIPLIER;

    // Calculate memory usage progress as a percentage (0.0 to 1.0)
    float progress = static_cast<float>(memoryAllocated) / adjustedTargetMemory;

    // Calculate the number of filled positions in the progress bar
    int pos = static_cast<int>(BAR_WIDTH * progress);

    // Build the memory usage progress bar
    std::string progressBar = "Memory: [";
    for (int i = 0; i < BAR_WIDTH; ++i) {
        progressBar += (i < pos) ? 
            std::string(ConsoleColors::GREEN) + "■" + ConsoleColors::RESET :
            "□";
    }

    // Clear the current console line to prepare for new output
    clearLine();

    // Display the memory progress bar and memory usage details
    std::cout << progressBar << "] "
              << memoryAllocated / (1024 * 1024) << "MB / "
              << adjustedTargetMemory / (1024 * 1024) << "MB" << std::flush;
}

void SystemStressTest::displayTimeProgress(int elapsedSeconds) const {
    // Clamp elapsedSeconds to not exceed TEST_DURATION
    elapsedSeconds = std::min(elapsedSeconds, TEST_DURATION);

    // Calculate the progress as a fraction of the total test duration
    float progress = static_cast<float>(elapsedSeconds) / TEST_DURATION;

    // Map the progress fraction to a position on the progress bar
    int pos = static_cast<int>(BAR_WIDTH * progress);

    // Initialize the progress bar string with a label
    std::string progressBar = "Time:   [";

    // Build the visual representation of the progress bar
    for (int i = 0; i < BAR_WIDTH; ++i) {
        if (i < pos) {
            progressBar += std::string(ConsoleColors::CYAN) + "■" + ConsoleColors::RESET;
        } else {
            progressBar += "□";
        }
    }

    // Clear the current console line
    clearLine();

    // Output the progress bar, current elapsed time, and total test duration
    std::cout << progressBar << "] "
              << elapsedSeconds << "s / "
              << TEST_DURATION << "s" << std::flush;
}

void SystemStressTest::updateDisplay(int elapsedSeconds) {
    // Lock the console to prevent concurrent access by multiple threads
    std::lock_guard<std::mutex> lock(consoleMutex);

    // Clear the current console line
    clearLine();

    // Display the elapsed time
    displayTimeProgress(elapsedSeconds);

    // Output a newline to separate sections
    std::cout << std::endl;

    // Display current memory usage status
    displayMemoryStatus();

    // Output a newline for separation
    std::cout << std::endl;

    // Display the total number of hash operations performed
    std::cout << "HASH OPS: "
              << hashOps.load(std::memory_order_relaxed)
              << " ops" << std::flush;
}

// CPU stress test function
void SystemStressTest::cpuHashStressTest(int threadId) {
    constexpr int BATCH_SIZE = 4500;
    constexpr int CHUNK_SIZE = 1;

    // Define a compute-intensive hash-like function
    auto computeIntensiveHash = [](uint64_t base, uint64_t exponent, uint64_t mod) -> uint64_t {
        uint64_t result = 1;
        uint64_t nestedFactor = 1;

        for (uint64_t i = 0; i < exponent; ++i) {
            result = (result * base) % mod;
            nestedFactor = (nestedFactor * result) % mod;

            for (uint64_t j = 0; j < exponent; ++j) {
                nestedFactor += i + j;
                result *= nestedFactor;
            }

            if (i % 10 == 0) {
                result = (result + nestedFactor) % mod;
            }
        }
        return result;
    };

    uint64_t localHashOps = 0;

    // Main loop for stress testing
    while (running) {
        volatile uint64_t hashValue = 0;

        // Perform a batch of hash operations
        for (int i = 0; i < BATCH_SIZE && running; ++i) {
            // Generate pseudo-random input values
            volatile uint64_t randomBase = threadId * 123456789 + i * 987654321;
            volatile uint64_t randomExponent = ((i % 2000) + 500) * (threadId % 10 + 1);
            volatile uint64_t randomModulus = 1e9 + 12347;

            // Compute the hash-like value
            hashValue = computeIntensiveHash(randomBase, randomExponent, randomModulus);

            // Additional operation to avoid compiler optimizations
            if (hashValue % 1024 == 0) {
                hashValue = (hashValue + threadId) * (randomBase % 7);
            }

            ++localHashOps;

            // Update shared counter periodically
            if (localHashOps % CHUNK_SIZE == 0) {
                hashOps.fetch_add(CHUNK_SIZE, std::memory_order_relaxed);
                localHashOps = 0;
            }
        }

        // Add any remaining operations to the shared counter
        if (localHashOps > 0) {
            hashOps.fetch_add(localHashOps, std::memory_order_relaxed);
            localHashOps = 0;
        }
    }
}

// Memory stress test function
void SystemStressTest::memoryStressTest() {
    LinkedList<std::unique_ptr<std::vector<int>>> memoryBlocks;

    try {
        while (running && memoryAllocated < MULTIPLIER * TARGET_MEMORY) {
            static constexpr size_t blockSize = 1024 * 1024; // 1 MB blocks

            auto block = std::make_unique<std::vector<int>>(
                blockSize / sizeof(int), 1
            );

            memoryAllocated += blockSize;
            memoryBlocks.push_back(std::move(block));
        }
    } catch (const std::bad_alloc &e) {
        std::lock_guard<std::mutex> lock(consoleMutex);
        std::cout << "\n"
                  << ConsoleColors::RED
                  << "Memory allocation failed: " << e.what()
                  << ConsoleColors::RESET << std::endl;
    }
}

void SystemStressTest::manageThreadPool() {
    while (running) {
        float systemLoad = getCurrentSystemLoad();

        std::lock_guard<std::mutex> lock(consoleMutex);

        if (systemLoad > 0.75f && cpuThreads.size() < static_cast<size_t>(numCores)) {
            cpuThreads.emplace_back(&SystemStressTest::cpuHashStressTest, this, cpuThreads.size());
            std::cout << ConsoleColors::YELLOW 
                     << "\nAdding thread due to high load (" 
                     << systemLoad << ")" 
                     << ConsoleColors::RESET << std::flush;
        } 
        else if (systemLoad < 0.25f && cpuThreads.size() > 1) {
            if (cpuThreads.back().joinable()) {
                cpuThreads.back().join();
            }
            cpuThreads.pop_back();
            std::cout << ConsoleColors::YELLOW 
                     << "\nRemoving thread due to low load (" 
                     << systemLoad << ")" 
                     << ConsoleColors::RESET << std::flush;
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

void SystemStressTest::run() {
    // Initialize the console
    ConsoleInitializer::initialize();

    // Display the start banner
    std::cout << ConsoleColors::MAGENTA
              << "\n=== System Stress Test Starting ==="
              << ConsoleColors::RESET << std::endl;

    // Display warning message
    std::cout << ConsoleColors::YELLOW
              << "Warning: This program will stress your system for "
              << TEST_DURATION << " seconds."
              << ConsoleColors::RESET << std::endl;

    // Prompt the user to continue
    std::cout << "Press Enter to continue...";
    std::cin.get();

    // Detect the number of CPU cores
    const auto numCores = std::thread::hardware_concurrency();
    assert(numCores > 0 && "Failed to detect CPU cores");

    // Display detected cores
    std::cout << ConsoleColors::BLUE << "\nDetected "
              << numCores << " CPU cores"
              << ConsoleColors::RESET << std::endl;

    std::cout << "\nStarting stress test...\n\n" << std::flush;

    // Record the starting time
    auto startTime = std::chrono::steady_clock::now();

    // Launch CPU stress test threads
    std::vector<std::thread> cpuThreads;
    for (unsigned int i = 0; i < numCores; ++i) {
        cpuThreads.emplace_back(&SystemStressTest::cpuHashStressTest, this, i);
    }

    // Launch memory stress test thread
    std::thread memThread(&SystemStressTest::memoryStressTest, this);

    // Monitoring loop
    int elapsedSeconds = 0;
    while (elapsedSeconds <= TEST_DURATION) {
        auto elapsedTime = std::chrono::steady_clock::now() - startTime;
        elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(elapsedTime).count();

        updateDisplay(elapsedSeconds);

        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        moveCursor(2, true);
    }

    // Signal all threads to stop
    running = false;

    // Wait for all threads to complete
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