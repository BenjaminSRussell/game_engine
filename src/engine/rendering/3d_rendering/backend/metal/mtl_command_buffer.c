/*
 * mtl_command_buffer.c
 * Metal command buffer lifecycle implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "mtl_command_buffer.h"
#include <stdio.h>

#if defined(__OBJC__)
#import <Metal/Metal.h>

/* Internal Helper to bridge cast */
#define TO_CMD_BUF(x) ((__bridge id<MTLCommandBuffer>)(x))

#endif

mtl_command_buffer_t metal_command_buffer_create(metal_device_t* device) {
#if defined(__OBJC__)
    return metal_create_command_buffer(device); // Re-use the device method
#else
    return NULL;
#endif
}

void metal_command_buffer_set_label(mtl_command_buffer_t buffer, const char* label) {
#if defined(__OBJC__)
    if (buffer && label) {
        [TO_CMD_BUF(buffer) setLabel:[NSString stringWithUTF8String:label]];
    }
#endif
}

void metal_command_buffer_enqueue(mtl_command_buffer_t buffer) {
#if defined(__OBJC__)
    if (buffer) {
        [TO_CMD_BUF(buffer) enqueue];
    }
#endif
}

void metal_command_buffer_commit(mtl_command_buffer_t buffer) {
#if defined(__OBJC__)
    if (buffer) {
        [TO_CMD_BUF(buffer) commit];
    }
#endif
}

void metal_command_buffer_wait_until_scheduled(mtl_command_buffer_t buffer) {
#if defined(__OBJC__)
    if (buffer) {
        [TO_CMD_BUF(buffer) waitUntilScheduled];
    }
#endif
}

void metal_command_buffer_wait_until_completed(mtl_command_buffer_t buffer) {
#if defined(__OBJC__)
    if (buffer) {
        [TO_CMD_BUF(buffer) waitUntilCompleted];
    }
#endif
}

void metal_command_buffer_push_debug_group(mtl_command_buffer_t buffer, const char* label) {
#if defined(__OBJC__)
    if (buffer && label) {
        [TO_CMD_BUF(buffer) pushDebugGroup:[NSString stringWithUTF8String:label]];
    }
#endif
}

void metal_command_buffer_pop_debug_group(mtl_command_buffer_t buffer) {
#if defined(__OBJC__)
    if (buffer) {
        [TO_CMD_BUF(buffer) popDebugGroup];
    }
#endif
}

mtl_command_buffer_status_t metal_command_buffer_get_status(mtl_command_buffer_t buffer) {
#if defined(__OBJC__)
    if (!buffer) return MTL_COMMAND_BUFFER_STATUS_ERROR;
    
    MTLCommandBufferStatus status = [TO_CMD_BUF(buffer) status];
    switch (status) {
        case MTLCommandBufferStatusNotEnqueued: return MTL_COMMAND_BUFFER_STATUS_NOT_ENQUEUED;
        case MTLCommandBufferStatusEnqueued: return MTL_COMMAND_BUFFER_STATUS_ENQUEUED;
        case MTLCommandBufferStatusCommitted: return MTL_COMMAND_BUFFER_STATUS_COMMITTED;
        case MTLCommandBufferStatusScheduled: return MTL_COMMAND_BUFFER_STATUS_SCHEDULED;
        case MTLCommandBufferStatusCompleted: return MTL_COMMAND_BUFFER_STATUS_COMPLETED;
        case MTLCommandBufferStatusError: return MTL_COMMAND_BUFFER_STATUS_ERROR;
        default: return MTL_COMMAND_BUFFER_STATUS_ERROR;
    }
#else
    return MTL_COMMAND_BUFFER_STATUS_ERROR;
#endif
}

mtl_command_buffer_error_t metal_command_buffer_get_error(mtl_command_buffer_t buffer) {
#if defined(__OBJC__)
    if (!buffer) return MTL_COMMAND_BUFFER_ERROR_NONE;
    
    NSError *error = [TO_CMD_BUF(buffer) error];
    if (!error) return MTL_COMMAND_BUFFER_ERROR_NONE;
    
    /* Simplified mapping - can be expanded based on error domain/code */
    return MTL_COMMAND_BUFFER_ERROR_INTERNAL;
#else
    return MTL_COMMAND_BUFFER_ERROR_NONE;
#endif
}

const char* metal_command_buffer_get_error_string(mtl_command_buffer_t buffer) {
#if defined(__OBJC__)
    if (!buffer) return "NULL Buffer";
    
    NSError *error = [TO_CMD_BUF(buffer) error];
    if (!error) return "No Error";
    
    return [[error localizedDescription] UTF8String];
#else
    return "Not Implemented";
#endif
}

void metal_command_buffer_add_scheduled_handler(mtl_command_buffer_t buffer, mtl_command_buffer_handler_t handler) {
#if defined(__OBJC__)
    if (buffer && handler) {
        [TO_CMD_BUF(buffer) addScheduledHandler:^(id<MTLCommandBuffer> buf) {
             handler((__bridge mtl_command_buffer_t)buf);
        }];
    }
#endif
}

