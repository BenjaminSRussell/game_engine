/*
 * integration_test_suite.c
 * Integration test suite for Phase 5 implementation
 *
 * Tests system interactions, end-to-end workflows, and performance under load
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#include "gpu_cull_compute.h"
#include "cull_output_buffer.h"
#include "cull_statistics.h"

/* ============================================================================
 * INTEGRATION TEST FRAMEWORK
 * ============================================================================ */

#define INTEGRATION_TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("INTEGRATION FAIL: %s\n", message); \
            return 0; \
        } \
    } while(0)

static int integration_tests_run = 0;
static int integration_tests_passed = 0;

#define RUN_INTEGRATION_TEST(test_func) \
    do { \
        printf("Running integration %s... ", #test_func); \
        integration_tests_run++; \
        if (test_func()) { \
            printf("PASS\n"); \
            integration_tests_passed++; \
        } \
    } while(0)

/* ============================================================================
 * RENDERING PIPELINE INTEGRATION
 * ============================================================================ */

static int test_rendering_pipeline_integration(void) {
    printf("\n");
    
    // Initialize GPU culling system
    int result = culling_gpu_cull_compute_init();
    INTEGRATION_TEST_ASSERT(result == 0, "GPU culling system should initialize");
    
    // Create culling instance for rendering
    culling_gpu_cull_compute_handle_t cull_handle;
    culling_gpu_cull_compute_desc_t cull_desc = {
        .flags = 0x01  // Enable rendering optimizations
    };
    
    result = culling_gpu_cull_compute_create(&cull_handle, &cull_desc);
    INTEGRATION_TEST_ASSERT(result == 0, "Should create culling instance");
    
    // Simulate rendering frame with many objects
    uint32_t frame_objects = 10000;
    result = culling_gpu_cull_compute_update(cull_handle, &frame_objects, sizeof(frame_objects));
    INTEGRATION_TEST_ASSERT(result == 0, "Should update with frame objects");
    
    // Process culling for frame
    int processed = culling_gpu_cull_compute_process_pending();
    INTEGRATION_TEST_ASSERT(processed > 0, "Should process frame culling");
    
    // Verify culling statistics
    culling_gpu_cull_compute_info_t info;
    result = culling_gpu_cull_compute_get_info(cull_handle, &info);
    INTEGRATION_TEST_ASSERT(result == 0, "Should get culling info");
    INTEGRATION_TEST_ASSERT(info.initialized, "Instance should be initialized");
    
    // Cleanup
    culling_gpu_cull_compute_destroy(cull_handle);
    culling_gpu_cull_compute_shutdown();
    
    return 1;
}

/* ============================================================================
 * MULTI-THREADING INTEGRATION
 * ============================================================================ */

typedef struct {
    int thread_id;
    int operations;
    int successes;
} thread_test_data_t;

static void* culling_thread_worker(void* arg) {
    thread_test_data_t* data = (thread_test_data_t*)arg;
    
    for (int i = 0; i < data->operations; i++) {
        culling_gpu_cull_compute_handle_t handle;
        culling_gpu_cull_compute_desc_t desc = {0};
        
        if (culling_gpu_cull_compute_create(&handle, &desc) == 0) {
            uint32_t objects = 100 + (i % 900);
            if (culling_gpu_cull_compute_update(handle, &objects, sizeof(objects)) == 0) {
                culling_gpu_cull_compute_process_pending();
                data->successes++;
            }
            culling_gpu_cull_compute_destroy(handle);
        }
        
        usleep(1000);  // 1ms delay
    }
    
    return NULL;
}

static int test_multi_threading_integration(void) {
    printf("\n");
    
    culling_gpu_cull_compute_init();
    
    const int num_threads = 4;
    const int operations_per_thread = 50;
    
    pthread_t threads[num_threads];
    thread_test_data_t thread_data[num_threads];
    
    // Create threads
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].operations = operations_per_thread;
        thread_data[i].successes = 0;
        
        int result = pthread_create(&threads[i], NULL, culling_thread_worker, &thread_data[i]);
        INTEGRATION_TEST_ASSERT(result == 0, "Should create thread");
    }
    
    // Wait for threads to complete
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Verify results
    int total_successes = 0;
    for (int i = 0; i < num_threads; i++) {
        total_successes += thread_data[i].successes;
        printf("Thread %d: %d/%d operations succeeded\n", 
               i, thread_data[i].successes, operations_per_thread);
    }
    
    INTEGRATION_TEST_ASSERT(total_successes > 0, "Should have some successful operations");
    INTEGRATION_TEST_ASSERT(total_successes == num_threads * operations_per_thread, 
                           "All operations should succeed");
    
    culling_gpu_cull_compute_shutdown();
    return 1;
}

