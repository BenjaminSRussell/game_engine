/*
 * cascade_blending.c
 * Seamless cascade blending
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "cascade_blending.h"

void cascade_calculate_blend_bands(const float* splits, uint32_t count, float blend_fraction, 
                                  cascade_blend_info_t* out_bands) {
    if (!splits || !out_bands || count == 0) return;
    
    // blend_fraction is how much overlap we want relative to cascade size
    // e.g., 0.1 means 10% of the cascade range is for blending
    
    float previous_split = 0.0f; // Start at near plane (usually)
    
    for (uint32_t i = 0; i < count; i++) {
        float split_dist = splits[i];
        float range = split_dist - previous_split;
        float band_size = range * blend_fraction;
        
        out_bands[i].end_depth = split_dist;
        out_bands[i].start_fade = split_dist - band_size;
        out_bands[i].blend_size = band_size;
        
        previous_split = split_dist;
    }
}
