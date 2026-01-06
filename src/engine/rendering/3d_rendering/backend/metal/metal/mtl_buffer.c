/*
 * mtl_buffer.c
 * Metal buffer implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#import <Metal/Metal.h>
#include "mtl_buffer.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

// Forward declaration for device structure
typedef struct metal_device {
    id<MTLDevice> device;
    id<MTLCommandQueue> command_queue;
} metal_device_t;

static inline size_t align_size(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

static MTLResourceOptions storage_mode_to_mtl_options(metal_storage_mode_t mode) {
    switch (mode) {
        case METAL_STORAGE_SHARED:
            return MTLResourceStorageModeShared;
        case METAL_STORAGE_PRIVATE:
            return MTLResourceStorageModePrivate;
        case METAL_STORAGE_MEMORYLESS:
#if TARGET_OS_IOS || TARGET_OS_TV
            return MTLResourceStorageModeMemoryless;
#else
            // Memoryless not supported on macOS, fall back to private
            return MTLResourceStorageModePrivate;
#endif
        default:
            return MTLResourceStorageModeShared;
    }
}

/* ============================================================================
 * BUFFER MANAGEMENT
 * ============================================================================ */

metal_buffer_t* metal_buffer_create(metal_device_t* device, const metal_buffer_desc_t* desc) {
    if (!device || !desc || desc->size == 0) {
        return NULL;
    }

    metal_buffer_t* buffer = (metal_buffer_t*)calloc(1, sizeof(metal_buffer_t));
    if (!buffer) {
        return NULL;
    }

    MTLResourceOptions options = storage_mode_to_mtl_options(desc->storage_mode);
    
    // Create the Metal buffer
    if (desc->initial_data) {
        buffer->buffer = [device->device newBufferWithBytes:desc->initial_data
                                                     length:desc->size
                                                    options:options];
    } else {
        buffer->buffer = [device->device newBufferWithLength:desc->size
                                                     options:options];
    }

    if (!buffer->buffer) {
        free(buffer);
        return NULL;
    }

    buffer->storage_mode = desc->storage_mode;
    buffer->usage = desc->usage;
    buffer->size = desc->size;

    // Map pointer for shared storage mode
    if (desc->storage_mode == METAL_STORAGE_SHARED) {
        buffer->mapped_ptr = [buffer->buffer contents];
    } else {
        buffer->mapped_ptr = NULL;
    }

    return buffer;
}

void metal_buffer_destroy(metal_buffer_t* buffer) {
    if (!buffer) {
        return;
    }

    if (buffer->buffer) {
        buffer->buffer = nil;  // ARC will handle release
    }

    free(buffer);
}

void* metal_buffer_map(metal_buffer_t* buffer) {
    if (!buffer) {
        return NULL;
    }

    // Only shared storage mode can be mapped
    if (buffer->storage_mode != METAL_STORAGE_SHARED) {
        return NULL;
    }

    return buffer->mapped_ptr;
}

void metal_buffer_unmap(metal_buffer_t* buffer) {
    // No-op for Metal (included for API consistency with other backends)
    (void)buffer;
}

void metal_buffer_update(metal_buffer_t* buffer, const void* data, size_t size, size_t offset) {
    if (!buffer || !data || size == 0) {
        return;
    }

    if (offset + size > buffer->size) {
        return;  // Out of bounds
    }

    if (buffer->storage_mode == METAL_STORAGE_SHARED) {
        // Direct memory copy for shared buffers
        void* dest = (uint8_t*)buffer->mapped_ptr + offset;
        memcpy(dest, data, size);
    } else {
        // For private buffers, would need a blit command encoder
        // This is a simplified version - in production, you'd use a staging buffer
        // and a blit command encoder to transfer data
        // For now, we'll just assert this shouldn't be called on private buffers
        assert(buffer->storage_mode == METAL_STORAGE_SHARED && 
               "Direct update not supported for private buffers - use staging buffer");
    }
}

/* ============================================================================
 * RING BUFFER IMPLEMENTATION
 * ============================================================================ */

metal_ring_buffer_t* metal_ring_buffer_create(metal_device_t* device, size_t frame_size, uint32_t frame_count) {
    if (!device || frame_size == 0 || frame_count == 0) {
        return NULL;
    }

    metal_ring_buffer_t* ring_buffer = (metal_ring_buffer_t*)calloc(1, sizeof(metal_ring_buffer_t));
    if (!ring_buffer) {
        return NULL;
    }

    // Align frame size to 256 bytes (Metal constant buffer alignment)
    size_t aligned_frame_size = align_size(frame_size, 256);
    size_t total_size = aligned_frame_size * frame_count;

    // Create underlying buffer (always shared for CPU writes)
    metal_buffer_desc_t buffer_desc = {
        .size = total_size,
        .storage_mode = METAL_STORAGE_SHARED,
        .usage = METAL_BUFFER_USAGE_UNIFORM,
        .initial_data = NULL
    };

    ring_buffer->buffer = metal_buffer_create(device, &buffer_desc);
    if (!ring_buffer->buffer) {
        free(ring_buffer);
        return NULL;
    }

    ring_buffer->capacity = total_size;
    ring_buffer->frame_size = aligned_frame_size;
    ring_buffer->offset = 0;
    ring_buffer->frame_index = 0;

    return ring_buffer;
}

void metal_ring_buffer_destroy(metal_ring_buffer_t* ring_buffer) {
    if (!ring_buffer) {
        return;
    }

    if (ring_buffer->buffer) {
        metal_buffer_destroy(ring_buffer->buffer);
    }

    free(ring_buffer);
}

void* metal_ring_buffer_alloc(metal_ring_buffer_t* ring_buffer, size_t size, size_t alignment, size_t* out_offset) {
    if (!ring_buffer || size == 0) {
        return NULL;
    }

    // Align the current offset
    size_t aligned_offset = align_size(ring_buffer->offset, alignment);
    
    // Check if allocation fits in current frame
    size_t frame_start = ring_buffer->frame_index * ring_buffer->frame_size;
    size_t frame_end = frame_start + ring_buffer->frame_size;
    
    if (aligned_offset + size > frame_end) {
        // Doesn't fit in current frame
        return NULL;
    }

    // Calculate pointer
    void* ptr = (uint8_t*)ring_buffer->buffer->mapped_ptr + aligned_offset;
    
    if (out_offset) {
        *out_offset = aligned_offset;
    }

    // Advance offset for next allocation
    ring_buffer->offset = aligned_offset + size;

    return ptr;
}

void metal_ring_buffer_next_frame(metal_ring_buffer_t* ring_buffer) {
    if (!ring_buffer) {
        return;
    }

    // Move to next frame
    uint32_t max_frames = (uint32_t)(ring_buffer->capacity / ring_buffer->frame_size);
    ring_buffer->frame_index = (ring_buffer->frame_index + 1) % max_frames;
    ring_buffer->offset = ring_buffer->frame_index * ring_buffer->frame_size;
}

void metal_ring_buffer_reset(metal_ring_buffer_t* ring_buffer) {
    if (!ring_buffer) {
        return;
    }

    ring_buffer->offset = 0;
    ring_buffer->frame_index = 0;
}

/* End of mtl_buffer.c */
