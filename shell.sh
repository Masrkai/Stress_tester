#!/bin/bash

echo "Automatic Build Environment"

# Create and navigate to the build directory (default release build)
build_dir="build"
mkdir -p "$build_dir"
cd "$build_dir"

# Run CMake and Make for release build
echo "Building Release version..."
cmake -DCMAKE_BUILD_TYPE=Release ..
make

echo "Release build completed in '$build_dir' directory."

# Return to project root
cd ..

# Also build profiling version automatically
echo ""
echo "Building Profiling version for performance analysis..."
profiling_dir="profiling"
mkdir -p "$profiling_dir"
cd "$profiling_dir"

cmake -DCMAKE_BUILD_TYPE=Profiling ..
make

echo "Profiling build completed in '$profiling_dir' directory."

# Return to project root
cd ..

echo ""
echo "=== Build Summary ==="
echo "✓ Release build available in: ./build/"
echo "✓ Profiling build available in: ./profiling/"
echo ""
echo "Available commands:"
echo "  make build-release    # Builds release in build/"
echo "  make build-debug      # Builds debug in debug/"
echo "  make build-profiling  # Builds profiling in profiling/"
echo ""
echo "To run performance analysis:"
echo "  ./profile.sh [program_arguments]"

# Don't exit automatically - let user stay in the shell
# exit