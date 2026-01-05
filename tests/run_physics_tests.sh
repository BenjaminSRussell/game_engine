#!/bin/bash
# Master Test Runner for All Physics Systems

set -e

echo "==========================================="
echo "  COMPREHENSIVE PHYSICS TEST SUITE"
echo "==========================================="
echo ""

# Compiler settings
CC=gcc
CFLAGS="-std=c99 -g -Wall -Wextra"
INCLUDES="-I src/engine/include -I tests/physics -I /opt/homebrew/include"
LIBS="-L /opt/homebrew/lib -lm"

# Test tracking
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Function to run a single test
run_test() {
    local test_name=$1
    local test_file=$2
    
    echo "Building: $test_name..."
    $CC $CFLAGS $INCLUDES $test_file $LIBS -o tests/physics/$test_name 2>&1 | grep -v "warning:" || true
    
    if [ -f "tests/physics/$test_name" ]; then
        echo "Running: $test_name"
        echo "---"
        ./tests/physics/$test_name
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
        rm tests/physics/$test_name
    else
        echo "❌ $test_name FAILED TO COMPILE"
        ((FAILED_TESTS++))
        ((TOTAL_TESTS++))
    fi
    
    echo ""
}

# Run all test suites
echo "Running Test Suites..."
echo ""

run_test "test_foundation" "tests/physics/test_foundation_systems.c"
run_test "test_fluids" "tests/physics/test_fluid_dynamics.c"
run_test "test_advanced" "tests/physics/test_advanced_systems.c"
run_test "test_specialized" "tests/physics/test_specialized_systems.c"

# Final Summary
echo "==========================================="
echo "  FINAL TEST RESULTS"
echo "==========================================="
echo "Total Test Suites: $TOTAL_TESTS"
echo "Passed: $PASSED_TESTS ✅"
echo "Failed: $FAILED_TESTS ❌"

if [ $TOTAL_TESTS -gt 0 ]; then
    SUCCESS_RATE=$((100 * PASSED_TESTS / TOTAL_TESTS))
    echo "Success Rate: ${SUCCESS_RATE}%"
fi

echo "==========================================="

if [ $FAILED_TESTS -eq 0 ]; then
    echo ""
    echo "🎉 ALL TESTS PASSED! 🎉"
    exit 0
else
    echo ""
    echo "⚠️  SOME TESTS FAILED"
    exit 1
fi
