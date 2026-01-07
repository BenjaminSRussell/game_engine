#include "aces_tonemapper.h"
#include <stdio.h>

void aces_tonemapper_init(tonemapper_t* tm, id<MTLDevice> device, id<MTLLibrary> library) {
    NSError* error = nil;
    id<MTLFunction> kernel = [library newFunctionWithName:@"tonemapping_apply"];
    if (!kernel) {
        printf("Error: Failed to find tonemapping_apply kernel\n");
        return;
    }
    
    tm->tonemap_pipeline = [device newComputePipelineStateWithFunction:kernel error:&error];
    if (!tm->tonemap_pipeline) {
        printf("Error: Failed to create tonemap pipeline: %s\n", [[error localizedDescription] UTF8String]);
    }
}

void aces_tonemapper_render(tonemapper_t* tm, id<MTLCommandBuffer> cmd, id<MTLTexture> hdr_input, id<MTLTexture> bloom_input, id<MTLTexture> ldr_output) {
    if (!tm->tonemap_pipeline) return;

    id<MTLComputeCommandEncoder> encoder = [cmd computeCommandEncoder];
    [encoder setLabel:@"ACES Tonemap"];

    [encoder setComputePipelineState:tm->tonemap_pipeline];
    [encoder setTexture:hdr_input atIndex:0];
    [encoder setTexture:bloom_input atIndex:1];
    [encoder setTexture:ldr_output atIndex:2];
    
    if (tm->exposure_buffer) {
        [encoder setBuffer:tm->exposure_buffer offset:0 atIndex:0];
    } else {
        // Fallback or error
        float default_exposure = 1.0f;
        [encoder setBytes:&default_exposure length:sizeof(float) atIndex:0];
    }

    uint32_t w = [ldr_output width];
    uint32_t h = [ldr_output height];
    MTLSize groups = MTLSizeMake((w + 15) / 16, (h + 15) / 16, 1);
    [encoder dispatchThreadgroups:groups threadsPerThreadgroup:MTLSizeMake(16, 16, 1)];

    [encoder endEncoding];
}
