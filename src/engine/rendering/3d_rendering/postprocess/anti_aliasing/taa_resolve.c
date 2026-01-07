#include "taa_resolve.h"
#include <stdio.h>

struct TAAUniforms {
    simd_uint2 screen_size;
};

id<MTLComputePipelineState> taa_init_pipeline(id<MTLDevice> device) {
    NSError* error = nil;
    
    // Load default library
    id<MTLLibrary> library = [device newDefaultLibrary];
    if (!library) {
        printf("Failed to load default Metal library. TAA will not work.\n");
        return nil;
    }
    
    id<MTLFunction> kernel = [library newFunctionWithName:@"taa_resolve"];
    if (!kernel) {
        printf("Failed to find 'taa_resolve' kernel function.\n");
        return nil;
    }
    
    id<MTLComputePipelineState> pipeline = [device newComputePipelineStateWithFunction:kernel error:&error];
    if (error) {
        printf("Failed to create TAA pipeline state: %s\n", [[error localizedDescription] UTF8String]);
        return nil;
    }
    
    return pipeline;
}

void taa_render(taa_system_t* taa, id<MTLCommandBuffer> cmd,
                id<MTLTexture> current_frame, id<MTLTexture> depth,
                id<MTLTexture> output) {
    if (!taa || !taa->resolve_pipeline) return;

    id<MTLComputeCommandEncoder> encoder = [cmd computeCommandEncoder];
    if (!encoder) return;
    
    encoder.label = @"TAA Resolve";

    uint32_t prev_history = taa->current_history;
    taa->current_history = 1 - taa->current_history; // Swap history

    [encoder setComputePipelineState:taa->resolve_pipeline];
    
    // Texture bindings match shader:
    // 0: current
    // 1: history (read) -> prev_history
    // 2: velocity
    // 3: depth
    // 4: output (write) -> current_history (we write to the "new" history texture)
    
    // Note: The concept here is that we resolve INTO the new history texture, 
    // then copy THAT to the final output (or just use it as output).
    // The user's snippet does: write to history[current], then blit to output.
    
    [encoder setTexture:current_frame atIndex:0];
    [encoder setTexture:taa->history_texture[prev_history] atIndex:1];
    
    // Velocity texture must be set in the struct beforehand
    [encoder setTexture:taa->velocity_texture atIndex:2];
    
    [encoder setTexture:depth atIndex:3];
    [encoder setTexture:taa->history_texture[taa->current_history] atIndex:4];

    struct TAAUniforms uniforms;
    uniforms.screen_size = simd_make_uint2((uint32_t)current_frame.width, (uint32_t)current_frame.height);
    
    [encoder setBytes:&uniforms length:sizeof(uniforms) atIndex:0];

    MTLSize threadGroupSize = MTLSizeMake(8, 8, 1);
    MTLSize threadGroups = MTLSizeMake((current_frame.width + threadGroupSize.width - 1) / threadGroupSize.width,
                                       (current_frame.height + threadGroupSize.height - 1) / threadGroupSize.height,
                                       1);
    
    [encoder dispatchThreadgroups:threadGroups threadsPerThreadgroup:threadGroupSize];
    [encoder endEncoding];

    // Blit result to output
    if (output) {
        id<MTLBlitCommandEncoder> blit = [cmd blitCommandEncoder];
        blit.label = @"TAA Blit to Output";
        [blit copyFromTexture:taa->history_texture[taa->current_history] toTexture:output];
        [blit endEncoding];
    }
    
    taa->frame_index++;
}
