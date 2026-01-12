#ifndef TONEMAPPING_TYPES_H
#define TONEMAPPING_TYPES_H

#ifdef __OBJC__
#include <Metal/Metal.h>
#else
typedef void* id;
typedef struct {} MTLComputePipelineState;
typedef struct {} MTLBuffer;
#endif

typedef struct tonemapper {
    id<MTLComputePipelineState> histogram_pipeline;
    id<MTLComputePipelineState> adapt_pipeline; // Added for adaptation
    id<MTLComputePipelineState> tonemap_pipeline;
    id<MTLBuffer> histogram_buffer;  // 256 bins
    id<MTLBuffer> exposure_buffer;   // float exposure
    float min_exposure;
    float max_exposure;
    float adaptation_speed;
} tonemapper_t;

#endif // TONEMAPPING_TYPES_H
