/*
 * weathering_system.h
 * Overall weathering controller API
 *
 * Part of the Age Weathering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef AGE_WEATHERING_WEATHERING_SYSTEM_H
#define AGE_WEATHERING_WEATHERING_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "math/vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct weathering_system_handle {
    uint32_t id;
} weathering_system_handle_t;

typedef struct weathering_params {
    float age;                      // 0-1, overall weathering amount
    float rust_amount;              // 0-1, rust intensity
    float dirt_amount;              // 0-1, dirt intensity
    float wear_amount;              // 0-1, wear intensity
    float wetness;                  // 0-1, wetness amount
    
    // Texture handles (using uint32_t for compatibility)
    uint32_t noise_texture;         // 3D noise texture
    uint32_t cavity_map;            // Baked cavity map
    uint32_t curvature_map;         // Baked curvature map
    uint32_t height_map;            // Height map for puddles
    
    // Environmental factors
    vec3_t world_position;          // World position for noise sampling
    vec3_t surface_normal;          // Surface normal for wetness
    float exposure;                 // UV/weather exposure
} weathering_params_t;

typedef struct weathering_system_desc {
    uint32_t flags;
    void* user_data;
} weathering_system_desc_t;

typedef struct weathering_system_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} weathering_system_info_t;

// Material structure for weathering modification (simplified for the API)
typedef struct weathering_material_state {
    vec3_t albedo;
    float roughness;
    float metallic;
    vec2_t uv;
} weathering_material_state_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int weathering_system_init(void);
void weathering_system_shutdown(void);

/* Lifecycle */
int weathering_system_create(weathering_system_handle_t* out_handle, const weathering_system_desc_t* desc);
void weathering_system_destroy(weathering_system_handle_t handle);

/* Operations */
int weathering_system_update_params(weathering_system_handle_t handle, const weathering_params_t* params);
bool weathering_system_is_valid(weathering_system_handle_t handle);

/**
 * Apply weathering effects to a material state.
 * This function simulates the procedural blending of weathering layers.
 */
void weathering_system_apply(weathering_system_handle_t handle, weathering_material_state_t* mat, const weathering_params_t* params);

/* Statistics */
uint32_t weathering_system_get_count(void);
size_t weathering_system_get_memory_usage(void);

#ifdef __cplusplus
}
#endif

#endif /* AGE_WEATHERING_WEATHERING_SYSTEM_H */
