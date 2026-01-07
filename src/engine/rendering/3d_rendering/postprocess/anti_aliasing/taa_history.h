#ifndef TAA_HISTORY_H
#define TAA_HISTORY_H

#include <Metal/Metal.h>
#include <simd/simd.h>

typedef struct taa_system {
    id<MTLTexture> history_texture[2];  // Ping-pong buffers
    id<MTLTexture> velocity_texture;    // Reference to external velocity texture
    id<MTLComputePipelineState> resolve_pipeline;
    uint32_t current_history;
    uint32_t frame_index;
    simd_uint2 screen_size;
} taa_system_t;

// Initialize the TAA system
// device: Metal device to create textures and pipelines
// width, height: Dimensions of the render targets
taa_system_t* taa_create(id<MTLDevice> device, uint32_t width, uint32_t height);

// Destroy the TAA system and release resources
void taa_destroy(taa_system_t* taa);

// Resize history textures when screen size changes
void taa_resize(taa_system_t* taa, id<MTLDevice> device, uint32_t width, uint32_t height);

#endif // TAA_HISTORY_H
