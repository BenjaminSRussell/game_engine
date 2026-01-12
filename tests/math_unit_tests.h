/**
 * @file math_unit_tests.h
 * @brief Comprehensive unit tests for math library
 *
 * Unit test suite for vec3, quat, and mat4 math operations including
 * SIMD optimizations, edge cases, and numerical stability tests.
 */

#ifndef MATH_UNIT_TESTS_H
#define MATH_UNIT_TESTS_H

#include <stdbool.h>
#include <stdint.h>
#include "src/engine/math/vec3.h"
#include "src/engine/math/quat.h"
#include "src/engine/math/mat4.h"

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Test Configuration
// ========================================

typedef struct MathTestConfig {
    bool enable_verbose_output;         // Enable verbose test output
    bool enable_performance_tests;       // Enable performance benchmarks
    bool enable_stress_tests;           // Enable stress tests
    bool enable_edge_case_tests;        // Enable edge case tests
    float tolerance;                    // Numerical tolerance for comparisons
    int stress_test_iterations;         // Number of iterations for stress tests
    const char *output_file;            // Output file for results
} MathTestConfig;

// ========================================
// Test Results
// ========================================

typedef struct MathTestResults {
    uint32_t total_tests;               // Total tests run
    uint32_t passed_tests;              // Tests that passed
    uint32_t failed_tests;              // Tests that failed
    uint32_t skipped_tests;             // Tests that were skipped
    double total_time_ms;               // Total execution time
    char error_messages[4096];          // Accumulated error messages
} MathTestResults;

// ========================================
// Test Macros
// ========================================

#define MATH_ASSERT_FLOAT_EQ(expected, actual, tolerance) \
    do { \
        float diff = (expected) - (actual); \
        if (diff < 0) diff = -diff; \
        if (diff > (tolerance)) { \
            snprintf(current_error, sizeof(current_error), \
                    "FLOAT_EQ failed: expected %.6f, got %.6f (diff %.6f, tolerance %.6f) at %s:%d", \
                    (expected), (actual), diff, (tolerance), __FILE__, __LINE__); \
            return false; \
        } \
    } while(0)

#define MATH_ASSERT_VEC3_EQ(expected, actual, tolerance) \
    do { \
        MATH_ASSERT_FLOAT_EQ((expected).x, (actual).x, (tolerance)); \
        MATH_ASSERT_FLOAT_EQ((expected).y, (actual).y, (tolerance)); \
        MATH_ASSERT_FLOAT_EQ((expected).z, (actual).z, (tolerance)); \
    } while(0)

#define MATH_ASSERT_QUAT_EQ(expected, actual, tolerance) \
    do { \
        MATH_ASSERT_FLOAT_EQ((expected).x, (actual).x, (tolerance)); \
        MATH_ASSERT_FLOAT_EQ((expected).y, (actual).y, (tolerance)); \
        MATH_ASSERT_FLOAT_EQ((expected).z, (actual).z, (tolerance)); \
        MATH_ASSERT_FLOAT_EQ((expected).w, (actual).w, (tolerance)); \
    } while(0)

#define MATH_ASSERT_MAT4_EQ(expected, actual, tolerance) \
    do { \
        for (int i = 0; i < 16; i++) { \
            MATH_ASSERT_FLOAT_EQ((expected).m[i], (actual).m[i], (tolerance)); \
        } \
    } while(0)

#define MATH_ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            snprintf(current_error, sizeof(current_error), \
                    "TRUE assertion failed: %s at %s:%d", #condition, __FILE__, __LINE__); \
            return false; \
        } \
    } while(0)

#define MATH_ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            snprintf(current_error, sizeof(current_error), \
                    "FALSE assertion failed: %s at %s:%d", #condition, __FILE__, __LINE__); \
            return false; \
        } \
    } while(0)

// ========================================
// Global Test State
// ========================================

extern MathTestConfig g_math_test_config;
extern MathTestResults g_math_test_results;
extern char current_error[512];

// ========================================
// Test Suite Functions
// ========================================

/**
 * Initialize math test suite
 * @param config Test configuration
 * @return True if initialization successful
 */
bool math_tests_init(const MathTestConfig *config);

/**
 * Run all math unit tests
 * @return True if all tests pass
 */
bool math_run_all_tests(void);

/**
 * Get test results
 * @return Test results
 */
MathTestResults math_get_test_results(void);

/**
 * Print test summary
 */
void math_print_test_summary(void);

/**
 * Export test results to file
 * @param filename Output filename
 * @return True if export successful
 */
bool math_export_results(const char *filename);

// ========================================
// vec3 Unit Tests
// ========================================

/**
 * Test vec3 creation and basic operations
 * @return True if test passes
 */
bool test_vec3_creation_and_basic_ops(void);

/**
 * Test vec3 arithmetic operations
 * @return True if test passes
 */
bool test_vec3_arithmetic(void);

/**
 * Test vec3 dot product
 * @return True if test passes
 */
bool test_vec3_dot_product(void);

/**
 * Test vec3 cross product
 * @return True if test passes
 */
bool test_vec3_cross_product(void);

/**
 * Test vec3 length and normalization
 * @return True if test passes
 */
bool test_vec3_length_and_normalize(void);

/**
 * Test vec3 min/max operations
 * @return True if test passes
 */
bool test_vec3_min_max(void);

/**
 * Test vec3 edge cases (zero vectors, infinity, NaN)
 * @return True if test passes
 */
bool test_vec3_edge_cases(void);

/**
 * Test vec3 SIMD optimizations
 * @return True if test passes
 */
bool test_vec3_simd_optimizations(void);

/**
 * Performance test for vec3 operations
 * @return True if test passes
 */
bool test_vec3_performance(void);

