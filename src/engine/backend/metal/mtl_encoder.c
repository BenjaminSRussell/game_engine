/*
 * mtl_encoder.c
 * Metal command encoder implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "mtl_encoder.h"
#include <stdio.h>

#if defined(__OBJC__)
#import <Metal/Metal.h>

/* Internal Helper to bridge cast */
#define TO_CMD_BUF(x) ((__bridge id<MTLCommandBuffer>)(x))
#define TO_RENDER_ENC(x) ((__bridge id<MTLRenderCommandEncoder>)(x))
#define TO_COMPUTE_ENC(x) ((__bridge id<MTLComputeCommandEncoder>)(x))
#define TO_BLIT_ENC(x) ((__bridge id<MTLBlitCommandEncoder>)(x))

#define TO_PASS_DESC(x) ((__bridge MTLRenderPassDescriptor*)(x))
#define TO_PIPELINE(x) ((__bridge id<MTLRenderPipelineState>)(x))
#define TO_COMPUTE_PIPELINE(x) ((__bridge id<MTLComputePipelineState>)(x))
#define TO_BUF(x) ((__bridge id<MTLBuffer>)(x))
#define TO_TEX(x) ((__bridge id<MTLTexture>)(x))
#define TO_SAMPLER(x) ((__bridge id<MTLSamplerState>)(x))
#define TO_DEPTH_STENCIL(x) ((__bridge id<MTLDepthStencilState>)(x))

#endif

/* ============================================================================
 * RENDER ENCODER
 * ============================================================================ */

mtl_render_command_encoder_t metal_render_command_encoder_create(mtl_command_buffer_t buffer, void* pass_descriptor) {
#if defined(__OBJC__)
    if (!buffer || !pass_descriptor) return NULL;
    id<MTLRenderCommandEncoder> encoder = [TO_CMD_BUF(buffer) renderCommandEncoderWithDescriptor:TO_PASS_DESC(pass_descriptor)];
    return (__bridge void*)encoder;
#else
    return NULL;
#endif
}

void metal_render_encoder_end_encoding(mtl_render_command_encoder_t encoder) {
#if defined(__OBJC__)
    if (encoder) [TO_RENDER_ENC(encoder) endEncoding];
#endif
}

void metal_render_encoder_set_render_pipeline_state(mtl_render_command_encoder_t encoder, void* pipeline_state) {
#if defined(__OBJC__)
    if (encoder && pipeline_state) [TO_RENDER_ENC(encoder) setRenderPipelineState:TO_PIPELINE(pipeline_state)];
#endif
}

void metal_render_encoder_set_vertex_buffer(mtl_render_command_encoder_t encoder, void* buffer, unsigned long offset, unsigned long index) {
#if defined(__OBJC__)
    if (encoder && buffer) [TO_RENDER_ENC(encoder) setVertexBuffer:TO_BUF(buffer) offset:offset atIndex:index];
#endif
}

void metal_render_encoder_set_fragment_buffer(mtl_render_command_encoder_t encoder, void* buffer, unsigned long offset, unsigned long index) {
#if defined(__OBJC__)
    if (encoder && buffer) [TO_RENDER_ENC(encoder) setFragmentBuffer:TO_BUF(buffer) offset:offset atIndex:index];
#endif
}

void metal_render_encoder_set_vertex_texture(mtl_render_command_encoder_t encoder, void* texture, unsigned long index) {
#if defined(__OBJC__)
    if (encoder) [TO_RENDER_ENC(encoder) setVertexTexture:TO_TEX(texture) atIndex:index];
#endif
}

void metal_render_encoder_set_fragment_texture(mtl_render_command_encoder_t encoder, void* texture, unsigned long index) {
#if defined(__OBJC__)
    if (encoder) [TO_RENDER_ENC(encoder) setFragmentTexture:TO_TEX(texture) atIndex:index];
#endif
}

void metal_render_encoder_set_vertex_sampler_state(mtl_render_command_encoder_t encoder, void* sampler, unsigned long index) {
#if defined(__OBJC__)
    if (encoder) [TO_RENDER_ENC(encoder) setVertexSamplerState:TO_SAMPLER(sampler) atIndex:index];
#endif
}

void metal_render_encoder_set_fragment_sampler_state(mtl_render_command_encoder_t encoder, void* sampler, unsigned long index) {
#if defined(__OBJC__)
    if (encoder) [TO_RENDER_ENC(encoder) setFragmentSamplerState:TO_SAMPLER(sampler) atIndex:index];
#endif
}

