/*
 * lod_bias.h
 * LOD bias and forcing
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_LOD_BIAS_H
#define GEOMETRY_LOD_BIAS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_lod_bias_handle {
    uint32_t id;
} geometry_lod_bias_handle_t;

typedef struct geometry_lod_bias_desc {
    uint32_t flags;
    void* user_data;
} geometry_lod_bias_desc_t;

typedef struct geometry_lod_bias_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} geometry_lod_bias_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int geometry_lod_bias_init(void);
void geometry_lod_bias_shutdown(void);

/* Lifecycle */
int geometry_lod_bias_create(geometry_lod_bias_handle_t* out_handle, const geometry_lod_bias_desc_t* desc);
void geometry_lod_bias_destroy(geometry_lod_bias_handle_t handle);

/* Operations */
int geometry_lod_bias_update(geometry_lod_bias_handle_t handle, const void* data, size_t size);
bool geometry_lod_bias_is_valid(geometry_lod_bias_handle_t handle);
int geometry_lod_bias_get_info(geometry_lod_bias_handle_t handle, geometry_lod_bias_info_t* out_info);
void geometry_lod_bias_mark_dirty(geometry_lod_bias_handle_t handle);
int geometry_lod_bias_process_pending(void);

/* Statistics */
uint32_t geometry_lod_bias_get_count(void);
size_t geometry_lod_bias_get_memory_usage(void);
void geometry_lod_bias_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_LOD_BIAS_H */
