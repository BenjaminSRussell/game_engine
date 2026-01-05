/*
 * lod_crossfade.h
 * LOD transition blending
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_LOD_CROSSFADE_H
#define GEOMETRY_LOD_CROSSFADE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_lod_crossfade_handle {
    uint32_t id;
} geometry_lod_crossfade_handle_t;

typedef struct geometry_lod_crossfade_desc {
    uint32_t flags;
    void* user_data;
} geometry_lod_crossfade_desc_t;

typedef struct geometry_lod_crossfade_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} geometry_lod_crossfade_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int geometry_lod_crossfade_init(void);
void geometry_lod_crossfade_shutdown(void);

/* Lifecycle */
int geometry_lod_crossfade_create(geometry_lod_crossfade_handle_t* out_handle, const geometry_lod_crossfade_desc_t* desc);
void geometry_lod_crossfade_destroy(geometry_lod_crossfade_handle_t handle);

/* Operations */
int geometry_lod_crossfade_update(geometry_lod_crossfade_handle_t handle, const void* data, size_t size);
bool geometry_lod_crossfade_is_valid(geometry_lod_crossfade_handle_t handle);
int geometry_lod_crossfade_get_info(geometry_lod_crossfade_handle_t handle, geometry_lod_crossfade_info_t* out_info);
void geometry_lod_crossfade_mark_dirty(geometry_lod_crossfade_handle_t handle);
int geometry_lod_crossfade_process_pending(void);

/* Statistics */
uint32_t geometry_lod_crossfade_get_count(void);
size_t geometry_lod_crossfade_get_memory_usage(void);
void geometry_lod_crossfade_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_LOD_CROSSFADE_H */