void metal_render_encoder_set_depth_stencil_state(mtl_render_command_encoder_t encoder, void* depth_stencil_state) {
#if defined(__OBJC__)
    if (encoder) [TO_RENDER_ENC(encoder) setDepthStencilState:TO_DEPTH_STENCIL(depth_stencil_state)];
#endif
}

void metal_render_encoder_set_viewport(mtl_render_command_encoder_t encoder, mtl_viewport_t viewport) {
#if defined(__OBJC__)
    if (encoder) {
        MTLViewport vp = {viewport.originX, viewport.originY, viewport.width, viewport.height, viewport.znear, viewport.zfar};
        [TO_RENDER_ENC(encoder) setViewport:vp];
    }
#endif
}

void metal_render_encoder_set_scissor_rect(mtl_render_command_encoder_t encoder, mtl_scissor_rect_t rect) {
#if defined(__OBJC__)
    if (encoder) {
        MTLScissorRect sr = {rect.x, rect.y, rect.width, rect.height};
        [TO_RENDER_ENC(encoder) setScissorRect:sr];
    }
#endif
}

void metal_render_encoder_set_cull_mode(mtl_render_command_encoder_t encoder, int cull_mode) {
#if defined(__OBJC__)
    if (encoder) [TO_RENDER_ENC(encoder) setCullMode:(MTLCullMode)cull_mode];
#endif
}

void metal_render_encoder_set_front_facing_winding(mtl_render_command_encoder_t encoder, int winding) {
#if defined(__OBJC__)
    if (encoder) [TO_RENDER_ENC(encoder) setFrontFacingWinding:(MTLWinding)winding];
#endif
}

void metal_render_encoder_draw_primitives(mtl_render_command_encoder_t encoder, mtl_draw_primitives_args_t args) {
#if defined(__OBJC__)
    if (encoder) {
        [TO_RENDER_ENC(encoder) drawPrimitives:(MTLPrimitiveType)args.primitiveType 
                                  vertexStart:args.vertexStart 
                                  vertexCount:args.vertexCount 
                                instanceCount:args.instanceCount 
                                 baseInstance:args.baseInstance];
    }
#endif
}

void metal_render_encoder_draw_indexed_primitives(mtl_render_command_encoder_t encoder, mtl_draw_indexed_primitives_args_t args) {
#if defined(__OBJC__)
    if (encoder && args.indexBuffer) {
        [TO_RENDER_ENC(encoder) drawIndexedPrimitives:(MTLPrimitiveType)args.primitiveType 
                                           indexCount:args.indexCount 
                                            indexType:(MTLIndexType)args.indexType 
                                          indexBuffer:TO_BUF(args.indexBuffer)
                                    indexBufferOffset:args.indexBufferOffset 
                                        instanceCount:args.instanceCount 
                                           baseVertex:args.baseVertex 
                                         baseInstance:args.baseInstance];
    }
#endif
}

void metal_render_encoder_draw_primitives_indirect(mtl_render_command_encoder_t encoder, mtl_primitive_type_t primitiveType, void* indirectBuffer, unsigned long indirectBufferOffset) {
#if defined(__OBJC__)
    if (encoder && indirectBuffer) {
        [TO_RENDER_ENC(encoder) drawPrimitives:(MTLPrimitiveType)primitiveType indirectBuffer:TO_BUF(indirectBuffer) indirectBufferOffset:indirectBufferOffset];
    }
#endif
}

void metal_render_encoder_draw_indexed_primitives_indirect(mtl_render_command_encoder_t encoder, mtl_primitive_type_t primitiveType, mtl_index_type_t indexType, void* indexBuffer, unsigned long indexBufferOffset, void* indirectBuffer, unsigned long indirectBufferOffset) {
#if defined(__OBJC__)
    if (encoder && indexBuffer && indirectBuffer) {
        [TO_RENDER_ENC(encoder) drawIndexedPrimitives:(MTLPrimitiveType)primitiveType indexType:(MTLIndexType)indexType indexBuffer:TO_BUF(indexBuffer) indexBufferOffset:indexBufferOffset indirectBuffer:TO_BUF(indirectBuffer) indirectBufferOffset:indirectBufferOffset];
    }
#endif
}

