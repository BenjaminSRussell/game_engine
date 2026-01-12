/**
 * @file memory_allocation_test.h
 * @brief Memory allocation and deallocation stress test
 *
 * Comprehensive memory allocation test to verify no memory leaks
 * when allocating and deallocating 1000+ objects.
 */

#ifndef MEMORY_ALLOCATION_TEST_H
#define MEMORY_ALLOCATION_TEST_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Memory Allocation Test Configuration
// ========================================

typedef struct MemoryAllocationTestConfig {
    bool enable_verbose_output;         // Enable verbose test output
    bool enable_detailed_tracking;      // Enable detailed memory tracking
    bool enable_stress_testing;        // Enable stress testing
    uint32_t allocation_count;         // Number of allocations to perform
    uint32_t allocation_iterations;    // Number of allocation/deallocation cycles
    size_t min_allocation_size;        // Minimum allocation size
    size_t max_allocation_size;        // Maximum allocation size
    bool enable_random_sizes;          // Use random allocation sizes
    bool enable_alignment_testing;      // Test memory alignment
    size_t alignment_requirement;       // Memory alignment requirement
    bool enable_concurrent_testing;    // Enable concurrent allocation testing
    uint32_t thread_count;             // Number of threads for concurrent testing
    const char *output_file;            // Output file for results
} MemoryAllocationTestConfig;

// ========================================
// Memory Allocation Test Results
// ========================================

typedef struct MemoryAllocationTestResults {
    uint32_t total_allocations;        // Total allocations performed
    uint32_t total_deallocations;      // Total deallocations performed
    uint32_t successful_allocations;   // Successful allocations
    uint32_t failed_allocations;       // Failed allocations
    uint32_t memory_leaks_detected;    // Number of memory leaks detected
    size_t total_bytes_allocated;       // Total bytes allocated
    size_t total_bytes_freed;          // Total bytes freed
    size_t current_bytes_allocated;     // Currently allocated bytes
    size_t peak_bytes_allocated;        // Peak allocated bytes
    double allocation_time_ms;          // Total allocation time
    double deallocation_time_ms;       // Total deallocation time
    double average_allocation_time_ms;   // Average allocation time
    double average_deallocation_time_ms; // Average deallocation time
    uint32_t allocation_failures;       // Number of allocation failures
    uint32_t deallocation_failures;     // Number of deallocation failures
    bool test_passed;                   // Whether test passed
    char error_messages[4096];          // Accumulated error messages
    char performance_report[2048];      // Performance report
} MemoryAllocationTestResults;

// ========================================
// Memory Block Structure
// ========================================

typedef struct MemoryBlock {
    void *ptr;                         // Pointer to allocated memory
    size_t size;                        // Size of allocation
    size_t actual_size;                 // Actual size allocated (including overhead)
    uint32_t allocation_id;             // Unique allocation ID
    uint64_t allocation_timestamp;       // When allocation occurred
    uint32_t thread_id;                 // Thread that made allocation
    bool is_freed;                     // Whether block has been freed
    uint64_t free_timestamp;            // When free occurred
    struct MemoryBlock *next;           // Next block in list
    char tag[32];                      // Allocation tag for debugging
} MemoryBlock;

// ========================================
// Memory Tracker Structure
// ========================================

typedef struct MemoryTracker {
    MemoryBlock *active_blocks;          // List of active blocks
    MemoryBlock *freed_blocks;          // List of freed blocks (for debugging)
    uint32_t total_blocks;              // Total blocks allocated
    uint32_t active_count;              // Number of active blocks
    uint32_t freed_count;               // Number of freed blocks
    size_t total_active_bytes;          // Total active bytes
    size_t total_freed_bytes;          // Total freed bytes
    size_t peak_active_bytes;           // Peak active bytes
    uint32_t next_allocation_id;        // Next allocation ID
    bool is_tracking_enabled;           // Whether tracking is enabled
} MemoryTracker;

// ========================================
// Test Object Types
// ========================================

typedef enum TestObjectType {
    TEST_OBJECT_SIMPLE,                 // Simple object with basic fields
    TEST_OBJECT_COMPLEX,                // Complex object with nested structures
    TEST_OBJECT_ARRAY,                  // Object containing arrays
    TEST_OBJECT_STRING,                  // Object with string data
    TEST_OBJECT_LINKED_LIST,            // Linked list node
    TEST_OBJECT_TREE_NODE,               // Tree node
    TEST_OBJECT_GRAPH_NODE,              // Graph node
    TEST_OBJECT_BUFFER,                  // Buffer object
    TEST_OBJECT_CUSTOM,                  // Custom object type
    TEST_OBJECT_COUNT                    // Total object types
} TestObjectType;

