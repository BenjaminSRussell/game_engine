#!/bin/bash
# Build the engine library in Release mode using CMake.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR/.."
BUILD_DIR="$PROJECT_DIR/build"

echo "Building Minecraft Engine..."

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -- -j$(sysctl -n hw.ncpu)

echo "Build complete! Library at: $BUILD_DIR"
