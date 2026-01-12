// src/engine/rendering/post_processing/ssao_compute.h
//
// Purpose: Compute shader-based SSAO header
// High-performance SSAO using GPU compute shaders

#ifndef SSAO_COMPUTE_H
#define SSAO_COMPUTE_H

#include "core/math/types.h"
#include "rendering/core/texture.h"

#ifdef __cplusplus
extern "C" {
#endif

// SSAO compute settings
typedef struct SSAOComputeSettings {
    f32 radius;        // World space radius (0.5f default)
    f32 intensity;     // AO intensity (1.0f default)
    f32 bias;          // Depth bias (0.025f default)
    u32 sample_count;  // Number of samples (16 default)
} SSAOComputeSettings;

// SSAO compute context
typedef struct SSAOComputeContext SSAOComputeContext;

// ============================================================================
// CREATION AND DESTRUCTION
// ============================================================================

// Create SSAO compute system
SSAOComputeContext* ssao_compute_create(u32 width, u32 height);

// Destroy SSAO compute system
void ssao_compute_destroy(SSAOComputeContext* ctx);

// ============================================================================
// MAIN PROCESSING
// ============================================================================

// Process SSAO using compute shaders
void ssao_compute_process(SSAOComputeContext* ctx, 
                         TextureID depth_buffer,
                         TextureID normal_buffer);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Update SSAO settings
void ssao_compute_update_settings(SSAOComputeContext* ctx, const SSAOComputeSettings* settings);

// Get output texture
TextureID ssao_compute_get_output(SSAOComputeContext* ctx);

// Set projection parameters (near/far planes)
void ssao_compute_set_projection_params(SSAOComputeContext* ctx, f32 near_plane, f32 far_plane);

// Resize SSAO system
void ssao_compute_resize(SSAOComputeContext* ctx, u32 new_width, u32 new_height);

#ifdef __cplusplus
}
#endif

#endif // SSAO_COMPUTE_H
