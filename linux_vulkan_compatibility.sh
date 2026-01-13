#!/bin/bash

# Linux Vulkan Backend Compatibility Verification Script
# This script verifies Vulkan backend compatibility for deployment

echo "=========================================="
echo "Linux Vulkan Backend Compatibility Check"
echo "=========================================="

# Check if we're on Linux
if [[ "$OSTYPE" != "linux-gnu"* ]]; then
    echo "WARNING: This script is designed for Linux systems"
    echo "Current OS: $OSTYPE"
fi

# Check Vulkan SDK installation
echo ""
echo "1. Checking Vulkan SDK Installation..."
if command -v vulkaninfo &> /dev/null; then
    echo "✅ vulkaninfo found"
    vulkaninfo --summary | head -20
else
    echo "❌ vulkaninfo not found"
    echo "   Install Vulkan SDK with: sudo apt install vulkan-tools"
fi

# Check Vulkan headers
echo ""
echo "2. Checking Vulkan Headers..."
if [ -f "/usr/include/vulkan/vulkan.h" ] || [ -f "/usr/local/include/vulkan/vulkan.h" ]; then
    echo "✅ Vulkan headers found"
    find /usr -name "vulkan.h" 2>/dev/null | head -5
else
    echo "❌ Vulkan headers not found"
    echo "   Install with: sudo apt install libvulkan-dev"
fi

# Check Vulkan libraries
echo ""
echo "3. Checking Vulkan Libraries..."
if ldconfig -p | grep -q libvulkan; then
    echo "✅ Vulkan libraries found"
    ldconfig -p | grep libvulkan
else
    echo "❌ Vulkan libraries not found"
    echo "   Install with: sudo apt install libvulkan1"
fi

# Check validation layers
echo ""
echo "4. Checking Validation Layers..."
if [ -d "/usr/share/vulkan/implicit_layer.d" ] || [ -d "/usr/local/share/vulkan/implicit_layer.d" ]; then
    echo "✅ Validation layers available"
    find /usr -name "*validation*.json" 2>/dev/null | head -3
else
    echo "❌ Validation layers not found"
    echo "   Install with: sudo apt install vulkan-validationlayers"
fi

# Check GPU drivers
echo ""
echo "5. Checking GPU Drivers..."
if command -v nvidia-smi &> /dev/null; then
    echo "✅ NVIDIA drivers detected"
    nvidia-smi --query-gpu=name,driver_version --format=csv,noheader
elif lspci | grep -i "AMD\|Radeon"; then
    echo "✅ AMD GPU detected"
    lspci | grep -i "AMD\|Radeon" | head -3
elif lspci | grep -i "Intel.*Graphics"; then
    echo "✅ Intel GPU detected"
    lspci | grep -i "Intel.*Graphics" | head -3
else
    echo "⚠️  Unknown GPU configuration"
    lspci | grep -i "VGA\|3D" | head -3
fi

# Check CMake Vulkan configuration
echo ""
echo "6. Checking CMake Vulkan Configuration..."
CMAKE_FILE="CMakeLists.txt"
if grep -q "VULKAN_BUILD" "$CMAKE_FILE"; then
    echo "✅ VULKAN_BUILD flag found in CMakeLists.txt"
else
    echo "⚠️  VULKAN_BUILD flag not explicitly defined"
fi

if grep -q "src/engine/backend/vulkan" cmake/sources.cmake; then
    echo "✅ Vulkan sources configured for Linux"
else
    echo "❌ Vulkan sources not properly configured"
fi

# Check source files
echo ""
echo "7. Checking Vulkan Source Files..."
VULKAN_DIR="src/engine/backend/vulkan"
if [ -d "$VULKAN_DIR" ]; then
    echo "✅ Vulkan backend directory exists"
    echo "   Source files found:"
    find "$VULKAN_DIR" -name "*.c" | wc -l | xargs echo "   - .c files:"
    find "$VULKAN_DIR" -name "*.h" | wc -l | xargs echo "   - .h files:"
    
    # Check key files
    KEY_FILES=("vulkan.c" "vulkan_capabilities.c" "vulkan_backend.c")
    for file in "${KEY_FILES[@]}"; do
        if [ -f "$VULKAN_DIR/$file" ]; then
            echo "   ✅ $file"
        else
            echo "   ❌ $file missing"
        fi
    done
else
    echo "❌ Vulkan backend directory not found"
fi

# Check build system
echo ""
echo "8. Checking Build System..."
BUILD_DIR="build"
if [ -d "$BUILD_DIR" ]; then
    echo "✅ Build directory exists"
    if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
        echo "✅ CMake cache found"
        if grep -q "VULKAN" "$BUILD_DIR/CMakeCache.txt"; then
            echo "✅ Vulkan configuration in build cache"
        else
            echo "⚠️  Vulkan not configured in build"
        fi
    else
        echo "⚠️  CMake cache not found - run cmake first"
    fi
else
    echo "⚠️  Build directory not found"
fi

# Compilation test
echo ""
echo "9. Testing Vulkan Compilation..."
TEST_FILE="test_vulkan_compile.c"
cat > "$TEST_FILE" << 'EOF'
#include <vulkan/vulkan.h>
#include <stdio.h>

int main() {
    VkResult result = vkEnumerateInstanceVersion(NULL);
    printf("Vulkan compilation test: %s\n", result == VK_SUCCESS ? "SUCCESS" : "FAILED");
    return 0;
}
EOF

if gcc -I/usr/include -lvulkan "$TEST_FILE" -o test_vulkan 2>/dev/null; then
    echo "✅ Vulkan compilation successful"
    ./test_vulkan
    rm -f test_vulkan
else
    echo "❌ Vulkan compilation failed"
    echo "   Missing headers or libraries"
fi

rm -f "$TEST_FILE"

echo ""
echo "=========================================="
echo "Linux Vulkan Compatibility Check Complete"
echo "=========================================="

# Summary
echo ""
echo "DEPLOYMENT READINESS SUMMARY:"
echo "- Vulkan SDK: $(command -v vulkaninfo &> /dev/null && echo "✅ Ready" || echo "❌ Missing")"
echo "- Headers: $([ -f "/usr/include/vulkan/vulkan.h" ] && echo "✅ Ready" || echo "❌ Missing")"
echo "- Libraries: $(ldconfig -p | grep -q libvulkan && echo "✅ Ready" || echo "❌ Missing")"
echo "- Validation: $([ -d "/usr/share/vulkan/implicit_layer.d" ] && echo "✅ Ready" || echo "⚠️  Optional")"
echo "- Source Code: $([ -d "$VULKAN_DIR" ] && echo "✅ Ready" || echo "❌ Missing")"
echo "- Build Config: $(grep -q "src/engine/backend/vulkan" cmake/sources.cmake && echo "✅ Ready" || echo "❌ Missing")"

echo ""
echo "RECOMMENDATIONS FOR DEPLOYMENT:"
echo "1. Install Vulkan SDK: sudo apt install vulkan-tools vulkan-sdk"
echo "2. Install development packages: sudo apt install libvulkan-dev vulkan-validationlayers-dev"
echo "3. Ensure GPU drivers support Vulkan 1.2+"
echo "4. Test with: cmake -DVULKAN_BUILD=ON && make"
echo "5. Verify with vulkaninfo before deployment"
