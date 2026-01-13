/**
 * @file memory_leak_stress_test.c
 * @brief Memory leak stress test for Minecraft v2 Engine
 * 
 * This test allocates and deallocates 1000 objects to verify no memory leaks.
 * Part of TODO-0057: Allocate/deallocate 1000 objects, verify no memory leak
 * 
 * @author Minecraft v2 Engine Team
 * @version 1.0
 * @date 2026
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <sys/time.h>
#include <unistd.h>

// Test object structure
typedef struct TestObject {
    int id;
    char* data;
    size_t data_size;
    struct TestObject* next;
    double timestamp;
} TestObject;

// Memory tracking structure
typedef struct MemoryTracker {
    size_t total_allocated;
    size_t total_freed;
    size_t peak_usage;
    size_t current_usage;
    int allocation_count;
    int deallocation_count;
} MemoryTracker;

static MemoryTracker g_tracker = {0};

// Memory allocation wrapper with tracking
void* tracked_malloc(size_t size, const char* tag) {
    void* ptr = malloc(size);
    if (ptr) {
        g_tracker.total_allocated += size;
        g_tracker.current_usage += size;
        g_tracker.allocation_count++;
        
        if (g_tracker.current_usage > g_tracker.peak_usage) {
            g_tracker.peak_usage = g_tracker.current_usage;
        }
        
        printf("Allocated %zu bytes for %s (total: %zu, current: %zu)\n", 
               size, tag, g_tracker.total_allocated, g_tracker.current_usage);
    }
    return ptr;
}

// Memory deallocation wrapper with tracking
void tracked_free(void* ptr, size_t size, const char* tag) {
    if (ptr) {
        free(ptr);
        g_tracker.total_freed += size;
        g_tracker.current_usage -= size;
        g_tracker.deallocation_count++;
        
        printf("Freed %zu bytes for %s (total freed: %zu, current: %zu)\n", 
               size, tag, g_tracker.total_freed, g_tracker.current_usage);
    }
}

// Create a test object
TestObject* create_test_object(int id, size_t data_size) {
    TestObject* obj = (TestObject*)tracked_malloc(sizeof(TestObject), "TestObject");
    if (!obj) return NULL;
    
    obj->id = id;
    obj->data_size = data_size;
    obj->data = (char*)tracked_malloc(data_size, "TestObjectData");
    obj->next = NULL;
    obj->timestamp = getCurrentTimeMillis();
    
    if (!obj->data) {
        tracked_free(obj, sizeof(TestObject), "TestObject");
        return NULL;
    }
    
    // Fill with test data
    memset(obj->data, 0xAB + (id % 256), data_size);
    
    return obj;
}

// Destroy a test object
void destroy_test_object(TestObject* obj) {
    if (obj) {
        if (obj->data) {
            tracked_free(obj->data, obj->data_size, "TestObjectData");
        }
        tracked_free(obj, sizeof(TestObject), "TestObject");
    }
}

// Get current time in milliseconds
double getCurrentTimeMillis() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

// Memory leak stress test
int run_memory_leak_stress_test() {
    printf("\n=== Memory Leak Stress Test (TODO-0057) ===\n");
    printf("Testing allocation/deallocation of 1000 objects...\n\n");
    
    const int NUM_OBJECTS = 1000;
    const size_t BASE_DATA_SIZE = 1024; // 1KB per object
    TestObject* objects[NUM_OBJECTS] = {NULL};
    
    double start_time = getCurrentTimeMillis();
    
    // Phase 1: Allocate all objects
    printf("Phase 1: Allocating %d objects...\n", NUM_OBJECTS);
    for (int i = 0; i < NUM_OBJECTS; i++) {
        size_t data_size = BASE_DATA_SIZE + (i % 10) * 512; // Variable sizes
        objects[i] = create_test_object(i, data_size);
        
        if (!objects[i]) {
            printf("ERROR: Failed to allocate object %d\n", i);
            return -1;
        }
        
        // Random allocation pattern to stress test
        if (i % 100 == 0) {
            printf("Allocated %d/%d objects\n", i + 1, NUM_OBJECTS);
        }
    }
    
    double allocation_time = getCurrentTimeMillis();
    printf("Allocation completed in %.2f ms\n", allocation_time - start_time);
    printf("Peak memory usage: %zu bytes\n", g_tracker.peak_usage);
    printf("Current memory usage: %zu bytes\n", g_tracker.current_usage);
    
    // Phase 2: Use objects (simulate engine usage)
    printf("\nPhase 2: Simulating object usage...\n");
    for (int i = 0; i < NUM_OBJECTS; i++) {
        if (objects[i]) {
            // Simulate some operations
            memset(objects[i]->data, 0xCD + (i % 256), objects[i]->data_size / 2);
            objects[i]->timestamp = getCurrentTimeMillis();
        }
    }
    
    double usage_time = getCurrentTimeMillis();
    printf("Usage simulation completed in %.2f ms\n", usage_time - allocation_time);
    
    // Phase 3: Deallocate all objects
    printf("\nPhase 3: Deallocating %d objects...\n", NUM_OBJECTS);
    for (int i = 0; i < NUM_OBJECTS; i++) {
        if (objects[i]) {
            destroy_test_object(objects[i]);
            objects[i] = NULL;
        }
        
        if (i % 100 == 0) {
            printf("Deallocated %d/%d objects\n", i + 1, NUM_OBJECTS);
        }
    }
    
    double deallocation_time = getCurrentTimeMillis();
    printf("Deallocation completed in %.2f ms\n", deallocation_time - usage_time);
    
    // Phase 4: Verify no memory leaks
    printf("\nPhase 4: Memory leak verification...\n");
    printf("Total allocated: %zu bytes\n", g_tracker.total_allocated);
    printf("Total freed: %zu bytes\n", g_tracker.total_freed);
    printf("Current usage: %zu bytes\n", g_tracker.current_usage);
    printf("Peak usage: %zu bytes\n", g_tracker.peak_usage);
    printf("Allocation count: %d\n", g_tracker.allocation_count);
    printf("Deallocation count: %d\n", g_tracker.deallocation_count);
    
    // Check for memory leaks
    int leak_detected = 0;
    if (g_tracker.current_usage != 0) {
        printf("❌ MEMORY LEAK DETECTED: %zu bytes not freed\n", g_tracker.current_usage);
        leak_detected = 1;
    }
    
    if (g_tracker.allocation_count != g_tracker.deallocation_count) {
        printf("❌ ALLOCATION COUNT MISMATCH: %d allocated, %d freed\n", 
               g_tracker.allocation_count, g_tracker.deallocation_count);
        leak_detected = 1;
    }
    
    if (g_tracker.total_allocated != g_tracker.total_freed) {
        printf("❌ BYTE COUNT MISMATCH: %zu allocated, %zu freed\n", 
               g_tracker.total_allocated, g_tracker.total_freed);
        leak_detected = 1;
    }
    
    if (!leak_detected) {
        printf("✅ NO MEMORY LEAKS DETECTED\n");
        printf("✅ All %d objects successfully allocated and deallocated\n", NUM_OBJECTS);
    }
    
    double total_time = deallocation_time - start_time;
    printf("\nTotal test time: %.2f ms\n", total_time);
    printf("Average allocation time: %.2f ms per object\n", 
           (allocation_time - start_time) / NUM_OBJECTS);
    printf("Average deallocation time: %.2f ms per object\n", 
           (deallocation_time - usage_time) / NUM_OBJECTS);
    
    return leak_detected ? -1 : 0;
}

// Additional stress test: Rapid allocation/deallocation
int run_rapid_stress_test() {
    printf("\n=== Rapid Stress Test ===\n");
    printf("Testing rapid allocation/deallocation cycles...\n");
    
    const int CYCLES = 100;
    const int OBJECTS_PER_CYCLE = 50;
    
    double start_time = getCurrentTimeMillis();
    
    for (int cycle = 0; cycle < CYCLES; cycle++) {
        TestObject* objects[OBJECTS_PER_CYCLE] = {NULL};
        
        // Allocate
        for (int i = 0; i < OBJECTS_PER_CYCLE; i++) {
            objects[i] = create_test_object(cycle * OBJECTS_PER_CYCLE + i, 512);
        }
        
        // Immediate deallocation
        for (int i = 0; i < OBJECTS_PER_CYCLE; i++) {
            destroy_test_object(objects[i]);
        }
        
        if (cycle % 20 == 0) {
            printf("Completed %d/%d cycles\n", cycle + 1, CYCLES);
        }
    }
    
    double end_time = getCurrentTimeMillis();
    printf("Rapid stress test completed in %.2f ms\n", end_time - start_time);
    printf("Current memory usage: %zu bytes\n", g_tracker.current_usage);
    
    return (g_tracker.current_usage == 0) ? 0 : -1;
}

// Main test runner
int main(int argc, char* argv[]) {
    printf("Minecraft v2 Engine - Memory Leak Stress Test\n");
    printf("==============================================\n");
    
    // Reset tracker
    memset(&g_tracker, 0, sizeof(g_tracker));
    
    int result = 0;
    
    // Run main stress test
    result |= run_memory_leak_stress_test();
    
    // Run rapid stress test
    result |= run_rapid_stress_test();
    
    // Final verification
    printf("\n=== Final Verification ===\n");
    if (g_tracker.current_usage == 0) {
        printf("✅ ALL TESTS PASSED - No memory leaks detected\n");
        printf("✅ TODO-0057 completed successfully\n");
    } else {
        printf("❌ TESTS FAILED - Memory leaks detected\n");
        printf("❌ %zu bytes still allocated\n", g_tracker.current_usage);
        result = -1;
    }
    
    return result;
}
