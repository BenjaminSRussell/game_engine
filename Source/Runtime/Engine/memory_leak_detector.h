/**
 * @file memory_leak_detector.h
 * @brief Memory leak detection and tracking system
 *
 * Comprehensive memory leak detection system for the Minecraft v2 Engine.
 * Tracks allocations, deallocations, and provides detailed reporting.
 */

#ifndef MEMORY_LEAK_DETECTOR_H
#define MEMORY_LEAK_DETECTOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Memory Allocation Record
// ========================================

typedef struct MemoryRecord {
    void *ptr;                        // Pointer to allocated memory
    size_t size;                       // Size of allocation
    const char *file;                  // Source file where allocated
    int line;                          // Source line where allocated
    const char *function;              // Function name where allocated
    uint64_t timestamp;                // Allocation timestamp
    uint32_t allocation_id;           // Unique allocation ID
    uint32_t thread_id;                // Thread ID that made allocation
    bool is_freed;                     // Whether memory was freed
    uint64_t free_timestamp;           // When memory was freed
    struct MemoryRecord *next;         // Next record in hash table
    struct MemoryRecord *prev;         // Previous record in list
} MemoryRecord;

// ========================================
// Memory Statistics
// ========================================

typedef struct MemoryStatistics {
    uint64_t total_allocations;        // Total allocations made
    uint64_t total_deallocations;      // Total deallocations made
    uint64_t current_allocations;      // Currently active allocations
    uint64_t peak_allocations;         // Peak number of allocations
    size_t total_bytes_allocated;       // Total bytes allocated
    size_t total_bytes_freed;          // Total bytes freed
    size_t current_bytes_allocated;     // Currently allocated bytes
    size_t peak_bytes_allocated;        // Peak allocated bytes
    uint64_t allocation_count_by_size[32]; // Count by size buckets
    size_t bytes_by_size_bucket[32];     // Bytes by size buckets
    uint64_t allocations_by_thread[16];  // Allocations by thread ID
    double average_allocation_size;     // Average allocation size
    double allocation_rate;             // Allocations per second
    double deallocation_rate;           // Deallocations per second
} MemoryStatistics;

// ========================================
// Memory Leak Configuration
// ========================================

typedef struct MemoryLeakConfig {
    bool enabled;                      // Enable leak detection
    bool track_stack_traces;           // Track stack traces
    bool track_thread_info;            // Track thread information
    bool track_allocation_patterns;    // Track allocation patterns
    bool generate_reports_on_exit;     // Generate reports on program exit
    bool break_on_leak;                // Break on detected leak
    size_t min_allocation_size;        // Minimum allocation size to track
    uint32_t max_stack_frames;         // Maximum stack frames to capture
    const char *report_filename;       // Report output filename
    const char *ignore_patterns[16];   // Patterns to ignore
    int ignore_pattern_count;          // Number of ignore patterns
} MemoryLeakConfig;

// ========================================
// Memory Leak Report
// ========================================

typedef struct MemoryLeakReport {
    MemoryStatistics statistics;       // Memory statistics
    uint32_t leak_count;               // Number of leaks detected
    size_t total_leaked_bytes;         // Total bytes leaked
    MemoryRecord **leaks;              // Array of leaked records
    char summary[1024];                // Report summary
    char detailed_report[65536];       // Detailed report text
    uint64_t report_timestamp;         // When report was generated
    double generation_time_ms;         // Time to generate report
} MemoryLeakReport;

// ========================================
// Initialization and Configuration
// ========================================

/**
 * Initialize memory leak detector
 * @param config Configuration settings
 * @return True if initialization successful
 */
bool memory_leak_detector_init(const MemoryLeakConfig *config);

/**
 * Shutdown memory leak detector
 * @param generate_report Whether to generate final report
 */
void memory_leak_detector_shutdown(bool generate_report);

/**
 * Get current configuration
 * @return Current configuration
 */
MemoryLeakConfig memory_leak_detector_get_config(void);

/**
 * Update configuration
 * @param config New configuration
 */
void memory_leak_detector_set_config(const MemoryLeakConfig *config);

// ========================================
// Memory Tracking Functions
// ========================================

/**
 * Track memory allocation
 * @param ptr Pointer to allocated memory
 * @param size Size of allocation
 * @param file Source file
 * @param line Source line
 * @param function Function name
 * @return True if tracking successful
 */
bool memory_leak_track_allocation(void *ptr, size_t size, 
                                  const char *file, int line, const char *function);

/**
 * Track memory deallocation
 * @param ptr Pointer to deallocated memory
 * @return True if tracking successful
 */
bool memory_leak_track_deallocation(void *ptr);

/**
 * Check if pointer is tracked
 * @param ptr Pointer to check
 * @return True if pointer is being tracked
 */
bool memory_leak_is_tracked(void *ptr);

/**
 * Get allocation record for pointer
 * @param ptr Pointer to lookup
 * @return Memory record or NULL if not found
 */
MemoryRecord* memory_leak_get_record(void *ptr);

// ========================================
// Statistics and Monitoring
// ========================================

/**
 * Get current memory statistics
 * @return Memory statistics
 */
MemoryStatistics memory_leak_get_statistics(void);

