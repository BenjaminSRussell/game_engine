// Screen-Space Reflections (SSR) Header
// Efficient screen-space reflection algorithm with hierarchical ray marching

#ifndef SSR_H
#define SSR_H

#include "core/types.h"
#include "rendering/frame_graph/frame_graph.h"

#ifdef __cplusplus
extern "C" {
#endif

// Opaque SSR context
typedef struct SSRContext SSRContext;

// Create SSR context
SSRContext *ssr_create(u32 width, u32 height);

// Destroy SSR context
void ssr_destroy(SSRContext *ctx);

// Add SSR pass to render graph
RGResourceHandle ssr_add_to_graph(RenderGraph *rg,
                                 SSRContext *ctx,
                                 RGResourceHandle scene_color,
                                 RGResourceHandle normal_roughness,
                                 RGResourceHandle depth_buffer);

// Configure maximum ray march distance (screen space pixels)
void ssr_set_max_distance(SSRContext *ctx, f32 distance);

// Configure surface thickness for intersection testing
void ssr_set_thickness(SSRContext *ctx, f32 thickness);

// Configure maximum ray march steps
void ssr_set_max_steps(SSRContext *ctx, u32 steps);

#ifdef __cplusplus
}
#endif

#endif // SSR_H
