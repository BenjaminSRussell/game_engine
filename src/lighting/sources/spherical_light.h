/*
 * spherical_light.h
 * Spherical area light rendering
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_SPHERICAL_LIGHT_H
#define LIGHTING_SPHERICAL_LIGHT_H

#include "include/math/vec3.h"
#include "include/math/aabb.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_spherical_light_handle {
    uint32_t id;
} lighting_spherical_light_handle_t;

/**
 * Spherical light parameters
 */
typedef struct spherical_light_params {
    vec3_t position;
    vec3_t color;
    float intensity;
    float radius; // Physical radius of the sphere emitter
    float influence_radius; // Max range of influence
    
    // Falloff control
    float falloff_exponent;
    
    bool cast_shadows;
    float shadow_bias;
    void* user_data;
} spherical_light_params_t;

/**
 * Full spherical light structure
 */
typedef struct spherical_light {
    vec3_t position;
    vec3_t color;
    float intensity;
    
    float radius;
    float radius_sq;
    float influence_radius;
    float influence_radius_sq;
    float inverse_influence_radius_sq;
    
    float falloff_exponent;
    
    bool cast_shadows;
    float shadow_bias;
    uint32_t shadow_map_index; // Often 0, as area lights might not cast simple shadows
    
    // Runtime data
    bool active;
    uint32_t flags;
} spherical_light_t;

typedef struct lighting_spherical_light_desc {
    spherical_light_params_t params;
    uint32_t flags;
} lighting_spherical_light_desc_t;

typedef struct lighting_spherical_light_info {
    uint32_t id;
    spherical_light_t data;
    bool initialized;
} lighting_spherical_light_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lighting_spherical_light_init(void);
void lighting_spherical_light_shutdown(void);

/* Lifecycle */
int lighting_spherical_light_create(lighting_spherical_light_handle_t* out_handle, const lighting_spherical_light_desc_t* desc);
void lighting_spherical_light_destroy(lighting_spherical_light_handle_t handle);

/* Operations */
int lighting_spherical_light_update(lighting_spherical_light_handle_t handle, const spherical_light_params_t* params);
bool lighting_spherical_light_is_valid(lighting_spherical_light_handle_t handle);
int lighting_spherical_light_get_info(lighting_spherical_light_handle_t handle, lighting_spherical_light_info_t* out_info);
void lighting_spherical_light_mark_dirty(lighting_spherical_light_handle_t handle);
int lighting_spherical_light_process_pending(void);

/* Rendering & Calculation */
/**
 * Evaluate spherical light contribution (Karis 2013 / Representative Point Method)
 */
int lighting_spherical_light_evaluate(
    lighting_spherical_light_handle_t handle,
    vec3_t position,
    vec3_t normal,
    vec3_t view_dir,
    float roughness,
    vec3_t* out_color,
    vec3_t* out_direction, /* To representative point */
    float* out_attenuation
);

aabb_t lighting_spherical_light_get_aabb(lighting_spherical_light_handle_t handle);

/* Statistics */
uint32_t lighting_spherical_light_get_count(void);
size_t lighting_spherical_light_get_memory_usage(void);
void lighting_spherical_light_debug_print(void);

/* GPU Data Access */
typedef struct spherical_light_gpu_data {
    float position[3];
    float radius;
    float color[3];
    float intensity;
    float influence_radius;
    float inverse_influence_radius_sq;
    float padding[2];
} spherical_light_gpu_data_t;

uint32_t lighting_spherical_light_write_gpu_data(spherical_light_gpu_data_t* buffer, uint32_t max_count);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_SPHERICAL_LIGHT_H */
