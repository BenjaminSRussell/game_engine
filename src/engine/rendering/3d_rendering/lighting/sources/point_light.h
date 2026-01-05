/*
 * point_light.h
 * Point light rendering
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_POINT_LIGHT_H
#define LIGHTING_POINT_LIGHT_H

#include "../../math/vec3.h"
#include "../../math/aabb.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_point_light_handle {
    uint32_t id;
} lighting_point_light_handle_t;

/**
 * Point light parameters
 */
typedef struct point_light_params {
    vec3_t position;
    vec3_t color;
    float intensity;
    float radius;
    float falloff_exponent; // Usually 2.0 for physical accuracy
    bool cast_shadows;
    float shadow_bias;
    void* user_data;
} point_light_params_t;

/**
 * Full point light structure
 */
typedef struct point_light {
    vec3_t position;
    vec3_t color;
    float intensity;
    float radius;
    float inverse_radius_sq; // 1.0 / (radius * radius)
    float falloff_exponent;
    bool cast_shadows;
    float shadow_bias;
    uint32_t shadow_map_index;
    
    // Runtime data
    bool active;
    uint32_t flags;
} point_light_t;

typedef struct lighting_point_light_desc {
    point_light_params_t params;
    uint32_t flags;
} lighting_point_light_desc_t;

typedef struct lighting_point_light_info {
    uint32_t id;
    point_light_t data;
    bool initialized;
} lighting_point_light_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lighting_point_light_init(void);
void lighting_point_light_shutdown(void);

/* Lifecycle */
int lighting_point_light_create(lighting_point_light_handle_t* out_handle, const lighting_point_light_desc_t* desc);
void lighting_point_light_destroy(lighting_point_light_handle_t handle);

/* Operations */
int lighting_point_light_update(lighting_point_light_handle_t handle, const point_light_params_t* params);
bool lighting_point_light_is_valid(lighting_point_light_handle_t handle);
int lighting_point_light_get_info(lighting_point_light_handle_t handle, lighting_point_light_info_t* out_info);
void lighting_point_light_mark_dirty(lighting_point_light_handle_t handle);
int lighting_point_light_process_pending(void);

/* Rendering & Calculation */
/**
 * Evaluate point light contribution at a specific position
 * @param handle Light handle
 * @param position World space position to illuminate
 * @param out_color Resulting light color * intensity * attenuation
 * @param out_direction Direction from position to light (normalized)
 * @param out_attenuation Scalar attenuation value
 */
int lighting_point_light_evaluate(
    lighting_point_light_handle_t handle,
    vec3_t position,
    vec3_t* out_color,
    vec3_t* out_direction,
    float* out_attenuation
);

/**
 * Get the bounding sphere of the light for culling
 * @param handle Light handle
 * @param out_center Center of the sphere
 * @param out_radius Radius of the sphere
 */
int lighting_point_light_get_bounds(
    lighting_point_light_handle_t handle,
    vec3_t* out_center,
    float* out_radius
);

/**
 * Get the bounding box of the light for culling
 */
aabb_t lighting_point_light_get_aabb(lighting_point_light_handle_t handle);

/* Statistics */
uint32_t lighting_point_light_get_count(void);
size_t lighting_point_light_get_memory_usage(void);
void lighting_point_light_debug_print(void);

/* GPU Data Access */
/**
 * specific structure for GPU buffer upload
 */
typedef struct point_light_gpu_data {
    float position[3];
    float radius;
    float color[3];
    float intensity;
    float inverse_radius_sq;
    uint32_t shadow_map_index;
    float padding[2];
} point_light_gpu_data_t;

/**
 * Write all active lights to a GPU-ready buffer
 * @param buffer Output buffer
 * @param max_count Maximum number of lights the buffer can hold
 * @return Number of lights written
 */
uint32_t lighting_point_light_write_gpu_data(point_light_gpu_data_t* buffer, uint32_t max_count);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_POINT_LIGHT_H */
