/*
 * mtl_buffer.h
 * Metal buffer management
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_BUFFER_H
#define PLATFORM_MTL_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

typedef struct metal_device metal_device_t;

/* ============================================================================
 * ENUMS
 * ============================================================================ */

typedef enum metal_storage_mode {
    METAL_STORAGE_SHARED = 0,      // CPU & GPU accessible
    METAL_STORAGE_PRIVATE = 1,     // GPU-only
    METAL_STORAGE_MEMORYLESS = 2   // Tile memory (iOS/tvOS)
} metal_storage_mode_t;

typedef enum metal_buffer_usage {
    METAL_BUFFER_USAGE_VERTEX = 1 << 0,
    METAL_BUFFER_USAGE_INDEX = 1 << 1,
    METAL_BUFFER_USAGE_UNIFORM = 1 << 2,
    METAL_BUFFER_USAGE_STORAGE = 1 << 3
} metal_buffer_usage_t;

/* ============================================================================
 * TYPES
 * ============================================================================ */


typedef struct metal_buffer_stats {
    size_t total_allocated_bytes;
    size_t peak_allocated_bytes;
    uint32_t active_buffer_count;
} metal_buffer_stats_t;

typedef struct metal_buffer {
#ifdef __OBJC__
    id<MTLBuffer> buffer;
#else
    void* buffer;
#endif
    metal_storage_mode_t storage_mode;
    uint32_t usage;
    size_t size;
    void* mapped_ptr;  // For shared storage mode
    
    /* Validation & Debug */
    char label[64];
    bool is_mapped;
} metal_buffer_t;

typedef struct metal_ring_buffer {
    metal_buffer_t* buffer;
    size_t capacity;
    size_t offset;
    size_t frame_size;
    uint32_t frame_index;
    uint32_t frame_count;
    
    // Statistics
    size_t total_allocations;
    size_t bytes_allocated_current_frame;
    size_t peak_frame_usage;
} metal_ring_buffer_t;

typedef struct metal_buffer_desc {
    size_t size;
    metal_storage_mode_t storage_mode;
    uint32_t usage;
    const void* initial_data;
    const char* label;
} metal_buffer_desc_t;

/* ============================================================================
 * API - Buffer Management
 * ============================================================================ */

/* Create a Metal buffer */
metal_buffer_t* metal_buffer_create(metal_device_t* device, const metal_buffer_desc_t* desc);

/* Destroy a Metal buffer */
void metal_buffer_destroy(metal_buffer_t* buffer);

/* Map buffer memory (only for shared storage mode) */
void* metal_buffer_map(metal_buffer_t* buffer);

/* Unmap buffer memory */
void metal_buffer_unmap(metal_buffer_t* buffer);

/* Update buffer data directly (best for shared memory) */
void metal_buffer_update(metal_buffer_t* buffer, const void* data, size_t size, size_t offset);

/* Validate buffer state */
bool metal_buffer_validate(metal_buffer_t* buffer);

/* Get buffer GPU address (requires generic but useful for bindless) */
uint64_t metal_buffer_get_gpu_address(metal_buffer_t* buffer);

/* Get buffer statistics */
metal_buffer_stats_t metal_buffer_get_stats(void);


/* ============================================================================
 * API - Ring Buffer (for per-frame uniform data)
 * ============================================================================ */

/* Initialize a ring buffer */
metal_ring_buffer_t* metal_ring_buffer_create(metal_device_t* device, size_t frame_size, uint32_t frame_count);

/* Destroy a ring buffer */
void metal_ring_buffer_destroy(metal_ring_buffer_t* ring_buffer);

/* Allocate from ring buffer for current frame */
void* metal_ring_buffer_alloc(metal_ring_buffer_t* ring_buffer, size_t size, size_t alignment, size_t* out_offset);

/* Advance to next frame */
void metal_ring_buffer_next_frame(metal_ring_buffer_t* ring_buffer);

/* Reset ring buffer */
void metal_ring_buffer_reset(metal_ring_buffer_t* ring_buffer);

/* Get ring buffer statistics */
typedef struct metal_ring_buffer_stats {
    size_t total_allocations;
    size_t bytes_allocated_current_frame;
    size_t peak_frame_usage;
    double utilization;  // Percentage of frame budget used
} metal_ring_buffer_stats_t;

metal_ring_buffer_stats_t metal_ring_buffer_get_stats(metal_ring_buffer_t* ring_buffer);

/* Check if allocation would overflow current frame */
bool metal_ring_buffer_can_allocate(metal_ring_buffer_t* ring_buffer, size_t size, size_t alignment);


/* ============================================================================
 * API - Buffer Sub-Allocation (for efficient small buffer pooling)
 * ============================================================================ */

typedef struct metal_buffer_allocator metal_buffer_allocator_t;

/* Create a buffer allocator with a backing pool */
metal_buffer_allocator_t* metal_buffer_allocator_create(metal_device_t* device, 
                                                         size_t pool_size,
                                                         metal_storage_mode_t storage_mode);

/* Destroy allocator and all sub-allocated buffers */
void metal_buffer_allocator_destroy(metal_buffer_allocator_t* allocator);

/* Sub-allocate a buffer from the pool */
metal_buffer_t* metal_buffer_suballocate(metal_buffer_allocator_t* allocator, 
                                         size_t size, 
                                         size_t alignment);

/* Free a sub-allocated buffer back to the pool */
void metal_buffer_subfree(metal_buffer_allocator_t* allocator, metal_buffer_t* buffer);

/* Reset allocator (frees all sub-allocations) */
void metal_buffer_allocator_reset(metal_buffer_allocator_t* allocator);

typedef struct metal_buffer_allocator_stats {
    size_t pool_size;
    size_t allocated_bytes;
    size_t free_bytes;
    uint32_t allocation_count;
    double fragmentation;  // 0.0 = no fragmentation, 1.0 = heavily fragmented
} metal_buffer_allocator_stats_t;

metal_buffer_allocator_stats_t metal_buffer_allocator_get_stats(metal_buffer_allocator_t* allocator);


/* ============================================================================
 * API - Async Buffer Operations
 * ============================================================================ */

/* Callback invoked when async operation completes */
typedef void (*metal_buffer_upload_callback_t)(void* user_data);

/* Upload data to buffer asynchronously (non-blocking) */
void metal_buffer_upload_async(metal_buffer_t* buffer, 
                               const void* data, 
                               size_t size, 
                               size_t offset,
                               metal_buffer_upload_callback_t callback, 
                               void* user_data);

/* Copy buffer to buffer asynchronously */
void metal_buffer_copy_async(metal_buffer_t* src,
                             metal_buffer_t* dst,
                             size_t src_offset,
                             size_t dst_offset,
                             size_t size,
                             metal_buffer_upload_callback_t callback,
                             void* user_data);

/* Wait for all pending async buffer operations to complete */
void metal_buffer_wait_async_operations(void);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_BUFFER_H */
