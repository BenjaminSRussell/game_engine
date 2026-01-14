/*
 * rect_light.h
 * Rectangular area lights (LTC)
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_RECT_LIGHT_H
#define LIGHTING_RECT_LIGHT_H

#include "include/math/vec3.h"
#include "include/math/mat4.h"
#include "include/math/quat.h"
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

typedef struct lighting_rect_light_handle {
    uint32_t id;
} lighting_rect_light_handle_t;

/**
 * Rect light parameters
 */
typedef struct rect_light_params {
    vec3_t position;
    vec3_t axis_x; // Local X axis (width direction) - normalized
    vec3_t axis_y; // Local Y axis (height direction) - normalized
    vec3_t color;
    float intensity;
    
    // Dimensions
    float width;
    float height;
    
    // Barn door effects or attenuation limits?
    float range; 
    
    // Textures
    bool use_texture;
    // texture_handle_t texture; // Forward declaration or void* if not avail
    
    bool two_sided;
    void* user_data;
} rect_light_params_t;

/**
 * Full rect light structure
 */
typedef struct rect_light {
    vec3_t position;
    vec3_t axis_x; 
    vec3_t axis_y;
    vec3_t normal; // Computed: cross(axis_x, axis_y)
    
    vec3_t color;
    float intensity;
    float width;
    float height;
    float half_width;
    float half_height;
    
    float range;
    bool two_sided;
    
    // LTC precomputed points (corners in local space or similar)
    vec3_t points[4]; 
    
    // Runtime data
    bool active;
    uint32_t flags;
} rect_light_t;

typedef struct lighting_rect_light_desc {
    rect_light_params_t params;
    uint32_t flags;
} lighting_rect_light_desc_t;

typedef struct lighting_rect_light_info {
    uint32_t id;
    rect_light_t data;
    bool initialized;
} lighting_rect_light_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lighting_rect_light_init(void);
void lighting_rect_light_shutdown(void);

/* Lifecycle */
int lighting_rect_light_create(lighting_rect_light_handle_t* out_handle, const lighting_rect_light_desc_t* desc);
void lighting_rect_light_destroy(lighting_rect_light_handle_t handle);

/* Operations */
int lighting_rect_light_update(lighting_rect_light_handle_t handle, const rect_light_params_t* params);
bool lighting_rect_light_is_valid(lighting_rect_light_handle_t handle);
int lighting_rect_light_get_info(lighting_rect_light_handle_t handle, lighting_rect_light_info_t* out_info);
void lighting_rect_light_mark_dirty(lighting_rect_light_handle_t handle);
int lighting_rect_light_process_pending(void);

/* Rendering & Calculation */
/**
 * Evaluate rect light using Linearly Transformed Cosines (LTC)
 * Note: Actual LTC approximation usually needs specular roughness and view vector
 * This is a simplified interface for debugging
 */
int lighting_rect_light_evaluate_simple(
    lighting_rect_light_handle_t handle,
    vec3_t position,
    vec3_t normal,
    vec3_t* out_color
);

aabb_t lighting_rect_light_get_aabb(lighting_rect_light_handle_t handle);

/* Statistics */
uint32_t lighting_rect_light_get_count(void);
size_t lighting_rect_light_get_memory_usage(void);
void lighting_rect_light_debug_print(void);

/* GPU Data Access */
typedef struct rect_light_gpu_data {
    float position[3];
    float intensity;
    float axis_x[3];
    float half_width;
    float axis_y[3];
    float half_height;
    float color[3];
    float range;
    float points[12]; // 4 points * 3 floats (corners)
} rect_light_gpu_data_t;

uint32_t lighting_rect_light_write_gpu_data(rect_light_gpu_data_t* buffer, uint32_t max_count);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_RECT_LIGHT_H */
