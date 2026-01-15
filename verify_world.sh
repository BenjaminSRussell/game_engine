#!/bin/bash

echo "Verifying World System integration..."

# Compiler settings
CC=clang
CFLAGS="-std=c11 -Wall -Wextra -Werror -Wno-unused-parameter"

# Include paths
INCLUDES="-I. \
-ISource/Runtime/World/Public \
-ISource/Runtime/World/Private \
-ISource/Runtime/Core/Memory/Public \
-ISource/Runtime/Core/Logging/Public \
-Isrc \
-Isrc/include"

# World system files
WORLD_SOURCES="
Source/Runtime/World/Private/world_core.c
"

# Test main
TEST_MAIN="Source/Runtime/World/Private/world_test_main.c"

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
echo "Compiling World System..."
$CC $CFLAGS $INCLUDES \
    $WORLD_SOURCES \
    $MEMORY_SOURCES \
    $LOGGER_SOURCES \
    $TEST_MAIN \
    -lm \
    -o world_test 2>&1

if [ $? -ne 0 ]; then
    echo "World System compilation FAILED."
    exit 1
fi

echo "World System compiled successfully."
echo "Running World Verification..."
./world_test

if [ $? -eq 0 ]; then
    echo ""
    echo "World Verification Successful!"
    rm -f world_test
else
    echo ""
    echo "World Verification FAILED."
    exit 1
fi
