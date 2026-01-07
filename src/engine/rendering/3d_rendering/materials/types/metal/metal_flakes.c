#include "../3d_rendering.h"
#include <math.h>
#include <stdlib.h> // for rand() placeholder if needed

/* ==================== Car Paint Metallic Flakes ==================== */

// TODO: Reuse math helpers
static Vec3 vec3_normalize(Vec3 v) {
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 0.0f) {
        return (Vec3){v.x / len, v.y / len, v.z / len};
    }
    return (Vec3){0, 0, 0};
}

static float vec3_dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static Vec3 vec3_add(Vec3 a, Vec3 b) {
    return (Vec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

static Vec3 vec3_scale(Vec3 v, float s) {
    return (Vec3){v.x * s, v.y * s, v.z * s};
}

typedef struct {
    float flake_density;
    float flake_size;
    float flake_roughness;
    Vec3 flake_color;
    float flake_normal_perturbation_strength;
} car_paint_flakes_t;

// Pseudo-random function for procedural noise
static float hash12(float x, float y) {
    return fmodf(sinf(x * 12.9898f + y * 78.233f) * 43758.5453123f, 1.0f);
}

// Simulates sparkly flakes by modifying the normal at high frequency
// and checking for high specular highlights
Vec3 shade_metallic_flakes(Vec3 N, Vec3 V, Vec3 L, Vec2 uv, car_paint_flakes_t* params) {
    if (!params) return (Vec3){0, 0, 0};

    Vec3 H = vec3_normalize(vec3_add(V, L));
    
    // 1. Procedural Flake Generation
    // Scale UV by density/size
    float scale = 1000.0f * (1.0f / fmaxf(params->flake_size, 0.001f));
    Vec2 scaled_uv = {uv.x * scale, uv.y * scale};
    
    // Cell ID for noise
    float cell_x = floorf(scaled_uv.x);
    float cell_y = floorf(scaled_uv.y);
    
    // Hash to determine if this cell has a flake
    float has_flake_prob = hash12(cell_x, cell_y);
    
    if (has_flake_prob > params->flake_density) {
        // No flake in this cell, return practically nothing
        return (Vec3){0, 0, 0};
    }
    
    // 2. Perturb Normal for Flake
    // Generate a random normal deviation for the flake
    float rx = hash12(cell_x + 3.41f, cell_y + 1.23f) * 2.0f - 1.0f;
    float ry = hash12(cell_x - 1.55f, cell_y + 8.11f) * 2.0f - 1.0f;
    float rz = sqrtf(fmaxf(1.0f - rx*rx - ry*ry, 0.0f)); // Z is usually positive in variance
    
    // Simple tangent space perturbation (assuming N is roughly Z in tangent space for this micro facet)
    // For object space, we would need T and B. Here we just add noise to N assuming N is world space.
    // NOTE: This is a simplified "sparkle" hack.
    
    Vec3 flake_normal_local = {rx, ry, rz};
    
    // Blend with macroscopic normal
    Vec3 flake_N = vec3_add(N, vec3_scale(flake_normal_local, params->flake_normal_perturbation_strength));
    flake_N = vec3_normalize(flake_N);
    
    // 3. Calculate Specular for Flake
    float NdotH = fmaxf(vec3_dot(flake_N, H), 0.0f);
    
    // Flakes work like tiny mirrors, very low roughness
    float roughness = params->flake_roughness;
    float spread = roughness * roughness;
    // Phong-like specular for sparkles is often sharper/cheaper or GGX
    // Using simple Blinn-Phong power for intense sparkle
    float shininess = 200.0f / fmaxf(spread, 0.001f);
    
    float spec_intensity = powf(NdotH, shininess);
    
    // Mask by NdotL for self-shadowing
    float NdotL = fmaxf(vec3_dot(N, L), 0.0f);
    
    return (Vec3){
        params->flake_color.x * spec_intensity * NdotL,
        params->flake_color.y * spec_intensity * NdotL,
        params->flake_color.z * spec_intensity * NdotL
    };
}