/* ============================================================================
 * PERFORMANCE BENCHMARK INTEGRATION
 * ============================================================================ */

static int test_performance_benchmark_integration(void) {
    printf("\n");
    
    culling_gpu_cull_compute_init();
    
    const int benchmark_iterations = 10000;
    const int objects_per_iteration = 1000;
    
    culling_gpu_cull_compute_handle_t handle;
    culling_gpu_cull_compute_desc_t desc = {0};
    culling_gpu_cull_compute_create(&handle, &desc);
    
    printf("Running %d iterations with %d objects each...\n", 
           benchmark_iterations, objects_per_iteration);
    
    clock_t start = clock();
    
    for (int i = 0; i < benchmark_iterations; i++) {
        uint32_t objects = objects_per_iteration + (i % 500);
        culling_gpu_cull_compute_update(handle, &objects, sizeof(objects));
        culling_gpu_cull_compute_process_pending();
    }
    
    clock_t end = clock();
    double elapsed_seconds = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    double ops_per_second = benchmark_iterations / elapsed_seconds;
    double avg_time_per_op = (elapsed_seconds / benchmark_iterations) * 1000.0; // ms
    
    printf("Benchmark Results:\n");
    printf("  Total time: %.3f seconds\n", elapsed_seconds);
    printf("  Operations per second: %.0f\n", ops_per_second);
    printf("  Average time per operation: %.3f ms\n", avg_time_per_op);
    
    // Performance assertions
    INTEGRATION_TEST_ASSERT(ops_per_second > 1000, "Should achieve >1000 ops/sec");
    INTEGRATION_TEST_ASSERT(avg_time_per_op < 1.0, "Should average <1ms per operation");
    
    // Memory usage check
    size_t memory_usage = culling_gpu_cull_compute_get_memory_usage();
    printf("  Memory usage: %zu bytes\n", memory_usage);
    INTEGRATION_TEST_ASSERT(memory_usage < 10 * 1024 * 1024, "Memory usage should be reasonable");
    
    culling_gpu_cull_compute_destroy(handle);
    culling_gpu_cull_compute_shutdown();
    
    return 1;
}

/* ============================================================================
 * STRESS TEST INTEGRATION
 * ============================================================================ */

static int test_stress_integration(void) {
    printf("\n");
    
    culling_gpu_cull_compute_init();
    
    const int stress_instances = 500;
    const int stress_operations = 100;
    
    printf("Creating %d instances with %d operations each...\n", 
           stress_instances, stress_operations);
    
    culling_gpu_cull_compute_handle_t* handles = 
        malloc(sizeof(culling_gpu_cull_compute_handle_t) * stress_instances);
    INTEGRATION_TEST_ASSERT(handles != NULL, "Should allocate handles");
    
    // Create many instances
    clock_t start = clock();
    for (int i = 0; i < stress_instances; i++) {
        culling_gpu_cull_compute_desc_t desc = {.flags = i % 4};
        int result = culling_gpu_cull_compute_create(&handles[i], &desc);
        INTEGRATION_TEST_ASSERT(result == 0, "Should create stress instance");
    }
    
    // Perform operations on all instances
    for (int op = 0; op < stress_operations; op++) {
        for (int i = 0; i < stress_instances; i++) {
            uint32_t objects = 50 + (i * 2) + (op % 100);
            culling_gpu_cull_compute_update(handles[i], &objects, sizeof(objects));
        }
        culling_gpu_cull_compute_process_pending();
        
        if (op % 20 == 0) {
            printf("  Operation %d/%d completed\n", op + 1, stress_operations);
        }
    }
    
    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("Stress test completed in %.3f seconds\n", elapsed);
    INTEGRATION_TEST_ASSERT(elapsed < 30.0, "Stress test should complete in reasonable time");
    
    // Verify all instances are still valid
    uint32_t final_count = culling_gpu_cull_compute_get_count();
    INTEGRATION_TEST_ASSERT(final_count == stress_instances, "All instances should remain valid");
    
    // Cleanup
    for (int i = 0; i < stress_instances; i++) {
        culling_gpu_cull_compute_destroy(handles[i]);
    }
    free(handles);
    
    culling_gpu_cull_compute_shutdown();
    return 1;
}

