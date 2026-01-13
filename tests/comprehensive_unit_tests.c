/*
 * comprehensive_unit_tests.c
 * Comprehensive unit test suite for Phase 5 implementation
 *
 * Tests GPU culling, memory management, performance, and integration
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <pthread.h>

#include "gpu_cull_compute.h"
#include "cull_output_buffer.h"
#include "cull_statistics.h"

/* ============================================================================
 * TEST FRAMEWORK
 * ============================================================================ */

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s\n", message); \
            return 0; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(expected, actual, message) \
    do { \
        if ((expected) != (actual)) { \
            printf("FAIL: %s (expected %d, got %d)\n", message, (int)(expected), (int)(actual)); \
            return 0; \
        } \
    } while(0)

static int tests_run = 0;
static int tests_passed = 0;

#define RUN_TEST(test_func) \
    do { \
        printf("Running %s... ", #test_func); \
        tests_run++; \
        if (test_func()) { \
            printf("PASS\n"); \
            tests_passed++; \
        } \
    } while(0)

/* ============================================================================
 * GPU CULLING TESTS
 * ============================================================================ */

static int test_gpu_cull_init_shutdown(void) {
    int result = culling_gpu_cull_compute_init();
    TEST_ASSERT_EQ(0, result, "GPU cull init should succeed");
    
    culling_gpu_cull_compute_shutdown();
    return 1;
}

static int test_gpu_cull_create_destroy(void) {
    culling_gpu_cull_compute_init();
    
    culling_gpu_cull_compute_handle_t handle;
    culling_gpu_cull_compute_desc_t desc = {0};
    
    int result = culling_gpu_cull_compute_create(&handle, &desc);
    TEST_ASSERT_EQ(0, result, "GPU cull create should succeed");
    
    bool valid = culling_gpu_cull_compute_is_valid(handle);
    TEST_ASSERT(valid, "Handle should be valid");
    
    culling_gpu_cull_compute_destroy(handle);
    culling_gpu_cull_compute_shutdown();
    return 1;
}

static int test_gpu_cull_update_process(void) {
    culling_gpu_cull_compute_init();
    
    culling_gpu_cull_compute_handle_t handle;
    culling_gpu_cull_compute_desc_t desc = {0};
    culling_gpu_cull_compute_create(&handle, &desc);
    
    uint32_t object_count = 1000;
    int result = culling_gpu_cull_compute_update(handle, &object_count, sizeof(object_count));
    TEST_ASSERT_EQ(0, result, "GPU cull update should succeed");
    
    int processed = culling_gpu_cull_compute_process_pending();
    TEST_ASSERT(processed > 0, "Should process pending culls");
    
    culling_gpu_cull_compute_destroy(handle);
    culling_gpu_cull_compute_shutdown();
    return 1;
}

/* ============================================================================
 * MEMORY TESTS
 * ============================================================================ */

static int test_memory_allocation_tracking(void) {
    culling_gpu_cull_compute_init();
    
    size_t initial_memory = culling_gpu_cull_compute_get_memory_usage();
    TEST_ASSERT(initial_memory > 0, "Should track initial memory usage");
    
    culling_gpu_cull_compute_handle_t handles[10];
    culling_gpu_cull_compute_desc_t desc = {0};
    
    for (int i = 0; i < 10; i++) {
        culling_gpu_cull_compute_create(&handles[i], &desc);
    }
    
    size_t after_creation = culling_gpu_cull_compute_get_memory_usage();
    TEST_ASSERT(after_creation > initial_memory, "Memory usage should increase");
    
    for (int i = 0; i < 10; i++) {
        culling_gpu_cull_compute_destroy(handles[i]);
    }
    
    culling_gpu_cull_compute_shutdown();
    return 1;
}

/* ============================================================================
 * PERFORMANCE TESTS
 * ============================================================================ */

static int test_performance_culling_speed(void) {
    culling_gpu_cull_compute_init();
    
    culling_gpu_cull_compute_handle_t handle;
    culling_gpu_cull_compute_desc_t desc = {0};
    culling_gpu_cull_compute_create(&handle, &desc);
    
    clock_t start = clock();
    
    for (int i = 0; i < 1000; i++) {
        uint32_t object_count = 100 + (i % 900);
        culling_gpu_cull_compute_update(handle, &object_count, sizeof(object_count));
        culling_gpu_cull_compute_process_pending();
    }
    
    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    TEST_ASSERT(elapsed < 1.0, "1000 culls should complete in < 1 second");
    
    culling_gpu_cull_compute_destroy(handle);
    culling_gpu_cull_compute_shutdown();
    return 1;
}

/* ============================================================================
 * INTEGRATION TESTS
 * ============================================================================ */

static int test_integration_multiple_instances(void) {
    culling_gpu_cull_compute_init();
    
    culling_gpu_cull_compute_handle_t handles[5];
    culling_gpu_cull_compute_desc_t desc = {0};
    
    // Create multiple instances
    for (int i = 0; i < 5; i++) {
        int result = culling_gpu_cull_compute_create(&handles[i], &desc);
        TEST_ASSERT_EQ(0, result, "Multiple instances should create successfully");
    }
    
    // Update all instances
    for (int i = 0; i < 5; i++) {
        uint32_t object_count = (i + 1) * 200;
        culling_gpu_cull_compute_update(handles[i], &object_count, sizeof(object_count));
    }
    
    // Process all
    int processed = culling_gpu_cull_compute_process_pending();
    TEST_ASSERT_EQ(5, processed, "Should process all 5 instances");
    
    // Cleanup
    for (int i = 0; i < 5; i++) {
        culling_gpu_cull_compute_destroy(handles[i]);
    }
    
    culling_gpu_cull_compute_shutdown();
    return 1;
}

/* ============================================================================
 * STRESS TESTS
 * ============================================================================ */

static int test_stress_maximum_instances(void) {
    culling_gpu_cull_compute_init();
    
    culling_gpu_cull_compute_handle_t handles[100];
    culling_gpu_cull_compute_desc_t desc = {0};
    
    // Create many instances
    for (int i = 0; i < 100; i++) {
        int result = culling_gpu_cull_compute_create(&handles[i], &desc);
        TEST_ASSERT_EQ(0, result, "Should handle many instances");
    }
    
    uint32_t count = culling_gpu_cull_compute_get_count();
    TEST_ASSERT_EQ(100, count, "Should track correct instance count");
    
    // Cleanup
    for (int i = 0; i < 100; i++) {
        culling_gpu_cull_compute_destroy(handles[i]);
    }
    
    culling_gpu_cull_compute_shutdown();
    return 1;
}

/* ============================================================================
 * MAIN TEST RUNNER
 * ============================================================================ */

int main(void) {
    printf("=== Comprehensive Unit Test Suite ===\n\n");
    
    // GPU Culling Tests
    RUN_TEST(test_gpu_cull_init_shutdown);
    RUN_TEST(test_gpu_cull_create_destroy);
    RUN_TEST(test_gpu_cull_update_process);
    
    // Memory Tests
    RUN_TEST(test_memory_allocation_tracking);
    
    // Performance Tests
    RUN_TEST(test_performance_culling_speed);
    
    // Integration Tests
    RUN_TEST(test_integration_multiple_instances);
    
    // Stress Tests
    RUN_TEST(test_stress_maximum_instances);
    
    printf("\n=== Test Results ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    printf("Success rate: %.1f%%\n", (float)tests_passed / tests_run * 100.0f);
    
    if (tests_passed == tests_run) {
        printf("\n🎉 ALL TESTS PASSED! 🎉\n");
        return 0;
    } else {
        printf("\n❌ SOME TESTS FAILED ❌\n");
        return 1;
    }
}
