/*
 * additive_blending.h
 * Additive animation layers
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_ADDITIVE_BLENDING_H
#define ANIMATION_ADDITIVE_BLENDING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_additive_blending_handle {
    uint32_t id;
} animation_additive_blending_handle_t;

typedef struct animation_additive_blending_desc {
    uint32_t flags;
    void* user_data;
} animation_additive_blending_desc_t;

typedef struct animation_additive_blending_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} animation_additive_blending_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_additive_blending_init(void);
void animation_additive_blending_shutdown(void);

/* Lifecycle */
int animation_additive_blending_create(animation_additive_blending_handle_t* out_handle, const animation_additive_blending_desc_t* desc);
void animation_additive_blending_destroy(animation_additive_blending_handle_t handle);

/* Operations */
int animation_additive_blending_update(animation_additive_blending_handle_t handle, const void* data, size_t size);
bool animation_additive_blending_is_valid(animation_additive_blending_handle_t handle);
int animation_additive_blending_get_info(animation_additive_blending_handle_t handle, animation_additive_blending_info_t* out_info);
void animation_additive_blending_mark_dirty(animation_additive_blending_handle_t handle);
int animation_additive_blending_process_pending(void);

/* Statistics */
uint32_t animation_additive_blending_get_count(void);
size_t animation_additive_blending_get_memory_usage(void);
void animation_additive_blending_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_ADDITIVE_BLENDING_H */
