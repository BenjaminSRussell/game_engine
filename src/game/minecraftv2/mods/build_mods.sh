#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR/.."
MOD_DIR="$SCRIPT_DIR"

echo "Building mods..."

# Compile example_mod
gcc -fPIC -shared \
    -I"$PROJECT_DIR/include" \
    -I"$PROJECT_DIR/include_private" \
    "$MOD_DIR/example_mod.c" \
    -o "$MOD_DIR/libexample_mod.so" \
    -lm

echo "Mods built successfully!"
echo "Location: $MOD_DIR/*.so"
