#!/bin/bash
# Build VoxelForge Swift Editor for macOS
# Requires: macOS 14+, Swift 6.0+, Xcode Command Line Tools

set -e  # Exit on error

echo "🎨 Building VoxelForge Swift Editor..."
echo "========================================="

# Colors
GREEN='\033[0.32m'
YELLOW='\033[0;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Check prerequisites
echo "Checking prerequisites..."

if ! command -v swift &> /dev/null; then
    echo -e "${RED}❌ Swift not found${NC}"
    echo "Install Xcode Command Line Tools: xcode-select --install"
    exit 1
fi

SWIFT_VERSION=$(swift --version | head -n 1)
echo -e "${GREEN}✓${NC} $SWIFT_VERSION"

# Navigate to editor directory
cd "$(dirname "$0")/src/editor"

# Clean build artifacts
echo ""
echo "Cleaning previous build..."
rm -rf .build
echo -e "${GREEN}✓${NC} Clean complete"

# Build C engine library first (if not already built)
echo ""
echo "Building C engine library..."
cd ../..

if [ ! -d "build" ]; then
    mkdir -p build
fi

cd build

# Configure with CMake (macOS only, no editor sources)
echo "Configuring CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="14.0" 2>&1 | grep -E "(Configuring|Building|Error|Warning)" || true

# Build just the engine library
echo "Building GameEngine library..."
cmake --build . --target EngineLib -j$(sysctl -n hw.ncpu) 2>&1 | grep -E "(Building|Linking|Error|Warning)" || true

if [ ! -f "libEngineLib.a" ]; then
    echo -e "${RED}❌ Failed to build libEngineLib.a${NC}"
    exit 1
fi

echo -e "${GREEN}✓${NC} GameEngine library built successfully"

# Return to editor directory  
cd ../src/editor

#Build Swift package
echo ""
echo "Building Swift Package..."
swift build --configuration release -Xswiftc -warnings-as-errors 2>&1 | tail -20

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓${NC} Swift build complete"
    
    # Show build artifacts
    echo ""
    echo "Build artifacts:"
    ls -lh .build/release/ | grep VoxelForge || echo "  (executable in .build/release/)"
    
    echo ""
    echo -e "${GREEN}=========================================${NC}"
    echo -e "${GREEN}✅ Build successful!${NC}"
    echo ""
    echo "Run the editor:"
    echo -e "  ${YELLOW}swift run VoxelForgeEditor${NC}"
    echo ""
    echo "Run tests:"
    echo -e "  ${YELLOW}swift test${NC}"
    echo ""
else
    echo -e "${RED}=========================================${NC}"
    echo -e "${RED}❌ Build failed${NC}"
    echo ""
    echo "Common issues:"
    echo "  1. Missing libEngineLib.a - build C engine first"
    echo "  2. Swift 6.0+ required - update Xcode"
    echo "  3. macOS 14+ required"
    exit 1
fi
