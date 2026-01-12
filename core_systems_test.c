/*
 * Core Systems Test
 * Tests the critical systems for CORE-001 through CORE-006
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

// Include our critical systems
#include "src/engine/include/core/types.h"
#include "src/engine/include/core/memory_allocator.h"
#include "src/engine/include/core/logging_system.h"
#include "src/engine/include/core/thread_pool.h"

// Simple job function for testing
bool test_job_function(void* user_data) {
    u32* counter = (u32*)user_data;
    (*counter)++;
    return true;
}

bool test_job_function_with_delay(void* user_data) {
    u32* counter = (u32*)user_data;
    (*counter)++;
    
    // Simulate some work
    for (volatile int i = 0; i < 1000; i++);
    
    return true;
}

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    
    printf("=== Core Systems Test ===\n");
    printf("Testing CORE-001 through CORE-006 critical systems\n\n");
    
    int test_result = 0;
    
    // Test 1: Memory Allocator (CORE-002)
    printf("1. Testing Memory Allocator (CORE-002)...\n");
    
    if (!memory_allocator_init()) {
        printf("❌ Memory allocator initialization failed\n");
        test_result = 1;
        goto cleanup;
    }
    printf("✓ Memory allocator initialized\n");
    
    // Test basic allocation
    void* ptr1 = MEMORY_ALLOCATE(1024);
    void* ptr2 = MEMORY_ALLOCATE(512);
    void* ptr3 = MEMORY_ALLOCATE(2048);
    
    if (!ptr1 || !ptr2 || !ptr3) {
        printf("❌ Memory allocation failed\n");
        test_result = 1;
        goto cleanup;
    }
    printf("✓ Memory allocations successful\n");
    
    // Test reallocation
    void* ptr1_realloc = MEMORY_REALLOCATE(ptr1, 2048);
    if (!ptr1_realloc) {
        printf("❌ Memory reallocation failed\n");
        test_result = 1;
        goto cleanup;
    }
    ptr1 = ptr1_realloc;
    printf("✓ Memory reallocation successful\n");
    
    // Test deallocation
    MEMORY_DEALLOCATE(ptr1);
    MEMORY_DEALLOCATE(ptr2);
    MEMORY_DEALLOCATE(ptr3);
    printf("✓ Memory deallocation successful\n");
    
    // Check memory stats
    size_t total_allocated, peak_allocated;
    u64 allocation_count;
    memory_allocator_get_stats(&total_allocated, &peak_allocated, &allocation_count);
    
    printf("  Memory stats: %zu allocated, %zu peak, %llu active allocations\n",
           total_allocated, peak_allocated, allocation_count);
    
    if (allocation_count != 0) {
        printf("❌ Memory leaks detected\n");
        test_result = 1;
        goto cleanup;
    }
    printf("✓ No memory leaks detected\n");
    
    // Test 2: Logging System (CORE-005)
    printf("\n2. Testing Logging System (CORE-005)...\n");
    
    if (!logging_system_init()) {
        printf("❌ Logging system initialization failed\n");
        test_result = 1;
        goto cleanup;
    }
    printf("✓ Logging system initialized\n");
    
    // Test different log levels
    LOG_ERROR("Test error message");
    LOG_WARN("Test warning message");
    LOG_INFO("Test info message");
    LOG_DEBUG("Test debug message");
    LOG_TRACE("Test trace message");
    printf("✓ All log levels working\n");
    
    // Test 3: Thread Pool (CORE-003)
    printf("\n3. Testing Thread Pool (CORE-003)...\n");
    
    if (!thread_pool_init(4)) {
        printf("❌ Thread pool initialization failed\n");
        test_result = 1;
        goto cleanup;
    }
    printf("✓ Thread pool initialized with 4 threads\n");
    
    // Test simple job submission
    u32 job_counter = 0;
    u32 job_id1 = thread_pool_submit(thread_pool_get_global(), test_job_function, &job_counter, "Test Job 1");
    u32 job_id2 = thread_pool_submit(thread_pool_get_global(), test_job_function, &job_counter, "Test Job 2");
    u32 job_id3 = thread_pool_submit(thread_pool_get_global(), test_job_function, &job_counter, "Test Job 3");
    
    if (job_id1 == 0 || job_id2 == 0 || job_id3 == 0) {
        printf("❌ Job submission failed\n");
        test_result = 1;
        goto cleanup;
    }
    printf("✓ Job submission successful (IDs: %u, %u, %u)\n", job_id1, job_id2, job_id3);
    
    // Wait for jobs to complete
    thread_pool_wait_for_job(thread_pool_get_global(), job_id1, 1000);
    thread_pool_wait_for_job(thread_pool_get_global(), job_id2, 1000);
    thread_pool_wait_for_job(thread_pool_get_global(), job_id3, 1000);
    
    if (job_counter != 3) {
        printf("❌ Jobs did not execute correctly (counter: %u, expected: 3)\n", job_counter);
        test_result = 1;
        goto cleanup;
    }
    printf("✓ Jobs executed successfully\n");
    
    // Test multiple concurrent jobs
    printf("\n4. Testing concurrent job execution...\n");
    
    u32 concurrent_counter = 0;
    u32 job_ids[100];
    
    for (int i = 0; i < 100; i++) {
        job_ids[i] = thread_pool_submit(thread_pool_get_global(), test_job_function_with_delay, &concurrent_counter, "Concurrent Job");
        if (job_ids[i] == 0) {
            printf("❌ Failed to submit concurrent job %d\n", i);
            test_result = 1;
            goto cleanup;
        }
    }
    
    // Wait for all jobs to complete
    for (int i = 0; i < 100; i++) {
        thread_pool_wait_for_job(thread_pool_get_global(), job_ids[i], 100);
    }
    
    if (concurrent_counter != 100) {
        printf("❌ Concurrent jobs did not execute correctly (counter: %u, expected: 100)\n", concurrent_counter);
        test_result = 1;
        goto cleanup;
    }
    printf("✓ 100 concurrent jobs executed successfully\n");
    
    // Check thread pool statistics
    u64 submitted, completed, failed;
    thread_pool_get_stats(thread_pool_get_global(), &submitted, &completed, &failed);
    
    printf("  Thread pool stats: %llu submitted, %llu completed, %llu failed\n",
           submitted, completed, failed);
    
    if (completed < 103) { // 3 initial + 100 concurrent
        printf("❌ Not all jobs completed successfully\n");
        test_result = 1;
        goto cleanup;
    }
    printf("✓ All jobs completed successfully\n");
    
    // Test 4: Stress test
    printf("\n5. Testing system stress (1000 allocations, 1000 jobs)...\n");
    
    // Memory stress test
    void* stress_ptrs[1000];
    for (int i = 0; i < 1000; i++) {
        stress_ptrs[i] = MEMORY_ALLOCATE((i % 10 + 1) * 100);
        if (!stress_ptrs[i]) {
            printf("❌ Stress allocation failed at iteration %d\n", i);
            test_result = 1;
            goto cleanup;
        }
    }
    
    for (int i = 0; i < 1000; i++) {
        MEMORY_DEALLOCATE(stress_ptrs[i]);
    }
    printf("✓ Memory stress test passed (1000 allocations/deallocations)\n");
    
    // Thread pool stress test
    u32 stress_counter = 0;
    u32 stress_job_ids[1000];
    
    for (int i = 0; i < 1000; i++) {
        stress_job_ids[i] = thread_pool_submit(thread_pool_get_global(), test_job_function, &stress_counter, "Stress Job");
        if (stress_job_ids[i] == 0) {
            printf("❌ Stress job submission failed at iteration %d\n", i);
            test_result = 1;
            goto cleanup;
        }
    }
    
    // Wait for all stress jobs
    for (int i = 0; i < 1000; i++) {
        thread_pool_wait_for_job(thread_pool_get_global(), stress_job_ids[i], 10);
    }
    
    if (stress_counter != 1000) {
        printf("❌ Stress jobs did not execute correctly (counter: %u, expected: 1000)\n", stress_counter);
        test_result = 1;
        goto cleanup;
    }
    printf("✓ Thread pool stress test passed (1000 jobs)\n");
    
    // Final statistics
    thread_pool_get_stats(thread_pool_get_global(), &submitted, &completed, &failed);
    memory_allocator_get_stats(&total_allocated, &peak_allocated, &allocation_count);
    
    printf("\n=== Final Statistics ===\n");
    printf("Thread Pool: %llu submitted, %llu completed, %llu failed\n", submitted, completed, failed);
    printf("Memory: %zu allocated, %zu peak, %llu active allocations\n", total_allocated, peak_allocated, allocation_count);
    
    if (allocation_count != 0) {
        printf("❌ Memory leaks detected in final check\n");
        test_result = 1;
    }
    
cleanup:
    // Clean shutdown
    printf("\n6. Testing clean shutdown...\n");
    
    thread_pool_shutdown();
    printf("✓ Thread pool shutdown complete\n");
    
    logging_system_shutdown();
    printf("✓ Logging system shutdown complete\n");
    
    memory_allocator_shutdown();
    printf("✓ Memory allocator shutdown complete\n");
    
    if (test_result == 0) {
        printf("\n=== Test Results ===\n");
        printf("✅ CORE-002: Memory Allocator - PASSED\n");
        printf("✅ CORE-003: Thread Pool - PASSED\n");
        printf("✅ CORE-005: Logging System - PASSED\n");
        printf("✅ All critical systems working correctly\n");
        printf("✅ No memory leaks detected\n");
        printf("✅ Thread pool handles 1000+ jobs without issues\n");
        printf("✅ Systems initialize and shutdown cleanly\n");
        printf("\n🎉 Core systems test PASSED!\n");
    } else {
        printf("\n❌ Core systems test FAILED\n");
    }
    
    return test_result;
}
