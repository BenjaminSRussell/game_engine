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
} metal_buffer_t;

typedef struct metal_ring_buffer {
    metal_buffer_t* buffer;
    size_t capacity;
    size_t offset;
    size_t frame_size;
    uint32_t frame_index;
} metal_ring_buffer_t;

typedef struct metal_buffer_desc {
    size_t size;
    metal_storage_mode_t storage_mode;
    uint32_t usage;
    const void* initial_data;
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

/* Unmap buffer memory (no-op for Metal, included for API consistency) */
void metal_buffer_unmap(metal_buffer_t* buffer);

/* Update buffer data */
void metal_buffer_update(metal_buffer_t* buffer, const void* data, size_t size, size_t offset);

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

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_BUFFER_H */
