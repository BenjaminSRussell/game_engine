#ifndef CORE_MEMORY_FRAGMENTATION_METRIC_H
#define CORE_MEMORY_FRAGMENTATION_METRIC_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Memory Fragmentation Analysis
 *
 * Detects and quantifies memory fragmentation in allocators.
 * Provides heuristics for defragmentation decisions.
 */

typedef struct {
    size_t external_fragmentation;  // % fragmentation (0-100)
    size_t largest_free_block;      // Largest contiguous free block
    size_t total_free;              // Total free space
    size_t total_allocated;         // Total allocated
    uint32_t free_block_count;      // Number of free blocks
    uint32_t allocated_block_count; // Number of allocated blocks
} FragmentationMetrics;

typedef struct {
    size_t *block_sizes;
    uint32_t *frequencies;
    uint32_t count;
    size_t min_block;
    size_t max_block;
} FragmentationHistogram;

/**
 * Calculate external fragmentation percentage
 * Formula: (1 - largest_free_block / total_free) * 100
 *
 * @param total_memory Total memory size
 * @param allocated_memory Currently allocated
 * @param largest_free Largest free block
 * @return Fragmentation percentage (0-100)
 */
size_t fragmentation_calc_external(size_t total_memory, size_t allocated_memory, size_t largest_free);

/**
 * Find largest free block using allocator introspection
 *
 * @param allocator_context Allocator to analyze
 * @param get_block_fn Callback to iterate blocks
 * @return Size of largest free block
 */
size_t fragmentation_find_largest_free_block(void *allocator_context,
                                            bool (*get_block_fn)(void *, size_t, bool, size_t *));

/**
 * Generate histogram of block sizes
 *
 * @param allocator_context Allocator to analyze
 * @param get_block_fn Callback to iterate blocks
 * @param histogram Output histogram structure
 * @return true if successful
 */
bool fragmentation_generate_histogram(void *allocator_context,
                                     bool (*get_block_fn)(void *, size_t, bool, size_t *),
                                     FragmentationHistogram *histogram);

/**
 * Suggest defragmentation strategy
 *
 * @param metrics Current fragmentation metrics
 * @return Priority level (0=no action, 1=monitor, 2=consider, 3=urgent)
 */
uint32_t fragmentation_suggest_defrag(const FragmentationMetrics *metrics);

/**
 * Set alarm threshold for fragmentation
 *
 * @param percentage Trigger alarm when fragmentation exceeds this %
 */
void fragmentation_set_alarm_threshold(size_t percentage);

/**
 * Check if fragmentation alarm is triggered
 *
 * @param metrics Current metrics
 * @return true if over threshold
 */
bool fragmentation_check_alarm(const FragmentationMetrics *metrics);

/**
 * Allocator stress test - simulate pathological allocation patterns
 *
 * @param iterations Number of iterations
 * @return true if all tests passed
 */
bool fragmentation_stress_test(uint32_t iterations);

/**
 * Free histogram memory
 *
 * @param histogram Histogram to free
 */
void fragmentation_histogram_free(FragmentationHistogram *histogram);

#ifdef __cplusplus
}
#endif

#endif // CORE_MEMORY_FRAGMENTATION_METRIC_H
