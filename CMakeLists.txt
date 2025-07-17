cmake_minimum_required(VERSION 3.10)
project(SystemStressTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add compiler flags for optimization and warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

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
    src/SystemStressTest.cpp
    src/TimeManager.cpp
)

# Header files
set(HEADERS
    include/ConsoleInitializer.h
    include/ConsoleColors.h
    include/LinkedList.h
    include/SystemStressTest.h
    include/TimeManager.h
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