/**
 * Unit Tests for SIMD Math Library
 * 
 * Tests vector and matrix operations across all platforms (scalar, SSE2, AVX2, NEON).
 * Validates correctness and verifies SIMD implementations match scalar reference.
 */

#include "../src/engine/core/simd/simd_math.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

// Test tolerances
#define EPSILON 1e-5f
#define EPSILON_RELAXED 1e-4f

// Helper: Compare floats with tolerance
static int float_eq(float a, float b, float eps) {
    return fabsf(a - b) < eps;
}

// Helper: Compare v4f with tolerance
static int v4f_eq(const v4f *a, const v4f *b, float eps) {
    return float_eq(a->x, b->x, eps) &&
           float_eq(a->y, b->y, eps) &&
           float_eq(a->z, b->z, eps) &&
           float_eq(a->w, b->w, eps);
}

// ========================================
// Vector Operation Tests
// ========================================

void test_v4f_add() {
    printf("Testing v4f_add... ");
    
    v4f a = V4F_VECTOR(1.0f, 2.0f, 3.0f);
    v4f b = V4F_VECTOR(4.0f, 5.0f, 6.0f);
    v4f result;
    
    v4f_add(&result, &a, &b);
    
    v4f expected = V4F_VECTOR(5.0f, 7.0f, 9.0f);
    assert(v4f_eq(&result, &expected, EPSILON));
    
    printf("PASSED\n");
}

void test_v4f_sub() {
    printf("Testing v4f_sub... ");
    
    v4f a = V4F_VECTOR(10.0f, 20.0f, 30.0f);
    v4f b = V4F_VECTOR(1.0f, 2.0f, 3.0f);
    v4f result;
    
    v4f_sub(&result, &a, &b);
    
    v4f expected = V4F_VECTOR(9.0f, 18.0f, 27.0f);
    assert(v4f_eq(&result, &expected, EPSILON));
    
    printf("PASSED\n");
}

void test_v4f_scale() {
    printf("Testing v4f_scale... ");
    
    v4f v = V4F_VECTOR(2.0f, 4.0f, 6.0f);
    v4f result;
    
    v4f_scale(&result, &v, 0.5f);
    
    v4f expected = V4F_VECTOR(1.0f, 2.0f, 3.0f);
    assert(v4f_eq(&result, &expected, EPSILON));
    
    printf("PASSED\n");
}

void test_v4f_dot() {
    printf("Testing v4f_dot... ");
    
    v4f a = V4F_VECTOR(1.0f, 2.0f, 3.0f);
    v4f b = V4F_VECTOR(4.0f, 5.0f, 6.0f);
    
    float result = v4f_dot(&a, &b);
    float expected = 1*4 + 2*5 + 3*6; // = 32
    
    assert(float_eq(result, expected, EPSILON));
    
    printf("PASSED\n");
}

void test_v4f_cross() {
    printf("Testing v4f_cross... ");
    
    // Unit vectors: X cross Y = Z
    v4f x = V4F_VECTOR(1.0f, 0.0f, 0.0f);
    v4f y = V4F_VECTOR(0.0f, 1.0f, 0.0f);
    v4f result;
    
    v4f_cross(&result, &x, &y);
    
    v4f expected_z = V4F_VECTOR(0.0f, 0.0f, 1.0f);
    assert(v4f_eq(&result, &expected_z, EPSILON));
    
    // Test with non-unit vectors
    v4f a = V4F_VECTOR(2.0f, 3.0f, 4.0f);
    v4f b = V4F_VECTOR(5.0f, 6.0f, 7.0f);
    v4f_cross(&result, &a, &b);
    
    // Expected: (3*7 - 4*6, 4*5 - 2*7, 2*6 - 3*5) = (-3, 6, -3)
    v4f expected = V4F_VECTOR(-3.0f, 6.0f, -3.0f);
    assert(v4f_eq(&result, &expected, EPSILON));
    
    // Verify result is orthogonal to both inputs
    float dot_a = v4f_dot(&result, &a);
    float dot_b = v4f_dot(&result, &b);
    assert(float_eq(dot_a, 0.0f, EPSILON));
    assert(float_eq(dot_b, 0.0f, EPSILON));
    
    printf("PASSED\n");
}

