// include/rendering/metal_fx.h
//
// MetalFX Upscaling Integration (Spatial + Temporal)
// Provides DLSS/FSR-quality upscaling on Apple Silicon
//
#ifndef METAL_FX_H
#define METAL_FX_H

#include "include/common.h"
#include <Metal/Metal.h>

#ifdef __OBJC__
#import <MetalFX/MetalFX.h>
#endif

// MetalFX upscaling modes
typedef enum {
    METALFX_MODE_NONE = 0,
    METALFX_MODE_SPATIAL,           // Spatial upscaling (quick, 2x perf boost)
    METALFX_MODE_TEMPORAL,          // Temporal upscaling (DLSS-quality)
    METALFX_MODE_TEMPORAL_AA_ONLY   // Use only for anti-aliasing
} MetalFXMode;

// Quality presets
typedef enum {
    METALFX_QUALITY_PERFORMANCE = 0,  // 50% render resolution
    METALFX_QUALITY_BALANCED,         // 58% render resolution
    METALFX_QUALITY_QUALITY,          // 67% render resolution
    METALFX_QUALITY_ULTRA_QUALITY     // 77% render resolution
} MetalFXQuality;

// MetalFX upscaler configuration
typedef struct {
    u32 input_width;
    u32 input_height;
    u32 output_width;
    u32 output_height;
    
    MetalFXMode mode;
    MetalFXQuality quality;
    
    bool enable_auto_exposure;
    bool enable_reactive_mask;
    f32 sharpness;  // 0.0 to 1.0
    
    // Temporal-specific
    f32 jitter_offset_x;
    f32 jitter_offset_y;
    bool reset_history;
} MetalFXConfig;

// Opaque upscaler handle
typedef struct MetalFXUpscaler MetalFXUpscaler;

#ifdef __cplusplus
extern "C" {
#endif

// Capability detection
bool metalfx_is_supported(id<MTLDevice> device);
bool metalfx_is_spatial_supported(id<MTLDevice> device);
bool metalfx_is_temporal_supported(id<MTLDevice> device);

// Upscaler lifecycle
MetalFXUpscaler* metalfx_create_spatial_upscaler(id<MTLDevice> device, const MetalFXConfig* config);
MetalFXUpscaler* metalfx_create_temporal_upscaler(id<MTLDevice> device, const MetalFXConfig* config);
void metalfx_destroy_upscaler(MetalFXUpscaler* upscaler);

// Get recommended quality mode based on target resolution
MetalFXQuality metalfx_get_recommended_quality(u32 target_width, u32 target_height);

// Calculate render resolution for quality preset
void metalfx_get_render_resolution(u32 output_width, u32 output_height, 
                                    MetalFXQuality quality,
                                    u32* out_render_width, u32* out_render_height);

// Upscaling operation
void metalfx_upscale(MetalFXUpscaler* upscaler,
                     id<MTLCommandBuffer> command_buffer,
                     id<MTLTexture> color_input,
                     id<MTLTexture> depth_input,
                     id<MTLTexture> motion_vector_input,  // Required for temporal
                     id<MTLTexture> output);

// Update config (for resolution changes, etc.)
void metalfx_update_config(MetalFXUpscaler* upscaler, const MetalFXConfig* config);

// Temporal history reset (for scene cuts, teleports)
void metalfx_reset_temporal_history(MetalFXUpscaler* upscaler);

// Jitter pattern generation (for temporal)
void metalfx_get_jitter_offset(u32 frame_index, f32* out_x, f32* out_y);

#ifdef __cplusplus
}
#endif

#endif // METAL_FX_H
