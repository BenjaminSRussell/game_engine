/*
 * blend_tree.h
 * Animation blend tree
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_BLEND_TREE_H
#define ANIMATION_BLEND_TREE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_blend_tree_handle {
    uint32_t id;
} animation_blend_tree_handle_t;

typedef struct animation_blend_tree_desc {
    uint32_t flags;
    void* user_data;
} animation_blend_tree_desc_t;

typedef struct animation_blend_tree_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} animation_blend_tree_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_blend_tree_init(void);
void animation_blend_tree_shutdown(void);

/* Lifecycle */
int animation_blend_tree_create(animation_blend_tree_handle_t* out_handle, const animation_blend_tree_desc_t* desc);
void animation_blend_tree_destroy(animation_blend_tree_handle_t handle);

/* Operations */
int animation_blend_tree_update(animation_blend_tree_handle_t handle, const void* data, size_t size);
bool animation_blend_tree_is_valid(animation_blend_tree_handle_t handle);
int animation_blend_tree_get_info(animation_blend_tree_handle_t handle, animation_blend_tree_info_t* out_info);
void animation_blend_tree_mark_dirty(animation_blend_tree_handle_t handle);
int animation_blend_tree_process_pending(void);

/* Statistics */
uint32_t animation_blend_tree_get_count(void);
size_t animation_blend_tree_get_memory_usage(void);
void animation_blend_tree_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_BLEND_TREE_H */
