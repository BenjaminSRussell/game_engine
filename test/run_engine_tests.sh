#!/bin/bash
# Engine Test Runner

set -e

echo "==========================================="
echo "  ENGINE TEST SUITE"
echo "==========================================="
echo ""

# Compiler settings
CC=gcc
CFLAGS="-std=c99 -g -Wall -Wextra"
# Note: In a real build these would link against the engine lib
# For these self-contained unit tests, we compile just the test file
INCLUDES="-I src/engine/include -I test"

# Test tracking
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

run_test() {
    local test_name=$1
    local test_file=$2
    
    echo "Building: $test_name..."
    # Intentionally not linking complex libs for these mock tests
    $CC $CFLAGS $INCLUDES $test_file -o test/$test_name 2>&1 | grep -v "warning:" || true
    
    if [ -f "test/$test_name" ]; then
        echo "Running: $test_name"
        echo "---"
        ./test/$test_name
        local exit_code=$?
        echo ""
        
        if [ $exit_code -eq 0 ]; then
            echo "✅ $test_name PASSED"
            ((PASSED_TESTS++))
        else
            echo "❌ $test_name FAILED (exit code: $exit_code)"
            ((FAILED_TESTS++))
        fi
        ((TOTAL_TESTS++))
        
        # Clean up binary
        rm test/$test_name
    else
        echo "❌ $test_name FAILED TO COMPILE"
        ((FAILED_TESTS++))
        ((TOTAL_TESTS++))
    fi
    
    echo ""
}

# Run tests
run_test "test_render_graph" "test/test_render_graph.c"
run_test "test_material_system" "test/test_material_system.c"
run_test "test_post_processing" "test/test_post_processing.c"

# Summary
echo "==========================================="
echo "  FINAL TEST RESULTS"
echo "==========================================="
echo "Passed: $PASSED_TESTS / $TOTAL_TESTS"

if [ $FAILED_TESTS -eq 0 ]; then
    echo "🎉 ALL ENGINE TESTS PASSED!"
    exit 0
else
    echo "⚠️  SOME TESTS FAILED"
    exit 1
fi
