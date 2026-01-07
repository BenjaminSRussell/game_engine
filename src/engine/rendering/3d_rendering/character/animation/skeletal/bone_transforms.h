/*
 * bone_transforms.h
 * Bone transform computation
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_BONE_TRANSFORMS_H
#define ANIMATION_BONE_TRANSFORMS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_bone_transforms_handle {
    uint32_t id;
} animation_bone_transforms_handle_t;

typedef struct animation_bone_transforms_desc {
    uint32_t flags;
    void* user_data;
} animation_bone_transforms_desc_t;

typedef struct animation_bone_transforms_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} animation_bone_transforms_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_bone_transforms_init(void);
void animation_bone_transforms_shutdown(void);

/* Lifecycle */
int animation_bone_transforms_create(animation_bone_transforms_handle_t* out_handle, const animation_bone_transforms_desc_t* desc);
void animation_bone_transforms_destroy(animation_bone_transforms_handle_t handle);

/* Operations */
int animation_bone_transforms_update(animation_bone_transforms_handle_t handle, const void* data, size_t size);
bool animation_bone_transforms_is_valid(animation_bone_transforms_handle_t handle);
int animation_bone_transforms_get_info(animation_bone_transforms_handle_t handle, animation_bone_transforms_info_t* out_info);
void animation_bone_transforms_mark_dirty(animation_bone_transforms_handle_t handle);
int animation_bone_transforms_process_pending(void);

/* Statistics */
uint32_t animation_bone_transforms_get_count(void);
size_t animation_bone_transforms_get_memory_usage(void);
void animation_bone_transforms_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_BONE_TRANSFORMS_H */
