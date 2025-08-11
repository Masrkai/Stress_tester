#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "../include/CPUStressTest.hpp"
#include "../include/TimeManager.hpp"

class CPUStressTestTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clean up any existing TimeManager instance
        TimeManager::cleanup();
    }

    void TearDown() override {
        TimeManager::cleanup();
    }
};

TEST_F(CPUStressTestTest, Initialization) {
    CPUStressTest cpuTest;

    // Before initialization
    EXPECT_EQ(0, cpuTest.getCoreCount());
    EXPECT_EQ(0, cpuTest.getHashOperations());
    EXPECT_FALSE(cpuTest.isRunning());

    cpuTest.initialize();

    // After initialization
    EXPECT_GT(cpuTest.getCoreCount(), 0);
    EXPECT_EQ(0, cpuTest.getHashOperations());
    EXPECT_TRUE(cpuTest.isRunning());
}

TEST_F(CPUStressTestTest, CoreDetection) {
    CPUStressTest cpuTest;
    cpuTest.initialize();

    int detectedCores = cpuTest.getCoreCount();
    int systemCores = std::thread::hardware_concurrency();

    EXPECT_EQ(systemCores, detectedCores);
    EXPECT_GT(detectedCores, 0);
}

TEST_F(CPUStressTestTest, StartAndStop) {
    CPUStressTest cpuTest;
    TimeManager& tm = TimeManager::getInstance();

    cpuTest.initialize();
    tm.startTimer();

    EXPECT_TRUE(cpuTest.isRunning());

    cpuTest.start();

    // Let it run briefly
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Should have performed some operations
    EXPECT_GT(cpuTest.getHashOperations(), 0);

    cpuTest.stop();
    EXPECT_FALSE(cpuTest.isRunning());

    uint64_t opsBeforeWait = cpuTest.getHashOperations();

    cpuTest.waitForCompletion();

    // Operations should not increase after stopping
    uint64_t opsAfterWait = cpuTest.getHashOperations();
    EXPECT_EQ(opsBeforeWait, opsAfterWait);
}

TEST_F(CPUStressTestTest, HashOperationsIncrease) {
    CPUStressTest cpuTest;
    TimeManager& tm = TimeManager::getInstance();

    cpuTest.initialize();
    tm.startTimer();
    cpuTest.start();

    uint64_t initialOps = cpuTest.getHashOperations();

    // Wait for some operations to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    uint64_t laterOps = cpuTest.getHashOperations();

    cpuTest.stop();
    cpuTest.waitForCompletion();

    // Operations should have increased
    EXPECT_GT(laterOps, initialOps);
}

TEST_F(CPUStressTestTest, MultipleStartStopCycles) {
    CPUStressTest cpuTest;
    TimeManager& tm = TimeManager::getInstance();

    cpuTest.initialize();

    for (int cycle = 0; cycle < 3; ++cycle) {
        tm.reset();
        tm.startTimer();

        uint64_t startOps = cpuTest.getHashOperations();

        cpuTest.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        cpuTest.stop();
        cpuTest.waitForCompletion();

        uint64_t endOps = cpuTest.getHashOperations();

        // Each cycle should produce operations
        EXPECT_GT(endOps, startOps);
    }
}

TEST_F(CPUStressTestTest, ThreadSafety) {
    CPUStressTest cpuTest;
    TimeManager& tm = TimeManager::getInstance();

    cpuTest.initialize();
    tm.startTimer();
    cpuTest.start();

    std::atomic<uint64_t> maxOperations{0};
    std::atomic<int> readThreads{0};

    // Launch multiple threads that read the hash operations
    auto readerFunc = [&]() {
        readThreads++;
        for (int i = 0; i < 100; ++i) {
            uint64_t currentOps = cpuTest.getHashOperations();
            uint64_t expected = maxOperations.load();
            while (currentOps > expected && !maxOperations.compare_exchange_weak(expected, currentOps)) {
                expected = maxOperations.load();
            }
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        readThreads--;
    };

    std::vector<std::thread> readers;
    for (int i = 0; i < 5; ++i) {
        readers.emplace_back(readerFunc);
    }

    // Let everything run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    cpuTest.stop();

    // Wait for reader threads
    for (auto& t : readers) {
        t.join();
    }

    cpuTest.waitForCompletion();

    // Should have performed operations without crashes
    EXPECT_GT(cpuTest.getHashOperations(), 0);
    EXPECT_EQ(0, readThreads.load());
}

TEST_F(CPUStressTestTest, PerformanceBaseline) {
    CPUStressTest cpuTest;
    TimeManager& tm = TimeManager::getInstance();

    cpuTest.initialize();
    tm.startTimer();

    auto start = std::chrono::high_resolution_clock::now();

    cpuTest.start();

    // Run for a fixed duration
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    cpuTest.stop();
    cpuTest.waitForCompletion();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    uint64_t totalOps = cpuTest.getHashOperations();

    // Should have performed a reasonable number of operations
    EXPECT_GT(totalOps, 1000);  // At least 1000 operations in 500ms

    // Calculate operations per second
    double opsPerSecond = static_cast<double>(totalOps) / (duration.count() / 1000.0);

    // Should achieve reasonable throughput (adjust based on expected performance)
    EXPECT_GT(opsPerSecond, 1000.0);  // At least 1000 ops/second
}

TEST_F(CPUStressTestTest, StressTestDuration) {
    CPUStressTest cpuTest;
    TimeManager& tm = TimeManager::getInstance();

    cpuTest.initialize();
    tm.startTimer();
    cpuTest.start();

    // Test should respect the time manager duration
    auto testStart = std::chrono::steady_clock::now();

    // Wait for the test to naturally complete or timeout
    for (int i = 0; i < 100 && cpuTest.isRunning(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    auto testEnd = std::chrono::steady_clock::now();
    auto testDuration = std::chrono::duration_cast<std::chrono::seconds>(testEnd - testStart);

    cpuTest.stop();
    cpuTest.waitForCompletion();

    // Test should have run for some time but not exceed reasonable limits
    EXPECT_GT(cpuTest.getHashOperations(), 0);
    EXPECT_LT(testDuration.count(), 35);  // Should not run longer than 35 seconds
}

TEST_F(CPUStressTestTest, ResourceCleanup) {
    // Test that resources are properly cleaned up
    {
        CPUStressTest cpuTest;
        TimeManager& tm = TimeManager::getInstance();

        cpuTest.initialize();
        tm.startTimer();
        cpuTest.start();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        cpuTest.stop();
        cpuTest.waitForCompletion();

        EXPECT_GT(cpuTest.getHashOperations(), 0);
    }

    // Destructor should have cleaned up properly
    // Test passes if no memory leaks or hanging threads
}

TEST_F(CPUStressTestTest, ZeroOperationsWhenNotStarted) {
    CPUStressTest cpuTest;

    cpuTest.initialize();

    // Should not perform operations without starting
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(0, cpuTest.getHashOperations());
}

TEST_F(CPUStressTestTest, StopWithoutStart) {
    CPUStressTest cpuTest;

    cpuTest.initialize();

    // Should handle stop without start gracefully
    cpuTest.stop();
    cpuTest.waitForCompletion();

    EXPECT_EQ(0, cpuTest.getHashOperations());
    EXPECT_FALSE(cpuTest.isRunning());
}