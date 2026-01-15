/*
 * lod_dithering.h
 * Temporal dithering for LOD transitions
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_LOD_DITHERING_H
#define GEOMETRY_LOD_DITHERING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

// Dither pattern type
typedef enum lod_dither_pattern {
    LOD_DITHER_BAYER_4x4,
    LOD_DITHER_BAYER_8x8,
    LOD_DITHER_BLUE_NOISE,
    LOD_DITHER_INTERLEAVED_GRADIENT,
    LOD_DITHER_TEMPORAL,
    LOD_DITHER_COUNT
} lod_dither_pattern_t;

// Dither configuration
typedef struct lod_dither_config {
    lod_dither_pattern_t pattern;
    float dither_strength;      // Dither strength [0-1]
    bool temporal_jitter;       // Use temporal jittering
    uint32_t frame_index;       // Current frame for temporal patterns
} lod_dither_config_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Configuration */
lod_dither_config_t lod_dither_get_default_config(void);

/* Dither Value Calculation */
// Get dither threshold for pixel at (x, y) with transition parameter t
float lod_get_dither_threshold(
    uint32_t x,
    uint32_t y,
    float transition_t,
    const lod_dither_config_t* config
);

// Check if pixel should render LOD A (true) or LOD B (false) during transition
bool lod_should_render_lod_a(
    uint32_t x,
    uint32_t y,
    float transition_t,
    const lod_dither_config_t* config
);

/* Pattern Generation */
float lod_dither_bayer_4x4(uint32_t x, uint32_t y);
float lod_dither_bayer_8x8(uint32_t x, uint32_t y);
float lod_dither_interleaved_gradient(uint32_t x, uint32_t y, uint32_t frame);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_LOD_DITHERING_H */
