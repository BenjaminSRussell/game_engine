#include "math_unit_tests.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

MathTestConfig g_math_test_config;
MathTestResults g_math_test_results;
char current_error[512];

static uint64_t get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
}

bool math_tests_init(const MathTestConfig *config) {
    if (config) {
        g_math_test_config = *config;
    } else {
        g_math_test_config = (MathTestConfig){
            .enable_verbose_output = false,
            .enable_performance_tests = true,
            .enable_stress_tests = false,
            .enable_edge_case_tests = true,
            .tolerance = 1e-6f,
            .stress_test_iterations = 10000,
            .output_file = NULL
        };
    }
    
    memset(&g_math_test_results, 0, sizeof(g_math_test_results));
    memset(current_error, 0, sizeof(current_error));
    
    srand((unsigned int)time(NULL));
    return true;
}

bool test_vec3_creation_and_basic_ops(void) {
    vec3_t v = vec3_set(1.0f, 2.0f, 3.0f);
    MATH_ASSERT_FLOAT_EQ(v.x, 1.0f, g_math_test_config.tolerance);
    MATH_ASSERT_FLOAT_EQ(v.y, 2.0f, g_math_test_config.tolerance);
    MATH_ASSERT_FLOAT_EQ(v.z, 3.0f, g_math_test_config.tolerance);
    
    vec3_t zero = vec3_zero();
    MATH_ASSERT_VEC3_EQ(zero, vec3_set(0.0f, 0.0f, 0.0f), g_math_test_config.tolerance);
    
    return true;
}

bool test_vec3_arithmetic(void) {
    vec3_t a = vec3_set(1.0f, 2.0f, 3.0f);
    vec3_t b = vec3_set(4.0f, 5.0f, 6.0f);
    
    vec3_t sum = vec3_add(a, b);
    MATH_ASSERT_VEC3_EQ(sum, vec3_set(5.0f, 7.0f, 9.0f), g_math_test_config.tolerance);
    
    vec3_t diff = vec3_sub(b, a);
    MATH_ASSERT_VEC3_EQ(diff, vec3_set(3.0f, 3.0f, 3.0f), g_math_test_config.tolerance);
    
    vec3_t scaled = vec3_scale(a, 2.0f);
    MATH_ASSERT_VEC3_EQ(scaled, vec3_set(2.0f, 4.0f, 6.0f), g_math_test_config.tolerance);
    
    return true;
}

bool test_vec3_dot_product(void) {
    vec3_t a = vec3_set(1.0f, 2.0f, 3.0f);
    vec3_t b = vec3_set(4.0f, 5.0f, 6.0f);
    
    float dot = vec3_dot(a, b);
    MATH_ASSERT_FLOAT_EQ(dot, 32.0f, g_math_test_config.tolerance);
    
    float dot_self = vec3_dot(a, a);
    MATH_ASSERT_FLOAT_EQ(dot_self, 14.0f, g_math_test_config.tolerance);
    
    return true;
}

bool test_vec3_cross_product(void) {
    vec3_t a = vec3_set(1.0f, 0.0f, 0.0f);
    vec3_t b = vec3_set(0.0f, 1.0f, 0.0f);
    
    vec3_t cross = vec3_cross(a, b);
    MATH_ASSERT_VEC3_EQ(cross, vec3_set(0.0f, 0.0f, 1.0f), g_math_test_config.tolerance);
    
    vec3_t cross_reverse = vec3_cross(b, a);
    MATH_ASSERT_VEC3_EQ(cross_reverse, vec3_set(0.0f, 0.0f, -1.0f), g_math_test_config.tolerance);
    
    return true;
}

bool test_quat_creation_and_identity(void) {
    quat_t identity = quat_identity();
    MATH_ASSERT_FLOAT_EQ(identity.x, 0.0f, g_math_test_config.tolerance);
    MATH_ASSERT_FLOAT_EQ(identity.y, 0.0f, g_math_test_config.tolerance);
    MATH_ASSERT_FLOAT_EQ(identity.z, 0.0f, g_math_test_config.tolerance);
    MATH_ASSERT_FLOAT_EQ(identity.w, 1.0f, g_math_test_config.tolerance);
    
    return true;
}

bool test_mat4_creation_and_identity(void) {
    mat4_t identity = mat4_identity();
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            float expected = (i == j) ? 1.0f : 0.0f;
            MATH_ASSERT_FLOAT_EQ(identity.e[i][j], expected, g_math_test_config.tolerance);
        }
    }
    
    return true;
}

bool test_mat4_multiplication(void) {
    mat4_t a = mat4_identity();
    mat4_t b = mat4_identity();
    
    mat4_t result = mat4_mul(a, b);
    MATH_ASSERT_MAT4_EQ(result, mat4_identity(), g_math_test_config.tolerance);
    
    return true;
}

float math_random_float(float min, float max) {
    float scale = rand() / (float)RAND_MAX;
    return min + scale * (max - min);
}

bool math_run_all_tests(void) {
    uint64_t start_time = get_time_ms();
    
    bool (*tests[])(void) = {
        test_vec3_creation_and_basic_ops,
        test_vec3_arithmetic,
        test_vec3_dot_product,
        test_vec3_cross_product,
        test_quat_creation_and_identity,
        test_mat4_creation_and_identity,
        test_mat4_multiplication
    };
    
    int num_tests = sizeof(tests) / sizeof(tests[0]);
    
    for (int i = 0; i < num_tests; i++) {
        g_math_test_results.total_tests++;
        
        if (tests[i]()) {
            g_math_test_results.passed_tests++;
        } else {
            g_math_test_results.failed_tests++;
            strncat(g_math_test_results.error_messages, current_error, 
                    sizeof(g_math_test_results.error_messages) - strlen(g_math_test_results.error_messages) - 1);
            strncat(g_math_test_results.error_messages, "\n", 
                    sizeof(g_math_test_results.error_messages) - strlen(g_math_test_results.error_messages) - 1);
        }
    }
    
    g_math_test_results.total_time_ms = get_time_ms() - start_time;
    
    return g_math_test_results.failed_tests == 0;
}

void math_print_test_summary(void) {
    printf("\n=== Math Unit Test Summary ===\n");
    printf("Total Tests: %u\n", g_math_test_results.total_tests);
    printf("Passed: %u\n", g_math_test_results.passed_tests);
    printf("Failed: %u\n", g_math_test_results.failed_tests);
    printf("Skipped: %u\n", g_math_test_results.skipped_tests);
    printf("Total Time: %.2f ms\n", g_math_test_results.total_time_ms);
    
    if (g_math_test_results.failed_tests > 0) {
        printf("\nFailed Tests:\n%s\n", g_math_test_results.error_messages);
    }
    
    printf("=============================\n");
}
