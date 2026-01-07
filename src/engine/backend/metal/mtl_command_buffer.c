/*
 * mtl_command_buffer.c
 * Metal command buffer lifecycle implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "backend/metal/mtl_command_buffer.h"
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
