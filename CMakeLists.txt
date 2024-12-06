# Set minimum required CMake version
cmake_minimum_required(VERSION 3.30)

# Project name and language
project(Stress_test CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(EXE_NAME Stress_test.exe)

# Define source and header files
set(SOURCE_FILES src/main.cpp )
#set(HEADER_FILES include/Recursive_Combination.hpp include/Recursive_Combination.hpp)

# Define executable
add_executable(
               ${EXE_NAME}
               ${SOURCE_FILES}
               #${HEADER_FILES}
              )

# Find and link pthread library
find_package(Threads REQUIRED)
target_link_libraries(${EXE_NAME} PRIVATE Threads::Threads)

# Set binary output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)