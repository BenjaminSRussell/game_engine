/*
 * resource_viewer.h
 * Resource viewer
 *
 * Part of the Profiling subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PROFILING_RESOURCE_VIEWER_H
#define PROFILING_RESOURCE_VIEWER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct profiling_resource_viewer_handle {
    uint32_t id;
} profiling_resource_viewer_handle_t;

typedef struct profiling_resource_viewer_desc {
    uint32_t flags;
    void* user_data;
} profiling_resource_viewer_desc_t;

typedef struct profiling_resource_viewer_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} profiling_resource_viewer_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int profiling_resource_viewer_init(void);
void profiling_resource_viewer_shutdown(void);

/* Lifecycle */
int profiling_resource_viewer_create(profiling_resource_viewer_handle_t* out_handle, const profiling_resource_viewer_desc_t* desc);
void profiling_resource_viewer_destroy(profiling_resource_viewer_handle_t handle);

/* Operations */
int profiling_resource_viewer_update(profiling_resource_viewer_handle_t handle, const void* data, size_t size);
bool profiling_resource_viewer_is_valid(profiling_resource_viewer_handle_t handle);
int profiling_resource_viewer_get_info(profiling_resource_viewer_handle_t handle, profiling_resource_viewer_info_t* out_info);
void profiling_resource_viewer_mark_dirty(profiling_resource_viewer_handle_t handle);
int profiling_resource_viewer_process_pending(void);

/* Statistics */
uint32_t profiling_resource_viewer_get_count(void);
size_t profiling_resource_viewer_get_memory_usage(void);
void profiling_resource_viewer_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* PROFILING_RESOURCE_VIEWER_H */
