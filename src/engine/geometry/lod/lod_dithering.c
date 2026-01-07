/*
 * lod_dithering.c
 * Temporal dithering for LOD transitions
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#include "lod_dithering.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

// 4x4 Bayer matrix
static const float BAYER_4x4[16] = {
     0.0f/16.0f,  8.0f/16.0f,  2.0f/16.0f, 10.0f/16.0f,
    12.0f/16.0f,  4.0f/16.0f, 14.0f/16.0f,  6.0f/16.0f,
     3.0f/16.0f, 11.0f/16.0f,  1.0f/16.0f,  9.0f/16.0f,
    15.0f/16.0f,  7.0f/16.0f, 13.0f/16.0f,  5.0f/16.0f
};

// 8x8 Bayer matrix
static const float BAYER_8x8[64] = {
     0.0f/64.0f, 32.0f/64.0f,  8.0f/64.0f, 40.0f/64.0f,  2.0f/64.0f, 34.0f/64.0f, 10.0f/64.0f, 42.0f/64.0f,
    48.0f/64.0f, 16.0f/64.0f, 56.0f/64.0f, 24.0f/64.0f, 50.0f/64.0f, 18.0f/64.0f, 58.0f/64.0f, 26.0f/64.0f,
    12.0f/64.0f, 44.0f/64.0f,  4.0f/64.0f, 36.0f/64.0f, 14.0f/64.0f, 46.0f/64.0f,  6.0f/64.0f, 38.0f/64.0f,
    60.0f/64.0f, 28.0f/64.0f, 52.0f/64.0f, 20.0f/64.0f, 62.0f/64.0f, 30.0f/64.0f, 54.0f/64.0f, 22.0f/64.0f,
     3.0f/64.0f, 35.0f/64.0f, 11.0f/64.0f, 43.0f/64.0f,  1.0f/64.0f, 33.0f/64.0f,  9.0f/64.0f, 41.0f/64.0f,
    51.0f/64.0f, 19.0f/64.0f, 59.0f/64.0f, 27.0f/64.0f, 49.0f/64.0f, 17.0f/64.0f, 57.0f/64.0f, 25.0f/64.0f,
    15.0f/64.0f, 47.0f/64.0f,  7.0f/64.0f, 39.0f/64.0f, 13.0f/64.0f, 45.0f/64.0f,  5.0f/64.0f, 37.0f/64.0f,
    63.0f/64.0f, 31.0f/64.0f, 55.0f/64.0f, 23.0f/64.0f, 61.0f/64.0f, 29.0f/64.0f, 53.0f/64.0f, 21.0f/64.0f
};

/* ============================================================================
 * CONFIGURATION
 * ============================================================================ */

lod_dither_config_t lod_dither_get_default_config(void) {
    lod_dither_config_t config = {
        .pattern = LOD_DITHER_BAYER_8x8,
        .dither_strength = 1.0f,
        .temporal_jitter = true,
        .frame_index = 0
    };
    return config;
}

/* ============================================================================
 * DITHER PATTERNS
 * ============================================================================ */

float lod_dither_bayer_4x4(uint32_t x, uint32_t y) {
    uint32_t idx = (y & 3) * 4 + (x & 3);
    return BAYER_4x4[idx];
}

float lod_dither_bayer_8x8(uint32_t x, uint32_t y) {
    uint32_t idx = (y & 7) * 8 + (x & 7);
    return BAYER_8x8[idx];
}

float lod_dither_interleaved_gradient(uint32_t x, uint32_t y, uint32_t frame) {
    // Interleaved gradient noise
    x += frame * 47;
    y += frame * 31;
    float xx = (float)x + 0.5f;
    float yy = (float)y + 0.5f;
    return fmodf(52.9829189f * fmodf(0.06711056f*xx + 0.00583715f*yy, 1.0f), 1.0f);
}

/* ============================================================================
 * DITHER THRESHOLD
 * ============================================================================ */

float lod_get_dither_threshold(
    uint32_t x,
    uint32_t y,
    float transition_t,
    const lod_dither_config_t* config
) {
    if (!config) return 0.5f;
    
    float threshold = 0.5f;
    
    switch (config->pattern) {
        case LOD_DITHER_BAYER_4x4:
            threshold = lod_dither_bayer_4x4(x, y);
            break;
        
        case LOD_DITHER_BAYER_8x8:
            threshold = lod_dither_bayer_8x8(x, y);
            break;
        
        case LOD_DITHER_INTERLEAVED_GRADIENT:
            threshold = lod_dither_interleaved_gradient(x, y, config->frame_index);
            break;
        
        case LOD_DITHER_TEMPORAL:
            {
                // Temporal dithering with frame jitter
                float base = lod_dither_bayer_8x8(x, y);
                if (config->temporal_jitter) {
                    float jitter = lod_dither_interleaved_gradient(x, y, config->frame_index);
                    threshold = fmodf(base + jitter * 0.25f, 1.0f);
                } else {
                    threshold = base;
                }
            }
            break;
        
        default:
            threshold = lod_dither_bayer_8x8(x, y);
            break;
    }
    
    // Apply dither strength
    // strength = 0 -> no dithering (hard switch at 0.5)
    // strength = 1 -> full dithering
    threshold = 0.5f + (threshold - 0.5f) * config->dither_strength;
    
    return threshold;
}

bool lod_should_render_lod_a(
    uint32_t x,
    uint32_t y,
    float transition_t,
    const lod_dither_config_t* config
) {
    float threshold = lod_get_dither_threshold(x, y, transition_t, config);
    
    // During transition (t=0 to t=1):
    // t=0 -> 100% LOD A (transition_t < threshold for all pixels)
    // t=1 -> 100% LOD B (transition_t >= threshold for all pixels)
    
    return transition_t < threshold;
}

/* End of lod_dithering.c */
