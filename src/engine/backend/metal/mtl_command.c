#import <Metal/Metal.h>
#include "mtl_command.h"

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================ */

/* We need to define the command buffer struct here or cast it.
   Assuming mtl_command_buffer_t is an opaque pointer to id<MTLCommandBuffer>
   or a wrapper struct. Based on previous files, it seems to be treated as void*.
   However, we need to treat it as id<MTLCommandBuffer> to call methods.
*/

/* ============================================================================
 * API IMPLEMENTATION
 * ============================================================================ */

void metal_command_buffer_commit(mtl_command_buffer_t buffer) {
    if (buffer) {
        id<MTLCommandBuffer> cmd_buf = (__bridge id<MTLCommandBuffer>)buffer;
        [cmd_buf commit];
    }
}

void metal_command_buffer_wait_until_completed(mtl_command_buffer_t buffer) {
    if (buffer) {
        id<MTLCommandBuffer> cmd_buf = (__bridge id<MTLCommandBuffer>)buffer;
        [cmd_buf waitUntilCompleted];
    }
}

void metal_command_buffer_enqueue(mtl_command_buffer_t buffer) {
    if (buffer) {
        id<MTLCommandBuffer> cmd_buf = (__bridge id<MTLCommandBuffer>)buffer;
        [cmd_buf enqueue];
    }
}

mtl_render_command_encoder_t metal_command_encoder_render_create(mtl_command_buffer_t buffer, void* pass_descriptor) {
    if (!buffer || !pass_descriptor) return NULL;
    
    id<MTLCommandBuffer> cmd_buf = (__bridge id<MTLCommandBuffer>)buffer;
    MTLRenderPassDescriptor* mtl_pass = (__bridge MTLRenderPassDescriptor*)pass_descriptor;
    
    return (__bridge_retained void*)[cmd_buf renderCommandEncoderWithDescriptor:mtl_pass];
}

mtl_compute_command_encoder_t metal_command_encoder_compute_create(mtl_command_buffer_t buffer) {
    if (!buffer) return NULL;
    
    id<MTLCommandBuffer> cmd_buf = (__bridge id<MTLCommandBuffer>)buffer;
    return (__bridge_retained void*)[cmd_buf computeCommandEncoder];
}

mtl_blit_command_encoder_t metal_command_encoder_blit_create(mtl_command_buffer_t buffer) {
    if (!buffer) return NULL;
    
    id<MTLCommandBuffer> cmd_buf = (__bridge id<MTLCommandBuffer>)buffer;
    return (__bridge_retained void*)[cmd_buf blitCommandEncoder];
}
