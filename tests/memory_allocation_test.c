#include "memory_allocation_test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>

MemoryAllocationTestConfig g_memory_allocation_test_config;
MemoryAllocationTestResults g_memory_allocation_test_results;
MemoryTracker g_memory_tracker;

static uint64_t get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
}

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

bool memory_tracker_init(void) {
    memset(&g_memory_tracker, 0, sizeof(g_memory_tracker));
    g_memory_tracker.is_tracking_enabled = true;
    g_memory_tracker.next_allocation_id = 1;
    return true;
}

void memory_tracker_shutdown(void) {
    // Clean up any remaining blocks
    MemoryBlock *current = g_memory_tracker.active_blocks;
    while (current) {
        MemoryBlock *next = current->next;
        free(current);
        current = next;
    }
    
    current = g_memory_tracker.freed_blocks;
    while (current) {
        MemoryBlock *next = current->next;
        free(current);
        current = next;
    }
    
    memset(&g_memory_tracker, 0, sizeof(g_memory_tracker));
}

bool memory_tracker_track_allocation(void *ptr, size_t size, const char *tag) {
    if (!g_memory_tracker.is_tracking_enabled || !ptr) {
        return false;
    }
    
    MemoryBlock *block = malloc(sizeof(MemoryBlock));
    if (!block) return false;
    
    block->ptr = ptr;
    block->size = size;
    block->actual_size = size + sizeof(MemoryBlock); // Include tracking overhead
    block->allocation_id = g_memory_tracker.next_allocation_id++;
    block->allocation_timestamp = get_time_ns();
    block->thread_id = (uint32_t)pthread_self();
    block->is_freed = false;
    block->free_timestamp = 0;
    block->next = g_memory_tracker.active_blocks;
    
    if (tag) {
        strncpy(block->tag, tag, sizeof(block->tag) - 1);
    } else {
        strcpy(block->tag, "unknown");
    }
    
    g_memory_tracker.active_blocks = block;
    g_memory_tracker.total_blocks++;
    g_memory_tracker.active_count++;
    g_memory_tracker.total_active_bytes += size;
    
    if (g_memory_tracker.total_active_bytes > g_memory_tracker.peak_active_bytes) {
        g_memory_tracker.peak_active_bytes = g_memory_tracker.total_active_bytes;
    }
    
    return true;
}

bool memory_tracker_track_deallocation(void *ptr) {
    if (!g_memory_tracker.is_tracking_enabled || !ptr) {
        return false;
    }
    
    MemoryBlock **current = &g_memory_tracker.active_blocks;
    while (*current) {
        if ((*current)->ptr == ptr && !(*current)->is_freed) {
            MemoryBlock *block = *current;
            *current = block->next;
            
            block->is_freed = true;
            block->free_timestamp = get_time_ns();
            
            // Move to freed list
            block->next = g_memory_tracker.freed_blocks;
            g_memory_tracker.freed_blocks = block;
            
            g_memory_tracker.active_count--;
            g_memory_tracker.freed_count++;
            g_memory_tracker.total_active_bytes -= block->size;
            g_memory_tracker.total_freed_bytes += block->size;
            
            return true;
        }
        current = &(*current)->next;
    }
    
    return false; // Block not found
}

bool memory_tracker_check_leaks(uint32_t *leak_count) {
    uint32_t leaks = 0;
    MemoryBlock *current = g_memory_tracker.active_blocks;
    
    while (current) {
        if (!current->is_freed) {
            leaks++;
        }
        current = current->next;
    }
    
    if (leak_count) {
        *leak_count = leaks;
    }
    
    return leaks > 0;
}

TestObject* create_simple_object(int value) {
    TestObject *obj = malloc(sizeof(TestObject));
    if (!obj) return NULL;
    
    obj->type = TEST_OBJECT_SIMPLE;
    obj->id = (uint32_t)(rand() % 1000000);
    obj->size = sizeof(TestObject);
    obj->creation_time = get_time_ns();
    obj->ref_count = 1;
    obj->is_valid = true;
    obj->data = malloc(sizeof(int));
    obj->next = NULL;
    
    if (!obj->data) {
        free(obj);
        return NULL;
    }
    
    *(int*)obj->data = value;
    
    memory_tracker_track_allocation(obj, obj->size + sizeof(int), "simple_object");
    return obj;
}

