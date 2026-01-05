/**
 * Unit Test for Vec3 Math
 * Tests 3D vector operations
 */

#include "../../../src/engine/math/vec3.h"
#include "../../../tests/test_framework_unified.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

// Test vector creation
static TestResult test_vec3_create(void) {
    Vec3 v = vec3(1.0f, 2.0f, 3.0f);
    TEST_ASSERT_FLOAT_EQ(v.x, 1.0f, 0.001f, "X component");
    TEST_ASSERT_FLOAT_EQ(v.y, 2.0f, 0.001f, "Y component");
    TEST_ASSERT_FLOAT_EQ(v.z, 3.0f, 0.001f, "Z component");
    return TEST_PASS;
}

// Test vector addition
static TestResult test_vec3_add(void) {
    Vec3 a = vec3(1.0f, 2.0f, 3.0f);
    Vec3 b = vec3(4.0f, 5.0f, 6.0f);
    Vec3 result = vec3_add(a, b);
    
    TEST_ASSERT_FLOAT_EQ(result.x, 5.0f, 0.001f, "X sum");
    TEST_ASSERT_FLOAT_EQ(result.y, 7.0f, 0.001f, "Y sum");
    TEST_ASSERT_FLOAT_EQ(result.z, 9.0f, 0.001f, "Z sum");
    return TEST_PASS;
}

// Test vector subtraction
static TestResult test_vec3_sub(void) {
    Vec3 a = vec3(5.0f, 7.0f, 9.0f);
    Vec3 b = vec3(1.0f, 2.0f, 3.0f);
    Vec3 result = vec3_sub(a, b);
    
    TEST_ASSERT_FLOAT_EQ(result.x, 4.0f, 0.001f, "X difference");
    TEST_ASSERT_FLOAT_EQ(result.y, 5.0f, 0.001f, "Y difference");
    TEST_ASSERT_FLOAT_EQ(result.z, 6.0f, 0.001f, "Z difference");
    return TEST_PASS;
}

// Test vector scaling
static TestResult test_vec3_scale(void) {
    Vec3 v = vec3(1.0f, 2.0f, 3.0f);
    Vec3 result = vec3_scale(v, 2.0f);
    
    TEST_ASSERT_FLOAT_EQ(result.x, 2.0f, 0.001f, "X scaled");
    TEST_ASSERT_FLOAT_EQ(result.y, 4.0f, 0.001f, "Y scaled");
    TEST_ASSERT_FLOAT_EQ(result.z, 6.0f, 0.001f, "Z scaled");
    return TEST_PASS;
}

// Test dot product
static TestResult test_vec3_dot(void) {
    Vec3 a = vec3(1.0f, 2.0f, 3.0f);
    Vec3 b = vec3(4.0f, 5.0f, 6.0f);
    float dot = vec3_dot(a, b);
    
    // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
    TEST_ASSERT_FLOAT_EQ(dot, 32.0f, 0.001f, "Dot product");
    return TEST_PASS;
}

// Test cross product
static TestResult test_vec3_cross(void) {
    Vec3 a = vec3(1.0f, 0.0f, 0.0f);
    Vec3 b = vec3(0.0f, 1.0f, 0.0f);
    Vec3 result = vec3_cross(a, b);
    
    // X cross Y = Z
    TEST_ASSERT_FLOAT_EQ(result.x, 0.0f, 0.001f, "Cross X");
    TEST_ASSERT_FLOAT_EQ(result.y, 0.0f, 0.001f, "Cross Y");
    TEST_ASSERT_FLOAT_EQ(result.z, 1.0f, 0.001f, "Cross Z");
    return TEST_PASS;
}

// Test vector length
static TestResult test_vec3_length(void) {
    Vec3 v = vec3(3.0f, 4.0f, 0.0f);
    float len = vec3_length(v);
    
    // sqrt(3^2 + 4^2) = sqrt(9 + 16) = sqrt(25) = 5
    TEST_ASSERT_FLOAT_EQ(len, 5.0f, 0.001f, "Vector length");
    return TEST_PASS;
}

// Test vector normalization
static TestResult test_vec3_normalize(void) {
    Vec3 v = vec3(3.0f, 4.0f, 0.0f);
    Vec3 normalized = vec3_normalize(v);
    
    float len = vec3_length(normalized);
    TEST_ASSERT_FLOAT_EQ(len, 1.0f, 0.001f, "Normalized length is 1");
    
    // Direction should be preserved
    TEST_ASSERT_FLOAT_EQ(normalized.x, 0.6f, 0.001f, "Normalized X");
    TEST_ASSERT_FLOAT_EQ(normalized.y, 0.8f, 0.001f, "Normalized Y");
    
    return TEST_PASS;
}

// Test vector distance
static TestResult test_vec3_distance(void) {
    Vec3 a = vec3(0.0f, 0.0f, 0.0f);
    Vec3 b = vec3(3.0f, 4.0f, 0.0f);
    float dist = vec3_distance(a, b);
    
    TEST_ASSERT_FLOAT_EQ(dist, 5.0f, 0.001f, "Distance between points");
    return TEST_PASS;
}

// Test vector lerp (linear interpolation)
static TestResult test_vec3_lerp(void) {
    Vec3 a = vec3(0.0f, 0.0f, 0.0f);
    Vec3 b = vec3(10.0f, 10.0f, 10.0f);
    
    Vec3 mid = vec3_lerp(a, b, 0.5f);
    TEST_ASSERT_FLOAT_EQ(mid.x, 5.0f, 0.001f, "Lerp X at 0.5");
    TEST_ASSERT_FLOAT_EQ(mid.y, 5.0f, 0.001f, "Lerp Y at 0.5");
    TEST_ASSERT_FLOAT_EQ(mid.z, 5.0f, 0.001f, "Lerp Z at 0.5");
    
    Vec3 start = vec3_lerp(a, b, 0.0f);
    TEST_ASSERT_FLOAT_EQ(start.x, 0.0f, 0.001f, "Lerp at 0.0 is start");
    
    Vec3 end = vec3_lerp(a, b, 1.0f);
    TEST_ASSERT_FLOAT_EQ(end.x, 10.0f, 0.001f, "Lerp at 1.0 is end");
    
    return TEST_PASS;
}

// Register all tests
int main(void) {
    test_init();
    
    test_register("Math:Vec3", "Create", test_vec3_create, NULL, NULL);
    test_register("Math:Vec3", "Add", test_vec3_add, NULL, NULL);
    test_register("Math:Vec3", "Subtract", test_vec3_sub, NULL, NULL);
    test_register("Math:Vec3", "Scale", test_vec3_scale, NULL, NULL);
    test_register("Math:Vec3", "Dot Product", test_vec3_dot, NULL, NULL);
    test_register("Math:Vec3", "Cross Product", test_vec3_cross, NULL, NULL);
    test_register("Math:Vec3", "Length", test_vec3_length, NULL, NULL);
    test_register("Math:Vec3", "Normalize", test_vec3_normalize, NULL, NULL);
    test_register("Math:Vec3", "Distance", test_vec3_distance, NULL, NULL);
    test_register("Math:Vec3", "Lerp", test_vec3_lerp, NULL, NULL);
    
    TestStats stats = test_run_all();
    
    printf("\n");
    printf("════════════════════════════════════════════════════════\n");
    printf("  Vec3 Math Test Results\n");
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
