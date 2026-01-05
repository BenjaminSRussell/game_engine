#!/bin/bash
# scripts/verify_migration.sh
# Verifies all files were moved correctly

errors=0

echo "Verifying file migration..."

# Check math files
echo "Checking math library..."
for file in vec2.c vec3.c vec4.c mat4.c quat.c; do
    if [ -f "math/$file" ]; then
        echo "  ✓ math/$file"
    else
        echo "  ✗ math/$file MISSING"
        errors=$((errors+1))
    fi
done

# Check renderer subdirectories
echo "Checking renderer subdirectories..."
for subdir in vulkan gpu systems advanced core controllers; do
    count=$(find renderer/$subdir -name "*.c" -type f 2>/dev/null | wc -l | tr -d ' ')
    if [ "$count" -gt 0 ]; then
        echo "  ✓ renderer/$subdir ($count files)"
    else
        echo "  ✗ renderer/$subdir EMPTY"
        errors=$((errors+1))
    fi
done

# Check physics
echo "Checking physics..."
count=$(find physics/core -name "*.c" -type f 2>/dev/null | wc -l | tr -d ' ')
if [ "$count" -gt 0 ]; then
    echo "  ✓ physics/core ($count files)"
else
    echo "  ✗ physics/core EMPTY"
    errors=$((errors+1))
fi

# Check ECS and input
echo "Checking ECS and input..."
[ -f "ecs/ecs.c" ] && echo "  ✓ ecs/ecs.c" || { echo "  ✗ ecs/ecs.c MISSING"; errors=$((errors+1)); }
[ -f "input/input.c" ] && echo "  ✓ input/input.c" || { echo "  ✗ input/input.c MISSING"; errors=$((errors+1)); }

echo ""
if [ $errors -eq 0 ]; then
    echo "✅ All files migrated successfully!"
    exit 0
else
    echo "❌ Migration errors: $errors"
    exit 1
fi
