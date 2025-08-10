#pragma once

#include <mutex>
#include <vector>
#include <thread>
#include <atomic>
#include <cstdint>

#include "TimeManager.hpp"

class CPUStressTest {
private:
    static constexpr int TEST_DURATION = 30; // seconds

    // Shared atomic variables to track CPU metrics
    std::atomic<uint64_t> hashOps{0};        // Total Hashing operations
    std::atomic<bool>     running{true};     // Flag to indicate if the test is running

    int numCores;
    std::mutex consoleMutex;
    std::vector<std::thread> cpuThreads;

    // Reference to global time manager
    TimeManager& timeManager;

    // Helper methods
    float getCurrentSystemLoad();
    void  cpuHashStressTest(int threadId);
    void  manageThreadPool();

public:
    CPUStressTest() : timeManager(TimeManager::getInstance()) {}
    ~CPUStressTest() = default;

    // Public interface
    void initialize();
    void start();
    void stop();
    void waitForCompletion();

    // Getters for monitoring
    uint64_t getHashOperations() const { return hashOps.load(std::memory_order_relaxed); }
    int getCoreCount() const { return numCores; }
    bool isRunning() const { return running.load(); }

    // Disable copy constructor and assignment
    CPUStressTest(const CPUStressTest&) = delete;
    CPUStressTest& operator=(const CPUStressTest&) = delete;
};