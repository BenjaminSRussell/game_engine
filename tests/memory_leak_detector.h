/**
 * @file memory_leak_detector.h
 * @brief Advanced memory leak detection system for engine validation
 * 
 * This header defines a comprehensive memory leak detection system that tracks
 * all allocations, detects leaks, validates memory integrity, and provides
 * detailed reporting for debugging purposes.
 * 
 * @author Minecraft v2 Engine Team
 * @date 2026-01-13
 */

#ifndef MEMORY_LEAK_DETECTOR_H
#define MEMORY_LEAK_DETECTOR_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Memory Allocation Tracking
// ========================================

/**
 * @brief Memory allocation record for tracking
 * 
 * @param ptr Pointer to allocated memory
 * @param size Size of allocation in bytes
 * @param requested_size Original requested size (may be larger due to alignment)
 * @param file Source file where allocation occurred
 * @param line Line number where allocation occurred
 * @param function Function name where allocation occurred
 * @param thread_id Thread ID that made the allocation
 * @param timestamp Timestamp of allocation (nanoseconds)
 * @param allocation_id Unique allocation ID
 * @param magic Magic number for corruption detection
 * @param guard_before Guard pattern before allocation
 * @param guard_after Guard pattern after allocation
 * @param next Next allocation in hash table
 */
typedef struct MemoryAllocation {
    void* ptr;
    size_t size;
    size_t requested_size;
    char file[256];
    char function[128];
    int line;
    pthread_t thread_id;
    uint64_t timestamp;
    uint64_t allocation_id;
    uint32_t magic;
    uint32_t guard_before;
    uint32_t guard_after;
    struct MemoryAllocation* next;
} MemoryAllocation;

/**
 * @brief Memory leak detection statistics
 * 
 * @param total_allocations Total number of allocations tracked
 * @param active_allocations Current number of active allocations
 * @param total_allocated_bytes Total bytes allocated
 * @param active_allocated_bytes Current bytes allocated
 * @param peak_allocated_bytes Peak bytes allocated at any time
 * @param total_deallocations Total number of deallocations
 * @param failed_allocations Number of failed allocations
 * @param corruption_detected Number of corruption detections
 * @param double_frees_detected Number of double-free detections
 * @param invalid_frees_detected Number of invalid free attempts
 */
typedef struct {
    uint64_t total_allocations;
    uint64_t active_allocations;
    uint64_t total_allocated_bytes;
    uint64_t active_allocated_bytes;
    uint64_t peak_allocated_bytes;
    uint64_t total_deallocations;
    uint64_t failed_allocations;
    uint64_t corruption_detected;
    uint64_t double_frees_detected;
    uint64_t invalid_frees_detected;
} MemoryLeakStats;

/**
 * @brief Memory leak detection configuration
 * 
 * @param enable_tracking Enable memory tracking
 * @param enable_guard_pages Enable guard page detection
 * @param enable_magic_numbers Enable magic number validation
 * @param enable_stack_traces Enable stack trace capture
 * @param hash_table_size Size of allocation hash table
 * @param max_allocations Maximum allocations to track
 * @param guard_pattern Pattern to use for guard bytes
 * @param magic_number Magic number for allocation validation
 * @param enable_thread_safety Enable thread-safe tracking
 * @param enable_performance_monitoring Enable performance impact monitoring
 */
typedef struct {
    bool enable_tracking;
    bool enable_guard_pages;
    bool enable_magic_numbers;
    bool enable_stack_traces;
    uint32_t hash_table_size;
    uint32_t max_allocations;
    uint32_t guard_pattern;
    uint32_t magic_number;
    bool enable_thread_safety;
    bool enable_performance_monitoring;
} MemoryLeakConfig;

/**
 * @brief Memory leak report entry
 * 
 * @param allocation_id Unique allocation ID
 * @param ptr Pointer to leaked memory
 * @param size Size of leaked allocation
 * @param file Source file of allocation
 * @param line Line number of allocation
 * @param function Function name of allocation
 * @param thread_id Thread ID that made allocation
 * @param timestamp Allocation timestamp
 * @param age_seconds Age of leak in seconds
 * @param stack_trace Stack trace of allocation (if available)
 */
