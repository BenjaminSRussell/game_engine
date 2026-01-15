#!/bin/bash

echo "Verifying Asset System integration..."

# Compiler settings
CC=clang
CFLAGS="-std=c11 -Wall -Wextra -Werror -Wno-unused-parameter"

# Include paths
INCLUDES="-I. \
-ISource/Runtime/Core/Assets/Public \
-ISource/Runtime/Core/Assets/Private \
-ISource/Runtime/Rendering/Public \
-ISource/Runtime/Core/Memory/Public \
-ISource/Runtime/Core/Logging/Public \
-Isrc \
-Isrc/include"

# Asset system files
ASSET_SOURCES="
Source/Runtime/Core/Assets/Private/asset_registry.c
Source/Runtime/Core/Assets/Private/asset_hot_reload.c
"

# Test main
TEST_MAIN="Source/Runtime/Core/Assets/Private/asset_test_main.c"

# Dependencies
MEMORY_SOURCES="
Source/Runtime/Core/Memory/Private/memory_core.c
Source/Runtime/Core/Memory/Private/memory_tracking.c
Source/Runtime/Core/Memory/Private/memory_profiler.c
"

LOGGER_SOURCES="
Source/Runtime/Core/Logging/Private/logger_core.c
Source/Runtime/Core/Logging/Private/logger_sinks.c
Source/Runtime/Core/Logging/Private/logger_format.c
"

# Compile
echo "Compiling Asset System..."
$CC $CFLAGS $INCLUDES \
    $ASSET_SOURCES \
    $MEMORY_SOURCES \
    $LOGGER_SOURCES \
    $TEST_MAIN \
    -o asset_test 2>&1

if [ $? -ne 0 ]; then
    echo "Asset System compilation FAILED."
    exit 1
fi

echo "Asset System compiled successfully."
echo "Running Asset Verification..."
./asset_test

if [ $? -eq 0 ]; then
    echo ""
    echo "Asset Verification Successful!"
    rm -f asset_test
else
    echo ""
    echo "Asset Verification FAILED."
    exit 1
fi
