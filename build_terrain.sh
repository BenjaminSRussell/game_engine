#!/bin/bash
set -e

echo "Building Simple Terrain Demo..."

# Core sources (minimal - just window and logger)
CORE_SRCS="src/engine/core/logger.c src/engine/core/memory.c src/engine/core/window.c src/engine/core/utils.c src/engine/core/string_utils.c"

# Output binary
OUT="demo_terrain"

# Compile
gcc -std=c99 -g \
    src/game/demo_simple_terrain.c \
    $CORE_SRCS \
    -I src/engine/include \
    -I /opt/homebrew/include \
    -L /opt/homebrew/lib \
    -lglfw \
    -framework Cocoa -framework OpenGL -framework IOKit \
    -o $OUT

echo "Build successful! Run with ./$OUT"