typedef struct {
    uint64_t allocation_id;
    void* ptr;
    size_t size;
    char file[256];
    int line;
    char function[128];
    pthread_t thread_id;
    uint64_t timestamp;
    double age_seconds;
    char stack_trace[1024];
} MemoryLeakReport;

// ========================================
// Core Functions
// ========================================

/**
 * @brief Initialize memory leak detection system
 * 
 * Sets up the memory leak detection system with the specified configuration.
 * Must be called before any memory tracking functions are used.
 * 
 * @param config Configuration for memory leak detection (NULL for defaults)
 * @return true if initialization successful, false otherwise
 */
bool memory_leak_detector_init(const MemoryLeakConfig* config);

/**
 * @brief Shutdown memory leak detection system
 * 
 * Cleans up the memory leak detection system and generates a final report.
 * Should be called when the application is shutting down.
 * 
 * @param report_file File to write final report to (NULL for stdout)
 * @return true if shutdown successful, false otherwise
 */
bool memory_leak_detector_shutdown(const char* report_file);

/**
 * @brief Track a memory allocation
 * 
 * Records a memory allocation for leak detection and corruption monitoring.
 * This function is typically called by custom memory allocators.
 * 
 * @param ptr Pointer to allocated memory
 * @param size Size of allocation in bytes
 * @param file Source file name (use __FILE__ macro)
 * @param line Line number (use __LINE__ macro)
 * @param function Function name (use __func__ macro)
 * @return true if tracking successful, false otherwise
 */
bool memory_leak_detector_track_allocation(void* ptr, size_t size, 
                                          const char* file, int line, 
                                          const char* function);

/**
 * @brief Track a memory deallocation
 * 
 * Records a memory deallocation and validates against tracked allocations.
 * Detects double-frees and invalid free attempts.
 * 
 * @param ptr Pointer to memory being deallocated
 * @param file Source file name (use __FILE__ macro)
 * @param line Line number (use __LINE__ macro)
 * @param function Function name (use __func__ macro)
 * @return true if deallocation tracked successfully, false otherwise
 */
bool memory_leak_detector_track_deallocation(void* ptr, 
                                           const char* file, int line, 
                                           const char* function);

/**
 * @brief Validate a specific memory allocation
 * 
 * Checks a specific allocation for corruption using guard patterns
 * and magic numbers.
 * 
 * @param ptr Pointer to memory to validate
 * @return true if memory is valid, false if corruption detected
 */
bool memory_leak_detector_validate_allocation(void* ptr);

/**
 * @brief Validate all tracked allocations
 * 
 * Scans all tracked allocations and checks for corruption.
 * Returns the number of corrupted allocations found.
 * 
 * @return Number of corrupted allocations detected
 */
uint32_t memory_leak_detector_validate_all(void);

// ========================================
// Statistics and Reporting
// ========================================

/**
 * @brief Get current memory leak detection statistics
 * 
 * Returns comprehensive statistics about memory allocations,
 * deallocations, and detected issues.
 * 
 * @return Current statistics structure
 */
MemoryLeakStats memory_leak_detector_get_stats(void);

/**
 * @brief Generate memory leak report
 * 
 * Generates a detailed report of all memory leaks detected.
 * 
 * @param report Array to store report entries
 * @param max_entries Maximum number of entries to store
 * @param include_active Include active allocations in report
 * @return Number of report entries generated
 */
uint32_t memory_leak_detector_generate_report(MemoryLeakReport* report, 
                                             uint32_t max_entries,
                                             bool include_active);

/**
 * @brief Print memory leak statistics
 * 
 * Prints current memory leak statistics to the specified output.
 * 
 * @param output_file File to print to (NULL for stdout)
 * @param verbose Enable verbose output with detailed information
 */
void memory_leak_detector_print_stats(const char* output_file, bool verbose);

/**
 * @brief Print memory leak report
 * 
 * Prints a detailed memory leak report to the specified output.
 * 
 * @param output_file File to print to (NULL for stdout)
 * @param sort_by_age Sort leaks by age (oldest first)
 * @param include_stack_traces Include stack traces in output
 */
