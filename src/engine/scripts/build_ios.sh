#!/bin/bash
# Build the engine for iOS using the Xcode CMake generator.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR/.."
BUILD_DIR="$PROJECT_DIR/build/ios"

echo "Building Minecraft for iOS..."

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. \
    -G Xcode \
    -DCMAKE_TOOLCHAIN_FILE=/opt/homebrew/share/cmake/ios.toolchain.cmake \
    -DIOS_PLATFORM=OS \
    -DIOS_ARCH=arm64 \
    -DCMAKE_BUILD_TYPE=Release

cmake --build . --config Release

echo "iOS build complete!"
