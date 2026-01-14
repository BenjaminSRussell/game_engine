/**
 * @file data_structure_stress_tests.h
 * @brief Comprehensive data structure stress tests for core engine systems
 * 
 * This header defines stress testing framework for all core data structures
 * including memory allocators, containers, and engine subsystems.
 * 
 * @author Minecraft v2 Engine Team
 * @date 2026-01-13
 */

#ifndef DATA_STRUCTURE_STRESS_TESTS_H
#define DATA_STRUCTURE_STRESS_TESTS_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <pthread.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Test Configuration and Results
// ========================================

/**
 * @brief Configuration for stress test execution
 * 
 * @param iterations Number of iterations to run for each test
 * @param concurrent_threads Number of concurrent threads for multi-threaded tests
 * @param memory_limit_mb Memory limit in MB for tests
 * @param timeout_seconds Maximum time to run each test
 * @param enable_verbose_logging Enable detailed logging output
 * @param enable_memory_validation Enable memory corruption detection
 * @param enable_performance_profiling Enable detailed performance metrics
 */
typedef struct {
    uint32_t iterations;
    uint32_t concurrent_threads;
    uint32_t memory_limit_mb;
    uint32_t timeout_seconds;
    bool enable_verbose_logging;
    bool enable_memory_validation;
    bool enable_performance_profiling;
} StressTestConfig;

/**
 * @brief Results from stress test execution
 * 
 * @param test_name Name of the test that was run
 * @param iterations_completed Number of iterations actually completed
 * @param total_time_ms Total time taken in milliseconds
 * @param memory_peak_mb Peak memory usage in MB
 * @param operations_per_second Operations per second achieved
 * @param success_rate Percentage of successful operations
 * @param errors_detected Number of errors detected during test
 * @param memory_leaks_detected Number of memory leaks detected
 * @param corruption_detected Whether memory corruption was detected
 */
typedef struct {
    char test_name[256];
    uint32_t iterations_completed;
    uint64_t total_time_ms;
    uint32_t memory_peak_mb;
    double operations_per_second;
    double success_rate;
    uint32_t errors_detected;
    uint32_t memory_leaks_detected;
    bool corruption_detected;
} StressTestResult;

/**
 * @brief Memory allocation tracking for leak detection
 * 
 * @param ptr Pointer to allocated memory
 * @param size Size of allocation
 * @param file Source file where allocation occurred
 * @param line Line number where allocation occurred
 * @param thread_id Thread ID that made the allocation
 * @param timestamp Timestamp of allocation
 */
typedef struct {
    void* ptr;
    size_t size;
    char file[256];
    int line;
    pthread_t thread_id;
    uint64_t timestamp;
    struct MemoryAllocation* next;
} MemoryAllocation;

/**
 * @brief Global stress test state
 * 
 * @param config Current test configuration
 * @param allocation_head Head of allocation tracking list
 * @param allocation_mutex Mutex for allocation tracking
 * @param total_allocations Total number of active allocations
 * @param total_allocated_bytes Total bytes currently allocated
 * @param peak_allocated_bytes Peak bytes allocated during test
 * @param errors_detected Total errors detected across all tests
 */
typedef struct {
    StressTestConfig config;
    MemoryAllocation* allocation_head;
    pthread_mutex_t allocation_mutex;
    uint32_t total_allocations;
    uint64_t total_allocated_bytes;
    uint64_t peak_allocated_bytes;
    uint32_t errors_detected;
} StressTestState;

// ========================================
// Core Test Functions
// ========================================

/**
 * @brief Initialize the stress testing framework
 * 
 * Sets up global state, initializes memory tracking, and prepares
 * the testing environment for stress tests.
 * 
 * @param config Configuration parameters for stress testing
 * @return true if initialization successful, false otherwise
 */
bool stress_test_init(const StressTestConfig* config);

/**
 * @brief Shutdown the stress testing framework
 * 
 * Cleans up all resources, reports final statistics, and detects
 * any remaining memory leaks.
 * 
 * @return true if shutdown completed successfully, false otherwise
 */
bool stress_test_shutdown(void);

/**
 * @brief Run all data structure stress tests
 * 
 * Executes the complete suite of stress tests for all core data
 * structures and reports comprehensive results.
 * 
 * @param results Array to store test results (must be large enough)
 * @param max_results Maximum number of results that can be stored
 * @return Number of tests executed, or -1 on error
 */
int stress_test_run_all(StressTestResult* results, int max_results);

// ========================================
// Individual Test Functions
// ========================================

/**
 * @brief Stress test memory allocator performance and correctness
 * 
 * Tests allocation/deallocation patterns, fragmentation handling,
 * and memory leak detection under high load.
 * 
 * @param result Pointer to store test results
 * @return true if test passed, false otherwise
 */
bool stress_test_memory_allocator(StressTestResult* result);

/**
 * @brief Stress test linked list data structure operations
 * 
 * Tests insertion, deletion, traversal, and concurrent access
 * performance with large datasets.
 * 
 * @param result Pointer to store test results
 * @return true if test passed, false otherwise
 */
bool stress_test_linked_list(StressTestResult* result);

/**
 * @brief Stress test hash table operations and collision handling
 * 
 * Tests insertion, lookup, deletion, and resize operations under
 * high load with various key distributions.
 * 
 * @param result Pointer to store test results
 * @return true if test passed, false otherwise
 */
bool stress_test_hash_table(StressTestResult* result);