/* ============================================================================
 * MEMORY LEAK DETECTION INTEGRATION
 * ============================================================================ */

static int test_memory_leak_integration(void) {
    printf("\n");
    
    size_t initial_memory = 0;
    size_t peak_memory = 0;
    
    // Multiple initialize/shutdown cycles
    for (int cycle = 0; cycle < 10; cycle++) {
        culling_gpu_cull_compute_init();
        
        if (cycle == 0) {
            initial_memory = culling_gpu_cull_compute_get_memory_usage();
        }
        
        // Create and destroy many instances
        culling_gpu_cull_compute_handle_t handles[100];
        for (int i = 0; i < 100; i++) {
            culling_gpu_cull_compute_desc_t desc = {0};
            culling_gpu_cull_compute_create(&handles[i], &desc);
        }
        
        size_t current_memory = culling_gpu_cull_compute_get_memory_usage();
        if (current_memory > peak_memory) {
            peak_memory = current_memory;
        }
        
        // Update and process
        for (int i = 0; i < 100; i++) {
            uint32_t objects = 100 + i;
            culling_gpu_cull_compute_update(handles[i], &objects, sizeof(objects));
        }
        culling_gpu_cull_compute_process_pending();
        
        // Cleanup
        for (int i = 0; i < 100; i++) {
            culling_gpu_cull_compute_destroy(handles[i]);
        }
        
        culling_gpu_cull_compute_shutdown();
        
        printf("  Cycle %d: memory tracked\n", cycle + 1);
    }
    
    printf("Memory Analysis:\n");
    printf("  Initial memory: %zu bytes\n", initial_memory);
    printf("  Peak memory: %zu bytes\n", peak_memory);
    
    // Final check - system should be clean
    culling_gpu_cull_compute_init();
    size_t final_memory = culling_gpu_cull_compute_get_memory_usage();
    culling_gpu_cull_compute_shutdown();
    
    printf("  Final memory: %zu bytes\n", final_memory);
    
    // Memory should be back to baseline
    INTEGRATION_TEST_ASSERT(final_memory <= initial_memory * 2, 
                           "Memory should return to baseline after cleanup");
    
    return 1;
}

/* ============================================================================
 * MAIN INTEGRATION TEST RUNNER
 * ============================================================================ */

int main(void) {
    printf("=== Integration Test Suite ===\n");
    printf("Testing system interactions and end-to-end workflows\n\n");
    
    // Run integration tests
    RUN_INTEGRATION_TEST(test_rendering_pipeline_integration);
    RUN_INTEGRATION_TEST(test_multi_threading_integration);
    RUN_INTEGRATION_TEST(test_performance_benchmark_integration);
    RUN_INTEGRATION_TEST(test_stress_integration);
    RUN_INTEGRATION_TEST(test_memory_leak_integration);
    
    printf("\n=== Integration Test Results ===\n");
    printf("Tests run: %d\n", integration_tests_run);
    printf("Tests passed: %d\n", integration_tests_passed);
    printf("Tests failed: %d\n", integration_tests_run - integration_tests_passed);
    printf("Success rate: %.1f%%\n", (float)integration_tests_passed / integration_tests_run * 100.0f);
    
    if (integration_tests_passed == integration_tests_run) {
        printf("\n🚀 ALL INTEGRATION TESTS PASSED! 🚀\n");
        printf("System is ready for production deployment.\n");
        return 0;
    } else {
        printf("\n⚠️  SOME INTEGRATION TESTS FAILED ⚠️\n");
        printf("Review failed tests before deployment.\n");
        return 1;
    }
}
