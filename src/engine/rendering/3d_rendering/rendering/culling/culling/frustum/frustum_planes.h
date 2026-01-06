/*
 * frustum_planes.h
 * Frustum plane extraction
 *
 * Part of the Culling subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CULLING_FRUSTUM_PLANES_H
#define CULLING_FRUSTUM_PLANES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct culling_frustum_planes_handle {
    uint32_t id;
} culling_frustum_planes_handle_t;

typedef struct culling_frustum_planes_desc {
    uint32_t flags;
    void* user_data;
} culling_frustum_planes_desc_t;

typedef struct culling_frustum_planes_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} culling_frustum_planes_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int culling_frustum_planes_init(void);
void culling_frustum_planes_shutdown(void);

/* Lifecycle */
int culling_frustum_planes_create(culling_frustum_planes_handle_t* out_handle, const culling_frustum_planes_desc_t* desc);
void culling_frustum_planes_destroy(culling_frustum_planes_handle_t handle);

/* Operations */
int culling_frustum_planes_update(culling_frustum_planes_handle_t handle, const void* data, size_t size);
bool culling_frustum_planes_is_valid(culling_frustum_planes_handle_t handle);
int culling_frustum_planes_get_info(culling_frustum_planes_handle_t handle, culling_frustum_planes_info_t* out_info);
void culling_frustum_planes_mark_dirty(culling_frustum_planes_handle_t handle);
int culling_frustum_planes_process_pending(void);

/* Statistics */
uint32_t culling_frustum_planes_get_count(void);
size_t culling_frustum_planes_get_memory_usage(void);
void culling_frustum_planes_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_FRUSTUM_PLANES_H */
