/*
 * two_bone_ik.h
 * Two-bone IK
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_TWO_BONE_IK_H
#define ANIMATION_TWO_BONE_IK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_two_bone_ik_handle {
    uint32_t id;
} animation_two_bone_ik_handle_t;

typedef struct animation_two_bone_ik_desc {
    uint32_t flags;
    void* user_data;
} animation_two_bone_ik_desc_t;

typedef struct animation_two_bone_ik_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} animation_two_bone_ik_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_two_bone_ik_init(void);
void animation_two_bone_ik_shutdown(void);

/* Lifecycle */
int animation_two_bone_ik_create(animation_two_bone_ik_handle_t* out_handle, const animation_two_bone_ik_desc_t* desc);
void animation_two_bone_ik_destroy(animation_two_bone_ik_handle_t handle);

/* Operations */
int animation_two_bone_ik_update(animation_two_bone_ik_handle_t handle, const void* data, size_t size);
bool animation_two_bone_ik_is_valid(animation_two_bone_ik_handle_t handle);
int animation_two_bone_ik_get_info(animation_two_bone_ik_handle_t handle, animation_two_bone_ik_info_t* out_info);
void animation_two_bone_ik_mark_dirty(animation_two_bone_ik_handle_t handle);
int animation_two_bone_ik_process_pending(void);

/* Statistics */
uint32_t animation_two_bone_ik_get_count(void);
size_t animation_two_bone_ik_get_memory_usage(void);
void animation_two_bone_ik_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_TWO_BONE_IK_H */
