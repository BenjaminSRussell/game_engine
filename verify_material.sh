#!/bin/bash

echo "Verifying Material System integration..."

# Compiler settings
CC=clang
CFLAGS="-std=c11 -Wall -Wextra -Werror -Wno-unused-parameter"

# Include paths
INCLUDES="-I. \
-ISource/Runtime/Rendering/Public \
-ISource/Runtime/Rendering/Private \
-ISource/Runtime/Core/Memory/Public \
-ISource/Runtime/Core/Logging/Public \
-Isrc \
-Isrc/include"

# Material system files
MATERIAL_SOURCES="
Source/Runtime/Rendering/Private/Materials/material_core.c
Source/Runtime/Rendering/Private/Materials/material_params.c
"

# Shader system (dependency)
SHADER_SOURCES="
Source/Runtime/Rendering/Private/Shaders/shader_manager.c
Source/Runtime/Rendering/Private/Shaders/shader_compiler.c
"

# Renderer core (dependency)
RENDERER_SOURCES="
Source/Runtime/Rendering/Private/Core/renderer_core.c
Source/Runtime/Rendering/Private/RHI/Backends/metal_rhi.c
Source/Runtime/Rendering/Private/RHI/Backends/backend_stubs.c
"

# Test main
TEST_MAIN="Source/Runtime/Rendering/Private/Materials/material_test_main.c"

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
echo "Compiling Material System..."
$CC $CFLAGS $INCLUDES \
    $MATERIAL_SOURCES \
    $SHADER_SOURCES \
    $RENDERER_SOURCES \
    $MEMORY_SOURCES \
    $LOGGER_SOURCES \
    $TEST_MAIN \
    -o material_test 2>&1

if [ $? -ne 0 ]; then
    echo "Material System compilation FAILED."
    exit 1
fi

echo "Material System compiled successfully."
echo "Running Material Verification..."
./material_test

if [ $? -eq 0 ]; then
    echo ""
    echo "Material Verification Successful!"
    rm -f material_test
else
    echo ""
    echo "Material Verification FAILED."
    exit 1
fi