void memory_leak_detector_print_report(const char* output_file, 
                                       bool sort_by_age,
                                       bool include_stack_traces);

/**
 * @brief Reset memory leak detection statistics
 * 
 * Resets all statistics counters while maintaining tracking.
 * Useful for testing different phases of application execution.
 */
void memory_leak_detector_reset_stats(void);

/**
 * @brief Clear all tracked allocations
 * 
 * Removes all tracked allocations and resets the system.
 * WARNING: This will prevent leak detection for existing allocations.
 */
void memory_leak_detector_clear_all(void);

// ========================================
// Advanced Features
// ========================================

/**
 * @brief Set memory leak detection threshold
 * 
 * Sets a threshold for memory leak warnings. If the number of
 * leaks exceeds this threshold, warnings will be generated.
 * 
 * @param leak_threshold Number of leaks that trigger warnings
 * @param byte_threshold Number of bytes that trigger warnings
 */
void memory_leak_detector_set_threshold(uint32_t leak_threshold, 
                                       uint64_t byte_threshold);

/**
 * @brief Enable/disable automatic validation
 * 
 * Enables or disables automatic memory validation on each
 * allocation/deallocation operation.
 * 
 * @param enable Enable automatic validation
 */
void memory_leak_detector_set_auto_validation(bool enable);

/**
 * @brief Get allocation by pointer
 * 
 * Retrieves the allocation record for a specific pointer.
 * 
 * @param ptr Pointer to look up
 * @return Allocation record or NULL if not found
 */
const MemoryAllocation* memory_leak_detector_get_allocation(void* ptr);

/**
 * @brief Get allocations by thread
 * 
 * Retrieves all allocations made by a specific thread.
 * 
 * @param thread_id Thread ID to filter by
 * @param allocations Array to store results
 * @param max_allocations Maximum number of results to store
 * @return Number of allocations found
 */
uint32_t memory_leak_detector_get_allocations_by_thread(pthread_t thread_id,
                                                       MemoryAllocation* allocations,
                                                       uint32_t max_allocations);

/**
 * @brief Get allocations by size range
 * 
 * Retrieves all allocations within a specified size range.
 * 
 * @param min_size Minimum size (inclusive)
 * @param max_size Maximum size (inclusive)
 * @param allocations Array to store results
 * @param max_allocations Maximum number of results to store
 * @return Number of allocations found
 */
uint32_t memory_leak_detector_get_allocations_by_size(size_t min_size,
                                                     size_t max_size,
                                                     MemoryAllocation* allocations,
                                                     uint32_t max_allocations);

/**
 * @brief Dump memory allocation map
 * 
 * Creates a visual map of memory allocations for debugging.
 * 
 * @param output_file File to write map to (NULL for stdout)
 * @param include_details Include detailed allocation information
 */
void memory_leak_detector_dump_memory_map(const char* output_file,
                                          bool include_details);

// ========================================
// Performance Monitoring
// ========================================

/**
 * @brief Get performance impact statistics
 * 
 * Returns statistics about the performance impact of memory
 * leak detection on the application.
 * 
 * @param tracking_overhead_ms Time spent in tracking operations (ms)
 * @param validation_overhead_ms Time spent in validation operations (ms)
 * @param memory_overhead_bytes Memory overhead used by tracking (bytes)
 * @param fragmentation_ratio Estimated fragmentation ratio
 */
void memory_leak_detector_get_performance_impact(uint64_t* tracking_overhead_ms,
                                                uint64_t* validation_overhead_ms,
                                                uint64_t* memory_overhead_bytes,
                                                double* fragmentation_ratio);

/**
 * @brief Enable/disable performance monitoring
 * 
 * Enables or disables detailed performance monitoring of the
 * memory leak detection system itself.
 * 
 * @param enable Enable performance monitoring
 */
void memory_leak_detector_set_performance_monitoring(bool enable);

#ifdef __cplusplus
}
#endif

#endif // MEMORY_LEAK_DETECTOR_H
