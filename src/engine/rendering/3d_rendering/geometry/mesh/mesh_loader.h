/*
 * mesh_loader.h
 * Mesh file format loading
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_MESH_LOADER_H
#define GEOMETRY_MESH_LOADER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_mesh_loader_handle {
    uint32_t id;
} geometry_mesh_loader_handle_t;

typedef struct geometry_mesh_loader_desc {
    uint32_t flags;
    void* user_data;
} geometry_mesh_loader_desc_t;

typedef struct geometry_mesh_loader_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} geometry_mesh_loader_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int geometry_mesh_loader_init(void);
void geometry_mesh_loader_shutdown(void);

/* Lifecycle */
int geometry_mesh_loader_create(geometry_mesh_loader_handle_t* out_handle, const geometry_mesh_loader_desc_t* desc);
void geometry_mesh_loader_destroy(geometry_mesh_loader_handle_t handle);

/* Operations */
int geometry_mesh_loader_update(geometry_mesh_loader_handle_t handle, const void* data, size_t size);
bool geometry_mesh_loader_is_valid(geometry_mesh_loader_handle_t handle);
int geometry_mesh_loader_get_info(geometry_mesh_loader_handle_t handle, geometry_mesh_loader_info_t* out_info);
void geometry_mesh_loader_mark_dirty(geometry_mesh_loader_handle_t handle);
int geometry_mesh_loader_process_pending(void);

/* Statistics */
uint32_t geometry_mesh_loader_get_count(void);
size_t geometry_mesh_loader_get_memory_usage(void);
void geometry_mesh_loader_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_MESH_LOADER_H */
