#!/bin/bash

# Fix remaining include issues
echo "Fixing remaining include issues..."

# Fix all remaining angled includes in common.h referenced files
find src -name "*.c" -o -name "*.h" | xargs grep -l "include <core/" | while read file; do
    echo "Fixing core includes in $file"
    sed -i '' 's|#include <core/\([^>]*\)>|#include "core/\1"|g' "$file"
done

# Fix all remaining math includes
find src -name "*.c" -o -name "*.h" | xargs grep -l "include <math/" | while read file; do
    echo "Fixing math includes in $file"
    sed -i '' 's|#include <math/\([^>]*\)>|#include "\1"|g' "$file"
done

# Fix remaining include/math/math.h patterns
find src -name "*.c" -o -name "*.h" | xargs grep -l "include.*/math/math.h" | while read file; do
    echo "Fixing math/math.h includes in $file"
    sed -i '' 's|#include.*/math/math.h|#include "engine/include/math/math.h"|g' "$file"
done

echo "Include fixes completed!"

# Count remaining errors
REMAINING_ERRORS=$(find src -name "*.c" -exec gcc -I src -I include -Wall -Wextra -fsyntax-only {} \; 2>&1 | grep "error:" | wc -l)
echo "Remaining errors: $REMAINING_ERRORS"

# Show top error types
echo "Top remaining error types:"
find src -name "*.c" -exec gcc -I src -I include -Wall -Wextra -fsyntax-only {} \; 2>&1 | grep "error:" | head -10
