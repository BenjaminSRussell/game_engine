#include "exposure_adaptation.h"
#include <stdio.h>

void exposure_adaptation_init(tonemapper_t* tm, id<MTLDevice> device, id<MTLLibrary> library) {
    NSError* error = nil;
    
    // Histogram pipeline
    id<MTLFunction> hist_kernel = [library newFunctionWithName:@"tonemapping_histogram"];
    if (!hist_kernel) {
        printf("Error: Failed to find tonemapping_histogram kernel\n");
    } else {
        tm->histogram_pipeline = [device newComputePipelineStateWithFunction:hist_kernel error:&error];
        if (!tm->histogram_pipeline) {
             printf("Error: Failed to create histogram pipeline: %s\n", [[error localizedDescription] UTF8String]);
        }
    }
    
    // Adaptation pipeline
    id<MTLFunction> adapt_kernel = [library newFunctionWithName:@"tonemapping_adapt_exposure"];
    if (!adapt_kernel) {
        printf("Error: Failed to find tonemapping_adapt_exposure kernel\n");
    } else {
        tm->adapt_pipeline = [device newComputePipelineStateWithFunction:adapt_kernel error:&error];
    }
    
    // Buffers
    // 256 uints for histogram
    tm->histogram_buffer = [device newBufferWithLength:256 * sizeof(uint32_t) options:MTLResourceStorageModePrivate];
    
    // Exposure buffer (float)
    float initial_exposure = 1.0f;
    tm->exposure_buffer = [device newBufferWithBytes:&initial_exposure length:sizeof(float) options:MTLResourceStorageModePrivate];
}

void exposure_adaptation_dispatch(tonemapper_t* tm, id<MTLCommandBuffer> cmd, id<MTLTexture> hdr_input, float dt) {
    if (!tm->histogram_pipeline || !tm->adapt_pipeline) return;

    id<MTLComputeCommandEncoder> encoder = [cmd computeCommandEncoder];
    [encoder setLabel:@"Exposure Adaptation"];
    
    // 1. Generate Histogram
    [encoder setComputePipelineState:tm->histogram_pipeline];
    [encoder setTexture:hdr_input atIndex:0];
    [encoder setBuffer:tm->histogram_buffer offset:0 atIndex:0];
    
    uint32_t w = [hdr_input width];
    uint32_t h = [hdr_input height];
    MTLSize groups = MTLSizeMake((w + 15) / 16, (h + 15) / 16, 1);
    [encoder dispatchThreadgroups:groups threadsPerThreadgroup:MTLSizeMake(16, 16, 1)];
    
    // Barrier to ensure histogram is ready
    [encoder memoryBarrierWithScope:MTLBarrierScopeBuffers];
    
    // 2. Adapt Exposure
    [encoder setComputePipelineState:tm->adapt_pipeline];
    [encoder setBuffer:tm->histogram_buffer offset:0 atIndex:0];
    [encoder setBuffer:tm->exposure_buffer offset:0 atIndex:1];
    
    [encoder setBytes:&tm->min_exposure length:sizeof(float) atIndex:2];
    [encoder setBytes:&tm->max_exposure length:sizeof(float) atIndex:3];
    [encoder setBytes:&tm->adaptation_speed length:sizeof(float) atIndex:4];
    [encoder setBytes:&dt length:sizeof(float) atIndex:5];
    uint32_t pixel_count = w * h;
    [encoder setBytes:&pixel_count length:sizeof(uint32_t) atIndex:6];
    
    // Single thread for adaptation
    [encoder dispatchThreadgroups:MTLSizeMake(1, 1, 1) threadsPerThreadgroup:MTLSizeMake(1, 1, 1)];
    
    [encoder endEncoding];
}
