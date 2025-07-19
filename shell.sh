echo "Automatic Build Environment"

# Create and navigate to the build directory
build_dir="build"
mkdir -p "$build_dir"
cd "$build_dir"

# Run CMake and Make
cmake ..
make

# From any build directory
# make build-release    # Builds release in build/
# make build-debug      # Builds debug in debug/
# make build-profiling  # Builds profiling in profiling/

echo "Build completed. You can find the output in the '$build_dir' directory."
exit