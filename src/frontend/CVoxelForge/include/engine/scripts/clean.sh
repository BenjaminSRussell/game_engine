#!/bin/bash
# Remove build artifacts for the engine and app.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR/.."

echo "Cleaning build artifacts..."

rm -rf "$PROJECT_DIR/build"
rm -rf "$PROJECT_DIR/App/build"

echo "Clean complete!"