TestObject* create_complex_object(size_t data_size) {
    TestObject *obj = malloc(sizeof(TestObject));
    if (!obj) return NULL;
    
    obj->type = TEST_OBJECT_COMPLEX;
    obj->id = (uint32_t)(rand() % 1000000);
    obj->size = sizeof(TestObject) + data_size;
    obj->creation_time = get_time_ns();
    obj->ref_count = 1;
    obj->is_valid = true;
    obj->data = malloc(data_size);
    obj->next = NULL;
    
    if (!obj->data) {
        free(obj);
        return NULL;
    }
    
    // Fill with random data
    uint8_t *data = (uint8_t*)obj->data;
    for (size_t i = 0; i < data_size; i++) {
        data[i] = (uint8_t)(rand() % 256);
    }
    
    memory_tracker_track_allocation(obj, obj->size, "complex_object");
    return obj;
}

TestObject* create_array_object(uint32_t array_size) {
    TestObject *obj = malloc(sizeof(TestObject));
    if (!obj) return NULL;
    
    size_t data_size = array_size * sizeof(int);
    obj->type = TEST_OBJECT_ARRAY;
    obj->id = (uint32_t)(rand() % 1000000);
    obj->size = sizeof(TestObject) + data_size;
    obj->creation_time = get_time_ns();
    obj->ref_count = 1;
    obj->is_valid = true;
    obj->data = malloc(data_size);
    obj->next = NULL;
    
    if (!obj->data) {
        free(obj);
        return NULL;
    }
    
    // Fill array with sequential values
    int *array = (int*)obj->data;
    for (uint32_t i = 0; i < array_size; i++) {
        array[i] = (int)i;
    }
    
    memory_tracker_track_allocation(obj, obj->size, "array_object");
    return obj;
}

TestObject* create_string_object(const char *text) {
    TestObject *obj = malloc(sizeof(TestObject));
    if (!obj) return NULL;
    
    size_t text_len = strlen(text) + 1;
    obj->type = TEST_OBJECT_STRING;
    obj->id = (uint32_t)(rand() % 1000000);
    obj->size = sizeof(TestObject) + text_len;
    obj->creation_time = get_time_ns();
    obj->ref_count = 1;
    obj->is_valid = true;
    obj->data = malloc(text_len);
    obj->next = NULL;
    
    if (!obj->data) {
        free(obj);
        return NULL;
    }
    
    strcpy((char*)obj->data, text);
    
    memory_tracker_track_allocation(obj, obj->size, "string_object");
    return obj;
}

TestObject* create_linked_list_node(int data, TestObject *next) {
    TestObject *obj = malloc(sizeof(TestObject));
    if (!obj) return NULL;
    
    obj->type = TEST_OBJECT_LINKED_LIST;
    obj->id = (uint32_t)(rand() % 1000000);
    obj->size = sizeof(TestObject) + sizeof(int);
    obj->creation_time = get_time_ns();
    obj->ref_count = 1;
    obj->is_valid = true;
    obj->data = malloc(sizeof(int));
    obj->next = next;
    
    if (!obj->data) {
        free(obj);
        return NULL;
    }
    
    *(int*)obj->data = data;
    
    memory_tracker_track_allocation(obj, obj->size, "linked_list_node");
    return obj;
}

TestObject* create_buffer_object(size_t buffer_size) {
    TestObject *obj = malloc(sizeof(TestObject));
    if (!obj) return NULL;
    
    obj->type = TEST_OBJECT_BUFFER;
    obj->id = (uint32_t)(rand() % 1000000);
    obj->size = sizeof(TestObject) + buffer_size;
    obj->creation_time = get_time_ns();
    obj->ref_count = 1;
    obj->is_valid = true;
    obj->data = malloc(buffer_size);
    obj->next = NULL;
    
    if (!obj->data) {
        free(obj);
        return NULL;
    }
    
    // Initialize buffer with pattern
    uint8_t *buffer = (uint8_t*)obj->data;
    for (size_t i = 0; i < buffer_size; i++) {
        buffer[i] = (uint8_t)(i % 256);
    }
    
    memory_tracker_track_allocation(obj, obj->size, "buffer_object");
    return obj;
}

