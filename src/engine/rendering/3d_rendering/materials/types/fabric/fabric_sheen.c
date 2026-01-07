#include "fabric_rendering.h"
#include <math.h>

/**
 * @file fabric_sheen.c
 * @brief Implementation of fabric edge sheen (Charlie BRDF)
 */

// Charlie BRDF D term (distribution)
static float d_charlie(float roughness, float ndoth) {
    float inv_r = 1.0f / roughness;
    float cos2h = ndoth * ndoth;
    float sin2h = fmaxf(0.0078125f, 1.0f - cos2h);
    return (2.0f + inv_r) * powf(sin2h, inv_r * 0.5f) / (2.0f * PI);
}

float calculate_fabric_sheen(vec3_t V, vec3_t N, vec3_t L, float roughness) {
    // 1. Calculate half vector
    vec3_t H = vec3_normalize(vec3_add(V, L));
    float ndoth = fmaxf(0.0f, vec3_dot(N, H));
    
    // 2. Apply Charlie distribution
    // TODO: Implement proper G (shadowing) and F (fresnel) terms for cloth
    return d_charlie(roughness, ndoth);
}

vec3_t shade_fabric(vec3_t V, vec3_t N, vec3_t L, fabric_params_t* params) {
    // 1. Base diffuse/specular (standard BRDF)
    // TODO: Implement standard Lambert/GGX base
    vec3_t base = params->base_color;
    
    // 2. Add sheen layer
    float sheen = calculate_fabric_sheen(V, N, L, params->roughness);
    vec3_t sheen_layer = vec3_scale(params->sheen_color, sheen * params->sheen_intensity);
    
    return vec3_add(base, sheen_layer);
}

// TODO: Implement velvet-style retro-reflection
// TODO: Implement silk-style anisotropy
