#include "../3d_rendering.h"
#include <math.h>

/* ==================== Stone Surface Detail ==================== */

// Types for handling textures would typically be handles. 
// Here we simulate the logic of combining a macro normal with a detail normal.

typedef struct {
    float detail_scale;     // Tiling frequency of detail map
    float detail_intensity; // Strength of detail normal
    float roughness_blend;  // How much detail affects roughness
} stone_detail_params_t;

// Standard Reoriented Normal Mapping (RNM) or similar blending is ideal.
// Here we use a safe linear blend approximation for simplicity in C code structure.

static Vec3 vec3_normalize(Vec3 v) {
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 0.0f) {
        return (Vec3){v.x / len, v.y / len, v.z / len};
    }
    return (Vec3){0, 0, 1.0f};
}

static Vec3 sample_detail_normal(Vec2 uv, float scale) {
    // Placeholder: In real engine, sample texture(uv * scale)
    // Return dummy vector pointing up-ish
    return (Vec3){0.0f, 0.0f, 1.0f}; 
}

static float sample_detail_roughness(Vec2 uv, float scale) {
    // Placeholder
    return 0.5f; 
}

// Update Normal and Roughness based on detail maps
void apply_stone_detail(Vec3* macro_normal, float* macro_roughness, Vec2 uv, 
                       stone_detail_params_t* params) {
    if (!params) return;

    // 1. Detail Normal
    // Assuming macro_normal is in tangent space for simplicity, typical for material graphs
    // If macro_normal is World Space, we need TBN matrix here. 
    // Let's assume this function operates in Tangent Space or blending relative vectors.
    
    Vec3 detail_N = sample_detail_normal(uv, params->detail_scale);
    
    // Scale detail intensity (UnpackNormal strength)
    // Lerp between (0,0,1) and detail_N
    detail_N.x *= params->detail_intensity;
    detail_N.y *= params->detail_intensity;
    // reconstruct Z or just normalize
    detail_N = vec3_normalize(detail_N);
    
    // Simple UDN (Unreal Detail Normal) blending: N_combined = normalize(N_macro.xy + N_detail.xy, N_macro.z)
    // Note: macro_normal is pointer to vec3
    
    Vec3 blended_N;
    blended_N.x = macro_normal->x + detail_N.x;
    blended_N.y = macro_normal->y + detail_N.y;
    blended_N.z = macro_normal->z; // Simplification, usually z * blending
    
    *macro_normal = vec3_normalize(blended_N);
    
    // 2. Detail Roughness
    float detail_R = sample_detail_roughness(uv, params->detail_scale);
    
    // Blend: Overlay or geometric mean?
    // Let's use simple linear blend guided by parameter
    // detail_R typically centered around 0.5 for invalid/no change? Or 0..1 raw
    // Assuming detail_R is 0..1 noise.
    
    // "Composite" roughness - increase roughness where detail is high?
    // Often detail maps make things rougher.
    float factor = (detail_R - 0.5f) * 2.0f; // -1 to 1 range
    *macro_roughness = *macro_roughness + (factor * params->roughness_blend);
    
    // Clamp
    if (*macro_roughness < 0.0f) *macro_roughness = 0.0f;
    if (*macro_roughness > 1.0f) *macro_roughness = 1.0f;
}
