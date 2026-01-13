/*
 * buffer_pool.h
 * Pooling system for indirect command buffers
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_BUFFER_POOL_H
#define RENDERING_BUFFER_POOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

typedef struct metal_buffer metal_buffer_t;
typedef struct indirect_command_buffer indirect_command_buffer_t;

/* ============================================================================
 * TYPES & STRUCTURES
 * ============================================================================ */

// Buffer pool descriptor
typedef struct buffer_pool_desc {
    uint32_t initial_capacity;      // Initial number of buffers
    size_t buffer_size;             // Size of each buffer
    uint32_t max_capacity;          // Maximum buffers (0 = unlimited)
    bool allow_growth;              // Allow pool to grow
    const char* label;
} buffer_pool_desc_t;

// Pooled buffer handle
typedef struct pooled_buffer_handle {
    uint32_t pool_id;
    uint32_t buffer_index;
    uint64_t generation;            // For detecting stale handles
} pooled_buffer_handle_t;

// Buffer pool statistics
typedef struct buffer_pool_stats {
    uint32_t capacity;
    uint32_t allocated;
    uint32_t peak_allocated;
    uint32_t total_allocations;
    uint32_t total_deallocations;
    size_t total_memory_bytes;
    float utilization;              // allocated / capacity
} buffer_pool_stats_t;

// Buffer pool
typedef struct buffer_pool {
    metal_buffer_t** buffers;       // Array of buffers
    uint64_t* generations;          // Generation counter per buffer
    uint32_t* free_list;            // Free buffer indices
    
    uint32_t capacity;
    uint32_t allocated;
    uint32_t free_count;
    uint32_t peak_allocated;
    uint32_t total_allocations;
    
    size_t buffer_size;
    uint32_t max_capacity;
    bool allow_growth;
    
    uint32_t pool_id;
    char label[64];
} buffer_pool_t;

/* ============================================================================
 * API - POOL MANAGEMENT
 * ============================================================================ */

/**
 * Create a buffer pool
 */
buffer_pool_t* buffer_pool_create(const buffer_pool_desc_t* desc);

/**
 * Destroy a buffer pool
 */
void buffer_pool_destroy(buffer_pool_t* pool);

/**
 * Reset pool (mark all buffers as free)
 */
void buffer_pool_reset(buffer_pool_t* pool);

/* ============================================================================
 * API - BUFFER ALLOCATION
 * ============================================================================ */

/**
 * Allocate a buffer from the pool
 * @param pool Buffer pool
 * @param out_handle Output handle
 * @param out_buffer Output buffer pointer
 * @return 0 on success, -1 on failure
 */
int buffer_pool_allocate(
    buffer_pool_t* pool,
    pooled_buffer_handle_t* out_handle,
    metal_buffer_t** out_buffer);

/**
 * Free a buffer back to the pool
 */
int buffer_pool_free(
    buffer_pool_t* pool,
    pooled_buffer_handle_t handle);

/**
 * Get buffer from handle
 */
metal_buffer_t* buffer_pool_get_buffer(
    buffer_pool_t* pool,
    pooled_buffer_handle_t handle);

/**
 * Check if handle is valid
 */
bool buffer_pool_is_valid(
    const buffer_pool_t* pool,
    pooled_buffer_handle_t handle);

/* ============================================================================
 * API - STATISTICS
 * ============================================================================ */

/**
 * Get pool statistics
 */
buffer_pool_stats_t buffer_pool_get_stats(const buffer_pool_t* pool);

/**
 * Print pool debug info
 */
void buffer_pool_debug_print(const buffer_pool_t* pool);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_BUFFER_POOL_H */
