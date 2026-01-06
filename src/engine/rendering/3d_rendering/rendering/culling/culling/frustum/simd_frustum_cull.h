/*
 * simd_frustum_cull.h
 * SIMD batch frustum cull
 *
 * Part of the Culling subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CULLING_SIMD_FRUSTUM_CULL_H
#define CULLING_SIMD_FRUSTUM_CULL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct culling_simd_frustum_cull_handle {
    uint32_t id;
} culling_simd_frustum_cull_handle_t;

typedef struct culling_simd_frustum_cull_desc {
    uint32_t flags;
    void* user_data;
} culling_simd_frustum_cull_desc_t;

typedef struct culling_simd_frustum_cull_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} culling_simd_frustum_cull_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int culling_simd_frustum_cull_init(void);
void culling_simd_frustum_cull_shutdown(void);

/* Lifecycle */
int culling_simd_frustum_cull_create(culling_simd_frustum_cull_handle_t* out_handle, const culling_simd_frustum_cull_desc_t* desc);
void culling_simd_frustum_cull_destroy(culling_simd_frustum_cull_handle_t handle);

/* Operations */
int culling_simd_frustum_cull_update(culling_simd_frustum_cull_handle_t handle, const void* data, size_t size);
bool culling_simd_frustum_cull_is_valid(culling_simd_frustum_cull_handle_t handle);
int culling_simd_frustum_cull_get_info(culling_simd_frustum_cull_handle_t handle, culling_simd_frustum_cull_info_t* out_info);
void culling_simd_frustum_cull_mark_dirty(culling_simd_frustum_cull_handle_t handle);
int culling_simd_frustum_cull_process_pending(void);

/* Statistics */
uint32_t culling_simd_frustum_cull_get_count(void);
size_t culling_simd_frustum_cull_get_memory_usage(void);
void culling_simd_frustum_cull_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_SIMD_FRUSTUM_CULL_H */