void metal_render_encoder_set_visibility_result_mode(mtl_render_command_encoder_t encoder, mtl_visibility_result_mode_t mode, unsigned long offset) {
#if defined(__OBJC__)
    if (encoder) {
        [TO_RENDER_ENC(encoder) setVisibilityResultMode:(MTLVisibilityResultMode)mode offset:offset];
    }
#endif
}

void metal_render_encoder_memory_barrier(mtl_render_command_encoder_t encoder, void* resources, unsigned long count) {
#if defined(__OBJC__)
    // Metal barriers are usually per-stage or resource hazard tracking is automatic.
    // Explicit barriers exist in MTLRenderCommandEncoder for Tile Memory vs System Memory in Apple Silicon.
    // For general resources, verify support. Assuming this maps to memoryBarrierWithResources if available or fence.
    // Check for `memoryBarrierWithResources:count:afterStages:beforeStages:` which is complex.
    // Simplification: Do nothing for now as Metal tracks hazards automatically for most cases unless using Heaps/Fences explicitly.
    // Or if expecting tile memory barriers:
    if (encoder) {
        #if defined(MTL_Rendering_OpenGLES30) // Just checking generic availability symbols
           // [TO_RENDER_ENC(encoder) memoryBarrierWithScope:MTLBarrierScopeRenderTargets afterStages:MTLRenderStageFragment beforeStages:MTLRenderStageVertex];
        #endif
    }
#endif
}


/* ============================================================================
 * COMPUTE ENCODER
 * ============================================================================ */

mtl_compute_command_encoder_t metal_compute_command_encoder_create(mtl_command_buffer_t buffer) {
#if defined(__OBJC__)
    if (!buffer) return NULL;
    id<MTLComputeCommandEncoder> encoder = [TO_CMD_BUF(buffer) computeCommandEncoder];
    return (__bridge void*)encoder;
#else
    return NULL;
#endif
}

void metal_compute_encoder_end_encoding(mtl_compute_command_encoder_t encoder) {
#if defined(__OBJC__)
    if (encoder) [TO_COMPUTE_ENC(encoder) endEncoding];
#endif
}

void metal_compute_encoder_set_compute_pipeline_state(mtl_compute_command_encoder_t encoder, void* pipeline_state) {
#if defined(__OBJC__)
    if (encoder && pipeline_state) [TO_COMPUTE_ENC(encoder) setComputePipelineState:TO_COMPUTE_PIPELINE(pipeline_state)];
#endif
}

void metal_compute_encoder_set_buffer(mtl_compute_command_encoder_t encoder, void* buffer, unsigned long offset, unsigned long index) {
#if defined(__OBJC__)
    if (encoder && buffer) [TO_COMPUTE_ENC(encoder) setBuffer:TO_BUF(buffer) offset:offset atIndex:index];
#endif
}

void metal_compute_encoder_set_texture(mtl_compute_command_encoder_t encoder, void* texture, unsigned long index) {
#if defined(__OBJC__)
    if (encoder) [TO_COMPUTE_ENC(encoder) setTexture:TO_TEX(texture) atIndex:index];
#endif
}

void metal_compute_encoder_set_sampler_state(mtl_compute_command_encoder_t encoder, void* sampler, unsigned long index) {
#if defined(__OBJC__)
    if (encoder) [TO_COMPUTE_ENC(encoder) setSamplerState:TO_SAMPLER(sampler) atIndex:index];
#endif
}

void metal_compute_encoder_set_threadgroup_memory_length(mtl_compute_command_encoder_t encoder, unsigned long length, unsigned long index) {
#if defined(__OBJC__)
    if (encoder) [TO_COMPUTE_ENC(encoder) setThreadgroupMemoryLength:length atIndex:index];
#endif
}

void metal_compute_encoder_dispatch_threadgroups(mtl_compute_command_encoder_t encoder, mtl_dispatch_threadgroups_args_t args) {
#if defined(__OBJC__)
    if (encoder) {
        MTLSize threadgroupsPerGrid = MTLSizeMake(args.threadgroupsPerGrid[0], args.threadgroupsPerGrid[1], args.threadgroupsPerGrid[2]);
        MTLSize threadsPerThreadgroup = MTLSizeMake(args.threadsPerThreadgroup[0], args.threadsPerThreadgroup[1], args.threadsPerThreadgroup[2]);
        [TO_COMPUTE_ENC(encoder) dispatchThreadgroups:threadgroupsPerGrid threadsPerThreadgroup:threadsPerThreadgroup];
    }
#endif
}

