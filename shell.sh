echo "Automatic Build Environment"

# Create and navigate to the build directory
build_dir="build"
mkdir -p "$build_dir"
cd "$build_dir"

# Run CMake and Make
cmake ..
make

echo "Build completed. You can find the output in the '$build_dir' directory."
exit