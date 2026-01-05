#!/bin/bash
set -e

echo "Building Physics Sandbox Demo..."

# Find source files
# Core: Need logger, memory, window (exclude vec3.c,mat4.c,quat.c - missing headers)
CORE_SRCS="src/engine/core/logger.c src/engine/core/memory.c src/engine/core/window.c src/engine/core/utils.c src/engine/core/string_utils.c"

# Physics: only core implementation, exclude advanced/collision subdirs with broken files
PHYS_SRCS="src/engine/physics/physics_core_impl.c src/engine/physics/physics_api_impl.c src/engine/physics/rigid_body/rigid_body_impl.c src/engine/physics/collision/collision_primitive.c src/engine/physics/collision/broadphase.c"

# Output binary
OUT="demo_sandbox"

# Compile
gcc -std=c99 -g \
    src/game/demo_physics_sandbox.c \
    $CORE_SRCS \
    $PHYS_SRCS \
    -I src/engine/include \
    -I /opt/homebrew/include \
    -L /opt/homebrew/lib \
    -lglfw \
    -framework Cocoa -framework OpenGL -framework IOKit \
    -o $OUT

echo "Build successful! Run with ./$OUT"
