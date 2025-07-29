#include "../include/SystemStressTest.hpp"
#include "../include/ConsoleInitializer.hpp"
#include "../include/ConsoleColors.hpp"
#include "../include/LinkedList.hpp"
#include "../include/TimeManager.hpp"

#include <chrono>
#include <iomanip>
#include <memory>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <random>
#include <cstring>

// Helper to get current system CPU load (simplified version)
float SystemStressTest::getCurrentSystemLoad() {
    // This is a placeholder implementation
    // In a real implementation, you would:
    // 1. Sample CPU usage over a short period
    // 2. Compare previous and current measurements
    // 3. Return a value between 0.0 and 1.0

    // For now, return a value based on hash operations rate
    static uint64_t lastOps = 0;
    static int64_t lastCheck = 0;

    int64_t currentTime = timeManager.getElapsedMilliseconds();
    int64_t duration = currentTime - lastCheck;

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

void SystemStressTest::displayBandwidthStatus() const {
    double currentBandwidth = memoryBandwidth.load(std::memory_order_relaxed);

    // Clear the current console line
    clearLine();

    // Display memory bandwidth with color coding based on performance
    std::string colorCode = ConsoleColors::CYAN;
    if (currentBandwidth > 20000) {      // > 20 GB/s - excellent DDR5 performance
        colorCode = ConsoleColors::GREEN;
    } else if (currentBandwidth > 10000) { // > 10 GB/s - good performance
        colorCode = ConsoleColors::YELLOW;
    } else if (currentBandwidth > 5000) {  // > 5 GB/s - moderate performance
        colorCode = ConsoleColors::CYAN;
    } else {                               // < 5 GB/s - lower performance
        colorCode = ConsoleColors::RED;
    }

    std::cout << "RAM BW: " << colorCode << std::fixed << std::setprecision(2)
              << currentBandwidth << " MB/s" << ConsoleColors::RESET;

    // Add estimated frequency for DDR5 (very rough approximation)
    if (currentBandwidth > 0) {
        // DDR5 dual channel rough estimation: bandwidth ≈ frequency × 2 channels × 8 bytes × efficiency
        // Assuming ~70% efficiency: frequency ≈ bandwidth / (2 × 8 × 0.7) ≈ bandwidth / 11.2
        double estimatedFreq = currentBandwidth / 11.2;
        std::cout << " (~" << static_cast<int>(estimatedFreq) << " MHz est.)" << std::flush;
    }
}

void SystemStressTest::displayTimeProgress() const {
    // Get current elapsed time from global time manager
    double elapsedSecondsDouble = timeManager.getElapsedSeconds();
    int elapsedSeconds = static_cast<int>(elapsedSecondsDouble);

    // Clamp elapsedSeconds to not exceed TEST_DURATION for display purposes
    int displaySeconds = std::min(elapsedSeconds, TEST_DURATION);

    // Calculate the progress as a fraction of the total test duration
    float progress = static_cast<float>(displaySeconds) / TEST_DURATION;

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
              << displaySeconds << "s / "
              << TEST_DURATION << "s" << std::flush;
}

void SystemStressTest::updateDisplay() {
    // Lock the console to prevent concurrent access by multiple threads
    std::lock_guard<std::mutex> lock(consoleMutex);

    // Clear the current console line
    clearLine();

    // Display the elapsed time using global time manager
    displayTimeProgress();

    // Output a newline to separate sections
    std::cout << std::endl;

    // Display current memory usage status
    displayMemoryStatus();

    // Output a newline for separation
    std::cout << std::endl;

    // Display memory bandwidth status
    displayBandwidthStatus();

    // Output a newline for separation
    std::cout << std::endl;

    // Display the total number of hash operations performed
    std::cout << "HASH OPS: "
              << hashOps.load(std::memory_order_relaxed)
              << " ops" << std::flush;
}

// Memory bandwidth measurement methods
double SystemStressTest::performSequentialRead(uint8_t* buffer, size_t size) {
    auto start = std::chrono::high_resolution_clock::now();

    volatile uint64_t sum = 0;
    for (size_t i = 0; i < size; i += 64) { // 64-byte cache line aligned
        sum += buffer[i];
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    return (static_cast<double>(size) / 1024.0 / 1024.0) / (duration.count() / 1e9);
}

double SystemStressTest::performSequentialWrite(uint8_t* buffer, size_t size) {
    auto start = std::chrono::high_resolution_clock::now();

    // Write pattern
    for (size_t i = 0; i < size; i += 64) {
        buffer[i] = static_cast<uint8_t>(i & 0xFF);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    return (static_cast<double>(size) / 1024.0 / 1024.0) / (duration.count() / 1e9);
}

double SystemStressTest::performRandomAccess(uint8_t* buffer, size_t size) {
    const size_t numAccesses = size / 64; // Access every 64 bytes
    std::vector<size_t> indices(numAccesses);

    // Generate random indices
    for (size_t i = 0; i < numAccesses; ++i) {
        indices[i] = (i * 64) % size;
    }

    // Shuffle for random access pattern
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(indices.begin(), indices.end(), gen);

    auto start = std::chrono::high_resolution_clock::now();

    volatile uint64_t sum = 0;
    for (size_t idx : indices) {
        sum += buffer[idx];
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    return (static_cast<double>(size) / 1024.0 / 1024.0) / (duration.count() / 1e9);
}

void SystemStressTest::measureMemoryBandwidth() {
    if (!bandwidthTestBuffer) {
        bandwidthTestBuffer = std::make_unique<uint8_t[]>(BANDWIDTH_TEST_SIZE);

        // Initialize buffer with test data
        for (size_t i = 0; i < BANDWIDTH_TEST_SIZE; ++i) {
            bandwidthTestBuffer[i] = static_cast<uint8_t>(i & 0xFF);
        }
    }

    double totalBandwidth = 0.0;
    int validTests = 0;

    // Perform multiple test iterations for averaging
    for (int i = 0; i < BANDWIDTH_ITERATIONS && bandwidthTestRunning; ++i) {
        // Sequential read test
        double readBW = performSequentialRead(bandwidthTestBuffer.get(), BANDWIDTH_TEST_SIZE);

        // Sequential write test
        double writeBW = performSequentialWrite(bandwidthTestBuffer.get(), BANDWIDTH_TEST_SIZE);

        // Random access test (typically much slower)
        double randomBW = performRandomAccess(bandwidthTestBuffer.get(), BANDWIDTH_TEST_SIZE);

        // Use the highest bandwidth measurement (sequential read typically performs best)
        double maxBW = std::max({readBW, writeBW, randomBW * 2}); // Weight random access

        if (maxBW > 0 && maxBW < 1000000) { // Sanity check (less than 1TB/s)
            totalBandwidth += maxBW;
            validTests++;
        }

        // Small delay between iterations
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (validTests > 0) {
        double avgBandwidth = totalBandwidth / validTests;
        memoryBandwidth.store(avgBandwidth, std::memory_order_relaxed);
    }
}

void SystemStressTest::continuousBandwidthTest() {
    bandwidthTestRunning = true;

    // Perform initial measurement
    measureMemoryBandwidth();

    // Continue measuring periodically during the test
    while (running && timeManager.shouldContinue(TEST_DURATION) && bandwidthTestRunning) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        measureMemoryBandwidth();
    }

    bandwidthTestRunning = false;
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

    // Main loop for stress testing - now uses global time manager
    while (running && timeManager.shouldContinue(TEST_DURATION)) {
        volatile uint64_t hashValue = 0;

        // Perform a batch of hash operations
        for (int i = 0; i < BATCH_SIZE && running && timeManager.shouldContinue(TEST_DURATION); ++i) {
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

// Function to stress test memory allocation
void SystemStressTest::memoryStressTest() {
    try {
        // Loop to allocate memory until the target threshold is reached or the test is stopped
        while (running && memoryAllocated < (MULTIPLIER * TARGET_MEMORY) - BANDWIDTH_TEST_SIZE && timeManager.shouldContinue(TEST_DURATION)) {
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
            memoryBlocks.Insert_At_End(std::move(block));
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

void SystemStressTest::manageThreadPool() {
    while (running && timeManager.shouldContinue(TEST_DURATION)) {
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
    numCores = std::thread::hardware_concurrency();
    assert(numCores > 0 && "Failed to detect CPU cores");

    // Display detected cores
    std::cout << ConsoleColors::BLUE << "\nDetected "
              << numCores << " CPU cores"
              << ConsoleColors::RESET << std::endl;

    std::cout << "\nStarting stress test...\n\n" << std::flush;

    // Start the global timer
    timeManager.startTimer();

    // Launch CPU stress test threads
    std::vector<std::thread> cpuThreads;
    for (int i = 0; i < numCores; ++i) {
        cpuThreads.emplace_back(&SystemStressTest::cpuHashStressTest, this, i);
    }

    // Launch memory stress test thread
    std::thread memThread(&SystemStressTest::memoryStressTest, this);

    // Launch memory bandwidth test thread
    std::thread bandwidthThread(&SystemStressTest::continuousBandwidthTest, this);

    // Monitoring loop - now uses global time manager
    while (timeManager.shouldContinue(TEST_DURATION)) {
        updateDisplay();

        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        moveCursor(3, true); // Updated to move up 3 lines now
    }

    // Signal all threads to stop and end the timer
    running = false;
    bandwidthTestRunning = false;
    timeManager.endTimer();

    // Wait for all threads to complete
    for (auto& thread : cpuThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    if (memThread.joinable()) {
        memThread.join();
    }

    if (bandwidthThread.joinable()) {
        bandwidthThread.join();
    }

    // Display test results using precise timing
    std::cout << std::endl;

    std::cout << "\n\n" << ConsoleColors::MAGENTA
              << "=== Test Results ==="
              << ConsoleColors::RESET << std::endl;

    std::cout << ConsoleColors::CYAN
              << "Total hashing operations: " << hashOps.load(std::memory_order_relaxed)
              << " ops" << ConsoleColors::RESET << std::endl;

    std::cout << ConsoleColors::CYAN
              << "Total execution time: " << std::fixed << std::setprecision(3)
              << timeManager.getElapsedSeconds() << " seconds"
              << ConsoleColors::RESET << std::endl;

    std::cout << ConsoleColors::CYAN
              << "Maximum memory allocated: " << ( (memoryAllocated + BANDWIDTH_TEST_SIZE) / (1024 * 1024) )
              << "MB" << ConsoleColors::RESET << std::endl;

    std::cout << ConsoleColors::CYAN
              << "Memory bandwidth: " << std::fixed << std::setprecision(2)
              << memoryBandwidth.load(std::memory_order_relaxed) << " MB/s"
              << ConsoleColors::RESET << std::endl;

    std::cout << ConsoleColors::CYAN
              << "CPU cores utilized: " << numCores
              << ConsoleColors::RESET << std::endl;

    // Cleanup
    TimeManager::cleanup();
}