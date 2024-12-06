# Stress tester


## why we used these libraries:

```
 <iostream>: Used throughout for console output/input
 - cout/cin: Used in main() for progress bars and user input
 - endl/flush: Used in display functions for output formatting

 <mutex>: Synchronizes access to shared resources
 - Used in displayMemoryStatus() and displayTimeProgress() via consoleMutex
 - Prevents garbled output when multiple threads write to console

 <vector>: Dynamic array container 
 - Used in memoryStressTest() to store allocated memory blocks
 - Used in main() to store CPU stress test threads

 <thread>: Provides threading support
 - Used in main() to create CPU stress test threads
 - Used in memoryStressTest() for parallel memory allocation

 <chrono>: Time-related functionality
 - Used in main() for test duration timing
 - Used in memoryStressTest() for sleep delays

 <atomic>: Thread-safe variables
 - atomic<bool> running: Controls test termination
 - atomic<size_t> memoryAllocated: Tracks allocated memory

```