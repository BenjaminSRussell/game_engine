#include "bloom_downsample.h"
#include <stdio.h>

void bloom_downsample_init(bloom_system_t* bloom, id<MTLDevice> device, id<MTLLibrary> library) {
    NSError* error = nil;
    id<MTLFunction> kernel = [library newFunctionWithName:@"bloom_downsample"];
    if (!kernel) {
        printf("Error: Failed to find bloom_downsample kernel\n");
        return;
    }
    
    bloom->downsample_pipeline = [device newComputePipelineStateWithFunction:kernel error:&error];
    if (!bloom->downsample_pipeline) {
        printf("Error: Failed to create bloom downsample pipeline: %s\n", [[error localizedDescription] UTF8String]);
    }
}

void bloom_downsample_dispatch(bloom_system_t* bloom, id<MTLCommandBuffer> cmd, id<MTLTexture> hdr_input) {
    if (!bloom->downsample_pipeline) return;

    id<MTLComputeCommandEncoder> encoder = [cmd computeCommandEncoder];
    [encoder setLabel:@"Bloom Downsample"];
    
    [encoder setComputePipelineState:bloom->downsample_pipeline];

    id<MTLTexture> src = hdr_input;
    for (uint32_t i = 0; i < bloom->mip_count; i++) {
        [encoder setTexture:src atIndex:0];
        [encoder setTexture:bloom->mip_chain[i] atIndex:1];

        float threshold = (i == 0) ? bloom->threshold : 0.0f;
        [encoder setBytes:&threshold length:sizeof(float) atIndex:0];

        uint32_t w = [bloom->mip_chain[i] width];
        uint32_t h = [bloom->mip_chain[i] height];
        MTLSize groups = MTLSizeMake((w + 7) / 8, (h + 7) / 8, 1);
        [encoder dispatchThreadgroups:groups threadsPerThreadgroup:MTLSizeMake(8, 8, 1)];

        src = bloom->mip_chain[i];
    }
    
    [encoder endEncoding];
}
