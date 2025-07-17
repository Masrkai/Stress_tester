#pragma once

#include <mutex>
#include <vector>
#include <thread>
#include <atomic>
#include <cstdint>

class SystemStressTest {
private:
    static constexpr int BAR_WIDTH = 30;                        // Progress bar width for time and memory displays
    static constexpr int MULTIPLIER = 15;                       // Memory multiplier for stress test
    static constexpr int TEST_DURATION = 30;                    // seconds
    static constexpr size_t TARGET_MEMORY = 1024 * 1024 * 1024; // 1 GB

    // Shared atomic variables to track system metrics
    std::atomic<bool> running{true};             // Flag to indicate if the test is running
    std::atomic<size_t> memoryAllocated{0};     // Memory allocated in bytes
    std::atomic<uint64_t> hashOps{0};          // Total Hashing operations

    std::mutex consoleMutex;
    std::vector<std::thread> cpuThreads;
    int numCores;

    // Helper methods
    float getCurrentSystemLoad();
    void clearLine() const;
    void moveCursor(int lines, bool up) const;
    void displayMemoryStatus() const;
    void displayTimeProgress(int elapsedSeconds) const;
    void updateDisplay(int elapsedSeconds);
    
    // Stress test methods
    void cpuHashStressTest(int threadId);
    void memoryStressTest();
    void manageThreadPool();

public:
    void run();
};