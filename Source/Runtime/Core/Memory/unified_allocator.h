/**
 * @file unified_allocator.h
 * @brief Unified memory allocation interface for the entire engine
 * @details Provides a single allocation interface with support for multiple allocation strategies.
 *
 * Allocation Types:
 * - ALLOC_GENERAL: General purpose malloc/free (thread-safe)
 * - ALLOC_POOL: Fixed-size object pools (fast, zero-copy)
 * - ALLOC_LINEAR: Linear arena allocators (LIFO, fast, efficient)
 * - ALLOC_TEMP: Temporary/frame allocators (reset every frame)
 * - ALLOC_PERSISTENT: Game state allocators (never freed)
 *
 * Usage:
 *   #include "core/memory/unified_allocator.h"
 *
 *   void* ptr = alloc(1024, ALLOC_GENERAL);
 *   // ...
 *   free(ptr, ALLOC_GENERAL);
 *
 * Dependencies:
 *   - sys/memory.h (OS memory operations)
 *   - core/simd/atomic.h (thread-safe operations)
 *   - core/containers/linked_list.h (allocation tracking)
 */

#ifndef UNIFIED_ALLOCATOR_H
#define UNIFIED_ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ============================================================================
 * ALLOCATION TYPES
 * ============================================================================
 */

typedef enum {
    ALLOC_GENERAL = 0,         // General purpose heap allocation
    ALLOC_POOL,                // Fixed-size object pools
    ALLOC_LINEAR,              // Linear arena allocators
    ALLOC_TEMP,                // Temporary/frame allocators
    ALLOC_PERSISTENT,          // Never freed allocations
    ALLOC_COUNT                // Total allocation types
} AllocationType;

/**
 * ============================================================================
 * STATISTICS & DIAGNOSTICS
 * ============================================================================
 */

typedef struct {
    // Per-allocator stats
    uint64_t total_allocated;      // Total bytes allocated
    uint64_t total_freed;          // Total bytes freed
    uint64_t current_usage;        // Currently used bytes
    uint64_t peak_usage;           // Peak usage so far
    uint32_t allocation_count;     // Number of active allocations
    uint32_t free_count;           // Number of free operations

    // Fragmentation metrics
    float fragmentation_ratio;     // (free_space / total_space)
    uint32_t largest_free_block;   // Size of largest contiguous free block

    // Performance metrics
    uint64_t avg_allocation_time_us;  // Average allocation time
    uint64_t avg_free_time_us;        // Average free time
} AllocatorStats;

typedef struct {
    AllocatorStats stats[ALLOC_COUNT];
    uint64_t total_system_memory;
    uint64_t total_engine_memory;
    uint32_t active_allocations;
} AllocationMetrics;

/**
 * ============================================================================
 * CORE ALLOCATION INTERFACE
 * ============================================================================
 */

/**
 * Initialize the unified allocator system
 * @param initial_heap_size Initial heap size in bytes
 * @return true on success, false on failure
 */
bool allocator_init(size_t initial_heap_size);

/**
 * Shutdown the allocator system and free all memory
 */
void allocator_shutdown(void);

/**
 * Allocate memory from specified allocator
 * @param size Number of bytes to allocate
 * @param type Allocation type (strategy)
 * @return Pointer to allocated memory, or NULL on failure
 *
 * Note: Allocation is zero-initialized for safety
 */
void* alloc(size_t size, AllocationType type);

/**
 * Allocate memory with custom alignment
 * @param size Number of bytes to allocate
 * @param alignment Required alignment (must be power of 2)
 * @param type Allocation type
 * @return Aligned pointer, or NULL on failure
 */
void* alloc_aligned(size_t size, size_t alignment, AllocationType type);

/**
 * Free previously allocated memory
 * @param ptr Pointer to free
 * @param type Allocation type (must match original allocation)
 *
 * Note: Passing wrong type may cause memory leak or corruption
 */
void free_alloc(void* ptr, AllocationType type);

/**
 * Reallocate memory (may move data)
 * @param ptr Existing pointer
 * @param new_size New size in bytes
 * @param type Allocation type
 * @return New pointer, or NULL on failure
 */
void* realloc_alloc(void* ptr, size_t new_size, AllocationType type);

