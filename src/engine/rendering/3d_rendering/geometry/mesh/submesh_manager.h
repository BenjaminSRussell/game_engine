/*
 * submesh_manager.h
 * Sub-mesh and material slots
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_SUBMESH_MANAGER_H
#define GEOMETRY_SUBMESH_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_submesh_manager_handle {
    uint32_t id;
} geometry_submesh_manager_handle_t;

typedef struct geometry_submesh_manager_desc {
    uint32_t flags;
    void* user_data;
} geometry_submesh_manager_desc_t;

typedef struct geometry_submesh_manager_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} geometry_submesh_manager_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int geometry_submesh_manager_init(void);
void geometry_submesh_manager_shutdown(void);

/* Lifecycle */
int geometry_submesh_manager_create(geometry_submesh_manager_handle_t* out_handle, const geometry_submesh_manager_desc_t* desc);
void geometry_submesh_manager_destroy(geometry_submesh_manager_handle_t handle);

/* Operations */
int geometry_submesh_manager_update(geometry_submesh_manager_handle_t handle, const void* data, size_t size);
bool geometry_submesh_manager_is_valid(geometry_submesh_manager_handle_t handle);
int geometry_submesh_manager_get_info(geometry_submesh_manager_handle_t handle, geometry_submesh_manager_info_t* out_info);
void geometry_submesh_manager_mark_dirty(geometry_submesh_manager_handle_t handle);
int geometry_submesh_manager_process_pending(void);

/* Statistics */
uint32_t geometry_submesh_manager_get_count(void);
size_t geometry_submesh_manager_get_memory_usage(void);
void geometry_submesh_manager_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_SUBMESH_MANAGER_H */