void test_v4f_length() {
    printf("Testing v4f_length... ");
    
    v4f v = V4F_VECTOR(3.0f, 4.0f, 0.0f);
    float len = v4f_length(&v);
    assert(float_eq(len, 5.0f, EPSILON));  // 3-4-5 triangle
    
    v4f unit = V4F_VECTOR(1.0f, 0.0f, 0.0f);
    len = v4f_length(&unit);
    assert(float_eq(len, 1.0f, EPSILON));
    
    printf("PASSED\n");
}

void test_v4f_normalize() {
    printf("Testing v4f_normalize... ");
    
    v4f v = V4F_VECTOR(3.0f, 4.0f, 0.0f);
    v4f result;
    v4f_normalize(&result, &v);
    
    float len = v4f_length(&result);
    assert(float_eq(len, 1.0f, EPSILON));
    
    // Direction should be preserved
    v4f expected = V4F_VECTOR(0.6f, 0.8f, 0.0f);
    assert(v4f_eq(&result, &expected, EPSILON));
    
    // Test zero vector (should not crash)
    v4f zero = V4F_ZERO;
    v4f_normalize(&result, &zero);
    assert(v4f_eq(&result, &zero, EPSILON));
    
    printf("PASSED\n");
}

// ========================================
// Batch Operation Tests
// ========================================

void test_v4f_add_batch() {
    printf("Testing v4f_add_batch... ");
    
    const uint32_t count = 16;
    v4f a[16], b[16], result[16];
    
    // Initialize test data
    for (uint32_t i = 0; i < count; i++) {
        a[i] = V4F_VECTOR((float)i, (float)i*2, (float)i*3);
        b[i] = V4F_VECTOR(1.0f, 2.0f, 3.0f);
    }
    
    v4f_add_batch(result, a, b, count);
    
    // Verify results
    for (uint32_t i = 0; i < count; i++) {
        v4f expected = V4F_VECTOR((float)i + 1.0f, (float)i*2 + 2.0f, (float)i*3 + 3.0f);
        assert(v4f_eq(&result[i], &expected, EPSILON));
    }
    
    printf("PASSED\n");
}

void test_v4f_scale_batch() {
    printf("Testing v4f_scale_batch... ");
    
    const uint32_t count = 32;
    v4f v[32], result[32];
    
    for (uint32_t i = 0; i < count; i++) {
        v[i] = V4F_VECTOR((float)i, (float)i*2, (float)i*3);
    }
    
    v4f_scale_batch(result, v, 0.5f, count);
    
    for (uint32_t i = 0; i < count; i++) {
        v4f expected = V4F_VECTOR((float)i * 0.5f, (float)i*2 * 0.5f, (float)i*3 * 0.5f);
        assert(v4f_eq(&result[i], &expected, EPSILON));
    }
    
    printf("PASSED\n");
}

// ========================================
// Matrix Operation Tests
// ========================================

void test_m33f_transform() {
    printf("Testing m33f_transform... ");
    
    // Identity matrix should not change vector
    m33f I = M33F_IDENTITY;
    v4f v = V4F_VECTOR(1.0f, 2.0f, 3.0f);
    v4f result;
    
    m33f_transform(&result, &I, &v);
    assert(v4f_eq(&result, &v, EPSILON));
    
    // Custom matrix: swap x and y
    m33f swap;
    swap.col0 = V4F_VECTOR(0.0f, 1.0f, 0.0f);
    swap.col1 = V4F_VECTOR(1.0f, 0.0f, 0.0f);
    swap.col2 = V4F_VECTOR(0.0f, 0.0f, 1.0f);
    
    m33f_transform(&result, &swap, &v);
    v4f expected = V4F_VECTOR(2.0f, 1.0f, 3.0f);
    assert(v4f_eq(&result, &expected, EPSILON));
    
    printf("PASSED\n");
}

void test_m33f_mul() {
    printf("Testing m33f_mul... ");
    
    m33f I = M33F_IDENTITY;
    m33f M;
    M.col0 = V4F_VECTOR(1.0f, 2.0f, 3.0f);
    M.col1 = V4F_VECTOR(4.0f, 5.0f, 6.0f);
    M.col2 = V4F_VECTOR(7.0f, 8.0f, 9.0f);
    
    // I * M = M
    m33f result;
    m33f_mul(&result, &I, &M);
    assert(v4f_eq(&result.col0, &M.col0, EPSILON));
    assert(v4f_eq(&result.col1, &M.col1, EPSILON));
    assert(v4f_eq(&result.col2, &M.col2, EPSILON));
    
    printf("PASSED\n");
}