void destroy_test_object(TestObject *object) {
    if (!object || !object->is_valid) return;
    
    memory_tracker_track_deallocation(object);
    
    if (object->data) {
        free(object->data);
    }
    
    object->is_valid = false;
    free(object);
}

void destroy_linked_list(TestObject *head) {
    TestObject *current = head;
    while (current) {
        TestObject *next = current->next;
        destroy_test_object(current);
        current = next;
    }
}

size_t generate_random_size(size_t min_size, size_t max_size) {
    if (max_size <= min_size) {
        return min_size;
    }
    
    size_t range = max_size - min_size;
    return min_size + (size_t)(rand() % (range + 1));
}

bool test_basic_allocation_deallocation(void) {
    const int test_count = 100;
    TestObject *objects[test_count];
    
    // Allocate objects
    for (int i = 0; i < test_count; i++) {
        objects[i] = create_simple_object(i);
        if (!objects[i]) {
            // Cleanup allocated objects
            for (int j = 0; j < i; j++) {
                destroy_test_object(objects[j]);
            }
            return false;
        }
    }
    
    // Verify all objects are valid
    for (int i = 0; i < test_count; i++) {
        if (!validate_object_integrity(objects[i])) {
            // Cleanup
            for (int j = 0; j < test_count; j++) {
                if (objects[j]) {
                    destroy_test_object(objects[j]);
                }
            }
            return false;
        }
    }
    
    // Deallocate all objects
    for (int i = 0; i < test_count; i++) {
        destroy_test_object(objects[i]);
    }
    
    return true;
}

bool test_multiple_allocation_cycles(void) {
    const int cycles = 10;
    const int objects_per_cycle = 50;
    
    for (int cycle = 0; cycle < cycles; cycle++) {
        TestObject **objects = malloc(objects_per_cycle * sizeof(TestObject*));
        if (!objects) return false;
        
        // Allocate objects
        for (int i = 0; i < objects_per_cycle; i++) {
            objects[i] = create_complex_object(64 + (i % 10) * 32);
            if (!objects[i]) {
                // Cleanup
                for (int j = 0; j < i; j++) {
                    destroy_test_object(objects[j]);
                }
                free(objects);
                return false;
            }
        }
        
        // Deallocate half the objects
        for (int i = 0; i < objects_per_cycle / 2; i++) {
            destroy_test_object(objects[i]);
        }
        
        // Deallocate remaining objects
        for (int i = objects_per_cycle / 2; i < objects_per_cycle; i++) {
            destroy_test_object(objects[i]);
        }
        
        free(objects);
    }
    
    return true;
}

bool test_variable_size_allocations(void) {
    const int test_count = 100;
    TestObject *objects[test_count];
    
    // Allocate objects with varying sizes
    for (int i = 0; i < test_count; i++) {
        size_t size = generate_random_size(16, 4096);
        objects[i] = create_complex_object(size);
        if (!objects[i]) {
            // Cleanup
            for (int j = 0; j < i; j++) {
                destroy_test_object(objects[j]);
            }
            return false;
        }
    }
    
    // Verify sizes
    for (int i = 0; i < test_count; i++) {
        if (!validate_object_integrity(objects[i])) {
            // Cleanup
            for (int j = 0; j < test_count; j++) {
                if (objects[j]) {
                    destroy_test_object(objects[j]);
                }
            }
            return false;
        }
    }
    
    // Deallocate all objects
    for (int i = 0; i < test_count; i++) {
        destroy_test_object(objects[i]);
    }
    
    return true;
}

