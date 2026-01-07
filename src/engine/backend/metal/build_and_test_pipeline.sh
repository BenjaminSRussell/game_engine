#!/bin/bash
#
# build_and_test_pipeline.sh
# Compile and run Metal pipeline tests
#

set -e  # Exit on error

echo "=========================================="
echo "Metal Pipeline System - Build & Test"
echo "=========================================="
echo ""

# Configuration
METAL_DIR="/Users/benjaminrussell/Desktop/Minecraft v2/src/engine/rendering/3d_rendering/backend/metal"
OUTPUT_DIR="$METAL_DIR/build"
TEST_BINARY="$OUTPUT_DIR/test_pipeline"

# Create build directory
mkdir -p "$OUTPUT_DIR"

echo "📦 Compiling Metal pipeline system..."
echo ""

# Compile all source files
clang \
    -framework Metal \
    -framework Foundation \
    -o "$TEST_BINARY" \
    "$METAL_DIR/test_mtl_pipeline.m" \
    "$METAL_DIR/mtl_pipeline.m" \
    "$METAL_DIR/mtl_pipeline_extensions.m" \
    "$METAL_DIR/mtl_shader_library.m" \
    "$METAL_DIR/mtl_shader_library_extensions.m" \
    "$METAL_DIR/mtl_shader_compiler.m" \
    "$METAL_DIR/mtl_device.c" \
    -I"$METAL_DIR" \
    -Wno-deprecated-declarations \
    2>&1

if [ $? -eq 0 ]; then
    echo "✅ Compilation successful!"
    echo ""
    
    echo "🧪 Running tests..."
    echo ""
    "$TEST_BINARY"
    
    TEST_RESULT=$?
    echo ""
    
    if [ $TEST_RESULT -eq 0 ]; then
        echo "✅ All tests passed!"
    else
        echo "❌ Some tests failed (exit code: $TEST_RESULT)"
        exit 1
    fi
else
    echo "❌ Compilation failed!"
    exit 1
fi

echo ""
echo "=========================================="
echo "Build & Test Complete"
echo "=========================================="