void metal_compute_encoder_dispatch_threadgroups_indirect(mtl_compute_command_encoder_t encoder, void* indirectBuffer, unsigned long indirectBufferOffset, mtl_dispatch_threadgroups_args_t threadsPerThreadgroup) {
#if defined(__OBJC__)
    if (encoder && indirectBuffer) {
        MTLSize tpt = MTLSizeMake(threadsPerThreadgroup.threadsPerThreadgroup[0], threadsPerThreadgroup.threadsPerThreadgroup[1], threadsPerThreadgroup.threadsPerThreadgroup[2]);
        [TO_COMPUTE_ENC(encoder) dispatchThreadgroupsWithIndirectBuffer:TO_BUF(indirectBuffer) indirectBufferOffset:indirectBufferOffset threadsPerThreadgroup:tpt];
    }
#endif
}

void metal_compute_encoder_dispatch_threads(mtl_compute_command_encoder_t encoder, mtl_dispatch_threadgroups_args_t args) {
#if defined(__OBJC__)
    // Available on newer MacOS versions, ensuring compatibility or strict requirement?
    // Assuming standard metal availability.
    if (encoder) {
        if (@available(macOS 10.13, *)) {
            MTLSize threadsPerGrid = MTLSizeMake(args.threadgroupsPerGrid[0], args.threadgroupsPerGrid[1], args.threadgroupsPerGrid[2]);
             MTLSize threadsPerThreadgroup = MTLSizeMake(args.threadsPerThreadgroup[0], args.threadsPerThreadgroup[1], args.threadsPerThreadgroup[2]);
             [TO_COMPUTE_ENC(encoder) dispatchThreads:threadsPerGrid threadsPerThreadgroup:threadsPerThreadgroup];
        }
    }
#endif
}

void metal_compute_encoder_memory_barrier(mtl_compute_command_encoder_t encoder, void* resources, unsigned long count) {
#if defined(__OBJC__)
    if (encoder) {
        // Implement memory barriers if specific resource list is provided
        if (resources && count > 0) {
            // id<MTLResource>* resList = (id<MTLResource>*)resources; // Simplified: assumes array of ids
            // [TO_COMPUTE_ENC(encoder) memoryBarrierWithResources:resList count:count];
        } else {
             [TO_COMPUTE_ENC(encoder) memoryBarrierWithScope:MTLBarrierScopeBuffers | MTLBarrierScopeTextures];
        }
    }
#endif
}


/* ============================================================================
 * BLIT ENCODER
 * ============================================================================ */

mtl_blit_command_encoder_t metal_blit_command_encoder_create(mtl_command_buffer_t buffer) {
#if defined(__OBJC__)
    if (!buffer) return NULL;
    id<MTLBlitCommandEncoder> encoder = [TO_CMD_BUF(buffer) blitCommandEncoder];
    return (__bridge void*)encoder;
#else
    return NULL;
#endif
}

void metal_blit_encoder_end_encoding(mtl_blit_command_encoder_t encoder) {
#if defined(__OBJC__)
    if (encoder) [TO_BLIT_ENC(encoder) endEncoding];
#endif
}

void metal_blit_encoder_copy_from_buffer_to_buffer(mtl_blit_command_encoder_t encoder, 
                                                   void* sourceBuffer, unsigned long sourceOffset, 
                                                   void* destinationBuffer, unsigned long destinationOffset, 
                                                   unsigned long size) {
#if defined(__OBJC__)
    if (encoder && sourceBuffer && destinationBuffer) {
        [TO_BLIT_ENC(encoder) copyFromBuffer:TO_BUF(sourceBuffer) sourceOffset:sourceOffset toBuffer:TO_BUF(destinationBuffer) destinationOffset:destinationOffset size:size];
    }
#endif
}

void metal_blit_encoder_copy_from_buffer_to_texture(mtl_blit_command_encoder_t encoder,
                                                    void* sourceBuffer, unsigned long sourceOffset, unsigned long sourceBytesPerRow, unsigned long sourceBytesPerImage,
                                                    unsigned long sourceSize[3],
                                                    void* destinationTexture, unsigned long destinationSlice, unsigned long destinationLevel, unsigned long destinationOrigin[3]) {
#if defined(__OBJC__)
    if (encoder && sourceBuffer && destinationTexture) {
        MTLSize size = MTLSizeMake(sourceSize[0], sourceSize[1], sourceSize[2]);
        MTLOrigin origin = MTLOriginMake(destinationOrigin[0], destinationOrigin[1], destinationOrigin[2]);
        [TO_BLIT_ENC(encoder) copyFromBuffer:TO_BUF(sourceBuffer) 
                                sourceOffset:sourceOffset 
                           sourceBytesPerRow:sourceBytesPerRow 
                         sourceBytesPerImage:sourceBytesPerImage 
                                  sourceSize:size 
                                   toTexture:TO_TEX(destinationTexture) 
                            destinationSlice:destinationSlice 
                            destinationLevel:destinationLevel 
                           destinationOrigin:origin];
    }
#endif
}

