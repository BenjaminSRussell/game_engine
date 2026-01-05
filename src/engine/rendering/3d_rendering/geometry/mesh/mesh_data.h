/*
 * mesh_data.h
 * Mesh vertex/index data storage
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_MESH_DATA_H
#define GEOMETRY_MESH_DATA_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_mesh_data_handle {
    uint32_t id;
} geometry_mesh_data_handle_t;

typedef struct geometry_mesh_data_desc {
    uint32_t flags;
    void* user_data;
} geometry_mesh_data_desc_t;

typedef struct geometry_mesh_data_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} geometry_mesh_data_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int geometry_mesh_data_init(void);
void geometry_mesh_data_shutdown(void);

/* Lifecycle */
int geometry_mesh_data_create(geometry_mesh_data_handle_t* out_handle, const geometry_mesh_data_desc_t* desc);
void geometry_mesh_data_destroy(geometry_mesh_data_handle_t handle);

/* Operations */
int geometry_mesh_data_update(geometry_mesh_data_handle_t handle, const void* data, size_t size);
bool geometry_mesh_data_is_valid(geometry_mesh_data_handle_t handle);
int geometry_mesh_data_get_info(geometry_mesh_data_handle_t handle, geometry_mesh_data_info_t* out_info);
void geometry_mesh_data_mark_dirty(geometry_mesh_data_handle_t handle);
int geometry_mesh_data_process_pending(void);

/* Statistics */
uint32_t geometry_mesh_data_get_count(void);
size_t geometry_mesh_data_get_memory_usage(void);
void geometry_mesh_data_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_MESH_DATA_H */
