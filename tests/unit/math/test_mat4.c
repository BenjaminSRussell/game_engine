/**
 * Unit Test for Mat4 (4x4 Matrix Math)
 * Tests matrix operations for 3D transformations
 */

#include "../../../src/engine/math/mat4.h"
#include "../../../src/engine/math/vec3.h"
#include "../../../tests/test_framework_unified.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

// Test identity matrix
static TestResult test_mat4_identity(void) {
    Mat4 m = mat4_identity();
    
    // Identity matrix has 1s on diagonal, 0s elsewhere
    TEST_ASSERT_FLOAT_EQ(m.m[0], 1.0f, 0.001f, "m[0][0] = 1");
    TEST_ASSERT_FLOAT_EQ(m.m[5], 1.0f, 0.001f, "m[1][1] = 1");
    TEST_ASSERT_FLOAT_EQ(m.m[10], 1.0f, 0.001f, "m[2][2] = 1");
    TEST_ASSERT_FLOAT_EQ(m.m[15], 1.0f, 0.001f, "m[3][3] = 1");
    
    TEST_ASSERT_FLOAT_EQ(m.m[1], 0.0f, 0.001f, "Off-diagonal is 0");
    TEST_ASSERT_FLOAT_EQ(m.m[2], 0.0f, 0.001f, "Off-diagonal is 0");
    
    return TEST_PASS;
}

// Test matrix multiplication
static TestResult test_mat4_multiply(void) {
    Mat4 a = mat4_identity();
    Mat4 b = mat4_identity();
    
    // Identity * Identity = Identity
    Mat4 result = mat4_multiply(a, b);
    
    TEST_ASSERT_FLOAT_EQ(result.m[0], 1.0f, 0.001f, "Result is identity");
    TEST_ASSERT_FLOAT_EQ(result.m[5], 1.0f, 0.001f, "Result is identity");
    
    return TEST_PASS;
}

// Test translation matrix
static TestResult test_mat4_translate(void) {
    Vec3 translation = vec3(10.0f, 20.0f, 30.0f);
    Mat4 m = mat4_translate(translation);
    
    // Translation values should be in last column
    TEST_ASSERT_FLOAT_EQ(m.m[12], 10.0f, 0.001f, "Translation X");
    TEST_ASSERT_FLOAT_EQ(m.m[13], 20.0f, 0.001f, "Translation Y");
    TEST_ASSERT_FLOAT_EQ(m.m[14], 30.0f, 0.001f, "Translation Z");
    
    return TEST_PASS;
}

// Test scale matrix
static TestResult test_mat4_scale(void) {
    Vec3 scale = vec3(2.0f, 3.0f, 4.0f);
    Mat4 m = mat4_scale(scale);
    
    // Scale values should be on diagonal
    TEST_ASSERT_FLOAT_EQ(m.m[0], 2.0f, 0.001f, "Scale X");
    TEST_ASSERT_FLOAT_EQ(m.m[5], 3.0f, 0.001f, "Scale Y");
    TEST_ASSERT_FLOAT_EQ(m.m[10], 4.0f, 0.001f, "Scale Z");
    
    return TEST_PASS;
}

// Test rotation matrix (around Y axis)
static TestResult test_mat4_rotate_y(void) {
    // 90 degree rotation
    float angle = M_PI / 2.0f;
    Mat4 m = mat4_rotate_y(angle);
    
    // After 90° rotation around Y:
    // X axis becomes -Z axis
    // Z axis becomes X axis
    TEST_ASSERT_FLOAT_EQ(m.m[0], 0.0f, 0.01f, "cos(90°) ≈ 0");
    TEST_ASSERT_FLOAT_EQ(m.m[10], 0.0f, 0.01f, "cos(90°) ≈ 0");
    
    return TEST_PASS;
}

// Test perspective projection matrix
static TestResult test_mat4_perspective(void) {
    float fov = M_PI / 4.0f;  // 45 degrees
    float aspect = 16.0f / 9.0f;
    float near = 0.1f;
    float far = 100.0f;
    
    Mat4 m = mat4_perspective(fov, aspect, near, far);
    
    // Perspective matrix should have specific structure
    TEST_ASSERT(m.m[15] == 0.0f, "Perspective matrix [3][3] is 0");
    
    return TEST_PASS;
}

// Test orthographic projection matrix
static TestResult test_mat4_ortho(void) {
    Mat4 m = mat4_ortho(0.0f, 800.0f, 0.0f, 600.0f, -1.0f, 1.0f);
    
    // Orthographic matrix should have 1 in bottom-right
    TEST_ASSERT_FLOAT_EQ(m.m[15], 1.0f, 0.001f, "Ortho [3][3] = 1");
    
    return TEST_PASS;
}

// Test lookAt matrix
static TestResult test_mat4_lookat(void) {
    Vec3 eye = vec3(0.0f, 0.0f, 10.0f);
    Vec3 center = vec3(0.0f, 0.0f, 0.0f);
    Vec3 up = vec3(0.0f, 1.0f, 0.0f);
    
    Mat4 m = mat4_lookat(eye, center, up);
    
    // LookAt matrix should be valid (non-zero)
    TEST_ASSERT(m.m[0] != 0.0f || m.m[1] != 0.0f, "LookAt matrix is not zero");
    
    return TEST_PASS;
}

// Test matrix transpose
static TestResult test_mat4_transpose(void) {
    Mat4 m = mat4_identity();
    m.m[1] = 5.0f;  // Set off-diagonal element
    m.m[4] = 7.0f;
    
    Mat4 t = mat4_transpose(m);
    
    // Transposed elements should be swapped
    TEST_ASSERT_FLOAT_EQ(t.m[4], 5.0f, 0.001f, "Transposed m[1] -> m[4]");
    TEST_ASSERT_FLOAT_EQ(t.m[1], 7.0f, 0.001f, "Transposed m[4] -> m[1]");
    
    return TEST_PASS;
}

// Register all tests
int main(void) {
    test_init();
    
    test_register("Math:Mat4", "Identity", test_mat4_identity, NULL, NULL);
    test_register("Math:Mat4", "Multiply", test_mat4_multiply, NULL, NULL);
    test_register("Math:Mat4", "Translate", test_mat4_translate, NULL, NULL);
    test_register("Math:Mat4", "Scale", test_mat4_scale, NULL, NULL);
    test_register("Math:Mat4", "Rotate Y", test_mat4_rotate_y, NULL, NULL);
    test_register("Math:Mat4", "Perspective", test_mat4_perspective, NULL, NULL);
    test_register("Math:Mat4", "Orthographic", test_mat4_ortho, NULL, NULL);
    test_register("Math:Mat4", "LookAt", test_mat4_lookat, NULL, NULL);
    test_register("Math:Mat4", "Transpose", test_mat4_transpose, NULL, NULL);
    
    TestStats stats = test_run_all();
    
    printf("\n");
    printf("════════════════════════════════════════════════════════\n");
    printf("  Mat4 Math Test Results\n");
    printf("════════════════════════════════════════════════════════\n");
    printf("  Total:   %u\n", stats.total);
    printf("  Passed:  %u\n", stats.passed);
    printf("  Failed:  %u\n", stats.failed);
    printf("  Skipped: %u\n", stats.skipped);
    printf("  Time:    %.2f ms\n", stats.duration_ms);
    printf("════════════════════════════════════════════════════════\n");
    
    test_cleanup();
    
    return stats.failed > 0 ? 1 : 0;
}
