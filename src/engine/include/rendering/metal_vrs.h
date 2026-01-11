// include/rendering/metal_vrs.h
// Variable Rate Shading for Metal (2-4x performance boost)
#ifndef METAL_VRS_H
#define METAL_VRS_H

#include "include/common.h"
#include "include/math/vec2.h"
#include <Metal/Metal.h>

typedef enum {
  VRS_RATE_1X1 = 0, // Full rate
  VRS_RATE_1X2,     // Half rate horizontal
  VRS_RATE_2X1,     // Half rate vertical
  VRS_RATE_2X2,     // Quarter rate (both)
  VRS_RATE_4X2,     // 1/8 rate
  VRS_RATE_2X4,     // 1/8 rate
  VRS_RATE_4X4      // 1/16 rate (max)
} MetalVRSRate;

typedef struct MetalVRSImage MetalVRSImage;

#ifdef __cplusplus
extern "C" {
#endif

// Capability check
bool metal_vrs_is_supported(id<MTLDevice> device);

// Create VRS rate map (where to apply reduced shading)
MetalVRSImage *metal_vrs_create_rate_map(id<MTLDevice> device, u32 width,
                                         u32 height);
void metal_vrs_destroy_rate_map(MetalVRSImage *vrs_map);

// Update rate map (e.g., center full rate, edges quarter rate)
void metal_vrs_set_foveated(MetalVRSImage *vrs_map, id<MTLCommandBuffer> cmd,
                            Vec2 focus_center, f32 inner_radius,
                            f32 outer_radius);

// Set uniform rate (for testing)
void metal_vrs_set_uniform_rate(MetalVRSImage *vrs_map, MetalVRSRate rate);

// Apply VRS to render pass
void metal_vrs_apply_to_pass(id<MTLRenderCommandEncoder> encoder,
                             MetalVRSImage *vrs_map);

#ifdef __cplusplus
}
#endif

#endif // METAL_VRS_H
