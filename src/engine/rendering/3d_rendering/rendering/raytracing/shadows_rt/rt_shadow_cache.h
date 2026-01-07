/*
 * rt_shadow_cache.h
 * Shadow ray caching
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_RT_SHADOW_CACHE_H
#define RAYTRACING_RT_SHADOW_CACHE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct raytracing_rt_shadow_cache_handle {
    uint32_t id;
} raytracing_rt_shadow_cache_handle_t;

typedef struct raytracing_rt_shadow_cache_desc {
    uint32_t flags;
    void* user_data;
} raytracing_rt_shadow_cache_desc_t;

typedef struct raytracing_rt_shadow_cache_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} raytracing_rt_shadow_cache_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int raytracing_rt_shadow_cache_init(void);
void raytracing_rt_shadow_cache_shutdown(void);

/* Lifecycle */
int raytracing_rt_shadow_cache_create(raytracing_rt_shadow_cache_handle_t* out_handle, const raytracing_rt_shadow_cache_desc_t* desc);
void raytracing_rt_shadow_cache_destroy(raytracing_rt_shadow_cache_handle_t handle);

/* Operations */
int raytracing_rt_shadow_cache_update(raytracing_rt_shadow_cache_handle_t handle, const void* data, size_t size);
bool raytracing_rt_shadow_cache_is_valid(raytracing_rt_shadow_cache_handle_t handle);
int raytracing_rt_shadow_cache_get_info(raytracing_rt_shadow_cache_handle_t handle, raytracing_rt_shadow_cache_info_t* out_info);
void raytracing_rt_shadow_cache_mark_dirty(raytracing_rt_shadow_cache_handle_t handle);
int raytracing_rt_shadow_cache_process_pending(void);

/* Statistics */
uint32_t raytracing_rt_shadow_cache_get_count(void);
size_t raytracing_rt_shadow_cache_get_memory_usage(void);
void raytracing_rt_shadow_cache_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_RT_SHADOW_CACHE_H */