void metal_command_buffer_add_completed_handler(mtl_command_buffer_t buffer, mtl_command_buffer_handler_t handler) {
#if defined(__OBJC__)
    if (buffer && handler) {
        [TO_CMD_BUF(buffer) addCompletedHandler:^(id<MTLCommandBuffer> buf) {
             handler((__bridge mtl_command_buffer_t)buf);
        }];
    }
#endif
}

double metal_command_buffer_get_gpu_start_time(mtl_command_buffer_t buffer) {
#if defined(__OBJC__)
    if (buffer) {
        return [TO_CMD_BUF(buffer) GPUStartTime];
    }
#endif
    return 0.0;
}

double metal_command_buffer_get_gpu_end_time(mtl_command_buffer_t buffer) {
#if defined(__OBJC__)
    if (buffer) {
        return [TO_CMD_BUF(buffer) GPUEndTime];
    }
#endif
    return 0.0;
}

double metal_command_buffer_get_kernel_start_time(mtl_command_buffer_t buffer) {
#if defined(__OBJC__)
    if (buffer) {
        return [TO_CMD_BUF(buffer) kernelStartTime];
    }
#endif
    return 0.0;
}

double metal_command_buffer_get_kernel_end_time(mtl_command_buffer_t buffer) {
#if defined(__OBJC__)
    if (buffer) {
        return [TO_CMD_BUF(buffer) kernelEndTime];
    }
#endif
    return 0.0;
}

/* ============================================================================
 * COMMAND BUFFER POOLING
 * ============================================================================ */

#if defined(__OBJC__)

struct mtl_command_buffer_pool {
    metal_device_t* device;
    NSMutableArray* available_buffers;
    NSMutableArray* in_use_buffers;
    unsigned long max_count;
    unsigned long total_created;
};

#endif

mtl_command_buffer_pool_t metal_command_buffer_pool_create(metal_device_t* device, unsigned long max_count) {
#if defined(__OBJC__)
    if (!device) return NULL;
    
    struct mtl_command_buffer_pool* pool = malloc(sizeof(struct mtl_command_buffer_pool));
    if (!pool) return NULL;
    
    pool->device = device;
    pool->available_buffers = [[NSMutableArray alloc] init];
    pool->in_use_buffers = [[NSMutableArray alloc] init];
    pool->max_count = max_count;
    pool->total_created = 0;
    
    return pool;
#else
    return NULL;
#endif
}

void metal_command_buffer_pool_destroy(mtl_command_buffer_pool_t pool) {
#if defined(__OBJC__)
    if (pool) {
        [pool->available_buffers removeAllObjects];
        [pool->in_use_buffers removeAllObjects];
        free(pool);
    }
#endif
}

mtl_command_buffer_t metal_command_buffer_pool_acquire(mtl_command_buffer_pool_t pool) {
#if defined(__OBJC__)
    if (!pool) return NULL;
    
    // Try to reuse a completed buffer
    if ([pool->available_buffers count] > 0) {
        id<MTLCommandBuffer> buffer = [pool->available_buffers lastObject];
        [pool->available_buffers removeLastObject];
        [pool->in_use_buffers addObject:buffer];
        return (__bridge mtl_command_buffer_t)buffer;
    }
    
    // Create a new buffer if under limit
    if (pool->max_count == 0 || pool->total_created < pool->max_count) {
        mtl_command_buffer_t new_buffer = metal_command_buffer_create(pool->device);
        if (new_buffer) {
            pool->total_created++;
            [pool->in_use_buffers addObject:(__bridge id)new_buffer];
        }
        return new_buffer;
    }
    
    // Pool exhausted, return NULL
    return NULL;
#else
    return NULL;
#endif
}

void metal_command_buffer_pool_release(mtl_command_buffer_pool_t pool, mtl_command_buffer_t buffer) {
#if defined(__OBJC__)
    if (pool && buffer) {
        id<MTLCommandBuffer> cmd_buf = TO_CMD_BUF(buffer);
        
        // Remove from in-use list
        [pool->in_use_buffers removeObject:cmd_buf];
        
        // Only add to available if status is completed
        if ([cmd_buf status] == MTLCommandBufferStatusCompleted) {
            [pool->available_buffers addObject:cmd_buf];
        }
    }
#endif
}

/* ============================================================================
 * TRIPLE BUFFERING & FRAME SYNCHRONIZATION
 * ============================================================================ */

#if defined(__OBJC__)
#import <dispatch/dispatch.h>

struct mtl_frame_sync {
    dispatch_semaphore_t semaphore;
    unsigned int current_frame_index;
};

#endif

