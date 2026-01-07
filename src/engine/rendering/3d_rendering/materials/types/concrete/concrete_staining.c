#include "../3d_rendering.h"
#include <math.h>

/* ==================== Concrete Surface Staining ==================== */

typedef struct {
    float stain_intensity;
    Vec3 stain_tint;
    float roughness_bias; // Stains often wet or grimy (smoother or rougher)
} concrete_stain_params_t;

static float sample_stain_map(ResourceHandle stain_map, Vec2 uv) {
    return 0.0f; // 0 = no stain, 1 = full stain
}

static float mix(float a, float b, float t) {
    return a * (1.0f - t) + b * t;
}

void apply_concrete_staining(Vec3* albedo, float* roughness, Vec2 uv,
                            ResourceHandle stain_map, concrete_stain_params_t* params) {
    if (!params) return;

    float stain_mask = sample_stain_map(stain_map, uv);
    
    // Apply strength
    stain_mask *= params->stain_intensity;
    if (stain_mask > 1.0f) stain_mask = 1.0f;
    
    // Blend Albedo
    // Multiply blend often looks best for stains (darkening)
    // Or Lerp to tint
    
    Vec3 original = *albedo;
    Vec3 stained;
    stained.x = original.x * params->stain_tint.x;
    stained.y = original.y * params->stain_tint.y;
    stained.z = original.z * params->stain_tint.z;
    
    albedo->x = mix(original.x, stained.x, stain_mask);
    albedo->y = mix(original.y, stained.y, stain_mask);
    albedo->z = mix(original.z, stained.z, stain_mask);
    
    // Modify Roughness
    // If bias is negative -> smoother (wet stain)
    // If bias is positive -> rougher (dried dirt)
    *roughness = *roughness + (params->roughness_bias * stain_mask);
    
    if (*roughness < 0.0f) *roughness = 0.0f;
    if (*roughness > 1.0f) *roughness = 1.0f;
}
