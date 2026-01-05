#!/bin/bash
# Run all spatial audio tests

set -e

echo "========================================"
echo "  Spatial Audio Test Suite"
echo "========================================"
echo ""

cd "$(dirname "$0")"

# Build tests
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make

echo ""
echo "Running tests..."
echo "========================================"
echo ""

# Run each test
./test_spatial_audio_config
echo ""
./test_dolby_atmos_renderer
echo ""
./test_spatial_audio_bridge

echo ""
echo "========================================"
echo "✅ All spatial audio tests passed!"
echo "========================================"
