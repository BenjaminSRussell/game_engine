#!/bin/bash

# Build and Run 3D Rendering Pipeline Tests
# This script builds and executes the unit tests for the rendering pipeline

set -e

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  3D RENDERING PIPELINE - TEST BUILD & EXECUTION              ║"
echo "╚═══════════════════════════════════════════════════════════════╝"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Create build directory
BUILD_DIR="build_tests"
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${BLUE}📁 Creating build directory: $BUILD_DIR${NC}"
    mkdir -p "$BUILD_DIR"
fi

# Configure CMake
echo -e "\n${BLUE}⚙️  Configuring CMake...${NC}"
cd "$BUILD_DIR"
cmake -DCMAKE_BUILD_TYPE=Release ..
cd ..

# Build tests
echo -e "\n${BLUE}🔨 Building tests...${NC}"
cmake --build "$BUILD_DIR" --config Release

# Check if build succeeded
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✅ Build successful!${NC}\n"
else
    echo -e "${RED}❌ Build failed!${NC}\n"
    exit 1
fi

# Run tests
echo -e "${BLUE}▶️  Running tests...${NC}\n"
TEST_EXECUTABLE="$BUILD_DIR/bin/tests/test_rendering_pipeline"

if [ -f "$TEST_EXECUTABLE" ]; then
    "$TEST_EXECUTABLE"
    TEST_RESULT=$?
else
    echo -e "${RED}❌ Test executable not found: $TEST_EXECUTABLE${NC}"
    exit 1
fi

# Print summary
echo -e "\n${BLUE}═══════════════════════════════════════════════════════════${NC}"

if [ $TEST_RESULT -eq 0 ]; then
    echo -e "${GREEN}✅ ALL TESTS PASSED!${NC}"
    echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"
    exit 0
else
    echo -e "${RED}❌ SOME TESTS FAILED!${NC}"
    echo -e "${RED}═══════════════════════════════════════════════════════════${NC}"
    exit 1
fi
