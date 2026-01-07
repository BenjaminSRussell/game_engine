/*
 * mtl_buffer.c
 * Metal buffer implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#import <Metal/Metal.h>
#include "mtl_buffer.h"
#include "mtl_device.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
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

/* ============================================================================
 * GLOBAL STATISTICS
 * ============================================================================ */

static metal_buffer_stats_t g_buffer_stats = {0};

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

    if (desc->label) {
        [buffer->buffer setLabel:[NSString stringWithUTF8String:desc->label]];
        strncpy(buffer->label, desc->label, sizeof(buffer->label) - 1);
    }

    buffer->storage_mode = desc->storage_mode;
    buffer->usage = desc->usage;
    buffer->size = desc->size;

    // Map pointer for shared storage mode
    if (desc->storage_mode == METAL_STORAGE_SHARED) {
        buffer->mapped_ptr = [buffer->buffer contents];
        buffer->is_mapped = true;
    } else {
        buffer->mapped_ptr = NULL;
        buffer->is_mapped = false;
    }

    // Update statistics
    g_buffer_stats.total_allocated_bytes += desc->size;
    g_buffer_stats.active_buffer_count++;
    if (g_buffer_stats.total_allocated_bytes > g_buffer_stats.peak_allocated_bytes) {
        g_buffer_stats.peak_allocated_bytes = g_buffer_stats.total_allocated_bytes;
    }

    return buffer;
}

