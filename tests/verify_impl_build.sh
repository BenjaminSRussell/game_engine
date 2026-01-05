#!/bin/bash
# Verify Batch Implementation Build
# Compiles the new batch implementation files to ensure syntax correctness

set -e

echo "==========================================="
echo "  VERIFYING BATCH IMPLEMENTATIONS"
echo "==========================================="

CC=gcc
CFLAGS="-std=c99 -Wall -Wextra -c" # Compile only, don't link

verify_file() {
    local file=$1
    echo "Checking: $file..."
    if [ -f "$file" ]; then
        $CC $CFLAGS "$file" -o /dev/null
        echo "✅ Syntax OK"
    else
        echo "❌ File not found: $file"
        exit 1
    fi
}

# New batch files
verify_file "src/engine/cinematics/cinematics_batch_impl.c"
verify_file "src/engine/world_building/world_batch_impl.c"
verify_file "src/engine/multiplayer_framework/core_batch_impl.c"

# Consolidated tests
verify_file "tests/engine/test_render_graph.c"
verify_file "tests/engine/test_material_system.c"
verify_file "tests/engine/test_post_processing.c"

echo ""
echo "🎉 All batch implementations valid!"
