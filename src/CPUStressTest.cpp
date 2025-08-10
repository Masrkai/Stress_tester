#include "../include/CPUStressTest.hpp"
#include "../include/ConsoleColors.hpp"

#include <chrono>
#include <iostream>
#include <algorithm>
#include <cassert>

// Helper to get current system CPU load (simplified version)
float CPUStressTest::getCurrentSystemLoad() {
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

// CPU stress test function
void CPUStressTest::cpuHashStressTest(int threadId) {
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

void CPUStressTest::manageThreadPool() {
    while (running && timeManager.shouldContinue(TEST_DURATION)) {
        float systemLoad = getCurrentSystemLoad();

        std::lock_guard<std::mutex> lock(consoleMutex);

        if (systemLoad > 0.75f && cpuThreads.size() < static_cast<size_t>(numCores)) {
            cpuThreads.emplace_back(&CPUStressTest::cpuHashStressTest, this, cpuThreads.size());
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

void CPUStressTest::initialize() {
    // Detect the number of CPU cores
    numCores = std::thread::hardware_concurrency();
    assert(numCores > 0 && "Failed to detect CPU cores");

    // Reset atomic variables
    hashOps.store(0);
    running.store(true);
}

void CPUStressTest::start() {
    // Launch CPU stress test threads
    for (int i = 0; i < numCores; ++i) {
        cpuThreads.emplace_back(&CPUStressTest::cpuHashStressTest, this, i);
    }
}

void CPUStressTest::stop() {
    running = false;
}

void CPUStressTest::waitForCompletion() {
    // Wait for all threads to complete
    for (auto& thread : cpuThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    cpuThreads.clear();
}