// ========================================
// Quaternion Unit Tests
// ========================================

/**
 * Test quaternion creation and identity
 * @return True if test passes
 */
bool test_quat_creation_and_identity(void);

/**
 * Test quaternion from axis-angle
 * @return True if test passes
 */
bool test_quat_from_axis_angle(void);

/**
 * Test quaternion multiplication
 * @return True if test passes
 */
bool test_quat_multiplication(void);

/**
 * Test quaternion normalization
 * @return True if test passes
 */
bool test_quat_normalization(void);

/**
 * Test quaternion linear interpolation (lerp)
 * @return True if test passes
 */
bool test_quat_lerp(void);

/**
 * Test quaternion spherical linear interpolation (slerp)
 * @return True if test passes
 */
bool test_quat_slerp(void);

/**
 * Test quaternion to matrix conversion
 * @return True if test passes
 */
bool test_quat_to_matrix(void);

/**
 * Test quaternion edge cases
 * @return True if test passes
 */
bool test_quat_edge_cases(void);

/**
 * Performance test for quaternion operations
 * @return True if test passes
 */
bool test_quat_performance(void);

// ========================================
// Matrix Unit Tests
// ========================================

/**
 * Test matrix creation and identity
 * @return True if test passes
 */
bool test_mat4_creation_and_identity(void);

/**
 * Test matrix multiplication
 * @return True if test passes
 */
bool test_mat4_multiplication(void);

/**
 * Test matrix transpose
 * @return True if test passes
 */
bool test_mat4_transpose(void);

/**
 * Test matrix inverse
 * @return True if test passes
 */
bool test_mat4_inverse(void);

/**
 * Test matrix transformations (translate, scale, rotate)
 * @return True if test passes
 */
bool test_mat4_transformations(void);

/**
 * Test matrix projections (perspective, orthographic)
 * @return True if test passes
 */
bool test_mat4_projections(void);

/**
 * Test matrix look-at transformation
 * @return True if test passes
 */
bool test_mat4_look_at(void);

/**
 * Test matrix-vector multiplication
 * @return True if test passes
 */
bool test_mat4_vector_multiplication(void);

/**
 * Test matrix edge cases
 * @return True if test passes
 */
bool test_mat4_edge_cases(void);

/**
 * Performance test for matrix operations
 * @return True if test passes
 */
bool test_mat4_performance(void);

// ========================================
// Integration Tests
// ========================================

/**
 * Test coordinate system transformations
 * @return True if test passes
 */
bool test_coordinate_system_transforms(void);

/**
 * Test rotation composition with quaternions and matrices
 * @return True if test passes
 */
bool test_rotation_composition(void);

/**
 * Test view-projection matrix pipeline
 * @return True if test passes
 */
bool test_view_projection_pipeline(void);

/**
 * Test bounding box transformations
 * @return True if test passes
 */
bool test_bounding_box_transforms(void);

// ========================================
// Stress Tests
// ========================================

/**
 * Stress test with large number of vector operations
 * @return True if test passes
 */
bool stress_test_vector_operations(void);

/**
 * Stress test with large number of matrix operations
 * @return True if test passes
 */
bool stress_test_matrix_operations(void);

/**
 * Stress test with rotation operations
 * @return True if test passes
 */
bool stress_test_rotation_operations(void);

/**
 * Memory stress test for math operations
 * @return True if test passes
 */
bool stress_test_memory_usage(void);

// ========================================
// Utility Functions
// ========================================

/**
 * Generate random float in range
 * @param min Minimum value
 * @param max Maximum value
 * @return Random float
 */
float math_random_float(float min, float max);

/**
 * Generate random vec3
 * @param min Minimum value per component
 * @param max Maximum value per component
 * @return Random vec3
 */
vec3_t math_random_vec3(float min, float max);

/**
 * Generate random quaternion
 * @return Random quaternion
 */
quat_t math_random_quat(void);

/**
 * Generate random matrix
 * @return Random matrix
 */
mat4_t math_random_mat4(void);

/**
 * Check if two floats are approximately equal
 * @param a First value
 * @param b Second value
 * @param tolerance Tolerance
 * @return True if approximately equal
 */
bool math_approximately_equal(float a, float b, float tolerance);

/**
 * Check if two vec3 are approximately equal
 * @param a First vector
 * @param b Second vector
 * @param tolerance Tolerance
 * @return True if approximately equal
 */
bool math_vec3_approximately_equal(vec3_t a, vec3_t b, float tolerance);

/**
 * Check if two quaternions are approximately equal
 * @param a First quaternion
 * @param b Second quaternion
 * @param tolerance Tolerance
 * @return True if approximately equal
 */
bool math_quat_approximately_equal(quat_t a, quat_t b, float tolerance);

/**
 * Check if two matrices are approximately equal
 * @param a First matrix
 * @param b Second matrix
 * @param tolerance Tolerance
 * @return True if approximately equal
 */
bool math_mat4_approximately_equal(mat4_t a, mat4_t b, float tolerance);

// ========================================
// Benchmark Functions
// ========================================

/**
 * Benchmark vec3 operations
 * @param iterations Number of iterations
 * @param time_ms Output time in milliseconds
 */
void benchmark_vec3_operations(int iterations, double *time_ms);

/**
 * Benchmark quaternion operations
 * @param iterations Number of iterations
 * @param time_ms Output time in milliseconds
 */
void benchmark_quat_operations(int iterations, double *time_ms);

/**
 * Benchmark matrix operations
 * @param iterations Number of iterations
 * @param time_ms Output time in milliseconds
 */
void benchmark_mat4_operations(int iterations, double *time_ms);

#ifdef __cplusplus
}
#endif

#endif /* MATH_UNIT_TESTS_H */