/**
 * Reset memory statistics
 */
void memory_leak_reset_statistics(void);

/**
 * Get memory usage at specific time
 * @param timestamp Timestamp to query
 * @return Memory usage at that time
 */
size_t memory_leak_get_usage_at_time(uint64_t timestamp);

/**
 * Get allocation rate over time window
 * @param start_time Start timestamp
 * @param end_time End timestamp
 * @return Allocations per second
 */
double memory_leak_get_allocation_rate(uint64_t start_time, uint64_t end_time);

/**
 * Check for memory leaks
 * @param leak_count Output number of leaks
 * @param leaked_bytes Output total leaked bytes
 * @return True if leaks detected
 */
bool memory_leak_check_leaks(uint32_t *leak_count, size_t *leaked_bytes);

// ========================================
// Reporting and Analysis
// ========================================

/**
 * Generate memory leak report
 * @return Generated report or NULL on failure
 */
MemoryLeakReport* memory_leak_generate_report(void);

/**
 * Generate memory usage graph data
 * @param time_points Output time points array
 * @param usage_values Output usage values array
 * @param max_points Maximum number of points to generate
 * @return Number of points generated
 */
int memory_leak_generate_usage_graph(uint64_t *time_points, size_t *usage_values, int max_points);

/**
 * Export report to file
 * @param filename Output filename
 * @param format Export format (text, json, xml, csv)
 * @return True if export successful
 */
bool memory_leak_export_report(const char *filename, const char *format);

/**
 * Print memory leak summary to console
 */
void memory_leak_print_summary(void);

/**
 * Print detailed memory leak report
 * @param include_freed Include freed allocations in report
 */
void memory_leak_print_detailed_report(bool include_freed);

/**
 * Print allocation patterns analysis
 */
void memory_leak_print_allocation_patterns(void);

// ========================================
// Advanced Analysis
// ========================================

/**
 * Find potential memory leaks by pattern
 * @param pattern Pattern to search for
 * @param matches Output array of matching records
 * @param max_matches Maximum matches to return
 * @return Number of matches found
 */
int memory_leak_find_by_pattern(const char *pattern, MemoryRecord **matches, int max_matches);

/**
 * Analyze allocation patterns
 * @param window_size_ms Time window size in milliseconds
 * @return Pattern analysis report
 */
char* memory_leak_analyze_patterns(uint64_t window_size_ms);

/**
 * Detect memory fragmentation
 * @return Fragmentation percentage (0-100)
 */
double memory_leak_detect_fragmentation(void);

/**
 * Find memory hotspots (frequently allocated/freed)
 * @param hotspots Output array of hotspot locations
 * @param max_hotspots Maximum hotspots to return
 * @return Number of hotspots found
 */
int memory_leak_find_hotspots(MemoryRecord **hotspots, int max_hotspots);

// ========================================
// Debugging and Utilities
// ========================================

/**
 * Enable/disable debug mode
 * @param enabled Enable debug mode
 */
void memory_leak_set_debug_mode(bool enabled);

/**
 * Set custom allocation callback
 * @param callback Custom callback function
 */
void memory_leak_set_allocation_callback(void (*callback)(MemoryRecord*));

/**
 * Set custom deallocation callback
 * @param callback Custom callback function
 */
void memory_leak_set_deallocation_callback(void (*callback)(MemoryRecord*));

/**
 * Dump all active allocations to log
 */
void memory_leak_dump_active_allocations(void);

/**
 * Validate memory tracking integrity
 * @return True if tracking is consistent
 */
bool memory_leak_validate_tracking(void);

/**
 * Force garbage collection of tracking data
 */
void memory_leak_force_gc(void);

// ========================================
// Macros for Easy Integration
// ========================================

#ifdef MEMORY_LEAK_DETECTION_ENABLED
    #define MALLOC(size) memory_leak_malloc(size, __FILE__, __LINE__, __FUNCTION__)
    #define FREE(ptr) memory_leak_free(ptr, __FILE__, __LINE__, __FUNCTION__)
    #define CALLOC(count, size) memory_leak_calloc(count, size, __FILE__, __LINE__, __FUNCTION__)
    #define REALLOC(ptr, size) memory_leak_realloc(ptr, size, __FILE__, __LINE__, __FUNCTION__)
#else
    #define MALLOC(size) malloc(size)
    #define FREE(ptr) free(ptr)
    #define CALLOC(count, size) calloc(count, size)
    #define REALLOC(ptr, size) realloc(ptr, size)
#endif

// ========================================
// Wrapper Functions
// ========================================

/**
 * Wrapper for malloc with tracking
 */
void* memory_leak_malloc(size_t size, const char *file, int line, const char *function);

/**
 * Wrapper for free with tracking
 */
void memory_leak_free(void *ptr, const char *file, int line, const char *function);

/**
 * Wrapper for calloc with tracking
 */
void* memory_leak_calloc(size_t count, size_t size, const char *file, int line, const char *function);

/**
 * Wrapper for realloc with tracking
 */
void* memory_leak_realloc(void *ptr, size_t size, const char *file, int line, const char *function);

#ifdef __cplusplus
}
#endif

#endif /* MEMORY_LEAK_DETECTOR_H */
