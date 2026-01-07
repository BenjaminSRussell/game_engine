/*
 * meshlet_renderer.h
 * Mesh shader meshlet rendering
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_MESHLET_RENDERER_H
#define GEOMETRY_MESHLET_RENDERER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_meshlet_renderer_handle {
    uint32_t id;
} geometry_meshlet_renderer_handle_t;

typedef struct geometry_meshlet_renderer_desc {
    uint32_t flags;
    void* user_data;
} geometry_meshlet_renderer_desc_t;

typedef struct geometry_meshlet_renderer_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} geometry_meshlet_renderer_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int geometry_meshlet_renderer_init(void);
void geometry_meshlet_renderer_shutdown(void);

/* Lifecycle */
int geometry_meshlet_renderer_create(geometry_meshlet_renderer_handle_t* out_handle, const geometry_meshlet_renderer_desc_t* desc);
void geometry_meshlet_renderer_destroy(geometry_meshlet_renderer_handle_t handle);

/* Operations */
int geometry_meshlet_renderer_update(geometry_meshlet_renderer_handle_t handle, const void* data, size_t size);
bool geometry_meshlet_renderer_is_valid(geometry_meshlet_renderer_handle_t handle);
int geometry_meshlet_renderer_get_info(geometry_meshlet_renderer_handle_t handle, geometry_meshlet_renderer_info_t* out_info);
void geometry_meshlet_renderer_mark_dirty(geometry_meshlet_renderer_handle_t handle);
int geometry_meshlet_renderer_process_pending(void);

/* Statistics */
uint32_t geometry_meshlet_renderer_get_count(void);
size_t geometry_meshlet_renderer_get_memory_usage(void);
void geometry_meshlet_renderer_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_MESHLET_RENDERER_H */
