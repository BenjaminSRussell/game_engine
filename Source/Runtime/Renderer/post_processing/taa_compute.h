// src/engine/rendering/post_processing/taa_compute.h
//
// Purpose: Compute shader-based Temporal Anti-Aliasing header
// High-performance TAA using GPU compute shaders

#ifndef TAA_COMPUTE_H
#define TAA_COMPUTE_H

#include "core/math/types.h"
#include "rendering/core/texture.h"

#ifdef __cplusplus
extern "C" {
#endif

// TAA compute settings
typedef struct TAAComputeSettings {
    f32 blend_factor;        // 0.05f default (temporal blend strength)
    f32 sharpness;           // 0.5f default (optional sharpening)
    bool enable_sharpening;
    bool enable_jitter;      // Camera jitter for sub-pixel sampling
    
    // Quality settings
    u32 sample_pattern;      // Halton sequence index
    f32 jitter_scale;        // 1.0f default
} TAAComputeSettings;

// TAA compute context (persistent state)
typedef struct TAAComputeContext TAAComputeContext;

// ============================================================================
// CREATION AND DESTRUCTION
// ============================================================================

// Create TAA compute system
TAAComputeContext *taa_compute_create(u32 width, u32 height);

// Destroy TAA compute system
void taa_compute_destroy(TAAComputeContext *ctx);

// ============================================================================
// MAIN PROCESSING
// ============================================================================

// Process frame with TAA using compute shaders
void taa_compute_process(TAAComputeContext *ctx, 
                      TextureID current_frame,
                      TextureID velocity_buffer,
                      TextureID depth_buffer);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Get current jitter offset for camera
void taa_compute_get_jitter_offset(TAAComputeContext *ctx, f32 *out_x, f32 *out_y);

// Update settings
void taa_compute_update_settings(TAAComputeContext *ctx, const TAAComputeSettings *settings);

// Get output texture (current frame after TAA)
TextureID taa_compute_get_output(TAAComputeContext *ctx);

// Get history texture (for debugging)
TextureID taa_compute_get_history(TAAComputeContext *ctx);

// Reset TAA state (clear history)
void taa_compute_reset(TAAComputeContext *ctx);

#ifdef __cplusplus
}
#endif

#endif // TAA_COMPUTE_H
