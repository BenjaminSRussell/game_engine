/**
 * Unit Test for Quaternion Math
 * Tests quaternion operations for 3D rotations
 */

#include "../../../src/engine/math/quat.h"
#include "../../../src/engine/math/vec3.h"
#include "../../../tests/test_framework_unified.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

// Test quaternion identity
static TestResult test_quat_identity(void) {
    Quaternion q = quat_identity();
    
    TEST_ASSERT_FLOAT_EQ(q.x, 0.0f, 0.001f, "X = 0");
    TEST_ASSERT_FLOAT_EQ(q.y, 0.0f, 0.001f, "Y = 0");
    TEST_ASSERT_FLOAT_EQ(q.z, 0.0f, 0.001f, "Z = 0");
    TEST_ASSERT_FLOAT_EQ(q.w, 1.0f, 0.001f, "W = 1");
    
    return TEST_PASS;
}

// Test quaternion from axis-angle
static TestResult test_quat_from_axis_angle(void) {
    Vec3 axis = vec3(0.0f, 1.0f, 0.0f);  // Y axis
    float angle = M_PI / 2.0f;  // 90 degrees
    
    Quaternion q = quat_from_axis_angle(axis, angle);
    
    // Quaternion should be non-zero
    TEST_ASSERT(q.w != 0.0f || q.y != 0.0f, "Quaternion created from axis-angle");
    
    return TEST_PASS;
}

// Test quaternion multiplication
static TestResult test_quat_multiply(void) {
    Quaternion q1 = quat_identity();
    Quaternion q2 = quat_identity();
    
    Quaternion result = quat_multiply(q1, q2);
    
    // Identity * Identity = Identity
    TEST_ASSERT_FLOAT_EQ(result.w, 1.0f, 0.001f, "Result is identity");
    TEST_ASSERT_FLOAT_EQ(result.x, 0.0f, 0.001f, "X = 0");
    
    return TEST_PASS;
}

// Test quaternion normalization
static TestResult test_quat_normalize(void) {
    Quaternion q = {1.0f, 1.0f, 1.0f, 1.0f};
    Quaternion normalized = quat_normalize(q);
    
    // Length should be 1
    float len = sqrtf(normalized.x * normalized.x + 
                      normalized.y * normalized.y + 
                      normalized.z * normalized.z + 
                      normalized.w * normalized.w);
    
    TEST_ASSERT_FLOAT_EQ(len, 1.0f, 0.001f, "Normalized length is 1");
    
    return TEST_PASS;
}

// Test quaternion conjugate
static TestResult test_quat_conjugate(void) {
    Quaternion q = {1.0f, 2.0f, 3.0f, 4.0f};
    Quaternion conj = quat_conjugate(q);
    
    // Conjugate negates x, y, z
    TEST_ASSERT_FLOAT_EQ(conj.x, -1.0f, 0.001f, "X negated");
    TEST_ASSERT_FLOAT_EQ(conj.y, -2.0f, 0.001f, "Y negated");
    TEST_ASSERT_FLOAT_EQ(conj.z, -3.0f, 0.001f, "Z negated");
    TEST_ASSERT_FLOAT_EQ(conj.w, 4.0f, 0.001f, "W unchanged");
    
    return TEST_PASS;
}

// Test quaternion dot product
static TestResult test_quat_dot(void) {
    Quaternion q1 = {1.0f, 2.0f, 3.0f, 4.0f};
    Quaternion q2 = {5.0f, 6.0f, 7.0f, 8.0f};
    
    float dot = quat_dot(q1, q2);
    
    // 1*5 + 2*6 + 3*7 + 4*8 = 5 + 12 + 21 + 32 = 70
    TEST_ASSERT_FLOAT_EQ(dot, 70.0f, 0.001f, "Dot product");
    
    return TEST_PASS;
}

// Test quaternion slerp (spherical linear interpolation)
static TestResult test_quat_slerp(void) {
    Quaternion q1 = quat_identity();
    Vec3 axis = vec3(0.0f, 1.0f, 0.0f);
    Quaternion q2 = quat_from_axis_angle(axis, M_PI / 2.0f);
    
    Quaternion mid = quat_slerp(q1, q2, 0.5f);
    
    // Mid should be between q1 and q2
    TEST_ASSERT(mid.w != q1.w, "Slerp result different from start");
    TEST_ASSERT(mid.w != q2.w, "Slerp result different from end");
    
    return TEST_PASS;
}

// Test quaternion to euler angles
static TestResult test_quat_to_euler(void) {
    Vec3 axis = vec3(0.0f, 1.0f, 0.0f);
    Quaternion q = quat_from_axis_angle(axis, M_PI / 2.0f);
    
    Vec3 euler = quat_to_euler(q);
    
    // Y rotation should be non-zero
    TEST_ASSERT(euler.y != 0.0f, "Y rotation exists");
    
    return TEST_PASS;
}

// Test rotating vector by quaternion
static TestResult test_quat_rotate_vec3(void) {
    Vec3 axis = vec3(0.0f, 1.0f, 0.0f);  // Y axis
    Quaternion q = quat_from_axis_angle(axis, M_PI / 2.0f);  // 90° around Y
    
    Vec3 v = vec3(1.0f, 0.0f, 0.0f);  // X axis
    Vec3 rotated = quat_rotate_vec3(q, v);
    
    // After 90° rotation around Y, X axis becomes -Z axis
    TEST_ASSERT_FLOAT_EQ(rotated.x, 0.0f, 0.1f, "Rotated X ≈ 0");
    TEST_ASSERT_FLOAT_EQ(rotated.z, -1.0f, 0.1f, "Rotated Z ≈ -1");
    
    return TEST_PASS;
}

// Register all tests
int main(void) {
    test_init();
    
    test_register("Math:Quat", "Identity", test_quat_identity, NULL, NULL);
    test_register("Math:Quat", "From Axis-Angle", test_quat_from_axis_angle, NULL, NULL);
    test_register("Math:Quat", "Multiply", test_quat_multiply, NULL, NULL);
    test_register("Math:Quat", "Normalize", test_quat_normalize, NULL, NULL);
    test_register("Math:Quat", "Conjugate", test_quat_conjugate, NULL, NULL);
    test_register("Math:Quat", "Dot Product", test_quat_dot, NULL, NULL);
    test_register("Math:Quat", "Slerp", test_quat_slerp, NULL, NULL);
    test_register("Math:Quat", "To Euler", test_quat_to_euler, NULL, NULL);
    test_register("Math:Quat", "Rotate Vec3", test_quat_rotate_vec3, NULL, NULL);
    
    TestStats stats = test_run_all();
    
    printf("\n");
    printf("════════════════════════════════════════════════════════\n");
    printf("  Quaternion Math Test Results\n");
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
