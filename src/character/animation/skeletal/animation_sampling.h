/*
 * animation_sampling.h
 * Keyframe sampling
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_ANIMATION_SAMPLING_H
#define ANIMATION_ANIMATION_SAMPLING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_animation_sampling_handle {
    uint32_t id;
} animation_animation_sampling_handle_t;

typedef struct animation_animation_sampling_desc {
    uint32_t flags;
    void* user_data;
} animation_animation_sampling_desc_t;

typedef struct animation_animation_sampling_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} animation_animation_sampling_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_animation_sampling_init(void);
void animation_animation_sampling_shutdown(void);

/* Lifecycle */
int animation_animation_sampling_create(animation_animation_sampling_handle_t* out_handle, const animation_animation_sampling_desc_t* desc);
void animation_animation_sampling_destroy(animation_animation_sampling_handle_t handle);

/* Operations */
int animation_animation_sampling_update(animation_animation_sampling_handle_t handle, const void* data, size_t size);
bool animation_animation_sampling_is_valid(animation_animation_sampling_handle_t handle);
int animation_animation_sampling_get_info(animation_animation_sampling_handle_t handle, animation_animation_sampling_info_t* out_info);
void animation_animation_sampling_mark_dirty(animation_animation_sampling_handle_t handle);
int animation_animation_sampling_process_pending(void);

/* Statistics */
uint32_t animation_animation_sampling_get_count(void);
size_t animation_animation_sampling_get_memory_usage(void);
void animation_animation_sampling_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_ANIMATION_SAMPLING_H */
