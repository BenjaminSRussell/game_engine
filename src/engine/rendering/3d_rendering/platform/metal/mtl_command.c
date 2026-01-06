/*
 * mtl_command.c
 * Metal command encoding implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "mtl_command.h"
#include <stdio.h>

#if defined(__OBJC__)
#import <Metal/Metal.h>

// Helper to cast void* back to id<Protocol>
#define TO_CMD_BUF(x) ((__bridge id<MTLCommandBuffer>)(x))
#define TO_RENDER_ENC(x) ((__bridge id<MTLRenderCommandEncoder>)(x))
#define TO_COMPUTE_ENC(x) ((__bridge id<MTLComputeCommandEncoder>)(x))
#define TO_ENC(x) ((__bridge id<MTLCommandEncoder>)(x))
#define TO_PASS_DESC(x) ((__bridge MTLRenderPassDescriptor*)(x))

#endif

void metal_command_buffer_commit(mtl_command_buffer_t buffer) {
#if defined(__OBJC__)
    if (buffer) {
        [TO_CMD_BUF(buffer) commit];
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

void metal_command_buffer_enqueue(mtl_command_buffer_t buffer) {
#if defined(__OBJC__)
    if (buffer) {
        [TO_CMD_BUF(buffer) enqueue];
    }
#endif
}

mtl_render_command_encoder_t metal_command_encoder_render_create(mtl_command_buffer_t buffer, void* pass_descriptor) {
#if defined(__OBJC__)
    if (!buffer || !pass_descriptor) return NULL;
    
    id<MTLRenderCommandEncoder> encoder = [TO_CMD_BUF(buffer) renderCommandEncoderWithDescriptor:TO_PASS_DESC(pass_descriptor)];
    return (__bridge void*)encoder;
#else
    return NULL;
#endif
}

mtl_compute_command_encoder_t metal_command_encoder_compute_create(mtl_command_buffer_t buffer) {
#if defined(__OBJC__)
    if (!buffer) return NULL;
    
    id<MTLComputeCommandEncoder> encoder = [TO_CMD_BUF(buffer) computeCommandEncoder];
    return (__bridge void*)encoder;
#else
    return NULL;
#endif
}

void metal_command_encoder_end(void* encoder) {
#if defined(__OBJC__)
    if (encoder) {
        [TO_ENC(encoder) endEncoding];
    }
#endif
}

void metal_command_buffer_push_debug_group(mtl_command_buffer_t buffer, const char* label) {
#if defined(__OBJC__)
    if (buffer && label) {
        NSString* str = [NSString stringWithUTF8String:label];
        [TO_CMD_BUF(buffer) pushDebugGroup:str];
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
