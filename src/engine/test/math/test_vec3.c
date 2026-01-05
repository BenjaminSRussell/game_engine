// test/math/test_vec3.c - Sample unit test for Vec3 math operations
#include "test_framework.h"
#include <math.h>
#include <math/vec3.h>

// Test fixtures
static Vec3 v1, v2, v3;

static TestResult setup(void) {
  v1 = vec3(1.0f, 2.0f, 3.0f);
  v2 = vec3(4.0f, 5.0f, 6.0f);
  v3 = vec3_zero();
  return TEST_PASS;
}

static TestResult teardown(void) { return TEST_PASS; }

// Test cases
static TestResult test_vec3_creation(void) {
  TEST_ASSERT_FLOAT_EQ(v1.x, 1.0f, 1e-6f, "Vec3 x component");
  TEST_ASSERT_FLOAT_EQ(v1.y, 2.0f, 1e-6f, "Vec3 y component");
  TEST_ASSERT_FLOAT_EQ(v1.z, 3.0f, 1e-6f, "Vec3 z component");
  return TEST_PASS;
}

static TestResult test_vec3_zero(void) {
  TEST_ASSERT_FLOAT_EQ(v3.x, 0.0f, 1e-6f, "Zero vector x");
  TEST_ASSERT_FLOAT_EQ(v3.y, 0.0f, 1e-6f, "Zero vector y");
  TEST_ASSERT_FLOAT_EQ(v3.z, 0.0f, 1e-6f, "Zero vector z");
  return TEST_PASS;
}

static TestResult test_vec3_add(void) {
  Vec3 result = vec3_add(v1, v2);
  TEST_ASSERT_FLOAT_EQ(result.x, 5.0f, 1e-6f, "Add x component");
  TEST_ASSERT_FLOAT_EQ(result.y, 7.0f, 1e-6f, "Add y component");
  TEST_ASSERT_FLOAT_EQ(result.z, 9.0f, 1e-6f, "Add z component");
  return TEST_PASS;
}

static TestResult test_vec3_sub(void) {
  Vec3 result = vec3_sub(v2, v1);
  TEST_ASSERT_FLOAT_EQ(result.x, 3.0f, 1e-6f, "Sub x component");
  TEST_ASSERT_FLOAT_EQ(result.y, 3.0f, 1e-6f, "Sub y component");
  TEST_ASSERT_FLOAT_EQ(result.z, 3.0f, 1e-6f, "Sub z component");
  return TEST_PASS;
}

static TestResult test_vec3_mul(void) {
  Vec3 result = vec3_mul(v1, 2.0f);
  TEST_ASSERT_FLOAT_EQ(result.x, 2.0f, 1e-6f, "Mul x component");
  TEST_ASSERT_FLOAT_EQ(result.y, 4.0f, 1e-6f, "Mul y component");
  TEST_ASSERT_FLOAT_EQ(result.z, 6.0f, 1e-6f, "Mul z component");
  return TEST_PASS;
}

static TestResult test_vec3_dot(void) {
  float result = vec3_dot(v1, v2);
  // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
  TEST_ASSERT_FLOAT_EQ(result, 32.0f, 1e-6f, "Dot product");
  return TEST_PASS;
}

static TestResult test_vec3_cross(void) {
  Vec3 result = vec3_cross(v1, v2);
  // (2*6 - 3*5, 3*4 - 1*6, 1*5 - 2*4) = (-3, 6, -3)
  TEST_ASSERT_FLOAT_EQ(result.x, -3.0f, 1e-6f, "Cross x component");
  TEST_ASSERT_FLOAT_EQ(result.y, 6.0f, 1e-6f, "Cross y component");
  TEST_ASSERT_FLOAT_EQ(result.z, -3.0f, 1e-6f, "Cross z component");
  return TEST_PASS;
}

static TestResult test_vec3_length(void) {
  float result = vec3_length(v1);
  // sqrt(1^2 + 2^2 + 3^2) = sqrt(14) ≈ 3.742
  TEST_ASSERT_FLOAT_EQ(result, sqrtf(14.0f), 1e-5f, "Vector length");
  return TEST_PASS;
}

static TestResult test_vec3_normalize(void) {
  Vec3 result = vec3_normalize(v1);
  float len = vec3_length(result);
  TEST_ASSERT_FLOAT_EQ(len, 1.0f, 1e-5f, "Normalized vector length");
  return TEST_PASS;
}

static TestResult test_vec3_distance(void) {
  float result = vec3_distance(v1, v2);
  // Distance between (1,2,3) and (4,5,6) = sqrt(3^2 + 3^2 + 3^2) = sqrt(27)
  TEST_ASSERT_FLOAT_EQ(result, sqrtf(27.0f), 1e-5f, "Distance between vectors");
  return TEST_PASS;
}

// Register tests
void register_vec3_tests(void) {
  TEST_REGISTER_WITH_SETUP("Math", "vec3_creation", test_vec3_creation, setup,
                           teardown);
  TEST_REGISTER_WITH_SETUP("Math", "vec3_zero", test_vec3_zero, setup,
                           teardown);
  TEST_REGISTER_WITH_SETUP("Math", "vec3_add", test_vec3_add, setup, teardown);
  TEST_REGISTER_WITH_SETUP("Math", "vec3_sub", test_vec3_sub, setup, teardown);
  TEST_REGISTER_WITH_SETUP("Math", "vec3_mul", test_vec3_mul, setup, teardown);
  TEST_REGISTER_WITH_SETUP("Math", "vec3_dot", test_vec3_dot, setup, teardown);
  TEST_REGISTER_WITH_SETUP("Math", "vec3_cross", test_vec3_cross, setup,
                           teardown);
  TEST_REGISTER_WITH_SETUP("Math", "vec3_length", test_vec3_length, setup,
                           teardown);
  TEST_REGISTER_WITH_SETUP("Math", "vec3_normalize", test_vec3_normalize, setup,
                           teardown);
  TEST_REGISTER_WITH_SETUP("Math", "vec3_distance", test_vec3_distance, setup,
                           teardown);
}
