/*
 * cascade_resolution.c
 * Cascade resolution management
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "lighting/cascades/cascade_resolution.h"
#include <include/math/math_all.h>
#include <string.h>

#define MIN_CASCADE_RESOLUTION 256
#define MAX_CASCADE_RESOLUTION 8192



uint32_t cascade_calculate_resolution(float frustum_diagonal_size, float texel_density_target) {
    if (texel_density_target <= 0.0f) return 1024; // Default fallback
    
    // Required pixels = world_size * pixels_per_unit
    float required_pixels = frustum_diagonal_size * texel_density_target;
    
    uint32_t resolution = (uint32_t)ceilf(required_pixels);
    resolution = next_power_of_two(resolution);
    
    if (resolution < MIN_CASCADE_RESOLUTION) resolution = MIN_CASCADE_RESOLUTION;
    if (resolution > MAX_CASCADE_RESOLUTION) resolution = MAX_CASCADE_RESOLUTION;
    
    return resolution;
}

float cascade_calculate_texel_density(uint32_t resolution, float world_diagonal_size) {
    if (world_diagonal_size <= 0.001f) return 0.0f;
    return (float)resolution / world_diagonal_size;
}

void cascade_apply_quality_preset(cascade_quality_preset_t preset, uint32_t* out_resolutions, uint32_t count) {
    if (!out_resolutions || count == 0) return;
    
    uint32_t base_res = 1024;
    
    switch (preset) {
        case CASCADE_QUALITY_LOW:
            base_res = 512;
            break;
        case CASCADE_QUALITY_MEDIUM:
            base_res = 1024;
            break;
        case CASCADE_QUALITY_HIGH:
            base_res = 2048;
            break;
        case CASCADE_QUALITY_ULTRA:
            base_res = 4096;
            break;
    }
    
    // Typically, cascades 0 (nearest) needs highest res, others can drop
    // Strategy: First cascade gets base_res, subsequent ones can optionally drop
    // For high quality, we often keep them uniform or only drop for very distant ones
    
    for (uint32_t i = 0; i < count; i++) {
        // Simple uniform resolution for now to avoid popping
        out_resolutions[i] = base_res;
        
        // Extended logic: Decrease resolution for distant cascades if needed
        // if (preset == CASCADE_QUALITY_LOW && i > 1) out_resolutions[i] = base_res / 2;
    }
}
