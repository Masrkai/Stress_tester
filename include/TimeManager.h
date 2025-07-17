#pragma once

#include <chrono>
#include <atomic>
#include <mutex>

/*
 * Global time management class for precise timing control
 * Provides centralized time tracking for the entire stress test duration
 */
class TimeManager {
private:
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;
    std::atomic<bool> testStarted{false};
    std::atomic<bool> testEnded{false};
    mutable std::mutex timeMutex;
    
    static TimeManager* instance;
    static std::mutex instanceMutex;
    
    // Private constructor for singleton pattern
    TimeManager() = default;
    
public:
    // Singleton pattern implementation
    static TimeManager& getInstance() {
        std::lock_guard<std::mutex> lock(instanceMutex);
        if (!instance) {
            instance = new TimeManager();
        }
        return *instance;
    }
    
    // Start the global timer
    void startTimer() {
        std::lock_guard<std::mutex> lock(timeMutex);
        if (!testStarted.load()) {
            startTime = std::chrono::steady_clock::now();
            testStarted.store(true);
        }
    }
    
    // End the global timer
    void endTimer() {
        std::lock_guard<std::mutex> lock(timeMutex);
        if (testStarted.load() && !testEnded.load()) {
            endTime = std::chrono::steady_clock::now();
            testEnded.store(true);
        }
    }
    
    // Get elapsed time in seconds (double precision)
    double getElapsedSeconds() const {
        std::lock_guard<std::mutex> lock(timeMutex);
        if (!testStarted.load()) {
            return 0.0;
        }
        
        auto currentTime = testEnded.load() ? endTime : std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime);
        return duration.count() / 1000000.0; // Convert microseconds to seconds
    }
    
    // Get elapsed time in milliseconds
    int64_t getElapsedMilliseconds() const {
        std::lock_guard<std::mutex> lock(timeMutex);
        if (!testStarted.load()) {
            return 0;
        }
        
        auto currentTime = testEnded.load() ? endTime : std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();
    }
    
    // Get elapsed time in integer seconds (for display purposes)
    int getElapsedSecondsInt() const {
        return static_cast<int>(getElapsedSeconds());
    }
    
    // Check if the test should continue based on duration
    bool shouldContinue(int maxDurationSeconds) const {
        return getElapsedSeconds() < maxDurationSeconds;
    }
    
    // Check if test has started
    bool hasStarted() const {
        return testStarted.load();
    }
    
    // Check if test has ended
    bool hasEnded() const {
        return testEnded.load();
    }
    
    // Reset the timer (for testing purposes)
    void reset() {
        std::lock_guard<std::mutex> lock(timeMutex);
        testStarted.store(false);
        testEnded.store(false);
    }
    
    // Get precise start time
    std::chrono::steady_clock::time_point getStartTime() const {
        std::lock_guard<std::mutex> lock(timeMutex);
        return startTime;
    }
    
    // Get precise end time
    std::chrono::steady_clock::time_point getEndTime() const {
        std::lock_guard<std::mutex> lock(timeMutex);
        return endTime;
    }
    
    // Cleanup function
    static void cleanup() {
        std::lock_guard<std::mutex> lock(instanceMutex);
        delete instance;
        instance = nullptr;
    }
    
    // Destructor
    ~TimeManager() = default;
    
    // Delete copy constructor and assignment operator
    TimeManager(const TimeManager&) = delete;
    TimeManager& operator=(const TimeManager&) = delete;
};