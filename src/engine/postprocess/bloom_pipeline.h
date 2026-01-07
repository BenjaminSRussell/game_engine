/*
 * bloom_pipeline.h
 * Unified bloom post-processing pipeline
 * 
 * Orchestrates threshold extraction, multi-level downsampling,
 * upsampling with tent filters, and final composition.
 * 
 * Part of the Post-Processing system
 * Advanced 3D Rendering Engine
 */

#ifndef BLOOM_PIPELINE_H
#define BLOOM_PIPELINE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define BLOOM_MAX_MIP_LEVELS 8
#define BLOOM_DEFAULT_MIP_LEVELS 6

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef enum {
    BLOOM_QUALITY_LOW = 0,      // 3 mip levels, fast
    BLOOM_QUALITY_MEDIUM = 1,   // 5 mip levels
    BLOOM_QUALITY_HIGH = 2,     // 6 mip levels
    BLOOM_QUALITY_ULTRA = 3     // 8 mip levels, slowest
} bloom_quality_t;

typedef struct bloom_settings {
    // Threshold
    float threshold;            // Luminance threshold (default: 1.0)
    float soft_knee;            // Soft threshold knee (default: 0.5)
    
    // Intensity
    float intensity;            // Bloom strength (default: 0.04)
    float scatter;              // Bloom scatter/spread (default: 0.7)
    
    // Color
    float color_shift_r;        // Red channel shift (default: 1.0)
    float color_shift_g;        // Green channel shift (default: 1.0)
    float color_shift_b;        // Blue channel shift (default: 1.0)
    
    // Quality
    bloom_quality_t quality;    // Quality preset
    uint32_t mip_count;         // Number of mip levels (overrides quality)
    
    // Advanced features (optional)
    bool enable_anamorphic;     // Enable anamorphic bloom
    bool enable_lens_dirt;      // Enable lens dirt
    bool enable_lens_flares;    // Enable lens flares
    float dirt_intensity;       // Lens dirt intensity
    float flare_intensity;      // Lens flare intensity
} bloom_settings_t;

typedef struct bloom_pipeline bloom_pipeline_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int bloom_pipeline_init(void);
void bloom_pipeline_shutdown(void);

/* Lifecycle */
bloom_pipeline_t* bloom_pipeline_create(uint32_t width, uint32_t height, const bloom_settings_t* settings);
void bloom_pipeline_destroy(bloom_pipeline_t* pipeline);

/* Configuration */
void bloom_pipeline_set_settings(bloom_pipeline_t* pipeline, const bloom_settings_t* settings);
void bloom_pipeline_get_settings(const bloom_pipeline_t* pipeline, bloom_settings_t* out_settings);
bloom_settings_t bloom_pipeline_default_settings(void);

/* Resize */
int bloom_pipeline_resize(bloom_pipeline_t* pipeline, uint32_t width, uint32_t height);

/* Processing 
 * 
 * Process bloom effect on input HDR texture, compositing result back
 * 
 * input_hdr: Input HDR texture (RGBA float format)
 * output_hdr: Output HDR texture (RGBA float format) - can be same as input
 * width, height: Texture dimensions
 */
int bloom_pipeline_process(bloom_pipeline_t* pipeline, 
                          const float* input_hdr, 
                          float* output_hdr, 
                          uint32_t width, 
                          uint32_t height);

/* Get intermediate buffers for debugging */
const float* bloom_pipeline_get_threshold_buffer(const bloom_pipeline_t* pipeline, uint32_t* out_width, uint32_t* out_height);
const float* bloom_pipeline_get_mip_buffer(const bloom_pipeline_t* pipeline, uint32_t mip_level, uint32_t* out_width, uint32_t* out_height);

/* Statistics */
void bloom_pipeline_get_stats(const bloom_pipeline_t* pipeline, 
                             size_t* out_memory_used,
                             uint32_t* out_mip_count,
                             float* out_last_process_time_ms);

#ifdef __cplusplus
}
#endif

#endif /* BLOOM_PIPELINE_H */
