/*
 * rt_diffuse_gi.h
 * RT diffuse GI rays
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_RT_DIFFUSE_GI_H
#define RAYTRACING_RT_DIFFUSE_GI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct raytracing_rt_diffuse_gi_handle {
    uint32_t id;
} raytracing_rt_diffuse_gi_handle_t;

typedef struct raytracing_rt_diffuse_gi_desc {
    uint32_t flags;
    void* user_data;
} raytracing_rt_diffuse_gi_desc_t;

typedef struct raytracing_rt_diffuse_gi_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} raytracing_rt_diffuse_gi_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int raytracing_rt_diffuse_gi_init(void);
void raytracing_rt_diffuse_gi_shutdown(void);

/* Lifecycle */
int raytracing_rt_diffuse_gi_create(raytracing_rt_diffuse_gi_handle_t* out_handle, const raytracing_rt_diffuse_gi_desc_t* desc);
void raytracing_rt_diffuse_gi_destroy(raytracing_rt_diffuse_gi_handle_t handle);

/* Operations */
int raytracing_rt_diffuse_gi_update(raytracing_rt_diffuse_gi_handle_t handle, const void* data, size_t size);
bool raytracing_rt_diffuse_gi_is_valid(raytracing_rt_diffuse_gi_handle_t handle);
int raytracing_rt_diffuse_gi_get_info(raytracing_rt_diffuse_gi_handle_t handle, raytracing_rt_diffuse_gi_info_t* out_info);
void raytracing_rt_diffuse_gi_mark_dirty(raytracing_rt_diffuse_gi_handle_t handle);
int raytracing_rt_diffuse_gi_process_pending(void);

/* Statistics */
uint32_t raytracing_rt_diffuse_gi_get_count(void);
size_t raytracing_rt_diffuse_gi_get_memory_usage(void);
void raytracing_rt_diffuse_gi_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_RT_DIFFUSE_GI_H */
