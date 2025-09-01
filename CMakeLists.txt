#? Project Initialization and Compiler Settings
cmake_minimum_required(VERSION 3.10)
project(SystemStressTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# a comment to check collaboration ability!









#? Unit Testing Configuration with GoogleTest

# Enable testing
enable_testing()

# Find or fetch GoogleTest
find_package(GTest QUIET)

# Test source files (exclude main.cpp to avoid multiple main() definitions)
set(TEST_SOURCES
    src/ConsoleInitializer.cpp
    src/TimeManager.cpp
    src/CPUStressTest.cpp
    src/MemoryStressTest.cpp
)

# Test files
set(TEST_FILES
    tests/test_cpustresstest.cpp
    tests/test_linkedlist.cpp
    tests/test_memorystresstest.cpp
    # tests/test_timemanager.cpp

    # Add more test files here as you create them
    # tests/test_memorystresstest.cpp
    # tests/test_timemanager.cpp
)

# Create test executable
add_executable(${PROJECT_NAME}_tests ${TEST_SOURCES} ${TEST_FILES})

# Include directories for tests
target_include_directories(${PROJECT_NAME}_tests PRIVATE include)

# Link libraries for tests
target_link_libraries(${PROJECT_NAME}_tests
    gtest_main
    gtest
    Threads::Threads
)










#? Build Configuration, Compiler Flags, and Targets
# Build type handling
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# Base compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# Build-specific flags
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -DNDEBUG")
elseif(CMAKE_BUILD_TYPE STREQUAL "Profiling")
    # Profiling build: optimized but with debug info and frame pointers
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g3 -fno-omit-frame-pointer")
    # Optional: preserve more function boundaries for clearer flame graphs
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-inline-small-functions")
    # Add profiling-specific defines
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPROFILING_BUILD")
endif()

# Platform-specific flags
if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32")
endif()

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/main.cpp
    src/ConsoleInitializer.cpp
    src/TimeManager.cpp

    src/CPUStressTest.cpp
    src/MemoryStressTest.cpp
)

# Header files
set(HEADERS
    include/ConsoleInitializer.hpp
    include/ConsoleColors.hpp
    include/LinkedList.hpp
    include/TimeManager.hpp

    include/CPUStressTest.hpp
    include/MemoryStressTest.hpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link threading library
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} Threads::Threads)













# Platform-specific libraries
if(WIN32)
    target_link_libraries(${PROJECT_NAME} kernel32)
endif()

# Custom targets for different build types
add_custom_target(build-release
    COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_SOURCE_DIR}/build
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_SOURCE_DIR}/build
    COMMENT "Building Release version in build/"
)

add_custom_target(build-profiling
    COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Profiling -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_SOURCE_DIR}/profiling
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_SOURCE_DIR}/profiling
    COMMENT "Building Profiling version in profiling/"
)