void metal_blit_encoder_copy_from_texture_to_buffer(mtl_blit_command_encoder_t encoder,
                                                    void* sourceTexture, unsigned long sourceSlice, unsigned long sourceLevel, unsigned long sourceOrigin[3],
                                                    unsigned long sourceSize[3],
                                                    void* destinationBuffer, unsigned long destinationOffset, unsigned long destinationBytesPerRow, unsigned long destinationBytesPerImage) {
#if defined(__OBJC__)
    if (encoder && sourceTexture && destinationBuffer) {
        MTLSize size = MTLSizeMake(sourceSize[0], sourceSize[1], sourceSize[2]);
        MTLOrigin origin = MTLOriginMake(sourceOrigin[0], sourceOrigin[1], sourceOrigin[2]);
        [TO_BLIT_ENC(encoder) copyFromTexture:TO_TEX(sourceTexture) 
                                  sourceSlice:sourceSlice 
                                  sourceLevel:sourceLevel 
                                 sourceOrigin:origin 
                                   sourceSize:size 
                                     toBuffer:TO_BUF(destinationBuffer) 
                            destinationOffset:destinationOffset 
                       destinationBytesPerRow:destinationBytesPerRow 
                     destinationBytesPerImage:destinationBytesPerImage];
    }
#endif
}

void metal_blit_encoder_copy_from_texture_to_texture(mtl_blit_command_encoder_t encoder,
                                                     void* sourceTexture, unsigned long sourceSlice, unsigned long sourceLevel, unsigned long sourceOrigin[3],
                                                     unsigned long sourceSize[3],
                                                     void* destinationTexture, unsigned long destinationSlice, unsigned long destinationLevel, unsigned long destinationOrigin[3]) {
#if defined(__OBJC__)
    if (encoder && sourceTexture && destinationTexture) {
        MTLSize size = MTLSizeMake(sourceSize[0], sourceSize[1], sourceSize[2]);
        MTLOrigin srcOrigin = MTLOriginMake(sourceOrigin[0], sourceOrigin[1], sourceOrigin[2]);
        MTLOrigin dstOrigin = MTLOriginMake(destinationOrigin[0], destinationOrigin[1], destinationOrigin[2]);
        [TO_BLIT_ENC(encoder) copyFromTexture:TO_TEX(sourceTexture) 
                                  sourceSlice:sourceSlice 
                                  sourceLevel:sourceLevel 
                                 sourceOrigin:srcOrigin 
                                   sourceSize:size 
                                    toTexture:TO_TEX(destinationTexture) 
                             destinationSlice:destinationSlice 
                             destinationLevel:destinationLevel 
                            destinationOrigin:dstOrigin];
    }
#endif
}

void metal_blit_encoder_generate_mipmaps(mtl_blit_command_encoder_t encoder, void* texture) {
#if defined(__OBJC__)
    if (encoder && texture) {
        [TO_BLIT_ENC(encoder) generateMipmapsForTexture:TO_TEX(texture)];
    }
#endif
}

void metal_blit_encoder_fill_buffer(mtl_blit_command_encoder_t encoder, void* buffer, unsigned long range_start, unsigned long range_length, uint8_t value) {
#if defined(__OBJC__)
    if (encoder && buffer) {
        NSRange range = NSMakeRange(range_start, range_length);
        [TO_BLIT_ENC(encoder) fillBuffer:TO_BUF(buffer) range:range value:value];
    }
#endif
}

void metal_blit_encoder_synchronize_resource(mtl_blit_command_encoder_t encoder, void* resource) {
#if defined(__OBJC__)
    if (encoder && resource) {
        // Only applicable if the device is not unified memory, usually useful for Mac
        #if TARGET_OS_OSX
            [TO_BLIT_ENC(encoder) synchronizeResource:(__bridge id<MTLResource>)resource];
        #endif
    }
#endif
}
