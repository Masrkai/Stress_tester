#pragma once

#include <mutex>
#include <vector>
#include <thread>
#include <atomic>
#include <cstdint>

class SystemStressTest {
private:
    static constexpr int    BAR_WIDTH = 30;                     // Progress bar width for time and memory displays
    static constexpr int    MULTIPLIER = 2;                     // Memory multiplier for stress test (resulting in a 2 GB Max Allocation)
    static constexpr int    TEST_DURATION = 30;                 // seconds
    static constexpr size_t TARGET_MEMORY = 1024 * 1024 * 1024; // 1 GB

    // Shared atomic variables to track system metrics
    std::atomic<uint64_t> hashOps{0};           // Total Hashing operations
    std::atomic<bool>     running{true};        // Flag to indicate if the test is running
    std::atomic<size_t>   memoryAllocated{0};   // Memory allocated in bytes

    int numCores;
    std::mutex consoleMutex;
    std::vector<std::thread> cpuThreads;

    // Helper methods
    void clearLine() const;
    void displayMemoryStatus() const;
    void moveCursor(int lines, bool up) const;
    void displayTimeProgress(int elapsedSeconds) const;

    float getCurrentSystemLoad();
    void updateDisplay(int elapsedSeconds);

    // Stress test methods
    void memoryStressTest();
    void manageThreadPool();
    void cpuHashStressTest(int threadId);

public:
    void run();
};