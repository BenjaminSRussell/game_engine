#ifndef BLOOM_TYPES_H
#define BLOOM_TYPES_H

#include <Metal/Metal.h>

typedef struct bloom_system {
    id<MTLTexture> mip_chain[8];
    uint32_t mip_count;
    id<MTLComputePipelineState> downsample_pipeline;
    id<MTLComputePipelineState> upsample_pipeline;
    float threshold;
    float intensity;
} bloom_system_t;

#endif // BLOOM_TYPES_H
