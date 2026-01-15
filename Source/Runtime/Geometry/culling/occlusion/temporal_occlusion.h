/*
 * temporal_occlusion.h
 * Temporal reprojection cull
 *
 * Part of the Culling subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CULLING_TEMPORAL_OCCLUSION_H
#define CULLING_TEMPORAL_OCCLUSION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct culling_temporal_occlusion_handle {
    uint32_t id;
} culling_temporal_occlusion_handle_t;

typedef struct culling_temporal_occlusion_desc {
    uint32_t flags;
    void* user_data;
} culling_temporal_occlusion_desc_t;

typedef struct culling_temporal_occlusion_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} culling_temporal_occlusion_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int culling_temporal_occlusion_init(void);
void culling_temporal_occlusion_shutdown(void);

/* Lifecycle */
int culling_temporal_occlusion_create(culling_temporal_occlusion_handle_t* out_handle, const culling_temporal_occlusion_desc_t* desc);
void culling_temporal_occlusion_destroy(culling_temporal_occlusion_handle_t handle);

/* Operations */
int culling_temporal_occlusion_update(culling_temporal_occlusion_handle_t handle, const void* data, size_t size);
bool culling_temporal_occlusion_is_valid(culling_temporal_occlusion_handle_t handle);
int culling_temporal_occlusion_get_info(culling_temporal_occlusion_handle_t handle, culling_temporal_occlusion_info_t* out_info);
void culling_temporal_occlusion_mark_dirty(culling_temporal_occlusion_handle_t handle);
int culling_temporal_occlusion_process_pending(void);

/* Statistics */
uint32_t culling_temporal_occlusion_get_count(void);
size_t culling_temporal_occlusion_get_memory_usage(void);
void culling_temporal_occlusion_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_TEMPORAL_OCCLUSION_H */
