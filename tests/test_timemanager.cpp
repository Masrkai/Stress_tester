#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "../include/TimeManager.hpp"
#include "../src/TimeManager.cpp"


class TimeManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clean up any existing instance
        TimeManager::cleanup();
    }

    void TearDown() override {
        TimeManager::cleanup();
    }
};

TEST_F(TimeManagerTest, SingletonInstance) {
    TimeManager& instance1 = TimeManager::getInstance();
    TimeManager& instance2 = TimeManager::getInstance();

    // Should be the same instance
    EXPECT_EQ(&instance1, &instance2);
}

TEST_F(TimeManagerTest, InitialState) {
    TimeManager& tm = TimeManager::getInstance();

    EXPECT_FALSE(tm.hasStarted());
    EXPECT_FALSE(tm.hasEnded());
    EXPECT_EQ(0.0, tm.getElapsedSeconds());
    EXPECT_EQ(0, tm.getElapsedMilliseconds());
    EXPECT_EQ(0, tm.getElapsedSecondsInt());
}

TEST_F(TimeManagerTest, StartTimer) {
    TimeManager& tm = TimeManager::getInstance();

    tm.startTimer();

    EXPECT_TRUE(tm.hasStarted());
    EXPECT_FALSE(tm.hasEnded());

    // Small delay to ensure some time has passed
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_GT(tm.getElapsedSeconds(), 0.0);
    EXPECT_GT(tm.getElapsedMilliseconds(), 0);
}

TEST_F(TimeManagerTest, EndTimer) {
    TimeManager& tm = TimeManager::getInstance();

    tm.startTimer();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    tm.endTimer();

    EXPECT_TRUE(tm.hasStarted());
    EXPECT_TRUE(tm.hasEnded());

    double elapsedAtEnd = tm.getElapsedSeconds();

    // Wait some more time
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Elapsed time should not change after end
    EXPECT_EQ(elapsedAtEnd, tm.getElapsedSeconds());
}

TEST_F(TimeManagerTest, ShouldContinue) {
    TimeManager& tm = TimeManager::getInstance();

    // Should continue for large duration without starting
    EXPECT_TRUE(tm.shouldContinue(100));

    tm.startTimer();

    // Should continue for reasonable duration
    EXPECT_TRUE(tm.shouldContinue(10));

    // Should not continue for very small duration
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(tm.shouldContinue(0));
}

TEST_F(TimeManagerTest, Reset) {
    TimeManager& tm = TimeManager::getInstance();

    tm.startTimer();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    tm.endTimer();

    EXPECT_TRUE(tm.hasStarted());
    EXPECT_TRUE(tm.hasEnded());

    tm.reset();

    EXPECT_FALSE(tm.hasStarted());
    EXPECT_FALSE(tm.hasEnded());
    EXPECT_EQ(0.0, tm.getElapsedSeconds());
}

TEST_F(TimeManagerTest, TimeAccuracy) {
    TimeManager& tm = TimeManager::getInstance();

    tm.startTimer();

    // Sleep for known duration
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    double elapsed = tm.getElapsedSeconds();
    int64_t elapsedMs = tm.getElapsedMilliseconds();

    // Allow for some timing variance (±20ms)
    EXPECT_GE(elapsed, 0.08);  // 80ms minimum
    EXPECT_LE(elapsed, 0.15);  // 150ms maximum

    EXPECT_GE(elapsedMs, 80);
    EXPECT_LE(elapsedMs, 150);
}

TEST_F(TimeManagerTest, ConcurrentAccess) {
    TimeManager& tm = TimeManager::getInstance();

    std::atomic<int> startedThreads{0};
    std::atomic<bool> allReady{false};

    auto threadFunc = [&]() {
        startedThreads++;
        while (!allReady) {
            std::this_thread::yield();
        }

        double elapsed1 = tm.getElapsedSeconds();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        double elapsed2 = tm.getElapsedSeconds();

        // Time should be monotonically increasing
        EXPECT_GE(elapsed2, elapsed1);
    };

    tm.startTimer();

    // Launch multiple threads
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back(threadFunc);
    }

    // Wait for all threads to be ready
    while (startedThreads.load() < 10) {
        std::this_thread::yield();
    }

    allReady = true;

    // Join all threads
    for (auto& t : threads) {
        t.join();
    }
}