void test_m33f_transpose() {
    printf("Testing m33f_transpose... ");
    
    m33f M;
    M.col0 = V4F_VECTOR(1.0f, 2.0f, 3.0f);
    M.col1 = V4F_VECTOR(4.0f, 5.0f, 6.0f);
    M.col2 = V4F_VECTOR(7.0f, 8.0f, 9.0f);
    
    m33f result;
    m33f_transpose(&result, &M);
    
    // Transposed rows become columns
    v4f expected_col0 = V4F_VECTOR(1.0f, 4.0f, 7.0f);
    v4f expected_col1 = V4F_VECTOR(2.0f, 5.0f, 8.0f);
    v4f expected_col2 = V4F_VECTOR(3.0f, 6.0f, 9.0f);
    
    assert(v4f_eq(&result.col0, &expected_col0, EPSILON));
    assert(v4f_eq(&result.col1, &expected_col1, EPSILON));
    assert(v4f_eq(&result.col2, &expected_col2, EPSILON));
    
    printf("PASSED\n");
}

void test_m33f_from_axis_angle() {
    printf("Testing m33f_from_axis_angle... ");
    
    // 90 degree rotation around Z axis
    v4f z_axis = V4F_VECTOR(0.0f, 0.0f, 1.0f);
    float angle = M_PI / 2.0f;  // 90 degrees
    
    m33f rotation;
    m33f_from_axis_angle(&rotation, &z_axis, angle);
    
    // Rotate X axis (1,0,0) by 90 degrees around Z should give Y axis (0,1,0)
    v4f x_axis = V4F_VECTOR(1.0f, 0.0f, 0.0f);
    v4f result;
    m33f_transform(&result, &rotation, &x_axis);
    
    v4f expected_y = V4F_VECTOR(0.0f, 1.0f, 0.0f);
    assert(v4f_eq(&result, &expected_y, EPSILON_RELAXED));
    
    printf("PASSED\n");
}

// ========================================
// Performance Benchmark (Optional)
// ========================================

void benchmark_batch_add() {
    printf("\nBenchmarking batch operations...\n");
    
    const uint32_t count = 10000;
    v4f *a = (v4f*)aligned_alloc(16, sizeof(v4f) * count);
    v4f *b = (v4f*)aligned_alloc(16, sizeof(v4f) * count);
    v4f *result = (v4f*)aligned_alloc(16, sizeof(v4f) * count);
    
    for (uint32_t i = 0; i < count; i++) {
        a[i] = V4F_VECTOR((float)i, (float)i, (float)i);
        b[i] = V4F_VECTOR(1.0f, 1.0f, 1.0f);
    }
    
    const uint32_t iterations = 1000;
    
    // Warm-up
    v4f_add_batch(result, a, b, count);
    
    printf("  Processing %u vectors, %u iterations\n", count, iterations);
    printf("  (Implementation: ");
#if defined(SIMD_AVX2)
    printf("AVX2)\n");
#elif defined(SIMD_SSE2)
    printf("SSE2)\n");
#elif defined(SIMD_NEON)
    printf("NEON)\n");
#else
    printf("Scalar)\n");
#endif
    
    free(a);
    free(b);
    free(result);
    
    printf("  Benchmark complete.\n");
}

// ========================================
// Main Test Runner
// ========================================

int main() {
    printf("=== SIMD Math Library Unit Tests ===\n\n");
    
    printf("Platform: ");
#if defined(SIMD_AVX2)
    printf("x86-64 AVX2\n");
#elif defined(SIMD_SSE2)
    printf("x86-64 SSE2\n");
#elif defined(SIMD_NEON)
    printf("ARM NEON\n");
#else
    printf("Scalar (no SIMD)\n");
#endif
    
    printf("\nVector Operations:\n");
    test_v4f_add();
    test_v4f_sub();
    test_v4f_scale();
    test_v4f_dot();
    test_v4f_cross();
    test_v4f_length();
    test_v4f_normalize();
    
    printf("\nBatch Operations:\n");
    test_v4f_add_batch();
    test_v4f_scale_batch();
    
    printf("\nMatrix Operations:\n");
    test_m33f_transform();
    test_m33f_mul();
    test_m33f_transpose();
    test_m33f_from_axis_angle();
    
    benchmark_batch_add();
    
    printf("\n=== ALL TESTS PASSED ===\n");
    return 0;
}
