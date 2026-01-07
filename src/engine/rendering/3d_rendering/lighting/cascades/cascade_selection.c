/*
 * cascade_selection.c
 * Runtime cascade selection logic
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "cascade_selection.h"
#include <string.h>

void cascade_calculate_split_depths(float near_plane, float far_plane, float lambda, uint32_t count, float* out_splits) {
    if (!out_splits || count == 0) return;
    
    // Practical Split Scheme (PSNM)
    // d_i = lambda * d_log + (1 - lambda) * d_uni
    
    // Using shadow distance instead of far_plane usually results in better quality
    float range = far_plane - near_plane;
    float ratio = far_plane / near_plane;
    
    for (uint32_t i = 0; i < count; i++) {
        float p = (float)(i + 1) / (float)count;
        
        float log_split = near_plane * powf(ratio, p);
        float uni_split = near_plane + range * p;
        
        float d = lambda * log_split + (1.0f - lambda) * uni_split;
        out_splits[i] = d;
    }
}

void cascade_calculate_transition_regions(const float* splits, uint32_t count, float transition_fraction, float* out_regions) {
    if (!splits || !out_regions || count == 0) return;
    
    // Calculate start/end of transition zone for each cascade
    // Typically, we blend from split - transition to split + transition?
    // Or just fade out at the edge.
    // Here we return a vec4 per cascade: {split_start, split_end, fade_start, fade_end} or similar.
    // For PCF, usually we just need the split distances and a fade factor.
    
    for (uint32_t i = 0; i < count; i++) {
        // Simple storage of split distance
        out_regions[i] = splits[i];
    }
}
