/*
 * rt_reflection_fallback.h
 * RT-SSR hybrid
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_RT_REFLECTION_FALLBACK_H
#define RAYTRACING_RT_REFLECTION_FALLBACK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct raytracing_rt_reflection_fallback_handle {
    uint32_t id;
} raytracing_rt_reflection_fallback_handle_t;

typedef struct raytracing_rt_reflection_fallback_desc {
    uint32_t flags;
    void* user_data;
} raytracing_rt_reflection_fallback_desc_t;

typedef struct raytracing_rt_reflection_fallback_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} raytracing_rt_reflection_fallback_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int raytracing_rt_reflection_fallback_init(void);
void raytracing_rt_reflection_fallback_shutdown(void);

/* Lifecycle */
int raytracing_rt_reflection_fallback_create(raytracing_rt_reflection_fallback_handle_t* out_handle, const raytracing_rt_reflection_fallback_desc_t* desc);
void raytracing_rt_reflection_fallback_destroy(raytracing_rt_reflection_fallback_handle_t handle);

/* Operations */
int raytracing_rt_reflection_fallback_update(raytracing_rt_reflection_fallback_handle_t handle, const void* data, size_t size);
bool raytracing_rt_reflection_fallback_is_valid(raytracing_rt_reflection_fallback_handle_t handle);
int raytracing_rt_reflection_fallback_get_info(raytracing_rt_reflection_fallback_handle_t handle, raytracing_rt_reflection_fallback_info_t* out_info);
void raytracing_rt_reflection_fallback_mark_dirty(raytracing_rt_reflection_fallback_handle_t handle);
int raytracing_rt_reflection_fallback_process_pending(void);

/* Statistics */
uint32_t raytracing_rt_reflection_fallback_get_count(void);
size_t raytracing_rt_reflection_fallback_get_memory_usage(void);
void raytracing_rt_reflection_fallback_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_RT_REFLECTION_FALLBACK_H */
