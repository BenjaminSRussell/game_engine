#!/bin/bash
set -e

# Setup paths
ROOT_DIR="$(pwd)/src/engine"
INCLUDE_DIR="$ROOT_DIR/include"
RENDER_DIR="$ROOT_DIR/rendering/3d_rendering"

echo "Verifying Integration Files..."

# Compile Integration (Syntax check only, no linking)
echo "Compiling render_config.c..."
clang -c "$ROOT_DIR/integration/render_config.c" -I "$INCLUDE_DIR" -o /dev/null

echo "Compiling render_world.c..."
clang -x objective-c -c "$ROOT_DIR/integration/render_world.c" -I "$INCLUDE_DIR" -I "$ROOT_DIR" -framework Metal -framework Foundation -o /dev/null

echo "Compiling scene_renderer.c..."
clang -x objective-c -c "$ROOT_DIR/integration/scene_renderer.c" -I "$INCLUDE_DIR" -I "$ROOT_DIR" -I "$RENDER_DIR" -framework Metal -framework Foundation -o /dev/null

echo "Compiling Testing Framework..."
clang -x objective-c -c "$ROOT_DIR/testing/render_tests.c" -I "$INCLUDE_DIR" -I "$ROOT_DIR" -framework Metal -framework Foundation -o /dev/null
clang -x objective-c -c "$ROOT_DIR/testing/visual_tests.c" -I "$INCLUDE_DIR" -I "$ROOT_DIR" -framework Metal -framework Foundation -o /dev/null
clang -x objective-c -c "$ROOT_DIR/testing/benchmark_suite.c" -I "$INCLUDE_DIR" -I "$ROOT_DIR" -framework Metal -framework Foundation -o /dev/null

echo "Verification Successful!"
