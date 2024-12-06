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

 <iomanip>: Output formatting
 - setprecision(): Used in displayMemoryStatus() for memory size formatting
 - fixed: Used for floating-point number formatting

 <atomic>: Thread-safe variables
 - atomic<bool> running: Controls test termination
 - atomic<size_t> memoryAllocated: Tracks allocated memory

 <cstring>: String manipulation (C-style)
 - Not directly used in shown code, could be removed
 - Typically used with C-style strings

 <locale>: Localization support
 - Not directly used in shown code, could be removed
 - Often used for formatting numbers/dates
```