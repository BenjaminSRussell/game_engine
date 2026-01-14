// TAA (Temporal Anti-Aliasing) Implementation
// Integrates with render graph system

#ifndef TAA_H
#define TAA_H

#include "rendering/frame_graph/frame_graph.h"
#include "rendering/core/texture.h"
#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// TAA settings
typedef struct TAASettings {
    f32 blend_factor;        // 0.05f default (temporal blend strength)
    f32 sharpness;           // 0.5f default (optional sharpening)
    bool enable_sharpening;
    bool enable_jitter;      // Camera jitter for sub-pixel sampling
    
    // Quality settings
    u32 sample_pattern;      // Halton sequence index
    f32 jitter_scale;        // 1.0f default
} TAASettings;

// TAA context (persistent state)
typedef struct TAAContext {
    TAASettings settings;
    
    // History buffer (persistent between frames)
    TextureID history_buffer;
    
    // Compiled shader
    u32 shader_program;
    
    // Jitter pattern
    u32 frame_index;
    
    bool initialized;
} TAAContext;

// Initialize TAA system
TAAContext *taa_create(u32 width, u32 height);

// Destroy TAA system
void taa_destroy(TAAContext *ctx);

// Add TAA pass to render graph
RGResourceHandle taa_add_to_graph(RenderGraph *rg,
                                   TAAContext *ctx,
                                   RGResourceHandle scene_color,
                                   RGResourceHandle velocity_buffer);

// Get current jitter offset for camera
void taa_get_jitter_offset(TAAContext *ctx, f32 *out_x, f32 *out_y);

// Update settings
void taa_update_settings(TAAContext *ctx, const TAASettings *settings);

#ifdef __cplusplus
}
#endif

#endif // TAA_H
