/*
 * cloud_animation.h
 * Cloud movement/animation
 *
 * Part of the Atmosphere subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ATMOSPHERE_CLOUD_ANIMATION_H
#define ATMOSPHERE_CLOUD_ANIMATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct atmosphere_cloud_animation_handle {
    uint32_t id;
} atmosphere_cloud_animation_handle_t;

typedef struct atmosphere_cloud_animation_desc {
    uint32_t flags;
    void* user_data;
} atmosphere_cloud_animation_desc_t;

typedef struct atmosphere_cloud_animation_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} atmosphere_cloud_animation_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int atmosphere_cloud_animation_init(void);
void atmosphere_cloud_animation_shutdown(void);

/* Lifecycle */
int atmosphere_cloud_animation_create(atmosphere_cloud_animation_handle_t* out_handle, const atmosphere_cloud_animation_desc_t* desc);
void atmosphere_cloud_animation_destroy(atmosphere_cloud_animation_handle_t handle);

/* Operations */
int atmosphere_cloud_animation_update(atmosphere_cloud_animation_handle_t handle, const void* data, size_t size);
bool atmosphere_cloud_animation_is_valid(atmosphere_cloud_animation_handle_t handle);
int atmosphere_cloud_animation_get_info(atmosphere_cloud_animation_handle_t handle, atmosphere_cloud_animation_info_t* out_info);
void atmosphere_cloud_animation_mark_dirty(atmosphere_cloud_animation_handle_t handle);
int atmosphere_cloud_animation_process_pending(void);

/* Statistics */
uint32_t atmosphere_cloud_animation_get_count(void);
size_t atmosphere_cloud_animation_get_memory_usage(void);
void atmosphere_cloud_animation_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ATMOSPHERE_CLOUD_ANIMATION_H */
