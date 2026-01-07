#include "bloom_upsample.h"
#include <stdio.h>

void bloom_upsample_init(bloom_system_t* bloom, id<MTLDevice> device, id<MTLLibrary> library) {
    NSError* error = nil;
    id<MTLFunction> kernel = [library newFunctionWithName:@"bloom_upsample"];
    if (!kernel) {
        printf("Error: Failed to find bloom_upsample kernel\n");
        return;
    }
    
    bloom->upsample_pipeline = [device newComputePipelineStateWithFunction:kernel error:&error];
    if (!bloom->upsample_pipeline) {
        printf("Error: Failed to create bloom upsample pipeline: %s\n", [[error localizedDescription] UTF8String]);
    }
}

void bloom_upsample_dispatch(bloom_system_t* bloom, id<MTLCommandBuffer> cmd) {
    if (!bloom->upsample_pipeline) return;

    id<MTLComputeCommandEncoder> encoder = [cmd computeCommandEncoder];
    [encoder setLabel:@"Bloom Upsample"];

    [encoder setComputePipelineState:bloom->upsample_pipeline];

    float radius = 1.0f; // Default filter radius
    [encoder setBytes:&radius length:sizeof(float) atIndex:0];

    // IMPORTANT: The loop goes from smallest mip to largest.
    // mip_chain[mip_count-1] is the smallest.
    // mip_chain[0] is the largest.
    // We blend [i+1] (smaller) into [i] (larger).
    
    for (int i = bloom->mip_count - 2; i >= 0; i--) {
        [encoder setTexture:bloom->mip_chain[i + 1] atIndex:0];  // Low res input
        [encoder setTexture:bloom->mip_chain[i] atIndex:1];      // High res input/output (read_write)

        uint32_t w = [bloom->mip_chain[i] width];
        uint32_t h = [bloom->mip_chain[i] height];
        MTLSize groups = MTLSizeMake((w + 7) / 8, (h + 7) / 8, 1);
        [encoder dispatchThreadgroups:groups threadsPerThreadgroup:MTLSizeMake(8, 8, 1)];
    }

    [encoder endEncoding];
}
