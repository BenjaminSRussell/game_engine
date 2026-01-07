/*
 * distance_lod.h
 * Distance-based LOD
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_DISTANCE_LOD_H
#define GEOMETRY_DISTANCE_LOD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_distance_lod_handle {
    uint32_t id;
} geometry_distance_lod_handle_t;

typedef struct geometry_distance_lod_desc {
    uint32_t flags;
    void* user_data;
} geometry_distance_lod_desc_t;

typedef struct geometry_distance_lod_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} geometry_distance_lod_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int geometry_distance_lod_init(void);
void geometry_distance_lod_shutdown(void);

/* Lifecycle */
int geometry_distance_lod_create(geometry_distance_lod_handle_t* out_handle, const geometry_distance_lod_desc_t* desc);
void geometry_distance_lod_destroy(geometry_distance_lod_handle_t handle);

/* Operations */
int geometry_distance_lod_update(geometry_distance_lod_handle_t handle, const void* data, size_t size);
bool geometry_distance_lod_is_valid(geometry_distance_lod_handle_t handle);
int geometry_distance_lod_get_info(geometry_distance_lod_handle_t handle, geometry_distance_lod_info_t* out_info);
void geometry_distance_lod_mark_dirty(geometry_distance_lod_handle_t handle);
int geometry_distance_lod_process_pending(void);

/* Statistics */
uint32_t geometry_distance_lod_get_count(void);
size_t geometry_distance_lod_get_memory_usage(void);
void geometry_distance_lod_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_DISTANCE_LOD_H */
