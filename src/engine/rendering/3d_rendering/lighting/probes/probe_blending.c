/*
 * probe_blending.c
 * Probe blending logic implementation
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "probe_parallax.h"

// SH Basis Constants
static const float SH_C0 = 0.28209479177387814347f; // 1 / (2 * sqrt(pi))
static const float SH_C1 = 0.48860251190291992159f; // sqrt(3) / (2 * sqrt(pi))
static const float SH_C2_1 = 1.09254843059207907054f; // sqrt(15) / (2 * sqrt(pi))
static const float SH_C2_2 = 0.31539156525252000603f; // sqrt(5) / (4 * sqrt(pi))
static const float SH_C2_3 = 0.54627421529603953527f; // sqrt(15) / (4 * sqrt(pi))

static simd_float3 evaluate_sh(const simd_float4* sh, simd_float3 n) {
    simd_float3 result = simd_make_float3(0.0f, 0.0f, 0.0f);

    // L0
    // Y0,0 = 0.282095
    float y00 = SH_C0;
    
    // L1
    // Y1,-1 = 0.488603 * y
    // Y1,0  = 0.488603 * z
    // Y1,1  = 0.488603 * x
    float y1m1 = SH_C1 * n.y;
    float y10  = SH_C1 * n.z;
    float y11  = SH_C1 * n.x;

    // L2
    // Y2,-2 = 1.092548 * x * y
    // Y2,-1 = 1.092548 * y * z
    // Y2,0  = 0.315392 * (3 * z * z - 1)
    // Y2,1  = 1.092548 * x * z
    // Y2,2  = 0.546274 * (x * x - y * y)
    float y2m2 = SH_C2_1 * n.x * n.y;
    float y2m1 = SH_C2_1 * n.y * n.z;
    float y20  = SH_C2_2 * (3.0f * n.z * n.z - 1.0f);
    float y21  = SH_C2_1 * n.x * n.z;
    float y22  = SH_C2_3 * (n.x * n.x - n.y * n.y);

    // Accumulate
    // Coeffs are stored as 9 float4s. Each float4 is (R, G, B, X).
    
    // L0
    result += sh[0].xyz * y00;

    // L1
    result += sh[1].xyz * y1m1;
    result += sh[2].xyz * y10;
    result += sh[3].xyz * y11;

    // L2
    result += sh[4].xyz * y2m2;
    result += sh[5].xyz * y2m1;
    result += sh[6].xyz * y20;
    result += sh[7].xyz * y21;
    result += sh[8].xyz * y22;

    return result;
}

float probe_calculate_weight(simd_float3 world_pos, const irradiance_probe_t* probe) {
    if (!probe) return 0.0f;
    
    float dist = simd_distance(world_pos, probe->position);
    float r = probe->influence_radius;
    
    if (dist >= r) return 0.0f;
    
    // Smooth falloff
    // x = dist / radius
    // weight = 1 - x (linear) or smoothstep
    float x = dist / r;
    // Quadratic falloff is better: 1 - x^2? 
    // Or just linear for trilinear interpolation conceptualization.
    // However, for blending arbitrary probes, smoothstep is good.
    // smoothstep(1, 0, x)
    
    float t = simd_clamp(1.0f - x, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

simd_float3 probe_blend_irradiance(const uint32_t* probe_indices, const float* weights, uint32_t count, const probe_grid_t* grid, simd_float3 normal) {
    if (!grid || !probe_indices || !weights || count == 0) return simd_make_float3(0.0f, 0.0f, 0.0f);
    
    simd_float3 accumulated_irradiance = 0;
    float total_weight = 0.0f;
    
    for (uint32_t i = 0; i < count; i++) {
        uint32_t idx = probe_indices[i];
        if (idx >= grid->probe_count) continue;
        
        float w = weights[i];
        if (w <= 0.0f) continue;
        
        simd_float3 irradiance = evaluate_sh(grid->probes[idx].sh_coefficients, normal);
        accumulated_irradiance += irradiance * w;
        total_weight += w;
    }
    
    if (total_weight > 0.0f) {
        return accumulated_irradiance / total_weight;
    }
    
    return accumulated_irradiance;
}