/**
 * @brief Stress test dynamic array operations and resizing
 * 
 * Tests append, insert, delete, and automatic resizing performance
 * with various growth patterns.
 * 
 * @param result Pointer to store test results
 * @return true if test passed, false otherwise
 */
bool stress_test_dynamic_array(StressTestResult* result);

/**
 * @brief Stress test binary search tree operations
 * 
 * Tests insertion, deletion, search, and balancing operations
 * with various data distributions.
 * 
 * @param result Pointer to store test results
 * @return true if test passed, false otherwise
 */
bool stress_test_binary_search_tree(StressTestResult* result);

/**
 * @brief Stress test priority queue operations
 * 
 * Tests insert, extract-min, decrease-key, and heap operations
 * under high load conditions.
 * 
 * @param result Pointer to store test results
 * @return true if test passed, false otherwise
 */
bool stress_test_priority_queue(StressTestResult* result);

/**
 * @brief Stress test string operations and memory management
 * 
 * Tests string creation, concatenation, copying, and memory
 * management under stress conditions.
 * 
 * @param result Pointer to store test results
 * @return true if test passed, false otherwise
 */
bool stress_test_string_operations(StressTestResult* result);

/**
 * @brief Stress test entity system performance
 * 
 * Tests entity creation, destruction, component management, and
 * query performance with large numbers of entities.
 * 
 * @param result Pointer to store test results
 * @return true if test passed, false otherwise
 */
bool stress_test_entity_system(StressTestResult* result);

/**
 * @brief Stress test resource management and caching
 * 
 * Tests resource loading, caching, eviction, and memory management
 * under high load conditions.
 * 
 * @param result Pointer to store test results
 * @return true if test passed, false otherwise
 */
bool stress_test_resource_management(StressTestResult* result);

/**
 * @brief Stress test event system performance
 * 
 * Tests event dispatch, subscription, unsubscription, and
 * performance under high event rates.
 * 
 * @param result Pointer to store test results
 * @return true if test passed, false otherwise
 */
bool stress_test_event_system(StressTestResult* result);

// ========================================
// Memory Tracking and Validation Functions
// ========================================

/**
 * @brief Track a memory allocation for leak detection
 * 
 * Records allocation metadata for later validation and leak detection.
 * Thread-safe and can be called from any allocator implementation.
 * 
 * @param ptr Pointer to allocated memory
 * @param size Size of allocation in bytes
 * @param file Source file name (can be NULL)
 * @param line Line number (0 if unknown)
 * @return true if tracking successful, false otherwise
 */
bool stress_test_track_allocation(void* ptr, size_t size, const char* file, int line);

/**
 * @brief Remove a memory allocation from tracking
 * 
 * Removes allocation from tracking and validates that it was
 * previously tracked. Detects double-free errors.
 * 
 * @param ptr Pointer to memory being deallocated
 * @return true if deallocation tracked successfully, false otherwise
 */
bool stress_test_track_deallocation(void* ptr);

/**
 * @brief Validate all tracked allocations for corruption
 * 
 * Scans all tracked allocations and checks for memory corruption
 * using guard patterns and checksums.
 * 
 * @return Number of corrupted allocations detected
 */
uint32_t stress_test_validate_memory(void);

/**
 * @brief Generate memory leak report
 * 
 * Generates a detailed report of all memory leaks detected,
 * including allocation locations and sizes.
 * 
 * @param output_file File to write report to (NULL for stdout)
 * @return Number of memory leaks detected
 */
uint32_t stress_test_generate_leak_report(const char* output_file);

/**
 * @brief Get current memory usage statistics
 * 
 * Returns current memory usage statistics including peak usage
 * and allocation counts.
 * 
 * @param total_allocations Pointer to store total allocation count
 * @param total_bytes Pointer to store total allocated bytes
 * @param peak_bytes Pointer to store peak allocated bytes
 */
void stress_test_get_memory_stats(uint32_t* total_allocations, 
                                 uint64_t* total_bytes, 
                                 uint64_t* peak_bytes);

// ========================================
// Utility Functions
// ========================================

/**
 * @brief Generate random test data
 * 
 * Generates random data of specified size with configurable
 * patterns for testing purposes.
 * 
 * @param buffer Buffer to fill with random data
 * @param size Size of buffer in bytes
 * @param seed Random seed to use (0 for time-based)
 */
void stress_test_generate_random_data(void* buffer, size_t size, uint32_t seed);

/**
 * @brief Calculate test performance metrics
 * 
 * Calculates various performance metrics from test execution data.
 * 
 * @param iterations Number of iterations completed
 * @param time_ms Time taken in milliseconds
 * @param bytes_processed Total bytes processed (0 if not applicable)
 * @param ops_per_sec Pointer to store operations per second
 * @param throughput_mb_sec Pointer to store throughput in MB/s
 */
void stress_test_calculate_metrics(uint32_t iterations, uint64_t time_ms, 
                                  uint64_t bytes_processed,
                                  double* ops_per_sec, 
                                  double* throughput_mb_sec);

/**
 * @brief Print formatted test result
 * 
 * Prints a formatted test result to the specified output stream.
 * 
 * @param result Test result to print
 * @param verbose Enable verbose output with additional details
 */
void stress_test_print_result(const StressTestResult* result, bool verbose);

#ifdef __cplusplus
}
#endif

#endif // DATA_STRUCTURE_STRESS_TESTS_H
