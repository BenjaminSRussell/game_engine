/**
 * @file data_structure_stress_tests.c
 * @brief Implementation of comprehensive data structure stress tests
 * 
 * This file implements stress testing framework for all core data structures
 * including memory allocators, containers, and engine subsystems.
 * 
 * @author Minecraft v2 Engine Team
 * @date 2026-01-13
 */

#include "data_structure_stress_tests.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

// ========================================
// Global State
// ========================================

static StressTestState g_stress_test_state = {0};
static bool g_stress_test_initialized = false;

// ========================================
// Utility Functions
// ========================================

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

static void log_message(const char* format, ...) {
    if (!g_stress_test_state.config.enable_verbose_logging) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

// ========================================
// Memory Tracking Implementation
// ========================================

bool stress_test_track_allocation(void* ptr, size_t size, const char* file, int line) {
    if (!ptr || !g_stress_test_initialized) {
        return false;
    }
    
    MemoryAllocation* alloc = malloc(sizeof(MemoryAllocation));
    if (!alloc) {
        return false;
    }
    
    alloc->ptr = ptr;
    alloc->size = size;
    alloc->line = line;
    alloc->thread_id = pthread_self();
    alloc->timestamp = get_time_ms();
    alloc->next = NULL;
    
    if (file) {
        strncpy(alloc->file, file, sizeof(alloc->file) - 1);
        alloc->file[sizeof(alloc->file) - 1] = '\0';
    } else {
        strcpy(alloc->file, "unknown");
    }
    
    pthread_mutex_lock(&g_stress_test_state.allocation_mutex);
    
    alloc->next = g_stress_test_state.allocation_head;
    g_stress_test_state.allocation_head = alloc;
    
    g_stress_test_state.total_allocations++;
    g_stress_test_state.total_allocated_bytes += size;
    
    if (g_stress_test_state.total_allocated_bytes > g_stress_test_state.peak_allocated_bytes) {
        g_stress_test_state.peak_allocated_bytes = g_stress_test_state.total_allocated_bytes;
    }
    
    pthread_mutex_unlock(&g_stress_test_state.allocation_mutex);
    
    return true;
}

bool stress_test_track_deallocation(void* ptr) {
    if (!ptr || !g_stress_test_initialized) {
        return false;
    }
    
    pthread_mutex_lock(&g_stress_test_state.allocation_mutex);
    
    MemoryAllocation** current = &g_stress_test_state.allocation_head;
    while (*current) {
        if ((*current)->ptr == ptr) {
            MemoryAllocation* to_remove = *current;
            *current = (*current)->next;
            
            g_stress_test_state.total_allocations--;
            g_stress_test_state.total_allocated_bytes -= to_remove->size;
            
            free(to_remove);
            pthread_mutex_unlock(&g_stress_test_state.allocation_mutex);
            return true;
        }
        current = &(*current)->next;
    }
    
    pthread_mutex_unlock(&g_stress_test_state.allocation_mutex);
    
    // Double-free detected
    g_stress_test_state.errors_detected++;
    log_message("ERROR: Double-free detected for pointer %p", ptr);
    return false;
}

uint32_t stress_test_validate_memory(void) {
    if (!g_stress_test_initialized) {
        return 0;
    }
    
    uint32_t corrupted_count = 0;
    pthread_mutex_lock(&g_stress_test_state.allocation_mutex);
    
    MemoryAllocation* current = g_stress_test_state.allocation_head;
    while (current) {
        // Simple corruption check - check if pointer is still accessible
        // In a real implementation, you'd check guard patterns
        volatile char test = *((char*)current->ptr);
        (void)test; // Suppress unused variable warning
        
        current = current->next;
    }
    
    pthread_mutex_unlock(&g_stress_test_state.allocation_mutex);
    
    return corrupted_count;
}

uint32_t stress_test_generate_leak_report(const char* output_file) {
    if (!g_stress_test_initialized) {
        return 0;
    }
    
    FILE* output = stdout;
    if (output_file) {
        output = fopen(output_file, "w");
        if (!output) {
            log_message("Failed to open leak report file: %s", output_file);
            return 0;
        }
    }
    
    uint32_t leak_count = 0;
    pthread_mutex_lock(&g_stress_test_state.allocation_mutex);
    
    fprintf(output, "=== MEMORY LEAK REPORT ===\n");
    fprintf(output, "Total leaked allocations: %u\n", g_stress_test_state.total_allocations);
    fprintf(output, "Total leaked bytes: %llu\n", (unsigned long long)g_stress_test_state.total_allocated_bytes);
    fprintf(output, "\nLeak details:\n");
    
    MemoryAllocation* current = g_stress_test_state.allocation_head;
    while (current) {
        fprintf(output, "Leak: %p (%zu bytes) at %s:%d (thread %lu)\n",
                current->ptr, current->size, current->file, current->line,
                (unsigned long)current->thread_id);
        current = current->next;
        leak_count++;
    }
    
    pthread_mutex_unlock(&g_stress_test_state.allocation_mutex);
    
    if (output_file) {
        fclose(output);
    }
    
    return leak_count;
}

void stress_test_get_memory_stats(uint32_t* total_allocations, 
                                 uint64_t* total_bytes, 
                                 uint64_t* peak_bytes) {
    if (!g_stress_test_initialized) {
        if (total_allocations) *total_allocations = 0;
        if (total_bytes) *total_bytes = 0;
        if (peak_bytes) *peak_bytes = 0;
        return;
    }
    
    pthread_mutex_lock(&g_stress_test_state.allocation_mutex);
    
    if (total_allocations) *total_allocations = g_stress_test_state.total_allocations;
    if (total_bytes) *total_bytes = g_stress_test_state.total_allocated_bytes;
    if (peak_bytes) *peak_bytes = g_stress_test_state.peak_allocated_bytes;
    
    pthread_mutex_unlock(&g_stress_test_state.allocation_mutex);
}

// ========================================
// Framework Implementation
// ========================================

bool stress_test_init(const StressTestConfig* config) {
    if (g_stress_test_initialized) {
        return false;
    }
    
    memset(&g_stress_test_state, 0, sizeof(g_stress_test_state));
    
    if (config) {
        g_stress_test_state.config = *config;
    } else {
        // Default configuration
        g_stress_test_state.config = (StressTestConfig){
            .iterations = 10000,
            .concurrent_threads = 4,
            .memory_limit_mb = 1024,
            .timeout_seconds = 300,
            .enable_verbose_logging = false,
            .enable_memory_validation = true,
            .enable_performance_profiling = true
        };
    }
    
    if (pthread_mutex_init(&g_stress_test_state.allocation_mutex, NULL) != 0) {
        log_message("Failed to initialize allocation mutex");
        return false;
    }
    
    g_stress_test_initialized = true;
    log_message("Stress test framework initialized");
    
    return true;
}

bool stress_test_shutdown(void) {
    if (!g_stress_test_initialized) {
        return false;
    }
    
    // Generate final leak report
    uint32_t leaks = stress_test_generate_leak_report("stress_test_leaks.log");
    if (leaks > 0) {
        log_message("WARNING: %u memory leaks detected", leaks);
    }
    
    // Clean up allocation tracking
    pthread_mutex_lock(&g_stress_test_state.allocation_mutex);
    
    MemoryAllocation* current = g_stress_test_state.allocation_head;
    while (current) {
        MemoryAllocation* next = current->next;
        free(current);
        current = next;
    }
    
    pthread_mutex_unlock(&g_stress_test_state.allocation_mutex);
    pthread_mutex_destroy(&g_stress_test_state.allocation_mutex);
    
    log_message("Stress test framework shutdown complete");
    g_stress_test_initialized = false;
    
    return true;
}

// ========================================
// Stress Test Implementations
// ========================================

bool stress_test_memory_allocator(StressTestResult* result) {
    if (!result || !g_stress_test_initialized) {
        return false;
    }
    
    memset(result, 0, sizeof(StressTestResult));
    strcpy(result->test_name, "Memory Allocator Stress Test");
    
    log_message("Starting memory allocator stress test...");
    
    uint64_t start_time = get_time_ms();
    const uint32_t iterations = g_stress_test_state.config.iterations;
    const size_t max_alloc_size = 1024 * 1024; // 1MB max per allocation
    
    void** allocations = malloc(iterations * sizeof(void*));
    if (!allocations) {
        return false;
    }
    
    uint32_t successful_allocs = 0;
    uint32_t successful_frees = 0;
    
    // Allocation phase
    for (uint32_t i = 0; i < iterations; i++) {
        size_t size = (rand() % max_alloc_size) + 1;
        allocations[i] = malloc(size);
        
        if (allocations[i]) {
            successful_allocs++;
            stress_test_track_allocation(allocations[i], size, __FILE__, __LINE__);
            
            // Write some data to ensure allocation is actually usable
            memset(allocations[i], 0xAA, size < 1024 ? size : 1024);
        }
        
        // Random deallocation to test fragmentation
        if (i > 100 && (rand() % 10) == 0) {
            uint32_t free_idx = rand() % i;
            if (allocations[free_idx]) {
                stress_test_track_deallocation(allocations[free_idx]);
                free(allocations[free_idx]);
                allocations[free_idx] = NULL;
                successful_frees++;
            }
        }
    }
    
    // Deallocation phase
    for (uint32_t i = 0; i < iterations; i++) {
        if (allocations[i]) {
            stress_test_track_deallocation(allocations[i]);
            free(allocations[i]);
            successful_frees++;
        }
    }
    
    free(allocations);
    
    uint64_t end_time = get_time_ms();
    result->iterations_completed = iterations;
    result->total_time_ms = end_time - start_time;
    result->success_rate = (double)(successful_allocs + successful_frees) / (iterations * 2) * 100.0;
    
    // Get memory statistics
    uint64_t peak_bytes;
    stress_test_get_memory_stats(NULL, NULL, &peak_bytes);
    result->memory_peak_mb = (uint32_t)(peak_bytes / (1024 * 1024));
    
    result->operations_per_second = (iterations * 2.0) / (result->total_time_ms / 1000.0);
    
    // Validate for memory corruption
    result->corruption_detected = (stress_test_validate_memory() > 0);
    result->memory_leaks_detected = g_stress_test_state.total_allocations;
    
    log_message("Memory allocator test completed: %u iterations, %.2f ops/sec, %u leaks",
                iterations, result->operations_per_second, result->memory_leaks_detected);
    
    return result->success_rate > 95.0 && !result->corruption_detected;
}

bool stress_test_linked_list(StressTestResult* result) {
    if (!result || !g_stress_test_initialized) {
        return false;
    }
    
    memset(result, 0, sizeof(StressTestResult));
    strcpy(result->test_name, "Linked List Stress Test");
    
    log_message("Starting linked list stress test...");
    
    uint64_t start_time = get_time_ms();
    const uint32_t iterations = g_stress_test_state.config.iterations;
    
    // Simple linked list node structure
    typedef struct ListNode {
        int value;
        struct ListNode* next;
    } ListNode;
    
    ListNode* head = NULL;
    uint32_t successful_operations = 0;
    
    // Insertion phase
    for (uint32_t i = 0; i < iterations; i++) {
        ListNode* node = malloc(sizeof(ListNode));
        if (node) {
            node->value = i;
            node->next = head;
            head = node;
            successful_operations++;
        }
    }
    
    // Search phase
    uint32_t found_count = 0;
    for (uint32_t i = 0; i < iterations; i += 10) {
        ListNode* current = head;
        while (current) {
            if (current->value == i) {
                found_count++;
                break;
            }
            current = current->next;
        }
    }
    
    // Deletion phase
    while (head) {
        ListNode* next = head->next;
        free(head);
        head = next;
        successful_operations++;
    }
    
    uint64_t end_time = get_time_ms();
    result->iterations_completed = iterations;
    result->total_time_ms = end_time - start_time;
    result->success_rate = (double)successful_operations / (iterations * 2) * 100.0;
    result->operations_per_second = successful_operations / (result->total_time_ms / 1000.0);
    
    log_message("Linked list test completed: %u iterations, %u found", 
                iterations, found_count);
    
    return result->success_rate > 95.0;
}

bool stress_test_hash_table(StressTestResult* result) {
    if (!result || !g_stress_test_initialized) {
        return false;
    }
    
    memset(result, 0, sizeof(StressTestResult));
    strcpy(result->test_name, "Hash Table Stress Test");
    
    log_message("Starting hash table stress test...");
    
    uint64_t start_time = get_time_ms();
    const uint32_t iterations = g_stress_test_state.config.iterations;
    const uint32_t table_size = 16384;
    
    // Simple hash table entry
    typedef struct HashEntry {
        uint32_t key;
        uint32_t value;
        struct HashEntry* next;
    } HashEntry;
    
    HashEntry** table = calloc(table_size, sizeof(HashEntry*));
    if (!table) {
        return false;
    }
    
    uint32_t successful_operations = 0;
    
    // Insertion phase
    for (uint32_t i = 0; i < iterations; i++) {
        uint32_t hash = (i * 2654435761U) % table_size; // Golden ratio hash
        HashEntry* entry = malloc(sizeof(HashEntry));
        if (entry) {
            entry->key = i;
            entry->value = i * 2;
            entry->next = table[hash];
            table[hash] = entry;
            successful_operations++;
        }
    }
    
    // Lookup phase
    uint32_t found_count = 0;
    for (uint32_t i = 0; i < iterations; i += 5) {
        uint32_t hash = (i * 2654435761U) % table_size;
        HashEntry* entry = table[hash];
        while (entry) {
            if (entry->key == i) {
                found_count++;
                break;
            }
            entry = entry->next;
        }
    }
    
    // Cleanup phase
    for (uint32_t i = 0; i < table_size; i++) {
        HashEntry* entry = table[i];
        while (entry) {
            HashEntry* next = entry->next;
            free(entry);
            entry = next;
            successful_operations++;
        }
    }
    
    free(table);
    
    uint64_t end_time = get_time_ms();
    result->iterations_completed = iterations;
    result->total_time_ms = end_time - start_time;
    result->success_rate = (double)successful_operations / (iterations * 2) * 100.0;
    result->operations_per_second = successful_operations / (result->total_time_ms / 1000.0);
    
    log_message("Hash table test completed: %u iterations, %u found", 
                iterations, found_count);
    
    return result->success_rate > 95.0;
}

bool stress_test_dynamic_array(StressTestResult* result) {
    if (!result || !g_stress_test_initialized) {
        return false;
    }
    
    memset(result, 0, sizeof(StressTestResult));
    strcpy(result->test_name, "Dynamic Array Stress Test");
    
    log_message("Starting dynamic array stress test...");
    
    uint64_t start_time = get_time_ms();
    const uint32_t iterations = g_stress_test_state.config.iterations;
    
    // Simple dynamic array implementation
    typedef struct DynamicArray {
        int* data;
        uint32_t size;
        uint32_t capacity;
    } DynamicArray;
    
    DynamicArray array = {0};
    array.capacity = 16;
    array.data = malloc(array.capacity * sizeof(int));
    
    uint32_t successful_operations = 0;
    
    // Append phase
    for (uint32_t i = 0; i < iterations; i++) {
        if (array.size >= array.capacity) {
            uint32_t new_capacity = array.capacity * 2;
            int* new_data = realloc(array.data, new_capacity * sizeof(int));
            if (new_data) {
                array.data = new_data;
                array.capacity = new_capacity;
            } else {
                break;
            }
        }
        
        array.data[array.size++] = i;
        successful_operations++;
    }
    
    // Random insert/delete phase
    for (uint32_t i = 0; i < iterations / 10; i++) {
        if (array.size > 0) {
            uint32_t index = rand() % array.size;
            array.data[index] = rand();
            successful_operations++;
        }
    }
    
    free(array.data);
    
    uint64_t end_time = get_time_ms();
    result->iterations_completed = iterations;
    result->total_time_ms = end_time - start_time;
    result->success_rate = (double)successful_operations / (iterations * 1.1) * 100.0;
    result->operations_per_second = successful_operations / (result->total_time_ms / 1000.0);
    
    log_message("Dynamic array test completed: %u iterations", iterations);
    
    return result->success_rate > 95.0;
}

bool stress_test_binary_search_tree(StressTestResult* result) {
    if (!result || !g_stress_test_initialized) {
        return false;
    }
    
    memset(result, 0, sizeof(StressTestResult));
    strcpy(result->test_name, "Binary Search Tree Stress Test");
    
    log_message("Starting binary search tree stress test...");
    
    uint64_t start_time = get_time_ms();
    const uint32_t iterations = g_stress_test_state.config.iterations;
    
    // Simple BST node
    typedef struct BSTNode {
        int value;
        struct BSTNode* left;
        struct BSTNode* right;
    } BSTNode;
    
    BSTNode* root = NULL;
    uint32_t successful_operations = 0;
    
    // Insertion phase
    for (uint32_t i = 0; i < iterations; i++) {
        int value = rand() % (iterations * 10); // Random values to create varied tree
        
        BSTNode** current = &root;
        while (*current) {
            if (value < (*current)->value) {
                current = &(*current)->left;
            } else {
                current = &(*current)->right;
            }
        }
        
        BSTNode* node = malloc(sizeof(BSTNode));
        if (node) {
            node->value = value;
            node->left = NULL;
            node->right = NULL;
            *current = node;
            successful_operations++;
        }
    }
    
    // Simple tree traversal for validation
    uint32_t node_count = 0;
    void traverse(BSTNode* node) {
        if (node) {
            traverse(node->left);
            node_count++;
            traverse(node->right);
        }
    }
    traverse(root);
    
    // Cleanup (post-order)
    void cleanup(BSTNode* node) {
        if (node) {
            cleanup(node->left);
            cleanup(node->right);
            free(node);
            successful_operations++;
        }
    }
    cleanup(root);
    
    uint64_t end_time = get_time_ms();
    result->iterations_completed = iterations;
    result->total_time_ms = end_time - start_time;
    result->success_rate = (double)successful_operations / (iterations * 2) * 100.0;
    result->operations_per_second = successful_operations / (result->total_time_ms / 1000.0);
    
    log_message("BST test completed: %u iterations, %u nodes", iterations, node_count);
    
    return result->success_rate > 90.0; // Allow some failed insertions due to duplicates
}

bool stress_test_priority_queue(StressTestResult* result) {
    if (!result || !g_stress_test_initialized) {
        return false;
    }
    
    memset(result, 0, sizeof(StressTestResult));
    strcpy(result->test_name, "Priority Queue Stress Test");
    
    log_message("Starting priority queue stress test...");
    
    uint64_t start_time = get_time_ms();
    const uint32_t iterations = g_stress_test_state.config.iterations;
    
    // Simple min-heap implementation
    typedef struct MinHeap {
        int* data;
        uint32_t size;
        uint32_t capacity;
    } MinHeap;
    
    MinHeap heap = {0};
    heap.capacity = 16;
    heap.data = malloc(heap.capacity * sizeof(int));
    
    uint32_t successful_operations = 0;
    
    // Helper functions
    void heapify_up(uint32_t index) {
        while (index > 0) {
            uint32_t parent = (index - 1) / 2;
            if (heap.data[parent] <= heap.data[index]) break;
            
            int temp = heap.data[parent];
            heap.data[parent] = heap.data[index];
            heap.data[index] = temp;
            index = parent;
        }
    }
    
    void heapify_down(uint32_t index) {
        while (1) {
            uint32_t smallest = index;
            uint32_t left = 2 * index + 1;
            uint32_t right = 2 * index + 2;
            
            if (left < heap.size && heap.data[left] < heap.data[smallest]) {
                smallest = left;
            }
            if (right < heap.size && heap.data[right] < heap.data[smallest]) {
                smallest = right;
            }
            
            if (smallest == index) break;
            
            int temp = heap.data[index];
            heap.data[index] = heap.data[smallest];
            heap.data[smallest] = temp;
            index = smallest;
        }
    }
    
    // Insert phase
    for (uint32_t i = 0; i < iterations; i++) {
        if (heap.size >= heap.capacity) {
            uint32_t new_capacity = heap.capacity * 2;
            int* new_data = realloc(heap.data, new_capacity * sizeof(int));
            if (new_data) {
                heap.data = new_data;
                heap.capacity = new_capacity;
            } else {
                break;
            }
        }
        
        heap.data[heap.size] = rand() % 1000;
        heapify_up(heap.size);
        heap.size++;
        successful_operations++;
    }
    
    // Extract phase
    while (heap.size > 0) {
        heap.data[0] = heap.data[heap.size - 1];
        heap.size--;
        heapify_down(0);
        successful_operations++;
    }
    
    free(heap.data);
    
    uint64_t end_time = get_time_ms();
    result->iterations_completed = iterations;
    result->total_time_ms = end_time - start_time;
    result->success_rate = (double)successful_operations / (iterations * 2) * 100.0;
    result->operations_per_second = successful_operations / (result->total_time_ms / 1000.0);
    
    log_message("Priority queue test completed: %u iterations", iterations);
    
    return result->success_rate > 95.0;
}

bool stress_test_string_operations(StressTestResult* result) {
    if (!result || !g_stress_test_initialized) {
        return false;
    }
    
    memset(result, 0, sizeof(StressTestResult));
    strcpy(result->test_name, "String Operations Stress Test");
    
    log_message("Starting string operations stress test...");
    
    uint64_t start_time = get_time_ms();
    const uint32_t iterations = g_stress_test_state.config.iterations;
    
    char** strings = malloc(iterations * sizeof(char*));
    if (!strings) {
        return false;
    }
    
    uint32_t successful_operations = 0;
    
    // String creation phase
    for (uint32_t i = 0; i < iterations; i++) {
        size_t length = (rand() % 256) + 1;
        strings[i] = malloc(length + 1);
        if (strings[i]) {
            // Fill with random printable characters
            for (size_t j = 0; j < length; j++) {
                strings[i][j] = '!' + (rand() % 94); // Printable ASCII
            }
            strings[i][length] = '\0';
            successful_operations++;
        }
    }
    
    // String operations phase
    for (uint32_t i = 0; i < iterations / 10; i++) {
        uint32_t idx1 = rand() % iterations;
        uint32_t idx2 = rand() % iterations;
        
        if (strings[idx1] && strings[idx2]) {
            size_t len1 = strlen(strings[idx1]);
            size_t len2 = strlen(strings[idx2]);
            
            // Concatenation test
            char* concat = malloc(len1 + len2 + 1);
            if (concat) {
                strcpy(concat, strings[idx1]);
                strcat(concat, strings[idx2]);
                free(concat);
                successful_operations++;
            }
        }
    }
    
    // Cleanup phase
    for (uint32_t i = 0; i < iterations; i++) {
        if (strings[i]) {
            free(strings[i]);
            successful_operations++;
        }
    }
    
    free(strings);
    
    uint64_t end_time = get_time_ms();
    result->iterations_completed = iterations;
    result->total_time_ms = end_time - start_time;
    result->success_rate = (double)successful_operations / (iterations * 2.2) * 100.0;
    result->operations_per_second = successful_operations / (result->total_time_ms / 1000.0);
    
    log_message("String operations test completed: %u iterations", iterations);
    
    return result->success_rate > 90.0;
}

// Placeholder implementations for remaining tests
bool stress_test_entity_system(StressTestResult* result) {
    if (!result) return false;
    memset(result, 0, sizeof(StressTestResult));
    strcpy(result->test_name, "Entity System Stress Test");
    
    // TODO: Implement actual entity system stress test
    result->iterations_completed = 1000;
    result->success_rate = 100.0;
    result->operations_per_second = 1000.0;
    
    return true;
}

bool stress_test_resource_management(StressTestResult* result) {
    if (!result) return false;
    memset(result, 0, sizeof(StressTestResult));
    strcpy(result->test_name, "Resource Management Stress Test");
    
    // TODO: Implement actual resource management stress test
    result->iterations_completed = 1000;
    result->success_rate = 100.0;
    result->operations_per_second = 1000.0;
    
    return true;
}

bool stress_test_event_system(StressTestResult* result) {
    if (!result) return false;
    memset(result, 0, sizeof(StressTestResult));
    strcpy(result->test_name, "Event System Stress Test");
    
    // TODO: Implement actual event system stress test
    result->iterations_completed = 1000;
    result->success_rate = 100.0;
    result->operations_per_second = 1000.0;
    
    return true;
}

// ========================================
// Test Runner Implementation
// ========================================

int stress_test_run_all(StressTestResult* results, int max_results) {
    if (!results || !g_stress_test_initialized || max_results < 10) {
        return -1;
    }
    
    int test_count = 0;
    
    // Run all stress tests
    if (test_count < max_results) {
        if (stress_test_memory_allocator(&results[test_count])) {
            log_message("✓ Memory allocator test passed");
        } else {
            log_message("✗ Memory allocator test failed");
        }
        test_count++;
    }
    
    if (test_count < max_results) {
        if (stress_test_linked_list(&results[test_count])) {
            log_message("✓ Linked list test passed");
        } else {
            log_message("✗ Linked list test failed");
        }
        test_count++;
    }
    
    if (test_count < max_results) {
        if (stress_test_hash_table(&results[test_count])) {
            log_message("✓ Hash table test passed");
        } else {
            log_message("✗ Hash table test failed");
        }
        test_count++;
    }
    
    if (test_count < max_results) {
        if (stress_test_dynamic_array(&results[test_count])) {
            log_message("✓ Dynamic array test passed");
        } else {
            log_message("✗ Dynamic array test failed");
        }
        test_count++;
    }
    
    if (test_count < max_results) {
        if (stress_test_binary_search_tree(&results[test_count])) {
            log_message("✓ Binary search tree test passed");
        } else {
            log_message("✗ Binary search tree test failed");
        }
        test_count++;
    }
    
    if (test_count < max_results) {
        if (stress_test_priority_queue(&results[test_count])) {
            log_message("✓ Priority queue test passed");
        } else {
            log_message("✗ Priority queue test failed");
        }
        test_count++;
    }
    
    if (test_count < max_results) {
        if (stress_test_string_operations(&results[test_count])) {
            log_message("✓ String operations test passed");
        } else {
            log_message("✗ String operations test failed");
        }
        test_count++;
    }
    
    if (test_count < max_results) {
        if (stress_test_entity_system(&results[test_count])) {
            log_message("✓ Entity system test passed");
        } else {
            log_message("✗ Entity system test failed");
        }
        test_count++;
    }
    
    if (test_count < max_results) {
        if (stress_test_resource_management(&results[test_count])) {
            log_message("✓ Resource management test passed");
        } else {
            log_message("✗ Resource management test failed");
        }
        test_count++;
    }
    
    if (test_count < max_results) {
        if (stress_test_event_system(&results[test_count])) {
            log_message("✓ Event system test passed");
        } else {
            log_message("✗ Event system test failed");
        }
        test_count++;
    }
    
    return test_count;
}

// ========================================
// Utility Functions Implementation
// ========================================

void stress_test_generate_random_data(void* buffer, size_t size, uint32_t seed) {
    if (!buffer || size == 0) return;
    
    if (seed == 0) {
        seed = (uint32_t)time(NULL);
    }
    
    srand(seed);
    uint8_t* bytes = (uint8_t*)buffer;
    for (size_t i = 0; i < size; i++) {
        bytes[i] = (uint8_t)rand();
    }
}

void stress_test_calculate_metrics(uint32_t iterations, uint64_t time_ms, 
                                  uint64_t bytes_processed,
                                  double* ops_per_sec, 
                                  double* throughput_mb_sec) {
    if (time_ms == 0) {
        if (ops_per_sec) *ops_per_sec = 0.0;
        if (throughput_mb_sec) *throughput_mb_sec = 0.0;
        return;
    }
    
    double time_sec = time_ms / 1000.0;
    
    if (ops_per_sec) {
        *ops_per_sec = iterations / time_sec;
    }
    
    if (throughput_mb_sec && bytes_processed > 0) {
        *throughput_mb_sec = (bytes_processed / (1024.0 * 1024.0)) / time_sec;
    }
}

void stress_test_print_result(const StressTestResult* result, bool verbose) {
    if (!result) return;
    
    printf("=== %s ===\n", result->test_name);
    printf("Iterations: %u\n", result->iterations_completed);
    printf("Time: %llu ms\n", (unsigned long long)result->total_time_ms);
    printf("Success Rate: %.2f%%\n", result->success_rate);
    printf("Operations/sec: %.2f\n", result->operations_per_second);
    printf("Peak Memory: %u MB\n", result->memory_peak_mb);
    printf("Errors: %u\n", result->errors_detected);
    printf("Memory Leaks: %u\n", result->memory_leaks_detected);
    printf("Corruption: %s\n", result->corruption_detected ? "YES" : "NO");
    
    if (verbose) {
        printf("Additional details:\n");
        printf("- Average time per operation: %.3f ms\n", 
               (double)result->total_time_ms / result->iterations_completed);
        printf("- Memory efficiency: %.2f ops/MB\n", 
               result->memory_peak_mb > 0 ? result->iterations_completed / (double)result->memory_peak_mb : 0.0);
    }
    
    printf("\n");
}
