/*
 * rust_color.c
 * Rust color variation implementation
 *
 * Part of the Rust Corrosion subsystem
 * Advanced 3D Rendering Engine
 */

#include "rust_color.h"
#include "../../../include/math/math.h"
#include <math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

static const vec3_t RUST_PALETTE[] = {
    {0.75f, 0.35f, 0.10f}, // FRESH: Bright orange/yellow-red
    {0.44f, 0.22f, 0.13f}, // AGED: Standard brown-red
    {0.25f, 0.12f, 0.08f}  // DEEP: Very dark brown
};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

vec3_t rust_color_sample(const rust_color_params_t* params, float mask, vec3_t world_pos) {
    if (mask < 0.01f) return (vec3_t){0,0,0};

    // Determine base stage based on oxidation level and mask (deeper mask = older rust)
    float stage_val = params->oxidation_level * 2.0f + mask * 0.5f;
    stage_val = saturate(stage_val / (float)RUST_STAGE_COUNT);
    
    int index_a = (int)(stage_val * (RUST_STAGE_COUNT - 1));
    int index_b = min_int(index_a + 1, RUST_STAGE_COUNT - 1);
    float t = (stage_val * (RUST_STAGE_COUNT - 1)) - (float)index_a;
    
    vec3_t base_color = vec3_lerp(RUST_PALETTE[index_a], RUST_PALETTE[index_b], t);
    
    // Add spatial variance
    float variation = (sinf(world_pos.x * 10.0f) + cosf(world_pos.y * 11.0f) + sinf(world_pos.z * 12.0f)) * 0.05f;
    variation *= params->variance;
    
    return vec3_add_scalar(base_color, variation);
}

void rust_get_pbr_properties(float mask, float oxidation, float* out_roughness, float* out_metallic) {
    if (!out_roughness || !out_metallic) return;
    
    // Rust is always non-metal and very rough
    // Deeper oxidation is rougher
    *out_roughness = lerp(0.7f, 1.0f, oxidation * mask);
    *out_metallic = lerp(0.1f, 0.0f, mask); // Slight specular for very fresh rust? usually 0.
}
