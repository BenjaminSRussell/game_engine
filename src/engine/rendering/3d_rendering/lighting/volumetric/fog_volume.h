/*
 * fog_volume.h
 * Fog volume primitives and management
 */

#ifndef LIGHTING_FOG_VOLUME_H
#define LIGHTING_FOG_VOLUME_H

#include <stdint.h>
#include <stdbool.h>
#include "../../core/math/math/vec3.h"
#include "../../core/math/math/mat4.h"
#include "froxel_grid.h"

#ifdef __cplusplus
extern "C" {
#endif

// Fog volume shape types
typedef enum fog_volume_shape {
    FOG_VOLUME_SHAPE_BOX,
    FOG_VOLUME_SHAPE_SPHERE,
    FOG_VOLUME_SHAPE_CAPSULE,
    FOG_VOLUME_SHAPE_GLOBAL,  // Infinite volumefor atmospheric fog
} fog_volume_shape_t;

// Fog volume parameters
typedef struct fog_volume_params {
    // Scattering properties
    float density;              // Overall fog density
    float absorption;           // Absorption coefficient
    float scattering;           // Scattering coefficient
    float anisotropy;           // Phase function g-factor [-1, 1]
    
    // Visual properties
    vec3_t color;               // Fog tint color
    vec3_t emission;            // Self-illumination
    
    // Attenuation
    float height_falloff;       // Exponential height decay
    float distance_falloff;     // Distance-based attenuation
} fog_volume_params_t;

// Transform for fog volumes
typedef struct fog_volume_transform {
    vec3_t position;
    vec3_t rotation;  // Euler angles (pitch, yaw, roll)
    vec3_t scale;
    mat4_t matrix;    // Cached transform matrix
    mat4_t inv_matrix; // Cached inverse
    bool dirty;       // Matrix needs recalculation
} fog_volume_transform_t;

// Fog volume instance
typedef struct fog_volume {
    uint32_t id;
    fog_volume_shape_t shape;
    fog_volume_params_t params;
    fog_volume_transform_t transform;
    
    // Shape-specific data
    union {
        struct {
            vec3_t extents;  // Half-extents
        } box;
        
        struct {
            float radius;
        } sphere;
        
        struct {
            float radius;
            float height;  // Total height (not half)
        } capsule;
    };
    
    // State
    bool active;
    int32_t priority;  // Higher priority volumes override lower ones
    
    // Bounds (AABB in world space)
    vec3_t bounds_min;
    vec3_t bounds_max;
} fog_volume_t;

// Fog volume manager
#define MAX_FOG_VOLUMES 64

typedef struct fog_volume_manager {
    fog_volume_t volumes[MAX_FOG_VOLUMES];
    uint32_t active_count;
    uint32_t next_id;
    
    // Global fog parameters
    fog_volume_params_t global_params;
    bool global_fog_enabled;
} fog_volume_manager_t;

/**
 * Initializes the fog volume manager.
 */
void fog_volume_manager_init(fog_volume_manager_t* manager);

/**
 * Shuts down the fog volume manager.
 */
void fog_volume_manager_shutdown(fog_volume_manager_t* manager);

/**
 * Creates a new fog volume.
 * Returns the volume ID, or 0 on failure.
 */
uint32_t fog_volume_create(fog_volume_manager_t* manager, fog_volume_shape_t shape,
                            const fog_volume_params_t* params);

/**
 * Destroys a fog volume by ID.
 */
void fog_volume_destroy(fog_volume_manager_t* manager, uint32_t id);

/**
 * Gets a fog volume by ID.
 */
fog_volume_t* fog_volume_get(fog_volume_manager_t* manager, uint32_t id);

/**
 * Updates fog volume parameters.
 */
void fog_volume_set_params(fog_volume_t* volume, const fog_volume_params_t* params);

/**
 * Sets fog volume transform (position, rotation, scale).
 */
void fog_volume_set_transform(fog_volume_t* volume, vec3_t position, vec3_t rotation, vec3_t scale);

/**
 * Sets box extents (for box volumes).
 */
void fog_volume_set_box_extents(fog_volume_t* volume, vec3_t extents);

/**
 * Sets sphere radius (for sphere volumes).
 */
void fog_volume_set_sphere_radius(fog_volume_t* volume, float radius);

/**
 * Sets capsule parameters (for capsule volumes).
 */
void fog_volume_set_capsule(fog_volume_t* volume, float radius, float height);

/**
 * Activates/deactivates a fog volume.
 */
void fog_volume_set_active(fog_volume_t* volume, bool active);

/**
 * Sets fog volume priority (higher = more important).
 */
void fog_volume_set_priority(fog_volume_t* volume, int32_t priority);

/**
 * Calculates world-space AABB bounds for a fog volume.
 */
void fog_volume_calculate_bounds(fog_volume_t* volume);

/**
 * Tests if a point (in world space) is inside a fog volume.
 * Returns density [0, 1] at that point.
 */
float fog_volume_density_at_point(const fog_volume_t* volume, vec3_t world_pos);

/**
 * Tests if a fog volume intersects with a froxel.
 */
bool fog_volume_intersects_froxel(const fog_volume_t* volume, const froxel_grid_t* grid,
                                   uint32_t x, uint32_t y, uint32_t z, mat4_t view_matrix);

/**
 * Enables/disables global fog.
 */
void fog_volume_manager_set_global_fog(fog_volume_manager_t* manager, bool enabled,
                                        const fog_volume_params_t* params);

/**
 * Updates all fog volume transform matrices if dirty.
 */
void fog_volume_manager_update(fog_volume_manager_t* manager);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_FOG_VOLUME_H */
