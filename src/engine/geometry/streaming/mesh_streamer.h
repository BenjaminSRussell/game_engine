/*
 * mesh_streamer.h
 * Async mesh data streaming
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_MESH_STREAMER_H
#define GEOMETRY_MESH_STREAMER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_mesh_streamer_handle {
    uint32_t id;
} geometry_mesh_streamer_handle_t;

typedef struct geometry_mesh_streamer_desc {
    uint32_t flags;
    void* user_data;
} geometry_mesh_streamer_desc_t;

typedef struct geometry_mesh_streamer_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} geometry_mesh_streamer_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int geometry_mesh_streamer_init(void);
void geometry_mesh_streamer_shutdown(void);

/* Lifecycle */
int geometry_mesh_streamer_create(geometry_mesh_streamer_handle_t* out_handle, const geometry_mesh_streamer_desc_t* desc);
void geometry_mesh_streamer_destroy(geometry_mesh_streamer_handle_t handle);

/* Operations */
int geometry_mesh_streamer_update(geometry_mesh_streamer_handle_t handle, const void* data, size_t size);
bool geometry_mesh_streamer_is_valid(geometry_mesh_streamer_handle_t handle);
int geometry_mesh_streamer_get_info(geometry_mesh_streamer_handle_t handle, geometry_mesh_streamer_info_t* out_info);
void geometry_mesh_streamer_mark_dirty(geometry_mesh_streamer_handle_t handle);
int geometry_mesh_streamer_process_pending(void);

/* Statistics */
uint32_t geometry_mesh_streamer_get_count(void);
size_t geometry_mesh_streamer_get_memory_usage(void);
void geometry_mesh_streamer_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_MESH_STREAMER_H */
