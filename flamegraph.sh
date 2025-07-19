#!/bin/bash
# flamegraph.sh - Automated Flamegraph Generator for SystemStressTest
# Place this script in the root directory of your project


set -e

# Use profiling build - optimized but with frame pointers and debug symbols
BINARY="./profiling/SystemStressTest"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
OUTPUT_DIR="flamegraphs"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Create output directory
mkdir -p "$OUTPUT_DIR"

echo -e "${BLUE}Starting flamegraph generation for SystemStressTest...${NC}"

# Check if profiling binary exists
if [ ! -f "$BINARY" ]; then
    echo -e "${RED}Error: Profiling binary $BINARY not found.${NC}"
    echo "Please build the profiling version first:"
    echo "  ./shell.sh profiling"
    echo "  OR"
    echo "  mkdir -p profiling && cd profiling && cmake -DCMAKE_BUILD_TYPE=Profiling .. && make"
    exit 1
fi

# Verify that the binary has debug symbols
if ! objdump -h "$BINARY" | grep -q ".debug_info"; then
    echo -e "${YELLOW}Warning: Binary may not have debug symbols for readable flamegraphs${NC}"
    echo "Consider rebuilding with: cmake -DCMAKE_BUILD_TYPE=Profiling"
fi

# Check if running with appropriate permissions
if [ "$EUID" -ne 0 ]; then
    echo -e "${YELLOW}Note: This script requires root privileges for perf.${NC}"
    echo "Re-running with sudo..."
    exec sudo -E "$0" "$@"
fi

# Adjust kernel parameters for better profiling
echo -e "${YELLOW}Adjusting kernel parameters for profiling...${NC}"
echo 1 > /proc/sys/kernel/perf_event_paranoid
echo 0 > /proc/sys/kernel/kptr_restrict

# Function to cleanup on exit
cleanup() {
    echo -e "${YELLOW}Cleaning up temporary files...${NC}"
    rm -f perf_*.data
    # Reset kernel parameters
    echo 3 > /proc/sys/kernel/perf_event_paranoid 2>/dev/null || true
    echo 1 > /proc/sys/kernel/kptr_restrict 2>/dev/null || true
}
trap cleanup EXIT

# Generate different types of flamegraphs
echo -e "${GREEN}1. Generating CPU cycles flamegraph...${NC}"
perf record -F 997 -g --call-graph dwarf -e cycles -o perf_cpu.data "$BINARY" 2>/dev/null || {
    echo -e "${RED}Failed to record CPU data${NC}"
    exit 1
}
perf script -i perf_cpu.data | flamegraph.pl --title "CPU Cycles - Profiling Build - $TIMESTAMP" > "$OUTPUT_DIR/cpu_${TIMESTAMP}.svg"
echo -e "${GREEN}   ✓ CPU flamegraph saved to: $OUTPUT_DIR/cpu_${TIMESTAMP}.svg${NC}"

echo -e "${GREEN}2. Generating memory access flamegraph...${NC}"
perf record -F 99 -g --call-graph dwarf -e page-faults,cache-misses -o perf_memory.data "$BINARY" 2>/dev/null || {
    echo -e "${YELLOW}Warning: Memory events recording failed, generating basic flamegraph${NC}"
    perf record -F 99 -g --call-graph dwarf -o perf_memory.data "$BINARY" 2>/dev/null
}
perf script -i perf_memory.data | flamegraph.pl --title "Memory Access - Profiling Build - $TIMESTAMP" > "$OUTPUT_DIR/memory_${TIMESTAMP}.svg"
echo -e "${GREEN}   ✓ Memory flamegraph saved to: $OUTPUT_DIR/memory_${TIMESTAMP}.svg${NC}"

echo -e "${GREEN}3. Generating thread activity flamegraph...${NC}"
perf record -F 99 -g --call-graph dwarf -s -o perf_threads.data "$BINARY" 2>/dev/null || {
    echo -e "${RED}Failed to record thread data${NC}"
    exit 1
}
perf script -i perf_threads.data | flamegraph.pl --title "Multi-threaded Activity - Profiling Build - $TIMESTAMP" > "$OUTPUT_DIR/threads_${TIMESTAMP}.svg"
echo -e "${GREEN}   ✓ Thread flamegraph saved to: $OUTPUT_DIR/threads_${TIMESTAMP}.svg${NC}"

# Generate performance statistics
echo -e "${GREEN}4. Generating performance statistics...${NC}"
perf stat -o "$OUTPUT_DIR/stats_${TIMESTAMP}.txt" "$BINARY" 2>&1 || {
    echo -e "${YELLOW}Warning: Performance statistics generation failed${NC}"
}

# Optional: Generate differential flamegraph if previous run exists
PREV_CPU=$(ls -t "$OUTPUT_DIR"/cpu_*.svg 2>/dev/null | sed -n '2p')
if [ -n "$PREV_CPU" ] && [ -f "$PREV_CPU" ]; then
    echo -e "${GREEN}5. Generating differential flamegraph...${NC}"
    # This would require additional tooling - placeholder for now
    echo -e "${YELLOW}   Differential flamegraph requires additional setup${NC}"
fi

echo -e "${BLUE}Flamegraph generation completed!${NC}"
echo -e "${BLUE}Generated files:${NC}"
ls -la "$OUTPUT_DIR/"*${TIMESTAMP}.*

echo -e "${BLUE}Analysis Tips:${NC}"
echo "• Open SVG files in a web browser for interactive exploration"
echo "• Look for wide bars (high CPU usage) and tall stacks (deep calls)"
echo "• Focus on your hash function and memory allocation patterns"
echo "• Check thread synchronization overhead around mutex operations"
echo "• This profiling build provides accurate performance data with readable symbols"

echo -e "${BLUE}Quick Commands:${NC}"
echo "• View in browser: firefox $OUTPUT_DIR/cpu_${TIMESTAMP}.svg"
echo "• Search functions: Use Ctrl+F in the browser"
echo "• Zoom in: Click on function bars"

# # Optional: Open in browser if available
# if command -v firefox >/dev/null 2>&1; then
#     read -p "Open CPU flamegraph in Firefox? (y/n): " -n 1 -r
#     echo
#     if [[ $REPLY =~ ^[Yy]$ ]]; then
#         firefox "$OUTPUT_DIR/cpu_${TIMESTAMP}.svg" &
#     fi
# fi

echo -e "${GREEN}Flamegraph generation complete!${NC}"
echo -e "${BLUE}Build used: Profiling (optimized + debug symbols + frame pointers)${NC}"