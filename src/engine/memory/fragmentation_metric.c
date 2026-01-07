#include "../../include/core/memory/fragmentation_metric.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * =================================================================================================
 *                          MEMORY FRAGMENTATION METRICS - IMPLEMENTATION
 * =================================================================================================
 *
 * PURPOSE: Calculate external entropy to detect heap rot.
 * =================================================================================================
 */

static size_t g_fragmentation_alarm_threshold = 60;  // 60% default threshold

size_t fragmentation_calc_external(size_t total_memory, size_t allocated_memory, size_t largest_free) {
    if (total_memory == 0 || allocated_memory == 0) return 0;

    size_t total_free = total_memory - allocated_memory;
    if (total_free == 0) return 0;

    // External fragmentation = (total_free - largest_free) / total_free * 100
    size_t fragmented = total_free - largest_free;
    return (fragmented * 100) / total_free;
}

size_t fragmentation_find_largest_free_block(void *allocator_context,
                                            bool (*get_block_fn)(void *, size_t, bool, size_t *)) {
    if (!allocator_context || !get_block_fn) return 0;

    size_t largest = 0;
    size_t idx = 0;
    bool is_free;
    size_t block_size;

    // Iterate blocks until callback returns false
    while (get_block_fn(allocator_context, idx, true, &block_size)) {
        is_free = (block_size > 0);
        if (is_free && block_size > largest) {
            largest = block_size;
        }
        idx++;
    }

    return largest;
}

bool fragmentation_generate_histogram(void *allocator_context,
                                     bool (*get_block_fn)(void *, size_t, bool, size_t *),
                                     FragmentationHistogram *histogram) {
    if (!allocator_context || !get_block_fn || !histogram) return false;

    // Allocate temporary storage for blocks
    size_t *blocks = (size_t *)malloc(sizeof(size_t) * 1024);  // Max 1024 blocks
    if (!blocks) return false;

    uint32_t count = 0;
    size_t idx = 0;
    size_t block_size;
    size_t min_block = (size_t)-1;
    size_t max_block = 0;

    // Collect blocks
    while (count < 1024 && get_block_fn(allocator_context, idx, false, &block_size)) {
        blocks[count] = block_size;
        if (block_size < min_block) min_block = block_size;
        if (block_size > max_block) max_block = block_size;
        count++;
        idx++;
    }

    if (count == 0) {
        free(blocks);
        return false;
    }

    // Build histogram
    uint32_t num_buckets = 32;  // Create 32 size buckets
    histogram->block_sizes = (size_t *)malloc(sizeof(size_t) * num_buckets);
    histogram->frequencies = (uint32_t *)malloc(sizeof(uint32_t) * num_buckets);
    if (!histogram->block_sizes || !histogram->frequencies) {
        free(blocks);
        free(histogram->block_sizes);
        free(histogram->frequencies);
        return false;
    }

    memset(histogram->frequencies, 0, sizeof(uint32_t) * num_buckets);
    histogram->count = num_buckets;
    histogram->min_block = min_block;
    histogram->max_block = max_block;

    // Classify blocks into buckets
    size_t bucket_size = (max_block - min_block) / num_buckets + 1;
    for (uint32_t i = 0; i < count; i++) {
        uint32_t bucket = (blocks[i] - min_block) / bucket_size;
        if (bucket >= num_buckets) bucket = num_buckets - 1;
        histogram->frequencies[bucket]++;
        histogram->block_sizes[bucket] = min_block + bucket * bucket_size;
    }

    free(blocks);
    return true;
}

uint32_t fragmentation_suggest_defrag(const FragmentationMetrics *metrics) {
    if (!metrics) return 0;

    // Decision tree for defragmentation priority
    size_t frag_percentage = metrics->external_fragmentation;
    size_t free_ratio = metrics->total_free > 0 ?
        (metrics->largest_free_block * 100) / metrics->total_free : 0;

    if (frag_percentage > 80) return 3;  // URGENT - high fragmentation
    if (frag_percentage > 60 && free_ratio < 30) return 2;  // CONSIDER
    if (frag_percentage > 40) return 1;  // MONITOR
    return 0;  // NO ACTION
}

void fragmentation_set_alarm_threshold(size_t percentage) {
    if (percentage > 100) percentage = 100;
    g_fragmentation_alarm_threshold = percentage;
}

bool fragmentation_check_alarm(const FragmentationMetrics *metrics) {
    if (!metrics) return false;
    return metrics->external_fragmentation > g_fragmentation_alarm_threshold;
}

bool fragmentation_stress_test(uint32_t iterations) {
    // Simulate pathological allocation patterns
    // 1. Many small allocations
    // 2. Free every other block
    // 3. Allocate large block (should fail if fragmented)

    void **ptrs = (void **)malloc(sizeof(void *) * 1000);
    if (!ptrs) return false;

    // Pattern: allocate, free every other
    for (uint32_t iter = 0; iter < iterations; iter++) {
        // Allocate small blocks
        for (int i = 0; i < 1000; i++) {
            ptrs[i] = malloc(64 + (i % 32));
        }

        // Free every other
        for (int i = 0; i < 1000; i += 2) {
            free(ptrs[i]);
            ptrs[i] = NULL;
        }

        // Try to allocate large block (fragmentation test)
        void *large = malloc(8192);
        if (large) {
            free(large);
        }

        // Cleanup
        for (int i = 0; i < 1000; i++) {
            if (ptrs[i]) {
                free(ptrs[i]);
                ptrs[i] = NULL;
            }
        }
    }

    free(ptrs);
    return true;
}

void fragmentation_histogram_free(FragmentationHistogram *histogram) {
    if (!histogram) return;
    free(histogram->block_sizes);
    free(histogram->frequencies);
    histogram->block_sizes = NULL;
    histogram->frequencies = NULL;
    histogram->count = 0;
}
