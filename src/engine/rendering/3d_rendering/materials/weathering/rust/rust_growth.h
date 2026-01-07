/*
 * rust_growth.h
 * Procedural rust patterns API
 *
 * Part of the Rust Corrosion subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RUST_CORROSION_RUST_GROWTH_H
#define RUST_CORROSION_RUST_GROWTH_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../math/vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rust_growth_handle {
    uint32_t id;
} rust_growth_handle_t;

typedef struct rust_pattern_params {
    float spread_rate;          // How fast rust spreads from edges
    float pitting_intensity;    // Deep vs surface rust
    float noise_scale;          // Detail of rust patterns
    uint32_t noise_texture;     // Texture handle for variation
} rust_pattern_params_t;

typedef struct rust_growth_desc {
    uint32_t flags;
    void* user_data;
} rust_growth_desc_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int rust_growth_init(void);
void rust_growth_shutdown(void);

/* Lifecycle */
int rust_growth_create(rust_growth_handle_t* out_handle, const rust_growth_desc_t* desc);
void rust_growth_destroy(rust_growth_handle_t handle);

/**
 * Calculate rust mask based on curvature, noise, and growth parameters.
 * returns a float mask (0-1) where 1 is fully rusted.
 */
float rust_growth_calculate_mask(rust_growth_handle_t handle, 
                               const rust_pattern_params_t* params,
                               vec3_t world_pos,
                               float curvature,
                               float salt_exposure);

/* Statistics */
uint32_t rust_growth_get_count(void);
size_t rust_growth_get_memory_usage(void);

#ifdef __cplusplus
}
#endif

#endif /* RUST_CORROSION_RUST_GROWTH_H */
