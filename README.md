# System Stress Test

A comprehensive C++ system stress testing tool that performs simultaneous CPU and memory stress testing with real-time monitoring and colorized console output.

## Features

- **Multi-threaded CPU stress testing** with hash-intensive operations
- **Memory stress testing** with configurable allocation targets
- **Real-time progress monitoring** with visual progress bars
- **Cross-platform support** (Windows and Unix-like systems)
- **Colorized console output** for better visibility
- **Automatic CPU core detection** and utilization
- **Thread-safe operations** with proper synchronization

## Demo

The tool displays real-time progress with:
- Time progress bar showing test duration
- Memory allocation progress with current usage
- Hash operations counter
- Colorized status messages

## Requirements

- **C++17 compatible compiler**
- **CMake 3.10 or higher**
- **Threading support** (pthread on Unix, native threads on Windows)

### Platform-specific requirements:
- **Windows**: Visual Studio 2017+ or MinGW-w64
- **Linux/macOS**: GCC 7+ or Clang 5+

## Installation

### Option 1: Using Nix (Recommended)

If you have Nix installed, simply run:

```bash
nix-shell
```

This will automatically:
- Set up the build environment
- Create the build directory
- Run CMake and compile the project
- Generate the executable in the `build` directory

### Option 2: Manual Build

1. **Clone the repository**:
   ```bash
   git clone <repository-url>
   cd SystemStressTest
   ```

2. **Create build directory**:
   ```bash
   mkdir build
   cd build
   ```

3. **Configure with CMake**:
   ```bash
   cmake ..
   ```

4. **Build the project**:
   ```bash
   make
   ```

## Usage

Run the executable from the build directory:

```bash
./SystemStressTest
```

The program will:
1. Display a warning about system stress testing
2. Prompt for confirmation to continue
3. Detect available CPU cores
4. Run stress tests for 30 seconds (configurable)
5. Display real-time progress and metrics
6. Show final test results

### Configuration

Key parameters can be modified in `include/SystemStressTest.h`:

```cpp
static constexpr int    BAR_WIDTH = 30;                     // Progress bar width for time and memory displays
static constexpr int    MULTIPLIER = 2;                     // Memory multiplier for stress test (resulting in a 2 GB Max Allocation)
static constexpr int    TEST_DURATION = 30;                 // seconds
static constexpr size_t TARGET_MEMORY = 1024 * 1024 * 1024; // 1 GB

// Memory bandwidth measurement constants
static constexpr size_t BANDWIDTH_TEST_SIZE = 64 * 1024 * 1024; // 64MB test buffer
static constexpr int    BANDWIDTH_ITERATIONS = 5;               // Number of iterations for averaging
```

## Project Structure

```
 ├──  include
 │   ├──  ConsoleColors.h
 │   ├──  ConsoleInitializer.h
 │   ├──  LinkedList.h
 │   ├──  SystemStressTest.h
 │   └──  TimeManager.h
 ├──  src
 │   ├──  ConsoleInitializer.cpp
 │   ├──  main.cpp
 │   ├──  SystemStressTest.cpp
 │   └──  TimeManager.cpp
 ├──  .gitignore
 ├──  CMakeLists.txt
 ├──  LICENSE
 ├── 󰂺 README.md
 └──  shell.nix
```

## Technical Details

### CPU Stress Testing
- Uses compute-intensive hash-like operations
- Spawns one thread per CPU core
- Performs batched operations for efficiency
- Uses atomic counters for thread-safe operation tracking

### Memory Stress Testing
- Allocates memory in 1MB blocks
- Uses custom linked list for memory management
- Employs RAII principles with smart pointers
- Handles allocation failures gracefully

### Key Libraries Used

| Library | Purpose |
|---------|---------|
| `<iostream>` | Console I/O operations |
| `<mutex>` | Thread synchronization |
| `<vector>` | Dynamic memory containers |
| `<thread>` | Multi-threading support |
| `<chrono>` | Time measurement |
| `<atomic>` | Thread-safe variables |

### Cross-Platform Considerations

- **Windows**: Uses Windows API for console initialization and UTF-8 support
- **Unix-like systems**: Uses ANSI escape sequences for colors and formatting
- **Thread management**: Uses C++11 standard threading library

## Safety Features

- **Memory allocation limits** to prevent system crashes
- **Graceful error handling** for allocation failures
- **Thread-safe console output** with mutex protection
- **Controlled test duration** to prevent indefinite stress

## Performance Metrics

The tool tracks and displays:
- **Hash operations per second**: Measures CPU performance
- **Memory allocation rate**: Tracks memory subsystem performance
- **Real-time progress**: Visual feedback during testing
- **Resource utilization**: Shows CPU cores and memory usage

## Troubleshooting

### Common Issues

1. **Build fails on Windows**:
   - Ensure you have Visual Studio Build Tools installed
   - Use Developer Command Prompt for VS

2. **Colors not displaying**:
   - On Windows, ensure you're using Windows 10 version 1607 or later
   - Try running from Windows Terminal instead of cmd.exe

3. **Memory allocation errors**:
   - Reduce the `MULTIPLIER` value in the header file
   - Ensure sufficient system memory is available

### Performance Considerations

- **High memory usage**: This is intentional for stress testing
- **CPU temperature**: Monitor system temperature during extended use
- **System responsiveness**: Close other applications for accurate testing

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Ensure code follows the existing style
5. Add appropriate comments and documentation
6. Test on multiple platforms if possible
7. Submit a pull request

## License

This project is open source. Please refer to the license file for details.

## Acknowledgments

- Built with modern C++17 features
- Uses standard library threading and atomic operations
- Inspired by system benchmarking and stress testing tools

---

**Warning**: This tool is designed to stress your system. Use responsibly and monitor system temperature and stability during testing.