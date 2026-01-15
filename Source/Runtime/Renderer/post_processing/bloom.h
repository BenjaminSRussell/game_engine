// Bloom Post-Processing Pipeline
// Advanced bloom effect with multi-level mip pyramid and tent filtering
#ifndef BLOOM_H
#define BLOOM_H

#include "core/math/types.h"
#include "rendering/frame_graph/frame_graph.h"

#ifdef __cplusplus
extern "C" {
#endif

// Maximum mip levels for bloom pyramid
#define BLOOM_MAX_MIP_LEVELS 8
#define BLOOM_DEFAULT_MIP_LEVELS 6

// Bloom quality preset
typedef enum {
    BLOOM_QUALITY_LOW = 0,      // 3 mip levels, fast
    BLOOM_QUALITY_MEDIUM = 1,   // 5 mip levels
    BLOOM_QUALITY_HIGH = 2,     // 6 mip levels
    BLOOM_QUALITY_ULTRA = 3     // 8 mip levels
} BloomQuality;

// Bloom settings
typedef struct {
    f32 threshold;              // Luminance threshold (default: 1.0)
    f32 soft_knee;              // Soft threshold knee (default: 0.5)
    f32 intensity;              // Bloom strength (default: 0.04)
    f32 scatter;                // Bloom scatter/spread (default: 0.7)
    f32 color_shift_r;          // Red channel shift (default: 1.0)
    f32 color_shift_g;          // Green channel shift (default: 1.0)
    f32 color_shift_b;          // Blue channel shift (default: 1.0)
    BloomQuality quality;       // Quality preset
    u32 mip_count;              // Number of mip levels
    bool enable_anamorphic;     // Enable anamorphic bloom
    bool enable_lens_dirt;      // Enable lens dirt effect
    bool enable_lens_flare;     // Enable lens flare
    f32 lens_dirt_intensity;    // Lens dirt intensity (0-1)
} BloomSettings;

// Opaque bloom context
typedef struct BloomContext BloomContext;

// Create bloom context
BloomContext* bloom_create(u32 width, u32 height);

// Destroy bloom context
void bloom_destroy(BloomContext *ctx);

// Update bloom settings
void bloom_update_settings(BloomContext *ctx, const BloomSettings *settings);

// Add bloom pipeline to render graph
// Returns the final bloom output texture handle
RGResourceHandle bloom_add_to_graph(RenderGraph *rg,
                                     BloomContext *ctx,
                                     RGResourceHandle scene_color);

#ifdef __cplusplus
}
#endif

#endif // BLOOM_H