void metal_buffer_destroy(metal_buffer_t* buffer) {
    if (!buffer) {
        return;
    }

    // Update statistics
    if (g_buffer_stats.total_allocated_bytes >= buffer->size) {
        g_buffer_stats.total_allocated_bytes -= buffer->size;
    }
    if (g_buffer_stats.active_buffer_count > 0) {
        g_buffer_stats.active_buffer_count--;
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

bool metal_buffer_validate(metal_buffer_t* buffer) {
    if (!buffer) return false;
    if (!buffer->buffer) return false;
    return true;
}

uint64_t metal_buffer_get_gpu_address(metal_buffer_t* buffer) {
    if (!buffer || !buffer->buffer) return 0;
    // Note: This requires Metal 3.0 (macOS 13+). Checking for availability if needed,
    // but assuming Engine targets modern macOS for this path.
    if (@available(macOS 13.0, *)) {
        return [buffer->buffer gpuAddress];
    }
    return 0;
}

metal_buffer_stats_t metal_buffer_get_stats(void) {
    return g_buffer_stats;
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
        .initial_data = NULL,
        .label = "Ring Buffer"
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
    ring_buffer->frame_count = frame_count;

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
    size_t start_offset = ring_buffer->frame_index * ring_buffer->frame_size;
    size_t local_offset = ring_buffer->offset - start_offset; // Offset within current frame
    
    // Align relative to the start of the buffer, not just the frame
    size_t current_abs_offset = ring_buffer->offset;
    size_t aligned_abs_offset = align_size(current_abs_offset, alignment);
    
    // Check if allocation fits in current frame
    size_t frame_end = start_offset + ring_buffer->frame_size;
    
    if (aligned_abs_offset + size > frame_end) {
        // Doesn't fit in current frame
        return NULL;
    }

    // Calculate pointer
    void* ptr = (uint8_t*)ring_buffer->buffer->mapped_ptr + aligned_abs_offset;
    
    if (out_offset) {
        *out_offset = aligned_abs_offset;
    }

    // Advance offset
    ring_buffer->offset = aligned_abs_offset + size;
    
    // Update statistics
    ring_buffer->total_allocations++;
    ring_buffer->bytes_allocated_current_frame += size;
    
    size_t current_usage = ring_buffer->offset - start_offset;
    if (current_usage > ring_buffer->peak_frame_usage) {
        ring_buffer->peak_frame_usage = current_usage;
    }

    return ptr;
}

void metal_ring_buffer_next_frame(metal_ring_buffer_t* ring_buffer) {
    if (!ring_buffer) {
        return;
    }

    // Move to next frame
    ring_buffer->frame_index = (ring_buffer->frame_index + 1) % ring_buffer->frame_count;
    ring_buffer->offset = ring_buffer->frame_index * ring_buffer->frame_size;
    
    // Reset per-frame statistics
    ring_buffer->bytes_allocated_current_frame = 0;
}

void metal_ring_buffer_reset(metal_ring_buffer_t* ring_buffer) {
    if (!ring_buffer) {
        return;
    }

    ring_buffer->offset = 0;
    ring_buffer->frame_index = 0;
    ring_buffer->bytes_allocated_current_frame = 0;
}

metal_ring_buffer_stats_t metal_ring_buffer_get_stats(metal_ring_buffer_t* ring_buffer) {
    metal_ring_buffer_stats_t stats = {0};
    if (!ring_buffer) {
        return stats;
    }
    
    stats.total_allocations = ring_buffer->total_allocations;
    stats.bytes_allocated_current_frame = ring_buffer->bytes_allocated_current_frame;
    stats.peak_frame_usage = ring_buffer->peak_frame_usage;
    
    // Calculate utilization
    if (ring_buffer->frame_size > 0) {
        stats.utilization = (double)ring_buffer->bytes_allocated_current_frame / (double)ring_buffer->frame_size;
    }
    
    return stats;
}

bool metal_ring_buffer_can_allocate(metal_ring_buffer_t* ring_buffer, size_t size, size_t alignment) {
    if (!ring_buffer || size == 0) {
        return false;
    }
    
    size_t start_offset = ring_buffer->frame_index * ring_buffer->frame_size;
    size_t current_abs_offset = ring_buffer->offset;
    size_t aligned_abs_offset = align_size(current_abs_offset, alignment);
    size_t frame_end = start_offset + ring_buffer->frame_size;
    
    return (aligned_abs_offset + size <= frame_end);
}

/* ============================================================================
 * BUFFER SUB-ALLOCATOR IMPLEMENTATION
 * ============================================================================ */

#define MAX_SUBALLOCATIONS 512

typedef struct buffer_suballocation {
    size_t offset;
    size_t size;
    bool in_use;
} buffer_suballocation_t;

struct metal_buffer_allocator {
    metal_device_t* device;
    metal_buffer_t* backing_buffer;
    
    buffer_suballocation_t suballocations[MAX_SUBALLOCATIONS];
    uint32_t suballocation_count;
    
    size_t pool_size;
    size_t allocated_bytes;
    metal_storage_mode_t storage_mode;
    
    pthread_mutex_t mutex;
};

metal_buffer_allocator_t* metal_buffer_allocator_create(metal_device_t* device, 
                                                         size_t pool_size,
                                                         metal_storage_mode_t storage_mode) {
    if (!device || pool_size == 0) {
        return NULL;
    }
    
    metal_buffer_allocator_t* allocator = (metal_buffer_allocator_t*)calloc(1, sizeof(metal_buffer_allocator_t));
    if (!allocator) {
        return NULL;
    }
    
    // Create backing buffer
    metal_buffer_desc_t desc = {
        .size = pool_size,
        .storage_mode = storage_mode,
        .usage = METAL_BUFFER_USAGE_VERTEX | METAL_BUFFER_USAGE_INDEX | METAL_BUFFER_USAGE_UNIFORM,
        .initial_data = NULL,
        .label = "Sub-Allocator Pool"
    };
    
    allocator->backing_buffer = metal_buffer_create(device, &desc);
    if (!allocator->backing_buffer) {
        free(allocator);
        return NULL;
    }
    
    allocator->device = device;
    allocator->pool_size = pool_size;
    allocator->allocated_bytes = 0;
    allocator->storage_mode = storage_mode;
    allocator->suballocation_count = 0;
    
    pthread_mutex_init(&allocator->mutex, NULL);
    
    return allocator;
}

void metal_buffer_allocator_destroy(metal_buffer_allocator_t* allocator) {
    if (!allocator) {
        return;
    }
    
    if (allocator->backing_buffer) {
        metal_buffer_destroy(allocator->backing_buffer);
    }
    
    pthread_mutex_destroy(&allocator->mutex);
    free(allocator);
}

metal_buffer_t* metal_buffer_suballocate(metal_buffer_allocator_t* allocator, 
                                         size_t size, 
                                         size_t alignment) {
    if (!allocator || size == 0) {
        return NULL;
    }
    
    pthread_mutex_lock(&allocator->mutex);
    
    // Align size
    size_t aligned_size = align_size(size, alignment);
    
    // Try to find free space using first-fit strategy
    size_t current_offset = 0;
    
    // Sort suballocations by offset (simple bubble sort for now)
    for (uint32_t i = 0; i < allocator->suballocation_count; i++) {
        for (uint32_t j = i + 1; j < allocator->suballocation_count; j++) {
            if (allocator->suballocations[j].offset < allocator->suballocations[i].offset) {
                buffer_suballocation_t temp = allocator->suballocations[i];
                allocator->suballocations[i] = allocator->suballocations[j];
                allocator->suballocations[j] = temp;
            }
        }
    }
    
    // Find gap that fits
    size_t allocation_offset = 0;
    bool found = false;
    
    for (uint32_t i = 0; i < allocator->suballocation_count; i++) {
        if (allocator->suballocations[i].in_use) {
            size_t aligned_current = align_size(current_offset, alignment);
            size_t gap = allocator->suballocations[i].offset - aligned_current;
            
            if (gap >= aligned_size) {
                allocation_offset = aligned_current;
                found = true;
                break;
            }
            
            current_offset = allocator->suballocations[i].offset + allocator->suballocations[i].size;
        }
    }
    
    // Check if we can allocate at the end
    if (!found) {
        allocation_offset = align_size(current_offset, alignment);
        if (allocation_offset + aligned_size <= allocator->pool_size) {
            found = true;
        }
    }
    
    if (!found) {
        pthread_mutex_unlock(&allocator->mutex);
        return NULL;  // Out of space
    }
    
    // Check if we have room for tracking
    if (allocator->suballocation_count >= MAX_SUBALLOCATIONS) {
        pthread_mutex_unlock(&allocator->mutex);
        return NULL;
    }
    
    // Create wrapper buffer
    metal_buffer_t* buffer = (metal_buffer_t*)calloc(1, sizeof(metal_buffer_t));
    if (!buffer) {
        return NULL;
    }
    
    // Point to offset in backing buffer
    buffer->buffer = allocator->backing_buffer->buffer;
    buffer->storage_mode = allocator->storage_mode;
    buffer->usage = allocator->backing_buffer->usage;
    buffer->size = aligned_size;
    
    if (allocator->storage_mode == METAL_STORAGE_SHARED) {
        buffer->mapped_ptr = (uint8_t*)allocator->backing_buffer->mapped_ptr + allocation_offset;
        buffer->is_mapped = true;
    }
    
    // Track suballocation
    buffer_suballocation_t* suballoc = &allocator->suballocations[allocator->suballocation_count++];
    suballoc->offset = allocation_offset;
    suballoc->size = aligned_size;
    suballoc->in_use = true;
    
    allocator->allocated_bytes += aligned_size;
    
    pthread_mutex_unlock(&allocator->mutex);
    return buffer;
}

void metal_buffer_subfree(metal_buffer_allocator_t* allocator, metal_buffer_t* buffer) {
    if (!allocator || !buffer) {
        return;
    }
    
    pthread_mutex_lock(&allocator->mutex);
    
    // Find matching suballocation
    for (uint32_t i = 0; i < allocator->suballocation_count; i++) {
        if (allocator->suballocations[i].in_use &&
            allocator->suballocations[i].size == buffer->size) {
            
            // Mark as free
            allocator->suballocations[i].in_use = false;
            allocator->allocated_bytes -= buffer->size;
            
            // Free buffer wrapper (not the backing buffer!)
            free(buffer);
            pthread_mutex_unlock(&allocator->mutex);
            return;
        }
    }
    pthread_mutex_unlock(&allocator->mutex);
}

void metal_buffer_allocator_reset(metal_buffer_allocator_t* allocator) {
    if (!allocator) {
        return;
    }
    
    pthread_mutex_lock(&allocator->mutex);
    allocator->suballocation_count = 0;
    allocator->allocated_bytes = 0;
    pthread_mutex_unlock(&allocator->mutex);
}

metal_buffer_allocator_stats_t metal_buffer_allocator_get_stats(metal_buffer_allocator_t* allocator) {
    metal_buffer_allocator_stats_t stats = {0};
    if (!allocator) {
        return stats;
    }
    
    pthread_mutex_lock(&allocator->mutex);
    
    stats.pool_size = allocator->pool_size;
    stats.allocated_bytes = allocator->allocated_bytes;
    stats.free_bytes = allocator->pool_size - allocator->allocated_bytes;
    stats.allocation_count = allocator->suballocation_count;
    
    // Calculate fragmentation (simple metric: number of free gaps)
    uint32_t gaps = 0;
    for (uint32_t i = 0; i < allocator->suballocation_count - 1; i++) {
        if (!allocator->suballocations[i].in_use) {
            gaps++;
        }
    }
    
    if (allocator->suballocation_count > 0) {
        stats.fragmentation = (double)gaps / (double)allocator->suballocation_count;
    }
    
    pthread_mutex_unlock(&allocator->mutex);
    return stats;
}

/* ============================================================================
 * ASYNC BUFFER OPERATIONS
 * ============================================================================ */

// Global async command queue (initialized externally)
static id<MTLCommandQueue> g_async_command_queue = nil;

void metal_buffer_upload_async(metal_buffer_t* buffer, 
                               const void* data, 
                               size_t size, 
                               size_t offset,
                               metal_buffer_upload_callback_t callback, 
                               void* user_data) {
    if (!buffer || !data || size == 0 || !g_async_command_queue) {
        if (callback) callback(user_data);
        return;
    }
    
    // For shared buffers, we can just do direct copy
    if (buffer->storage_mode == METAL_STORAGE_SHARED) {
        metal_buffer_update(buffer, data, size, offset);
        if (callback) callback(user_data);
        return;
    }
    
    // For private buffers, use blit encoder with staging buffer
    id<MTLCommandBuffer> cmd_buffer = [g_async_command_queue commandBuffer];
    
    // Create staging buffer
    id<MTLBuffer> staging = [buffer->buffer.device newBufferWithBytes:data
                                                              length:size
                                                             options:MTLResourceStorageModeShared];
    
    // Blit from staging to target
    id<MTLBlitCommandEncoder> blit = [cmd_buffer blitCommandEncoder];
    [blit copyFromBuffer:staging
            sourceOffset:0
                toBuffer:buffer->buffer
       destinationOffset:offset
                    size:size];
    [blit endEncoding];
    
    // Add completion handler
    if (callback) {
        [cmd_buffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull) {
            callback(user_data);
        }];
    }
    
    [cmd_buffer commit];
}

void metal_buffer_copy_async(metal_buffer_t* src,
                             metal_buffer_t* dst,
                             size_t src_offset,
                             size_t dst_offset,
                             size_t size,
                             metal_buffer_upload_callback_t callback,
                             void* user_data) {
    if (!src || !dst || size == 0 || !g_async_command_queue) {
        if (callback) callback(user_data);
        return;
    }
    
    id<MTLCommandBuffer> cmd_buffer = [g_async_command_queue commandBuffer];
    id<MTLBlitCommandEncoder> blit = [cmd_buffer blitCommandEncoder];
    
    [blit copyFromBuffer:src->buffer
            sourceOffset:src_offset
                toBuffer:dst->buffer
       destinationOffset:dst_offset
                    size:size];
    [blit endEncoding];
    
    if (callback) {
        [cmd_buffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull) {
            callback(user_data);
        }];
    }
    
    [cmd_buffer commit];
}

void metal_buffer_wait_async_operations(void) {
    // Wait for all async operations to complete
    if (g_async_command_queue) {
        id<MTLCommandBuffer> fence_buffer = [g_async_command_queue commandBuffer];
        [fence_buffer commit];
        [fence_buffer waitUntilCompleted];
    }
}

/* End of mtl_buffer.c */