/**
 * Get statistics for allocator type
 * @param type Allocation type
 * @return Statistics for this allocator
 */
AllocatorStats allocator_get_stats(AllocationType type);

/**
 * Get overall allocation metrics
 * @return Complete allocation metrics
 */
AllocationMetrics allocator_get_metrics(void);

/**
 * Reset linear/temp allocator (clears all allocations)
 * Typically called once per frame for ALLOC_TEMP
 * @param type Allocation type to reset
 */
void allocator_reset(AllocationType type);

/**
 * Debug print allocator status
 */
void allocator_debug_print(void);

/**
 * Dump allocation statistics to file
 * @param filepath File to write to
 */
void allocator_dump_stats(const char* filepath);

/**
 * Verify allocator integrity (debug only)
 * @return true if valid, false if corruption detected
 */
bool allocator_verify(void);

/**
 * ============================================================================
 * POOL ALLOCATOR (Fixed-Size Objects)
 * ============================================================================
 */

typedef struct PoolAllocator PoolAllocator;

/**
 * Create a fixed-size object pool
 * @param object_size Size of each object
 * @param pool_size Total number of objects
 * @return Pool allocator handle
 */
PoolAllocator* pool_allocator_create(size_t object_size, uint32_t pool_size);

/**
 * Destroy a pool allocator
 * @param pool Pool handle
 */
void pool_allocator_destroy(PoolAllocator* pool);

/**
 * Allocate from pool
 * @param pool Pool handle
 * @return Pointer to pool object, or NULL if pool exhausted
 */
void* pool_alloc(PoolAllocator* pool);

/**
 * Return object to pool
 * @param pool Pool handle
 * @param ptr Object pointer
 */
void pool_free(PoolAllocator* pool, void* ptr);

/**
 * ============================================================================
 * LINEAR ALLOCATOR (Arena)
 * ============================================================================
 */

typedef struct LinearAllocator LinearAllocator;

/**
 * Create a linear allocator
 * @param capacity Total capacity in bytes
 * @return Linear allocator handle
 */
LinearAllocator* linear_allocator_create(size_t capacity);

/**
 * Destroy a linear allocator
 * @param alloc Allocator handle
 */
void linear_allocator_destroy(LinearAllocator* alloc);

/**
 * Allocate from linear allocator
 * @param alloc Allocator handle
 * @param size Bytes to allocate
 * @return Pointer to allocated memory
 */
void* linear_alloc(LinearAllocator* alloc, size_t size);

/**
 * Allocate aligned from linear allocator
 * @param alloc Allocator handle
 * @param size Bytes to allocate
 * @param alignment Alignment requirement
 * @return Aligned pointer
 */
void* linear_alloc_aligned(LinearAllocator* alloc, size_t size, size_t alignment);

/**
 * Reset linear allocator (clear all allocations)
 * @param alloc Allocator handle
 */
void linear_allocator_reset(LinearAllocator* alloc);

/**
 * ============================================================================
 * CONVENIENCE MACROS
 * ============================================================================
 */

/** Allocate single item */
#define ALLOC_OBJ(type, alloc_type) \
    (type*)alloc(sizeof(type), alloc_type)

/** Allocate array of items */
#define ALLOC_ARRAY(type, count, alloc_type) \
    (type*)alloc(sizeof(type) * (count), alloc_type)

/** Allocate aligned item */
#define ALLOC_ALIGNED_OBJ(type, alignment, alloc_type) \
    (type*)alloc_aligned(sizeof(type), alignment, alloc_type)

/** Free with type checking (macro) */
#define FREE(ptr, alloc_type) \
    do { if (ptr) { free_alloc(ptr, alloc_type); (ptr) = NULL; } } while(0)

/** Allocate temporary (frame) memory */
#define ALLOC_TEMP(size) alloc(size, ALLOC_TEMP)

/** Allocate persistent memory */
#define ALLOC_PERSIST(size) alloc(size, ALLOC_PERSISTENT)

/** Allocate general memory */
#define ALLOC(size) alloc(size, ALLOC_GENERAL)

#ifdef __cplusplus
}
#endif

#endif // UNIFIED_ALLOCATOR_H
