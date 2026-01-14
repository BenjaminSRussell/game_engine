// Screen-Space Ambient Occlusion (SSAO) Header
// Horizon-based SSAO for efficient ambient occlusion

#ifndef SSAO_H
#define SSAO_H

#include "core/types.h"
#include "rendering/frame_graph/frame_graph.h"

#ifdef __cplusplus
extern "C" {
#endif

// Opaque SSAO context
typedef struct SSAOContext SSAOContext;

// Create SSAO context
SSAOContext *ssao_create(u32 width, u32 height);

// Destroy SSAO context
void ssao_destroy(SSAOContext *ctx);

// Add SSAO pass to render graph
RGResourceHandle ssao_add_to_graph(RenderGraph *rg,
                                  SSAOContext *ctx,
                                  RGResourceHandle depth_buffer,
                                  RGResourceHandle normal_buffer);

// Configure SSAO radius (screen space pixels)
void ssao_set_radius(SSAOContext *ctx, f32 radius);

// Configure SSAO intensity (0.0 to 2.0)
void ssao_set_intensity(SSAOContext *ctx, f32 intensity);

#ifdef __cplusplus
}
#endif

#endif // SSAO_H