bool stress_test_1000_allocations(void) {
    const int allocation_count = 1000;
    TestObject **objects = malloc(allocation_count * sizeof(TestObject*));
    if (!objects) return false;
    
    uint64_t allocation_start = get_time_ms();
    
    // Allocate 1000 objects
    for (int i = 0; i < allocation_count; i++) {
        TestObjectType type = (TestObjectType)(i % TEST_OBJECT_COUNT);
        TestObject *obj = NULL;
        
        switch (type) {
            case TEST_OBJECT_SIMPLE:
                obj = create_simple_object(i);
                break;
            case TEST_OBJECT_COMPLEX:
                obj = create_complex_object(128 + (i % 10) * 64);
                break;
            case TEST_OBJECT_ARRAY:
                obj = create_array_object(10 + (i % 20));
                break;
            case TEST_OBJECT_STRING:
                {
                    char text[32];
                    snprintf(text, sizeof(text), "test_string_%d", i);
                    obj = create_string_object(text);
                }
                break;
            case TEST_OBJECT_BUFFER:
                obj = create_buffer_object(256 + (i % 5) * 128);
                break;
            default:
                obj = create_simple_object(i);
                break;
        }
        
        objects[i] = obj;
        if (!obj) {
            // Cleanup allocated objects
            for (int j = 0; j < i; j++) {
                if (objects[j]) {
                    destroy_test_object(objects[j]);
                }
            }
            free(objects);
            return false;
        }
    }
    
    uint64_t allocation_end = get_time_ms();
    g_memory_allocation_test_results.allocation_time_ms = 
        (double)(allocation_end - allocation_start);
    
    // Verify all objects
    for (int i = 0; i < allocation_count; i++) {
        if (!validate_object_integrity(objects[i])) {
            // Cleanup
            for (int j = 0; j < allocation_count; j++) {
                if (objects[j]) {
                    destroy_test_object(objects[j]);
                }
            }
            free(objects);
            return false;
        }
    }
    
    uint64_t deallocation_start = get_time_ms();
    
    // Deallocate all objects
    for (int i = 0; i < allocation_count; i++) {
        destroy_test_object(objects[i]);
    }
    
    uint64_t deallocation_end = get_time_ms();
    g_memory_allocation_test_results.deallocation_time_ms = 
        (double)(deallocation_end - deallocation_start);
    
    free(objects);
    
    // Update statistics
    g_memory_allocation_test_results.total_allocations = allocation_count;
    g_memory_allocation_test_results.total_deallocations = allocation_count;
    g_memory_allocation_test_results.successful_allocations = allocation_count;
    g_memory_allocation_test_results.average_allocation_time_ms = 
        g_memory_allocation_test_results.allocation_time_ms / allocation_count;
    g_memory_allocation_test_results.average_deallocation_time_ms = 
        g_memory_allocation_test_results.deallocation_time_ms / allocation_count;
    
    return true;
}

bool validate_object_integrity(const TestObject *object) {
    if (!object) return false;
    
    // Check basic fields
    if (!object->is_valid || object->ref_count == 0) {
        return false;
    }
    
    // Check type-specific integrity
    switch (object->type) {
        case TEST_OBJECT_SIMPLE:
            return (object->data != NULL && object->size >= sizeof(TestObject));
        case TEST_OBJECT_COMPLEX:
            return (object->data != NULL && object->size > sizeof(TestObject));
        case TEST_OBJECT_ARRAY:
            return (object->data != NULL && object->size > sizeof(TestObject));
        case TEST_OBJECT_STRING:
            return (object->data != NULL && object->size > sizeof(TestObject));
        case TEST_OBJECT_BUFFER:
            return (object->data != NULL && object->size > sizeof(TestObject));
        default:
            return false;
    }
}

bool validate_no_memory_leaks(void) {
    uint32_t leak_count;
    bool has_leaks = memory_tracker_check_leaks(&leak_count);
    
    g_memory_allocation_test_results.memory_leaks_detected = leak_count;
    
    return !has_leaks;
}

bool memory_allocation_test_init(const MemoryAllocationTestConfig *config) {
    if (config) {
        g_memory_allocation_test_config = *config;
    } else {
        g_memory_allocation_test_config = (MemoryAllocationTestConfig){
            .enable_verbose_output = false,
            .enable_detailed_tracking = true,
            .enable_stress_testing = true,
            .allocation_count = 1000,
            .allocation_iterations = 1,
            .min_allocation_size = 16,
            .max_allocation_size = 4096,
            .enable_random_sizes = true,
            .enable_alignment_testing = false,
            .alignment_requirement = 16,
            .enable_concurrent_testing = false,
            .thread_count = 1,
            .output_file = NULL
        };
    }
    
    memset(&g_memory_allocation_test_results, 0, sizeof(g_memory_allocation_test_results));
    
    // Initialize memory tracker
    if (!memory_tracker_init()) {
        return false;
    }
    
    srand((unsigned int)time(NULL));
    return true;
}