// ========================================
// Test Object Base Structure
// ========================================

typedef struct TestObject {
    TestObjectType type;                 // Object type
    uint32_t id;                        // Object ID
    size_t size;                        // Object size
    uint64_t creation_time;              // Creation timestamp
    uint32_t ref_count;                 // Reference count
    bool is_valid;                      // Whether object is valid
    void *data;                         // Object-specific data
    struct TestObject *next;             // Next object (for linked structures)
} TestObject;

// ========================================
// Global Test State
// ========================================

extern MemoryAllocationTestConfig g_memory_allocation_test_config;
extern MemoryAllocationTestResults g_memory_allocation_test_results;
extern MemoryTracker g_memory_tracker;

// ========================================
// Test Suite Functions
// ========================================

/**
 * Initialize memory allocation test
 * @param config Test configuration
 * @return True if initialization successful
 */
bool memory_allocation_test_init(const MemoryAllocationTestConfig *config);

/**
 * Shutdown memory allocation test
 * @param generate_report Whether to generate final report
 */
void memory_allocation_test_shutdown(bool generate_report);

/**
 * Run memory allocation test
 * @return True if test passes
 */
bool memory_allocation_test_run(void);

/**
 * Get test results
 * @return Test results
 */
MemoryAllocationTestResults memory_allocation_test_get_results(void);

/**
 * Print test summary
 */
void memory_allocation_test_print_summary(void);

/**
 * Export test results to file
 * @param filename Output filename
 * @return True if export successful
 */
bool memory_allocation_test_export_results(const char *filename);

// ========================================
// Memory Tracking Functions
// ========================================

/**
 * Initialize memory tracker
 * @return True if initialization successful
 */
bool memory_tracker_init(void);

/**
 * Shutdown memory tracker
 */
void memory_tracker_shutdown(void);

/**
 * Track memory allocation
 * @param ptr Allocated pointer
 * @param size Allocation size
 * @param tag Allocation tag
 * @return True if tracking successful
 */
bool memory_tracker_track_allocation(void *ptr, size_t size, const char *tag);

/**
 * Track memory deallocation
 * @param ptr Deallocated pointer
 * @return True if tracking successful
 */
bool memory_tracker_track_deallocation(void *ptr);

/**
 * Get memory tracker statistics
 * @return Memory tracker statistics
 */
MemoryTracker memory_tracker_get_statistics(void);

/**
 * Check for memory leaks
 * @param leak_count Output number of leaks
 * @return True if leaks detected
 */
bool memory_tracker_check_leaks(uint32_t *leak_count);

/**
 * Generate memory leak report
 * @param report Output report string
 * @param max_size Maximum report size
 * @return True if report generated
 */
bool memory_tracker_generate_leak_report(char *report, size_t max_size);

// ========================================
// Test Object Creation Functions
// ========================================

/**
 * Create simple test object
 * @param value Integer value
 * @return Created object or NULL on failure
 */
TestObject* create_simple_object(int value);

/**
 * Create complex test object
 * @param data_size Size of complex data
 * @return Created object or NULL on failure
 */
TestObject* create_complex_object(size_t data_size);

/**
 * Create array test object
 * @param array_size Size of array
 * @return Created object or NULL on failure
 */
TestObject* create_array_object(uint32_t array_size);

/**
 * Create string test object
 * @param text String content
 * @return Created object or NULL on failure
 */
TestObject* create_string_object(const char *text);

/**
 * Create linked list node
 * @param data Node data
 * @param next Next node
 * @return Created object or NULL on failure
 */
TestObject* create_linked_list_node(int data, TestObject *next);

/**
 * Create tree node
 * @param value Node value
 * @param left Left child
 * @param right Right child
 * @return Created object or NULL on failure
 */
TestObject* create_tree_node(int value, TestObject *left, TestObject *right);

/**
 * Create graph node
 * @param value Node value
 * @param neighbor_count Number of neighbors
 * @return Created object or NULL on failure
 */
TestObject* create_graph_node(int value, uint32_t neighbor_count);

/**
 * Create buffer object
 * @param buffer_size Buffer size
 * @return Created object or NULL on failure
 */
