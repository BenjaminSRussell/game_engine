/*
 * instance_lod.h
 * Per-instance LOD selection
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_INSTANCE_LOD_H
#define GEOMETRY_INSTANCE_LOD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_instance_lod_handle {
    uint32_t id;
} geometry_instance_lod_handle_t;

typedef struct geometry_instance_lod_desc {
    uint32_t flags;
    void* user_data;
} geometry_instance_lod_desc_t;

typedef struct geometry_instance_lod_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} geometry_instance_lod_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int geometry_instance_lod_init(void);
void geometry_instance_lod_shutdown(void);

/* Lifecycle */
int geometry_instance_lod_create(geometry_instance_lod_handle_t* out_handle, const geometry_instance_lod_desc_t* desc);
void geometry_instance_lod_destroy(geometry_instance_lod_handle_t handle);

/* Operations */
int geometry_instance_lod_update(geometry_instance_lod_handle_t handle, const void* data, size_t size);
bool geometry_instance_lod_is_valid(geometry_instance_lod_handle_t handle);
int geometry_instance_lod_get_info(geometry_instance_lod_handle_t handle, geometry_instance_lod_info_t* out_info);
void geometry_instance_lod_mark_dirty(geometry_instance_lod_handle_t handle);
int geometry_instance_lod_process_pending(void);

/* Statistics */
uint32_t geometry_instance_lod_get_count(void);
size_t geometry_instance_lod_get_memory_usage(void);
void geometry_instance_lod_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_INSTANCE_LOD_H */
