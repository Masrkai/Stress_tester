#pragma once

#include <mutex>
#include <vector>
#include <thread>
#include <atomic>
#include <cstdint>

#include "LinkedList.hpp"
#include "TimeManager.hpp"

class SystemStressTest {
private:
    LinkedList<std::unique_ptr<std::vector<int>>> memoryBlocks;  // Keep this here instead of the function
                                                                 // because allocated pointer won't live in it

    static constexpr int    BAR_WIDTH = 30;                     // Progress bar width for time and memory displays
    static constexpr int    MULTIPLIER = 2;                     // Memory multiplier for stress test (resulting in a 2 GB Max Allocation)
    static constexpr int    TEST_DURATION = 30;                 // seconds
    static constexpr size_t TARGET_MEMORY = 1024 * 1024 * 1024; // 1 GB

    // Memory bandwidth measurement constants
    static constexpr size_t BANDWIDTH_TEST_SIZE = 64 * 1024 * 1024; // 64MB test buffer
    static constexpr int    BANDWIDTH_ITERATIONS = 5;               // Number of iterations for averaging

    // Shared atomic variables to track system metrics
    std::atomic<uint64_t> hashOps{0};           // Total Hashing operations
    std::atomic<bool>     running{true};        // Flag to indicate if the test is running
    std::atomic<size_t>   memoryAllocated{0};   // Memory allocated in bytes
    std::atomic<double>   memoryBandwidth{0.0}; // Memory bandwidth in MB/s

    int numCores;
    std::mutex consoleMutex;
    std::vector<std::thread> cpuThreads;

    // Reference to global time manager
    TimeManager& timeManager;

    // Memory bandwidth measurement variables
    std::unique_ptr<uint8_t[]> bandwidthTestBuffer;
    std::atomic<bool> bandwidthTestRunning{false};

    // Helper methods
    void clearLine() const;
    void displayMemoryStatus() const;
    void moveCursor(int lines, bool up) const;
    void displayTimeProgress() const;  // Removed parameter - now uses global time
    void displayBandwidthStatus() const;

    float getCurrentSystemLoad();
    void  updateDisplay();  // Removed parameter - now uses global time

    // Memory bandwidth measurement methods
    void measureMemoryBandwidth();
    double performSequentialRead(uint8_t* buffer, size_t size);
    double performSequentialWrite(uint8_t* buffer, size_t size);
    double performRandomAccess(uint8_t* buffer, size_t size);
    void   continuousBandwidthTest();

    // Stress test methods
    void memoryStressTest();
    void manageThreadPool();
    void cpuHashStressTest(int threadId);

public:
    SystemStressTest() : timeManager(TimeManager::getInstance()) {}
    void run();
};