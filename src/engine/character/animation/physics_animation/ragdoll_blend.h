/*
 * ragdoll_blend.h
 * Ragdoll-animation blending
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_RAGDOLL_BLEND_H
#define ANIMATION_RAGDOLL_BLEND_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_ragdoll_blend_handle {
    uint32_t id;
} animation_ragdoll_blend_handle_t;

typedef struct animation_ragdoll_blend_desc {
    uint32_t flags;
    void* user_data;
} animation_ragdoll_blend_desc_t;

typedef struct animation_ragdoll_blend_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} animation_ragdoll_blend_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_ragdoll_blend_init(void);
void animation_ragdoll_blend_shutdown(void);

/* Lifecycle */
int animation_ragdoll_blend_create(animation_ragdoll_blend_handle_t* out_handle, const animation_ragdoll_blend_desc_t* desc);
void animation_ragdoll_blend_destroy(animation_ragdoll_blend_handle_t handle);

/* Operations */
int animation_ragdoll_blend_update(animation_ragdoll_blend_handle_t handle, const void* data, size_t size);
bool animation_ragdoll_blend_is_valid(animation_ragdoll_blend_handle_t handle);
int animation_ragdoll_blend_get_info(animation_ragdoll_blend_handle_t handle, animation_ragdoll_blend_info_t* out_info);
void animation_ragdoll_blend_mark_dirty(animation_ragdoll_blend_handle_t handle);
int animation_ragdoll_blend_process_pending(void);

/* Statistics */
uint32_t animation_ragdoll_blend_get_count(void);
size_t animation_ragdoll_blend_get_memory_usage(void);
void animation_ragdoll_blend_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_RAGDOLL_BLEND_H */
