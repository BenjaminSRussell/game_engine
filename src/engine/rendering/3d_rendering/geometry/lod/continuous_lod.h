/*
 * continuous_lod.h
 * Continuous LOD (Geomorphing) System
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_CONTINUOUS_LOD_H
#define GEOMETRY_CONTINUOUS_LOD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_continuous_lod_handle {
    uint32_t id;
} geometry_continuous_lod_handle_t;

typedef struct geometry_continuous_lod_desc {
    uint32_t flags;
    float transition_speed;  // Speed of LOD transitions (units per second)
    void* user_data;
} geometry_continuous_lod_desc_t;

typedef struct geometry_continuous_lod_info {
    uint32_t id;
    uint32_t flags;
    float blend_factor;      // Current blend factor (0.0 - 1.0)
    uint32_t current_lod;    // Current LOD level
    uint32_t target_lod;     // Target LOD level
    bool transitioning;      // Whether currently transitioning
    bool initialized;
} geometry_continuous_lod_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int geometry_continuous_lod_init(void);
void geometry_continuous_lod_shutdown(void);

/* Lifecycle */
int geometry_continuous_lod_create(geometry_continuous_lod_handle_t* out_handle, const geometry_continuous_lod_desc_t* desc);
void geometry_continuous_lod_destroy(geometry_continuous_lod_handle_t handle);

/* Operations */
int geometry_continuous_lod_update(geometry_continuous_lod_handle_t handle, float distance, const float* lod_distances, uint32_t lod_count, float delta_time);
float geometry_continuous_lod_get_blend_factor(geometry_continuous_lod_handle_t handle);
uint32_t geometry_continuous_lod_get_current_lod(geometry_continuous_lod_handle_t handle);
bool geometry_continuous_lod_is_transitioning(geometry_continuous_lod_handle_t handle);
bool geometry_continuous_lod_is_valid(geometry_continuous_lod_handle_t handle);
int geometry_continuous_lod_get_info(geometry_continuous_lod_handle_t handle, geometry_continuous_lod_info_t* out_info);

/* Statistics */
uint32_t geometry_continuous_lod_get_count(void);
size_t geometry_continuous_lod_get_memory_usage(void);
void geometry_continuous_lod_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_CONTINUOUS_LOD_H */