TestObject* create_buffer_object(size_t buffer_size);

/**
 * Create custom test object
 * @param custom_data Custom data pointer
 * @param data_size Custom data size
 * @return Created object or NULL on failure
 */
TestObject* create_custom_object(void *custom_data, size_t data_size);

// ========================================
// Test Object Destruction Functions
// ========================================

/**
 * Destroy test object
 * @param object Object to destroy
 */
void destroy_test_object(TestObject *object);

/**
 * Destroy linked list
 * @param head Head of linked list
 */
void destroy_linked_list(TestObject *head);

/**
 * Destroy tree
 * @param root Root of tree
 */
void destroy_tree(TestObject *root);

/**
 * Destroy graph
 * @param nodes Array of graph nodes
 * @param node_count Number of nodes
 */
void destroy_graph(TestObject **nodes, uint32_t node_count);

// ========================================
// Allocation Test Functions
// ========================================

/**
 * Test basic allocation and deallocation
 * @return True if test passes
 */
bool test_basic_allocation_deallocation(void);

/**
 * Test multiple allocation cycles
 * @return True if test passes
 */
bool test_multiple_allocation_cycles(void);

/**
 * Test allocation with different sizes
 * @return True if test passes
 */
bool test_variable_size_allocations(void);

/**
 * Test allocation with alignment requirements
 * @return True if test passes
 */
bool test_aligned_allocations(void);

/**
 * Test large allocations
 * @return True if test passes
 */
bool test_large_allocations(void);

/**
 * Test many small allocations
 * @return True if test passes
 */
bool test_many_small_allocations(void);

/**
 * Test allocation failure scenarios
 * @return True if test passes
 */
bool test_allocation_failures(void);

// ========================================
// Stress Test Functions
// ========================================

/**
 * Stress test with 1000 allocations
 * @return True if test passes
 */
bool stress_test_1000_allocations(void);

/**
 * Stress test with allocation/deallocation cycles
 * @return True if test passes
 */
bool stress_test_allocation_cycles(void);

/**
 * Stress test with concurrent allocations
 * @return True if test passes
 */
bool stress_test_concurrent_allocations(void);

/**
 * Stress test with memory pressure
 * @return True if test passes
 */
bool stress_test_memory_pressure(void);

// ========================================
// Performance Test Functions
// ========================================

/**
 * Performance test allocation speed
 * @return True if test passes
 */
bool performance_test_allocation_speed(void);

/**
 * Performance test deallocation speed
 * @return True if test passes
 */
bool performance_test_deallocation_speed(void);

/**
 * Performance test memory fragmentation
 * @return True if test passes
 */
bool performance_test_memory_fragmentation(void);

// ========================================
// Validation Functions
// ========================================

/**
 * Validate memory integrity
 * @return True if memory is valid
 */
bool validate_memory_integrity(void);

/**
 * Validate object integrity
 * @param object Object to validate
 * @return True if object is valid
 */
bool validate_object_integrity(const TestObject *object);

/**
 * Validate no memory leaks
 * @return True if no leaks detected
 */
bool validate_no_memory_leaks(void);

/**
 * Validate memory alignment
 * @param ptr Memory pointer
 * @param alignment Required alignment
 * @return True if properly aligned
 */
bool validate_memory_alignment(const void *ptr, size_t alignment);

// ========================================
// Utility Functions
// ========================================

/**
 * Generate random allocation size
 * @param min_size Minimum size
 * @param max_size Maximum size
 * @return Random size
 */
size_t generate_random_size(size_t min_size, size_t max_size);

/**
 * Generate random string
 * @param length String length
 * @param output Output buffer
 * @return Generated string
 */
char* generate_random_string(uint32_t length, char *output);

/**
 * Calculate memory fragmentation
 * @return Fragmentation percentage (0-100)
 */
double calculate_memory_fragmentation(void);

/**
 * Get current memory usage
 * @return Current memory usage in bytes
 */
size_t get_current_memory_usage(void);

/**
 * Get peak memory usage
 * @return Peak memory usage in bytes
 */
size_t get_peak_memory_usage(void);

/**
 * Print memory statistics
 */
void print_memory_statistics(void);

/**
 * Generate performance report
 * @param report Output report string
 * @param max_size Maximum report size
 * @return True if report generated
 */
bool generate_performance_report(char *report, size_t max_size);

#ifdef __cplusplus
}
#endif

#endif /* MEMORY_ALLOCATION_TEST_H */
