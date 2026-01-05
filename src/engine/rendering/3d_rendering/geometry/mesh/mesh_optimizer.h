/*
 * mesh_optimizer.h
 * Vertex cache optimization
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_MESH_OPTIMIZER_H
#define GEOMETRY_MESH_OPTIMIZER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_mesh_optimizer_handle {
    uint32_t id;
} geometry_mesh_optimizer_handle_t;

typedef struct geometry_mesh_optimizer_desc {
    uint32_t flags;
    void* user_data;
} geometry_mesh_optimizer_desc_t;

typedef struct geometry_mesh_optimizer_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} geometry_mesh_optimizer_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int geometry_mesh_optimizer_init(void);
void geometry_mesh_optimizer_shutdown(void);

/* Lifecycle */
int geometry_mesh_optimizer_create(geometry_mesh_optimizer_handle_t* out_handle, const geometry_mesh_optimizer_desc_t* desc);
void geometry_mesh_optimizer_destroy(geometry_mesh_optimizer_handle_t handle);

/* Operations */
int geometry_mesh_optimizer_update(geometry_mesh_optimizer_handle_t handle, const void* data, size_t size);
bool geometry_mesh_optimizer_is_valid(geometry_mesh_optimizer_handle_t handle);
int geometry_mesh_optimizer_get_info(geometry_mesh_optimizer_handle_t handle, geometry_mesh_optimizer_info_t* out_info);
void geometry_mesh_optimizer_mark_dirty(geometry_mesh_optimizer_handle_t handle);
int geometry_mesh_optimizer_process_pending(void);

/* Statistics */
uint32_t geometry_mesh_optimizer_get_count(void);
size_t geometry_mesh_optimizer_get_memory_usage(void);
void geometry_mesh_optimizer_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_MESH_OPTIMIZER_H */
