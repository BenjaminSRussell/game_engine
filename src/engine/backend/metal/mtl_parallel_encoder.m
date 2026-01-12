/*
 * mtl_parallel_encoder.c
 * Metal parallel render command encoder implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "mtl_parallel_encoder.h"

#if defined(__OBJC__)
#import <Metal/Metal.h>

#define TO_CMD_BUF(x) ((__bridge id<MTLCommandBuffer>)(x))
#define TO_PARALLEL_ENC(x) ((__bridge id<MTLParallelRenderCommandEncoder>)(x))
#define TO_PASS_DESC(x) ((__bridge MTLRenderPassDescriptor*)(x))

#endif

mtl_parallel_render_command_encoder_t metal_parallel_render_command_encoder_create(mtl_command_buffer_t buffer, void* pass_descriptor) {
#if defined(__OBJC__)
    if (!buffer || !pass_descriptor) return NULL;
    id<MTLParallelRenderCommandEncoder> encoder = [TO_CMD_BUF(buffer) parallelRenderCommandEncoderWithDescriptor:TO_PASS_DESC(pass_descriptor)];
    return (__bridge void*)encoder;
#else
    return NULL;
#endif
}

mtl_render_command_encoder_t metal_parallel_render_encoder_create_command_encoder(mtl_parallel_render_command_encoder_t parallel_encoder) {
#if defined(__OBJC__)
    if (!parallel_encoder) return NULL;
    id<MTLRenderCommandEncoder> encoder = [TO_PARALLEL_ENC(parallel_encoder) renderCommandEncoder];
    return (__bridge void*)encoder;
#else
    return NULL;
#endif
}

void metal_parallel_render_encoder_end_encoding(mtl_parallel_render_command_encoder_t parallel_encoder) {
#if defined(__OBJC__)
    if (parallel_encoder) {
        [TO_PARALLEL_ENC(parallel_encoder) endEncoding];
    }
#endif
}