void memory_allocation_test_shutdown(bool generate_report) {
    if (generate_report) {
        memory_allocation_test_print_summary();
        if (g_memory_allocation_test_config.output_file) {
            memory_allocation_test_export_results(g_memory_allocation_test_config.output_file);
        }
    }
    
    memory_tracker_shutdown();
}

bool memory_allocation_test_run(void) {
    uint64_t test_start_time = get_time_ms();
    
    // Get initial memory statistics
    MemoryTracker initial_stats = memory_tracker_get_statistics();
    
    bool test_passed = true;
    
    // Run basic tests
    if (!test_basic_allocation_deallocation()) {
        strcat(g_memory_allocation_test_results.error_messages, 
               "Basic allocation/deallocation test failed\n");
        test_passed = false;
    }
    
    if (!test_multiple_allocation_cycles()) {
        strcat(g_memory_allocation_test_results.error_messages, 
               "Multiple allocation cycles test failed\n");
        test_passed = false;
    }
    
    if (!test_variable_size_allocations()) {
        strcat(g_memory_allocation_test_results.error_messages, 
               "Variable size allocations test failed\n");
        test_passed = false;
    }
    
    // Run main stress test
    if (!stress_test_1000_allocations()) {
        strcat(g_memory_allocation_test_results.error_messages, 
               "1000 allocation stress test failed\n");
        test_passed = false;
    }
    
    // Check for memory leaks
    if (!validate_no_memory_leaks()) {
        char leak_msg[256];
        snprintf(leak_msg, sizeof(leak_msg), 
                "Memory leaks detected: %u leaks\n", 
                g_memory_allocation_test_results.memory_leaks_detected);
        strcat(g_memory_allocation_test_results.error_messages, leak_msg);
        test_passed = false;
    }
    
    // Get final memory statistics
    MemoryTracker final_stats = memory_tracker_get_statistics();
    
    // Update results
    g_memory_allocation_test_results.total_time_ms = get_time_ms() - test_start_time;
    g_memory_allocation_test_results.total_bytes_allocated = final_stats.total_active_bytes + final_stats.total_freed_bytes;
    g_memory_allocation_test_results.total_bytes_freed = final_stats.total_freed_bytes;
    g_memory_allocation_test_results.current_bytes_allocated = final_stats.total_active_bytes;
    g_memory_allocation_test_results.peak_bytes_allocated = final_stats.peak_active_bytes;
    g_memory_allocation_test_results.test_passed = test_passed;
    
    return test_passed;
}

void memory_allocation_test_print_summary(void) {
    printf("\n=== Memory Allocation Test Summary ===\n");
    printf("Configuration:\n");
    printf("  Allocation Count: %u\n", g_memory_allocation_test_config.allocation_count);
    printf("  Min Size: %zu bytes\n", g_memory_allocation_test_config.min_allocation_size);
    printf("  Max Size: %zu bytes\n", g_memory_allocation_test_config.max_allocation_size);
    printf("  Random Sizes: %s\n", g_memory_allocation_test_config.enable_random_sizes ? "Yes" : "No");
    
    printf("\nResults:\n");
    printf("  Total Allocations: %u\n", g_memory_allocation_test_results.total_allocations);
    printf("  Total Deallocations: %u\n", g_memory_allocation_test_results.total_deallocations);
    printf("  Successful Allocations: %u\n", g_memory_allocation_test_results.successful_allocations);
    printf("  Failed Allocations: %u\n", g_memory_allocation_test_results.failed_allocations);
    printf("  Memory Leaks Detected: %u\n", g_memory_allocation_test_results.memory_leaks_detected);
    printf("  Test Passed: %s\n", g_memory_allocation_test_results.test_passed ? "Yes" : "No");
    
    printf("\nMemory Usage:\n");
    printf("  Total Bytes Allocated: %zu\n", g_memory_allocation_test_results.total_bytes_allocated);
    printf("  Total Bytes Freed: %zu\n", g_memory_allocation_test_results.total_bytes_freed);
    printf("  Current Bytes Allocated: %zu\n", g_memory_allocation_test_results.current_bytes_allocated);
    printf("  Peak Bytes Allocated: %zu\n", g_memory_allocation_test_results.peak_bytes_allocated);
    
    printf("\nPerformance:\n");
    printf("  Total Time: %.2f ms\n", g_memory_allocation_test_results.total_time_ms);
    printf("  Allocation Time: %.2f ms\n", g_memory_allocation_test_results.allocation_time_ms);
    printf("  Deallocation Time: %.2f ms\n", g_memory_allocation_test_results.deallocation_time_ms);
    printf("  Average Allocation Time: %.6f ms\n", g_memory_allocation_test_results.average_allocation_time_ms);
    printf("  Average Deallocation Time: %.6f ms\n", g_memory_allocation_test_results.average_deallocation_time_ms);
    
    if (strlen(g_memory_allocation_test_results.error_messages) > 0) {
        printf("\nErrors:\n%s\n", g_memory_allocation_test_results.error_messages);
    }
    
    printf("===================================\n");
}

