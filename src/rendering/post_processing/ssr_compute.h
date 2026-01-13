// src/engine/rendering/post_processing/ssr_compute.h
//
// Purpose: Compute shader-based SSR header
// High-performance screen-space reflections using GPU compute shaders

#ifndef SSR_COMPUTE_H
#define SSR_COMPUTE_H

#include "core/math/types.h"
#include "rendering/core/texture.h"

#ifdef __cplusplus
extern "C" {
#endif

// SSR compute settings
typedef struct SSRComputeSettings {
    f32 max_distance;     // Maximum ray distance (50.0f default)
    f32 thickness;        // Surface thickness (0.1f default)
    u32 max_steps;        // Maximum ray march steps (64 default)
    f32 stride;           // Initial ray stride (1.0f default)
    f32 fade_distance;   // Distance to start fading (25.0f default)
} SSRComputeSettings;

// SSR compute context
typedef struct SSRComputeContext SSRComputeContext;

// ============================================================================
// CREATION AND DESTRUCTION
// ============================================================================

// Create SSR compute system
SSRComputeContext* ssr_compute_create(u32 width, u32 height);

// Destroy SSR compute system
void ssr_compute_destroy(SSRComputeContext* ctx);

// ============================================================================
// MAIN PROCESSING
// ============================================================================

// Process SSR using compute shaders
void ssr_compute_process(SSRComputeContext* ctx,
                        TextureID scene_color,
                        TextureID normal_roughness,
                        TextureID depth_buffer);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Update SSR settings
void ssr_compute_update_settings(SSRComputeContext* ctx, const SSRComputeSettings* settings);

// Get output texture
TextureID ssr_compute_get_output(SSRComputeContext* ctx);

// Set projection parameters (near/far planes)
void ssr_compute_set_projection_params(SSRComputeContext* ctx, f32 near_plane, f32 far_plane);

// Resize SSR system
void ssr_compute_resize(SSRComputeContext* ctx, u32 new_width, u32 new_height);

#ifdef __cplusplus
}
#endif

#endif // SSR_COMPUTE_H
