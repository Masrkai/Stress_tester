#!/usr/bin/env bash

# Create and navigate to the build directory (default release build)
echo ""
build_dir="build"
mkdir -p "$build_dir"
cd "$build_dir"

# Run CMake and Make for release build
echo "Building Release Version..."
cmake -DCMAKE_BUILD_TYPE=Release ..
make

echo "Release build completed in '$build_dir' directory!"

# Return to project root
cd ..