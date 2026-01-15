/*
 * rt_shadow_denoise.h
 * RT shadow denoising
 *
 * Part of the Shadows Advanced subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef SHADOWS_ADVANCED_RT_SHADOW_DENOISE_H
#define SHADOWS_ADVANCED_RT_SHADOW_DENOISE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct shadows_advanced_rt_shadow_denoise_handle {
    uint32_t id;
} shadows_advanced_rt_shadow_denoise_handle_t;

typedef struct shadows_advanced_rt_shadow_denoise_desc {
    uint32_t flags;
    void* user_data;
} shadows_advanced_rt_shadow_denoise_desc_t;

typedef struct shadows_advanced_rt_shadow_denoise_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} shadows_advanced_rt_shadow_denoise_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int shadows_advanced_rt_shadow_denoise_init(void);
void shadows_advanced_rt_shadow_denoise_shutdown(void);

/* Lifecycle */
int shadows_advanced_rt_shadow_denoise_create(shadows_advanced_rt_shadow_denoise_handle_t* out_handle, const shadows_advanced_rt_shadow_denoise_desc_t* desc);
void shadows_advanced_rt_shadow_denoise_destroy(shadows_advanced_rt_shadow_denoise_handle_t handle);

/* Operations */
int shadows_advanced_rt_shadow_denoise_update(shadows_advanced_rt_shadow_denoise_handle_t handle, const void* data, size_t size);
bool shadows_advanced_rt_shadow_denoise_is_valid(shadows_advanced_rt_shadow_denoise_handle_t handle);
int shadows_advanced_rt_shadow_denoise_get_info(shadows_advanced_rt_shadow_denoise_handle_t handle, shadows_advanced_rt_shadow_denoise_info_t* out_info);
void shadows_advanced_rt_shadow_denoise_mark_dirty(shadows_advanced_rt_shadow_denoise_handle_t handle);
int shadows_advanced_rt_shadow_denoise_process_pending(void);

/* Statistics */
uint32_t shadows_advanced_rt_shadow_denoise_get_count(void);
size_t shadows_advanced_rt_shadow_denoise_get_memory_usage(void);
void shadows_advanced_rt_shadow_denoise_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADOWS_ADVANCED_RT_SHADOW_DENOISE_H */