mtl_frame_sync_t metal_frame_sync_create(metal_device_t* device) {
#if defined(__OBJC__)
    struct mtl_frame_sync* sync = malloc(sizeof(struct mtl_frame_sync));
    if (!sync) return NULL;
    
    // Create a semaphore with initial value of 3 (triple buffering)
    sync->semaphore = dispatch_semaphore_create(3);
    sync->current_frame_index = 0;
    
    return sync;
#else
    return NULL;
#endif
}

void metal_frame_sync_destroy(mtl_frame_sync_t sync) {
#if defined(__OBJC__)
    if (sync) {
        // Release resources
        free(sync);
    }
#endif
}

void metal_frame_sync_wait_for_next_frame(mtl_frame_sync_t sync) {
#if defined(__OBJC__)
    if (sync) {
        // Wait for a frame slot to become available
        dispatch_semaphore_wait(sync->semaphore, DISPATCH_TIME_FOREVER);
        
        // Advance frame index (0 -> 1 -> 2 -> 0)
        sync->current_frame_index = (sync->current_frame_index + 1) % 3;
    }
#endif
}

void metal_frame_sync_signal_frame_complete(mtl_frame_sync_t sync) {
#if defined(__OBJC__)
    if (sync) {
        // Signal that a frame slot is available
        dispatch_semaphore_signal(sync->semaphore);
    }
#endif
}

unsigned int metal_frame_sync_get_current_frame_index(mtl_frame_sync_t sync) {
#if defined(__OBJC__)
    if (sync) {
        return sync->current_frame_index;
    }
#endif
    return 0;
}

/* ============================================================================
 * PRESENTATION TIMING
 * ============================================================================ */

void metal_command_buffer_present_drawable(mtl_command_buffer_t buffer, void* drawable) {
#if defined(__OBJC__)
    if (buffer && drawable) {
        id<CAMetalDrawable> metal_drawable = (__bridge id<CAMetalDrawable>)drawable;
        [TO_CMD_BUF(buffer) presentDrawable:metal_drawable];
    }
#endif
}

void metal_command_buffer_present_drawable_at_time(mtl_command_buffer_t buffer, void* drawable, double presentation_time) {
#if defined(__OBJC__)
    if (buffer && drawable) {
        id<CAMetalDrawable> metal_drawable = (__bridge id<CAMetalDrawable>)drawable;
        [TO_CMD_BUF(buffer) presentDrawable:metal_drawable atTime:presentation_time];
    }
#endif
}

void metal_command_buffer_present_drawable_after_minimum_duration(mtl_command_buffer_t buffer, void* drawable, double minimum_duration) {
#if defined(__OBJC__)
    if (buffer && drawable) {
        id<CAMetalDrawable> metal_drawable = (__bridge id<CAMetalDrawable>)drawable;
        [TO_CMD_BUF(buffer) presentDrawable:metal_drawable afterMinimumDuration:minimum_duration];
    }
#endif
}

/* ============================================================================
 * VALIDATION & RETRY
 * ============================================================================ */

bool metal_command_buffer_validate(mtl_command_buffer_t buffer) {
#if defined(__OBJC__)
    if (!buffer) return false;
    
    // Check buffer status
    MTLCommandBufferStatus status = [TO_CMD_BUF(buffer) status];
    
    // Buffer should be in a valid state for encoding
    if (status != MTLCommandBufferStatusNotEnqueued && 
        status != MTLCommandBufferStatusEnqueued) {
        return false;
    }
    
    // Additional validation could check:
    // - That at least one encoder was created
    // - That all encoders were properly ended
    // - That resources are still valid
    
    return true;
#else
    return false;
#endif
}

mtl_command_buffer_t metal_command_buffer_retry_on_error(mtl_command_buffer_t buffer, unsigned int max_retries) {
#if defined(__OBJC__)
    if (!buffer) return NULL;
    
    id<MTLCommandBuffer> cmd_buf = TO_CMD_BUF(buffer);
    
    // Check if buffer actually failed
    if ([cmd_buf status] != MTLCommandBufferStatusError) {
        return buffer; // Not an error, return as-is
    }
    
    NSError* error = [cmd_buf error];
    
    // Determine if error is retryable
    bool is_retryable = false;
    if (error) {
        // Check error domain and code
        if ([error.domain isEqualToString:@"MTLCommandBufferErrorDomain"]) {
            NSInteger code = error.code;
            // Timeout, page fault, and out of memory are potentially retryable
            if (code == 2 || code == 3 || code == 5) {
                is_retryable = true;
            }
        }
    }
    
    if (!is_retryable || max_retries == 0) {
        return NULL; // Can't or won't retry
    }
    
    // For a real retry, we would need to:
    // 1. Get the original command queue
    // 2. Create a new command buffer
    // 3. Re-encode all commands (requires storing commands or callback)
    //
    // This is simplified - in practice, you'd need command recording infrastructure
    
    // Get the device from the error buffer (simplified)
    // In a real implementation, you'd store the device/queue reference
    
    return NULL; // Retry not fully implemented (requires command recording)
#else
    return NULL;
#endif
}
