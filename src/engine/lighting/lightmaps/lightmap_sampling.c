/*
 * lightmap_sampling.c
 * Lightmap Sampling Logic
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "lighting/lightmaps/lightmap_sampling.h"
#include "include/math/vec2.h"
#include "include/math/vec3.h"

/*
 * Provides functions for manual CPU sampling of lightmaps, 
 * useful for light probes generation or dynamic object integration.
 */

vec3_t lightmap_sample_bilinear(void* texture_data, int width, int height, vec2_t uv) {
    // 1. Convert UV to pixel coords
    float px = uv.x * width;
    float py = uv.y * height;
    
    // 2. Sample 4 neighbors
    // 3. Output weighted average
    
    return (vec3_t){1.0f, 1.0f, 1.0f}; // Stub: white light
}

vec3_t lightmap_sample_bicubic(void* texture_data, int width, int height, vec2_t uv) {
    // Smoother sampling
    return (vec3_t){1.0f, 1.0f, 1.0f};
}
