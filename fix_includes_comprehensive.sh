#!/bin/bash

# Comprehensive include path fix script
echo "Starting comprehensive include path fixes..."

# Fix remaining math/math.h includes
find src -name "*.c" -o -name "*.h" | xargs grep -l "include.*/math/math.h" | while read file; do
    echo "Fixing math includes in $file"
    sed -i '' 's|#include.*/math/math.h|#include "engine/include/math/math.h"|g' "$file"
done

# Fix remaining core/logger.h includes
find src -name "*.c" -o -name "*.h" | xargs grep -l "include.*/core/logger.h" | while read file; do
    echo "Fixing logger includes in $file"
    sed -i '' 's|#include.*/core/logger.h|#include "engine/include/core/logger.h"|g' "$file"
done

# Fix remaining core/memory.h includes
find src -name "*.c" -o -name "*.h" | xargs grep -l "include.*/core/memory.h" | while read file; do
    echo "Fixing memory includes in $file"
    sed -i '' 's|#include.*/core/memory.h|#include "engine/include/core/memory.h"|g' "$file"
done

# Fix remaining common.h includes
find src -name "*.c" -o -name "*.h" | xargs grep -l "include.*/common.h" | while read file; do
    echo "Fixing common includes in $file"
    sed -i '' 's|#include.*/common.h|#include "engine/include/common.h"|g' "$file"
done

# Fix remaining include patterns with ../include/
find src -name "*.c" -o -name "*.h" | xargs grep -l "../include/" | while read file; do
    echo "Fixing relative includes in $file"
    sed -i '' 's|#include "../include/|#include "engine/include/|g' "$file"
done

echo "Include path fixes completed!"

# Count remaining errors
REMAINING_ERRORS=$(find src -name "*.c" -exec gcc -I src -I include -Wall -Wextra -fsyntax-only {} \; 2>&1 | grep "error:" | wc -l)
echo "Remaining errors: $REMAINING_ERRORS"

# Show top error types
echo "Top remaining error types:"
find src -name "*.c" -exec gcc -I src -I include -Wall -Wextra -fsyntax-only {} \; 2>&1 | grep "error:" | head -10
