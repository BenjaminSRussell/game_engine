#!/bin/bash

echo "Verifying Platform System integration..."

# Compiler settings
CC=clang
CFLAGS="-std=c11 -Wall -Wextra -Werror -Wno-unused-parameter"

# Get GLFW path
GLFW_PREFIX=$(brew --prefix glfw 2>/dev/null || echo "/opt/homebrew/opt/glfw")

# Include paths - add both new and legacy paths
INCLUDES="-I. \
-ISource/Runtime/Core/Memory/Public \
-ISource/Runtime/Core/Logging/Public \
-ISource/Runtime/Core/Platform/Public \
-ISource/Runtime/Core/Platform/Private \
-I$GLFW_PREFIX/include \
-Isrc \
-Isrc/include \
-Isrc/engine/include \
-Isrc/core/Include"

# Library paths
LIBS="-L$GLFW_PREFIX/lib -lglfw -framework Cocoa -framework IOKit -framework CoreVideo"

# Platform module files
PLATFORM_SOURCES="
Source/Runtime/Core/Platform/Private/platform_window.c
Source/Runtime/Core/Platform/Private/Input/input_core.c
Source/Runtime/Core/Platform/Private/Input/input_actions.c
Source/Runtime/Core/Platform/Private/Input/input_bindings.c
Source/Runtime/Core/Platform/Private/Input/input_events.c
Source/Runtime/Core/Platform/Private/Input/input_queries.c
"

# Test main
TEST_MAIN="Source/Runtime/Core/Platform/Private/platform_test_main.c"

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
echo "Compiling Platform System..."
$CC $CFLAGS $INCLUDES \
    $PLATFORM_SOURCES \
    $MEMORY_SOURCES \
    $LOGGER_SOURCES \
    $TEST_MAIN \
    $LIBS \
    -o platform_test 2>&1

if [ $? -ne 0 ]; then
    echo "Platform System compilation FAILED."
    exit 1
fi

echo "Platform System compiled successfully."
echo "Running Platform Verification..."
./platform_test

if [ $? -eq 0 ]; then
    echo ""
    echo "Platform Verification Successful!"
    rm -f platform_test
else
    echo ""
    echo "Platform Verification FAILED."
    exit 1
fi
