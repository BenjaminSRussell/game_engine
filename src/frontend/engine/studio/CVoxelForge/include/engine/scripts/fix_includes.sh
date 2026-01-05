#!/bin/bash
# scripts/fix_includes.sh
# Updates include paths to use clean angle brackets universally
# Target: src/engine/**/*.c AND include/**/*.h

echo "Fixing all includes..."

# Function to fix includes in a file
fix_file_includes() {
    local file=$1
    # Replace ../../../include/X with <X>
    sed -i.bak 's|#include "\.\./\.\./\.\./include/\(.*\)"|#include <\1>|g' "$file"
    # Replace ../../include/X with <X>
    sed -i.bak 's|#include "\.\./\.\./include/\(.*\)"|#include <\1>|g' "$file"
    # Replace ../include/X with <X>
    sed -i.bak 's|#include "\.\./include/\(.*\)"|#include <\1>|g' "$file"
    
    # Fix relative paths from headers to other headers
    # e.g. "../math/math.h" -> <math/math.h>
    sed -i.bak 's|#include "\.\./math/\(.*\)"|#include <math/\1>|g' "$file"
    sed -i.bak 's|#include "\.\./renderer.h"|#include <engine/renderer.h>|g' "$file"
}

# Process C files in engine
find . -name "*.c" -type f | while read file; do
    fix_file_includes "$file"
done

# Process H files in include directory (relative path from src/engine is ../../include)
find ../../include -name "*.h" -type f | while read file; do
    fix_file_includes "$file"
done

echo "Cleaning backup files..."
find . -name "*.bak" -delete
find ../../include -name "*.bak" -delete

echo "Include path updates complete!"
