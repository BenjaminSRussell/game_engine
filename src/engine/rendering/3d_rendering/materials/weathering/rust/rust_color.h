/*
 * rust_color.h
 * Rust color variation API
 *
 * Part of the Rust Corrosion subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RUST_CORROSION_RUST_COLOR_H
#define RUST_CORROSION_RUST_COLOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../math/vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum rust_stage {
    RUST_STAGE_FRESH,       // Bright orange, surface rust
    RUST_STAGE_AGED,        // Brownish red, established
    RUST_STAGE_DEEP,        // Dark brown, deep corrosion
    RUST_STAGE_COUNT
} rust_stage_t;

typedef struct rust_color_params {
    float oxidation_level;  // 0-1, overall rust age
    float variance;         // Color randomness
} rust_color_params_t;

/**
 * Get the color for a specific rust mask value and oxidation level.
 */
vec3_t rust_color_sample(const rust_color_params_t* params, float mask, vec3_t world_pos);

/**
 * Get the PBR properties (roughness, metallic) for rust.
 */
void rust_get_pbr_properties(float mask, float oxidation, float* out_roughness, float* out_metallic);

#ifdef __cplusplus
}
#endif

#endif /* RUST_CORROSION_RUST_COLOR_H */
