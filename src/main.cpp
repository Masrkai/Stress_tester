#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>

//#include <iomanip>
//#include <cstring>
//#include <locale>


//! Cross-platform color support
#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
    #include <fcntl.h>

    // Enable UTF-8 support and ANSI color processing
    void initializeConsole() {
        SetConsoleOutputCP(CP_UTF8);
        _setmode(_fileno(stdout), _O_U16TEXT);

        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
    }
#else
    //? No-op for non-Windows platforms
    void initializeConsole() {}
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


//------------------------------------------------------------------------>  Displaying
mutex consoleMutex; // Mutex for synchronizing console output

void clearLine() {
    cout << "\r\033[K"; // Clear the current line
}

void moveCursorUp(int lines) {
    cout << "\033[" << lines << "A";
}

void moveCursorDown(int lines) {
    cout << "\033[" << lines << "B";
}

// Display functions
void displayMemoryStatus() {
    const int barWidth = 30;
    float progress = static_cast<float>(memoryAllocated) / TARGET_MEMORY;
    int pos = static_cast<int>(barWidth * progress);

    string output = "Memory: [";
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) output += GREEN "■" RESET;
        else output += "□";
    }
    output += "] " + to_string(memoryAllocated / (1024 * 1024)) + "MB / "
              + to_string(TARGET_MEMORY / (1024 * 1024)) + "MB";

    clearLine();
    cout << output << flush;
}

void displayTimeProgress(int elapsedSeconds) {
    const int barWidth = 30;
    float progress = static_cast<float>(elapsedSeconds) / TEST_DURATION;
    int pos = static_cast<int>(barWidth * progress);

    string output = "Time:   [";
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) output += CYAN "■" RESET;
        else output += "□";
    }
    output += "] " + to_string(elapsedSeconds) + "s / " + to_string(TEST_DURATION) + "s";

    clearLine();
    cout << output << flush;
}

void updateDisplay(int elapsedSeconds) {
    lock_guard<mutex> lock(consoleMutex);
    clearLine();
    displayTimeProgress(elapsedSeconds);
    cout << endl;
    displayMemoryStatus();
    cout << endl;
}


//------------------------------------------------------------------------> Stressing
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
    // Create a vector to store pointers to dynamically allocated memory blocks.
    // We use std::unique_ptr to ensure the memory is automatically freed when no longer needed.
    vector<unique_ptr<vector<int>>> memoryBlocks;

    try {
        // Continue allocating memory while the test is running and the allocated memory is below the target.
        while (running && memoryAllocated < TARGET_MEMORY) {
            const size_t blockSize = 1024 * 1024; // Define the size of each memory block (1 MB).

            // Dynamically allocate a memory block (1 MB) using std::make_unique.
            // Each block is a vector of integers initialized with the value '1'.
            auto block = make_unique<vector<int>>(blockSize / sizeof(int), 1);

            // Update the total memory allocated by adding the size of the new block.
            memoryAllocated += blockSize;

            // Move the unique pointer for the allocated block into the memoryBlocks vector.
            // Using std::move is necessary because unique_ptr cannot be copied, only moved.
            memoryBlocks.push_back(move(block));

            // Note: There's no sleep delay here, meaning memory allocation occurs as fast as possible
            // until the target memory is reached or an exception is thrown.
        }
    } catch (const bad_alloc& e) {
        // If a memory allocation fails (e.g., system runs out of memory),
        // handle the exception gracefully by showing an error message.

        // lock_guard ensures only one thread accesses the console at a time to avoid garbled output.
        lock_guard<mutex> lock(consoleMutex);

        // Print the error message in red, along with details of the exception.
        cout << "\n" << RED << "Memory allocation failed: " << e.what() << RESET << endl;
    }

    // After the target memory is reached or an error occurs, the function enters this loop.
    // It keeps the memory blocks allocated until the test ends (running becomes false).
    while (running) {
        // Sleep for a short duration (100 ms) to avoid busy-waiting.
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    // No explicit cleanup is required for memoryBlocks because it stores unique_ptr.
    // When the vector (memoryBlocks) goes out of scope, all unique_ptrs in it automatically
    // release their allocated memory. This ensures no memory leaks.
}


int main() {
    initializeConsole();
    cout << MAGENTA << "\n=== System Stress Test Starting ===" << RESET << endl;
    cout << YELLOW << "Warning: This program will stress your system for " << TEST_DURATION << " seconds." << RESET << endl;
    cout << "Press Enter to continue...";
    cin.get();

    const int numCores = thread::hardware_concurrency();
    cout << BLUE << "\nDetected " << numCores << " CPU cores" << RESET << endl;
    cout << "\nStarting stress test...\n\n" << flush;

    auto startTime = chrono::steady_clock::now();

    vector<thread> cpuThreads;
    for (int i = 0; i < numCores; ++i) {
        cpuThreads.emplace_back(cpuStressTest, i);
    }

    thread memThread(memoryStressTest);

    while (running) {
        auto elapsedTime = chrono::steady_clock::now() - startTime;
        int elapsedSeconds = chrono::duration_cast<chrono::seconds>(elapsedTime).count();

        if (elapsedSeconds > TEST_DURATION) {
            break;
        }

        updateDisplay(elapsedSeconds);
        this_thread::sleep_for(chrono::milliseconds(250));
        moveCursorUp(2);
    }
    cout << endl;
    running = false;

    for (auto& thread : cpuThreads) {
        thread.join();
    }
    memThread.join();

    cout << "\n\n";

    auto endTime = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);

    cout << MAGENTA << "=== Test Results ===" << RESET << endl;
    cout << CYAN << "Total execution time: " << duration.count() / 1000.0 << " seconds" << RESET << endl;
    cout << CYAN << "Maximum memory allocated: " << memoryAllocated / (1024 * 1024) << "MB" << RESET << endl;
    cout << CYAN << "CPU cores utilized: " << numCores << RESET << endl;

    return 0;
}