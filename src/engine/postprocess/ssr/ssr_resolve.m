#include "postprocess/ssr/ssr_resolve.h"

void ssr_resolve_execute(id<MTLDevice> device, id<MTLCommandBuffer> cmd,
                         id<MTLComputePipelineState> pipeline,
                         id<MTLTexture> hit_texture,
                         id<MTLTexture> color_texture,
                         id<MTLTexture> output_texture,
                         uint32_t width, uint32_t height) {
    
    id<MTLComputeCommandEncoder> encoder = [cmd computeCommandEncoder];
    [encoder setLabel:@"SSR Resolve Pass"];
    
    [encoder setComputePipelineState:pipeline];
    [encoder setTexture:hit_texture atIndex:0];
    [encoder setTexture:color_texture atIndex:1];
    [encoder setTexture:output_texture atIndex:2];
    
    MTLSize groups = MTLSizeMake((width + 7) / 8, (height + 7) / 8, 1);
    MTLSize threads = MTLSizeMake(8, 8, 1);
    [encoder dispatchThreadgroups:groups threadsPerThreadgroup:threads];
    
    [encoder endEncoding];
}
