#!/bin/bash

# Systematic code quality fix script
echo "Starting systematic code quality fixes..."

# Count initial errors
INITIAL_ERRORS=$(find src -name "*.c" -exec gcc -I src -I include -Wall -Wextra -fsyntax-only {} \; 2>&1 | grep "error:" | wc -l)
echo "Initial errors: $INITIAL_ERRORS"

# Fix common include path issues
echo "Fixing common include path issues..."

# Fix math/math.h includes
find src -name "*.c" -o -name "*.h" | xargs grep -l "math/math.h" | while read file; do
    sed -i '' 's|#include <math/math.h>|#include "engine/include/math/math.h"|g' "$file"
    sed -i '' 's|#include "math/math.h"|#include "engine/include/math/math.h"|g' "$file"
done

# Fix core/logger.h includes
find src -name "*.c" -o -name "*.h" | xargs grep -l "core/logger.h" | while read file; do
    sed -i '' 's|#include "core/logger.h"|#include "engine/include/core/logger.h"|g' "$file"
    sed -i '' 's|#include <core/logger.h>|#include "engine/include/core/logger.h"|g' "$file"
done

# Fix core/memory.h includes
find src -name "*.c" -o -name "*.h" | xargs grep -l "core/memory.h" | while read file; do
    sed -i '' 's|#include "core/memory.h"|#include "engine/include/core/memory.h"|g' "$file"
    sed -i '' 's|#include <core/memory.h>|#include "engine/include/core/memory.h"|g' "$file"
done

# Fix common.h includes
find src -name "*.c" -o -name "*.h" | xargs grep -l "common.h" | while read file; do
    sed -i '' 's|#include "common.h"|#include "engine/include/common.h"|g' "$file"
    sed -i '' 's|#include <common.h>|#include "engine/include/common.h"|g' "$file"
done

# Count remaining errors
FINAL_ERRORS=$(find src -name "*.c" -exec gcc -I src -I include -Wall -Wextra -fsyntax-only {} \; 2>&1 | grep "error:" | wc -l)
echo "Remaining errors: $FINAL_ERRORS"
echo "Fixed: $((INITIAL_ERRORS - FINAL_ERRORS)) errors"

# Show top 10 remaining error types
echo "Top 10 remaining error types:"
find src -name "*.c" -exec gcc -I src -I include -Wall -Wextra -fsyntax-only {} \; 2>&1 | grep "error:" | head -10