MemoryAllocationTestResults memory_allocation_test_get_results(void) {
    return g_memory_allocation_test_results;
}

MemoryTracker memory_tracker_get_statistics(void) {
    return g_memory_tracker;
}

bool memory_allocation_test_export_results(const char *filename) {
    if (!filename) return false;
    
    FILE *file = fopen(filename, "w");
    if (!file) return false;
    
    fprintf(file, "Memory Allocation Test Results\n");
    fprintf(file, "===============================\n\n");
    
    fprintf(file, "Configuration:\n");
    fprintf(file, "  Allocation Count: %u\n", g_memory_allocation_test_config.allocation_count);
    fprintf(file, "  Min Size: %zu bytes\n", g_memory_allocation_test_config.min_allocation_size);
    fprintf(file, "  Max Size: %zu bytes\n", g_memory_allocation_test_config.max_allocation_size);
    fprintf(file, "  Random Sizes: %s\n", g_memory_allocation_test_config.enable_random_sizes ? "Yes" : "No");
    fprintf(file, "  Detailed Tracking: %s\n", g_memory_allocation_test_config.enable_detailed_tracking ? "Yes" : "No");
    
    fprintf(file, "\nResults:\n");
    fprintf(file, "  Total Allocations: %u\n", g_memory_allocation_test_results.total_allocations);
    fprintf(file, "  Total Deallocations: %u\n", g_memory_allocation_test_results.total_deallocations);
    fprintf(file, "  Successful Allocations: %u\n", g_memory_allocation_test_results.successful_allocations);
    fprintf(file, "  Failed Allocations: %u\n", g_memory_allocation_test_results.failed_allocations);
    fprintf(file, "  Memory Leaks Detected: %u\n", g_memory_allocation_test_results.memory_leaks_detected);
    fprintf(file, "  Test Passed: %s\n", g_memory_allocation_test_results.test_passed ? "Yes" : "No");
    
    fprintf(file, "\nMemory Usage:\n");
    fprintf(file, "  Total Bytes Allocated: %zu\n", g_memory_allocation_test_results.total_bytes_allocated);
    fprintf(file, "  Total Bytes Freed: %zu\n", g_memory_allocation_test_results.total_bytes_freed);
    fprintf(file, "  Current Bytes Allocated: %zu\n", g_memory_allocation_test_results.current_bytes_allocated);
    fprintf(file, "  Peak Bytes Allocated: %zu\n", g_memory_allocation_test_results.peak_bytes_allocated);
    
    fprintf(file, "\nPerformance:\n");
    fprintf(file, "  Total Time: %.2f ms\n", g_memory_allocation_test_results.total_time_ms);
    fprintf(file, "  Allocation Time: %.2f ms\n", g_memory_allocation_test_results.allocation_time_ms);
    fprintf(file, "  Deallocation Time: %.2f ms\n", g_memory_allocation_test_results.deallocation_time_ms);
    fprintf(file, "  Average Allocation Time: %.6f ms\n", g_memory_allocation_test_results.average_allocation_time_ms);
    fprintf(file, "  Average Deallocation Time: %.6f ms\n", g_memory_allocation_test_results.average_deallocation_time_ms);
    
    if (strlen(g_memory_allocation_test_results.error_messages) > 0) {
        fprintf(file, "\nErrors:\n%s\n", g_memory_allocation_test_results.error_messages);
    }
    
    fprintf(file, "\nPerformance Report:\n%s\n", g_memory_allocation_test_results.performance_report);
    
    fclose(file);
    return true;
}
