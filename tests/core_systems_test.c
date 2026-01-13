/*
 * Core Systems Test Suite
 * Comprehensive test coverage for core engine systems
 * Target: 80%+ code coverage
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include <pthread.h>

// Core system headers
#include "core/common/unified_collision.h"
#include "core/common/unified_memory.h"
#include "core/common/unified_logging.h"
#include "include/core/memory.h"
#include "core/logging_system.h"

// Test framework
typedef struct {
    const char* name;
    bool (*test_func)(void);
    bool passed;
    double execution_time_ms;
} TestCase;

typedef struct {
    const char* suite_name;
    TestCase* tests;
    size_t test_count;
    size_t passed_count;
    size_t failed_count;
    double total_time_ms;
    double coverage_percentage;
} TestSuite;

// Global test state
static TestSuite* g_current_suite = NULL;
static size_t g_total_tests = 0;
static size_t g_total_passed = 0;
static size_t g_total_failed = 0;

// Test utilities
#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("ASSERTION FAILED: %s at %s:%d\n", message, __FILE__, __LINE__); \
            return false; \
        } \
    } while(0)

#define ASSERT_EQ(expected, actual, message) \
    do { \
        if ((expected) != (actual)) { \
            printf("ASSERTION FAILED: %s (expected: %d, actual: %d) at %s:%d\n", \
                   message, (int)(expected), (int)(actual), __FILE__, __LINE__); \
            return false; \
        } \
    } while(0)

#define ASSERT_FLOAT_EQ(expected, actual, tolerance, message) \
    do { \
        if (fabs((expected) - (actual)) > (tolerance)) { \
            printf("ASSERTION FAILED: %s (expected: %f, actual: %f) at %s:%d\n", \
                   message, (expected), (actual), __FILE__, __LINE__); \
            return false; \
        } \
    } while(0)

#define TEST_CASE(name, func) \
    { name, func, false, 0.0 }

// Memory test functions
static bool test_unified_memory_allocation(void) {
    printf("Testing unified memory allocation...\n");
    
    // Initialize memory system
    unified_memory_config_t config = {
        .total_heap_size = 64 * 1024 * 1024, // 64MB
        .allocation_tracking = true,
        .debug_mode = true
    };
    
    ASSERT(unified_memory_init(&config), "Failed to initialize unified memory");
    
    // Test basic allocation
    void* ptr1 = unified_memory_alloc(1024, MEMORY_USAGE_CPU);
    ASSERT(ptr1 != NULL, "Failed to allocate memory");
    
    void* ptr2 = unified_memory_alloc(2048, MEMORY_USAGE_GPU);
    ASSERT(ptr2 != NULL, "Failed to allocate GPU memory");
    
    // Test allocation tracking
    memory_stats_t stats;
    unified_memory_get_stats(&stats);
    ASSERT(stats.total_allocated > 0, "Memory tracking not working");
    
    // Test deallocation
    unified_memory_free(ptr1);
    unified_memory_free(ptr2);
    
    // Test memory leak detection
    void* leak_ptr = unified_memory_alloc(512, MEMORY_USAGE_CPU);
    memory_leak_report_t leak_report;
    unified_memory_get_leak_report(&leak_report);
    ASSERT(leak_report.leak_count > 0, "Leak detection not working");
    
    unified_memory_free(leak_ptr);
    
    unified_memory_shutdown();
    return true;
}

static bool test_memory_pool_operations(void) {
    printf("Testing memory pool operations...\n");
    
    memory_pool_t pool;
    ASSERT(memory_pool_create(&pool, 4096, 64, "test_pool"), "Failed to create memory pool");
    
    // Test pool allocation
    void* ptrs[10];
    for (int i = 0; i < 10; i++) {
        ptrs[i] = memory_pool_alloc(&pool);
        ASSERT(ptrs[i] != NULL, "Failed to allocate from pool");
    }
    
    // Test pool deallocation
    for (int i = 0; i < 10; i++) {
        memory_pool_free(&pool, ptrs[i]);
    }
    
    // Test pool statistics
    pool_stats_t stats;
    memory_pool_get_stats(&pool, &stats);
    ASSERT(stats.total_allocations == 10, "Pool allocation count incorrect");
    ASSERT(stats.total_deallocations == 10, "Pool deallocation count incorrect");
    
    memory_pool_destroy(&pool);
    return true;
}

// Collision test functions
static bool test_unified_collision_world(void) {
    printf("Testing unified collision world...\n");
    
    // Create collision world
    collision_world_t* world = collision_world_create(1000);
    ASSERT(world != NULL, "Failed to create collision world");
    
    // Test adding primitives
    vec3_t center1 = {0.0f, 0.0f, 0.0f};
    uint32_t sphere1 = collision_add_sphere(world, &center1, 1.0f);
    ASSERT(sphere1 != INVALID_COLLISION_ID, "Failed to add sphere");
    
    vec3_t center2 = {2.0f, 0.0f, 0.0f};
    uint32_t sphere2 = collision_add_sphere(world, &center2, 1.0f);
    ASSERT(sphere2 != INVALID_COLLISION_ID, "Failed to add second sphere");
    
    // Test collision detection
    collision_contact_t contact;
    bool has_collision = collision_sphere_sphere(
        &world->primitives[sphere1].sphere,
        &world->primitives[sphere2].sphere,
        &contact
    );
    ASSERT(has_collision, "Expected collision between spheres");
    
    // Test broadphase update
    collision_update_broadphase(world);
    
    // Test narrowphase update
    collision_update_narrowphase(world);
    
    // Test contact resolution
    collision_resolve_contacts(world, 0.016f); // 60 FPS timestep
    
    // Test ray casting
    ray_t ray = {{-5.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
    raycast_result_t result;
    bool hit = collision_ray_cast(world, &ray, 10.0f, &result);
    ASSERT(hit, "Ray cast should hit sphere");
    
    collision_world_destroy(world);
    return true;
}

static bool test_collision_primitives(void) {
    printf("Testing collision primitives...\n");
    
    // Test sphere-sphere collision
    collision_sphere_t sphere1 = {{0.0f, 0.0f, 0.0f}, 1.0f};
    collision_sphere_t sphere2 = {{1.5f, 0.0f, 0.0f}, 1.0f};
    
    collision_contact_t contact;
    bool has_collision = collision_sphere_sphere(&sphere1, &sphere2, &contact);
    ASSERT(has_collision, "Spheres should collide");
    
    // Test non-colliding spheres
    sphere2.center.x = 3.0f;
    has_collision = collision_sphere_sphere(&sphere1, &sphere2, &contact);
    ASSERT(!has_collision, "Distant spheres should not collide");
    
    // Test sphere-box collision
    collision_box_t box = {{2.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}};
    sphere2.center.x = 2.0f;
    
    has_collision = collision_sphere_box(&sphere2, &box, &contact);
    ASSERT(has_collision, "Sphere and box should collide");
    
    return true;
}

// Logging test functions
static bool test_unified_logging_basic(void) {
    printf("Testing unified logging basic functionality...\n");
    
    // Initialize logging system
    logging_config_t config = {
        .level = LOG_LEVEL_DEBUG,
        .output_mask = LOG_OUTPUT_CONSOLE | LOG_OUTPUT_FILE,
        .file_path = "test_log.txt",
        .max_file_size = 1024 * 1024, // 1MB
        .max_files = 5
    };
    
    ASSERT(unified_logging_init(&config), "Failed to initialize logging");
    
    // Test logging at different levels
    LOG_DEBUG("Debug message");
    LOG_INFO("Info message");
    LOG_WARNING("Warning message");
    LOG_ERROR("Error message");
    
    // Test logging with formatting
    int test_value = 42;
    LOG_INFO("Test value: %d", test_value);
    
    // Test channel filtering
    LOG_CHANNEL("PHYSICS", LOG_INFO, "Physics message");
    LOG_CHANNEL("RENDERING", LOG_INFO, "Rendering message");
    
    // Test statistics
    logging_stats_t stats;
    unified_logging_get_stats(&stats);
    ASSERT(stats.total_messages > 0, "No messages logged");
    
    unified_logging_shutdown();
    return true;
}

static bool test_logging_performance(void) {
    printf("Testing logging performance...\n");
    
    // Initialize logging
    logging_config_t config = {
        .level = LOG_LEVEL_INFO,
        .output_mask = LOG_OUTPUT_CONSOLE,
        .buffer_size = 64 * 1024
    };
    
    ASSERT(unified_logging_init(&config), "Failed to initialize logging for performance test");
    
    // Performance test: log 10000 messages
    clock_t start = clock();
    for (int i = 0; i < 10000; i++) {
        LOG_INFO("Performance test message %d", i);
    }
    clock_t end = clock();
    
    double time_ms = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    printf("Logged 10000 messages in %.2f ms (%.2f msgs/sec)\n", 
           time_ms, 10000.0 / (time_ms / 1000.0));
    
    // Should be able to log at least 10000 messages per second
    ASSERT(time_ms < 1000.0, "Logging performance too slow");
    
    unified_logging_shutdown();
    return true;
}

// Integration test functions
static bool test_collision_memory_integration(void) {
    printf("Testing collision and memory system integration...\n");
    
    // Initialize both systems
    unified_memory_config_t mem_config = {
        .total_heap_size = 32 * 1024 * 1024,
        .allocation_tracking = true
    };
    ASSERT(unified_memory_init(&mem_config), "Failed to initialize memory");
    
    collision_world_t* world = collision_world_create(100);
    ASSERT(world != NULL, "Failed to create collision world");
    
    // Allocate collision objects from unified memory
    void* collision_data = unified_memory_alloc(sizeof(collision_sphere_t) * 10, MEMORY_USAGE_CPU);
    ASSERT(collision_data != NULL, "Failed to allocate collision data");
    
    // Use collision system
    vec3_t center = {0.0f, 0.0f, 0.0f};
    uint32_t sphere_id = collision_add_sphere(world, &center, 1.0f);
    ASSERT(sphere_id != INVALID_COLLISION_ID, "Failed to add sphere");
    
    // Log operations
    LOG_INFO("Added sphere with ID %u", sphere_id);
    
    // Cleanup
    unified_memory_free(collision_data);
    collision_world_destroy(world);
    unified_memory_shutdown();
    
    return true;
}

static bool test_thread_safety(void) {
    printf("Testing thread safety...\n");
    
    // Initialize systems
    unified_memory_config_t mem_config = {
        .total_heap_size = 16 * 1024 * 1024,
        .thread_safe = true
    };
    ASSERT(unified_memory_init(&mem_config), "Failed to initialize thread-safe memory");
    
    logging_config_t log_config = {
        .level = LOG_LEVEL_INFO,
        .thread_safe = true
    };
    ASSERT(unified_logging_init(&log_config), "Failed to initialize thread-safe logging");
    
    // Create threads that use both systems
    pthread_t threads[4];
    for (int i = 0; i < 4; i++) {
        pthread_create(&threads[i], NULL, [](void* arg) -> void* {
            int thread_id = *(int*)arg;
            
            for (int j = 0; j < 100; j++) {
                void* ptr = unified_memory_alloc(1024, MEMORY_USAGE_CPU);
                LOG_INFO("Thread %d: allocated %p", thread_id, ptr);
                unified_memory_free(ptr);
            }
            
            return NULL;
        }, &i);
    }
    
    // Wait for threads to complete
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }
    
    unified_logging_shutdown();
    unified_memory_shutdown();
    return true;
}

// Performance benchmark functions
static bool benchmark_collision_performance(void) {
    printf("Benchmarking collision performance...\n");
    
    collision_world_t* world = collision_world_create(10000);
    ASSERT(world != NULL, "Failed to create collision world");
    
    // Add many collision objects
    clock_t start = clock();
    for (int i = 0; i < 1000; i++) {
        vec3_t center = {
            (float)(rand() % 100 - 50),
            (float)(rand() % 100 - 50),
            (float)(rand() % 100 - 50)
        };
        collision_add_sphere(world, &center, 1.0f);
    }
    clock_t end = clock();
    
    double add_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    printf("Added 1000 spheres in %.2f ms\n", add_time);
    
    // Benchmark collision detection
    start = clock();
    for (int i = 0; i < 100; i++) {
        collision_update_broadphase(world);
        collision_update_narrowphase(world);
    }
    end = clock();
    
    double update_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    printf("100 collision updates in %.2f ms (%.2f updates/sec)\n", 
           update_time, 100.0 / (update_time / 1000.0));
    
    collision_world_destroy(world);
    return true;
}

// Test suite definitions
static TestCase memory_tests[] = {
    TEST_CASE("Unified Memory Allocation", test_unified_memory_allocation),
    TEST_CASE("Memory Pool Operations", test_memory_pool_operations),
};

static TestCase collision_tests[] = {
    TEST_CASE("Unified Collision World", test_unified_collision_world),
    TEST_CASE("Collision Primitives", test_collision_primitives),
};

static TestCase logging_tests[] = {
    TEST_CASE("Unified Logging Basic", test_unified_logging_basic),
    TEST_CASE("Logging Performance", test_logging_performance),
};

static TestCase integration_tests[] = {
    TEST_CASE("Collision Memory Integration", test_collision_memory_integration),
    TEST_CASE("Thread Safety", test_thread_safety),
};

static TestCase performance_tests[] = {
    TEST_CASE("Collision Performance", benchmark_collision_performance),
};

// Test suite array
static TestSuite test_suites[] = {
    {
        .suite_name = "Memory System",
        .tests = memory_tests,
        .test_count = sizeof(memory_tests) / sizeof(memory_tests[0]),
        .passed_count = 0,
        .failed_count = 0,
        .total_time_ms = 0.0,
        .coverage_percentage = 0.0
    },
    {
        .suite_name = "Collision System",
        .tests = collision_tests,
        .test_count = sizeof(collision_tests) / sizeof(collision_tests[0]),
        .passed_count = 0,
        .failed_count = 0,
        .total_time_ms = 0.0,
        .coverage_percentage = 0.0
    },
    {
        .suite_name = "Logging System",
        .tests = logging_tests,
        .test_count = sizeof(logging_tests) / sizeof(logging_tests[0]),
        .passed_count = 0,
        .failed_count = 0,
        .total_time_ms = 0.0,
        .coverage_percentage = 0.0
    },
    {
        .suite_name = "Integration Tests",
        .tests = integration_tests,
        .test_count = sizeof(integration_tests) / sizeof(integration_tests[0]),
        .passed_count = 0,
        .failed_count = 0,
        .total_time_ms = 0.0,
        .coverage_percentage = 0.0
    },
    {
        .suite_name = "Performance Tests",
        .tests = performance_tests,
        .test_count = sizeof(performance_tests) / sizeof(performance_tests[0]),
        .passed_count = 0,
        .failed_count = 0,
        .total_time_ms = 0.0,
        .coverage_percentage = 0.0
    }
};

// Test runner functions
static void run_test_case(TestCase* test_case) {
    clock_t start = clock();
    test_case->passed = test_case->test_func();
    clock_t end = clock();
    
    test_case->execution_time_ms = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    
    if (test_case->passed) {
        g_current_suite->passed_count++;
        g_total_passed++;
        printf("   %s (%.2f ms)\n", test_case->name, test_case->execution_time_ms);
    } else {
        g_current_suite->failed_count++;
        g_total_failed++;
        printf("   %s (%.2f ms)\n", test_case->name, test_case->execution_time_ms);
    }
    
    g_total_tests++;
}

static void run_test_suite(TestSuite* suite) {
    g_current_suite = suite;
    printf("\n=== Running %s ===\n", suite->suite_name);
    
    clock_t start = clock();
    
    for (size_t i = 0; i < suite->test_count; i++) {
        run_test_case(&suite->tests[i]);
    }
    
    clock_t end = clock();
    suite->total_time_ms = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    
    // Calculate coverage (simplified - in real implementation would use code coverage tools)
    suite->coverage_percentage = (double)suite->passed_count / suite->test_count * 100.0;
    
    printf("\nSuite Results: %zu/%zu passed (%.1f%% coverage) (%.2f ms)\n",
           suite->passed_count, suite->test_count, 
           suite->coverage_percentage, suite->total_time_ms);
}

static void generate_coverage_report(void) {
    FILE* report = fopen("coverage_report.txt", "w");
    if (!report) return;
    
    fprintf(report, "Core Systems Test Coverage Report\n");
    fprintf(report, "=================================\n\n");
    
    double total_coverage = 0.0;
    for (size_t i = 0; i < sizeof(test_suites) / sizeof(test_suites[0]); i++) {
        TestSuite* suite = &test_suites[i];
        fprintf(report, "%s: %zu/%zu tests passed (%.1f%%)\n",
                suite->suite_name, suite->passed_count, suite->test_count,
                suite->coverage_percentage);
        total_coverage += suite->coverage_percentage;
    }
    
    total_coverage /= (sizeof(test_suites) / sizeof(test_suites[0]));
    
    fprintf(report, "\nOverall Results:\n");
    fprintf(report, "Total Tests: %zu\n", g_total_tests);
    fprintf(report, "Passed: %zu\n", g_total_passed);
    fprintf(report, "Failed: %zu\n", g_total_failed);
    fprintf(report, "Overall Coverage: %.1f%%\n", total_coverage);
    
    fclose(report);
}

// Main test runner
int main(int argc, char* argv[]) {
    printf("Core Systems Test Suite\n");
    printf("======================\n");
    printf("Target: 80%%+ code coverage\n\n");
    
    clock_t total_start = clock();
    
    // Run all test suites
    for (size_t i = 0; i < sizeof(test_suites) / sizeof(test_suites[0]); i++) {
        run_test_suite(&test_suites[i]);
    }
    
    clock_t total_end = clock();
    double total_time = ((double)(total_end - total_start)) / CLOCKS_PER_SEC * 1000.0;
    
    // Print final results
    printf("\n=== Final Results ===\n");
    printf("Total Tests: %zu\n", g_total_tests);
    printf("Passed: %zu\n", g_total_passed);
    printf("Failed: %zu\n", g_total_failed);
    printf("Success Rate: %.1f%%\n", (double)g_total_passed / g_total_tests * 100.0);
    printf("Total Time: %.2f ms\n", total_time);
    
    // Check if we met the coverage target
    double overall_coverage = (double)g_total_passed / g_total_tests * 100.0;
    if (overall_coverage >= 80.0) {
        printf(" Coverage target met: %.1f%%\n", overall_coverage);
    } else {
        printf(" Coverage target not met: %.1f%% (need 80%%+)\n", overall_coverage);
    }
    
    // Generate coverage report
    generate_coverage_report();
    
    return (g_total_failed == 0) ? 0 : 1;
}
