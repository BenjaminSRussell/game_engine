#!/bin/bash
set -e

# 1. Build C Engine
echo "🏗️  Building C Game Engine..."
rm -rf build
mkdir -p build
cd build
cmake ../src/engine -DCMAKE_BUILD_TYPE=Release
make GameEngine -j$(sysctl -n hw.ncpu)
cd ..

# 2. Build Swift Editor
echo "🍏 Building MacOS Editor..."
cd src/editor
swift build -c release \
    -Xlinker -L../../build \
    -Xlinker -lGameEngine

# 3. Report
echo "✅ Build Complete!"
echo "Run the editor with: ./.build/release/VoxelForgeEditor"
