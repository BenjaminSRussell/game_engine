/*
 * blend_masks.h
 * Per-bone blend masks
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_BLEND_MASKS_H
#define ANIMATION_BLEND_MASKS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_blend_masks_handle {
    uint32_t id;
} animation_blend_masks_handle_t;

typedef struct animation_blend_masks_desc {
    uint32_t flags;
    void* user_data;
} animation_blend_masks_desc_t;

typedef struct animation_blend_masks_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} animation_blend_masks_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_blend_masks_init(void);
void animation_blend_masks_shutdown(void);

/* Lifecycle */
int animation_blend_masks_create(animation_blend_masks_handle_t* out_handle, const animation_blend_masks_desc_t* desc);
void animation_blend_masks_destroy(animation_blend_masks_handle_t handle);

/* Operations */
int animation_blend_masks_update(animation_blend_masks_handle_t handle, const void* data, size_t size);
bool animation_blend_masks_is_valid(animation_blend_masks_handle_t handle);
int animation_blend_masks_get_info(animation_blend_masks_handle_t handle, animation_blend_masks_info_t* out_info);
void animation_blend_masks_mark_dirty(animation_blend_masks_handle_t handle);
int animation_blend_masks_process_pending(void);

/* Statistics */
uint32_t animation_blend_masks_get_count(void);
size_t animation_blend_masks_get_memory_usage(void);
void animation_blend_masks_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_BLEND_MASKS_H */
