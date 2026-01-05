/*
 * spot_light.h
 * Spotlight rendering
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_SPOT_LIGHT_H
#define LIGHTING_SPOT_LIGHT_H

#include "../../math/vec3.h"
#include "../../math/aabb.h"
#include "../../math/frustum.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_spot_light_handle {
    uint32_t id;
} lighting_spot_light_handle_t;

/**
 * Spot light parameters
 */
typedef struct spot_light_params {
    vec3_t position;
    vec3_t direction; // Direction the spotlight is facing (normalized)
    vec3_t color;
    float intensity;
    float radius;
    float falloff_exponent; // Linear distance falloff (usually 2.0)
    
    // Cone angles in radians
    float inner_cone_angle; // Full intensity within this angle
    float outer_cone_angle; // Falloff to zero at this angle
    
    bool cast_shadows;
    float shadow_bias;
    void* user_data;
} spot_light_params_t;

/**
 * Full spot light structure
 */
typedef struct spot_light {
    vec3_t position;
    vec3_t direction;
    vec3_t color;
    
    float intensity;
    float radius;
    float inverse_radius_sq;
    float falloff_exponent;
    
    // Cone parameters
    float inner_cone_angle;
    float outer_cone_angle;
    float inner_cone_cos; // cos(inner_cone_angle)
    float outer_cone_cos; // cos(outer_cone_angle)
    float cone_scale;     // 1.0 / (inner_cos - outer_cos)
    float cone_offset;    // -outer_cos * cone_scale
    
    bool cast_shadows;
    float shadow_bias;
    uint32_t shadow_map_index;
    
    // Runtime data
    bool active;
    uint32_t flags;
} spot_light_t;

typedef struct lighting_spot_light_desc {
    spot_light_params_t params;
    uint32_t flags;
} lighting_spot_light_desc_t;

typedef struct lighting_spot_light_info {
    uint32_t id;
    spot_light_t data;
    bool initialized;
} lighting_spot_light_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lighting_spot_light_init(void);
void lighting_spot_light_shutdown(void);

/* Lifecycle */
int lighting_spot_light_create(lighting_spot_light_handle_t* out_handle, const lighting_spot_light_desc_t* desc);
void lighting_spot_light_destroy(lighting_spot_light_handle_t handle);

/* Operations */
int lighting_spot_light_update(lighting_spot_light_handle_t handle, const spot_light_params_t* params);
bool lighting_spot_light_is_valid(lighting_spot_light_handle_t handle);
int lighting_spot_light_get_info(lighting_spot_light_handle_t handle, lighting_spot_light_info_t* out_info);
void lighting_spot_light_mark_dirty(lighting_spot_light_handle_t handle);
int lighting_spot_light_process_pending(void);

/* Rendering & Calculation */
/**
 * Evaluate spot light contribution at a specific position
 */
int lighting_spot_light_evaluate(
    lighting_spot_light_handle_t handle,
    vec3_t position,
    vec3_t* out_color,
    vec3_t* out_direction,
    float* out_attenuation
);

/**
 * Get the bounding sphere of the light for fast culling
 */
int lighting_spot_light_get_bounds(
    lighting_spot_light_handle_t handle,
    vec3_t* out_center,
    float* out_radius
);

/**
 * Get the bounding box of the light
 */
aabb_t lighting_spot_light_get_aabb(lighting_spot_light_handle_t handle);

/* Statistics */
uint32_t lighting_spot_light_get_count(void);
size_t lighting_spot_light_get_memory_usage(void);
void lighting_spot_light_debug_print(void);

/* GPU Data Access */
typedef struct spot_light_gpu_data {
    float position[3];
    float radius;
    float direction[3];
    float intensity;
    float color[3];
    float inverse_radius_sq;
    float cone_scale;
    float cone_offset;
    uint32_t shadow_map_index;
    float padding;
} spot_light_gpu_data_t;

uint32_t lighting_spot_light_write_gpu_data(spot_light_gpu_data_t* buffer, uint32_t max_count);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_SPOT_LIGHT_H */
