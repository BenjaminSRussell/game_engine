// src/engine/rendering/post_processing/bloom_compute.h
//
// Purpose: Compute shader-based Bloom header
// High-performance bloom using GPU compute shaders

#ifndef BLOOM_COMPUTE_H
#define BLOOM_COMPUTE_H

#include "core/math/types.h"
#include "rendering/core/texture.h"

#ifdef __cplusplus
extern "C" {
#endif

// Bloom compute settings
typedef struct BloomComputeSettings {
    f32 threshold;        // Brightness threshold (1.0f default)
    f32 soft_knee;        // Soft threshold knee (0.5f default)
    f32 intensity;        // Bloom intensity (0.04f default)
    u32 iterations;       // Number of blur iterations (5 default)
    f32 sigma;            // Gaussian blur sigma (1.0f default)
    vec3 color_shift;     // Bloom color tint (default white)
} BloomComputeSettings;

// Bloom compute context
typedef struct BloomComputeContext BloomComputeContext;

// ============================================================================
// CREATION AND DESTRUCTION
// ============================================================================

// Create bloom compute system
BloomComputeContext* bloom_compute_create(u32 width, u32 height);

// Destroy bloom compute system
void bloom_compute_destroy(BloomComputeContext* ctx);

// ============================================================================
// MAIN PROCESSING
// ============================================================================

// Process HDR image with bloom using compute shaders
void bloom_compute_process(BloomComputeContext* ctx, TextureID hdr_input);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Update bloom settings
void bloom_compute_update_settings(BloomComputeContext* ctx, const BloomComputeSettings* settings);

// Get final output texture
TextureID bloom_compute_get_output(BloomComputeContext* ctx);

// Get threshold texture (for debugging)
TextureID bloom_compute_get_threshold(BloomComputeContext* ctx);

// Get blurred texture (for debugging)
TextureID bloom_compute_get_blurred(BloomComputeContext* ctx);

// Resize bloom system
void bloom_compute_resize(BloomComputeContext* ctx, u32 new_width, u32 new_height);

#ifdef __cplusplus
}
#endif

#endif // BLOOM_COMPUTE_H
