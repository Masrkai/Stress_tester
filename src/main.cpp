#include <iostream>
#include <mutex>
#include <vector>
#include <thread>
#include <chrono>
#include <iomanip>
#include <atomic>
#include <cstring>

// Cross-platform color support
#ifdef _WIN32
    #include <windows.h>
    #define ENABLE_COLORS() SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ENABLE_VIRTUAL_TERMINAL_PROCESSING)
#else
    #define ENABLE_COLORS()
#endif

// ANSI color codes
#define RED "\033[31m"
#define BLUE "\033[34m"
#define CYAN "\033[36m"
#define RESET "\033[0m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define MAGENTA "\033[35m"

using namespace std;
atomic<bool> running(true);
atomic<size_t> memoryAllocated(0);
const size_t TARGET_MEMORY = 1024 * 1024 * 1024; // 1 GB
const int TEST_DURATION = 30; // seconds

// Mutex for synchronizing console output
mutex consoleMutex;

void clearLine() {
    cout << "\r\033[K"; // Clear the current line
}

void moveCursorUp(int lines) {
    cout << "\033[" << lines << "A";
}

void moveCursorDown(int lines) {
    cout << "\033[" << lines << "B";
}

// Memory status display
void displayMemoryStatus() {
    const int barWidth = 40;
    float progress = static_cast<float>(memoryAllocated) / TARGET_MEMORY;
    int pos = static_cast<int>(barWidth * progress);

    lock_guard<mutex> lock(consoleMutex);
    clearLine();
    cout << "Memory: [";
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) cout << GREEN "■" RESET;
        else cout << "□";
    }
    cout << "] " << fixed << setprecision(1) 
         << (memoryAllocated / (1024.0 * 1024.0)) << "MB / "
         << (TARGET_MEMORY / (1024.0 * 1024.0)) << "MB" << flush;
}

// CPU stress test function
void cpuStressTest(int threadId) {
    volatile double result = 1.0;
    while (running) {
        for (int i = 1; i < 10000 && running; i++) {
            result *= i;
            result /= (i + 1);
        }
    }
}

// Memory stress test function
void memoryStressTest() {
    vector<vector<int>*> memoryBlocks;
    try {
        while (running && memoryAllocated < TARGET_MEMORY) {
            const size_t blockSize = 1024 * 1024; // 1MB blocks
            vector<int>* block = new vector<int>(blockSize/sizeof(int), 1);
            memoryBlocks.push_back(block);
            memoryAllocated += blockSize;
            
            displayMemoryStatus();
            this_thread::sleep_for(chrono::milliseconds(100));
        }
    } catch (const bad_alloc& e) {
        lock_guard<mutex> lock(consoleMutex);
        cout << "\n" << RED << "Memory allocation failed: " << e.what() << RESET << endl;
    }

    while (running) {
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    // Cleanup
    for (auto block : memoryBlocks) {
        delete block;
    }
}

void displayTimeProgress(int elapsedSeconds) {
    const int barWidth = 40;
    float progress = static_cast<float>(elapsedSeconds) / TEST_DURATION;
    int pos = static_cast<int>(barWidth * progress);

    lock_guard<mutex> lock(consoleMutex);
    clearLine();
    cout << "Time:   [";
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) cout << CYAN "■" RESET;
        else cout << "□";
    }
    cout << "] " << elapsedSeconds << "s / " << TEST_DURATION << "s" << flush;
}

int main() {
    ENABLE_COLORS();
    cout << MAGENTA << "\n=== System Stress Test Starting ===" << RESET << endl;
    cout << YELLOW << "Warning: This program will stress your system for " << TEST_DURATION << " seconds." << RESET << endl;
    cout << "Press Enter to continue...";
    cin.get();

    const int numCores = thread::hardware_concurrency();
    cout << BLUE << "\nDetected " << numCores << " CPU cores" << RESET << endl;
    cout << "\nStarting stress test...\n\n" << flush;

    // Start timing
    auto startTime = chrono::high_resolution_clock::now();

    // Create CPU stress threads
    vector<thread> cpuThreads;
    for (int i = 0; i < numCores; ++i) {
        cpuThreads.emplace_back(cpuStressTest, i);
    }

    // Start memory stress thread
    thread memThread(memoryStressTest);

    // Main loop with progress display
    for (int i = 0; i <= TEST_DURATION; ++i) {
        displayTimeProgress(i);
        cout << endl;
        this_thread::sleep_for(chrono::seconds(1));
        moveCursorUp(1);
    }

    // Stop all tests
    running = false;

    // Join all threads
    for (auto& thread : cpuThreads) {
        thread.join();
    }
    memThread.join();

    // Move cursor past the progress bars
    cout << "\n\n";

    // Calculate execution time
    auto endTime = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);

    // Print results
    cout << MAGENTA << "=== Test Results ===" << RESET << endl;
    cout << CYAN << "Total execution time: " << duration.count() / 1000.0 << " seconds" << RESET << endl;
    cout << CYAN << "Maximum memory allocated: " << memoryAllocated / (1024 * 1024) << "MB" << RESET << endl;
    cout << CYAN << "CPU cores utilized: " << numCores << RESET << endl;

    return 0;
}